#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/CoopRoleTypes.h"
#include "CoopSynergyHintWidget.generated.h"

// Build 1, M5. C++ base for WBP_TeamSynergiesPanel. Holds exactly one hardcoded hint row
// (Tank/Control -- the only synergy Build 1 teaches, Fortress), per CLAUDE.md §4.6: this is UI
// hint *data*, not a gameplay-resolution system, so it doesn't need (and must not become) a
// generic synergy framework. CLAUDE.md §6.3: reveals that a relationship exists, never the
// solution -- players must talk to work out Shield + Stabilize -> Fortress themselves.
UCLASS()
class UNREAL_FIRST_GAME_API UCoopSynergyHintWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Synergy")
	EPlayerRole GetHintRoleA() const { return EPlayerRole::Tank; }

	UFUNCTION(BlueprintPure, Category = "Synergy")
	EPlayerRole GetHintRoleB() const { return EPlayerRole::Control; }

	// Deliberately vague -- names the relationship, not the mechanic.
	UFUNCTION(BlueprintPure, Category = "Synergy")
	FText GetHintText() const;
};
