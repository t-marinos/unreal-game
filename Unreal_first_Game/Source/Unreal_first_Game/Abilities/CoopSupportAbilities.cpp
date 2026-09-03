#include "Abilities/CoopSupportAbilities.h"
#include "Core/CoopCharacter.h"
#include "Core/GameConstants.h"
#include "Tags/CoopGameplayTags.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"

namespace CoopSupportAbilities
{
	void ApplySpeed(ACoopCharacter* Support, const UGameConstants* GameConstants)
	{
		if (!Support || !Support->HasAuthority() || !GameConstants || !Support->GetWorld())
		{
			return;
		}

		const AGameStateBase* GameState = Support->GetWorld()->GetGameState();
		const float Now = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
		if (Now < Support->GetSpeedCooldownEndServerTime())
		{
			return;
		}

		// Cooldown and cast animation both fire on cast attempt, whether or not a valid ally is
		// found -- same "opens a window, doesn't guarantee a hit" philosophy as Stabilize.
		Support->SetSpeedCooldownEndServerTime(Now + GameConstants->SpeedCooldownSeconds);
		Support->PlayCastMontage(Support->GetSpeedCastMontage());

		ACoopCharacter* NearestAlly = nullptr;
		float NearestDistSq = FMath::Square(GameConstants->SpeedCastRangeUnits);
		const FVector SupportLocation = Support->GetActorLocation();

		for (TActorIterator<ACoopCharacter> It(Support->GetWorld()); It; ++It)
		{
			ACoopCharacter* Other = *It;
			if (!Other || Other == Support)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(SupportLocation, Other->GetActorLocation());
			if (DistSq <= NearestDistSq)
			{
				NearestDistSq = DistSq;
				NearestAlly = Other;
			}
		}

		if (!NearestAlly)
		{
			return;
		}

		NearestAlly->ApplyStatusTag(CoopGameplayTags::Status_SpeedBuff, GameConstants->SpeedBuffDurationSeconds);
	}
}
