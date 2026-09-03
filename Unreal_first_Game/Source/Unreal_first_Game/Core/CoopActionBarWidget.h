#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "CoopActionBarWidget.generated.h"

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
};
