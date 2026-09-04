#include "Core/CoopMonsterAIController.h"
#include "Core/CoopMonsterCharacter.h"
#include "Core/CoopFixateRetargetComponent.h"
#include "GameFramework/Pawn.h"

ACoopMonsterAIController::ACoopMonsterAIController()
{
	// AAIController leaves ticking off by default (behaviour trees drive most AI controllers) --
	// we need our own Tick for the straight-line steering below, same as ADummyAIController.
	PrimaryActorTick.bCanEverTick = true;
}

void ACoopMonsterAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// AI controllers only ever exist on the server, but guard anyway -- movement intent is
	// server-authoritative (CLAUDE.md §4.1); the CharacterMovementComponent then replicates the
	// resulting motion (the §4.2 movement-prediction exception).
	ACoopMonsterCharacter* Monster = Cast<ACoopMonsterCharacter>(GetPawn());
	if (!Monster || !Monster->HasAuthority())
	{
		return;
	}

	const UCoopFixateRetargetComponent* Targeting = Monster->GetTargetingComponent();
	const AActor* Target = Targeting ? Targeting->GetCurrentTarget() : nullptr;
	if (!Target)
	{
		// Everyone Downed, or the target was destroyed -- hold position, the monster's own
		// retarget logic (ACoopMonsterCharacter) will hand us a new target when it has one.
		return;
	}

	const float StopDistance = Monster->GetMeleeRangeUnits();
	const FVector ToTarget = Target->GetActorLocation() - Monster->GetActorLocation();

	// 2D only -- the room is flat and the monster shouldn't try to pitch toward a target at a
	// different Z. Once inside melee range we stop feeding input so ACoopMonsterCharacter's own
	// PerformAttackTick range check reads a stable position, not a jitter against the boundary.
	if (ToTarget.SizeSquared2D() > FMath::Square(StopDistance))
	{
		Monster->AddMovementInput(ToTarget.GetSafeNormal2D());
	}
}
