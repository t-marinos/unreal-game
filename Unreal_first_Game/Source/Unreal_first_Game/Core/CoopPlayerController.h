#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CoopPlayerController.generated.h"

class UGameConstants;
class ACoopOrbitCamera;
class UUserWidget;

// Will also hold the button/effect Server RPC (M7) and the DumpGameState exec command (M8) --
// both live here because Server RPCs and Exec commands are PlayerController responsibilities in
// Unreal (each player has exactly one).
UCLASS()
class UNREAL_FIRST_GAME_API ACoopPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACoopPlayerController();

protected:
	virtual void BeginPlay() override;

private:
	// Every tunable lives in DA_GameConstants per CLAUDE.md §10. Unlike ACoopGameMode, this class
	// has no Blueprint wrapper -- assign this directly on ACoopPlayerController's own CDO via
	// unreal-mcp's ObjectTools (a plain C++ UCLASS' CDO is just as reachable that way as a
	// Blueprint's, no wrapper asset needed).
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;

	// Spawned once in BeginPlay, only on the machine actually controlling this PlayerController
	// (CLAUDE.md §5's local-only camera). Never replicated -- see ACoopOrbitCamera.
	UPROPERTY()
	TObjectPtr<ACoopOrbitCamera> OrbitCamera;

	// M6: WBP_MatchTimer, content-wired on BP_PlayerController's CDO. Purely a local read of
	// ACoopGameState::GetElapsedMatchTime() -- no gameplay data lives on the widget itself.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MatchTimerWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MatchTimerWidget;
};
