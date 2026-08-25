#include "Core/CoopAbilityCardWidget.h"
#include "Core/CoopPlayerState.h"
#include "GameFramework/PlayerController.h"

namespace
{
	struct FCoopAbilityCardInfo
	{
		FText Name;
		FText Description;
	};

	// Build 1, M5: hardcoded per-role ability card text, sourced directly from docs/abilities.md's
	// specced (non-TBD) abilities only -- CLAUDE.md §4.6 forbids inventing ability details ad hoc,
	// so TBD slots are simply omitted rather than padded with placeholder cards. Text-only, no icon
	// (CLAUDE.md §5: "no VFX... everything readable, nothing pretty" -- icon art is out of scope).
	const TArray<FCoopAbilityCardInfo>& GetAbilitiesForRole(EPlayerRole Role)
	{
		static const TArray<FCoopAbilityCardInfo> Tank = {
			{ NSLOCTEXT("CoopAbilityCard", "TankShieldName", "Shield"),
			  NSLOCTEXT("CoopAbilityCard", "TankShieldDesc", "Raise a barrier in front of you that blocks damage from that direction.") },
			{ NSLOCTEXT("CoopAbilityCard", "TankArmorBreakName", "Armor Break"),
			  NSLOCTEXT("CoopAbilityCard", "TankArmorBreakDesc", "Mark a target, opening a brief window for Control to act on it.") },
		};
		static const TArray<FCoopAbilityCardInfo> Support = {
			{ NSLOCTEXT("CoopAbilityCard", "SupportSpeedName", "Speed"),
			  NSLOCTEXT("CoopAbilityCard", "SupportSpeedDesc", "Grant a teammate a burst of movement speed.") },
			{ NSLOCTEXT("CoopAbilityCard", "SupportLinkName", "Link"),
			  NSLOCTEXT("CoopAbilityCard", "SupportLinkDesc", "Bond yourself to a teammate -- the seed of a wider network.") },
		};
		static const TArray<FCoopAbilityCardInfo> Runner = {
			{ NSLOCTEXT("CoopAbilityCard", "RunnerDashName", "Dash"),
			  NSLOCTEXT("CoopAbilityCard", "RunnerDashDesc", "A short dash. Stronger if a teammate has buffed your speed.") },
			{ NSLOCTEXT("CoopAbilityCard", "RunnerCarryName", "Carry"),
			  NSLOCTEXT("CoopAbilityCard", "RunnerCarryDesc", "Pick up and carry an object.") },
			{ NSLOCTEXT("CoopAbilityCard", "RunnerChainName", "Chain"),
			  NSLOCTEXT("CoopAbilityCard", "RunnerChainDesc", "Fire a tether that pulls you or a target.") },
		};
		static const TArray<FCoopAbilityCardInfo> Control = {
			{ NSLOCTEXT("CoopAbilityCard", "ControlStabilizeName", "Stabilize"),
			  NSLOCTEXT("CoopAbilityCard", "ControlStabilizeDesc", "Cast on a shielded teammate to upgrade their shield for the whole team.") },
			{ NSLOCTEXT("CoopAbilityCard", "ControlMindFractureName", "Mind Fracture"),
			  NSLOCTEXT("CoopAbilityCard", "ControlMindFractureDesc", "Cast on a marked target to reveal the truth.") },
			{ NSLOCTEXT("CoopAbilityCard", "ControlChannelName", "Channel"),
			  NSLOCTEXT("CoopAbilityCard", "ControlChannelDesc", "Cast on a bonded pair to spread the link to the whole team.") },
		};
		static const TArray<FCoopAbilityCardInfo> Damage = {
			{ NSLOCTEXT("CoopAbilityCard", "DamageExecutionName", "Execution"),
			  NSLOCTEXT("CoopAbilityCard", "DamageExecutionDesc", "A finishing strike -- only lands while the target is physically vulnerable.") },
			{ NSLOCTEXT("CoopAbilityCard", "DamageOverloadName", "Overload"),
			  NSLOCTEXT("CoopAbilityCard", "DamageOverloadDesc", "A finishing strike -- only lands while the target is magically vulnerable.") },
		};
		static const TArray<FCoopAbilityCardInfo> Empty;

		switch (Role)
		{
			case EPlayerRole::Tank: return Tank;
			case EPlayerRole::Support: return Support;
			case EPlayerRole::Runner: return Runner;
			case EPlayerRole::Control: return Control;
			case EPlayerRole::Damage: return Damage;
			default: return Empty;
		}
	}
}

EPlayerRole UCoopAbilityCardWidget::GetLocalPlayerRole() const
{
	const APlayerController* PC = GetOwningPlayer();
	const ACoopPlayerState* CoopPS = PC ? PC->GetPlayerState<ACoopPlayerState>() : nullptr;
	return CoopPS ? CoopPS->GetRole() : EPlayerRole::Unassigned;
}

ESlateVisibility UCoopAbilityCardWidget::GetCardVisibility() const
{
	const bool bHasData = GetAbilitiesForRole(GetLocalPlayerRole()).IsValidIndex(CardIndex);
	return bHasData ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

FText UCoopAbilityCardWidget::GetCardName() const
{
	const TArray<FCoopAbilityCardInfo>& Abilities = GetAbilitiesForRole(GetLocalPlayerRole());
	return Abilities.IsValidIndex(CardIndex) ? Abilities[CardIndex].Name : FText::GetEmpty();
}

FText UCoopAbilityCardWidget::GetCardDescription() const
{
	const TArray<FCoopAbilityCardInfo>& Abilities = GetAbilitiesForRole(GetLocalPlayerRole());
	return Abilities.IsValidIndex(CardIndex) ? Abilities[CardIndex].Description : FText::GetEmpty();
}
