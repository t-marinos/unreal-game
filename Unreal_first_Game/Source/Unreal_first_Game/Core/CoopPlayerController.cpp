#include "Core/CoopPlayerController.h"
#include "Core/GameConstants.h"
#include "Camera/CoopOrbitCamera.h"
#include "Blueprint/UserWidget.h"

ACoopPlayerController::ACoopPlayerController()
{
	// Without this, APlayerController::OnPossess snaps the view target back to the possessed pawn
	// on every (re)possession (see AutoManageActiveCameraTarget), which would silently undo
	// BeginPlay's SetViewTarget(OrbitCamera) below the moment a pawn is possessed. We manage the
	// view target ourselves per CLAUDE.md §5 -- the camera must never follow a player.
	bAutoManageActiveCameraTarget = false;
}

void ACoopPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Local-only camera (CLAUDE.md §5): only the machine actually controlling this specific
	// PlayerController should spawn one. On a Listen Server there are up to 5 PlayerController
	// instances with HasAuthority()==true, but only the host's own is IsLocalController() here;
	// each remote client likewise only ever has its own single PlayerController, always local to
	// itself. This keeps the camera per-client with zero cross-client effect, by construction.
	if (!IsLocalController())
	{
		return;
	}

	OrbitCamera = GetWorld()->SpawnActor<ACoopOrbitCamera>();
	if (OrbitCamera)
	{
		OrbitCamera->Initialize(this, GameConstants);
		SetViewTarget(OrbitCamera);
	}

	// M6: one shared visible timer (CLAUDE.md §7). The widget itself only ever reads
	// ACoopGameState::GetElapsedMatchTime() -- no gameplay state lives here, purely local display.
	if (MatchTimerWidgetClass)
	{
		MatchTimerWidget = CreateWidget<UUserWidget>(this, MatchTimerWidgetClass);
		if (MatchTimerWidget)
		{
			MatchTimerWidget->AddToViewport();
		}
	}
}
