#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CoopPlayerController.generated.h"

class UGameConstants;
class ACoopOrbitCamera;
class UUserWidget;

// Server RPCs and Exec commands are PlayerController responsibilities in Unreal (each player has
// exactly one).
UCLASS()
class UNREAL_FIRST_GAME_API ACoopPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACoopPlayerController();

	// M7: intent only, per CLAUDE.md §4.1 -- "I walked into the button," never a result like "the
	// button is now lit." Called by ACoopButton when this controller's own pawn overlaps it.
	UFUNCTION(Server, Reliable)
	void Server_PressButton();

	// M8: console command (type "DumpGameState" in the in-game console on any machine -- server or
	// client). Walks GameState + every PlayerState and logs a JSON-shaped snapshot via UE_LOG, per
	// CLAUDE.md §4.3/§10's desync-debugging workflow: run this on the server, run it again on a
	// disagreeing client, diff the first field that differs.
	UFUNCTION(Exec)
	void DumpGameState();

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
