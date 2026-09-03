#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoopToastWidget.generated.h"

class UTextBlock;
class UGameConstants;

// Ability kit expansion: a centre-screen transient message ("Please choose a target", and any
// future "X failed" cue). Purely local, cosmetic UI (CLAUDE.md §4.2): it only READS two
// non-replicated fields on the owning ACoopPlayerController (GetPendingToastText() /
// GetPendingToastStartTime()) and fades itself.
//
// Same pattern as UCoopActionBarWidget / UCoopUnitFrameWidget: all feedback is NativeTick against a
// BindWidgetOptional pointer -- no Designer "Bind Function" bindings (unreal-mcp can't author them
// -- DECISIONS.md). Show/hide is a RenderOpacity toggle, NEVER this widget's own Visibility: it is
// created (in ACoopPlayerController::BeginPlay) with nothing to show, and a widget that leaves the
// "visible" family in its own NativeTick freezes forever (the P9 action-bar gotcha, DECISIONS.md).
UCLASS()
class UNREAL_FIRST_GAME_API UCoopToastWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// DA_GameConstants, set on WBP_Toast's CDO -- same content-wiring pattern as WBP_AbilitySlot.
	// Only read for ToastDurationSeconds; a null ref falls back to 2.0s.
	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
	TObjectPtr<UGameConstants> GameConstants;

	// WBP_Toast child, matched by name.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;
};
