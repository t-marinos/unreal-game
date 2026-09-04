#include "Scenes/CoopMonsterStrikeTelegraph.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

namespace
{
	// The engine Plane mesh is 100x100 units at scale 1 (half-extent 50), so scale = radius / 50 --
	// same basis as ACoopTargetRing.
	constexpr float PlaneHalfExtentUnits = 50.0f;

	// Red -- "a monster is about to hit here". Same shade as ACoopTargetRing's enemy ring.
	const FLinearColor StrikeRingColor(0.90f, 0.10f, 0.10f);
}

ACoopMonsterStrikeTelegraph::ACoopMonsterStrikeTelegraph()
{
	PrimaryActorTick.bCanEverTick = false;

	// Server spawns it, every client renders it (CLAUDE.md §4.5 -- all five see the same window at
	// the same instant). The spawn transform reaches clients in the spawn bunch and the actor never
	// moves; only the replicated TelegraphRadius still needs to arrive after that.
	bReplicates = true;

	RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingMesh"));
	RootComponent = RingMesh;
	RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RingMesh->SetCastShadow(false);
	RingMesh->SetGenerateOverlapEvents(false);

	// Engine plane, always present. The ring shape is drawn by M_TargetRing across the plane's
	// [0,1] UVs, not by the mesh -- same as ACoopTargetRing.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded())
	{
		RingMesh->SetStaticMesh(PlaneFinder.Object);
	}
}

void ACoopMonsterStrikeTelegraph::BeginPlay()
{
	Super::BeginPlay();

	// Clients: TelegraphRadius has already replicated in with the initial bunch (the monster calls
	// Initialize synchronously right after SpawnActor, before the first net update). Server host:
	// this runs with radius 0 and Initialize re-applies the real value a moment later.
	ApplyTelegraphVisual();
}

void ACoopMonsterStrikeTelegraph::Initialize(float RadiusUnits)
{
	TelegraphRadius = RadiusUnits;
	ApplyTelegraphVisual();
}

void ACoopMonsterStrikeTelegraph::ApplyTelegraphVisual()
{
	const float MeshScale = (TelegraphRadius > 0.0f) ? (TelegraphRadius / PlaneHalfExtentUnits) : 2.0f;
	RingMesh->SetRelativeScale3D(FVector(MeshScale, MeshScale, 1.0f));

	if (RingMaterial)
	{
		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(RingMesh->GetMaterial(0));
		if (!MID)
		{
			RingMesh->SetMaterial(0, RingMaterial);
			MID = RingMesh->CreateAndSetMaterialInstanceDynamic(0);
		}
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"), StrikeRingColor);
		}
	}
}

void ACoopMonsterStrikeTelegraph::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoopMonsterStrikeTelegraph, TelegraphRadius);
}
