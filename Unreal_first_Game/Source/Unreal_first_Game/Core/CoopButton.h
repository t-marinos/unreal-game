#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoopButton.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

// M7: the one button/effect actor proving the Server RPC + replication plumbing (CLAUDE.md §7).
// A player walking into the trigger volume sends an intent-only Server RPC
// (ACoopPlayerController::Server_PressButton) from their own PlayerController -- never a result,
// per CLAUDE.md §4.1. The server toggles ACoopGameState::bButtonPressed; every client (including
// this actor's own copy, via Tick) reads that replicated value and updates the button's colour
// purely as a cosmetic response, never reacting to the RPC or the overlap event directly.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopButton : public AActor
{
	GENERATED_BODY()

public:
	ACoopButton();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> TriggerVolume;

	// Local cache so Tick only re-applies the material tint on an actual change, not every frame --
	// purely a rendering optimisation, not gameplay state.
	bool bLastAppliedPressedState = false;

	void ApplyPressedVisual(bool bPressed);
};
