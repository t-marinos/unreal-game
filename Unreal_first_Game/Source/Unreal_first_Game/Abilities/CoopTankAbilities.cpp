#include "Abilities/CoopTankAbilities.h"
#include "Core/CoopCharacter.h"
#include "Core/GameConstants.h"
#include "Tags/CoopGameplayTags.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"

namespace CoopTankAbilities
{
	void ApplyShield(ACoopCharacter* Tank, const UGameConstants* GameConstants)
	{
		if (!Tank || !Tank->HasAuthority() || !GameConstants || !Tank->GetWorld())
		{
			return;
		}

		const AGameStateBase* GameState = Tank->GetWorld()->GetGameState();
		const float Now = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
		if (Now < Tank->GetShieldCooldownEndServerTime())
		{
			return;
		}

		Tank->SetShieldCooldownEndServerTime(Now + GameConstants->ShieldCooldownSeconds);
		Tank->ApplyStatusTag(CoopGameplayTags::Status_Shielded, GameConstants->ShieldDurationSeconds);

		// Coverage is a snapshot at cast time, not a per-hit direction check: whoever is standing in
		// Tank's forward cone right now gets Status.Shielded too, for the same duration as Tank's own.
		// docs/abilities.md leaves the exact coverage shape as a Hold the Gate implementation detail.
		const FVector TankLocation = Tank->GetActorLocation();
		const FVector TankForward = Tank->GetActorForwardVector();
		const float CoverageAngleCos = FMath::Cos(FMath::DegreesToRadians(GameConstants->ShieldCoverageAngleDegrees * 0.5f));

		for (TActorIterator<ACoopCharacter> It(Tank->GetWorld()); It; ++It)
		{
			ACoopCharacter* Other = *It;
			if (!Other || Other == Tank)
			{
				continue;
			}

			const FVector ToOther = Other->GetActorLocation() - TankLocation;
			const float Distance = ToOther.Size();
			if (Distance > GameConstants->ShieldCoverageRadiusUnits)
			{
				continue;
			}

			// A teammate standing (almost) exactly on top of Tank has an undefined normalized
			// direction -- skip the angle check rather than reject them for a degenerate case.
			if (Distance > KINDA_SMALL_NUMBER)
			{
				const float DotToOther = FVector::DotProduct(TankForward, ToOther.GetSafeNormal());
				if (DotToOther < CoverageAngleCos)
				{
					continue;
				}
			}

			Other->ApplyStatusTag(CoopGameplayTags::Status_Shielded, GameConstants->ShieldDurationSeconds);
		}
	}
}
