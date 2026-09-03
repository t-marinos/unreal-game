#pragma once

#include "CoreMinimal.h"

class ACoopCharacter;
class UGameConstants;

// Build 1, M8. Second file in Abilities/ -- CLAUDE.md §4.6: one hardcoded function per ability,
// the synergy conditional lives inline as a plain if/else, not a generic resolver. Plain
// namespace, same shape as CoopTankAbilities -- server-only resolution logic, no reason to be
// reflected; callers (CoopPlayerController's Server_* RPCs) are responsible for HasAuthority().
namespace CoopControlAbilities
{
	// Resolves Control's Stabilize cast: finds the nearest Tank-role ACoopCharacter within
	// StabilizeCastRangeUnits. This IS the Fortress synergy conditional (CLAUDE.md §4.6's literal
	// worked example): if that Tank currently holds Status.Shielded, upgrades it (and every
	// teammate within FortressCoverageRadiusUnits of the Tank) to Status.Fortress. If no Tank is
	// in range, or the nearest one isn't currently Shielded, the cast whiffs -- cooldown still
	// consumes (docs/abilities.md: Armor Break-style "opens a window, doesn't guarantee a hit"
	// philosophy applies here too), but no tag is written.
	void ResolveStabilize(ACoopCharacter* Control, const UGameConstants* GameConstants);
}
