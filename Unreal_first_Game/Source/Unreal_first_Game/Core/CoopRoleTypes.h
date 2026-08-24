#pragma once

#include "CoreMinimal.h"
#include "CoopRoleTypes.generated.h"

// Build 1, M2: the 5 fixed roles from CLAUDE.md §6.1, plus Unassigned as the default before a
// player picks one. Unassigned doubles as "this role is still available to claim" during the
// RoleSelect phase (M3/M4) -- a role is free iff no PlayerState in GameState->PlayerArray already
// holds it. A shared header (not nested in CoopPlayerState.h) avoids circular includes between
// PlayerState, GameMode, and the later role-select/ability-card widgets, all of which need this
// enum without needing each other.
UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
	Unassigned,
	Tank,
	Support,
	Runner,
	Control,
	Damage
};
