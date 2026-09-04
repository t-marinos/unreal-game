#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CoopMonsterAIController.generated.h"

// MONSTER_ENEMIES_PROGRESS.md Phase A. Server-only AI controller for Hold the Gate's trash monsters
// (ACoopMonsterCharacter). Deliberately a near-copy of Dev/ADummyAIController: every tick it walks
// its pawn in a STRAIGHT LINE toward the monster's current fixate target via AddMovementInput --
// no MoveToActor, no navmesh query, no behaviour tree.
//
// This lives in Core/ (not Dev/) because it is real gameplay AI now, not dev tooling -- but it is
// still the same "simplest thing that moves a pawn toward a point" as the dummy. DECISIONS.md's
// "Monster combat inside Hold the Gate" entry authorises the fixate/retarget behaviour and forbids
// "adaptive AI, behavior trees, or pathfinding" -- straight-line steering toward an already-chosen
// target is none of those, so this stays inside that carve-out. If a monster walks into a wall it
// presses against it; the plate room is open enough that this never matters (CLAUDE.md §1: ugly is
// correct).
UCLASS()
class UNREAL_FIRST_GAME_API ACoopMonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACoopMonsterAIController();

protected:
	virtual void Tick(float DeltaTime) override;
};
