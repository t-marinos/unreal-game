#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Core/CoopRoleTypes.h"
#include "CoopAbilityCardWidget.generated.h"

// Build 1, M5. C++ base for WBP_AbilityCard. One instance per ability-card slot (up to 4, per
// CLAUDE.md §6.3) is placed inside WBP_PrepArenaHUD with CardIndex set 0-3 in the Designer's
// Details panel. Reads the local player's role from their own PlayerState, then looks up that
// role's hardcoded ability list (see the .cpp) -- this is UI display *data*, not gameplay
// resolution, so hardcoding it here doesn't trip CLAUDE.md §4.6's "no generic ability system"
// rule, same reasoning as UCoopSynergyHintWidget's hardcoded Tank/Control hint. SUPPORT/RUNNER/
// DAMAGE only get card *text* in Build 1 (plan's confirmed "stub-only" scope) -- these cards
// never call an ability, they only describe one.
UCLASS()
class UNREAL_FIRST_GAME_API UCoopAbilityCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Which slot (0-3) of the local player's current role's ability list this card instance shows.
	// Set per-instance in the WBP_PrepArenaHUD Designer -- four WBP_AbilityCard children, indices
	// 0/1/2/3.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Card")
	int32 CardIndex = 0;

	// Bind a container panel's Visibility property directly to this (Designer "Bind Function").
	// Collapsed once CardIndex is past the local player's role's actual ability count -- Tank/
	// Support/Damage only have 2 specced abilities each, Control has 3, so slots past that simply
	// have nothing to show rather than inventing placeholder content (CLAUDE.md §4.6: "do not
	// treat a TBD line as permission to invent details ad hoc").
	UFUNCTION(BlueprintPure, Category = "Ability Card")
	ESlateVisibility GetCardVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Ability Card")
	FText GetCardName() const;

	UFUNCTION(BlueprintPure, Category = "Ability Card")
	FText GetCardDescription() const;

private:
	// Local player's currently chosen role, or Unassigned if the PlayerState/role isn't resolved
	// yet (e.g. this widget ticks for a frame before RoleSelect resolves).
	EPlayerRole GetLocalPlayerRole() const;
};
