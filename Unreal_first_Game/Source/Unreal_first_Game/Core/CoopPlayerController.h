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

	// Build 1, M7: bound to IA_Shield's Enhanced Input event in BP_PlayerCharacter's EventGraph
	// (CLAUDE.md §3.2's hybrid split -- the ability itself resolves in C++, Blueprint only wires
	// the keypress to this call). A thin BlueprintCallable wrapper around the Server RPC, same
	// shape as UCoopRoleSelectWidget::ClaimTank()/etc. wrapping Server_ClaimRole -- Blueprint never
	// calls a raw Server_* RPC directly in this project.
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ActivateShield();

	UFUNCTION(Server, Reliable)
	void Server_ActivateShield();

	// Build 1, M8: same shape as ActivateShield/Server_ActivateShield above, bound to IA_Stabilize.
	// Targeting is implicit (nearest Tank in range) -- see CoopControlAbilities::ResolveStabilize.
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ActivateStabilize();

	UFUNCTION(Server, Reliable)
	void Server_ActivateStabilize();

	// Build 1, M9: fired from UCoopDownedComponent::OnReviveTriggerBeginOverlap (the reviver's own
	// controller, same "client that owns the overlapping pawn fires the RPC" filter as
	// Server_PressButton). Takes no target -- the server does its own nearest-Downed-in-range search,
	// same implicit-target shape as Server_ActivateStabilize.
	UFUNCTION(Server, Reliable)
	void Server_AttemptRevive();

	// Build 1: same shape as ActivateShield/ActivateStabilize above, bound to IA_Speed. Support-only
	// -- CoopSupportAbilities::ApplySpeed.
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ActivateSpeed();

	UFUNCTION(Server, Reliable)
	void Server_ActivateSpeed();

	// Bound to IA_Dash. Runner-only -- CoopRunnerAbilities::ResolveDash.
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ActivateDash();

	UFUNCTION(Server, Reliable)
	void Server_ActivateDash();

	// Bound to IA_Execution. Damage-only -- CoopDamageAbilities::ResolveExecution.
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ActivateExecution();

	UFUNCTION(Server, Reliable)
	void Server_ActivateExecution();

	// Dev/test only (CLAUDE.md §7), same shape as ApplyTestDamage: grants the nearest
	// ACoopMonsterCharacter Status.Vulnerable.Physical so Execution (which has no real tag-writer
	// until Scene 5/"The Heart" exists, per docs/abilities.md) can actually be tested now.
	UFUNCTION(Exec)
	void ApplyTestVulnerable();

	UFUNCTION(Server, Reliable)
	void Server_ApplyTestVulnerable();

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

	// The bottom-screen WoW-style ability bar (WBP_ActionBar). Same create-once-in-BeginPlay,
	// leave-in-viewport pattern as the widgets above; UCoopActionBarWidget hides itself outside the
	// Prep / HoldTheGate phases.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ActionBarWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActionBarWidget;
};
