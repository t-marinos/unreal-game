#include "Dev/DummyAIController.h"
#include "GameFramework/Pawn.h"

ADummyAIController::ADummyAIController()
{
	// AAIController doesn't enable ticking by default (behavior trees drive most AI controllers
	// instead) -- we need our own Tick for the straight-line movement below.
	PrimaryActorTick.bCanEverTick = true;
}

void ADummyAIController::SetBehavior(EDummyBehavior NewBehavior, AActor* NewTarget)
{
	CurrentBehavior = NewBehavior;
	BehaviorTarget = NewTarget;
}

void ADummyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentBehavior == EDummyBehavior::Idle || !BehaviorTarget)
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const FVector ToTarget = BehaviorTarget->GetActorLocation() - ControlledPawn->GetActorLocation();
	if (ToTarget.SizeSquared2D() > FMath::Square(StopDistance))
	{
		ControlledPawn->AddMovementInput(ToTarget.GetSafeNormal2D());
	}
}
