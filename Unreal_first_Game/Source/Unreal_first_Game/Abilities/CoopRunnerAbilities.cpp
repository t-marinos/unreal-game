#include "Abilities/CoopRunnerAbilities.h"
#include "Core/CoopCharacter.h"
#include "Core/GameConstants.h"
#include "Tags/CoopGameplayTags.h"
#include "GameFramework/GameStateBase.h"

namespace CoopRunnerAbilities
{
	void ResolveDash(ACoopCharacter* Runner, const UGameConstants* GameConstants)
	{
		if (!Runner || !Runner->HasAuthority() || !GameConstants || !Runner->GetWorld())
		{
			return;
		}

		const AGameStateBase* GameState = Runner->GetWorld()->GetGameState();
		const float Now = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
		if (Now < Runner->GetDashCooldownEndServerTime())
		{
			return;
		}

		Runner->SetDashCooldownEndServerTime(Now + GameConstants->DashCooldownSeconds);

		// Thousand Dashes: docs/abilities.md -- "if the actor holds Status.SpeedBuff at cast time,
		// resolves instead as Thousand Dashes... checks, doesn't consume" -- SpeedBuff is left to
		// its own natural expiry, not cleared here.
		const bool bBuffed = Runner->HasStatusTag(CoopGameplayTags::Status_SpeedBuff);
		Runner->PlayCastMontage(Runner->GetDashCastMontage());

		const float ImpulseStrength = bBuffed ? GameConstants->ThousandDashesImpulseStrength : GameConstants->DashImpulseStrength;
		Runner->LaunchCharacter(Runner->GetActorForwardVector() * ImpulseStrength, true, true);
	}
}
