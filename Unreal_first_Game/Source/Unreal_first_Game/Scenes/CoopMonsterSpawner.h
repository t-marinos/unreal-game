#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoopMonsterSpawner.generated.h"

class UGameConstants;
class ACoopMonsterCharacter;

// Build 1, M11: Hold the Gate's monster chamber (docs/scenes/HOLD_THE_GATE.md,
// DECISIONS.md's "Monster combat inside Hold the Gate"). Hold-the-Gate-specific spawn
// choreography (timing/escalation/candidate gathering) -- kept local to this scene per that
// entry's scope boundary, unlike the reusable UCoopFixateRetargetComponent. One instance is placed
// per monster chamber in the level (content wiring, MCP-buildable per the plan's own table).
UCLASS()
class UNREAL_FIRST_GAME_API ACoopMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACoopMonsterSpawner();

	// Build 1, M12. Called by ACoopHoldTheGateScene::ResetScene() on a scene reset (wipe or manual
	// retry) -- restarts this spawner's escalation ramp from "early" and reschedules its next spawn,
	// same shape as a fresh BeginPlay but without re-polling the roster (roles are already resolved
	// by the time a HoldTheGate-phase reset is reachable at all).
	void ResetSpawner();

protected:
	virtual void BeginPlay() override;

private:
	void SpawnMonster();
	void ScheduleNextSpawn();

	// BeginPlay fires at level load, well before ACoopGameMode's RoleSelect phase resolves --
	// spawning immediately would let GatherNonTankCandidates() capture a monster's target pool while
	// every PlayerState still reads EPlayerRole::Unassigned, which trivially passes the "!= Tank"
	// filter and can leave the eventual Tank stuck as a permanent target (KnownCandidates/CurrentTarget
	// are captured once at spawn, not re-filtered later). Poll until roles are actually resolved
	// (Prep phase or later) before scheduling the first real spawn.
	void CheckRosterAndStartSpawning();

	FTimerHandle RosterCheckTimerHandle;
	static constexpr float RosterCheckIntervalSeconds = 0.5f;

	// Candidate pool for a spawned monster's fixate target: every real ACoopCharacter whose role
	// isn't Tank (docs/scenes/HOLD_THE_GATE.md: "monsters ... target the pinned plate-holders" --
	// Tank is the only mobile player, so targeting is role-based, not tied to which specific plate
	// someone is currently standing on). Gathered fresh at each spawn rather than cached once.
	TArray<AActor*> GatherNonTankCandidates() const;

	// Build 1, M12: a linear ramp from MonsterSpawnIntervalEarlySeconds down to
	// MonsterSpawnIntervalLateSeconds across the full HoldTheGateSceneDurationSeconds -- replaces
	// M11's hardcoded 60s placeholder window now that the real scene-duration constant exists.
	float ComputeCurrentSpawnInterval() const;

	// The monster type this chamber spawns -- set to BP_MonsterCharacter on BP_MonsterSpawner's
	// CDO, same TSubclassOf-on-a-BP-wrapper pattern already proven by AGameModeBase's own
	// PlayerControllerClass/PlayerStateClass/DefaultPawnClass fields.
	UPROPERTY(EditDefaultsOnly, Category = "Monster")
	TSubclassOf<ACoopMonsterCharacter> MonsterClass;

	// Every tunable lives in DA_GameConstants per CLAUDE.md §10.
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;

	FTimerHandle SpawnTimerHandle;
	float SpawnerStartServerTime = 0.0f;
};
