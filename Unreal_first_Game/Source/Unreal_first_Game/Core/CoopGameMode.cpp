#include "Core/CoopGameMode.h"
#include "Core/CoopPlayerState.h"
#include "Core/GameConstants.h"
#include "Dev/DummyAIController.h"
#include "GameFramework/GameStateBase.h"

ACoopGameMode::ACoopGameMode()
{
	// Wire the GameMode's own class defaults to the Coop skeletons now, ahead of any of
	// them having real logic, so the pattern is established early. DefaultPawnClass,
	// PlayerControllerClass, and GameStateClass are deliberately left untouched here --
	// all three need a Blueprint wrapper (BP_PlayerCharacter since M4, BP_PlayerController
	// since M5, BP_GameState since M6) so a UPROPERTY asset reference on them (mesh/
	// animation, GameConstants) can be set via content wiring instead of hardcoded in C++.
	// Until something points the project's GlobalDefaultGameMode at this class (M3), none
	// of this is live in PIE.
	PlayerStateClass = ACoopPlayerState::StaticClass();
}

void ACoopGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	int32 MaxPlayers = FallbackMaxPlayers;
	if (GameConstants)
	{
		MaxPlayers = GameConstants->MaxPlayers;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ACoopGameMode has no GameConstants assigned -- falling back to a hardcoded MaxPlayers of %d. Assign DA_GameConstants on BP_GameMode."), FallbackMaxPlayers);
	}

	// GameState->PlayerArray only holds players who have already completed login, so this
	// is exactly "how many are in the session right now," checked before letting a 6th
	// player in. Don't overwrite an earlier rejection reason if one is already set.
	if (ErrorMessage.IsEmpty() && GameState && GameState->PlayerArray.Num() >= MaxPlayers)
	{
		ErrorMessage = FString::Printf(TEXT("Session is full (%d/%d players)."), GameState->PlayerArray.Num(), MaxPlayers);
	}
}

void ACoopGameMode::BeginPlay()
{
	Super::BeginPlay();

	// -devmode lets a packaged Development build (CLAUDE.md §3.1) opt in without an Editor Class
	// Defaults edit; bDevMode can also just be checked directly on BP_GameMode for PIE testing.
	if (FParse::Param(FCommandLine::Get(), TEXT("devmode")))
	{
		bDevMode = true;
	}

	if (bDevMode)
	{
		FillEmptySlotsWithDummies();
	}
}

void ACoopGameMode::FillEmptySlotsWithDummies()
{
	int32 MaxPlayers = FallbackMaxPlayers;
	if (GameConstants)
	{
		MaxPlayers = GameConstants->MaxPlayers;
	}

	const int32 CurrentPlayers = GameState ? GameState->PlayerArray.Num() : 0;
	const int32 DummiesToSpawn = FMath::Max(0, MaxPlayers - CurrentPlayers);

	const AActor* SpawnPoint = FindPlayerStart(nullptr);
	const FTransform SpawnTransform = SpawnPoint ? SpawnPoint->GetActorTransform() : FTransform::Identity;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < DummiesToSpawn; ++Index)
	{
		APawn* DummyPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, SpawnTransform, SpawnParams);
		ADummyAIController* DummyController = GetWorld()->SpawnActor<ADummyAIController>();
		if (DummyPawn && DummyController)
		{
			DummyController->Possess(DummyPawn);
			DummyController->SetBehavior(EDummyBehavior::Idle);
			++SpawnedCount;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FillEmptySlotsWithDummies: failed to spawn dummy %d/%d."), Index + 1, DummiesToSpawn);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("FillEmptySlotsWithDummies: spawned %d/%d dummies (MaxPlayers=%d, CurrentPlayers=%d)."), SpawnedCount, DummiesToSpawn, MaxPlayers, CurrentPlayers);
}
