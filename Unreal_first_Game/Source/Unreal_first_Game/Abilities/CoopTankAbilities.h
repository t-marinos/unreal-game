#pragma once

#include "CoreMinimal.h"

class ACoopCharacter;
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
}
