#include "Core/CoopPrepCountdownWidget.h"
#include "Core/CoopGameState.h"
#include "Kismet/GameplayStatics.h"

FText UCoopPrepCountdownWidget::GetPrepRemainingSecondsText() const
{
	const ACoopGameState* GameState = Cast<ACoopGameState>(UGameplayStatics::GetGameState(this));
	if (!GameState || GameState->GetPrepPhaseEndServerTime() < 0.0f)
	{
		return FText::AsNumber(0);
	}

	const float Remaining = FMath::Max(0.0f, GameState->GetPrepPhaseEndServerTime() - GameState->GetServerWorldTimeSeconds());
	return FText::AsNumber(FMath::RoundToInt(Remaining));
}
