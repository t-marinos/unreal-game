#include "Core/CoopGameMode.h"
#include "Core/CoopGameState.h"
#include "Core/CoopPlayerState.h"
#include "Core/GameConstants.h"
#include "GameFramework/GameStateBase.h"

ACoopGameMode::ACoopGameMode()
{
	// Wire the GameMode's own class defaults to the Coop skeletons now, ahead of any of
	// them having real logic, so the pattern is established early. DefaultPawnClass and
	// PlayerControllerClass are deliberately left untouched here -- both need a Blueprint
	// wrapper (BP_PlayerCharacter since M4, BP_PlayerController since M5) so a UPROPERTY
	// asset reference on them (mesh/animation, GameConstants) can be set via content
	// wiring instead of hardcoded in C++. Until something points the project's
	// GlobalDefaultGameMode at this class (M3), none of this is live in PIE.
	GameStateClass = ACoopGameState::StaticClass();
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
