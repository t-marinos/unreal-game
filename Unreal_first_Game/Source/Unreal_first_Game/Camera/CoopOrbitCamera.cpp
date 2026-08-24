#include "Camera/CoopOrbitCamera.h"
#include "Core/GameConstants.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"

ACoopOrbitCamera::ACoopOrbitCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	// Explicit per CLAUDE.md §5, even though AActor already defaults to false -- spelling it out
	// so this never accidentally becomes a replicated actor later.
	bReplicates = false;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	RootComponent = SpringArm;
	SpringArm->TargetArmLength = 900.0f;
	SpringArm->SetRelativeRotation(FRotator(OrbitPitch, OrbitYaw, 0.0f));
	SpringArm->bDoCollisionTest = true;
	// This actor is never possessed/controlled -- orbiting is driven entirely by our own
	// OrbitYaw/OrbitPitch state in Tick, not by inheriting anyone's control rotation.
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
}

void ACoopOrbitCamera::Initialize(APlayerController* InOwningController, const UGameConstants* InGameConstants)
{
	OwningController = InOwningController;

	FVector ArenaCenter = FVector::ZeroVector;
	if (InGameConstants)
	{
		SpringArm->TargetArmLength = InGameConstants->CameraArmLength;
		OrbitPitch = InGameConstants->CameraDefaultPitch;
		MinPitch = InGameConstants->CameraMinPitch;
		MaxPitch = InGameConstants->CameraMaxPitch;
		OrbitYawSpeed = InGameConstants->CameraOrbitYawSpeed;
		OrbitPitchSpeed = InGameConstants->CameraOrbitPitchSpeed;
		ArenaCenter = InGameConstants->ArenaCenterLocation;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ACoopOrbitCamera::Initialize got no GameConstants -- using hardcoded fallback camera tuning. Assign DA_GameConstants on ACoopPlayerController's CDO."));
	}

	SetActorLocation(ArenaCenter);
	SpringArm->SetRelativeRotation(FRotator(OrbitPitch, OrbitYaw, 0.0f));
}

void ACoopOrbitCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* Controller = OwningController.Get();
	if (!Controller || !Controller->IsInputKeyDown(EKeys::RightMouseButton))
	{
		return;
	}

	float MouseDeltaX = 0.0f;
	float MouseDeltaY = 0.0f;
	Controller->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);

	OrbitYaw += MouseDeltaX * OrbitYawSpeed;
	OrbitPitch = FMath::Clamp(OrbitPitch - MouseDeltaY * OrbitPitchSpeed, MinPitch, MaxPitch);

	SpringArm->SetRelativeRotation(FRotator(OrbitPitch, OrbitYaw, 0.0f));
}
