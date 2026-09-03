#pragma once

#include "CoreMinimal.h"

class ACoopCharacter;
class UGameConstants;

// Build 1. Same shape as CoopTankAbilities/CoopControlAbilities -- see that file's header comment
// for the "no generic ability system" reasoning (CLAUDE.md §4.6).
namespace CoopRunnerAbilities
{
	// Resolves Runner's Dash cast: this IS the Thousand Dashes synergy's literal worked example
	// from CLAUDE.md §4.6 ("if (Ability == EAbilityId::Dash && Actor->HasMatchingGameplayTag(...))").
	// A short forward LaunchCharacter impulse normally; if the Runner currently holds
	// Status.SpeedBuff (checked, not consumed, per docs/abilities.md), resolves as the boosted
	// Thousand Dashes impulse instead. Movement itself rides on the standard
	// CharacterMovementComponent prediction already sanctioned by CLAUDE.md §4.2 -- only the
	// cooldown gate and impulse-strength decision are server-only.
	void ResolveDash(ACoopCharacter* Runner, const UGameConstants* GameConstants);
}
