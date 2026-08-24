#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoopOrbitCamera.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UGameConstants;
class APlayerController;

// Local-only, per-player orbit camera (CLAUDE.md §5). ACoopPlayerController spawns one of these
// only on the machine that actually controls it (IsLocalController()), so on a Listen Server the
// host gets exactly one and each remote client spawns their own, independently, on their own
// machine -- never replicated, and no client's camera can affect any other client's view. It
// orbits around a fixed pivot (the arena center) and never follows a player; only the viewing
// angle changes, per §5's "camera still never follows a player" rule.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopOrbitCamera : public AActor
{
	GENERATED_BODY()

public:
	ACoopOrbitCamera();

	// Called once by ACoopPlayerController right after spawning this. GameConstants may be null
	// (falls back to hardcoded defaults with a log warning) -- a plain, non-Blueprint-wrapped AActor
	// has no Class Defaults panel of its own to assign a DA_GameConstants reference on directly.
	void Initialize(APlayerController* InOwningController, const UGameConstants* InGameConstants);

	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	// Whose right-click-drag input drives the orbit. Never replicated -- this actor only ever
	// exists on the one machine that spawned it.
	TWeakObjectPtr<APlayerController> OwningController;

	// Current orbit angles, advanced by right-click-drag in Tick. Purely local input state, not
	// gameplay -- per CLAUDE.md §4.2 this is exactly the kind of cosmetic, client-side-only state
	// that's fine to compute locally with no server involvement.
	float OrbitYaw = 0.0f;
	float OrbitPitch = -50.0f;
	float MinPitch = -80.0f;
	float MaxPitch = -20.0f;
	float OrbitYawSpeed = 0.5f;
	float OrbitPitchSpeed = 0.5f;
};
