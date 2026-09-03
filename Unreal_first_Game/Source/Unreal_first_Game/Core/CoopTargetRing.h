#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoopTargetRing.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class ACoopPlayerController;

// Cursor-targeting feature (cursor_progress.md). Local-only, per-player flat ground ring under the
// current click-selected target -- CLAUDE.md §5's "a coloured ring on the ground ... is a spell
// effect", the sanctioned stand-in for a WoW-style selection outline (§5 forbids the
// post-processing an outline needs).
//
// Spawned only on the machine that controls its owning PlayerController (IsLocalController()),
// exactly like ACoopOrbitCamera -- bReplicates = false, never affects any other player's view. Its
// Tick reads ACoopPlayerController::GetCurrentTargetActor() fresh every frame, snaps to that
// actor's feet, tints itself green (teammate) / red (enemy), and hides itself when there's no
// target.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopTargetRing : public AActor
{
	GENERATED_BODY()

public:
	ACoopTargetRing();

	// Called once by ACoopPlayerController right after spawning this (mirrors
	// ACoopOrbitCamera::Initialize). RadiusUnits scales the plane mesh; GroundOffsetUnits is how far
	// below the target actor's origin to drop the ring so it sits on the floor rather than at
	// capsule-centre height.
	void Initialize(ACoopPlayerController* InOwningController, float RadiusUnits, float GroundOffsetUnits);

	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> RingMesh;

	// The ring plane material (M_TargetRing). Left unset (None) is a valid state -- the ring just
	// renders with the plane's default material until M_TargetRing is wired on BP_TargetRing's CDO
	// (P3, same content-wiring pattern as every other asset reference). If the ring work slips
	// entirely, ACoopPlayerController::TargetRingClass is simply left unset and this never spawns.
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TObjectPtr<UMaterialInterface> RingMaterial;

	// Never replicated -- this actor only ever exists on the one machine that spawned it.
	TWeakObjectPtr<ACoopPlayerController> OwningController;

	float GroundOffsetUnits = 88.0f;
};
