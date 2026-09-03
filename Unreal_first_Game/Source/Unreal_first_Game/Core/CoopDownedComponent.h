#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoopDownedComponent.generated.h"

class ACoopCharacter;
class UGameConstants;
class UPrimitiveComponent;
struct FHitResult;

// Server-only signal, broadcast from SetDowned() on every transition (both entering AND leaving
// Downed) -- same "fires on any change, listener decides what matters" shape as
// ACoopPressurePlate::OnOccupancyChanged. Build 1, M11: ACoopMonsterCharacter binds to its current
// target's instance of this to know when to retarget (only entering Downed matters to it; a revive
// completing does not).
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDownedStateChanged);

// Build 1, M9. Subscribes to UCoopHealthComponent::OnHealthDepleted -- when the owning
// ACoopCharacter hits 0 HP, it goes Downed instead of dying (CLAUDE.md §6.6): immobile, cannot use
// abilities, still visible and able to talk. Clears via a teammate's proximity-triggered revive
// channel (BeginRevive/CompleteRevive), or ForceRevive() for a full-party scene reset.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_FIRST_GAME_API UCoopDownedComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoopDownedComponent();

	UFUNCTION(BlueprintPure, Category = "Downed")
	bool IsDowned() const;

	UPROPERTY(BlueprintAssignable, Category = "Downed")
	FOnDownedStateChanged OnDownedStateChanged;

	// Server-only. Called by ACoopPlayerController::Server_AttemptRevive's RPC handler once it has
	// found this as the nearest Downed teammate within ReviveRadiusUnits of the reviver. Starts a
	// ReviveDurationSeconds timer; re-checks proximity only at the start (implicitly, via the RPC's
	// own range search) and again at completion -- a documented simplification of "standing adjacent
	// for the whole duration" (CLAUDE.md §6.6). A true continuous channel would need either a Tick
	// or repeated client RPCs while held; this milestone doesn't need that precision, per CLAUDE.md
	// §1's "take the simple approach, note the tradeoff." No-ops if not currently Downed, or already
	// mid-revive from someone else.
	void BeginRevive(ACoopCharacter* Reviver);

	// Server-only. Immediately clears Downed with no channel/proximity check and a full heal -- used
	// by ACoopGameState::RequestSceneReset() for a full-party wipe reset (CLAUDE.md §6.6's "instant
	// restart"). No-ops if not currently Downed.
	void ForceRevive();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleHealthDepleted();

	UFUNCTION()
	void OnReviveTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void CompleteRevive();
	void SetDowned(bool bNewDowned);

	// Every tunable lives in DA_GameConstants per CLAUDE.md §10. Component subobject on
	// ACoopCharacter -- the CDO-persistence gotcha applies (CoopHealthComponent's own comment
	// explains why): set this via BP_PlayerCharacter's component defaults, not the C++ CDO.
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;

	UPROPERTY()
	TObjectPtr<ACoopCharacter> OwnerCharacter;

	// Server-only bookkeeping: who's currently channeling a revive on this character, if anyone.
	// Not replicated -- Build 1 has no UI for "someone is reviving you" yet.
	UPROPERTY()
	TWeakObjectPtr<ACoopCharacter> ReviverInProgress;

	FTimerHandle ReviveTimerHandle;
};
