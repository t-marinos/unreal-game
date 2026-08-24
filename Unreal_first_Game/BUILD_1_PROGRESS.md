# Build 1 Progress Tracker

Working checklist for implementing Build 1 ("Hold the Gate" prototype), per the approved plan.
Full context, rationale, and verification detail for each milestone lives in the plan file at
`C:\Users\t-mar\.claude\plans\what-comes-next-for-breezy-eclipse.md` — this file is just the
resumable checklist: what's done, what's next.

**If a session gets interrupted (e.g. runs out of credits), a new session should:**
1. Read this file to see the last checked/unchecked item.
2. Read the plan file above for the full detail on the next unchecked milestone.
3. Read `DECISIONS.md` for anything settled along the way.
4. Continue from the first unchecked box.

**Scope note:** execution stops at M13. M14 (the live 5-human exit-criteria playtest) is
deliberately parked — the user expects further changes/decisions once M13 lands before M14 should
be scheduled. Don't start M14 without it being explicitly picked back up in a future session.

**Role assignment is player-chosen, not random** — a deliberate, logged override of CLAUDE.md
§6.1/§6.3's original "assigned randomly" wording. See M0 and `DECISIONS.md`.

---

## M0 — Decision Log & Progress Tracker
- [x] `DECISIONS.md`: add entry "Monster combat inside Hold the Gate" (what/why/scope, per CLAUDE.md
      §6.5/§7's citation of this entry)
- [x] `DECISIONS.md`: add entry "Role assignment is player-chosen, not random" (override + resolution
      mechanic)
- [x] Update CLAUDE.md §6.1/§6.3 wording to match player-chosen role selection
- [x] Create this file (`BUILD_1_PROGRESS.md`) — done by definition once this line is checked

## M1 — Gameplay Tags Foundation
- [x] `Source/Unreal_first_Game/Tags/CoopGameplayTags.h/.cpp` — `Status.Shielded`,
      `Status.Fortress`, `Status.Downed`
- [x] `Unreal_first_Game.Build.cs`: add `"GameplayTags"` dependency
- [x] Confirm/add the three tag spellings in `docs/abilities.md`
- [x] Verify: project compiles, all three tags register

## M2 — Role Enum & PlayerState Role Field
- [x] `Source/Unreal_first_Game/Core/CoopRoleTypes.h` — `EPlayerRole` (Unassigned/TANK/SUPPORT/
      RUNNER/CONTROL/DAMAGE)
- [x] `CoopPlayerState.h/.cpp`: `PlayerRole` field (default Unassigned), `GetRole()`, server-only
      `SetRole()`
- [x] Verify: 5-client PIE, confirmed defaults; full mutate+replicate check deferred to M3 (see log)

## M3 — Roster-Complete Trigger & Player Role Selection
- [x] `CoopGameMode.h/.cpp`: roster tracking in `PostLogin`, `bRosterComplete` guard,
      `OnRosterComplete()`, role-select timer, `ResolveRoleSelection()`
- [x] `CoopPlayerController.h/.cpp`: `Server_ClaimRole(EPlayerRole DesiredRole)`
- [x] New DA_GameConstants: `RoleSelectDurationSeconds = 30.0`
- [x] Verify: 5-client PIE — `OnRosterComplete`/timeout/auto-assign paths confirmed (see log); the
      claim RPC itself and duplicate-claim rejection are untestable until M5 gives it a caller (no
      function-call tool in unreal-mcp) — deferred, not skipped, see log

## M4 — Match Phase State Machine, Role-Select Phase & Prep Countdown
- [x] `CoopGameState.h/.cpp`: `EMatchPhase CurrentPhase` (WaitingForRoster/RoleSelect/Prep/
      HoldTheGate/Complete), `RoleSelectEndServerTime`, `PrepPhaseEndServerTime`
- [x] `CoopGameMode.h/.cpp`: phase transitions on `OnRosterComplete`/`ResolveRoleSelection`/prep
      timer expiry
- [x] New DA_GameConstants: `PrepArenaDurationSeconds = 60.0`
- [x] Verify: 5-client PIE, full RoleSelect→Prep→HoldTheGate progression confirmed with consistent
      server timestamps

## M5 — Role Select Screen, Ability Cards & Team Synergies Panel (UMG)
- [ ] `CoopRoleSelectWidget.h/.cpp` (C++ base)
- [ ] `CoopPrepCountdownWidget.h/.cpp` (C++ base)
- [ ] `CoopSynergyHintWidget.h/.cpp` (C++ base)
- [ ] `WBP_RoleSelect`, `WBP_PrepArenaHUD`, `WBP_AbilityCard`, `WBP_TeamSynergiesPanel`
- [ ] Verify: reflection confirms role-taken/countdown/synergy values and contested-claim resolution;
      human playtest for layout/readability

## M6 — Health & Damage Foundation
- [ ] `CoopHealthComponent.h/.cpp` — replicated Current/MaxHealth, server-only `ApplyDamage()`,
      `GetHealthPercent()`, 0-HP delegate
- [ ] `ACoopCharacter`: add the component
- [ ] New DA_GameConstants: `DefaultMaxHealth = 100.0`
- [ ] Verify: 5-client PIE, damage replication/clamping, 0-HP delegate fires once

## M7 — Tank Shield Ability
- [ ] `Source/Unreal_first_Game/Abilities/CoopTankAbilities.h/.cpp` — `ApplyShield()`, coverage
      query, damage negation
- [ ] `ACoopCharacter`: replicated `ActiveStatusTags` + server-tracked expiry map
- [ ] `CoopPlayerController.h/.cpp`: `Server_ActivateShield()`
- [ ] New Input Action `IA_Shield` wired into `IMC_Default`/`BP_PlayerController`
- [ ] New DA_GameConstants: `ShieldDurationSeconds`, `ShieldCooldownSeconds`,
      `ShieldCoverageAngleDegrees`, `ShieldCoverageRadiusUnits`
- [ ] Verify: reflection for tag/expiry/negation; human keypress + playtest for feel

## M8 — Control Stabilize + Fortress Synergy Conditional
- [ ] `Source/Unreal_first_Game/Abilities/CoopControlAbilities.h/.cpp` — `ResolveStabilize()` with
      the hardcoded Fortress conditional (CLAUDE.md §4.6 shape)
- [ ] `CoopPlayerController.h/.cpp`: `Server_ActivateStabilize()`
- [ ] New Input Action `IA_Stabilize`
- [ ] New DA_GameConstants: `StabilizeCooldownSeconds`, `StabilizeCastRangeUnits`,
      `FortressDurationSeconds`, `FortressCoverageRadiusUnits`, `FortressKnockbackResistPercent`
- [ ] Verify: Tank+Control combo produces `Status.Fortress`; whiff on unshielded Tank does nothing;
      human playtest for knockback resist

## M9 — Downed / Revive / Wipe / Instant Retry
- [ ] `CoopDownedComponent.h/.cpp` — 0-HP → Downed, revive channel, movement/ability lockout
- [ ] `CoopGameState.h/.cpp`: replicated Downed count, `IsPartyWiped()`, `RequestSceneReset()` hook
- [ ] `CoopPlayerController.h/.cpp`: `Server_AttemptRevive()`
- [ ] New DA_GameConstants: `ReviveDurationSeconds`, `ReviveRadiusUnits`,
      `ReviveHealthRestorePercent`
- [ ] Verify: 5-client PIE, single downed+revived correctly; simultaneous all-5-down flips
      `IsPartyWiped()` on every client

## M10 — Hold the Gate: Plates & Gate Logic
- [ ] `Source/Unreal_first_Game/Scenes/CoopPressurePlate.h/.cpp`
- [ ] `CoopGateActor.h/.cpp`
- [ ] `CoopHoldTheGateScene.h/.cpp`
- [ ] `BP_PressurePlate`, `BP_Gate` wrappers + level layout
- [ ] New DA_GameConstants: `PlateRestoreWindowSeconds` (confirm no `PlateCount` duplication)
- [ ] Verify: gate opens only with all 4 plates held simultaneously; restore-window/close behavior;
      server-authoritative

## M11 — Hold the Gate: Monster Spawner & Fixate/Retarget AI
*(Depends on M0 being committed first.)*
- [ ] `CoopFixateRetargetComponent.h/.cpp` — reusable targeting component
- [ ] `CoopMonsterCharacter.h/.cpp`
- [ ] `CoopMonsterSpawner.h/.cpp` — Hold-the-Gate-specific timing/escalation
- [ ] `BP_MonsterCharacter`, `BP_MonsterSpawner` + chamber visuals/placement
- [ ] New DA_GameConstants: `MonsterHealth`, `MonsterSpawnIntervalEarlySeconds`,
      `MonsterSpawnIntervalLateSeconds`, `MonsterFixateSwitchDelaySeconds`
- [ ] Verify: monsters target plate-holders, retarget correctly, server-driven spawn timing

## M12 — Hold the Gate: Escalation Tuning, Win/Lose Integration, Full Playtest
- [ ] `CoopHoldTheGateScene.h/.cpp` extended: win/lose state machine, `ResetScene()`,
      `CompleteScene()`
- [ ] New DA_GameConstants: `HoldTheGateSceneDurationSeconds`
- [ ] Verify: human playtest for escalation pacing (Shield-alone-sufficient → insufficient curve)

## M13 — Mini-Boss: Repeat → Combine → Rotate Fortress
- [ ] `CoopMiniBossCharacter.h/.cpp` — extends `CoopMonsterCharacter`, local phase state machine
      (repeat/combine/rotate — no overlap)
- [ ] `BP_MiniBoss` wrapper
- [ ] New DA_GameConstants: `MiniBossHealth`, `MiniBossPhase2HealthPercent`,
      `MiniBossPhase3HealthPercent`
- [ ] Verify: 5-client PIE + human playtest — each phase distinguishable, combine forces a real
      Tank/Control decision, rotate genuinely changes the mechanic's shape

---

## Parked (not started — do not begin without explicit go-ahead in a future session)
- **M14 — Exit Criteria Playtest**: five friends play the full loop and talk continuously. Held back
  because further changes/decisions are expected after M13 lands. See the plan file's "Stop point
  after M13" note.

## Log
(Newest entries at the bottom. One line per completed step: what was done, anything notable.)

- **File created.** `BUILD_1_PROGRESS.md` written mirroring `BUILD_0_PROGRESS.md`'s structure, with
  milestones M0–M13 from the approved plan as checkboxes and M14 explicitly parked. Next: M0
  (Decision Log & Progress Tracker) — starting with the two `DECISIONS.md` entries and the
  CLAUDE.md §6.1/§6.3 wording update.

- **M0 done.** Added two `DECISIONS.md` entries: "Monster combat inside Hold the Gate" (what's being
  built ahead of Build 2's general system, why — Hold the Gate needs a real threat for
  Shield/Fortress to defend against or the synergy can't be exercised at all — and the scope
  boundary: reusable fixate/retarget targeting component, but spawn choreography stays local to this
  scene) and "Role assignment is player-chosen, not random" (records the explicit user override of
  CLAUDE.md §6.1/§6.3's original "assigned randomly" wording, the resolution mechanic — first-claim-
  wins via `Server_ClaimRole`, with a `RoleSelectDurationSeconds` timeout auto-assigning any
  leftovers — and that the prep arena's countdown doesn't start until selection resolves). Updated
  CLAUDE.md §6.1 and §6.3 wording to match player-chosen selection, each pointing back at the new
  DECISIONS.md entry rather than re-deriving the reasoning inline. No code yet — M0 was
  documentation-only by design. Next: M1 (Gameplay Tags Foundation).

- **M1 done.** Added `Source/Unreal_first_Game/Tags/CoopGameplayTags.h/.cpp` — native
  `FGameplayTag`s via `UE_DECLARE_GAMEPLAY_TAG_EXTERN`/`UE_DEFINE_GAMEPLAY_TAG` in a
  `CoopGameplayTags` namespace: `Status_Shielded` ("Status.Shielded"), `Status_Fortress`
  ("Status.Fortress"), `Status_Downed` ("Status.Downed") — all three spellings already existed in
  `docs/abilities.md`'s tag glossary from an earlier planning pass, so no doc edit was needed.
  Added "GameplayTags" to `Unreal_first_Game.Build.cs`'s `PublicDependencyModuleNames`. User
  triggered Ctrl+Alt+F11; log confirmed `LogLiveCoding: Display: Live coding succeeded` (1 package
  changed, 10 classes unchanged, 1 enum unchanged, 12 functions remapped), no errors.
  **Real tooling gap found and logged, not worked around:** `unreal-mcp` has no console-command
  execution tool and no GameplayTags-specific toolset, so there is no way to run something like
  `GameplayTags.PrintReport` or otherwise enumerate the live `FGameplayTagsManager`'s registered
  tags through current tooling — checked `EditorAppToolset` and `LogsToolset` directly to confirm
  this isn't just a missed tool name. Verification for this milestone is therefore: clean compile
  plus no GameplayTags-related errors/warnings in the log around the reload (checked via
  `GetLogEntries`/grep, none found) — native tags register via static initialization at module
  load, so a clean module load is meaningful evidence, but it's not the same as a direct registry
  read. Worth revisiting if a future `unreal-mcp` update adds console-command dispatch.
  Next: M2 (Role Enum & PlayerState Role Field).

- **M2 done — with a real UHT bug caught and fixed, and a genuine reflection-tooling limit found.**
  Added `Source/Unreal_first_Game/Core/CoopRoleTypes.h` (`UENUM(BlueprintType) EPlayerRole`:
  Unassigned/Tank/Support/Runner/Control/Damage, same style as `EDummyBehavior`). Added a role
  field to `ACoopPlayerState`: `GetRole()` (`BlueprintPure`), server-only `SetRole()`, replicated via
  `DOREPLIFETIME` — same shape as the existing `bInvulnerable` pattern.
  **First compile attempt failed on a genuine UHT rule, not a tooling quirk:** naming the field
  `Role` collides with `AActor::Role` (the base class's own replication `ENetRole`) — UHT rejects
  shadowing a base class member name. Renamed the field to `PlayerRole` (getter/setter names
  unchanged) and recompiled clean (`Live coding succeeded`, 1 package changed, 1 class changed, 1
  enum new, benign packaging warning about the new data type — expected).
  **Verify:** 5-client PIE, found all 5 real `ACoopPlayerState` instances via `find_actors`,
  confirmed every one defaults to `PlayerRole=Unassigned`. **Attempted the plan's literal
  "set a role via reflection, confirm it replicates" step and hit a real, consistent tooling limit:**
  `ObjectTools.set_properties` returned `false` on every attempt against a live spawned PIE actor
  instance — tried `PlayerRole=Tank` in three different string-format variations, then `bInvulnerable`
  (an existing, already-working replicated bool), then a totally unrelated always-writable engine
  property (`CustomTimeDilation`) as a control case — all four failed identically. Cross-checked
  against `BUILD_0_PROGRESS.md`: every prior `set_properties` call across all of Build 0 targeted a
  CDO (`Default__X`), never a live PIE instance — only `get_properties` was ever used on spawned
  actors. Conclusion: this tool can write CDOs but not live PIE actor instances; not something to
  route around with a different format. **This is arguably the architecturally correct outcome
  anyway** (CLAUDE.md §4.1: replicated gameplay state should only change through an authoritative
  code path, not a raw reflection poke) — full mutate-and-confirm-replication verification is
  deferred to M3, where `Server_ClaimRole` provides the real RPC path to set `PlayerRole`, which is
  a better test than a synthetic one would have been. `StopPIE`.
  Next: M3 (Roster-Complete Trigger & Player Role Selection).

- **M3 done.** Added `RoleSelectDurationSeconds` (30.0) to `GameConstants.h`. `ACoopGameMode`
  gained: `PostLogin` override (fires `OnRosterComplete()` once `GameState->PlayerArray.Num() >=
  MaxPlayers`), a matching check appended to the end of `FillEmptySlotsWithDummies()` (dev mode can
  reach a full roster in one BeginPlay-time burst with no `PostLogin` call for the dummy-filled
  slots, so both paths need their own trigger point converging on the same fire-once guard),
  `OnRosterComplete()` (starts a `RoleSelectDurationSeconds` timer via `GetWorldTimerManager()`),
  `TryClaimRole(ACoopPlayerState*, EPlayerRole)` (public — scans `GameState->PlayerArray` for
  anyone else already holding the requested role, rejects silently if taken, otherwise calls
  `SetRole()` and checks for early completion), `CheckAllRealPlayersClaimed()` (iterates
  `GetWorld()->GetPlayerControllerIterator()` — deliberately real `APlayerController`s only, so
  dev-mode dummies never block early resolution), and `ResolveRoleSelection()` (fires on timeout or
  early-completion; builds the 5-role pool minus whatever's already claimed, randomly assigns the
  remainder to every still-`Unassigned` PlayerState — AFK real players and every dummy). Added
  `Server_ClaimRole(EPlayerRole)` to `ACoopPlayerController` (same intent-only RPC shape as
  `Server_PressButton`), forwarding to `GameMode->TryClaimRole()`.
  User triggered Ctrl+Alt+F11; log confirmed `Live coding succeeded` (3 classes changed —
  GameMode/PlayerController/PlayerState — matches expectation), only the same benign
  data-type-change packaging warning as M1/M2.
  **Verify:** 5-client PIE. Confirmed via `GetLogEntries` that `OnRosterComplete` fired **exactly
  once** (not once per client) with the correct 30.0s duration read from `DA_GameConstants`.
  **Confirmed a second real tooling gap:** `ObjectTools` (the only object-manipulation toolset) has
  no function-call capability — only `get/set/reset_properties`, `list_properties`, `get_class`,
  `search_subclasses` — so there is no way to invoke `Server_ClaimRole` via reflection at all. The
  claim path and its duplicate-rejection behavior genuinely can't be exercised until M5 gives it a
  real caller (the role-select widget) or a dev Exec command wraps it — deliberately not adding a
  test-only Exec wrapper now, that would be scope creep beyond what M3 asked for.
  **What *was* verifiable without any RPC call:** the timeout fallback, since it fires automatically
  from a plain server timer with zero claims needed. Left the 5-client PIE session idle through the
  full 30s window (confirmed via a background wait + log grep) and got
  `ResolveRoleSelection: role selection resolved (5 player(s) auto-assigned)`. Read back all 5
  `PlayerState`s' `PlayerRole` individually: `Support`, `Damage`, `Control`, `Runner`, `Tank` — all
  5 roles present, each exactly once, zero left `Unassigned` — confirms the random-remaining-role
  fallback logic is correct. `StopPIE`.
  Next: M4 (Match Phase State Machine, Role-Select Phase & Prep Countdown).

- **M4 done.** New `Source/Unreal_first_Game/Core/CoopMatchPhase.h` — `EMatchPhase`
  (WaitingForRoster/RoleSelect/Prep/HoldTheGate/Complete), a shared header for the same
  circular-include reason as `CoopRoleTypes.h`. `ACoopGameState` gained `GetCurrentPhase()`,
  `GetRoleSelectEndServerTime()`/`GetPrepPhaseEndServerTime()` (both `-1` sentinel until started,
  same reasoning as `MatchStartServerTime`), and server-only `StartRoleSelectPhase()`/
  `StartPrepPhase()`/`StartHoldTheGatePhase()` setters, all three replicated via `DOREPLIFETIME`.
  `ACoopGameMode` now calls `StartRoleSelectPhase()` inside `OnRosterComplete()`,
  `StartPrepPhase()` (plus a new `PrepPhaseTimerHandle`) at the end of `ResolveRoleSelection()`, and
  added `OnPrepPhaseExpired()` (fires `StartHoldTheGatePhase()` on `PrepArenaDurationSeconds`
  expiry — actual Hold the Gate scene setup is still M10's job, this milestone only wires the phase
  value). Added `PrepArenaDurationSeconds = 60.0` to `GameConstants.h`.
  User triggered Ctrl+Alt+F11; log confirmed `Live coding succeeded` (3 classes changed, 1 enum
  new), same benign packaging warning as prior milestones.
  **Verify:** 5-client PIE, full end-to-end. Immediately after roster-complete: confirmed
  `CurrentPhase=RoleSelect`, `RoleSelectEndServerTime≈30.47`, `PrepPhaseEndServerTime=-1` via
  reflection. Rather than poll, watched the log for both transition lines over the full ~90s window
  (RoleSelect's 30s timeout + Prep's 60s duration): got
  `ResolveRoleSelection: Prep phase started (60.0s)` at the expected time, then
  `OnPrepPhaseExpired: Prep phase ended -- HoldTheGate phase started` ~60s later. Final reflection
  read: `CurrentPhase=HoldTheGate`, `RoleSelectEndServerTime=30.47`, `PrepPhaseEndServerTime=90.72`
  — internally consistent (90.72 ≈ 30.47 + 60.0, accounting for real scheduling variance). No manual
  claim was made in this run (same as M3's test), so this exercised the full auto-resolve path
  through all three phases in one continuous session. `StopPIE`.
  Next: M5 (Role Select Screen, Ability Cards & Team Synergies Panel).
