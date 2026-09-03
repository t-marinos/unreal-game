#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "CoopActionBarWidget.generated.h"

class UCoopAbilitySlotWidget;
class UWidget;
class UTextBlock;

// C++ base for WBP_ActionBar -- the persistent bottom-screen ability bar. Created once in
// ACoopPlayerController::BeginPlay alongside the other HUD widgets and left in the viewport;
// NativeTick shows/hides it off the replicated match phase (visible during Prep and HoldTheGate)
// -- same NativeTick-not-Designer-bindings approach as the RoleSelect follow-on. It hides via
// RenderOpacity, not Visibility, because a widget that Collapses/Hides itself in its own tick
// freezes (Slate stops ticking it) and this one's first tick lands during RoleSelect -- see the
// .cpp comment and DECISIONS.md's "WoW-style action bar" entry. The slots themselves are
// UCoopAbilitySlotWidget instances placed in the WBP; this class only owns show/hide.
UCLASS()
class UNREAL_FIRST_GAME_API UCoopActionBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// The 3 placed WBP_AbilitySlot instances -- bound by name (Slot0/Slot1/Slot2 already exist as
	// WBP_ActionBar variables). Polled each tick for IsCursorOver() / HasAbilityEntry() to drive the
	// hover tooltip. Optional so a missing/renamed slot degrades gracefully rather than failing the
	// BindWidget contract (same reasoning as every other widget in this project).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCoopAbilitySlotWidget> Slot0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCoopAbilitySlotWidget> Slot1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCoopAbilitySlotWidget> Slot2;

	// LoL/WoW-style hover tooltip panel above the bar. Added to WBP_ActionBar's RootCanvas in P3.
	// Show/hide is a RenderOpacity toggle only -- never its own Visibility (the P9 self-hide freeze;
	// this widget is created during RoleSelect like the bar itself).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TooltipRoot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TooltipNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TooltipDescText;
};
