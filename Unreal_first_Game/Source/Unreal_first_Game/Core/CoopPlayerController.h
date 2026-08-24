#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CoopPlayerController.generated.h"

// Empty for now. Will hold the button/effect Server RPC (M7) and the DumpGameState
// exec command (M8) -- both live here because Server RPCs and Exec commands are
// PlayerController responsibilities in Unreal (each player has exactly one).
UCLASS()
class UNREAL_FIRST_GAME_API ACoopPlayerController : public APlayerController
{
	GENERATED_BODY()
};
