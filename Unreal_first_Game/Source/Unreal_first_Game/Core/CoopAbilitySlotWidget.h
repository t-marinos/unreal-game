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
// per-instance SlotIndex 0/1/2 set in the Designer's Details panel. All feedback is driven from
// NativeTick against BindWidgetOptional pointers -- no Designer "Bind Function" bindings, so the
// whole widget is buildable/verifiable through unreal-mcp (same reasoning as the RoleSelect
// follow-on, DECISIONS.md).
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

	// Read by UCoopActionBarWidget each tick to drive the shared hover tooltip. Name / description
	// come from this slot's (local role, SlotIndex) kit entry -- descriptions are duplicated into
	// CoopAbilitySlotWidget.cpp's kit table (same deliberate duplication as the names). IsCursorOver()
	// is a pure geometry poll set in NativeTick: the bar and every slot stay HitTestInvisible so
	// there is no Slate hover event and the cursor still falls through to the camera drag (CLAUDE.md
	// §5 / DECISIONS.md's WoW action bar entry). HasAbilityEntry() is false only for a slot index
	// past the local role's kit (the collapsed third tile).
	FText GetAbilityName() const;
	FText GetAbilityDescription() const;
	bool HasAbilityEntry() const;
	bool IsCursorOver() const { return bCursorOver; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	EPlayerRole GetLocalPlayerRole() const;

	// Cooldown end (absolute server time, CLAUDE.md §4.5) / duration for the slot-InSlotIndex ability
	// of Role, read off the local player's own pawn. Returns -1 / 0 if this (Role, slot) has no
	// implemented ability. One explicit (Role, slot) case each -- no generic map (CLAUDE.md §4.6),
	// matching every ability namespace in the project.
	float GetSlotCooldownEndServerTime(EPlayerRole Role, int32 InSlotIndex) const;
	float GetSlotCooldownDurationSeconds(EPlayerRole Role, int32 InSlotIndex) const;

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

	// Set every NativeTick: is the OS cursor inside this slot's widget rect? Plain bool, not a
	// UPROPERTY -- transient local UI state, read by UCoopActionBarWidget via IsCursorOver().
	bool bCursorOver = false;
};
