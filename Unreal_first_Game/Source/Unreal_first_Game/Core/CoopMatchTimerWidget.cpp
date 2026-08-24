#include "Core/CoopMatchTimerWidget.h"
#include "Core/CoopGameState.h"
#include "Kismet/GameplayStatics.h"

FText UCoopMatchTimerWidget::GetElapsedMatchTimeText() const
{
	if (const ACoopGameState* GameState = Cast<ACoopGameState>(UGameplayStatics::GetGameState(this)))
	{
		return FText::AsNumber(FMath::RoundToInt(GameState->GetElapsedMatchTime()));
	}
	return FText::GetEmpty();
}
