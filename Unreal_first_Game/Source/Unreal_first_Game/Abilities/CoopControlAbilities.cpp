#include "Abilities/CoopControlAbilities.h"
#include "Core/CoopCharacter.h"
#include "Core/CoopPlayerState.h"
#include "Core/GameConstants.h"
#include "Tags/CoopGameplayTags.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"

namespace CoopControlAbilities
{
	void ResolveStabilize(ACoopCharacter* Control, const UGameConstants* GameConstants)
	{
		if (!Control || !Control->HasAuthority() || !GameConstants || !Control->GetWorld())
		{
			return;
		}

		const AGameStateBase* GameState = Control->GetWorld()->GetGameState();
		const float Now = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
		if (Now < Control->GetStabilizeCooldownEndServerTime())
		{
			return;
		}

		// Cooldown consumes on cast, whether or not a valid Shielded Tank is found -- matches
		// Armor Break's "opens a window, doesn't guarantee a hit" philosophy (docs/abilities.md).
		Control->SetStabilizeCooldownEndServerTime(Now + GameConstants->StabilizeCooldownSeconds);

		// No explicit targeting UI exists yet (Build 1 has no crosshair/target-select input) --
		// the plan's chosen substitute is a server-side nearest-Tank-in-range search, same
		// "friends, not adversarial input" reasoning as every other implicit-target ability here.
		ACoopCharacter* NearestTank = nullptr;
		float NearestDistSq = FMath::Square(GameConstants->StabilizeCastRangeUnits);
		const FVector ControlLocation = Control->GetActorLocation();

		for (TActorIterator<ACoopCharacter> It(Control->GetWorld()); It; ++It)
		{
			ACoopCharacter* Other = *It;
			if (!Other || Other == Control)
			{
				continue;
			}

			const ACoopPlayerState* OtherPS = Other->GetPlayerState<ACoopPlayerState>();
			if (!OtherPS || OtherPS->GetRole() != EPlayerRole::Tank)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(ControlLocation, Other->GetActorLocation());
			if (DistSq <= NearestDistSq)
			{
				NearestDistSq = DistSq;
				NearestTank = Other;
			}
		}

		// This IS the Fortress synergy conditional (CLAUDE.md §4.6's worked example): Stabilize
		// only does something if the target Tank is currently holding Status.Shielded. No Tank in
		// range, or an unshielded Tank, means a silent whiff -- cooldown already consumed above.
		if (!NearestTank || !NearestTank->HasStatusTag(CoopGameplayTags::Status_Shielded))
		{
			return;
		}

		// Upgrade: Status.Shielded -> Status.Fortress on the Tank, then extend Fortress to every
		// other teammate within FortressCoverageRadiusUnits of the Tank (docs/abilities.md:
		// "multi-teammate coverage, not just Tank's own facing" -- a radius, not Shield's cone).
		// A character shouldn't hold both tags at once, so Shielded is cleared wherever Fortress
		// replaces it.
		NearestTank->RemoveStatusTag(CoopGameplayTags::Status_Shielded);
		NearestTank->ApplyStatusTag(CoopGameplayTags::Status_Fortress, GameConstants->FortressDurationSeconds);

		const FVector TankLocation = NearestTank->GetActorLocation();
		for (TActorIterator<ACoopCharacter> It(Control->GetWorld()); It; ++It)
		{
			ACoopCharacter* Other = *It;
			if (!Other || Other == NearestTank)
			{
				continue;
			}

			const float Distance = FVector::Dist(TankLocation, Other->GetActorLocation());
			if (Distance > GameConstants->FortressCoverageRadiusUnits)
			{
				continue;
			}

			if (Other->HasStatusTag(CoopGameplayTags::Status_Shielded))
			{
				Other->RemoveStatusTag(CoopGameplayTags::Status_Shielded);
			}
			Other->ApplyStatusTag(CoopGameplayTags::Status_Fortress, GameConstants->FortressDurationSeconds);
		}
	}
}
