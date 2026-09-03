#include "Core/CoopUnitFrameWidget.h"
#include "Core/CoopPlayerController.h"
#include "Core/CoopCharacter.h"
#include "Core/CoopMonsterCharacter.h"
#include "Core/CoopHealthComponent.h"
#include "Core/CoopPlayerState.h"
#include "Core/CoopRoleTypes.h"
#include "Core/CoopGameState.h"
#include "Core/CoopMatchPhase.h"
#include "Tags/CoopGameplayTags.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"

namespace
{
	// Cosmetic constants, hardcoded here (not GameConstants) -- same precedent as
	// UCoopStatusBarWidget::GetStatusColor / ACoopCharacter::GetColorForPlayerId: colour is a
	// cosmetic constant, not a gameplay tunable (CLAUDE.md §10 is about gameplay numbers).
	const FLinearColor AllyHealthColor(0.15f, 0.70f, 0.20f);           // green
	const FLinearColor EnemyHealthColor(0.75f, 0.15f, 0.15f);          // red
	const FLinearColor MyRowBorderColor(0.90f, 0.80f, 0.20f, 0.85f);   // warm yellow -- "this is me"
	const FLinearColor NeutralBorderColor(0.0f, 0.0f, 0.0f, 0.55f);    // neutral dark

	FText RoleToText(EPlayerRole Role)
	{
		switch (Role)
		{
			case EPlayerRole::Tank:    return NSLOCTEXT("CoopUnitFrame", "Tank", "TANK");
			case EPlayerRole::Support: return NSLOCTEXT("CoopUnitFrame", "Support", "SUPPORT");
			case EPlayerRole::Runner:  return NSLOCTEXT("CoopUnitFrame", "Runner", "RUNNER");
			case EPlayerRole::Control: return NSLOCTEXT("CoopUnitFrame", "Control", "CONTROL");
			case EPlayerRole::Damage:  return NSLOCTEXT("CoopUnitFrame", "Damage", "DAMAGE");
			default:                   return NSLOCTEXT("CoopUnitFrame", "Unassigned", "...");
		}
	}
}

UCoopHealthComponent* UCoopUnitFrameWidget::HealthOf(const AActor* Actor)
{
	if (const ACoopCharacter* Char = Cast<ACoopCharacter>(Actor))
	{
		return Char->GetHealthComponent();
	}
	if (const ACoopMonsterCharacter* Monster = Cast<ACoopMonsterCharacter>(Actor))
	{
		return Monster->GetHealthComponent();
	}
	return nullptr;
}

bool UCoopUnitFrameWidget::ActorHasTag(const AActor* Actor, const FGameplayTag& Tag)
{
	if (const ACoopCharacter* Char = Cast<ACoopCharacter>(Actor))
	{
		return Char->HasStatusTag(Tag);
	}
	if (const ACoopMonsterCharacter* Monster = Cast<ACoopMonsterCharacter>(Actor))
	{
		return Monster->HasStatusTag(Tag);
	}
	return false;
}

AActor* UCoopUnitFrameWidget::ResolveSubjectActor() const
{
	if (Source == EUnitFrameSource::CurrentTarget)
	{
		const ACoopPlayerController* PC = GetOwningPlayer<ACoopPlayerController>();
		return PC ? PC->GetCurrentTargetActor() : nullptr;
	}

	// PartyMember: GameState->PlayerArray[PartyMemberIndex]'s pawn. PlayerArray order is roughly
	// join order and is stable for a 5-friend run where nobody leaves mid-scene -- acceptable for a
	// prototype, per CLAUDE.md §1.
	const AGameStateBase* GameState = UGameplayStatics::GetGameState(this);
	if (!GameState || !GameState->PlayerArray.IsValidIndex(PartyMemberIndex))
	{
		return nullptr;
	}
	const APlayerState* PS = GameState->PlayerArray[PartyMemberIndex];
	return PS ? PS->GetPawn() : nullptr;
}

bool UCoopUnitFrameWidget::IsLocalPlayerSubject(const AActor* SubjectActor) const
{
	const APawn* SubjectPawn = Cast<APawn>(SubjectActor);
	const APlayerController* PC = GetOwningPlayer();
	return SubjectPawn && PC && SubjectPawn->GetPlayerState() == PC->PlayerState;
}

void UCoopUnitFrameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ACoopGameState* GameState = Cast<ACoopGameState>(UGameplayStatics::GetGameState(this));
	const EMatchPhase Phase = GameState ? GameState->GetCurrentPhase() : EMatchPhase::WaitingForRoster;
	const bool bPhaseAllowsFrames = (Phase == EMatchPhase::Prep || Phase == EMatchPhase::HoldTheGate);

	AActor* Subject = bPhaseAllowsFrames ? ResolveSubjectActor() : nullptr;
	UCoopHealthComponent* Health = HealthOf(Subject);
	const bool bHasSubject = (Subject != nullptr && Health != nullptr);

	// A party row showing a live teammate is left-clickable to target them (NativeOnMouseButtonDown);
	// an empty party slot and the whole target frame stay HitTestInvisible so the cursor falls
	// straight through to the right-click-drag camera (CLAUDE.md §5). Visible and HitTestInvisible
	// are both in Slate's "visible" family, so toggling between them here never triggers the P9
	// self-hide freeze -- show/hide is still RenderOpacity, never a jump to Collapsed/Hidden.
	const bool bClickable = (Source == EUnitFrameSource::PartyMember && bHasSubject);
	SetVisibility(bClickable ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);

	if (!bHasSubject)
	{
		SetRenderOpacity(0.0f);
		return;
	}
	SetRenderOpacity(1.0f);

	const bool bIsAlly = (Cast<ACoopCharacter>(Subject) != nullptr);

	if (NameText)
	{
		FString Name = TEXT("Enemy");
		if (const ACoopCharacter* Char = Cast<ACoopCharacter>(Subject))
		{
			if (const APlayerState* PS = Char->GetPlayerState())
			{
				Name = PS->GetPlayerName();
			}
		}
		NameText->SetText(FText::FromString(Name));
	}

	if (TypeText)
	{
		FText Type = NSLOCTEXT("CoopUnitFrame", "Enemy", "ENEMY");
		if (const ACoopCharacter* Char = Cast<ACoopCharacter>(Subject))
		{
			const ACoopPlayerState* PS = Cast<ACoopPlayerState>(Char->GetPlayerState());
			Type = RoleToText(PS ? PS->GetRole() : EPlayerRole::Unassigned);
		}
		TypeText->SetText(Type);
	}

	if (HealthBar)
	{
		HealthBar->SetPercent(Health->GetHealthPercent());
		HealthBar->SetFillColorAndOpacity(bIsAlly ? AllyHealthColor : EnemyHealthColor);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"),
			Health->GetCurrentHealth(), Health->GetMaxHealth())));
	}

	// Status line: concatenate the short label of whichever known tags are active (same approach as
	// UCoopStatusBarWidget). Downed first -- it's the state that most changes what you'd do next.
	if (StatusText)
	{
		TArray<FString> Parts;
		if (ActorHasTag(Subject, CoopGameplayTags::Status_Downed))              { Parts.Add(TEXT("DOWNED")); }
		if (ActorHasTag(Subject, CoopGameplayTags::Status_Fortress))            { Parts.Add(TEXT("FORTRESS")); }
		if (ActorHasTag(Subject, CoopGameplayTags::Status_Shielded))            { Parts.Add(TEXT("SHIELDED")); }
		if (ActorHasTag(Subject, CoopGameplayTags::Status_SpeedBuff))           { Parts.Add(TEXT("SPEED")); }
		if (ActorHasTag(Subject, CoopGameplayTags::Status_Broken))             { Parts.Add(TEXT("BROKEN")); }
		if (ActorHasTag(Subject, CoopGameplayTags::Status_Vulnerable_Physical)) { Parts.Add(TEXT("VULNERABLE-P")); }
		if (ActorHasTag(Subject, CoopGameplayTags::Status_Vulnerable_Magic))   { Parts.Add(TEXT("VULNERABLE-M")); }

		StatusText->SetText(FText::FromString(FString::Join(Parts, TEXT("  "))));
		StatusText->SetVisibility(Parts.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// Party stack only: tint my own row so I can pick myself out. The target frame's RootBorder just
	// stays neutral -- there's only ever one and it's whatever I clicked.
	if (RootBorder)
	{
		const bool bTintAsMe = (Source == EUnitFrameSource::PartyMember && IsLocalPlayerSubject(Subject));
		RootBorder->SetBrushColor(bTintAsMe ? MyRowBorderColor : NeutralBorderColor);
	}
}

FReply UCoopUnitFrameWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Only a party row reacts, and only to the left button. Right-click (and anything else) falls
	// through Unhandled so ACoopOrbitCamera still gets its drag with the mouse over the party frame.
	if (Source == EUnitFrameSource::PartyMember
		&& InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (AActor* Subject = ResolveSubjectActor())
		{
			if (ACoopPlayerController* PC = GetOwningPlayer<ACoopPlayerController>())
			{
				PC->SetCurrentTarget(Subject);
				return FReply::Handled();
			}
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
