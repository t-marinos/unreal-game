#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoopPrepCountdownWidget.generated.h"

// Build 1, M5. C++ base for WBP_PrepArenaHUD, mirroring UCoopMatchTimerWidget's reasoning exactly
// (see that class's comment): a BlueprintPure UFUNCTION is required for UMG's "Bind Function" list,
// and a Blueprint-graph function built via unreal-mcp's graph tools has no reflected way to be
// marked pure.
UCLASS()
class UNREAL_FIRST_GAME_API UCoopPrepCountdownWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Seconds remaining in the Prep phase, derived from ACoopGameState::GetPrepPhaseEndServerTime()
	// minus server world time every call -- never a locally ticked countdown (CLAUDE.md §4.5).
	// Returns "0" before the Prep phase has started (end time still the -1 sentinel).
	UFUNCTION(BlueprintPure, Category = "Match")
	FText GetPrepRemainingSecondsText() const;
};
