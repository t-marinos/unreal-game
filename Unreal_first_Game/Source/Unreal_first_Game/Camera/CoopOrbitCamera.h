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
// machine -- never replicated, and no client's camera can affect any other client's view. Its
// pivot tracks whichever pawn the owning controller currently possesses (Tick reads GetPawn()
// fresh every frame, so a dev-mode Possess() swap picks it up for free); the orbit angle around
// that pivot is still purely local input state driven by right-click-drag. See DECISIONS.md's
// "Camera follows the player" entry -- this reverses §5's original "never follows a player" rule.
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

	// Edge-detects the right-mouse-button so Tick can hide the cursor the frame RMB goes down and
	// show it again the frame it comes up (WoW-style: cursor vanishes while you're turning the
	// camera). Cursor visibility is really a PlayerController concern, but this actor already reads
	// the exact same RMB state every frame for the orbit, so the toggle lives here rather than
	// adding a Tick to the controller just for it.
	bool bWasOrbiting = false;

	// Hiding the cursor is not enough: with GameAndUI + DoNotLock (ACoopPlayerController::BeginPlay)
	// the hidden OS cursor still tracks the physical mouse during the drag, so on release it would
	// reappear wherever the drag ended, not where it started (playtest report, 2026-09-03). We
	// snapshot its viewport position on the press edge and warp it back there on the release edge.
	// SavedCursor* are only meaningful when bHasSavedCursorPos is true (GetMousePosition can fail if
	// the cursor wasn't over the viewport at press time).
	// Known limitation: because the cursor is not locked *during* the drag, a very large sweep can
	// push it into the viewport edge and stall the orbit until you drag back -- fix that with a
	// per-tick re-centre only if a playtest actually finds it annoying.
	bool bHasSavedCursorPos = false;
	float SavedCursorX = 0.0f;
	float SavedCursorY = 0.0f;
};
