#include "Abilities/CoopTankAbilities.h"
#include "Core/CoopCharacter.h"
#include "Core/CoopMonsterCharacter.h"
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

		// MONSTER_ENEMIES_PROGRESS.md Phase B -- Shield-shove. Every ACoopMonsterCharacter caught in
		// the same forward cone (docs/scenes/HOLD_THE_GATE.md's "knock enemies away") is launched
		// straight back from Tank. Server-only (this whole function asserts HasAuthority above); rides
		// the CLAUDE.md §4.2 movement-replication exception, same as Dash and the monster's own strike
		// knockback. Makes Shield a repositioning tool, not just a damage filter -- Fortress
		// deliberately adds no shove of its own, keeping this to one loop in one function.
		for (TActorIterator<ACoopMonsterCharacter> It(Tank->GetWorld()); It; ++It)
		{
			ACoopMonsterCharacter* Monster = *It;
			if (!Monster)
			{
				continue;
			}

			const FVector ToMonster = Monster->GetActorLocation() - TankLocation;
			const float Distance = ToMonster.Size();
			if (Distance > GameConstants->ShieldCoverageRadiusUnits)
			{
				continue;
			}

			// Same degenerate-direction guard as the teammate loop above.
			if (Distance > KINDA_SMALL_NUMBER)
			{
				const float DotToMonster = FVector::DotProduct(TankForward, ToMonster.GetSafeNormal());
				if (DotToMonster < CoverageAngleCos)
				{
					continue;
				}
			}

			FVector ShoveDir = ToMonster.GetSafeNormal2D();
			if (ShoveDir.IsNearlyZero())
			{
				ShoveDir = TankForward.GetSafeNormal2D();
			}
			Monster->LaunchCharacter(ShoveDir * GameConstants->ShieldShoveImpulse, true, false);
		}
	}

	void ResolveArmorBreak(ACoopCharacter* Tank, AActor* Target, const UGameConstants* GameConstants)
	{
		if (!Tank || !Tank->HasAuthority() || !GameConstants || !Tank->GetWorld())
		{
			return;
		}

		const AGameStateBase* GameState = Tank->GetWorld()->GetGameState();
		const float Now = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
		if (Now < Tank->GetArmorBreakCooldownEndServerTime())
		{
			return;
		}

		// Cooldown and cast animation both fire on any cast that clears the gate, whether or not the
		// deeper target checks below pass -- same "I pressed my button and my character did
		// something" feedback every ability in this project gives (DECISIONS.md "The Q ability").
		Tank->SetArmorBreakCooldownEndServerTime(Now + GameConstants->ArmorBreakCooldownSeconds);
		Tank->PlayCastMontage(Tank->GetArmorBreakCastMontage());

		// Re-validate the intent (CLAUDE.md §4.1): the client only ever sends something it clicked,
		// but the server owns the decision. Armor Break only affects an ACoopMonsterCharacter.
		ACoopMonsterCharacter* MonsterTarget = Cast<ACoopMonsterCharacter>(Target);
		if (!MonsterTarget)
		{
			return;
		}

		const float DistSq = FVector::DistSquared(Tank->GetActorLocation(), MonsterTarget->GetActorLocation());
		if (DistSq > FMath::Square(GameConstants->ArmorBreakCastRangeUnits))
		{
			return;
		}

		// docs/abilities.md: Armor Break applies Status.Broken to whatever it hits, real or fake --
		// it opens Mind Fracture's window, it does not reveal anything. Nothing reads Status.Broken
		// until Control's Mind Fracture + the False King clones (Build 2); until then the tag simply
		// shows on the target frame's status line (UCoopUnitFrameWidget) and expires.
		MonsterTarget->ApplyStatusTag(CoopGameplayTags::Status_Broken, GameConstants->BrokenDurationSeconds);
	}
}
