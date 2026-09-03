#pragma once

#include "CoreMinimal.h"

class ACoopCharacter;
class ACoopMonsterCharacter;
class UGameConstants;

// Build 1, M7. First file in Abilities/ -- CLAUDE.md §4.6: no generic ability/synergy system,
// one hardcoded function per ability, five explicit conditionals for the synergies once they
// exist. Plain namespace, not a UCLASS -- this is server-only resolution logic with no reason to
// be reflected or placed in a level; callers (CoopPlayerController's Server_* RPCs) are
// responsible for the HasAuthority() gate, matching every other RPC handler in the project.
namespace CoopTankAbilities
{
	// Raises Tank's Shield: applies Status.Shielded (server-timestamped expiry, via
	// ACoopCharacter::ApplyStatusTag) to Tank and to every other ACoopCharacter currently standing
	// in Tank's forward coverage cone (docs/abilities.md: "the actor(s) currently standing behind
	// it"). No-ops silently if Shield is still on cooldown -- same "friends, not adversarial input"
	// reasoning as every other reject-with-no-effect check in this project (CLAUDE.md §8).
	void ApplyShield(ACoopCharacter* Tank, const UGameConstants* GameConstants);

	// Ability kit expansion. Tank's second ability (docs/abilities.md: "Armor Break -- Mind Fracture
	// input"). Target is the local player's click-selected actor, forwarded from
	// ACoopPlayerController::Server_ActivateArmorBreak and RE-VALIDATED here (CLAUDE.md §4.1 -- the
	// server never trusts the client's type): it must be an ACoopMonsterCharacter within
	// ArmorBreakCastRangeUnits. On a valid hit, applies Status.Broken for BrokenDurationSeconds --
	// real or fake target, Armor Break reveals nothing itself (docs/abilities.md). NOTHING reads
	// Status.Broken yet (Control's Mind Fracture + the False King clones are Build 2); for now the
	// tag only shows on the target frame's status line and expires. Cooldown consumes on any cast
	// that clears the gate, same "opens a window, doesn't guarantee a hit" philosophy as every
	// ability here. Takes AActor* (not ACoopMonsterCharacter*) so the RPC layer and this function
	// share one signature and the cast/validation lives in exactly one place.
	void ResolveArmorBreak(ACoopCharacter* Tank, AActor* Target, const UGameConstants* GameConstants);
}
