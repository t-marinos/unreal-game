#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoopFixateRetargetComponent.generated.h"

// Build 1, M11: reusable fixate-then-retarget targeting behaviour (DECISIONS.md's "Monster combat
// inside Hold the Gate" -- Gravity Bridge is expected to reuse this same pattern later, so only the
// *targeting behavior* is generalized here, per that entry's scope boundary; spawn choreography
// stays local to each scene's own spawner class). This component owns nothing about *why* a target
// becomes invalid -- the owner picks an initial target from a candidate pool via
// PickInitialTarget(), then calls OnTargetDowned() whenever it learns (by its own means, e.g. a
// bound delegate) that the current target should be replaced. No Hold-the-Gate-specific references
// anywhere in this file.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_FIRST_GAME_API UCoopFixateRetargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoopFixateRetargetComponent();

	// Server-only. Remembers Candidates as the pool to retarget from later, then picks one at
	// random as the initial CurrentTarget. No-op if Candidates is empty.
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void PickInitialTarget(const TArray<AActor*>& Candidates);

	// Server-only. Picks a new CurrentTarget at random from the remembered candidate pool,
	// excluding the actor currently targeted (it's the one that just became invalid) and any
	// others that are no longer valid (e.g. destroyed). Leaves CurrentTarget unchanged if no valid
	// candidates remain -- the owner's own logic is expected to no-op against an invalid target
	// rather than this component inventing a "no target" sentinel state.
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void OnTargetDowned();

	UFUNCTION(BlueprintPure, Category = "Targeting")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

private:
	static AActor* PickRandomFrom(const TArray<AActor*>& Pool);

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> KnownCandidates;

	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentTarget;
};
