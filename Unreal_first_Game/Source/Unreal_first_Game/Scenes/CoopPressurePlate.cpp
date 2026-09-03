#include "Scenes/CoopPressurePlate.h"
#include "Core/CoopCharacter.h"
#include "Core/GameConstants.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACoopPressurePlate::ACoopPressurePlate()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Hardcoded engine content, same "flat coloured primitive" bar as ACoopButton (CLAUDE.md §5) --
	// a low flat box reads as a plate without any custom art.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMeshFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlateMaterialFinder(TEXT("/Game/Materials/M_CoopButton.M_CoopButton"));
	if (PlateMaterialFinder.Succeeded())
	{
		Mesh->SetMaterial(0, PlateMaterialFinder.Object);
	}
	SetActorScale3D(FVector(2.5f, 2.5f, 0.2f));

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(RootComponent);
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// GameConstants isn't resolved yet this early in construction, so this uses
	// ApplyTriggerVolumeSize's null-GameConstants fallback -- BeginPlay re-applies it once the
	// tuned value from DA_GameConstants is available. Actor scale (set just above) is already in
	// effect by this point, so the world-space sizing math below is correct even here.
	ApplyTriggerVolumeSize();
}

void ACoopPressurePlate::BeginPlay()
{
	Super::BeginPlay();

	ApplyTriggerVolumeSize();

	// Occupancy is a symmetric physical fact ("is any character standing here"), not an action
	// attributable to one specific player's own client the way ACoopButton's press is -- so, unlike
	// ACoopButton, this binds directly on the server rather than relying on a client-side
	// IsLocallyControlled() filter plus an RPC. Only the server needs to evaluate this at all.
	if (!HasAuthority())
	{
		return;
	}

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ACoopPressurePlate::OnTriggerBeginOverlap);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &ACoopPressurePlate::OnTriggerEndOverlap);
}

void ACoopPressurePlate::ApplyTriggerVolumeSize()
{
	// TriggerVolume is parented to Mesh (the actor's root, non-uniformly scaled to
	// (2.5, 2.5, 0.2) above) -- UBoxComponent::SetBoxExtent and SetRelativeLocation both take
	// *local*, pre-scale units that get multiplied by that inherited scale to produce the real
	// world-space box. Dividing the desired world-space numbers by the actor's own scale here
	// keeps everything below expressed in actual world units instead of silently drifting
	// whenever the plate's scale changes -- a bug the original 125/125/100 box extent already
	// had (it was written as if it were world-space, making the real footprint ~2.5x the plate's
	// visible size) and which this fixes at the same time as the height.
	const FVector PlateScale = GetActorScale3D();

	// The trigger used to be a symmetric 200-unit-tall column (+-100 world units around the
	// plate's own origin) -- tall enough that a character's capsule overlapped it well before
	// their feet reached the plate's surface, and stayed "occupying" it through most of a jump's
	// arc. That read as activating by proximity/being airborne above the plate rather than by
	// physically standing on it. This instead makes the trigger a thin world-space band starting
	// at the plate's own origin (safely inside the solid mesh, whose top surface sits ~10 world
	// units above origin) and extending only CatchHeight world units upward, sized horizontally
	// to match the mesh's actual 125-unit world-space footprint half-extent.
	const float CatchHeight = GameConstants ? GameConstants->PlateTriggerCatchHeightUnits : 30.0f;
	const float WorldFootprintHalfExtent = 125.0f;
	TriggerVolume->SetBoxExtent(FVector(WorldFootprintHalfExtent / PlateScale.X, WorldFootprintHalfExtent / PlateScale.Y, (CatchHeight * 0.5f) / PlateScale.Z));
	TriggerVolume->SetRelativeLocation(FVector(0.0f, 0.0f, (CatchHeight * 0.5f) / PlateScale.Z));
}

void ACoopPressurePlate::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<ACoopCharacter>(OtherActor))
	{
		RefreshOccupancy();
	}
}

void ACoopPressurePlate::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Cast<ACoopCharacter>(OtherActor))
	{
		RefreshOccupancy();
	}
}

void ACoopPressurePlate::RefreshOccupancy()
{
	TArray<AActor*> OverlappingActors;
	TriggerVolume->GetOverlappingActors(OverlappingActors, ACoopCharacter::StaticClass());
	SetOccupied(OverlappingActors.Num() > 0);
}

void ACoopPressurePlate::SetOccupied(bool bNewOccupied)
{
	if (!HasAuthority() || bIsOccupied == bNewOccupied)
	{
		return;
	}

	bIsOccupied = bNewOccupied;

	// OnRep_IsOccupied only fires on remote clients receiving replication -- the server (a Listen
	// Server host included) never gets its own OnRep callback for a property it wrote itself, so it
	// has to apply its own cosmetic response directly here.
	ApplyOccupiedVisual(bIsOccupied);

	OnOccupancyChanged.Broadcast();
}

void ACoopPressurePlate::OnRep_IsOccupied()
{
	ApplyOccupiedVisual(bIsOccupied);
}

void ACoopPressurePlate::ApplyOccupiedVisual(bool bOccupied)
{
	const FLinearColor Color = bOccupied ? FLinearColor(0.1f, 1.0f, 0.1f) : FLinearColor(0.6f, 0.6f, 0.6f);
	if (UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void ACoopPressurePlate::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoopPressurePlate, bIsOccupied);
}
