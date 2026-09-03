#include "Abilities/CoopDamageAbilities.h"
#include "Core/CoopCharacter.h"
#include "Core/CoopMonsterCharacter.h"
#include "Core/CoopHealthComponent.h"
#include "Core/GameConstants.h"
#include "Tags/CoopGameplayTags.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"

namespace CoopDamageAbilities
{
	void ResolveExecution(ACoopCharacter* DamageDealer, const UGameConstants* GameConstants)
	{
		if (!DamageDealer || !DamageDealer->HasAuthority() || !GameConstants || !DamageDealer->GetWorld())
		{
			return;
		}

		const AGameStateBase* GameState = DamageDealer->GetWorld()->GetGameState();
		const float Now = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
		if (Now < DamageDealer->GetExecutionCooldownEndServerTime())
		{
			return;
		}

		// Cooldown and cast animation both fire on cast attempt, same "opens a window, doesn't
		// guarantee a hit" philosophy as every other implicit-target ability here.
		DamageDealer->SetExecutionCooldownEndServerTime(Now + GameConstants->ExecutionCooldownSeconds);
		DamageDealer->PlayCastMontage(DamageDealer->GetExecutionCastMontage());

		ACoopMonsterCharacter* NearestVulnerableTarget = nullptr;
		float NearestDistSq = FMath::Square(GameConstants->ExecutionCastRangeUnits);
		const FVector CasterLocation = DamageDealer->GetActorLocation();

		for (TActorIterator<ACoopMonsterCharacter> It(DamageDealer->GetWorld()); It; ++It)
		{
			ACoopMonsterCharacter* Monster = *It;
			if (!Monster || !Monster->HasStatusTag(CoopGameplayTags::Status_Vulnerable_Physical))
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(CasterLocation, Monster->GetActorLocation());
			if (DistSq <= NearestDistSq)
			{
				NearestDistSq = DistSq;
				NearestVulnerableTarget = Monster;
			}
		}

		if (!NearestVulnerableTarget)
		{
			return;
		}

		// Consumes the tag on use, per docs/abilities.md.
		NearestVulnerableTarget->RemoveStatusTag(CoopGameplayTags::Status_Vulnerable_Physical);
		if (UCoopHealthComponent* MonsterHealth = NearestVulnerableTarget->GetHealthComponent())
		{
			MonsterHealth->ApplyDamage(GameConstants->ExecutionDamageAmount);
		}
	}
}
