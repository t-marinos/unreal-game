#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/CoopRoleTypes.h"
#include "CoopGameMode.generated.h"

class UGameConstants;
class ACoopPlayerState;

// Server-authoritative game mode for the prototype's 5-player coop sessions.
// GameModeBase only ever exists on the server (or the server side of a Listen Server),
// so every function here is implicitly authoritative per CLAUDE.md §4.1 -- there is no
// separate HasAuthority() check to make.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACoopGameMode();

	// M9: lets one person iterate without four friends online (CLAUDE.md §7's dev mode). Toggle in
	// the Editor on BP_GameMode's Class Defaults, or via -devmode on the command line for a
	// packaged build (checked in BeginPlay below).
	UPROPERTY(EditAnywhere, Category = "Dev Mode")
	bool bDevMode = false;

	// Build 1, M3: server-authoritative role claim. Called by
	// ACoopPlayerController::Server_ClaimRole's RPC handler once the RPC itself has resolved which
	// PlayerState is asking. Rejects silently (no state change) if DesiredRole is already held by a
	// different PlayerState -- same "intent that might be rejected" shape as PreLogin's cap check.
	void TryClaimRole(ACoopPlayerState* RequestingPlayerState, EPlayerRole DesiredRole);

protected:
	// Rejects a 6th connecting player once 5 are already present, per CLAUDE.md §4.7.
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	// Frees the slot PreLogin reserved below when a player disconnects.
	virtual void Logout(AController* Exiting) override;

	// Build 1, M3: this is where GameState->PlayerArray actually reflects a new real join (unlike
	// PreLogin, which only reserves a connection slot before any PlayerState exists yet).
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void BeginPlay() override;

private:
	// M9: fills every empty player slot (up to MaxPlayers) with an ADummyAIController-possessed
	// ACoopCharacter, so a solo dev client can test a full 5-player session alone. Deliberately a
	// one-shot fill at session start, not a live slot-tracking system -- CLAUDE.md §4.8, this is as
	// much as "iterate alone" actually needs for Build 0.
	void FillEmptySlotsWithDummies();

	// Build 1, M3: fires exactly once, the moment GameState->PlayerArray first reaches MaxPlayers --
	// from either PostLogin (real joins trickling in) or FillEmptySlotsWithDummies (dev mode can
	// reach a full roster in one BeginPlay-time burst, with no PostLogin call for the dummy-filled
	// slots). Starts the RoleSelect timeout. M4 additionally transitions GameState's match phase
	// here once that phase enum exists.
	void OnRosterComplete();

	// Build 1, M3: after a successful claim, checks whether every *real* player (an actual
	// APlayerController, found via the world's player controller iterator -- deliberately excludes
	// dev-mode dummies, which never claim) now holds a non-Unassigned role. If so, resolves role
	// selection immediately rather than waiting out the full timeout.
	void CheckAllRealPlayersClaimed();

	// Build 1, M3: fires on RoleSelectDurationSeconds timeout, or early via
	// CheckAllRealPlayersClaimed(). Auto-assigns a random remaining role to every PlayerState still
	// Unassigned at that point (an AFK real player, or any dev-mode dummy, which never claims one
	// itself) -- see DECISIONS.md's "Role assignment is player-chosen, not random" entry for why
	// this fallback exists instead of blocking indefinitely on one player.
	void ResolveRoleSelection();

	// Build 1, M4: fires when PrepArenaDurationSeconds expires, moving GameState from Prep to
	// HoldTheGate. Actual scene setup lands in M10 -- this milestone only wires the phase value.
	void OnPrepPhaseExpired();

	bool bRosterComplete = false;
	bool bRoleSelectionResolved = false;
	FTimerHandle RoleSelectTimerHandle;
	FTimerHandle PrepPhaseTimerHandle;

	// M10: reserved the instant a connection is *accepted* in PreLogin, not read from
	// GameState->PlayerArray.Num() (which only grows later, in PostLogin/InitNewPlayer). Found via
	// M10's full regression pass: when several connections arrive in the same frame (every PIE
	// client does, and real friends could too), each one's PreLogin call would see the same
	// still-too-low PlayerArray count, since none of them had been added yet -- so all of them
	// passed the cap check simultaneously. This counter is incremented/checked atomically inside
	// PreLogin itself instead, and decremented in Logout.
	// Starts at 1, not 0: confirmed via diagnostic logging that PreLogin is only ever called for
	// *remote* connections -- the Listen Server's own local host player never calls it, but is
	// always exactly one of the players (CLAUDE.md §3: this project is Listen-Server-only, never a
	// dedicated server, so there is always exactly one host-player present from world start).
	int32 AcceptedPlayerCount = 1;

	// Every tunable lives in DA_GameConstants per CLAUDE.md §10. Assigned as content wiring on
	// BP_GameMode (the Blueprint subclass that's the project's actual GlobalDefaultGameMode), not
	// hardcoded here. Left unset on the base C++ class on purpose. EditDefaultsOnly (no
	// BlueprintReadOnly -- UHT rejects that combination on a private member) is enough to expose
	// it on BP_GameMode's Class Defaults panel; nothing needs to read it from a Blueprint graph.
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;

	// Only used if GameConstants is somehow unassigned, so a missing data asset fails loud (a log
	// warning) instead of silently rejecting every 6th player forever.
	static constexpr int32 FallbackMaxPlayers = 5;

	// Same fallback-with-warning treatment as FallbackMaxPlayers above, for RoleSelectDurationSeconds.
	static constexpr float FallbackRoleSelectDurationSeconds = 30.0f;

	// Same fallback-with-warning treatment, for PrepArenaDurationSeconds.
	static constexpr float FallbackPrepArenaDurationSeconds = 60.0f;
};
