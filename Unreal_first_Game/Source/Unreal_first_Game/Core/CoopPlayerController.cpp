#include "Core/CoopPlayerController.h"
#include "Core/GameConstants.h"
#include "Core/CoopGameState.h"
#include "Core/CoopPlayerState.h"
#include "Core/CoopGameMode.h"
#include "Core/CoopCharacter.h"
#include "Core/CoopHealthComponent.h"
#include "Dev/DummyAIController.h"
#include "Camera/CoopOrbitCamera.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

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

	// Build 1, M5: role-select screen and prep-arena HUD (ability cards + synergy hint). Both are
	// created once here and stay in the viewport for the whole match, same reasoning as
	// MatchTimerWidget above -- each one's own Visibility binding hides it outside its phase.
	if (RoleSelectWidgetClass)
	{
		RoleSelectWidget = CreateWidget<UUserWidget>(this, RoleSelectWidgetClass);
		if (RoleSelectWidget)
		{
			RoleSelectWidget->AddToViewport();
		}
	}

	if (PrepArenaHUDWidgetClass)
	{
		PrepArenaHUDWidget = CreateWidget<UUserWidget>(this, PrepArenaHUDWidgetClass);
		if (PrepArenaHUDWidget)
		{
			PrepArenaHUDWidget->AddToViewport();
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

void ACoopPlayerController::PossessDummy(int32 Index)
{
	Server_PossessDummy(Index);
}

void ACoopPlayerController::Server_PossessDummy_Implementation(int32 Index)
{
	TArray<ADummyAIController*> Dummies;
	for (TActorIterator<ADummyAIController> It(GetWorld()); It; ++It)
	{
		Dummies.Add(*It);
	}

	if (!Dummies.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("PossessDummy: index %d out of range (found %d dummies)."), Index, Dummies.Num());
		return;
	}

	APawn* DummyPawn = Dummies[Index]->GetPawn();
	if (!DummyPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("PossessDummy: dummy %d has no pawn."), Index);
		return;
	}

	// Swap: this controller takes the dummy's pawn, and the dummy controller takes back this
	// controller's old pawn (so it isn't left standing uncontrolled) -- CLAUDE.md §7's
	// "Possess/UnPossess swap (built-in Unreal API, no custom prediction needed)".
	APawn* PreviousPawn = GetPawn();
	Possess(DummyPawn);
	if (PreviousPawn)
	{
		Dummies[Index]->Possess(PreviousPawn);
		Dummies[Index]->SetBehavior(EDummyBehavior::Idle);
	}
}

void ACoopPlayerController::SceneSkip()
{
	// Stub -- CLAUDE.md §7: Build 0 has no scenes yet. Real and working, wired up now so Build 1
	// only needs to give it content, not build the command itself.
	UE_LOG(LogTemp, Log, TEXT("SceneSkip: stub, no scenes exist yet (Build 1 will wire this up)."));
}

void ACoopPlayerController::ToggleGodMode()
{
	Server_ToggleGodMode();
}

void ACoopPlayerController::Server_ToggleGodMode_Implementation()
{
	if (ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>())
	{
		CoopPS->SetInvulnerable(!CoopPS->IsInvulnerable());
		UE_LOG(LogTemp, Log, TEXT("ToggleGodMode: %s is now %s."), *CoopPS->GetPlayerName(), CoopPS->IsInvulnerable() ? TEXT("invulnerable") : TEXT("vulnerable"));
	}
}

void ACoopPlayerController::Server_ClaimRole_Implementation(EPlayerRole DesiredRole)
{
	ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>();
	ACoopGameMode* CoopGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACoopGameMode>() : nullptr;
	if (CoopPS && CoopGameMode)
	{
		CoopGameMode->TryClaimRole(CoopPS, DesiredRole);
	}
}

void ACoopPlayerController::ApplyTestDamage(float Amount)
{
	Server_ApplyTestDamage(Amount);
}

void ACoopPlayerController::Server_ApplyTestDamage_Implementation(float Amount)
{
	if (ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(GetPawn()))
	{
		if (UCoopHealthComponent* Health = CoopCharacter->GetHealthComponent())
		{
			Health->ApplyDamage(Amount);
			UE_LOG(LogTemp, Log, TEXT("ApplyTestDamage: %s took %.1f, now %.1f/%.1f."),
				*GetNameSafe(CoopCharacter), Amount, Health->GetCurrentHealth(), Health->GetMaxHealth());
		}
	}
}
