#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DummyAIController.generated.h"

// M9: hardcoded dummy behaviors for dev-mode solo iteration (CLAUDE.md §7/§9's "no adaptive
// difficulty" principle applies to dev tooling too -- plain hardcoded logic, no behavior trees).
UENUM(BlueprintType)
enum class EDummyBehavior : uint8
{
	// Hold position -- the only behavior actually exercised by M9 itself.
	Idle,
	// Walk toward BehaviorTarget every tick. Mechanism only for now -- built ahead of the scenes
	// that will give it a real target (a fleeing player, a lever), per CLAUDE.md §4.8.
	FollowPlayer,
	// Walk toward and hold near BehaviorTarget -- same mechanism as FollowPlayer, kept as a
	// separate named behavior since a future scene's "occupy this pressure plate" need is
	// conceptually different from "chase this player" even though both move the same way today.
	StandOn
};

// Server-only AI controller for dev-mode filler pawns (CLAUDE.md §7 Build 0 dev mode). Never
// possesses a pawn on its own initiative -- ACoopGameMode explicitly Possess()es it onto a
// spawned dummy character.
UCLASS()
class UNREAL_FIRST_GAME_API ADummyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ADummyAIController();

	void SetBehavior(EDummyBehavior NewBehavior, AActor* NewTarget = nullptr);

protected:
	virtual void Tick(float DeltaTime) override;

private:
	EDummyBehavior CurrentBehavior = EDummyBehavior::Idle;

	UPROPERTY()
	TObjectPtr<AActor> BehaviorTarget;

	// Straight-line AddMovementInput toward the target, not navmesh pathfinding -- Build 0's level
	// has no nav data, and M9's own verify step doesn't require dummies to actually navigate, only
	// to exist/be possessable. Deliberately the simplest thing that could work, per CLAUDE.md §1.
	static constexpr float StopDistance = 150.0f;
};
