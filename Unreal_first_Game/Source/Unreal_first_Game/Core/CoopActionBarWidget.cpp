#include "Core/CoopActionBarWidget.h"
#include "Core/CoopAbilitySlotWidget.h"
#include "Core/CoopGameState.h"
#include "Core/CoopMatchPhase.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UCoopActionBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ACoopGameState* GameState = Cast<ACoopGameState>(UGameplayStatics::GetGameState(this));
	const EMatchPhase Phase = GameState ? GameState->GetCurrentPhase() : EMatchPhase::WaitingForRoster;
	const bool bShow = (Phase == EMatchPhase::Prep || Phase == EMatchPhase::HoldTheGate);

	// Show/hide via RenderOpacity, NOT via our own Visibility. This widget is created in
	// ACoopPlayerController::BeginPlay during the RoleSelect phase, so its first NativeTick runs
	// while bShow is false. Slate stops calling Tick on a widget the moment its own visibility
	// leaves the "visible" family (Collapsed AND Hidden both stop it) -- so hiding ourselves here
	// would freeze this NativeTick forever and the bar would never come back once Prep starts
	// (confirmed the hard way in P9). RenderOpacity 0 is fully invisible but keeps the widget
	// ticking, and we stay HitTestInvisible always so the mouse falls through to the camera drag
	// (CLAUDE.md §5) whether shown or not. Opacity multiplies down the tree, so this hides every
	// slot too. The project's other phase-gated HUD widgets use Designer Visibility bindings for
	// this, which re-evaluate even while collapsed -- unreal-mcp can't author those bindings
	// (DECISIONS.md), so RenderOpacity from NativeTick is the equivalent this tooling can build.
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(bShow ? 1.0f : 0.0f);

	// Hover tooltip: whichever of the 3 slots the cursor is over (and that has a kit entry) drives
	// one shared name + description panel above the bar. Geometry-poll (each slot's own NativeTick
	// sets IsCursorOver()) + RenderOpacity toggle -- bar and slots stay HitTestInvisible so the
	// cursor still falls through to the right-click camera drag (CLAUDE.md §5). Same never-touch-our-
	// own-Visibility rule as the show/hide above.
	const UCoopAbilitySlotWidget* Hovered = nullptr;
	if (bShow)
	{
		const UCoopAbilitySlotWidget* Slots[] = { Slot0.Get(), Slot1.Get(), Slot2.Get() };
		for (const UCoopAbilitySlotWidget* S : Slots)
		{
			if (S && S->IsCursorOver() && S->HasAbilityEntry())
			{
				Hovered = S;
				break;
			}
		}
	}
	if (TooltipNameText)
	{
		TooltipNameText->SetText(Hovered ? Hovered->GetAbilityName() : FText::GetEmpty());
	}
	if (TooltipDescText)
	{
		TooltipDescText->SetText(Hovered ? Hovered->GetAbilityDescription() : FText::GetEmpty());
	}
	if (TooltipRoot)
	{
		TooltipRoot->SetRenderOpacity(Hovered ? 1.0f : 0.0f);
	}
}
