#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Core/CoopRoleTypes.h"
#include "CoopAbilitySlotWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;
class UGameConstants;

// One square of the bottom-screen action bar (WoW-style). Reused 3x inside WBP_ActionBar with a
// per-instance SlotIndex 0/1/2, exactly like UCoopAbilityCardWidget's CardIndex. All feedback is
// driven from NativeTick against BindWidgetOptional pointers -- no Designer "Bind Function"
// bindings, so the whole widget is buildable/verifiable through unreal-mcp (same reasoning as the
// RoleSelect follow-on, DECISIONS.md).
//
// Slot 0 is the role's implemented "Q ability" (Shield/Speed/Dash/Stabilize/Execution): full
// colour, a "Q" keybind badge, a live cooldown sweep. Slots 1-2 are the role's remaining
// specced-but-unbuilt kit -- greyed tile, no keybind, no cooldown -- present so the bar reads as a
// real kit and future ability work has an obvious seam.
UCLASS()
class UNREAL_FIRST_GAME_API UCoopAbilitySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Which entry (0-2) of the local player's role kit this slot shows. Set per-instance in the
	// WBP_ActionBar Designer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Bar")
	int32 SlotIndex = 0;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	EPlayerRole GetLocalPlayerRole() const;

	// Cooldown end (absolute server time, CLAUDE.md §4.5) for THIS role's slot-0 ability, read off
	// the local player's own pawn. Returns -1 if unavailable. One explicit case per role -- no
	// generic map (CLAUDE.md §4.6), matching every ability namespace in the project.
	float GetSlotZeroCooldownEndServerTime(EPlayerRole Role) const;
	float GetSlotZeroCooldownDurationSeconds(EPlayerRole Role) const;

	// DA_GameConstants, set on WBP_AbilitySlot's CDO (same content-wiring pattern as
	// ACoopPlayerController::GameConstants and every scene's own reference).
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;

	// --- WBP_AbilitySlot children, matched by name ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> TileBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeybindText;

	// UImage whose brush material is M_CooldownSweep. NativeConstruct grabs one dynamic instance;
	// NativeTick pushes the 0-1 "Progress" scalar every frame.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CooldownImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownSecondsText;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CooldownMID;
};
