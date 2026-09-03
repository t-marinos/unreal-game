#include "Core/CoopAbilitySlotWidget.h"
#include "Core/CoopPlayerState.h"
#include "Core/CoopCharacter.h"
#include "Core/GameConstants.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Blueprint/WidgetLayoutLibrary.h"

namespace
{
	struct FCoopAbilitySlotInfo
	{
		FText Name;
		FLinearColor TileColor;
		bool bImplemented = false;
		FText Description;
	};

	// Per-role action-bar kit -- the sole home of the ability name + one-line description strings now
	// that the prep-arena ability cards (WBP_AbilityCard / UCoopAbilityCardWidget) are deleted. The
	// bar tile shows the name; the hover tooltip (UCoopActionBarWidget) shows both. Hardcoded UI
	// display data, not gameplay resolution -- CLAUDE.md §4.6's "no generic ability system" doesn't
	// apply (same reasoning the cards used). bImplemented slots: slot 0 (the Q ability) for every
	// role, plus Tank slot 1 (Armor Break, E) and Damage slot 1 (Overload, E) from the ability kit
	// expansion.
	const TArray<FCoopAbilitySlotInfo>& GetKitForRole(EPlayerRole Role)
	{
		static const TArray<FCoopAbilitySlotInfo> Tank = {
			{ NSLOCTEXT("CoopAbilitySlot", "Shield", "Shield"),               FLinearColor(0.20f, 0.40f, 0.65f), true,
			  NSLOCTEXT("CoopAbilitySlot", "ShieldDesc", "Raise a barrier in front of you that blocks damage from that direction.") },
			{ NSLOCTEXT("CoopAbilitySlot", "ArmorBreak", "Armor Break"),      FLinearColor(0.55f, 0.30f, 0.12f), true,
			  NSLOCTEXT("CoopAbilitySlot", "ArmorBreakDesc", "Mark a target, opening a brief window for Control to act on it.") },
		};
		static const TArray<FCoopAbilitySlotInfo> Support = {
			{ NSLOCTEXT("CoopAbilitySlot", "Speed", "Speed"),                 FLinearColor(0.15f, 0.52f, 0.42f), true,
			  NSLOCTEXT("CoopAbilitySlot", "SpeedDesc", "Grant a teammate a burst of movement speed.") },
			{ NSLOCTEXT("CoopAbilitySlot", "Link", "Link"),                   FLinearColor(0.40f, 0.25f, 0.55f), false,
			  NSLOCTEXT("CoopAbilitySlot", "LinkDesc", "Bond yourself to a teammate -- the seed of a wider network.") },
		};
		static const TArray<FCoopAbilitySlotInfo> Runner = {
			{ NSLOCTEXT("CoopAbilitySlot", "Dash", "Dash"),                   FLinearColor(0.42f, 0.52f, 0.15f), true,
			  NSLOCTEXT("CoopAbilitySlot", "DashDesc", "A short dash. Stronger if a teammate has buffed your speed.") },
			{ NSLOCTEXT("CoopAbilitySlot", "Carry", "Carry"),                 FLinearColor(0.45f, 0.35f, 0.22f), false,
			  NSLOCTEXT("CoopAbilitySlot", "CarryDesc", "Pick up and carry an object.") },
			{ NSLOCTEXT("CoopAbilitySlot", "Chain", "Chain"),                 FLinearColor(0.35f, 0.40f, 0.48f), false,
			  NSLOCTEXT("CoopAbilitySlot", "ChainDesc", "Fire a tether that pulls you or a target.") },
		};
		static const TArray<FCoopAbilitySlotInfo> Control = {
			{ NSLOCTEXT("CoopAbilitySlot", "Stabilize", "Stabilize"),         FLinearColor(0.25f, 0.30f, 0.60f), true,
			  NSLOCTEXT("CoopAbilitySlot", "StabilizeDesc", "Cast on a shielded teammate to upgrade their shield for the whole team.") },
			{ NSLOCTEXT("CoopAbilitySlot", "MindFracture", "Mind Fracture"),  FLinearColor(0.55f, 0.20f, 0.45f), false,
			  NSLOCTEXT("CoopAbilitySlot", "MindFractureDesc", "Cast on a marked target to reveal the truth.") },
			{ NSLOCTEXT("CoopAbilitySlot", "Channel", "Channel"),             FLinearColor(0.15f, 0.45f, 0.55f), false,
			  NSLOCTEXT("CoopAbilitySlot", "ChannelDesc", "Cast on a bonded pair to spread the link to the whole team.") },
		};
		static const TArray<FCoopAbilitySlotInfo> Damage = {
			{ NSLOCTEXT("CoopAbilitySlot", "Execution", "Execution"),         FLinearColor(0.62f, 0.16f, 0.20f), true,
			  NSLOCTEXT("CoopAbilitySlot", "ExecutionDesc", "A finishing strike -- only lands while the target is physically vulnerable.") },
			{ NSLOCTEXT("CoopAbilitySlot", "Overload", "Overload"),           FLinearColor(0.45f, 0.20f, 0.55f), true,
			  NSLOCTEXT("CoopAbilitySlot", "OverloadDesc", "A finishing strike -- only lands while the target is magically vulnerable.") },
		};
		static const TArray<FCoopAbilitySlotInfo> Empty;

		switch (Role)
		{
			case EPlayerRole::Tank:    return Tank;
			case EPlayerRole::Support: return Support;
			case EPlayerRole::Runner:  return Runner;
			case EPlayerRole::Control: return Control;
			case EPlayerRole::Damage:  return Damage;
			default:                   return Empty;
		}
	}

	const FLinearColor GreyedTileColor(0.16f, 0.16f, 0.18f);
	const FSlateColor  ImplementedTextColor(FLinearColor::White);
	const FSlateColor  GreyedTextColor(FLinearColor(0.45f, 0.45f, 0.48f));
}

void UCoopAbilitySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// One dynamic instance of M_CooldownSweep for this slot. GetDynamicMaterial() returns null if
	// CooldownImage has no material brush assigned yet -- tolerated, the tick just skips the push.
	if (CooldownImage)
	{
		CooldownMID = CooldownImage->GetDynamicMaterial();
	}
}

EPlayerRole UCoopAbilitySlotWidget::GetLocalPlayerRole() const
{
	const APlayerController* PC = GetOwningPlayer();
	const ACoopPlayerState* PS = PC ? PC->GetPlayerState<ACoopPlayerState>() : nullptr;
	return PS ? PS->GetRole() : EPlayerRole::Unassigned;
}

FText UCoopAbilitySlotWidget::GetAbilityName() const
{
	const TArray<FCoopAbilitySlotInfo>& Kit = GetKitForRole(GetLocalPlayerRole());
	return Kit.IsValidIndex(SlotIndex) ? Kit[SlotIndex].Name : FText::GetEmpty();
}

FText UCoopAbilitySlotWidget::GetAbilityDescription() const
{
	const TArray<FCoopAbilitySlotInfo>& Kit = GetKitForRole(GetLocalPlayerRole());
	return Kit.IsValidIndex(SlotIndex) ? Kit[SlotIndex].Description : FText::GetEmpty();
}

bool UCoopAbilitySlotWidget::HasAbilityEntry() const
{
	return GetKitForRole(GetLocalPlayerRole()).IsValidIndex(SlotIndex);
}

float UCoopAbilitySlotWidget::GetSlotCooldownEndServerTime(EPlayerRole Role, int32 InSlotIndex) const
{
	const APlayerController* PC = GetOwningPlayer();
	const ACoopCharacter* Char = PC ? Cast<ACoopCharacter>(PC->GetPawn()) : nullptr;
	if (!Char)
	{
		return -1.0f;
	}
	// One explicit (Role, slot) case -- no generic map (CLAUDE.md §4.6). Slot 0 is every role's Q
	// ability; slot 1 currently exists only for Tank (Armor Break) and Damage (Overload).
	if (InSlotIndex == 0)
	{
		switch (Role)
		{
			case EPlayerRole::Tank:    return Char->GetShieldCooldownEndServerTime();
			case EPlayerRole::Support: return Char->GetSpeedCooldownEndServerTime();
			case EPlayerRole::Runner:  return Char->GetDashCooldownEndServerTime();
			case EPlayerRole::Control: return Char->GetStabilizeCooldownEndServerTime();
			case EPlayerRole::Damage:  return Char->GetExecutionCooldownEndServerTime();
			default:                   return -1.0f;
		}
	}
	if (InSlotIndex == 1)
	{
		switch (Role)
		{
			case EPlayerRole::Tank:   return Char->GetArmorBreakCooldownEndServerTime();
			case EPlayerRole::Damage: return Char->GetOverloadCooldownEndServerTime();
			default:                  return -1.0f;
		}
	}
	return -1.0f;
}

float UCoopAbilitySlotWidget::GetSlotCooldownDurationSeconds(EPlayerRole Role, int32 InSlotIndex) const
{
	if (!GameConstants)
	{
		return 0.0f;
	}
	if (InSlotIndex == 0)
	{
		switch (Role)
		{
			case EPlayerRole::Tank:    return GameConstants->ShieldCooldownSeconds;
			case EPlayerRole::Support: return GameConstants->SpeedCooldownSeconds;
			case EPlayerRole::Runner:  return GameConstants->DashCooldownSeconds;
			case EPlayerRole::Control: return GameConstants->StabilizeCooldownSeconds;
			case EPlayerRole::Damage:  return GameConstants->ExecutionCooldownSeconds;
			default:                   return 0.0f;
		}
	}
	if (InSlotIndex == 1)
	{
		switch (Role)
		{
			case EPlayerRole::Tank:   return GameConstants->ArmorBreakCooldownSeconds;
			case EPlayerRole::Damage: return GameConstants->OverloadCooldownSeconds;
			default:                  return 0.0f;
		}
	}
	return 0.0f;
}

void UCoopAbilitySlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Pure geometry poll for the hover tooltip -- the bar and every slot are HitTestInvisible so
	// there is no Slate hover event (that is deliberate: the cursor must fall through to the
	// right-click camera drag, CLAUDE.md §5 / DECISIONS.md's WoW action bar entry).
	// GetMousePositionOnPlatform() (UMG, no extra module dep) is absolute desktop space, which is
	// what FGeometry::IsUnderLocation expects. UCoopActionBarWidget reads bCursorOver each tick.
	bCursorOver = MyGeometry.IsUnderLocation(UWidgetLayoutLibrary::GetMousePositionOnPlatform());

	const EPlayerRole Role = GetLocalPlayerRole();
	const TArray<FCoopAbilitySlotInfo>& Kit = GetKitForRole(Role);

	// Nothing to show in this slot. Two very different reasons, handled differently:
	//  - Role is Unassigned: transient -- resolves the instant RoleSelect ends. We must keep
	//    ticking to notice, and Slate freezes Tick on any widget that leaves the "visible"
	//    visibility family (Collapsed AND Hidden both stop it -- confirmed in P9). So stay
	//    HitTestInvisible and just drop RenderOpacity to 0. The whole bar is invisible during
	//    RoleSelect anyway (parent sets its own opacity 0), so the layout space this reserves is
	//    not seen.
	//  - Role is assigned but SlotIndex is past this role's kit (slot 2 on a 2-ability role):
	//    permanent -- the role never grows. Collapsed is correct here: no empty third tile, and
	//    it's fine for Tick to stop since this never needs to come back.
	if (!Kit.IsValidIndex(SlotIndex))
	{
		if (Role == EPlayerRole::Unassigned)
		{
			SetVisibility(ESlateVisibility::HitTestInvisible);
			SetRenderOpacity(0.0f);
		}
		else
		{
			SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(1.0f);

	const FCoopAbilitySlotInfo& Info = Kit[SlotIndex];

	if (NameText)
	{
		NameText->SetText(Info.Name);
		NameText->SetColorAndOpacity(Info.bImplemented ? ImplementedTextColor : GreyedTextColor);
	}
	if (TileBorder)
	{
		TileBorder->SetBrushColor(Info.bImplemented ? Info.TileColor : GreyedTileColor);
	}
	if (KeybindText)
	{
		// Slot 0 is bound to Q for every role, slot 1 to E (Tank Armor Break / Damage Overload) --
		// DECISIONS.md "The Q ability per role" + the ability kit expansion. Only an implemented
		// slot gets a badge.
		const bool bShowKey = Info.bImplemented && (SlotIndex == 0 || SlotIndex == 1);
		KeybindText->SetText(FText::FromString(SlotIndex == 0 ? TEXT("Q") : TEXT("E")));
		KeybindText->SetVisibility(bShowKey ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// --- Cooldown sweep: any implemented slot ---
	float Progress = 0.0f;
	int32 SecondsLeft = 0;
	if (Info.bImplemented)
	{
		const AGameStateBase* GS = UGameplayStatics::GetGameState(this);
		const float Now = GS ? GS->GetServerWorldTimeSeconds() : 0.0f;
		const float End = GetSlotCooldownEndServerTime(Role, SlotIndex);
		const float Duration = GetSlotCooldownDurationSeconds(Role, SlotIndex);
		const float Remaining = End - Now;
		if (Remaining > 0.0f && Duration > 0.0f)
		{
			Progress = FMath::Clamp(Remaining / Duration, 0.0f, 1.0f);
			SecondsLeft = FMath::CeilToInt(Remaining);
		}
	}

	if (CooldownMID)
	{
		CooldownMID->SetScalarParameterValue(TEXT("Progress"), Progress);
	}
	if (CooldownSecondsText)
	{
		if (SecondsLeft > 0)
		{
			CooldownSecondsText->SetText(FText::AsNumber(SecondsLeft));
			CooldownSecondsText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			CooldownSecondsText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
