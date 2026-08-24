#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoopMatchTimerWidget.generated.h"

// C++ base for WBP_MatchTimer (M6). Exists purely so GetElapsedMatchTimeText() is a genuine
// BlueprintPure UFUNCTION: UMG's per-property "Bind Function" list only shows pure (zero-exec-pin)
// functions, and a Blueprint-graph function built via unreal-mcp's graph tools has no reflected way
// to be marked pure (that's a Details-panel-only checkbox in the graph editor, not a settable
// property) -- any graph using a Cast node picks up exec pins and stays impure regardless. A C++
// BlueprintPure function sidesteps that entirely.
UCLASS()
class UNREAL_FIRST_GAME_API UCoopMatchTimerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Match")
	FText GetElapsedMatchTimeText() const;
};
