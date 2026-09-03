#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/CoopRoleTypes.h"
#include "CoopPlayerController.generated.h"

class UGameConstants;
class ACoopOrbitCamera;
class ACoopTargetRing;
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
	// TARGET-REQUIRED (DECISIONS.md "Target-required abilities need a click-selected target"): the
	// wrapper reads GetCurrentTargetActor(); null -> ShowToast("Please choose a target") and RETURN
	// without sending the RPC; non-null -> Server_ActivateExecution(Target), which re-validates.
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ActivateExecution();

	UFUNCTION(Server, Reliable)
	void Server_ActivateExecution(AActor* Target);

	// Ability kit expansion. Bound to IA_ArmorBreak (E). Tank-only. Same thin-wrapper /
	// target-required shape as ActivateExecution -- see DECISIONS.md. Resolves in
	// CoopTankAbilities::ResolveArmorBreak.
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ActivateArmorBreak();

	UFUNCTION(Server, Reliable)
	void Server_ActivateArmorBreak(AActor* Target);

	// Ability kit expansion. Bound to IA_Overload (E). Damage-only. Same target-required shape.
	// Resolves in CoopDamageAbilities::ResolveOverload. IA_ArmorBreak and IA_Overload share the E
	// key: harmless, every Server_Activate* is role-gated and no-ops for the wrong role -- same
	// pattern that puts all five first abilities on Q (DECISIONS.md "The Q ability per role").
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ActivateOverload();

	UFUNCTION(Server, Reliable)
	void Server_ActivateOverload(AActor* Target);

	// Ability kit expansion. Local, cosmetic (CLAUDE.md §4.2): shows a centre-screen message via
	// UCoopToastWidget. Called from the target-required ability wrappers when the ability is pressed
	// with no target selected. Not replicated -- each client's own missed-input feedback.
	void ShowToast(const FText& Message);

	FText GetPendingToastText() const { return PendingToastText; }
	float GetPendingToastStartTime() const { return PendingToastStartTime; }

	// Dev/test only (CLAUDE.md §7), mirror of ApplyTestVulnerable: grants the nearest
	// ACoopMonsterCharacter Status.Vulnerable.Magic so Overload can be tested before "The Heart"
	// (Scene 5) exists. DELETE both when that scene lands, alongside ApplyTestVulnerable.
	UFUNCTION(Exec)
	void ApplyTestVulnerableMagic();

	UFUNCTION(Server, Reliable)
	void Server_ApplyTestVulnerableMagic();

	// Dev/test only (CLAUDE.md §7), same shape as ApplyTestDamage: grants the nearest
	// ACoopMonsterCharacter Status.Vulnerable.Physical so Execution (which has no real tag-writer
	// until Scene 5/"The Heart" exists, per docs/abilities.md) can actually be tested now.
	UFUNCTION(Exec)
	void ApplyTestVulnerable();

	UFUNCTION(Server, Reliable)
	void Server_ApplyTestVulnerable();

	// Cursor-targeting feature (cursor_progress.md). PURELY LOCAL, cosmetic UI state (CLAUDE.md
	// §4.2): "what this player has clicked on". Never replicated, never sent to the server. The
	// target frame / party frame widgets (UCoopUnitFrameWidget) and the ground ring
	// (ACoopTargetRing) read this to decide what to draw; nothing gameplay-side reads it -- the five
	// abilities keep their own implicit nearest-X targeting. A future phase may route this into the
	// Server_Activate* RPCs as *intent* (server re-validates); this getter is that seam.
	AActor* GetCurrentTargetActor() const { return CurrentTargetActor.Get(); }

	// Set the current target directly, no cursor trace. Used by UCoopUnitFrameWidget when a
	// party-frame row is left-clicked -- the row already knows which teammate actor it's showing.
	// Same local-only / no-RPC / not-replicated contract as SelectTargetUnderCursor, and re-applies
	// the same ACoopCharacter / ACoopMonsterCharacter accept-filter so a stray caller can't stuff in
	// something nonsensical. A null / rejected NewTarget leaves the existing target untouched (use
	// ClearTarget() to clear).
	void SetCurrentTarget(AActor* NewTarget);

	// Bound to IA_Select (left mouse button) via BP_PlayerCharacter's EventGraph, same wiring shape
	// as the ability inputs. Deprojects the cursor; if it lands on an ACoopCharacter or an
	// ACoopMonsterCharacter that becomes the current target, otherwise the target is cleared
	// (cursor_progress.md decision #2). Local-only -- no RPC, no replicated state written.
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void SelectTargetUnderCursor();

	// Also bound to a key (Esc) in BP. Clears the current target.
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void ClearTarget();

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

	// Cursor-targeting feature (cursor_progress.md): the top-left single target frame (WBP_TargetFrame)
	// and the always-on 5-row party stack (WBP_PartyFrame). Same create-once-in-BeginPlay /
	// leave-in-viewport pattern as the widgets above; each UCoopUnitFrameWidget row self-gates via
	// RenderOpacity (phase + "is there anything to show").
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> TargetFrameWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> TargetFrameWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PartyFrameWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> PartyFrameWidget;

	// Ability kit expansion: WBP_Toast, the centre-screen "Please choose a target" message. Same
	// create-once-in-BeginPlay / leave-in-viewport pattern as every widget above; UCoopToastWidget
	// reads PendingToast* below in its own NativeTick and fades itself via RenderOpacity.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ToastWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> ToastWidget;

	// Local, non-replicated toast state -- what UCoopToastWidget::NativeTick reads. StartTime is
	// GetWorld()->GetTimeSeconds() at the ShowToast() call; -1 means "nothing shown yet".
	FText PendingToastText;
	float PendingToastStartTime = -1.0f;

	// Client-side "is this ability off cooldown?" check for the pre-RPC "Ability not ready" toast in
	// the Activate*() wrappers. Reads the owning pawn's COND_OwnerOnly-replicated
	// *CooldownEndServerTime vs. GetServerWorldTimeSeconds() -- the same value the action-bar sweep
	// reads. The server still enforces the cooldown authoritatively inside every Resolve*/Apply*.
	bool IsAbilityReady(float CooldownEndServerTime) const;

	// Cursor-targeting feature. TWeakObjectPtr so a targeted actor being destroyed (a monster dying)
	// silently null-resolves instead of dangling -- the widgets and ring treat null as "nothing to
	// show". NOT a UPROPERTY(Replicated), NO DOREPLIFETIME -- see GetCurrentTargetActor()'s comment.
	TWeakObjectPtr<AActor> CurrentTargetActor;

	// Local-only ground ring under the current target (cursor_progress.md). Spawned in BeginPlay
	// alongside OrbitCamera, only on the controlling machine. Class is content-wired on
	// BP_PlayerController's CDO (typically BP_TargetRing); unset simply means "no ring".
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<ACoopTargetRing> TargetRingClass;

	UPROPERTY()
	TObjectPtr<ACoopTargetRing> TargetRing;
};
