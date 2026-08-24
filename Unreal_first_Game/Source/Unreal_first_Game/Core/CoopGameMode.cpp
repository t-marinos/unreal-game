#include "Core/CoopGameMode.h"
#include "Core/CoopGameState.h"
#include "Core/CoopPlayerState.h"
#include "Core/CoopPlayerController.h"
#include "Core/GameConstants.h"
#include "GameFramework/GameStateBase.h"

ACoopGameMode::ACoopGameMode()
{
	// Wire the GameMode's own class defaults to the Coop skeletons now, ahead of any of
	// them having real logic, so the pattern is established early. DefaultPawnClass is
	// deliberately left untouched here -- that's M4's job, once BP_PlayerCharacter (a
	// Mannequin-based reparent of ACoopCharacter) actually exists. Until then, and until
	// something points the project's GlobalDefaultGameMode at this class (M3), none of
	// this is live in PIE, which is why M2 introduces no visual/gameplay change.
	GameStateClass = ACoopGameState::StaticClass();
	PlayerStateClass = ACoopPlayerState::StaticClass();
	PlayerControllerClass = ACoopPlayerController::StaticClass();
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
