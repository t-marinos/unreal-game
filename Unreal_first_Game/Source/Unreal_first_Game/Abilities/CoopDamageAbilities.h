#pragma once

#include "CoreMinimal.h"

class ACoopCharacter;
class UGameConstants;

// Build 1. Same shape as CoopTankAbilities/CoopControlAbilities -- see that file's header comment
// for the "no generic ability system" reasoning (CLAUDE.md §4.6). Execution and Overload are
// deliberately two fully spelled-out functions, not one parameterised helper: they differ only by a
// tag + three constants, but §4.6 wants the explicit copy over the clever abstraction, and it keeps
// each readable on its own.
namespace CoopDamageAbilities
{
	// Resolves the Damage role's Execution cast (docs/abilities.md's physical-branch finisher of
	// Execution/Overload). Target is the local player's click-selected actor, forwarded from
	// ACoopPlayerController::Server_ActivateExecution and RE-VALIDATED here (CLAUDE.md §4.1): it must
	// be an ACoopMonsterCharacter within ExecutionCastRangeUnits currently holding
	// Status.Vulnerable.Physical. On a valid hit, consumes that tag and deals ExecutionDamageAmount.
	// Whiffs silently (cooldown still consumes) otherwise. As of now the only writer of
	// Status.Vulnerable.Physical is the dev-only ACoopPlayerController::ApplyTestVulnerable -- Scene
	// 5 ("The Heart") doesn't exist yet.
	//
	// Retrofitted 2026-09-03 from an implicit "nearest vulnerable monster in range" search to an
	// explicit target (DECISIONS.md "Target-required abilities need a click-selected target").
	// Speed / Stabilize were deliberately NOT retrofitted -- they keep their auto-search.
	void ResolveExecution(ACoopCharacter* DamageDealer, AActor* Target, const UGameConstants* GameConstants);

	// Ability kit expansion. Damage's second ability -- the magic branch, identical in structure to
	// ResolveExecution but keyed to Status.Vulnerable.Magic and the Overload* constants. Reading
	// which branch (Physical vs Magic) is open and picking the right key under time pressure is The
	// Heart's actual test (docs/abilities.md) -- not exercised until that scene exists, but the
	// ability is fully functional now against a dev-granted tag
	// (ACoopPlayerController::ApplyTestVulnerableMagic).
	void ResolveOverload(ACoopCharacter* DamageDealer, AActor* Target, const UGameConstants* GameConstants);
}
