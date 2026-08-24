#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CoopGameState.generated.h"

class UGameConstants;

// Holds the one shared, replicated source of truth for "when did the match start" (CLAUDE.md §4.5:
// all timing derives from server time, never client DeltaTime). Everything else (UI countdowns,
// telegraph windows) should read GetElapsedMatchTime() rather than deriving its own clock.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	// Seconds since match start, derived from AGameStateBase::GetServerWorldTimeSeconds() (which
	// already corrects for client/server clock offset) minus the replicated start timestamp --
	// never accumulated per-frame DeltaTime, per CLAUDE.md §4.5. Returns 0 before the match starts.
	UFUNCTION(BlueprintPure, Category = "Match")
	float GetElapsedMatchTime() const;

	// M7: one shared cosmetic effect, proving the Server RPC + replication plumbing (CLAUDE.md §7).
	// Every client's cosmetic response (ACoopButton::Tick) reads this rather than reacting to the
	// RPC or the overlap event directly, per CLAUDE.md §4.1.
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsButtonPressed() const { return bButtonPressed; }

	// Server-only. Called by ACoopPlayerController::Server_PressButton's RPC handler.
	void ToggleButtonPressed();

protected:
	virtual void BeginPlay() override;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Set exactly once, on the server, in BeginPlay (CLAUDE.md §4.5). Replicates to every client as
	// a single value; each client derives elapsed time locally via GetElapsedMatchTime() rather than
	// the server pushing a continuously-updating countdown. VisibleAnywhere (not BlueprintReadOnly --
	// UHT rejects that combination on a private member, same rule hit in M3) so it's directly
	// inspectable (CLAUDE.md §4.3: state must always be printable) -- a bare Replicated-only
	// property isn't reachable through unreal-mcp's reflection tools at all.
	// -1 (never a legitimate world time) means "not set yet" -- NOT 0.0f, which
	// GetServerWorldTimeSeconds() can legitimately return if BeginPlay runs at world start.
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Match")
	float MatchStartServerTime = -1.0f;

	// M7: toggled server-side by ToggleButtonPressed(); replicates to every client, who each read
	// it (via IsButtonPressed()) as the sole source of truth for the button's cosmetic state.
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Interaction")
	bool bButtonPressed = false;

	// Every tunable lives in DA_GameConstants per CLAUDE.md §10. This class has no Blueprint
	// wrapper of its own to hold this reference the way BP_GameMode/BP_PlayerController do -- see
	// BP_GameState (content wiring), same pattern, needed for the same reason (a plain C++ UCLASS'
	// CDO can't persist a reflection-set property edit across restarts/recompiles).
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;
};
