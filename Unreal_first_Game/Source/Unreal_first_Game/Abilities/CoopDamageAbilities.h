#pragma once

#include "CoreMinimal.h"

class ACoopCharacter;
class UGameConstants;

// Build 1. Same shape as CoopTankAbilities/CoopControlAbilities -- see that file's header comment
// for the "no generic ability system" reasoning (CLAUDE.md §4.6).
namespace CoopDamageAbilities
{
	// Resolves the Damage role's Execution cast (docs/abilities.md's physical-branch finisher of
	// Execution/Overload). No explicit targeting UI exists yet -- finds the nearest
	// ACoopMonsterCharacter within ExecutionCastRangeUnits that currently holds
	// Status.Vulnerable.Physical, consumes that tag, and deals ExecutionDamageAmount. Whiffs
	// silently (cooldown still consumes) if no such target is in range -- as of Build 1, the only
	// writer of Status.Vulnerable.Physical is the dev/test-only
	// ACoopPlayerController::ApplyTestVulnerable, since Scene 5 ("The Heart") doesn't exist yet.
	void ResolveExecution(ACoopCharacter* DamageDealer, const UGameConstants* GameConstants);
}
