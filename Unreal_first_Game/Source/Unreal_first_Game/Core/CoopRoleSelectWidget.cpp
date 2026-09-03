#include "Core/CoopRoleSelectWidget.h"
#include "Core/CoopGameState.h"
#include "Core/CoopMatchPhase.h"
#include "Core/CoopPlayerState.h"
#include "Core/CoopPlayerController.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"

namespace
{
	// Button background tints. Deliberately mid-tone (not near-white) so the always-white caption
	// text stays readable on every state -- "ugly is correct", no per-state text colour juggling.
	const FLinearColor RoleSlotColor_Available(0.20f, 0.35f, 0.60f); // blue  -- free to claim
	const FLinearColor RoleSlotColor_Mine(0.15f, 0.55f, 0.20f);      // green -- this player's pick
	const FLinearColor RoleSlotColor_Taken(0.50f, 0.15f, 0.15f);     // red   -- another player holds it
}

bool UCoopRoleSelectWidget::IsRoleTaken(EPlayerRole Role) const
{
	if (Role == EPlayerRole::Unassigned)
	{
		return false;
	}

	const ACoopGameState* GameState = Cast<ACoopGameState>(UGameplayStatics::GetGameState(this));
	if (!GameState)
	{
		return false;
	}

	for (const TObjectPtr<APlayerState>& PS : GameState->PlayerArray)
	{
		const ACoopPlayerState* CoopPS = Cast<ACoopPlayerState>(PS);
		if (CoopPS && CoopPS->GetRole() == Role)
		{
			return true;
		}
	}
	return false;
}

FText UCoopRoleSelectWidget::GetRoleSelectRemainingSecondsText() const
{
	const ACoopGameState* GameState = Cast<ACoopGameState>(UGameplayStatics::GetGameState(this));
	if (!GameState || GameState->GetRoleSelectEndServerTime() < 0.0f)
	{
		return FText::AsNumber(0);
	}

	const float Remaining = FMath::Max(0.0f, GameState->GetRoleSelectEndServerTime() - GameState->GetServerWorldTimeSeconds());
	return FText::AsNumber(FMath::RoundToInt(Remaining));
}

ESlateVisibility UCoopRoleSelectWidget::GetRoleSelectVisibility() const
{
	const ACoopGameState* GameState = Cast<ACoopGameState>(UGameplayStatics::GetGameState(this));
	const bool bActive = GameState && GameState->GetCurrentPhase() == EMatchPhase::RoleSelect;
	return bActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

EPlayerRole UCoopRoleSelectWidget::GetMyRole() const
{
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (const ACoopPlayerState* PS = PC->GetPlayerState<ACoopPlayerState>())
		{
			return PS->GetRole();
		}
	}
	return EPlayerRole::Unassigned;
}

void UCoopRoleSelectWidget::ClaimRole(EPlayerRole DesiredRole)
{
	if (ACoopPlayerController* CoopPC = Cast<ACoopPlayerController>(GetOwningPlayer()))
	{
		CoopPC->Server_ClaimRole(DesiredRole);
	}
}

void UCoopRoleSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ACoopGameState* GameState = Cast<ACoopGameState>(UGameplayStatics::GetGameState(this));
	const bool bRoleSelectActive = GameState && GameState->GetCurrentPhase() == EMatchPhase::RoleSelect;

	// The rest of the match runs in Game-Only input with a hidden cursor -- the mouse drives the
	// orbit camera (CLAUDE.md §5). RoleSelect is the one screen a player has to *click* something,
	// so while it's active the local player needs a visible cursor + UI input; then it must switch
	// back so gameplay isn't left with a stuck cursor. Purely local input handling -- no gameplay
	// state, no replication (CLAUDE.md §4.2). bShowMouseCursor doubles as the "already switched"
	// flag so this only runs on the two phase edges, not every frame -- nothing else in the project
	// touches bShowMouseCursor.
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (bRoleSelectActive && !PC->bShowMouseCursor)
		{
			PC->SetShowMouseCursor(true);
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);
		}
		else if (!bRoleSelectActive && PC->bShowMouseCursor)
		{
			PC->SetShowMouseCursor(false);
			PC->SetInputMode(FInputModeGameOnly());
		}
	}

	// Everything below is the per-row button feedback; the panel is Collapsed outside RoleSelect
	// (GetRoleSelectVisibility) but NativeTick still fires, so bail once the cursor is handled.
	if (!bRoleSelectActive)
	{
		return;
	}

	UpdateRoleSlot(EPlayerRole::Tank, TankButton, TankButtonLabel);
	UpdateRoleSlot(EPlayerRole::Support, SupportButton, SupportButtonLabel);
	UpdateRoleSlot(EPlayerRole::Runner, RunnerButton, RunnerButtonLabel);
	UpdateRoleSlot(EPlayerRole::Control, ControlButton, ControlButtonLabel);
	UpdateRoleSlot(EPlayerRole::Damage, DamageButton, DamageButtonLabel);

	if (RoleSelectHeaderText)
	{
		RoleSelectHeaderText->SetText(FText::FromString(FString::Printf(
			TEXT("PICK YOUR ROLE  -  %s SEC LEFT"), *GetRoleSelectRemainingSecondsText().ToString())));
	}
}

void UCoopRoleSelectWidget::UpdateRoleSlot(EPlayerRole Role, UButton* Button, UTextBlock* Label)
{
	const bool bTaken = IsRoleTaken(Role);
	const bool bMine = (GetMyRole() == Role);

	if (Button)
	{
		// A role another player holds can't be claimed. Your own claimed role is shown (green) but
		// disabled too -- there is deliberately no click-to-release; switch by clicking a different
		// still-available role, which frees your previous pick automatically (server-side).
		Button->SetIsEnabled(!bTaken);
		Button->SetBackgroundColor(bMine ? RoleSlotColor_Mine : (bTaken ? RoleSlotColor_Taken : RoleSlotColor_Available));
	}

	if (Label)
	{
		Label->SetText(FText::FromString(bMine ? TEXT("YOURS") : (bTaken ? TEXT("TAKEN") : TEXT("CLAIM"))));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
}
