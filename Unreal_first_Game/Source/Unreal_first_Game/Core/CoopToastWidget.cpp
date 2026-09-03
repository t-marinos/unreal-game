#include "Core/CoopToastWidget.h"
#include "Core/CoopPlayerController.h"
#include "Core/GameConstants.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

void UCoopToastWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Stay HitTestInvisible forever -- never Collapsed/Hidden from our own tick (the P9 self-hide
	// freeze), and the cursor must fall straight through to the camera drag anyway (CLAUDE.md §5).
	SetVisibility(ESlateVisibility::HitTestInvisible);

	const ACoopPlayerController* PC = GetOwningPlayer<ACoopPlayerController>();
	if (!PC)
	{
		SetRenderOpacity(0.0f);
		return;
	}

	const FText Message = PC->GetPendingToastText();
	const float StartTime = PC->GetPendingToastStartTime();
	const float Duration = GameConstants ? GameConstants->ToastDurationSeconds : 2.0f;

	// Local cosmetic fade: elapsed = now - one stored stamp, NOT a DeltaTime accumulation
	// (CLAUDE.md §4.4) -- same shape as the action bar's cooldown sweep (End - Now). Game time
	// (not server time) is correct here: this is a purely local display, never gameplay timing.
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float Elapsed = Now - StartTime;

	if (Message.IsEmpty() || StartTime < 0.0f || Duration <= 0.0f || Elapsed >= Duration)
	{
		SetRenderOpacity(0.0f);
		return;
	}

	if (MessageText)
	{
		MessageText->SetText(Message);
	}

	// Full opacity for the first 60% of the window, then a linear fade to 0 over the last 40%.
	const float FadeStart = Duration * 0.6f;
	const float Opacity = (Elapsed <= FadeStart)
		? 1.0f
		: FMath::Clamp(1.0f - (Elapsed - FadeStart) / (Duration - FadeStart), 0.0f, 1.0f);
	SetRenderOpacity(Opacity);
}
