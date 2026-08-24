#pragma once

#include "CoreMinimal.h"
#include "CoopMatchPhase.generated.h"

// Build 1, M4: the match's top-level phase state machine. A shared header, same reasoning as
// CoopRoleTypes.h -- GameState, GameMode, and later UI widgets all need this enum without needing
// each other. Every transition is server-timestamped (CLAUDE.md §4.5) and written only by
// ACoopGameMode via ACoopGameState's server-only setters -- never inferred or predicted
// client-side.
UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
	// Before ACoopGameMode::OnRosterComplete fires -- waiting for the 5th player (or dev-mode
	// dummy fill).
	WaitingForRoster,
	// Each player claims a role (M3's Server_ClaimRole); resolves on early-completion or
	// RoleSelectDurationSeconds timeout.
	RoleSelect,
	// The 60-second preparation arena (CLAUDE.md §6.3).
	Prep,
	// Scene 2 -- Hold the Gate. Lands in M10-M12; only the phase value itself exists this early.
	HoldTheGate,
	Complete
};
