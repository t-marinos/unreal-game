#include "Scenes/CoopGateActor.h"
#include "Scenes/CoopHoldTheGateScene.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"

ACoopGateActor::ACoopGateActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Purely a cosmetic + collision responder to ACoopHoldTheGateScene's replicated IsGateOpen() --
	// it has no state of its own worth replicating, same reasoning as ACoopButton.
	bReplicates = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMeshFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GateMaterialFinder(TEXT("/Game/Materials/M_CoopButton.M_CoopButton"));
	if (GateMaterialFinder.Succeeded())
	{
		Mesh->SetMaterial(0, GateMaterialFinder.Object);
	}
	// A tall, wide, thin slab -- reads as a barrier across a doorway, not a button.
	SetActorScale3D(FVector(3.0f, 0.2f, 4.0f));
}

void ACoopGateActor::BeginPlay()
{
	Super::BeginPlay();

	ClosedZLocation = GetActorLocation().Z;

	for (TActorIterator<ACoopHoldTheGateScene> It(GetWorld()); It; ++It)
	{
		Scene = *It;
		break;
	}

	if (!Scene)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACoopGateActor::BeginPlay: no ACoopHoldTheGateScene found in the level -- gate will stay closed forever."));
	}

	ApplyGateVisual(false);
}

void ACoopGateActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Scene)
	{
		return;
	}

	const bool bOpen = Scene->IsGateOpen();
	if (bOpen != bLastAppliedOpenState)
	{
		bLastAppliedOpenState = bOpen;
		ApplyGateVisual(bOpen);
	}
}

void ACoopGateActor::ApplyGateVisual(bool bOpen)
{
	// Closed: full collision, blocks the doorway, red. Open: no collision, drops out of the way so
	// the party can walk through, green -- an instant, ugly-is-correct transition (CLAUDE.md §1), no
	// animation needed for a disposable prototype.
	Mesh->SetCollisionEnabled(bOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);

	FVector Location = GetActorLocation();
	Location.Z = bOpen ? (ClosedZLocation - 400.0f) : ClosedZLocation;
	SetActorLocation(Location);

	const FLinearColor Color = bOpen ? FLinearColor(0.1f, 1.0f, 0.1f) : FLinearColor(1.0f, 0.1f, 0.1f);
	if (UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	}
}
