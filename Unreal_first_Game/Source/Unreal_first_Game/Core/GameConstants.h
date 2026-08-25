#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameConstants.generated.h"

// Every tunable number for Build 0 lives here (CLAUDE.md §10) -- editable in the Unreal Editor
// or via unreal-mcp's data asset tools, without touching code. The actual instance is
// Content/Data/DA_GameConstants, referenced by BP_GameMode. Grows as later milestones add more
// tunables (movement speeds, cooldowns, arena dimensions, ...); nothing here is load-bearing
// until something references an instance of it.
UCLASS(BlueprintType)
class UNREAL_FIRST_GAME_API UGameConstants : public UDataAsset
{
	GENERATED_BODY()

public:
	// M2/M3: session player cap, checked in ACoopGameMode::PreLogin (CLAUDE.md §4.7).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 5;

	// M6: replication tick rate for CoopGameState, set explicitly per CLAUDE.md §4.4 (20-30Hz
	// range) rather than left at the engine's per-class default, so it's a known, debuggable
	// number instead of an accident.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Networking")
	float GameStateNetUpdateFrequency = 30.0f;

	// M6: how often the shared elapsed-time UMG widget refreshes its displayed text. This is a
	// local display throttle, not a replication rate -- MatchStartServerTime itself replicates
	// once (set a single time in BeginPlay) and every client derives elapsed time locally from it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	float MatchTimerDisplayUpdateIntervalSeconds = 0.1f;

	// M5: local-only orbit camera (CLAUDE.md §5). World-space point every player's camera orbits
	// around by default -- Build 0 has no real arena yet, so this defaults to the origin; each
	// future scene can tune it once an actual arena exists.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	FVector ArenaCenterLocation = FVector::ZeroVector;

	// M5: distance from the pivot to the camera.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float CameraArmLength = 900.0f;

	// M5: starting pitch (degrees) -- negative tips the camera down for the "high 3/4 angle" look
	// per §5. Also the value orbiting can never exceed on either side, see Min/MaxPitch below.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float CameraDefaultPitch = -50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float CameraMinPitch = -80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float CameraMaxPitch = -20.0f;

	// M5: degrees of orbit per unit of raw mouse delta while right-click-dragging.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float CameraOrbitYawSpeed = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float CameraOrbitPitchSpeed = 0.5f;

	// Build 1, M3: grace period for the RoleSelect phase (CLAUDE.md §6.1/§6.3, DECISIONS.md's "Role
	// assignment is player-chosen, not random"). Once every real player has claimed a role, or this
	// timer expires, ACoopGameMode::ResolveRoleSelection() auto-assigns any still-Unassigned
	// PlayerState (a real AFK player, or a dev-mode dummy, which never claims) a random remaining
	// role -- this prevents one player stalling the whole lobby indefinitely.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roles")
	float RoleSelectDurationSeconds = 30.0f;

	// Build 1, M4: the 60-second preparation arena (CLAUDE.md §6.3). Starts once RoleSelect
	// resolves, not when the roster completes -- players need to see their ability cards/synergy
	// hints for the full 60s, not have it eaten by however long role selection took.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match")
	float PrepArenaDurationSeconds = 60.0f;

	// Build 1, M6: every ACoopCharacter's starting/max HP (UCoopHealthComponent). Shield's damage
	// negation (M7) and Downed's 0-HP trigger (M9) both build on this.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float DefaultMaxHealth = 100.0f;

	// Build 1, M7: Tank Shield (CoopTankAbilities::ApplyShield). How long Status.Shielded persists
	// once applied, and how long before Shield can be cast again.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float ShieldDurationSeconds = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float ShieldCooldownSeconds = 8.0f;

	// Forward-facing arc (full angle, degrees, centered on Tank's forward vector) and radius (cm)
	// used to find which other actors count as "currently standing behind it" at the moment Shield
	// is cast -- docs/abilities.md's Tank Shield entry.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float ShieldCoverageAngleDegrees = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float ShieldCoverageRadiusUnits = 300.0f;
};
