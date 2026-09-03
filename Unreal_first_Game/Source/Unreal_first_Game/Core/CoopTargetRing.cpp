#include "Core/CoopTargetRing.h"
#include "Core/CoopPlayerController.h"
#include "Core/CoopCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The engine Plane mesh is 100x100 units at scale 1 (half-extent 50), so scale = radius / 50.
	constexpr float PlaneHalfExtentUnits = 50.0f;

	const FLinearColor AllyRingColor(0.15f, 0.85f, 0.20f);   // green
	const FLinearColor EnemyRingColor(0.90f, 0.15f, 0.15f);  // red
}

ACoopTargetRing::ACoopTargetRing()
{
	PrimaryActorTick.bCanEverTick = true;

	// Explicit per CLAUDE.md §5, like ACoopOrbitCamera -- must never become a replicated actor.
	bReplicates = false;

	RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingMesh"));
	RootComponent = RingMesh;
	RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RingMesh->SetCastShadow(false);
	RingMesh->SetGenerateOverlapEvents(false);

	// Engine plane, always present. The ring shape itself is drawn by M_TargetRing across the
	// plane's [0,1] UVs (P3), not by the mesh.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded())
	{
		RingMesh->SetStaticMesh(PlaneFinder.Object);
	}
}

void ACoopTargetRing::Initialize(ACoopPlayerController* InOwningController, float RadiusUnits, float InGroundOffsetUnits)
{
	OwningController = InOwningController;
	GroundOffsetUnits = InGroundOffsetUnits;

	const float MeshScale = (RadiusUnits > 0.0f) ? (RadiusUnits / PlaneHalfExtentUnits) : 2.0f;
	RingMesh->SetRelativeScale3D(FVector(MeshScale, MeshScale, 1.0f));

	if (RingMaterial)
	{
		RingMesh->SetMaterial(0, RingMaterial);
		RingMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	SetActorHiddenInGame(true);
}

void ACoopTargetRing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const ACoopPlayerController* PC = OwningController.Get();
	AActor* Target = PC ? PC->GetCurrentTargetActor() : nullptr;
	if (!Target)
	{
		SetActorHiddenInGame(true);
		return;
	}

	SetActorHiddenInGame(false);

	FVector Location = Target->GetActorLocation();
	Location.Z -= GroundOffsetUnits;
	SetActorLocation(Location);

	// Green for a teammate (ACoopCharacter), red for anything else (a monster).
	if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(RingMesh->GetMaterial(0)))
	{
		const bool bIsAlly = (Cast<ACoopCharacter>(Target) != nullptr);
		MID->SetVectorParameterValue(TEXT("Color"), bIsAlly ? AllyRingColor : EnemyRingColor);
	}
}
