#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CoopGameState.generated.h"

// Empty for now. Build 0 M6 adds the replicated match-start server timestamp here
// (CLAUDE.md §4.5 -- all timing derives from server time, never client DeltaTime).
UCLASS()
class UNREAL_FIRST_GAME_API ACoopGameState : public AGameStateBase
{
	GENERATED_BODY()
};
