#include "Core/CoopFixateRetargetComponent.h"
#include "GameFramework/Actor.h"

UCoopFixateRetargetComponent::UCoopFixateRetargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCoopFixateRetargetComponent::PickInitialTarget(const TArray<AActor*>& Candidates)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	KnownCandidates.Reset();
	TArray<AActor*> ValidCandidates;
	for (AActor* Candidate : Candidates)
	{
		if (Candidate)
		{
			KnownCandidates.Add(Candidate);
			ValidCandidates.Add(Candidate);
		}
	}

	CurrentTarget = PickRandomFrom(ValidCandidates);

	// CLAUDE.md §4.3: state must be printable -- this component has no reflectable target field
	// (TWeakObjectPtr isn't exposed to the project's reflection-based inspection tooling), so a log
	// line is the only way to observe fixate/retarget behaviour at all.
	UE_LOG(LogTemp, Log, TEXT("UCoopFixateRetargetComponent::PickInitialTarget: %s fixated on %s"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"),
		CurrentTarget.IsValid() ? *CurrentTarget->GetName() : TEXT("<none>"));
}

void UCoopFixateRetargetComponent::OnTargetDowned()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const FString OldTargetName = CurrentTarget.IsValid() ? CurrentTarget->GetName() : TEXT("<none>");

	TArray<AActor*> StillValid;
	for (const TWeakObjectPtr<AActor>& Candidate : KnownCandidates)
	{
		AActor* CandidateActor = Candidate.Get();
		if (CandidateActor && CandidateActor != CurrentTarget.Get())
		{
			StillValid.Add(CandidateActor);
		}
	}

	if (StillValid.Num() > 0)
	{
		CurrentTarget = PickRandomFrom(StillValid);
	}
	// else: no valid candidates left (e.g. everyone Downed at once) -- leave CurrentTarget as-is.

	UE_LOG(LogTemp, Log, TEXT("UCoopFixateRetargetComponent::OnTargetDowned: %s retargeted from %s to %s"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"),
		*OldTargetName,
		CurrentTarget.IsValid() ? *CurrentTarget->GetName() : TEXT("<none>"));
}

AActor* UCoopFixateRetargetComponent::PickRandomFrom(const TArray<AActor*>& Pool)
{
	if (Pool.Num() == 0)
	{
		return nullptr;
	}
	return Pool[FMath::RandHelper(Pool.Num())];
}
