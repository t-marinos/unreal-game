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

	// Ability kit expansion: how long UCoopToastWidget shows a centre-screen message
	// ("Please choose a target") before it has fully faded back out. A local cosmetic display
	// duration, not a gameplay timer -- but it is a tunable "duration", so it lives here per
	// CLAUDE.md §10 rather than being hardcoded in the widget.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	float ToastDurationSeconds = 2.0f;

	// WoW-style movement (DECISIONS.md's "Camera follows the player" entry): multiplies the
	// backward-movement input axis in BP_PlayerCharacter's Move function, so walking backward is
	// slower than forward/strafing -- see ACoopCharacter::GetBackpedalSpeedMultiplier.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float BackpedalSpeedMultiplier = 0.5f;

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

	// MONSTER_ENEMIES_PROGRESS.md Phase B: Shield-shove. Every ACoopMonsterCharacter caught in
	// Shield's forward cone (the same angle/radius test as the teammate-coverage loop) is
	// LaunchCharacter'd straight back from Tank by this impulse -- docs/scenes/HOLD_THE_GATE.md's
	// "knock enemies away", making Shield a repositioning tool, not just a damage filter. Fortress
	// deliberately adds no shove of its own.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float ShieldShoveImpulse = 900.0f;

	// Build 1, M8: Control Stabilize (CoopControlAbilities::ResolveStabilize). Cooldown, and how
	// far away a Tank can be and still be found by the implicit nearest-Tank targeting search.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float StabilizeCooldownSeconds = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float StabilizeCastRangeUnits = 800.0f;

	// How long Status.Fortress persists once Stabilize upgrades a Shielded Tank, the radius around
	// that Tank within which teammates also receive Fortress (docs/abilities.md: "multi-teammate
	// coverage, not just Tank's own facing"), and the fraction of knockback Fortress resists.
	// FortressKnockbackResistPercent's first consumer is ACoopMonsterCharacter::PerformStrike
	// (MONSTER_ENEMIES_PROGRESS.md Phase B) -- a Fortress'd target is launched only
	// (1 - FortressKnockbackResistPercent) as far by a monster strike.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float FortressDurationSeconds = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float FortressCoverageRadiusUnits = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float FortressKnockbackResistPercent = 0.75f;

	// Build 1: Support Speed (CoopSupportAbilities::ApplySpeed). Cooldown, how far away an ally
	// can be and still be found by the implicit nearest-ally targeting search (same "no crosshair
	// yet" reasoning as StabilizeCastRangeUnits), and how long the resulting Status.SpeedBuff lasts.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float SpeedCooldownSeconds = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float SpeedCastRangeUnits = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float SpeedBuffDurationSeconds = 6.0f;

	// Build 1: Runner Dash (CoopRunnerAbilities::ResolveDash). Cooldown, and the LaunchCharacter
	// impulse strength for a normal Dash vs. the boosted Thousand Dashes resolution when the
	// caster holds Status.SpeedBuff at cast time (docs/abilities.md: "checks, doesn't consume").
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float DashCooldownSeconds = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float DashImpulseStrength = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float ThousandDashesImpulseStrength = 2400.0f;

	// Build 1: Damage Execution (CoopDamageAbilities::ResolveExecution). Cooldown, cast range
	// against a Status.Vulnerable.Physical-holding ACoopMonsterCharacter, and the flat damage
	// dealt on a successful hit (docs/abilities.md gives no hard number beyond a "~5% of boss HP"
	// example that assumes a boss HP pool that doesn't exist yet -- a flat amount is the simpler
	// stand-in per CLAUDE.md §1).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float ExecutionCooldownSeconds = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float ExecutionCastRangeUnits = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float ExecutionDamageAmount = 100.0f;

	// Ability kit expansion: Tank Armor Break (CoopTankAbilities::ResolveArmorBreak). Cooldown, cast
	// range against the click-selected ACoopMonsterCharacter, and how long Status.Broken persists
	// once applied. docs/abilities.md calls Broken a "short window" -- deliberately shorter than
	// Fortress's 8s. Nothing reads Status.Broken until Control's Mind Fracture (Build 2) exists.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float ArmorBreakCooldownSeconds = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float ArmorBreakCastRangeUnits = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float BrokenDurationSeconds = 6.0f;

	// Ability kit expansion: Damage Overload (CoopDamageAbilities::ResolveOverload) -- the magic
	// branch of Execution/Overload. Mirrors Execution's numbers by design: same cooldown, cast range
	// against a Status.Vulnerable.Magic-holding ACoopMonsterCharacter, and flat damage on a hit.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float OverloadCooldownSeconds = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float OverloadCastRangeUnits = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float OverloadDamageAmount = 100.0f;

	// Dev/test only (ACoopPlayerController::ApplyTestVulnerable / ApplyTestVulnerableMagic) -- grants
	// the nearest ACoopMonsterCharacter Status.Vulnerable.Physical / .Magic, since nothing else
	// writes those tags until Scene 5 ("The Heart", Build 2) exists. Lets Execution / Overload
	// actually be tested now. Same "ApplyTestDamage" precedent, not a real gameplay ability.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float TestVulnerableDurationSeconds = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	float TestVulnerableRangeUnits = 1000.0f;

	// Build 1: overhead status badge (ACoopCharacter::StatusBarWidgetComponent /
	// UCoopStatusBarWidget). Z offset, in world units relative to the character's root, at which the
	// badge sits -- the stock Mannequin's default capsule half-height is commonly ~88 units in the
	// UE ThirdPerson template, so 180 clears the head with margin. A tunable default to eyeball in
	// PIE, not a precise measurement of this project's specific mesh.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status")
	float StatusBarHeightOffsetUnits = 180.0f;

	// Build 1, M9: Downed/revive (UCoopDownedComponent). How long a teammate must channel a revive
	// (re-validated at start via the RPC's own range search and again at completion, not
	// continuously -- see CoopDownedComponent.h's documented simplification), how close they need to
	// be, and what fraction of max health the revived player comes back with. No bleed-out timer --
	// Downed persists until revived or the party wipes (CLAUDE.md §6.6).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Downed")
	float ReviveDurationSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Downed")
	float ReviveRadiusUnits = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Downed")
	float ReviveHealthRestorePercent = 0.5f;

	// Build 1, M10: Hold the Gate (ACoopHoldTheGateScene/ACoopPressurePlate). Number of pressure
	// plates that must be simultaneously occupied for the gate to open -- docs/scenes/HOLD_THE_GATE.md
	// fixes this design at 4, but no field existed yet to hold it until this milestone.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoldTheGate")
	int32 PlateCount = 4;

	// How long the team has to restore full plate occupancy after it breaks before the gate closes
	// and the scene resets (CLAUDE.md §6.6's scene-specific wipe condition) -- the gate itself stays
	// open (cosmetically) through this grace window, per docs/scenes/HOLD_THE_GATE.md's "server-checked
	// continuously" framing paired with "can't be restored in time."
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoldTheGate")
	float PlateRestoreWindowSeconds = 5.0f;

	// How tall a pressure plate's occupancy trigger is, in units, measured upward from the
	// plate's own origin (ACoopPressurePlate::ApplyTriggerVolumeSize). The trigger used to be a
	// symmetric 200-unit-tall column that let a character register as "occupying" a plate while
	// still well above it or merely nearby, rather than physically standing on it -- this keeps
	// the trigger a thin band hugging the plate's actual surface instead.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoldTheGate")
	float PlateTriggerCatchHeightUnits = 30.0f;

	// Build 1, M12: total time the party has to hold the gate before ACoopHoldTheGateScene declares
	// the scripted threat sequence complete (docs/scenes/HOLD_THE_GATE.md's "Success: gate stays open
	// long enough / the scripted threat sequence completes"). Also what ACoopMonsterSpawner ramps its
	// spawn-interval escalation against, replacing M11's hardcoded 60s placeholder window.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoldTheGate")
	float HoldTheGateSceneDurationSeconds = 90.0f;

	// Build 1, M11: monster spawner/AI (DECISIONS.md's "Monster combat inside Hold the Gate").
	// Every trash monster's max HP (UCoopHealthComponent::SetMaxHealth, overriding the
	// player-oriented DefaultMaxHealth above).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster")
	float MonsterHealth = 50.0f;

	// ACoopMonsterSpawner's repeating spawn timer interval at scene start, and the interval it
	// ramps toward as the scene progresses (the real escalation curve against scene duration is
	// M12's job -- M11 only ramps linearly over a short, hardcoded local window as a placeholder).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster")
	float MonsterSpawnIntervalEarlySeconds = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster")
	float MonsterSpawnIntervalLateSeconds = 2.5f;

	// Brief pause (ACoopMonsterCharacter) between a monster's current target going Downed and it
	// actually locking onto a new one -- a readable "it hesitates, then re-fixates" beat rather than
	// an instant retarget.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster")
	float MonsterFixateSwitchDelaySeconds = 0.5f;

	// Not itemized in the Build 1 plan's own M11 constant list, but required to give
	// ACoopMonsterCharacter's "simple attack" (per the plan's own build-item wording) any actual
	// numbers -- same "found it was needed, added it" precedent as most prior milestones. Monster
	// attacks are abstracted as a periodic direct ApplyDamage tick against the current target (no
	// physical projectile actor, no range check) -- a documented simplification matching Shield's
	// own "negates all incoming damage, not just directional" precedent (CoopHealthComponent.cpp).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster")
	float MonsterAttackDamage = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster")
	float MonsterAttackIntervalSeconds = 2.0f;

	// MONSTER_ENEMIES_PROGRESS.md Phase A: monsters now walk toward their fixate target
	// (ACoopMonsterAIController straight-line steering) instead of harassing from where they
	// spawned. Walk speed set on ACharacter::GetCharacterMovement()->MaxWalkSpeed in
	// InitializeMonster -- kept a bit below player run speed (~500) so Tank/Runner can outpace them.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster")
	float MonsterMoveSpeed = 350.0f;

	// How close a monster must be to its target before PerformAttackTick will land a hit -- and how
	// far out ACoopMonsterAIController stops feeding movement input. Roughly capsule-to-capsule plus
	// a little reach. Read via ACoopMonsterCharacter::GetMeleeRangeUnits().
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster")
	float MonsterMeleeRangeUnits = 160.0f;

	// MONSTER_ENEMIES_PROGRESS.md Phase B: the telegraphed lead time before a monster's melee strike
	// lands. A flat red ground ring (ACoopMonsterStrikeTelegraph) shows at the target's feet for
	// this long, then ACoopMonsterCharacter::PerformStrike resolves -- re-checking range, so moving
	// the monster out of melee during the window (body-block, Shield-shove, or just walking away)
	// makes the strike whiff.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster")
	float MonsterAttackWindupSeconds = 1.0f;

	// LaunchCharacter impulse a landed monster strike applies to the player target, directly away
	// from the monster in the XY plane. Tuned (B9) strong enough to shove a plate-holder off their
	// plate -- the push moves them out of ACoopPressurePlate's thin overlap band, so the plate's own
	// OnOccupancyChanged fires and the gate closes, with no plate code changes. This makes the
	// monster threat attack the objective, not just HP. Status.Fortress resists it by
	// FortressKnockbackResistPercent (first consumer of that constant); Status.Shielded negates the
	// damage but deliberately does NOT resist knockback.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster")
	float MonsterKnockbackImpulse = 650.0f;

	// Cursor-targeting feature (cursor_progress.md): the flat ground ring under the click-selected
	// target (ACoopTargetRing / M_TargetRing). Ring radius in world units (the plane mesh is scaled
	// to match), and how far below the target actor's origin to drop the ring so it sits on the
	// floor -- ~88 is the stock Mannequin capsule half-height, same basis as
	// StatusBarHeightOffsetUnits above. Eyeball both in PIE.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	float TargetRingRadiusUnits = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	float TargetRingGroundOffsetUnits = 88.0f;
};
