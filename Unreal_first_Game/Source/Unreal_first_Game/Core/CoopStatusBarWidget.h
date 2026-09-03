#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Styling/SlateColor.h"
#include "CoopStatusBarWidget.generated.h"

class ACoopCharacter;

// Build 1. C++ base for WBP_StatusBar -- CLAUDE.md §5's "coloured bars above each character, a 3D
// Widget Component (UMG) attached to the actor and billboarded to camera" applied to
// Status.Shielded/Status.Fortress. One instance per ACoopCharacter, spawned by that character's own
// UWidgetComponent (see ACoopCharacter::StatusBarWidgetComponent). It learns which actor it belongs
// to via an explicit SetOwningCharacter call made once from ACoopCharacter::BeginPlay, not any
// implicit outer-chain cast or Designer-set index.
UCLASS()
class UNREAL_FIRST_GAME_API UCoopStatusBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOwningCharacter(ACoopCharacter* InOwningCharacter);

	// Bind one Text Block's Text/ColorAndOpacity/Visibility to these three (Designer "Bind
	// Function", same mechanism WBP_RoleSelect / WBP_PrepArenaHUD use for their bound getters).
	// Fortress is checked first in all three -- a character never holds both tags at once (see
	// CoopControlAbilities::ResolveStabilize, which always clears Status.Shielded before applying
	// Status.Fortress), but checking Fortress first is correct either way. Reads OwningCharacter's
	// tags fresh on every call, never caches -- same pattern as every other widget in this project.
	UFUNCTION(BlueprintPure, Category = "Status")
	FText GetStatusText() const;

	UFUNCTION(BlueprintPure, Category = "Status")
	FSlateColor GetStatusColor() const;

	// Collapsed whenever neither tag is present -- the whole badge disappears, not just the text.
	UFUNCTION(BlueprintPure, Category = "Status")
	ESlateVisibility GetStatusVisibility() const;

private:
	// Never replicated -- purely local, cosmetic UI state.
	TWeakObjectPtr<ACoopCharacter> OwningCharacter;
};
