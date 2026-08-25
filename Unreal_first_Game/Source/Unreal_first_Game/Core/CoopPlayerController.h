#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/CoopRoleTypes.h"
#include "CoopPlayerController.generated.h"

class UGameConstants;
class ACoopOrbitCamera;
class UUserWidget;

// Server RPCs and Exec commands are PlayerController responsibilities in Unreal (each player has
// exactly one).
UCLASS()
class UNREAL_FIRST_GAME_API ACoopPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACoopPlayerController();

	// M7: intent only, per CLAUDE.md §4.1 -- "I walked into the button," never a result like "the
	// button is now lit." Called by ACoopButton when this controller's own pawn overlaps it.
	UFUNCTION(Server, Reliable)
	void Server_PressButton();

	// M8: console command (type "DumpGameState" in the in-game console on any machine -- server or
	// client). Walks GameState + every PlayerState and logs a JSON-shaped snapshot via UE_LOG, per
	// CLAUDE.md §4.3/§10's desync-debugging workflow: run this on the server, run it again on a
	// disagreeing client, diff the first field that differs.
	UFUNCTION(Exec)
	void DumpGameState();

	// M9: dev-mode console commands (CLAUDE.md §7). Each is a real, Exec-callable command that
	// routes through a Server RPC (Possess/invuln-toggling must happen with authority, and Exec
	// commands run locally on whichever machine typed them, which may be a client) -- the RPC call
	// costs nothing extra when typed on the server itself (HasAuthority() is already true, so
	// Unreal calls the _Implementation directly, no network round trip).
	UFUNCTION(Exec)
	void PossessDummy(int32 Index);

	UFUNCTION(Server, Reliable)
	void Server_PossessDummy(int32 Index);

	// Stub: Build 0 has no scenes yet, so this is a real, working command with nowhere meaningful
	// to jump to (CLAUDE.md §7). Build 1 gives it content once scenes exist.
	UFUNCTION(Exec)
	void SceneSkip();

	UFUNCTION(Exec)
	void ToggleGodMode();

	UFUNCTION(Server, Reliable)
	void Server_ToggleGodMode();

	// Build 1, M3: intent only -- "I want to play Tank," never a result. Forwards to
	// ACoopGameMode::TryClaimRole, which is the actual authoritative check (is this role still
	// Unassigned across every PlayerState?) per CLAUDE.md §4.1. See DECISIONS.md's "Role assignment
	// is player-chosen, not random" entry.
	UFUNCTION(Server, Reliable)
	void Server_ClaimRole(EPlayerRole DesiredRole);

	// Build 1, M6: debug command for verifying health replication/clamping/depletion (no gameplay
	// ability applies damage yet -- that's M7's Shield/M8's Stabilize). Type on whichever machine
	// controls the pawn to be damaged; same intent-only Exec-then-Server-RPC shape as
	// ToggleGodMode/PossessDummy above.
	UFUNCTION(Exec)
	void ApplyTestDamage(float Amount);

	UFUNCTION(Server, Reliable)
	void Server_ApplyTestDamage(float Amount);

protected:
	virtual void BeginPlay() override;

private:
	// Every tunable lives in DA_GameConstants per CLAUDE.md §10. Unlike ACoopGameMode, this class
	// has no Blueprint wrapper -- assign this directly on ACoopPlayerController's own CDO via
	// unreal-mcp's ObjectTools (a plain C++ UCLASS' CDO is just as reachable that way as a
	// Blueprint's, no wrapper asset needed).
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;

	// Spawned once in BeginPlay, only on the machine actually controlling this PlayerController
	// (CLAUDE.md §5's local-only camera). Never replicated -- see ACoopOrbitCamera.
	UPROPERTY()
	TObjectPtr<ACoopOrbitCamera> OrbitCamera;

	// M6: WBP_MatchTimer, content-wired on BP_PlayerController's CDO. Purely a local read of
	// ACoopGameState::GetElapsedMatchTime() -- no gameplay data lives on the widget itself.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MatchTimerWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MatchTimerWidget;

	// Build 1, M5: WBP_RoleSelect and WBP_PrepArenaHUD, same pattern as MatchTimerWidgetClass above
	// -- created once and left in the viewport, each independently reading GameState's current
	// phase (via UCoopRoleSelectWidget::GetRoleSelectVisibility / UCoopPrepCountdownWidget::
	// GetPrepArenaVisibility) to show/hide itself rather than being toggled externally on a phase
	// transition event, matching the established replicated-state-drives-cosmetic pattern.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> RoleSelectWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> RoleSelectWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PrepArenaHUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> PrepArenaHUDWidget;
};
