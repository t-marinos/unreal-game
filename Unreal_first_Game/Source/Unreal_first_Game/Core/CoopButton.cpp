#include "Core/CoopButton.h"
#include "Core/CoopGameState.h"
#include "Core/CoopPlayerController.h"
#include "Core/CoopCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ACoopButton::ACoopButton()
{
	PrimaryActorTick.bCanEverTick = true;

	// Purely a cosmetic responder to ACoopGameState's replicated bool -- it has no state of its
	// own worth replicating.
	bReplicates = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// Hardcoded engine content, not a per-instance Blueprint tunable -- a plain cube/unlit-colour
	// material is exactly CLAUDE.md §5's "everything readable, nothing pretty" bar for Build 0.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMeshFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ButtonMaterialFinder(TEXT("/Game/Materials/M_CoopButton.M_CoopButton"));
	if (ButtonMaterialFinder.Succeeded())
	{
		Mesh->SetMaterial(0, ButtonMaterialFinder.Object);
	}
	SetActorScale3D(FVector(2.0f, 2.0f, 0.5f));

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(RootComponent);
	TriggerVolume->SetBoxExtent(FVector(150.0f, 150.0f, 150.0f));
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ACoopButton::BeginPlay()
{
	Super::BeginPlay();
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ACoopButton::OnTriggerBeginOverlap);
}

void ACoopButton::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only the client that actually owns the overlapping pawn should fire the RPC -- overlap events
	// also fire for simulated proxies (other clients' view of this same pawn), and we don't want
	// every machine independently trying to press the button on behalf of a pawn it doesn't own.
	ACoopCharacter* Character = Cast<ACoopCharacter>(OtherActor);
	if (!Character || !Character->IsLocallyControlled())
	{
		return;
	}

	if (ACoopPlayerController* PC = Cast<ACoopPlayerController>(Character->GetController()))
	{
		PC->Server_PressButton();
	}
}

void ACoopButton::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const ACoopGameState* GameState = GetWorld()->GetGameState<ACoopGameState>();
	if (!GameState)
	{
		return;
	}

	const bool bPressed = GameState->IsButtonPressed();
	if (bPressed != bLastAppliedPressedState)
	{
		bLastAppliedPressedState = bPressed;
		ApplyPressedVisual(bPressed);
	}
}

void ACoopButton::ApplyPressedVisual(bool bPressed)
{
	const FLinearColor Color = bPressed ? FLinearColor(0.1f, 1.0f, 0.1f) : FLinearColor(0.6f, 0.6f, 0.6f);
	if (UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	}
}
