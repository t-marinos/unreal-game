#include "Camera/CoopOrbitCamera.h"
#include "Core/GameConstants.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

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

	if (InGameConstants)
	{
		SpringArm->TargetArmLength = InGameConstants->CameraArmLength;
		OrbitPitch = InGameConstants->CameraDefaultPitch;
		MinPitch = InGameConstants->CameraMinPitch;
		MaxPitch = InGameConstants->CameraMaxPitch;
		OrbitYawSpeed = InGameConstants->CameraOrbitYawSpeed;
		OrbitPitchSpeed = InGameConstants->CameraOrbitPitchSpeed;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ACoopOrbitCamera::Initialize got no GameConstants -- using hardcoded fallback camera tuning. Assign DA_GameConstants on ACoopPlayerController's CDO."));
	}

	// Starting pivot: wherever the controller's pawn already is, if it has one yet (falls back to
	// the world origin otherwise). Tick's own pawn-tracking below corrects this the moment a pawn
	// exists, so a null pawn here is only ever visible for a single frame at worst.
	if (const APawn* Pawn = InOwningController ? InOwningController->GetPawn() : nullptr)
	{
		SetActorLocation(Pawn->GetActorLocation());
	}

	SpringArm->SetRelativeRotation(FRotator(OrbitPitch, OrbitYaw, 0.0f));
}

void ACoopOrbitCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* Controller = OwningController.Get();
	if (!Controller)
	{
		return;
	}

	// Reverses CLAUDE.md §5's original "never follows a player" rule (DECISIONS.md's "Camera
	// follows the player" entry) -- the pivot now tracks whichever pawn this controller currently
	// possesses. Read fresh every tick rather than cached, so a dev-mode Possess() swap onto a
	// different dummy picks up the new pawn for free with no extra plumbing.
	if (APawn* Pawn = Controller->GetPawn())
	{
		SetActorLocation(Pawn->GetActorLocation());
	}

	if (!Controller->IsInputKeyDown(EKeys::RightMouseButton))
	{
		return;
	}

	float MouseDeltaX = 0.0f;
	float MouseDeltaY = 0.0f;
	Controller->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);

	OrbitYaw += MouseDeltaX * OrbitYawSpeed;
	// + not -: dragging the mouse up should pitch the camera up (toward MaxPitch, less steeply
	// down), matching the non-inverted convention players expect. Was flipped -- caught by
	// playtesting, not something the earlier reflection-only verification could have caught.
	OrbitPitch = FMath::Clamp(OrbitPitch + MouseDeltaY * OrbitPitchSpeed, MinPitch, MaxPitch);

	SpringArm->SetRelativeRotation(FRotator(OrbitPitch, OrbitYaw, 0.0f));
}
