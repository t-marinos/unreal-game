#include "Core/CoopStatusBarWidget.h"
#include "Core/CoopCharacter.h"
#include "Tags/CoopGameplayTags.h"

void UCoopStatusBarWidget::SetOwningCharacter(ACoopCharacter* InOwningCharacter)
{
	OwningCharacter = InOwningCharacter;
}

FText UCoopStatusBarWidget::GetStatusText() const
{
	const ACoopCharacter* Character = OwningCharacter.Get();
	if (Character && Character->HasStatusTag(CoopGameplayTags::Status_Fortress))
	{
		return NSLOCTEXT("CoopStatusBar", "Fortress", "FORTRESS");
	}
	if (Character && Character->HasStatusTag(CoopGameplayTags::Status_Shielded))
	{
		return NSLOCTEXT("CoopStatusBar", "Shielded", "SHIELDED");
	}
	return FText::GetEmpty();
}

FSlateColor UCoopStatusBarWidget::GetStatusColor() const
{
	// Hardcoded directly here, not GameConstants -- same precedent as
	// ACoopCharacter::GetColorForPlayerId's per-player tint array: colour is a cosmetic constant,
	// not a gameplay tunable. Irrelevant whenever GetStatusVisibility() is Collapsed, so no third
	// "neither tag" branch is needed.
	const ACoopCharacter* Character = OwningCharacter.Get();
	if (Character && Character->HasStatusTag(CoopGameplayTags::Status_Fortress))
	{
		return FSlateColor(FLinearColor(1.0f, 0.65f, 0.0f)); // Gold/orange.
	}
	return FSlateColor(FLinearColor(0.15f, 0.45f, 1.0f)); // Blue.
}

ESlateVisibility UCoopStatusBarWidget::GetStatusVisibility() const
{
	const ACoopCharacter* Character = OwningCharacter.Get();
	const bool bHasEitherTag = Character &&
		(Character->HasStatusTag(CoopGameplayTags::Status_Fortress) ||
		 Character->HasStatusTag(CoopGameplayTags::Status_Shielded));
	return bHasEitherTag ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}
