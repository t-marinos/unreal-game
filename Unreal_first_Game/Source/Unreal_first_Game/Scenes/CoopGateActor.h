#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoopGateActor.generated.h"

class UStaticMeshComponent;
class ACoopHoldTheGateScene;

// Build 1, M10: the physical gate for Hold the Gate (docs/scenes/HOLD_THE_GATE.md). Purely a
// cosmetic + collision responder to ACoopHoldTheGateScene's replicated IsGateOpen() -- it has no
// state of its own worth replicating, exact same reasoning and shape as ACoopButton reading
// ACoopGameState::IsButtonPressed(). When closed it blocks movement (a real obstacle, not just a
// visual); when open its collision is disabled and it drops out of the way so the party can walk
// through, per docs/scenes/HOLD_THE_GATE.md's "the party proceeds through the gate."
UCLASS()
class UNREAL_FIRST_GAME_API ACoopGateActor : public AActor
{
	GENERATED_BODY()

public:
	ACoopGateActor();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void ApplyGateVisual(bool bOpen);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<ACoopHoldTheGateScene> Scene;

	// Local cache so Tick only re-applies collision/visuals on an actual change, matching
	// ACoopButton's bLastAppliedPressedState pattern.
	bool bLastAppliedOpenState = false;

	// World-space Z the mesh sits at while closed -- captured once in BeginPlay so "open" can drop
	// it a fixed offset below without needing a second Editor-placed reference point.
	float ClosedZLocation = 0.0f;
};
