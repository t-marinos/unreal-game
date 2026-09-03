#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoopPressurePlate.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UGameConstants;

// Server-only signal (see BeginPlay's HasAuthority() gate below). ACoopHoldTheGateScene binds one
// handler per plate via AddDynamic -- same dynamic-delegate shape as
// UCoopHealthComponent::OnHealthDepleted -- fires whenever this plate's occupancy actually changes,
// never on every overlap tick.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlateOccupancyChanged);

// Build 1, M10: one of Hold the Gate's four pressure plates (docs/scenes/HOLD_THE_GATE.md).
// Occupancy is server-only (BeginPlay's HasAuthority() gate below), event-driven off the trigger
// volume's Begin/End overlap -- no polling. bIsOccupied replicates purely so every client can show
// the same cosmetic lit/unlit response, same pattern as ACoopButton. OnOccupancyChanged is a
// server-only signal ACoopHoldTheGateScene binds to (one instance per plate) to recompute aggregate
// gate state -- this plate has no idea a scene or a gate exists, it only reports its own state,
// per the plan's "reports to the scene manager rather than a single global bool."
UCLASS()
class UNREAL_FIRST_GAME_API ACoopPressurePlate : public AActor
{
	GENERATED_BODY()

public:
	ACoopPressurePlate();

	UFUNCTION(BlueprintPure, Category = "Plate")
	bool IsOccupied() const { return bIsOccupied; }

	UPROPERTY(BlueprintAssignable, Category = "Plate")
	FOnPlateOccupancyChanged OnOccupancyChanged;

protected:
	virtual void BeginPlay() override;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Server-only. Recomputes occupancy from the trigger volume's own overlapping-actors list
	// (rather than trusting a simple increment/decrement counter) so a departing actor that never
	// fired its own EndOverlap (e.g. destroyed while standing on the plate) can't leave the plate
	// permanently stuck "occupied."
	void RefreshOccupancy();

	// Sizes/positions TriggerVolume from DA_GameConstants::PlateTriggerCatchHeightUnits. Runs in
	// BeginPlay, not the constructor, because GameConstants (an EditDefaultsOnly asset reference
	// set on the Blueprint's CDO) isn't resolved yet during native construction -- same reasoning
	// as ACoopHoldTheGateScene reading its own GameConstants fields only from BeginPlay onward.
	void ApplyTriggerVolumeSize();

	void SetOccupied(bool bNewOccupied);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> TriggerVolume;

	// Every tunable lives in DA_GameConstants per CLAUDE.md §10 -- set via BP_PressurePlate's CDO,
	// same CDO-persistence reasoning as ACoopHoldTheGateScene's own GameConstants reference.
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;

	UPROPERTY(ReplicatedUsing = OnRep_IsOccupied, VisibleAnywhere, Category = "Plate")
	bool bIsOccupied = false;

	UFUNCTION()
	void OnRep_IsOccupied();

	void ApplyOccupiedVisual(bool bOccupied);
};
