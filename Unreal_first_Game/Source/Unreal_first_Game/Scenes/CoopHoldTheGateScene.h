#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoopHoldTheGateScene.generated.h"

class ACoopPressurePlate;
class ACoopMonsterSpawner;
class UGameConstants;

// Build 1, M10: Hold the Gate's server-side scene logic/timeline (CLAUDE.md's documented Scenes/
// convention -- docs/scenes/HOLD_THE_GATE.md). A single instance is placed in the level alongside
// its four ACoopPressurePlate actors and one ACoopGateActor. Owns the aggregate "are all plates
// held" state, the gate-open bool every ACoopGateActor reads, and the restore-window timer that
// turns a broken hold into this scene's own wipe condition (CLAUDE.md §6.6).
//
// Build 1, M12: extended with the scene's win condition (HoldTheGateSceneDurationSeconds elapses
// while the gate is held open -- docs/scenes/HOLD_THE_GATE.md's "the scripted threat sequence
// completes") and the "all five Downed" wipe path (wired in ACoopGameState::IncrementDownedCount
// instead of here -- see that function's comment for why), plus the full ResetScene() behaviour
// CLAUDE.md §6.6's "instant restart from the beginning of the current scene" implies: respawn
// positions, and restarting the monster spawners' escalation clock, not just the gate/timer state
// M10 already reset.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopHoldTheGateScene : public AActor
{
	GENERATED_BODY()

public:
	ACoopHoldTheGateScene();

	UFUNCTION(BlueprintPure, Category = "HoldTheGate")
	bool IsGateOpen() const { return bGateOpen; }

protected:
	virtual void BeginPlay() override;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Server-only. Bound (AddDynamic) to every discovered ACoopPressurePlate's OnOccupancyChanged.
	// Recomputes aggregate occupancy from the plates' own current IsOccupied() values -- event-driven
	// per the plan ("overlap begin/end, not polling"), never ticked.
	UFUNCTION()
	void HandlePlateOccupancyChanged();

	// Server-only. Fires when PlateRestoreWindowSeconds elapses without full occupancy being
	// restored -- this scene's own wipe condition (docs/scenes/HOLD_THE_GATE.md's "gate closes ...
	// can't be restored in time").
	void OnRestoreWindowExpired();

	// Server-only. Bound to ACoopGameState::OnSceneResetRequested so a reset triggered from anywhere
	// (this scene's own restore-window wipe, or the all-Downed wipe wired in
	// ACoopGameState::IncrementDownedCount) also resets this scene's own state, not just revives
	// characters. Thin wrapper so the delegate signature (no params) stays separate from the actual
	// reset logic in ResetScene().
	UFUNCTION()
	void HandleSceneResetRequested();

	bool AreAllPlatesOccupied() const;

	// Build 1, M12. Polls (same 0.5s-interval idiom as ACoopMonsterSpawner::CheckRosterAndStartSpawning)
	// until ACoopGameState reports the HoldTheGate phase has actually started, then starts the scene's
	// own duration timer -- BeginPlay fires at level load, well before phase transitions resolve, so
	// starting the timer directly in BeginPlay would race the Prep phase's own countdown.
	void CheckPhaseAndStartSceneTimer();

	// Build 1, M12. Fires when HoldTheGateSceneDurationSeconds elapses. A held-open gate at that
	// moment is docs/scenes/HOLD_THE_GATE.md's actual win condition; running out the clock without
	// ever achieving/holding it open isn't one of CLAUDE.md §6.6's two named wipe conditions, but
	// isn't a win either -- treated as a reset via the same RequestSceneReset() path as the other
	// fail cases, a documented, minimal way to close that gap rather than leaving it unhandled.
	void OnSceneDurationExpired();

	// Build 1, M12. Win path: docs/scenes/HOLD_THE_GATE.md's "Success" condition.
	void CompleteScene();

	// Build 1, M12. The actual reset behaviour CLAUDE.md §6.6's "instant restart from the beginning
	// of the current scene" implies, beyond what M10 already did (clear the restore-window timer,
	// close the gate): restarts this scene's own duration timer, teleports every real ACoopCharacter
	// back to the level's PlayerStart (simultaneous teleport to the same point is a deliberate
	// ugly-is-correct simplification per CLAUDE.md §1 -- momentary capsule interpenetration is
	// cosmetic, not a desync), clears every live monster, and restarts each spawner's escalation
	// ramp via ACoopMonsterSpawner::ResetSpawner(). GameState's own RequestSceneReset() already
	// healed/un-Downed everyone before broadcasting OnSceneResetRequested, so this only needs to
	// handle what's specific to this scene.
	void ResetScene();

	// Read by ACoopGateActor's Tick each frame (same replicated-state-drives-cosmetic pattern as
	// ACoopGameState::bButtonPressed/ACoopButton) -- no OnRep needed since nothing on this actor
	// itself reacts cosmetically to the value changing.
	UPROPERTY(Replicated, VisibleAnywhere, Category = "HoldTheGate")
	bool bGateOpen = false;

	UPROPERTY()
	TArray<TObjectPtr<ACoopPressurePlate>> Plates;

	// Build 1, M12: gathered in BeginPlay alongside Plates, same TActorIterator pattern -- so
	// ResetScene() can restart every chamber's escalation ramp without each spawner needing to know
	// about the scene itself.
	UPROPERTY()
	TArray<TObjectPtr<ACoopMonsterSpawner>> Spawners;

	FTimerHandle RestoreWindowTimerHandle;
	FTimerHandle ScenePhaseCheckTimerHandle;
	FTimerHandle SceneDurationTimerHandle;
	static constexpr float ScenePhaseCheckIntervalSeconds = 0.5f;

	// Every tunable lives in DA_GameConstants per CLAUDE.md §10 -- set via BP_HoldTheGateScene's CDO,
	// same CDO-persistence reasoning as every other class holding an EditDefaultsOnly asset reference.
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;
};
