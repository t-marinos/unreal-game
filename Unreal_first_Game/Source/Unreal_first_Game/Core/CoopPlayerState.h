#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CoopPlayerState.generated.h"

// Empty for now. Establishes the pattern early -- Build 1 adds each player's assigned
// role and gameplay tags here, replicated per CLAUDE.md §4.3 so state stays inspectable.
UCLASS()
class UNREAL_FIRST_GAME_API ACoopPlayerState : public APlayerState
{
	GENERATED_BODY()
};
