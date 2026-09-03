#pragma once

#include "CoreMinimal.h"

class ACoopCharacter;
class UGameConstants;

// Build 1. Same shape as CoopTankAbilities/CoopControlAbilities -- CLAUDE.md §4.6: no generic
// ability/synergy system, one hardcoded function per ability. Plain namespace, not a UCLASS --
// server-only resolution logic; callers (CoopPlayerController's Server_* RPCs) are responsible
// for the HasAuthority() gate, matching every other RPC handler in this project.
namespace CoopSupportAbilities
{
	// Resolves Support's Speed cast: this IS the Thousand Dashes synergy input (CLAUDE.md §4.6's
	// own worked example). No explicit targeting UI exists yet (same "no crosshair yet" reasoning
	// as Stabilize's nearest-Tank search) -- finds the nearest other ACoopCharacter within
	// SpeedCastRangeUnits and applies Status.SpeedBuff (server-timestamped expiry) to them.
	// No-ops silently if Speed is still on cooldown, or if no ally is in range (cooldown still
	// consumes on a whiff, matching Stabilize's "opens a window, doesn't guarantee a hit"
	// precedent).
	void ApplySpeed(ACoopCharacter* Support, const UGameConstants* GameConstants);
}
