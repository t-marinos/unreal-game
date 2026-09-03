#include "Abilities/CoopDamageAbilities.h"
#include "Core/CoopCharacter.h"
#include "Core/CoopMonsterCharacter.h"
#include "Core/CoopHealthComponent.h"
#include "Core/GameConstants.h"
#include "Tags/CoopGameplayTags.h"
#include "GameFramework/GameStateBase.h"

namespace CoopDamageAbilities
{
	void ResolveExecution(ACoopCharacter* DamageDealer, AActor* Target, const UGameConstants* GameConstants)
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

		// Cooldown and cast animation both fire on any cast that clears the gate, whether or not the
		// deeper target checks below pass -- same "opens a window, doesn't guarantee a hit"
		// philosophy as every other ability here.
		DamageDealer->SetExecutionCooldownEndServerTime(Now + GameConstants->ExecutionCooldownSeconds);
		DamageDealer->PlayCastMontage(DamageDealer->GetExecutionCastMontage());

		// Re-validate the client's intent (CLAUDE.md §4.1). Execution only hits an
		// ACoopMonsterCharacter that is in range and currently Vulnerable (Physical branch).
		ACoopMonsterCharacter* MonsterTarget = Cast<ACoopMonsterCharacter>(Target);
		if (!MonsterTarget)
		{
			return;
		}

		const float DistSq = FVector::DistSquared(DamageDealer->GetActorLocation(), MonsterTarget->GetActorLocation());
		if (DistSq > FMath::Square(GameConstants->ExecutionCastRangeUnits))
		{
			return;
		}

		if (!MonsterTarget->HasStatusTag(CoopGameplayTags::Status_Vulnerable_Physical))
		{
			return;
		}

		// Consumes the tag on use, per docs/abilities.md.
		MonsterTarget->RemoveStatusTag(CoopGameplayTags::Status_Vulnerable_Physical);
		if (UCoopHealthComponent* MonsterHealth = MonsterTarget->GetHealthComponent())
		{
			MonsterHealth->ApplyDamage(GameConstants->ExecutionDamageAmount);
		}
	}

	void ResolveOverload(ACoopCharacter* DamageDealer, AActor* Target, const UGameConstants* GameConstants)
	{
		if (!DamageDealer || !DamageDealer->HasAuthority() || !GameConstants || !DamageDealer->GetWorld())
		{
			return;
		}

		const AGameStateBase* GameState = DamageDealer->GetWorld()->GetGameState();
		const float Now = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
		if (Now < DamageDealer->GetOverloadCooldownEndServerTime())
		{
			return;
		}

		DamageDealer->SetOverloadCooldownEndServerTime(Now + GameConstants->OverloadCooldownSeconds);
		DamageDealer->PlayCastMontage(DamageDealer->GetOverloadCastMontage());

		// Identical to ResolveExecution above but keyed to the Magic branch -- an explicit copy, not
		// a shared helper (CLAUDE.md §4.6).
		ACoopMonsterCharacter* MonsterTarget = Cast<ACoopMonsterCharacter>(Target);
		if (!MonsterTarget)
		{
			return;
		}

		const float DistSq = FVector::DistSquared(DamageDealer->GetActorLocation(), MonsterTarget->GetActorLocation());
		if (DistSq > FMath::Square(GameConstants->OverloadCastRangeUnits))
		{
			return;
		}

		if (!MonsterTarget->HasStatusTag(CoopGameplayTags::Status_Vulnerable_Magic))
		{
			return;
		}

		MonsterTarget->RemoveStatusTag(CoopGameplayTags::Status_Vulnerable_Magic);
		if (UCoopHealthComponent* MonsterHealth = MonsterTarget->GetHealthComponent())
		{
			MonsterHealth->ApplyDamage(GameConstants->OverloadDamageAmount);
		}
	}
}
