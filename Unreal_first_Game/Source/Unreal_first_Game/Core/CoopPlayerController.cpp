#include "Core/CoopPlayerController.h"
#include "Core/GameConstants.h"
#include "Core/CoopGameState.h"
#include "Camera/CoopOrbitCamera.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

ACoopPlayerController::ACoopPlayerController()
{
	// Without this, APlayerController::OnPossess snaps the view target back to the possessed pawn
	// on every (re)possession (see AutoManageActiveCameraTarget), which would silently undo
	// BeginPlay's SetViewTarget(OrbitCamera) below the moment a pawn is possessed. We manage the
	// view target ourselves per CLAUDE.md §5 -- the camera must never follow a player.
	bAutoManageActiveCameraTarget = false;
}

void ACoopPlayerController::Server_PressButton_Implementation()
{
	// Server-only, per CLAUDE.md §4.1: this is where intent becomes a result. The RPC itself
	// carries no payload beyond "this player pressed a button" -- ACoopGameState is the single
	// source of truth every client's cosmetic response reads from afterward.
	if (ACoopGameState* CoopGameState = GetWorld()->GetGameState<ACoopGameState>())
	{
		CoopGameState->ToggleButtonPressed();
	}
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

void ACoopPlayerController::DumpGameState()
{
	const ACoopGameState* CoopGameState = GetWorld() ? GetWorld()->GetGameState<ACoopGameState>() : nullptr;
	if (!CoopGameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("DumpGameState: no GameState yet."));
		return;
	}

	FString Dump = TEXT("{\n");
	Dump += FString::Printf(TEXT("  \"HasAuthority\": %s,\n"), HasAuthority() ? TEXT("true") : TEXT("false"));
	Dump += FString::Printf(TEXT("  \"ElapsedMatchTime\": %.2f,\n"), CoopGameState->GetElapsedMatchTime());
	Dump += FString::Printf(TEXT("  \"ButtonPressed\": %s,\n"), CoopGameState->IsButtonPressed() ? TEXT("true") : TEXT("false"));
	Dump += TEXT("  \"Players\": [\n");

	const TArray<TObjectPtr<APlayerState>>& Players = CoopGameState->PlayerArray;
	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		const APlayerState* PS = Players[Index];
		if (!PS)
		{
			continue;
		}

		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		if (const APawn* PlayerPawn = PS->GetPawn())
		{
			Location = PlayerPawn->GetActorLocation();
			Velocity = PlayerPawn->GetVelocity();
		}

		Dump += FString::Printf(
			TEXT("    { \"PlayerId\": %d, \"Name\": \"%s\", \"PingMs\": %.0f, \"Location\": {\"X\": %.1f, \"Y\": %.1f, \"Z\": %.1f}, \"Velocity\": {\"X\": %.1f, \"Y\": %.1f, \"Z\": %.1f} }%s\n"),
			PS->GetPlayerId(),
			*PS->GetPlayerName(),
			PS->GetPingInMilliseconds(),
			Location.X, Location.Y, Location.Z,
			Velocity.X, Velocity.Y, Velocity.Z,
			(Index < Players.Num() - 1) ? TEXT(",") : TEXT("")
		);
	}

	Dump += TEXT("  ]\n}");

	UE_LOG(LogTemp, Log, TEXT("DumpGameState:\n%s"), *Dump);
}
