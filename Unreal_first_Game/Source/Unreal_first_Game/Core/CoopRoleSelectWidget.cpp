#include "Core/CoopRoleSelectWidget.h"
#include "Core/CoopGameState.h"
#include "Core/CoopPlayerState.h"
#include "Core/CoopPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"

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

void UCoopRoleSelectWidget::ClaimRole(EPlayerRole DesiredRole)
{
	if (ACoopPlayerController* CoopPC = Cast<ACoopPlayerController>(GetOwningPlayer()))
	{
		CoopPC->Server_ClaimRole(DesiredRole);
	}
}
