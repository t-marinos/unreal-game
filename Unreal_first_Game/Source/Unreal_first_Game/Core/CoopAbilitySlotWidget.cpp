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

namespace
{
	struct FCoopAbilitySlotInfo
	{
		FText Name;
		FLinearColor TileColor;
		bool bImplemented = false;
	};

	// Per-role action-bar kit. Names duplicated from CoopAbilityCardWidget's table (which keeps the
	// long descriptions -- the bar only needs name + tile colour + castable-flag). A deliberate
	// ~11-string duplication, not a refactor of a verified widget (CLAUDE.md §4.8, §1 "hardcoded is
	// correct"). Only slot 0 of each role is bImplemented -- the one Q maps to.
	const TArray<FCoopAbilitySlotInfo>& GetKitForRole(EPlayerRole Role)
	{
		static const TArray<FCoopAbilitySlotInfo> Tank = {
			{ NSLOCTEXT("CoopAbilitySlot", "Shield", "Shield"),               FLinearColor(0.20f, 0.40f, 0.65f), true  },
			{ NSLOCTEXT("CoopAbilitySlot", "ArmorBreak", "Armor Break"),      FLinearColor(0.55f, 0.30f, 0.12f), false },
		};
		static const TArray<FCoopAbilitySlotInfo> Support = {
			{ NSLOCTEXT("CoopAbilitySlot", "Speed", "Speed"),                 FLinearColor(0.15f, 0.52f, 0.42f), true  },
			{ NSLOCTEXT("CoopAbilitySlot", "Link", "Link"),                   FLinearColor(0.40f, 0.25f, 0.55f), false },
		};
		static const TArray<FCoopAbilitySlotInfo> Runner = {
			{ NSLOCTEXT("CoopAbilitySlot", "Dash", "Dash"),                   FLinearColor(0.42f, 0.52f, 0.15f), true  },
			{ NSLOCTEXT("CoopAbilitySlot", "Carry", "Carry"),                 FLinearColor(0.45f, 0.35f, 0.22f), false },
			{ NSLOCTEXT("CoopAbilitySlot", "Chain", "Chain"),                 FLinearColor(0.35f, 0.40f, 0.48f), false },
		};
		static const TArray<FCoopAbilitySlotInfo> Control = {
			{ NSLOCTEXT("CoopAbilitySlot", "Stabilize", "Stabilize"),         FLinearColor(0.25f, 0.30f, 0.60f), true  },
			{ NSLOCTEXT("CoopAbilitySlot", "MindFracture", "Mind Fracture"),  FLinearColor(0.55f, 0.20f, 0.45f), false },
			{ NSLOCTEXT("CoopAbilitySlot", "Channel", "Channel"),             FLinearColor(0.15f, 0.45f, 0.55f), false },
		};
		static const TArray<FCoopAbilitySlotInfo> Damage = {
			{ NSLOCTEXT("CoopAbilitySlot", "Execution", "Execution"),         FLinearColor(0.62f, 0.16f, 0.20f), true  },
			{ NSLOCTEXT("CoopAbilitySlot", "Overload", "Overload"),           FLinearColor(0.45f, 0.20f, 0.55f), false },
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

float UCoopAbilitySlotWidget::GetSlotZeroCooldownEndServerTime(EPlayerRole Role) const
{
	const APlayerController* PC = GetOwningPlayer();
	const ACoopCharacter* Char = PC ? Cast<ACoopCharacter>(PC->GetPawn()) : nullptr;
	if (!Char)
	{
		return -1.0f;
	}
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

float UCoopAbilitySlotWidget::GetSlotZeroCooldownDurationSeconds(EPlayerRole Role) const
{
	if (!GameConstants)
	{
		return 0.0f;
	}
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

void UCoopAbilitySlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

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
		// Every ability is bound to Q by design (DECISIONS.md); only the implemented slot-0 one
		// actually does anything, so only it gets a badge.
		const bool bShowKey = (SlotIndex == 0 && Info.bImplemented);
		KeybindText->SetText(FText::FromString(TEXT("Q")));
		KeybindText->SetVisibility(bShowKey ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// --- Cooldown sweep: implemented slot-0 only ---
	float Progress = 0.0f;
	int32 SecondsLeft = 0;
	if (SlotIndex == 0 && Info.bImplemented)
	{
		const AGameStateBase* GS = UGameplayStatics::GetGameState(this);
		const float Now = GS ? GS->GetServerWorldTimeSeconds() : 0.0f;
		const float End = GetSlotZeroCooldownEndServerTime(Role);
		const float Duration = GetSlotZeroCooldownDurationSeconds(Role);
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
