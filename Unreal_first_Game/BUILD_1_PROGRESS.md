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
- [x] `CoopRoleSelectWidget.h/.cpp` (C++ base)
- [x] `CoopPrepCountdownWidget.h/.cpp` (C++ base)
- [x] `CoopSynergyHintWidget.h/.cpp` (C++ base)
- [x] `CoopAbilityCardWidget.h/.cpp` (C++ base — one instance per ability-card slot; not itemized in
      the original plan's checklist line above but required to give `WBP_AbilityCard` a parent class)
- [x] `WBP_RoleSelect`, `WBP_PrepArenaHUD`, `WBP_AbilityCard`, `WBP_TeamSynergiesPanel` — user
      completed the manual Designer work (Text Blocks, Buttons, `Bind Function`/`OnClicked` wiring)
      from the prior log entry's numbered steps. Verified populated and wired, not just present —
      see log entry below for exact evidence.
- [x] Verify: reflection confirms the data path and `WBP_RoleSelect`'s button wiring are correct (see
      log) and the full RoleSelect→Prep→HoldTheGate progression still works with real widgets
      attached — done. The layout bug that previously kept this box unchecked (ability card text
      overlaps/narrow-wraps illegibly) is now fixed and definitively verified — see the
      "Parked item resolved" log entry below. **Still open, not blocking:** a human playtest for
      general layout/readability feel (same carve-out as every other milestone's ability-feel
      playtest) and actual-button-click testing (no longer literally impossible —
      `SlateInspectorToolset.Click` now exists — but not exercised this session since it wasn't
      what was asked; worth doing whenever role-select click behaviour needs verifying).

**Parked item resolved (see the dedicated log entry below):** the `WBP_AbilityCard` description
text-wrap width bug logged 2026-08-25 as "leave it to fix it later" was fixed and verified in a later
session once `unreal-mcp` gained `UMGToolSet` (widget-tree read/write) and `SlateInspectorToolset`
(live screenshot + accessibility-tree text extraction) — the exact tooling gap that made this
un-fixable-by-reflection before. Kept here as a record of what was parked and why; see the log entry
for the real root cause (it wasn't the guessed-at `Size Box` at all) and the fix actually applied.

## M6 — Health & Damage Foundation
- [x] `CoopHealthComponent.h/.cpp` — replicated Current/MaxHealth, server-only `ApplyDamage()`,
      `GetHealthPercent()`, 0-HP delegate
- [x] `ACoopCharacter`: add the component
- [x] New DA_GameConstants: `DefaultMaxHealth = 100.0`
- [x] Verify: 5-client PIE, damage replication/clamping confirmed live; 0-HP delegate fire-once
      confirmed by code review only (no bound listener until M9) — see log

## M7 — Tank Shield Ability
- [x] `Source/Unreal_first_Game/Abilities/CoopTankAbilities.h/.cpp` — `ApplyShield()`, coverage
      query, damage negation
- [x] `ACoopCharacter`: replicated `ActiveStatusTags` + server-tracked expiry map
- [x] `CoopPlayerController.h/.cpp`: `Server_ActivateShield()`
- [x] New Input Action `IA_Shield` wired into `IMC_Default`/`BP_PlayerController`
- [x] New DA_GameConstants: `ShieldDurationSeconds`, `ShieldCooldownSeconds`,
      `ShieldCoverageAngleDegrees`, `ShieldCoverageRadiusUnits`
- [x] Verify: reflection for tag/expiry/negation confirmed live (see log). Human playtest for feel
      still outstanding — not blocking, see log's final note.

## M8 — Control Stabilize + Fortress Synergy Conditional
- [x] `Source/Unreal_first_Game/Abilities/CoopControlAbilities.h/.cpp` — `ResolveStabilize()` with
      the hardcoded Fortress conditional (CLAUDE.md §4.6 shape)
- [x] `CoopPlayerController.h/.cpp`: `Server_ActivateStabilize()`
- [x] New Input Action `IA_Stabilize` — duplicated from `IA_Shield`, mapped to `Q` in `IMC_Default`,
      wired into `BP_PlayerCharacter`'s `EventGraph`
- [x] New DA_GameConstants: `StabilizeCooldownSeconds`, `StabilizeCastRangeUnits`,
      `FortressDurationSeconds`, `FortressCoverageRadiusUnits`, `FortressKnockbackResistPercent`
      (confirmed live on the CDO with correct default values after the rebuild)
- [x] Verify: Tank+Control combo produces `Status.Fortress` (replacing `Status.Shielded`, and
      extending to nearby teammates); whiff on an unshielded Tank does nothing. Knockback resist
      has no consumer yet (parked until M11) — human playtest for feel still open, not blocking.

## M9 — Downed / Revive / Wipe / Instant Retry
- [x] `CoopDownedComponent.h/.cpp` — 0-HP → Downed, revive channel, movement/ability lockout
- [x] `CoopGameState.h/.cpp`: replicated Downed count, `IsPartyWiped()`, `RequestSceneReset()` hook
- [x] `CoopPlayerController.h/.cpp`: `Server_AttemptRevive()`
- [x] New DA_GameConstants: `ReviveDurationSeconds`, `ReviveRadiusUnits`,
      `ReviveHealthRestorePercent` (0.5, per explicit user confirmation this session)
- [x] Verify: 5-client PIE, single downed+revived correctly; simultaneous all-5-down flips
      `IsPartyWiped()` on every client — done via the new `GameplayTestToolset` plugin (see
      `build_toolset.md`), entirely self-driven, no human keypress/console typing anywhere

## M10 — Hold the Gate: Plates & Gate Logic
- [x] `Source/Unreal_first_Game/Scenes/CoopPressurePlate.h/.cpp`
- [x] `CoopGateActor.h/.cpp`
- [x] `CoopHoldTheGateScene.h/.cpp`
- [x] `BP_PressurePlate`, `BP_Gate` wrappers + level layout (also `BP_HoldTheGateScene`, not
      itemized in the original plan line but required — `CoopHoldTheGateScene` holds its own
      `EditDefaultsOnly` `GameConstants` reference, so the established CDO-persistence rule applies)
- [x] New DA_GameConstants: `PlateRestoreWindowSeconds` (confirmed no `PlateCount` duplication —
      no such field existed yet, added both)
- [x] Verify: gate opens only with all 4 plates held simultaneously; restore-window/close behavior;
      server-authoritative

## M11 — Hold the Gate: Monster Spawner & Fixate/Retarget AI
*(Depends on M0 being committed first.)*
- [x] `CoopFixateRetargetComponent.h/.cpp` — reusable targeting component
- [x] `CoopMonsterCharacter.h/.cpp`
- [x] `CoopMonsterSpawner.h/.cpp` — Hold-the-Gate-specific timing/escalation
- [x] `BP_MonsterCharacter`, `BP_MonsterSpawner` + chamber visuals/placement
- [x] New DA_GameConstants: `MonsterHealth`, `MonsterSpawnIntervalEarlySeconds`,
      `MonsterSpawnIntervalLateSeconds`, `MonsterFixateSwitchDelaySeconds` (plus two not itemized in
      the plan but required to give the monster's attack real numbers — `MonsterAttackDamage`,
      `MonsterAttackIntervalSeconds` — see log)
- [x] Verify: monsters target plate-holders, retarget correctly, server-driven spawn timing

## M12 — Hold the Gate: Escalation Tuning, Win/Lose Integration, Full Playtest
- [x] `CoopHoldTheGateScene.h/.cpp` extended: win/lose state machine, `ResetScene()`,
      `CompleteScene()`
- [x] New DA_GameConstants: `HoldTheGateSceneDurationSeconds`
- [x] Verify: win path, both reset paths (duration-expiry-without-win, restore-window-expiry) all
      confirmed live via reflection (see log) — a genuine bug (4 duplicate stray pressure plates
      left over from M10) was found and fixed along the way. **Still open, not blocking:** the
      plan's own suggested human/solo playtest for escalation *pacing feel* (Shield-alone-sufficient
      → insufficient curve) — reflection can confirm the win/lose logic is correct but not whether
      the curve feels right. Same carve-out as M7/M8's ability-feel playtests.

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

- **M5 C++ side done; UMG content partially done, blocked on a real, previously-documented tooling
  gap.** Wrote the four C++ widget bases: `CoopRoleSelectWidget.h/.cpp` (`GetRoleSelectVisibility()`,
  `GetRoleSelectRemainingSecondsText()`, and five parameterless `IsXAvailable()`/`ClaimX()` pairs —
  one per role — since UMG's Designer-side "Bind Function"/`OnClicked` wiring needs parameterless
  functions, not `IsRoleTaken(EPlayerRole)`/`ClaimRole(EPlayerRole)` directly), `CoopPrepCountdownWidget.h/.cpp`
  (`GetPrepArenaVisibility()`, `GetPrepRemainingSecondsText()`, mirrors `CoopMatchTimerWidget`),
  `CoopSynergyHintWidget.h/.cpp` (hardcoded Tank/Control hint row, deliberately vague per CLAUDE.md
  §6.3), and `CoopAbilityCardWidget.h/.cpp` (per-instance `CardIndex` 0-3, reads the local player's
  `PlayerState->GetRole()`, looks up a hardcoded per-role ability table sourced from
  `docs/abilities.md`'s specced abilities only). Added `RoleSelectWidgetClass`/`RoleSelectWidget` and
  `PrepArenaHUDWidgetClass`/`PrepArenaHUDWidget` to `CoopPlayerController`, created+added-to-viewport
  in `BeginPlay` exactly like the existing `MatchTimerWidget`.
  **Hit the Live Coding crash logged in `DECISIONS.md`** ("Live Coding must not be used to add a new
  UCLASS/UPROPERTY/UFUNCTION") when `CoopAbilityCardWidget` was added and a Live Coding compile was
  triggered — editor crashed (`EXCEPTION_ACCESS_VIOLATION` in the patch DLL's `DllMain`). Recovered
  per that entry's own fix: closed the editor, did a full external rebuild (confirmed via
  `UnrealEditor-Unreal_first_Game.dll`'s timestamp moving to 21:59, after the 21:50 crash), reopened
  the editor cleanly (fresh log, no errors) — this is what a new session found already done on
  resuming, not something this session had to redo.
  **UMG content:** created all four `WidgetBlueprint` assets (`BlueprintTools.create` with
  `asset_type = /Script/UMG.UserWidget`, same pattern already proven for `WBP_MatchTimer` in Build 0)
  under `/Game/Blueprints/UI/`: `WBP_AbilityCard`, `WBP_TeamSynergiesPanel`, `WBP_PrepArenaHUD`,
  `WBP_RoleSelect`. Reparented each to its C++ base (`BlueprintTools.set_parent`), compiled each
  (`compile_blueprint`, confirmed clean via `LogBlueprint`'s "Compiling Blueprint" entries with no
  following errors/warnings), and confirmed the reparent stuck by reading back `get_parent` on all
  four and `CardIndex` existing on `WBP_AbilityCard`'s CDO. Wired `BP_PlayerController`'s
  `RoleSelectWidgetClass` → `WBP_RoleSelect_C` and `PrepArenaHUDWidgetClass` → `WBP_PrepArenaHUD_C`
  (`ObjectTools.set_properties` + `compile_blueprint`), then re-verified every widget-class property
  on the CDO afterward per the plan's documented "corollary gotcha" (`MatchTimerWidgetClass` survived
  unchanged) and spot-checked `BP_GameMode`'s own class refs (`PlayerStateClass`/`GameStateClass`/
  `DefaultPawnClass`/`PlayerControllerClass`) for the same silent-reset risk — none regressed.
  `save_assets([])` after every compile.
  **Confirmed the actual blocker, not just recalled it: `unreal-mcp` still has no tool to populate a
  widget's `WidgetTree` or create a Designer-side "Bind Function"/`OnClicked` association**, the exact
  gap `BUILD_0_PROGRESS.md`'s M6 log already found for `WBP_MatchTimer`. This session re-confirmed it
  structurally rather than just citing the old note: enumerated every tool in `BlueprintTools` (~60
  tools — graph/node/variable/function/event-dispatcher manipulation, none touching `UWidget`s or a
  `WidgetTree`) and every tool in `ObjectTools` (`get/set/reset_properties`, `list_properties`,
  `get_class`, `search_subclasses` — generic UObject property access only, no way to construct a new
  `UWidget` or attach one to a tree). So the four widgets exist, are correctly typed, and are
  correctly wired into `BP_PlayerController`, but they are visually and functionally empty — no Text
  Blocks, no Buttons, no bindings — until a human does the in-Designer work below.
  **What's left — needs manual Editor UI work, then a re-verify pass** (same shape as
  `BUILD_0_PROGRESS.md`'s M6 WBP_MatchTimer completion, scaled to four widgets):
  1. **`WBP_AbilityCard`**: add a Text Block bound (Bind Function) to `GetCardName`, another bound to
     `GetCardDescription`, wrap both in a container (Border/VerticalBox) whose Visibility is bound to
     `GetCardVisibility`.
  2. **`WBP_TeamSynergiesPanel`**: one Text Block bound to `GetHintText`. Nothing else — CLAUDE.md
     §6.3 says this panel must only reveal *that* a relationship exists, never the solution.
  3. **`WBP_PrepArenaHUD`**: root panel Visibility bound to `GetPrepArenaVisibility`; a countdown Text
     Block bound to `GetPrepRemainingSecondsText`; four `WBP_AbilityCard` child instances with
     `CardIndex` set to 0/1/2/3 in each instance's own Details panel; one `WBP_TeamSynergiesPanel`
     child instance.
  4. **`WBP_RoleSelect`**: root panel Visibility bound to `GetRoleSelectVisibility`; a countdown Text
     Block bound to `GetRoleSelectRemainingSecondsText`; five rows (Tank/Support/Runner/Control/
     Damage), each a static Text label naming the role plus a Button whose IsEnabled is bound to that
     role's `IsXAvailable` and whose `OnClicked` calls that role's `ClaimX` (five independent
     bindings: Tank→`IsTankAvailable`/`ClaimTank`, Support→`IsSupportAvailable`/`ClaimSupport`,
     Runner→`IsRunnerAvailable`/`ClaimRunner`, Control→`IsControlAvailable`/`ClaimControl`,
     Damage→`IsDamageAvailable`/`ClaimDamage`).
  5. Save each widget, then re-run 5-client PIE and check off M5's remaining boxes.
  **What was verified this session despite the widget-tree gap:** 5-client PIE — confirmed all 5 real
  `BP_PlayerController_C` instances exist and no `CreateWidget`/UMG errors appear in the log around
  their `BeginPlay` (the two new `CreateWidget<UUserWidget>` calls don't crash even with empty widget
  trees); confirmed `OnRosterComplete`/`RoleSelect`→`Prep` phase transition still fires correctly
  (`CurrentPhase=Prep`, `RoleSelectEndServerTime≈30.09`, `PrepPhaseEndServerTime≈90.25`, internally
  consistent); confirmed all 5 `PlayerState`s auto-resolved to distinct roles (Tank/Control/Runner/
  Support/Damage, one real PIE player each — no manual claim, same auto-resolve path as M3/M4) — this
  is exactly the live data `CoopAbilityCardWidget`/`CoopRoleSelectWidget`'s functions will read from
  once bound, so the data path is confirmed correct even though the bindings themselves can't be
  exercised yet. `StopPIE`.
  **Explicitly NOT verified (deferred, matching this project's established practice for anything
  needing a real click or a bound widget instance):** actual button clicks/`OnClicked` firing (no
  input-injection tool exists, and there's no button yet to click); two-clients-claiming-the-same-role
  contention *through the UI* (the server-side rejection itself was already verified via reflection in
  M3 — only the not-yet-existing widget wiring is untested); any layout/readability/visual polish.
  Next: finish the five manual Designer steps above, re-verify, check off M5's remaining boxes, then
  M6 (Health & Damage Foundation).

- **User completed the manual Designer work; re-verified with real content in place, and found one
  genuine bug (layout, not logic).** All four `.uasset` files grew substantially on disk (e.g.
  `WBP_RoleSelect` ~24KB → ~72KB, consistent with 5 buttons plus event-graph wiring), confirming real
  content was added, not just re-saved empty.
  **Structural check:** recompiled all four (`compile_blueprint`) — clean, no errors/warnings in
  `LogBlueprint` around any of the four compiles. `list_properties` on `WBP_AbilityCard`'s CDO still
  shows `cardIndex`; cross-checked against the same call on the known-good `WBP_MatchTimer` CDO and
  confirmed neither exposes WidgetTree-nested child widgets this way (a calibration check, not a
  finding — this method genuinely can't see Designer-placed content either way, so its earlier
  "nothing there" read for M5's first attempt wasn't itself proof of an empty tree).
  **Definitive proof of correct wiring, not just presence:** `read_graph_dsl` on `WBP_RoleSelect`'s
  `EventGraph` returned five distinct `OnClicked` handlers, each on a different button, each calling
  exactly the right claim function — `Button_252→ClaimTank`, `Button_346→ClaimControl`,
  `Button_547→ClaimRunner`, `Button_714→ClaimSupport`, `Button_878→ClaimDamage`. All five present,
  all five distinct, none dangling. This is the strongest evidence available without an
  input-injection tool (still doesn't exist) — the graph-level wiring is provably correct even though
  an actual mouse click still can't be simulated.
  **5-client PIE, full natural run (no manual claim, same as every prior milestone's test):** no
  `CreateWidget`/`WidgetTree`/UMG runtime errors anywhere in the log for the whole session. Phase
  progressed `RoleSelect` (31.4s) → `Prep` (ended 91.7s) → `HoldTheGate` correctly, matching M4's
  established timing shape with the real widgets now attached (previously only proven with empty
  widget trees). All 5 `CoopPlayerState`s auto-resolved to distinct roles (Control/Damage/Runner/
  Support/Tank), confirming the auto-resolve path is unaffected. Screenshotted the live Prep-phase
  HUD via `CaptureEditorImage` (decoded the returned base64 PNG to a file and viewed it directly,
  since the raw payload is too large to inspect any other way) and confirmed **real, correct text is
  rendering**: the host PIE client's role resolved to Control, and its ability-card row showed
  exactly 3 visible cards reading "[Stabili]ze", "[Mind F]racture", "[Chann]el" — Control's actual 3
  abilities, in the right order, with the 4th slot (`CardIndex=3`, past Control's ability count)
  correctly collapsed rather than showing a garbage/empty card. This is solid indirect proof the four
  `WBP_AbilityCard` instances inside `WBP_PrepArenaHUD` do have distinct `CardIndex` values 0/1/2/3
  as instructed, and that `GetCardVisibility()`'s past-count collapse logic works live. No
  `WBP_RoleSelect` content was visible in the same shot (consistent with the phase having already
  advanced past `RoleSelect` by the time of capture), which is itself a correct-visibility signal —
  the two screens aren't both showing at once.
  **Real bug found, not fixed (Designer-only work, same tooling-gap reasoning as before — not
  something to patch via C++ or reflection):** the ability cards' description text overlaps
  illegibly — all three visible cards' description lines render on top of each other on a single
  shared row ("Cast on a shielded...Cast on a marked...link to the whole team." all run together),
  even though the three name labels above them *are* spaced correctly side by side. This points at
  the description `Text Block`s not being properly contained/sized within each card's own
  Border/VerticalBox (likely missing a width constraint or wrap setting, or the description Text
  Blocks ended up as siblings outside their card's container rather than nested inside it) — needs a
  human look in the `WBP_AbilityCard`/`WBP_PrepArenaHUD` Designer to fix. `WBP_TeamSynergiesPanel`'s
  hint text was not confirmed visible in the same screenshot (may be off the captured region, or may
  share the same overlap issue) — unconfirmed either way, worth checking during the same fix pass.
  `StopPIE`.
  **Still explicitly NOT verified (same limits as before, unchanged):** actual mouse-driven
  `OnClicked` firing (no input-injection tool); two-clients-claiming-the-same-role UI contention;
  general layout/readability beyond the specific overlap bug found above.
  Next: fix the `WBP_AbilityCard`/`WBP_PrepArenaHUD` text-overlap bug in the Designer (human), confirm
  `WBP_TeamSynergiesPanel`'s hint text renders correctly, do a human playtest pass (buttons, layout),
  then check off M5's final Verify box and move to M6 (Health & Damage Foundation).

- **User attempted the text-overlap fix; re-verified, and found the symptom changed but the bug is
  still not resolved.** Structural check: recompiled all four widgets clean (`compile_blueprint`, no
  errors in `LogBlueprint`). `list_graphs`/`read_graph_dsl` on `WBP_RoleSelect`'s `EventGraph`
  re-confirmed all 5 `OnClicked` handlers still correctly wired to their claim functions
  (`Button_252→ClaimTank`, `Button_346→ClaimControl`, `Button_547→ClaimRunner`,
  `Button_714→ClaimSupport`, `Button_878→ClaimDamage`) — the Designer edit session between the last
  two verification passes didn't disturb this.
  **5-client PIE, natural run:** `RoleSelect` (30.0s) → `Prep` (60.0s) progressed correctly
  (`OnRosterComplete`/`ResolveRoleSelection`/`Prep phase started` all logged as expected), zero
  `CreateWidget`/`WidgetTree`/UMG runtime errors in the log for the whole session.
  **Screenshotted during Prep phase (`CaptureEditorImage`, cropped/upscaled a PIE client window for
  legibility) and confirmed the previous overlap bug is gone, but replaced by a different, still-real
  layout bug:** the ability-card description text no longer overlaps horizontally — instead it now
  wraps at an extremely narrow width, roughly one-to-two characters per line, cascading down 10+ lines
  below each card's name label and running off the bottom of the visible HUD area. This reads as
  "Auto Wrap Text" now being enabled (correct direction) but paired with a container/description
  `Text Block` width that's far too narrow (e.g. a `Size Box` `Width Override` left at a small default,
  or a parent container that's shrinking to near-zero width) rather than the ~200-250px suggested in
  the original fix note. Confirmed this pattern on both visible cards in the captured client's row, so
  it's a widget-level layout issue, not a per-instance fluke.
  **`WBP_TeamSynergiesPanel`:** a single line of white text is visible in the expected position (to
  the right of the ability cards) in the same screenshot, which is structurally consistent with it
  correctly rendering as one line rather than wrapping — but the base capture resolution (the full
  5-client desktop screenshot is only 1280x397px before cropping) is too low to read the actual words
  even after upscaling, so the exact text still can't be confirmed either way from this method.
  `StopPIE`.
  **Still explicitly NOT verified (same limits as before, unchanged):** actual mouse-driven `OnClicked`
  firing (no input-injection tool); two-clients-claiming-the-same-role UI contention; general
  layout/readability beyond the specific bugs discussed above.
  **M5's Verify box stays unchecked.** Next: in the `WBP_AbilityCard` Designer, widen the description
  Text Block's container (increase the `Size Box` `Width Override` to roughly 200-250px, or remove an
  overly-narrow constraint) so `Auto Wrap Text` wraps at word boundaries across a few lines instead of
  near-single characters, confirm `WBP_TeamSynergiesPanel`'s hint text reads correctly at normal zoom
  in the Designer/editor (not just inferred from a low-res screenshot), do a human playtest pass
  (buttons, overall layout), then check off M5's final Verify box and move to M6 (Health & Damage
  Foundation).

- **Session resumed on the width-bug's parked note; requested check found the C++ side of M6 already
  written (by a prior session, uncommitted) but not yet verified.** User asked to verify
  `CoopHealthComponent`/`CoopAbilityCardWidget` and the four widgets (`WBP_AbilityCard`,
  `WBP_PrepArenaHUD`, `WBP_RoleSelect`, `WBP_TeamSynergiesPanel`) load with no compile/reference
  errors, then continue the tracker. `git diff --stat` confirmed the actual working-tree state: M6's
  full C++ side (`CoopHealthComponent.h/.cpp`, `ACoopCharacter` component wiring, `DefaultMaxHealth`)
  plus the remaining M5 widget C++ (`GetRoleSelectVisibility`, five `IsXAvailable`/`ClaimX` pairs,
  `GetPrepArenaVisibility`) already existed uncommitted — this session picks up verifying that work,
  not writing it.
  **Compile/reference check (the literal ask):** `search_subclasses` confirmed
  `UCoopHealthComponent`, `UCoopAbilityCardWidget`, `UCoopRoleSelectWidget`, `UCoopPrepCountdownWidget`
  are all registered in the already-running editor's loaded module (so the C++ compiles clean — no
  Live Coding needed this session, the binary was already built with this code). Log swept for errors
  around startup: only benign noise (missing profiler DLLs, an unrelated `LogModelContextProtocol`
  MCP-protocol error, an engine unit-test's own deliberate `LogTemp: Error test:` lines) — zero
  UHT/module-load errors. All four widgets' `get_parent` confirmed still correctly reparented to their
  C++ bases; `compile_blueprint` on all four came back clean with no follow-up `LogBlueprint`
  errors/warnings. `read_graph_dsl` on `WBP_RoleSelect`'s `EventGraph` re-confirmed all 5 `OnClicked`
  handlers still correctly wired (`Button_252→ClaimTank`, `Button_346→ClaimControl`,
  `Button_547→ClaimRunner`, `Button_714→ClaimSupport`, `Button_878→ClaimDamage`), and `WBP_AbilityCard`'s
  CDO still exposes `cardIndex`. **No compile or reference errors found anywhere in this pass.**
  **Continued into M6's own Verify step, 5-client PIE, and found a real bug — caught before it could
  hide:** all 5 pawns' `HealthComponent` initialized to `100/100` as expected, but the log showed
  `UCoopHealthComponent::BeginPlay: GameConstants not set on BP_PlayerCharacter_C_X, falling back to
  100.0` for all five. The `.h`'s own comment had already flagged this exact risk ("set this via
  BP_PlayerCharacter's component defaults... not directly on the C++ CDO") and it had been missed —
  the component was silently running on its hardcoded fallback, not `DA_GameConstants`, only invisible
  because the fallback (100.0) happens to equal `DefaultMaxHealth` (100.0). **Fixed, not just
  logged:** `set_properties` on `BP_PlayerCharacter`'s CDO (`HealthComponent.GameConstants` →
  `/Game/Data/DA_GameConstants.DA_GameConstants`), recompiled the blueprint (clean, no
  `LogBlueprint` errors, property survived the compile), `save_assets`. Re-ran 5-client PIE: the
  warning no longer appears, and reflection on a live pawn confirms `HealthComponent.GameConstants`
  now genuinely points at the DataAsset. Phase progression (`OnRosterComplete`→`RoleSelect`→
  `ResolveRoleSelection`→`Prep`) unaffected.
  **Damage/replication/clamping exercised live, not deferred:** `ApplyTestDamage` is a `UFUNCTION(Exec)`
  with no reflection call path (same tooling gap as `Server_ClaimRole` in M3 — `ObjectTools` still has
  no function-call capability), so asked the user to type it into a live PIE client console rather than
  deferring the check outright. User ran `ApplyTestDamage 30` then `ApplyTestDamage 90` against the
  same pawn; log confirmed `70.0/100.0` then `0.0/100.0` (clamped, not negative, on a 100-HP pawn hit
  for 120 total). **Found and corrected a real methodology mistake before it became a false bug
  report:** naively re-reading the same-numbered actor (`BP_PlayerCharacter_C_0`) from a second PIE
  world context (`UEDPIE_1`) showed stale `100/100` and looked like a replication failure — but
  cross-referencing `PlayerState.PlayerId` (stable across clients) proved `UEDPIE_0`'s damaged `C_0`
  (`PlayerId=261`, Tank) corresponds to `UEDPIE_1`'s `C_2` (`PlayerState_4`, `PlayerId=261`, Tank), not
  `UEDPIE_1`'s own `C_0` (`PlayerId=265`, a different player entirely). **Per-world PIE actor numbering
  does not correspond 1:1 across clients — cross-check via `PlayerState.PlayerId`, not actor name,
  when comparing the same networked actor across two PIE world contexts.** Worth remembering for any
  future multi-client replication check. Once the correct actor was identified, `UEDPIE_1`'s
  `C_2.HealthComponent` read `0.0/100.0` — exactly matching the server, confirming real replication and
  correct floor-clamping across the network, not just locally. 0-HP delegate fire-once was verified by
  code review only (`bWasAboveZero && CurrentHealth <= 0` guard, single `Broadcast()` call) since
  `OnHealthDepleted` has no bound listener until M9 gives it one — no way to observe a fire count at
  runtime yet, same reasoning as M1's tag-registration verification. Final log sweep: no new errors
  from this PIE session. `StopPIE`.
  Both M5's remaining widget C++ and M6 are now fully verified (M5's checkbox itself stays unchecked
  per the still-parked text-wrap width bug — unrelated, cosmetic, untouched this session). Next: M7
  (Tank Shield Ability).

- **M7 started — C++ and the Input Action asset done; paused for a required full rebuild before
  continuing.** Added `Source/Unreal_first_Game/Abilities/CoopTankAbilities.h/.cpp` (first file in
  `Abilities/`): a plain `namespace CoopTankAbilities` (not a `UCLASS` — server-only resolution
  logic, no reason to be reflected, per CLAUDE.md §4.6) holding `ApplyShield(ACoopCharacter* Tank,
  const UGameConstants* GameConstants)`. It cooldown-gates via a new `ShieldCooldownEndServerTime`
  field on `ACoopCharacter`, applies `Status.Shielded` to Tank via a new shared status-tag mechanism
  (see below), then iterates every `ACoopCharacter` via `TActorIterator` and applies the same tag to
  anyone within `ShieldCoverageRadiusUnits`/`ShieldCoverageAngleDegrees` of Tank's forward vector —
  a cast-time snapshot per docs/abilities.md ("the actor(s) currently standing behind it"), not a
  per-hit direction check.
  **`ACoopCharacter` gained the shared status-tag mechanism M7/M8/M9 all need:** a replicated
  `FGameplayTagContainer ActiveStatusTags`, a non-replicated `TMap<FGameplayTag, float>
  StatusTagExpiryServerTime` (server-only, kept purely for reflection-based debugging per CLAUDE.md
  §4.3), and a plain `TMap<FGameplayTag, FTimerHandle> StatusTagExpiryTimers` driving actual removal.
  `ApplyStatusTag(Tag, DurationSeconds)`/`RemoveStatusTag(Tag)` are server-only
  (`HasAuthority()`-gated); `HasStatusTag(Tag)` is `BlueprintPure` and safe on any client since it
  just reads the replicated container. Timer-driven expiry uses `GetWorldTimerManager().SetTimer`
  with a duration in seconds (not per-frame `DeltaTime` accumulation), and the recorded expiry
  timestamp itself comes from `AGameStateBase::GetServerWorldTimeSeconds()` — satisfies CLAUDE.md
  §4.4/§4.5 without a manual Tick-based poll loop.
  **`UCoopHealthComponent::ApplyDamage`** gained a `Status.Shielded` early-out negation check.
  **Documented simplification, not the full spec:** negates *all* incoming damage while shielded,
  not just damage "from that facing" as docs/abilities.md describes — `ApplyDamage` has no damage-
  source location to check a facing against, and nothing in the codebase deals real damage yet
  (M11's monsters are the first real attacker). Flagged in a comment as a contained follow-up for
  whenever a real attack exists to test a directional check against, per CLAUDE.md §1's "simple
  now, note the tradeoff" allowance.
  **`ACoopPlayerController`** gained `ActivateShield()` (`BlueprintCallable` — thin wrapper, same
  shape as `UCoopRoleSelectWidget::ClaimTank()`/etc. wrapping their RPCs, since Blueprint never
  calls a raw `Server_*` RPC directly in this project) and `Server_ActivateShield()` (role-gates to
  `EPlayerRole::Tank`, silent no-op otherwise — friends, not adversarial input, CLAUDE.md §8).
  **`GameConstants.h`** gained `ShieldDurationSeconds` (5.0), `ShieldCooldownSeconds` (8.0),
  `ShieldCoverageAngleDegrees` (90.0), `ShieldCoverageRadiusUnits` (300.0).
  **Input Action asset, done ahead of the rebuild since it needs none of the new C++ symbols:**
  duplicated `/Game/Input/Actions/IA_Jump` → `IA_Shield` (same shape: `ValueType=Boolean`,
  `bConsumeInput=true`, `Pressed`+`Released` triggers). Added a new `E` key mapping for it to
  `/Game/Input/IMC_Default`'s `DefaultKeyMappings` array via `set_properties` (read the full
  existing 12-entry array first, appended one entry, wrote it back — confirmed all 12 original
  entries survived unchanged plus the new one). `save_assets` on both.
  **Real discovery, corrects the plan document's wording:** the plan file says "bound in
  BP_PlayerController's existing BeginPlay input block," but reflection on the actual project
  (`read_graph_dsl` on both Blueprints) shows `AddMappingContext` calls live in
  `BP_PlayerController`'s `EventBeginPlay`, while the actual `EnhancedInputActionIA_*` event nodes
  (Move/Look/Jump) live in **`BP_PlayerCharacter`'s EventGraph**, not the Controller's. Following
  the real established pattern (not the plan's paraphrase of it) — `IA_Shield`'s event node will go
  in `BP_PlayerCharacter` too, calling `Get Controller` → `Cast to CoopPlayerController` →
  `ActivateShield`.
  **Stopped here because this session's C++ changes require a full rebuild, not Live Coding.**
  `ActiveStatusTags`/`ShieldCooldownEndServerTime` are new `UPROPERTY`s on the already-loaded
  `ACoopCharacter`, and `ActivateShield`/`Server_ActivateShield` are new `UFUNCTION`s on the
  already-loaded `ACoopPlayerController` — exactly the case DECISIONS.md's "Live Coding must not be
  used to add a new UCLASS/UPROPERTY/UFUNCTION" entry warns crashed the editor once already this
  build. Asked the user to close the editor and run a full external build (Visual Studio/Rider, or
  `Build.bat`) rather than Ctrl+Alt+F11, then reopen the editor so `unreal-mcp` reconnects. Next,
  once the editor is back up: confirm the new classes/functions load (`search_subclasses` for
  `ActiveStatusTags`/no reflection errors), wire `IA_Shield`'s event node into `BP_PlayerCharacter`
  via `create_node`/`connect_pins` (not `write_graph_dsl`, to avoid any risk of clobbering the
  existing Move/Look/Jump wiring in that same graph), 5-client PIE reflection-verify tag application/
  expiry/negation, then a human keypress + playtest pass for feel (per the plan's own note that
  reflection can't prove an Input Action fires or feels right).

- **M7 resumed after the rebuild: full rebuild confirmed clean, `IA_Shield` wired into
  `BP_PlayerCharacter`, tag application and damage negation confirmed live. Expiry check
  interrupted — pick up from there.** Full external rebuild succeeded (`CoopTankAbilities.cpp`,
  `CoopCharacter.cpp`, `CoopHealthComponent.cpp`, `CoopPlayerController.cpp`, `GameConstants.cpp` all
  compiled, `UnrealEditor-Unreal_first_Game.dll` linked, ~25s). Editor reopened, `unreal-mcp`
  reconnected. `list_properties` on `BP_PlayerCharacter`'s CDO confirmed `activeStatusTags`
  (`GameplayTagContainer`) and `statusTagExpiryServerTime` (`TMap`) both reflect correctly — no UHT
  errors. `ActivateShield`/`Server_ActivateShield` confirmed present via `CoopPlayerController.h`
  read (not reflectable themselves — RPCs aren't UPROPERTYs — but the header/source read confirms
  the code exists and matches the plan).
  **Wired `IA_Shield`'s event node into `BP_PlayerCharacter`'s `EventGraph`, via `create_node`/
  `connect_pins` exactly as the prior session planned (not `write_graph_dsl`):** 5 new nodes —
  `Input|EnhancedActionEvents|IA_Shield` (event), `Variables|Getareferencetoself` (note: this is the
  *creatable* `type_id`; `get_node_type_pins`/`get_node_infos` report the same node's `type_id` back
  as `Variables|Self-Reference` — the two strings refer to the same node class, just surfaced
  differently depending which tool call you use), `Pawn|GetController`, `Utilities|Casting|
  CastToCoopPlayerController`, `Abilities|ActivateShield`. Wired: `Self.self` → `GetController.self`;
  `IA_Shield.Triggered` → `Cast.execute`; `GetController.ReturnValue` → `Cast.Object`; `Cast.then` →
  `ActivateShield.execute`; `Cast.AsCoop Player Controller` → `ActivateShield.self`. `CastFailed` left
  unconnected (silent no-op if `GetController()` ever returns non-`CoopPlayerController`, which
  shouldn't happen in this project).
  **Real tooling gotcha found, not a wiring bug:** `read_graph_dsl` renders the new event with an
  empty body (`(event EnhancedInputActionIA_Shield (ActionValue ...))`, no nested calls) even though
  the wiring is fully connected — the DSL reader apparently doesn't reconstruct a body that starts
  from a non-default exec pin (`Triggered`, here) the way it does for the default/only exec pin on
  simpler events. This is a read-side limitation of `read_graph_dsl`, not a real absence of wiring —
  confirmed by cross-checking with `get_node_infos` on all 5 new nodes by their real `refPath`s
  (not `get_node_type_pins`, which only ever creates/queries an ephemeral preview node and can't see
  live graph state), which showed every connection above present and correct on the actual live
  nodes. **Worth remembering for any future graph-verification work in this project: `read_graph_dsl`
  is not reliable proof of absence for chains off a non-default exec pin — use `get_node_infos` on
  the specific node refs instead.** `arrange_nodes` tidied the new node cluster's layout.
  `compile_blueprint` came back clean (`LogBlueprint` showed only the "Compiling Blueprint" line, no
  follow-up errors/warnings — same clean-compile signature as every prior milestone). `save_assets`.
  **5-client PIE, tag application and damage negation confirmed live — genuinely exercised, not
  code-review-only:** started PIE, confirmed 5 real `CoopPlayerState`s via `find_actors`. First
  attempt: user pressed E on all 5 windows *before* the 30s `RoleSelect` auto-resolve had completed
  (everyone still `Unassigned`), so nothing happened — correctly explained as expected behaviour
  (`Server_ActivateShield` is role-gated to Tank; every player being `Unassigned` means every press
  is a legitimate no-op), not a bug, and not what was being tested. Restarted PIE, waited out the
  30s `RoleSelect` timeout properly this time, re-read all 5 roles (`Tank` landed on
  `CoopPlayerState_3`/`PlayerId=264` this run — role auto-assignment is randomized per session, does
  not repeat the same distribution as earlier milestones' logged runs).
  **Real methodology problem hit and solved, not worked around by lowering rigor:** `ShieldDurationSeconds`
  is only 5.0s — far too short a window to reliably catch via a human-mediated
  AskUserQuestion round-trip (user has to read the question, alt-tab to the right PIE window,
  press the key, alt-tab back, answer — routinely takes well over 5 real seconds). Rather than
  accept an unreliable race, temporarily widened `ShieldDurationSeconds` to 180 on the live
  `DA_GameConstants` asset (a sanctioned, reversible, already-designed-for-this tunable per
  CLAUDE.md §10 — not a code change) via `ObjectTools.set_properties`, confirmed the Tank window
  visually via its distinctive ability-card text ("Raise a barrier..."/"Break Mark a target...",
  since role-to-window mapping isn't fixed and varies per session), had the user press E once, then
  read `BP_PlayerCharacter_C_3`'s `activeStatusTags`/`statusTagExpiryServerTime` via reflection:
  `Status.Shielded` present, expiry timestamp `533.88` (server time) — consistent with the widened
  180s duration. **Damage negation confirmed via the same live pawn while shielded:** had the user
  type `ApplyTestDamage 30` into the Tank window's own console (the RPC targets the calling
  controller's own pawn, confirmed by reading `Server_ApplyTestDamage_Implementation`). Log line:
  `ApplyTestDamage: BP_PlayerCharacter_C_3 took 30.0, now 100.0/100.0.` — health unchanged despite
  "taking" 30 damage, confirming `UCoopHealthComponent::ApplyDamage`'s `Status.Shielded` early-out
  negation actually fires against a live, ability-applied tag (not just a reflection-poked one).
  Restored `ShieldDurationSeconds` back to 5.0 on `DA_GameConstants` afterward, confirmed via
  `get_properties`, `save_assets`.
  **Expiry check started but not completed — session ended here.** Set the duration back to the
  real 5.0s value and asked the user to press E once more on the Tank window (cooldown from the
  widened-duration test should have long since cleared by then) so expiry could be confirmed by
  reading `activeStatusTags` again after a self-paced wait (not racing a human round-trip this time
  — the plan was to capture the fresh application, then `sleep` past 5s myself before re-checking,
  since `RemoveStatusTag` erases both the tag *and* its `StatusTagExpiryServerTime` entry on fire, so
  post-expiry state alone can't distinguish "never fired" from "fired and expired" without bracketing
  it with a fresh, timestamped application). User asked to stop for the day before that last press
  was confirmed. `StopPIE` called, `DA_GameConstants` re-verified at `ShieldDurationSeconds=5.0`/
  `ShieldCooldownSeconds=8.0` (unchanged from before this session) and saved to disk.
  **Next session: resume the expiry check first (~2 minutes of work) before considering M7 done.**
  Start PIE, wait for `RoleSelect` to auto-resolve (~30s), identify the Tank window via its
  ability-card text, have the user press E once, immediately read that pawn's `activeStatusTags`/
  `statusTagExpiryServerTime` to confirm the fresh application, then wait (self-paced, e.g. a 7-8s
  `Bash` sleep — no need to involve the user again) and re-read the same properties to confirm
  `Status.Shielded` is gone and the expiry map entry is gone too. Once that's clean, check off M7's
  Verify box, `StopPIE`, and move to M8 (Control Stabilize + Fortress Synergy Conditional). Also
  still open, deferred from M7's own log, not new: a human playtest pass for how Shield *feels*
  (CLAUDE.md's own carve-out — reflection can confirm correctness but not feel).

- **M7 expiry check completed — M7's Verify box now checked.** Resumed exactly where the prior
  session stopped. `IsPIERunning` confirmed clean state, `StartPIE` (warmup 2s), waited (background
  grep-wait on the log for `Prep phase started`, the RoleSelect→Prep transition line) for the 30s
  `RoleSelect` auto-resolve. Identified this run's Tank via reflection rather than a screenshot
  guess: `find_actors` on the server world (`UEDPIE_0`) listed all 5 `CoopPlayerState`s;
  `CoopPlayerState_2` read `PlayerRole=Tank, PlayerId=258`. Cross-checked which PIE client window
  that corresponds to by reading each `UEDPIE_N`'s own local `BP_PlayerController_C_0.PlayerState`
  directly via its own soft path (confirms the M6 log's finding that per-world actor numbering
  doesn't line up across clients — `PlayerId` is the only stable cross-reference): `UEDPIE_2`'s
  local player carries `PlayerId=258`, so `UEDPIE_2` is this run's Tank client.
  **Tried identifying the on-screen window by decoding a full-desktop `CaptureEditorImage` capture**
  (base64 PNG was too large for the tool result directly — saved to a scratch file, decoded via a
  small Python script, viewed with `Read`) but the captured ability-card text was too ambiguous to
  reliably tell windows apart at that resolution. **Asked the user directly instead**, giving Tank's
  exact hardcoded card text (from `CoopAbilityCardWidget.cpp`: "Raise a barrier in front of you that
  blocks damage from that direction." / "Mark a target, opening a brief window for Control to act on
  it.") so they could self-identify the right window — more reliable than a compressed screenshot
  read, and matches the project's established pattern for this exact problem.
  **First press attempt read back empty (`ActiveStatusTags` and `StatusTagExpiryServerTime` both
  empty) — correctly diagnosed as the same round-trip-timing problem flagged in the prior session's
  log, not a real bug:** an `AskUserQuestion` round trip (read question, alt-tab, press key, alt-tab
  back, answer) routinely exceeds the real 5.0s `ShieldDurationSeconds`, so the shield had already
  fired and expired by the time reflection read it — indistinguishable from "never applied" without
  the bracketing this milestone's own log already called out. **Recovered with the exact precedent
  already established, not a new workaround:** widened `ShieldDurationSeconds` to 15.0 on the live
  `DA_GameConstants` (`set_properties`, confirmed via `get_properties` before and after), asked the
  user to press E once more in the same identified Tank window, then immediately read the pawn's
  tags: `Status.Shielded` present, `StatusTagExpiryServerTime["Status.Shielded"]=445.56` (server
  time) — fresh application confirmed. Waited ~20 real seconds (a `date`-based `until`-loop in a
  backgrounded `Bash` call, per the harness's guidance against bare `sleep`) past the widened 15s
  duration, self-paced with no user involvement, then re-read the same properties: both
  `ActiveStatusTags` and `StatusTagExpiryServerTime` empty again — confirms genuine timer-driven
  removal (`RemoveStatusTag` clearing both the tag and its expiry-map entry), not just "was never
  there." Restored `ShieldDurationSeconds` back to 5.0 (confirmed via `get_properties`), `save_assets`,
  `StopPIE`.
  **M7 fully verified: tag application, damage negation (prior session), and now expiry, all
  confirmed live against real ability-triggered state, not synthetic reflection pokes.** Checked off
  M7's Verify box. Still open, not blocking: a human playtest pass for how Shield *feels* at its real
  5.0s/8.0s duration/cooldown (CLAUDE.md's reflection-can't-prove-feel carve-out) — worth folding into
  a future full-scene playtest rather than doing solo.
  Next: M8 (Control Stabilize + Fortress Synergy Conditional).

- **M8 C++ written; paused for a required full rebuild, same as M7.** Added
  `Source/Unreal_first_Game/Abilities/CoopControlAbilities.h/.cpp` (second file in `Abilities/`,
  mirrors `CoopTankAbilities`'s shape exactly): `ResolveStabilize(ACoopCharacter* Control, const
  UGameConstants* GameConstants)` cooldown-gates via a new `StabilizeCooldownEndServerTime` field,
  then finds the nearest `ACoopCharacter` whose `PlayerState->GetRole() == EPlayerRole::Tank` within
  `StabilizeCastRangeUnits` (no targeting/crosshair UI exists yet, so "nearest Tank in range" is the
  implicit target, same "friends, not adversarial input" reasoning as every other ability here).
  **The hardcoded Fortress conditional itself, matching CLAUDE.md §4.6's literal worked example:** if
  that Tank currently holds `Status.Shielded`, removes it and applies `Status.Fortress` (duration
  `FortressDurationSeconds`) to the Tank, then extends `Status.Fortress` to every other
  `ACoopCharacter` within `FortressCoverageRadiusUnits` of the Tank (clearing their own
  `Status.Shielded` first if they had it from the original cone-coverage cast) — this is the "radius,
  not Shield's cone" multi-teammate coverage `docs/abilities.md` describes. No Tank in range, or an
  unshielded nearest Tank, is a silent whiff; the cooldown was already consumed before the check runs
  (matches Armor Break's "opens a window, doesn't guarantee a hit" philosophy).
  **`ACoopCharacter`** gained `StabilizeCooldownEndServerTime` + getter/setter, same shape as M7's
  `ShieldCooldownEndServerTime` (plain float, not replicated, not a `UPROPERTY` — Build 1's ability
  cards have no cooldown-remaining display yet).
  **`ACoopPlayerController`** gained `ActivateStabilize()`/`Server_ActivateStabilize()`, identical
  shape to `ActivateShield`/`Server_ActivateShield`: role-gates to `EPlayerRole::Control`, silent
  no-op otherwise.
  **`GameConstants.h`** gained `StabilizeCooldownSeconds` (10.0), `StabilizeCastRangeUnits` (800.0),
  `FortressDurationSeconds` (8.0), `FortressCoverageRadiusUnits` (600.0),
  `FortressKnockbackResistPercent` (0.75) — the last one has no consumer yet (no knockback exists
  until Hold the Gate's monsters land in M11), flagged in a header comment as a deliberate
  simplification per CLAUDE.md §1, matching M7's own documented "negates all damage, not just
  directional" carve-out for the same reason.
  **Stopped here because `Server_ActivateStabilize`/`ActivateStabilize` are new `UFUNCTION`s on the
  already-loaded `ACoopPlayerController`** — exactly the case DECISIONS.md's "Live Coding must not be
  used to add a new UCLASS/UPROPERTY/UFUNCTION" entry covers, same as M7 hit. Asked the user to close
  the editor and run a full external build, then reopen so `unreal-mcp` reconnects.
  **Next, once the editor is back up:** confirm the new symbols load (`search_subclasses`/no
  reflection errors), confirm the 5 new `GameConstants` fields read their C++ defaults on the live
  `DA_GameConstants` CDO via `get_properties` (same as every prior milestone's constants — no manual
  `set_properties` needed unless a value looks wrong), duplicate `IA_Shield` → `IA_Stabilize` and add
  its key mapping to `IMC_Default` (can happen in parallel with the rebuild, needs no new C++
  symbols, same as M7's `IA_Shield` prep), wire `IA_Stabilize`'s event node into
  `BP_PlayerCharacter`'s `EventGraph` via `create_node`/`connect_pins` (not `write_graph_dsl`, per
  M7's established caution), then 5-client PIE: reflection-verify Tank Shields + Control Stabilizes
  targeting that Tank produces `Status.Fortress` (replacing `Status.Shielded`), confirm whiffing on
  an unshielded Tank does nothing, and confirm cooldown consumes either way. Knockback resistance has
  no consumer yet, so nothing to verify there until M11. Human playtest for feel stays open, same
  carve-out as M7.
  **`IA_Stabilize` done ahead of the rebuild, same as M7's `IA_Shield`:** duplicated
  `/Game/Input/Actions/IA_Shield` → `IA_Stabilize` (`AssetTools.duplicate`), confirmed the duplicate
  carried over `valueType=Boolean`, `bConsumeInput=true`, Pressed+Released triggers. Read
  `IMC_Default`'s `defaultKeyMappings.mappings` (13 entries: 12 from Build 0 + M7's `IA_Shield`→`E`),
  appended `IA_Stabilize`→`Q` (E already taken by Shield), wrote the full 14-entry array back, read
  it back and confirmed all 13 original entries survived unchanged plus the new one. `save_assets`
  on both. Input-side prep for M8 is done; only the C++ rebuild and the `BP_PlayerCharacter`
  event-node wiring remain once the editor is back up.

- **M8 finished: rebuild done (via Live Coding, not a full external rebuild — see the new
  DECISIONS.md addendum), `IA_Stabilize` wired into `BP_PlayerCharacter`, Fortress synergy confirmed
  live end-to-end.** The user closed the loop on the rebuild ask by triggering an **in-editor Live
  Coding compile** rather than the full external rebuild this session had requested. Checked the log
  before trusting it: `Reload/Re-instancing Complete: 1 package changed, 2 classes changed, 13
  classes unchanged, 3 enums unchanged, 36 functions remapped`, only the same benign "data type
  changes may cause packaging to fail" warning every prior milestone's compile has also produced —
  no `EXCEPTION_ACCESS_VIOLATION`, no crash, `IsPIERunning` succeeded immediately afterward. This is
  a real counterexample to DECISIONS.md's blanket "new UPROPERTY/UFUNCTION requires a full rebuild"
  claim, so it's logged there as an addendum rather than silently treated as luck: the original
  crash's own stack trace was inside a UMG widget class's dynamic initializer while registering a
  **brand-new `UUserWidget` subclass**, not just new members on an already-existing class — this
  session's changes (new `UFUNCTION`s on `ACoopPlayerController`, new `UPROPERTY`s on
  `UGameConstants`, neither a new class nor UMG) are consistent with that narrower trigger. The
  broad rule stays the default (a new `UCLASS`, especially a widget, still needs a full rebuild
  unconditionally); new members on an existing non-widget class is now reasonable to try via Live
  Coding first, checking for the same clean-reload log signature before trusting it.
  **Confirmed via reflection before touching content:** `list_properties` on `DA_GameConstants`
  showed all 5 new M8 fields registered; `get_properties` read back their C++ defaults exactly as
  specced (10.0 / 800.0 / 8.0 / 600.0 / 0.75) with no manual `set_properties` needed, same as every
  prior milestone's constants.
  **Wired `IA_Stabilize` into `BP_PlayerCharacter`'s `EventGraph`**, mirroring `IA_Shield`'s chain
  exactly (5 new nodes: `EnhancedInputActionIA_Stabilize` event, `Self`, `Pawn|GetController`,
  `CastToCoopPlayerController`, `Abilities|ActivateStabilize`; `find_node_types` confirmed
  `Input|EnhancedActionEvents|IA_Stabilize` and `Abilities|ActivateStabilize` were both creatable
  post-rebuild). `create_node`/`connect_pins` for all 5 nodes and connections (not `write_graph_dsl`,
  per M7's established caution), verified via `get_node_infos` on all 5 by ref (not
  `read_graph_dsl`, which M7 already found unreliable for non-default exec pins) — all connections
  present and correct: `Triggered→Cast.execute`, `Self.self→GetController.self`,
  `GetController.ReturnValue→Cast.Object`, `Cast.then→ActivateStabilize.execute`,
  `Cast.AsCoopPlayerController→ActivateStabilize.self`. `arrange_nodes`, `compile_blueprint` (clean,
  only the "Compiling Blueprint" log line, no follow-up errors), `save_assets`.
  **5-client PIE, full Fortress synergy exercised live, not just tag-application in isolation:**
  identified this run's Tank (`PlayerId=263`, `UEDPIE_2`) and Control (`PlayerId=264`, `UEDPIE_3`)
  via the same PlayerId cross-reference method M7 established. **Whiff test first, correctly
  sequenced before Tank ever shielded:** had Control press Q against an unshielded Tank — read back
  `ActiveStatusTags`/`StatusTagExpiryServerTime` both empty, confirming a whiff writes nothing (and
  implicitly that the cooldown-consumes-regardless behavior didn't leave any stray tag). Then Tank
  pressed E — `Status.Shielded` confirmed present (temporarily widened `ShieldDurationSeconds` to 60
  first, same M7 precedent, to survive the `AskUserQuestion` round trip). **First Stabilize attempt
  read back empty again — correctly diagnosed as a second instance of the same round-trip-timing
  class of bug, not a new one:** `ShieldDurationSeconds` was widened but `FortressDurationSeconds`
  (8.0s) was not, so `Status.Fortress` had almost certainly applied and already expired before
  reflection could read it. Widened `FortressDurationSeconds` to 60 too, had Tank re-shield (fresh
  `Status.Shielded` confirmed), had Control cast Stabilize again: **`ActiveStatusTags` now read
  `Status.Fortress`, `Status.Shielded` gone — the upgrade behavior confirmed live.** Checked two
  other teammates' pawns (`BP_PlayerCharacter_C_0`/Support, `C_1`/Damage) with no further user
  action needed (positions already set from spawn) — both also carried `Status.Fortress`, confirming
  the multi-teammate radius coverage works, not just the Tank's own upgrade. Restored
  `ShieldDurationSeconds`→5.0 and `FortressDurationSeconds`→8.0, confirmed via `get_properties`,
  `save_assets`, `StopPIE`.
  **M8 fully verified: whiff-does-nothing, the Tank upgrade, and multi-teammate coverage all
  confirmed live against real ability-triggered state.** Checked off M8's remaining boxes. Still
  open, not blocking: knockback resistance has no consumer yet (parked until M11's monsters exist),
  and a human playtest pass for how Stabilize/Fortress *feels* (same carve-out as M7's Shield).
  Next: M9 (Downed / Revive / Wipe / Instant Retry).

- **M9 C++ written; paused for a required full rebuild — this one genuinely needs it (new UCLASS,
  not just new members on an existing class).** Asked the user first (per the plan's own flagged
  open question) whether revive should restore half or full health; user chose half (0.5), matching
  the plan's own default.
  **`ACoopCharacter`** gained `DownedComponent` (`UCoopDownedComponent`, mirrors `HealthComponent`'s
  placement) and `ReviveTriggerVolume` (a `USphereComponent`, `NoCollision` by default, attached to
  root -- mirrors `ACoopButton`'s `TriggerVolume`, since the capsule's own Pawn-vs-Pawn collision
  blocks rather than overlaps, so it can't fire the overlap Revive needs), plus
  `ApplyPersistentStatusTag(Tag)` -- same as `ApplyStatusTag` but with no expiry timer at all, for
  state (`Status.Downed`) that only clears via an explicit `RemoveStatusTag` call, not a timed buff.
  **`UCoopHealthComponent`** gained `Revive(HealthPercent)` -- server-only, restores
  `CurrentHealth` to `Percent * MaxHealth`, deliberately never touches `OnHealthDepleted` (that only
  fires on a fresh crossing into 0, never on a heal).
  **New `Source/Unreal_first_Game/Core/CoopDownedComponent.h/.cpp`** (placed in `Core/` alongside
  `CoopHealthComponent`, not a new folder -- same category of mechanic): subscribes to
  `OnHealthDepleted` to enter Downed (`ApplyPersistentStatusTag(Status.Downed)`,
  `DisableMovement()`, increments `GameState`'s Downed tally, enables+resizes
  `ReviveTriggerVolume` to `OverlapAllDynamic`/`ReviveRadiusUnits`). Revive is proximity-triggered,
  not keybound -- the plan's M9 bullet lists `Server_AttemptRevive()` on the PlayerController with no
  accompanying "new Input Action" line (unlike M7/M8), and CLAUDE.md §6.6's own wording ("standing
  adjacent for a few seconds") matches `ACoopButton`'s existing overlap-trigger shape better than a
  keypress would -- so `OnReviveTriggerBeginOverlap` reuses `ACoopButton`'s exact
  "only the client owning the overlapping pawn fires the RPC" filter (`IsLocallyControlled()`) to
  call `Server_AttemptRevive()` (no target parameter -- the RPC does its own nearest-Downed-in-range
  search server-side, matching `Server_ActivateStabilize`'s implicit-target shape).
  **Documented simplification, flagged in code, not silently taken:** "standing adjacent for a few
  seconds" is approximated as re-validated proximity at the *start* (the RPC's own range search) and
  *again at completion* (`CompleteRevive` re-checks distance + that the reviver hasn't died/gone
  Downed themselves), not a continuous per-tick channel -- a true continuous channel would need
  either `Tick` or repeated client RPCs while held, more than this milestone needs per CLAUDE.md §1.
  `ForceRevive()` is the separate, no-channel, full-heal path for a full-party scene reset.
  **`ACoopGameState`** gained `DownedPlayerCount` (replicated), `GetDownedPlayerCount()`,
  `IsPartyWiped()` (`DownedPlayerCount > 0 && DownedPlayerCount >= PlayerArray.Num()` -- confirmed
  dummies get real `PlayerState`s too via `AController::Possess`'s own `InitPlayerState`, so
  `PlayerArray.Num()` reliably reads 5 once the roster completes, dev-mode or not),
  `IncrementDownedCount()`/`DecrementDownedCount()` (called by `UCoopDownedComponent::SetDowned`),
  and `RequestSceneReset()` -- the scene-agnostic half of CLAUDE.md §6.6's "instant restart":
  iterates every `ACoopCharacter`, calls `ForceRevive()` on each one's `DownedComponent`, then
  broadcasts a new `OnSceneResetRequested` delegate for a future scene class to layer its own
  scene-specific reset onto (respawn positions, reset plates/gates) -- nothing binds to it yet since
  no scene class exists until M10; this milestone only builds the mechanism it will use.
  **`ACoopPlayerController`** gained `Server_AttemptRevive()` (the nearest-Downed-in-range search +
  `BeginRevive()` call described above) and a Downed lockout check added to both
  `Server_ActivateShield_Implementation` and `Server_ActivateStabilize_Implementation` (CLAUDE.md
  §6.6: Downed characters can't use abilities) -- a small, direct addition to M7/M8's existing RPCs
  rather than a new generic gating layer, since only two abilities exist to gate in Build 1.
  **`GameConstants.h`** gained `ReviveDurationSeconds` (3.0), `ReviveRadiusUnits` (150.0),
  `ReviveHealthRestorePercent` (0.5, per the user's explicit choice this session).
  **Stopped here because `UCoopDownedComponent` is a brand-new `UCLASS`** -- per the M8 DECISIONS.md
  addendum's own narrower guidance, that specific case (not just new members on an existing class)
  is still unconfirmed-safe under Live Coding and defaults to a full external rebuild. Asked the user
  to close the editor and run a full rebuild, then reopen so `unreal-mcp` reconnects.
  **Next, once the editor is back up:** confirm the new symbols/class load cleanly
  (`search_subclasses` for `UCoopDownedComponent`, no reflection errors), wire
  `DownedComponent.GameConstants` on `BP_PlayerCharacter`'s CDO to `DA_GameConstants` (the exact
  same CDO-persistence step M6's log already found necessary for `HealthComponent.GameConstants` --
  do this proactively this time, don't wait to discover it missing via a warning log line), confirm
  the 3 new `GameConstants` fields read their C++ defaults live, then 5-client PIE: apply enough test
  damage via `ApplyTestDamage` to down one pawn, confirm `Status.Downed`/movement lockout/ability
  lockout, move another pawn's capsule into range (via `set_actor_transform`, no keypress needed --
  Revive is overlap-triggered) and confirm the revive channel completes and restores
  `ReviveHealthRestorePercent` of health, then repeat for all 5 simultaneously and confirm
  `IsPartyWiped()` flips true on the server (and check it clears correctly after `RequestSceneReset()`
  is called manually via reflection, since nothing calls it automatically until M10's scene exists).

- **M9 Verify completed -- resumed in a later session once the new `GameplayTestToolset` MCP plugin
  (see `build_toolset.md`) existed to remove the last human-in-the-loop dependency
  (`ApplyTestDamage` and ability keys previously needed a human to type/press them in a live PIE
  window).** Full detail, including a real design finding and a real methodology mistake caught
  mid-session, lives in `build_toolset.md`'s log -- summary here:
  Downed a pawn via `ExecCommand("ApplyTestDamage 150")` on its own client-local controller and
  confirmed `Status.Downed`, `HealthComponent.CurrentHealth=0`, `CharMoveComp.MovementMode=MOVE_None`
  (movement lockout), and `ACoopGameState.DownedPlayerCount=1`, all via reflection. Ability lockout
  during Downed was confirmed by code review of `CoopPlayerController.cpp`'s `Status_Downed` guard in
  both `Server_ActivateShield_Implementation`/`Server_ActivateStabilize_Implementation` (this run's
  down target wasn't Tank/Control, so a live isolated test wasn't possible this session -- the role
  gate would already reject it independent of Downed state).
  **Real finding: the revive channel completed on its own, undirected, within the default 3.0s
  `ReviveDurationSeconds`** -- CLAUDE.md §6.3's small locked prep arena puts all 5 players within
  `ReviveRadiusUnits` (150 units) of each other by default, so a still-standing teammate is already
  overlapping the revive-trigger sphere the instant it activates. Confirmed this cleanly (rather than
  racing past it) by temporarily widening `ReviveDurationSeconds` on the live `DA_GameConstants`,
  same established pattern as M7's Shield-duration fix, restoring it to 3.0 afterward.
  Confirmed `CompleteRevive` restores exactly `ReviveHealthRestorePercent` (0.5) of max health and
  decrements `DownedPlayerCount` back to 0 -- driven by the same organic proximity, not a scripted
  teleport.
  **Wipe confirmed:** downed all 5 players via `ExecCommand` (widened `ReviveDurationSeconds` to 90
  first to prevent any auto-revive mid-sequence), `DownedPlayerCount` read `5` on two separate reads
  a few seconds apart -- stable, confirming `IsPartyWiped()` would evaluate true, and confirming
  `Server_AttemptRevive`'s reviver-not-Downed guard genuinely holds a full wipe (no one left able to
  revive anyone). `RequestSceneReset()` itself was not exercised (nothing calls it until M10's scene
  exists, per this milestone's own scope -- unchanged from the prior session's plan).
  M9 fully verified, all self-driven, no human keypress or console typing anywhere this session.
  Next: M10 (Hold the Gate: Plates & Gate Logic).

- **M10 finished — BP wrappers created, level laid out, full Verify pass completed, all self-driven,
  no human keypress or console typing anywhere this session.** Picked up after the user's full
  external rebuild succeeded (`CoopPressurePlate.cpp`, `CoopGateActor.cpp`, `CoopHoldTheGateScene.cpp`
  plus M8/M9's `CoopControlAbilities.cpp`/`CoopDownedComponent.cpp`, ~8.7s, `Result: Succeeded`).
  **Confirmed the three new classes load with no reflection errors:** `search_subclasses` on
  `/Script/Engine.Actor` filtered to "Coop" listed `CoopPressurePlate`, `CoopGateActor`, and
  `CoopHoldTheGateScene` alongside all prior classes. **Confirmed the new constants on the live
  `DA_GameConstants` CDO:** `PlateRestoreWindowSeconds=5`, `PlateCount=4`, both matching the C++
  defaults with no manual `set_properties` needed.
  **Created and wired all three BP wrappers** under a new `/Game/Blueprints/Scenes/` folder
  (mirroring `Source/.../Scenes/`): `BP_PressurePlate` → `ACoopPressurePlate`, `BP_Gate` →
  `ACoopGateActor`, `BP_HoldTheGateScene` → `ACoopHoldTheGateScene` (`BlueprintTools.create` with
  `asset_type=/Script/Engine.Actor`, `set_parent`, `compile_blueprint` — same two-step pattern as
  every prior wrapper). Set `BP_HoldTheGateScene`'s CDO `gameConstants` → `DA_GameConstants`
  (`ObjectTools.set_properties`), recompiled, and re-verified via `get_properties` that the reference
  survived the compile — same CDO-persistence check every prior milestone's constants-holder needed.
  **Level layout, with a real placement mistake caught and corrected before it became a hidden
  problem:** the level (`/Game/ThirdPerson/Lvl_ThirdPerson`) has no single flat "floor" — it's the
  ThirdPerson template's default terrain plus a large flat paved plaza roughly `-2000..2000` in X/Y,
  decorated with big static-mesh ramps/cylinders in the four corners and at the cardinal edges.
  First placement attempt (4 plates + gate + scene actor, southwest of the existing Build-0 button)
  used `SceneTools.add_to_scene_from_asset` with `snap_to_ground=true` per-actor, but landed the 4
  plates at wildly inconsistent heights (Z 100–275) and the gate sitting on top of a decorative ramp
  mesh — caught via `CaptureViewport` (with actor-label annotations, decoded from base64 to a local
  PNG and viewed directly, since the raw payload is too large for a tool result) rather than trusting
  the placement blind. Diagnosed via `find_actors` with a bounds-box query cross-referenced against
  `get_actor_bounds`: the southwest quadrant had real static-mesh clutter (one actor with plausible
  bounds `1200-1500` in both X/Y, plus a background mesh with degenerate `±16384m` bounds that matches
  every bounds query and isn't a real obstacle). **Removed all 6 actors and re-placed them in a
  scouted, verified-clear northeast quadrant** (`find_actors` bounds-box query returned only the
  background mesh + `Floor`, no real clutter): 4 plates in a 300-unit-spaced 2×2 grid at
  `(750,750)/(1050,750)/(750,1050)/(1050,1050)`, `BP_HoldTheGateScene_Instance` at the center
  `(900,900)`, `BP_Gate_HoldTheGate` past the plates at `(900,1300)` as the exit barrier. All 4 plates
  snapped to an identical `Z=100` this time (genuinely flat pavement), gate at `Z=50` — re-confirmed
  visually via a second `CaptureViewport` capture showing a clean 2×2 plate grid with the gate beyond
  it and the nearest decorative ramp clearly outside the room's footprint. `save_assets([])` after the
  correction.
  **Verify, 5-client PIE (dev-mode dummy-filled — only one real PIE window launched this session, per
  a "Not enough login credentials" editor warning, but the GameMode's existing dev-mode roster-fill
  logic completed all 5 `CoopPlayerState`/`ACoopCharacter` pairs on the server world regardless, so
  this didn't block anything):** confirmed via log sweep that `ACoopHoldTheGateScene::BeginPlay` found
  exactly 4 plates (no "expected 4" mismatch warning) and every `ACoopGateActor` found its scene (no
  "no ACoopHoldTheGateScene found" warning). `bGateOpen` read `false` at rest.
  **Gate-opens-only-with-all-4 confirmed by teleporting real pawns onto real plates** (`set_actor_transform`
  on 4 of the 5 live `ACoopCharacter` pawns, one at a time, matching each plate's `(X,Y)` with a
  calibrated `Z` — measured the existing `PlayerStart`'s stand height above known ground level once,
  `92.01` units, and reused that capsule-to-ground offset here): 3/4 held → `bGateOpen` still `false`;
  4th pawn onto the last plate → `bGateOpen` flipped `true`, and the live `ACoopGateActor`'s Z dropped
  from `50` to `-350` (the real `ApplyGateVisual(true)` cosmetic response, not a synthetic read) —
  confirms the full plate→scene→gate chain, not just the scene's internal bool.
  **Real methodology mistake caught and corrected, not silently worked around:** the first "step off a
  plate" attempt moved a pawn straight up in Z only (same X/Y as the plate), and `bIsOccupied` stayed
  `true` — initially looked like a possible box-scale bug (hypothesized the trigger volume's box
  extent might be getting multiplied by the plate mesh's non-uniform `(2.5,2.5,0.2)` actor scale via
  component-attachment inheritance, which would make plates' trigger boxes overlap their neighbors).
  **Checked before acting on the hypothesis:** `ActorTools.get_actor_bounds` on a live plate returned
  exactly the unscaled `250×250×200` box (`625-875` in X/Y around a `750`-center plate) — matching the
  C++'s literal `SetBoxExtent(125,125,100)` values with no scale multiplication, so the box-scale
  hypothesis was wrong, not a real bug. **The actual cause: `CharacterMovementComponent`'s gravity
  pulled the pawn straight back down onto the same `(X,Y)` footprint it was teleported "up" from**, so
  it never left the plate's small (non-overlapping, correctly-sized) trigger footprint. Redid the test
  moving the pawn sideways to `(900,900)` — the verified-neutral room center, confirmed outside all 4
  plates' `625-1175`-range boxes by direct bounds arithmetic — and `bIsOccupied` correctly flipped
  `false` immediately.
  **Restore-window and close behavior confirmed with controlled, self-paced timing (no human
  round-trip needed, unlike M7/M8's Shield/Fortress duration races):** re-armed all 4 plates
  (`bGateOpen` back to `true`), stepped the SE pawn off to the neutral center — `bGateOpen` stayed
  `true` immediately after (restore window running, gate not yet closed). Waited a plain `Bash sleep 7`
  (past the real `5.0s` `PlateRestoreWindowSeconds`, no constant-widening hack needed since nothing
  here required a human to react within the window) — re-read `bGateOpen`: `false`, and the live
  `ACoopGateActor`'s Z was back at `50` (closed cosmetic response). **Also confirmed the
  restore-*succeeds* path, not just the fail path**, since the plan's wording ("restore-window/close
  behavior") covers both: re-armed again, stepped off, waited `2s` (well inside the window), stepped
  back onto the same plate — `bGateOpen` still `true` immediately, then waited a further `7s` (past
  the *original* 5s deadline) and confirmed `bGateOpen` was still `true` — proves
  `GetWorldTimerManager().ClearTimer(RestoreWindowTimerHandle)` genuinely cancels the pending close on
  restoration, not just a lucky race.
  **Server-authoritative confirmed via code review, consistent with every prior milestone's practice**
  (no multi-client replication tool exists to prove it any other way): `HandlePlateOccupancyChanged`,
  `ACoopPressurePlate::BeginPlay`'s overlap binding, and both `bIsOccupied`/`bGateOpen` are gated behind
  `HasAuthority()` checks or only ever written server-side per the source already read this session;
  every mutation observed above flowed through the real overlap-delegate/timer callbacks on the
  server-world actor (`UEDPIE_0`), never a direct `set_properties` poke on the replicated bools
  themselves.
  `StopPIE`, `save_assets([])`.
  **M10 fully verified and complete.** Next: M11 (Hold the Gate: Monster Spawner & Fixate/Retarget
  AI) — depends on M0 already being committed, which it is.

- **M11 C++ written; paused for a required full rebuild (three brand-new UCLASSes, same rule as
  M9).** Added `Source/Unreal_first_Game/Core/CoopFixateRetargetComponent.h/.cpp` — a small,
  genuinely reusable `UActorComponent` per the plan's own reasoning
  (`PickInitialTarget(Candidates)`/`OnTargetDowned()`/`GetCurrentTarget()`), no Hold-the-Gate
  reference anywhere in the file — matches DECISIONS.md's "Monster combat inside Hold the Gate"
  scope boundary (Gravity Bridge is expected to reuse this same targeting *behavior* later).
  Added `Source/Unreal_first_Game/Core/CoopMonsterCharacter.h/.cpp` — a basic monster: reuses M6's
  `UCoopHealthComponent` directly (that component's own M6-era code comment already anticipated
  this — "not every owner is a possessed player pawn... M11's monsters will use this same
  component with no PlayerState at all"), owns a `UCoopFixateRetargetComponent`, and a simple
  periodic direct-`ApplyDamage` "attack" against its current target — no physical projectile actor,
  no range check, documented in a comment as a simplification matching Shield's own "negates all,
  not just directional" precedent. No movement, no `AAIController`, no pathfinding anywhere, per
  DECISIONS.md's explicit scope limit. Retargeting binds to the current target's new
  `OnDownedStateChanged` delegate (added to `UCoopDownedComponent`, see below) and, on the target
  entering Downed, waits `MonsterFixateSwitchDelaySeconds` before actually re-picking — a readable
  "hesitate, then re-fixate" beat instead of an instant snap.
  Added `Source/Unreal_first_Game/Scenes/CoopMonsterSpawner.h/.cpp` — Hold-the-Gate-specific spawn
  choreography, stays local per the plan/DECISIONS.md. Gathers its fixate candidate pool as "every
  real `ACoopCharacter` whose role isn't Tank" (role-based, not per-plate-occupancy-based — matches
  docs/scenes/HOLD_THE_GATE.md's "Tank is the only mobile one" framing directly, and avoids needing
  to extend `ACoopPressurePlate` to track *which* actor occupies it, which nothing else needs).
  Re-schedules its own timer on every spawn (interval recomputed each time, not a fixed-rate loop)
  using `TSubclassOf<ACoopMonsterCharacter> MonsterClass` set on `BP_MonsterSpawner`'s CDO — same
  TSubclassOf-on-a-BP-wrapper pattern already proven by `AGameModeBase`'s own class-reference fields.
  **Two small supporting additions to already-existing classes** (not new UCLASSes themselves, but
  bundled into this same rebuild since one's already required):
  - `UCoopHealthComponent::SetMaxHealth(float)` — server-only override for owners whose max HP isn't
    the player-oriented `DefaultMaxHealth` (monsters use `GameConstants->MonsterHealth` instead);
    resets `CurrentHealth` to match, since it's only ever called once, immediately post-spawn.
  - `UCoopDownedComponent::OnDownedStateChanged` — a new `DECLARE_DYNAMIC_MULTICAST_DELEGATE`,
    broadcast from `SetDowned()` on every transition (same "fires on any change, listener decides"
    shape as `ACoopPressurePlate::OnOccupancyChanged`) — this is what `ACoopMonsterCharacter` binds
    to for its retarget trigger.
  **`GameConstants.h` gained 6 new fields**, 4 matching the plan's own list exactly
  (`MonsterHealth = 50.0`, `MonsterSpawnIntervalEarlySeconds = 6.0`,
  `MonsterSpawnIntervalLateSeconds = 2.5`, `MonsterFixateSwitchDelaySeconds = 0.5`) plus 2 not
  itemized in the plan but required to give the monster's "simple attack" build item any real
  numbers — `MonsterAttackDamage = 5.0`, `MonsterAttackIntervalSeconds = 2.0` — same "found it was
  needed, added it, logged it" precedent nearly every prior milestone has hit.
  **Documented, deliberate placeholder, flagged in code comments, not silently taken:** the
  spawn-interval escalation (`MonsterSpawnIntervalEarlySeconds` → `...LateSeconds`) ramps linearly
  over a hardcoded local `EscalationRampSeconds = 60.0f` inside `CoopMonsterSpawner.cpp`, not
  against total scene duration — `HoldTheGateSceneDurationSeconds` doesn't exist as a constant until
  M12 ("Escalation Tuning"), so this milestone can't cleanly interpolate against it yet. M12 is
  expected to replace this local placeholder once that constant lands.
  **Stopped here because `CoopFixateRetargetComponent`, `CoopMonsterCharacter`, and
  `CoopMonsterSpawner` are three brand-new `UCLASS`es** — per the M8 DECISIONS.md addendum's own
  narrower Live-Coding-safe guidance, a new class (not just new members on an existing one) still
  needs a full external rebuild, unconditionally, same as M9's `UCoopDownedComponent` hit. Asked the
  user to close the editor and run a full external build, then reopen so `unreal-mcp` reconnects.
  **Next, once the editor is back up:** confirm the three new classes load (`search_subclasses`, no
  reflection errors) and the 6 new `GameConstants` fields read their C++ defaults live (same as
  every prior milestone's constants), create `BP_MonsterCharacter`/`BP_MonsterSpawner` wrappers
  (reparent + wire each class's own `GameConstants` reference — both `ACoopMonsterCharacter` and
  `ACoopMonsterSpawner` hold their own private pointer, so both wrappers need their own CDO wiring
  step, matching the established per-class pattern), wire `BP_MonsterSpawner`'s `MonsterClass` →
  `BP_MonsterCharacter_C`, place chamber instances in the level near the 4 pressure plates (content
  wiring, MCP-buildable), then the M11 Verify pass: 5-client PIE, confirm a spawned monster's
  `GetTargetingComponent()->GetCurrentTarget()` resolves to a non-Tank real player, down that target
  via the established `ExecCommand("ApplyTestDamage 150")` self-targeted pattern (M9's
  `GameplayTestToolset`, no human keypress needed) and confirm the monster retargets to a different
  non-Tank player after `MonsterFixateSwitchDelaySeconds`, and confirm spawn timing is
  server-driven/consistent (reflection-read `SpawnTimerHandle`-driven spawns are server-authoritative
  by construction, same code-review-based server-authoritative argument as M10). Monster HP
  depletion/death is not expected to be organically testable this milestone (Damage role has no
  functional ability yet in Build 1 — nothing can deal real damage to a monster) — same deferral
  precedent as M6's 0-HP delegate, not a gap to chase down now.

- **M11 finished — resumed in a new session, found the full rebuild/wiring/placement already done by
  a prior unlogged pass, then completed the Verify pass and closed a real tooling gap along the way.**
  Session started cold (context compression), so began by re-establishing ground truth rather than
  trusting the tracker's last-written state: `git status` confirmed all M11 source files
  (`CoopFixateRetargetComponent`, `CoopMonsterCharacter`, `CoopMonsterSpawner`) present as untracked
  new files. Checked the live editor (already running, `UnrealEditor-Unreal_first_Game.dll` rebuilt at
  15:38, editor process started 15:40 -- i.e. the requested full external rebuild had already happened
  and the editor was already reopened) and found, contrary to the tracker's last checkpoint, that
  everything through "place chamber instances in the level" was **already done**: `search_subclasses`
  confirmed all three new classes load with no reflection errors; `DA_GameConstants` already read all
  6 new fields at their correct C++ defaults; `BP_MonsterCharacter`/`BP_MonsterSpawner` were already
  created, correctly reparented (`get_parent`), and correctly wired (`BP_MonsterCharacter`'s CDO
  `gameConstants` → `DA_GameConstants`; `BP_MonsterSpawner`'s CDO `gameConstants` →
  `DA_GameConstants` and `monsterClass` → `BP_MonsterCharacter_C`); 4 `BP_MonsterSpawner` instances
  were already placed in the level, one per cardinal side of the 2×2 plate grid
  ((900,1200)/(900,600)/(600,900)/(1200,900) around the room center (900,900) established in M10) --
  confirmed via `get_actor_transform`, no correction needed, unlike M10's placement mistake. Checked
  off the two remaining unchecked boxes (`BP_MonsterCharacter`/`BP_MonsterSpawner` wrappers, and the
  constants line) accordingly -- this work was real and correct, just never logged.
  **Real tooling gap found attempting the Verify pass's literal ask:** `CoopFixateRetargetComponent`'s
  `CurrentTarget` (a plain `UPROPERTY() TWeakObjectPtr<AActor>`, no Edit/BlueprintVisible specifiers)
  could not be read via `ObjectTools.get_properties` or even enumerated via `list_properties` --
  confirmed this wasn't a fluke by cross-checking `list_properties` on the same component, which
  omitted `CurrentTarget`/`KnownCandidates` entirely from its output. This is a different failure mode
  from earlier tooling gaps (M3/M7's "no function-call capability") -- this is a genuine property-read
  limitation specific to un-annotated `TWeakObjectPtr` fields, confirmed not fixable via the
  `ProgrammaticToolset` either (it only orchestrates the same registered tools via sandboxed Python,
  no raw `unreal` module access, so it can't call the `GetCurrentTarget()` `BlueprintPure` accessor
  directly).
  **Fixed by adding two `UE_LOG` lines, not by inventing a new debug-only accessor:** added logging to
  `UCoopFixateRetargetComponent::PickInitialTarget`/`OnTargetDowned` (function-body-only changes, no
  new `UPROPERTY`/`UFUNCTION`/`UCLASS`) naming the owning monster and the target actor. Justified as a
  real, permanent fix rather than a throwaway test hook: CLAUDE.md §4.3 requires state to be
  "printable," and this subsystem had no way to observe its own core state at all before this --
  worth keeping past this session, not reverting. `LiveCodingToolset.CompileLiveCoding` came back
  clean (`Live coding succeeded`, function-body-only change, matches the established
  Live-Coding-safe category from DECISIONS.md's M8 addendum) -- no full rebuild needed.
  **Verify pass, run twice (once before the logging fix, once after) -- both runs on a genuine 5-client
  PIE session, not the dev-mode dummy-filled kind used in most prior milestones:** discovered mid-session
  that `FillEmptySlotsWithDummies` never fired (`bDevMode` false this session, confirmed by its absence
  from the log) yet the roster still completed -- meaning all 5 `ACoopPlayerState`s belonged to real
  network clients, each in genuinely separate `UEDPIE_0`..`UEDPIE_4` world contexts, not 1 real
  window + 4 `ADummyAIController`-possessed dummies. Confirmed this the hard way: `GameplayTestToolset`
  rejected `ApplyTestDamage` against a server-world (`UEDPIE_0`) replica of a remote client's controller
  ("Controller has no LocalPlayer -- pass that client's own client-local PlayerController"), then
  successfully targeted `UEDPIE_1`'s own local controller -- confirming 4 additional real client worlds
  actually exist despite the "Not enough login credentials" warning in the log (that warning is a red
  herring for this project's offline/no-OSS setup, not an actual cap on client count). Cross-referenced
  each `UEDPIE_N`'s local `PlayerState.PlayerId` against the server world's per-role `PlayerId`s (same
  established method from M6/M7) to find which real window corresponded to Control.
  **First run (12 monsters spawned across two spawn waves) confirmed the fixate filter cleanly:** all
  12 `PickInitialTarget` log lines named a target -- every single one landed on the run's Runner/
  Control/Support/Damage pawns, **zero** ever landed on the run's Tank pawn (`BP_PlayerCharacter_C_0`
  that run). Confirmed via a full-session `PlayerCharacter_C_0` log grep after the second run too --
  zero matches across the *entire* session's fixate and retarget lines, meaning Tank was never once
  targeted by any of the many dozens of spawn/retarget events across both runs.
  **Second run (after the logging fix) additionally confirmed retargeting under real organic load, not
  a single synthetic poke:** rather than needing a manual down, one non-Tank player
  (`BP_PlayerCharacter_C_4`, Support that run) was already being repeatedly downed by the natural
  8-12-monster barrage before any test command was issued -- the log showed **30+ independent
  `OnTargetDowned` retarget events over the session's ~2 minutes**, every one reading
  "retargeted from BP_PlayerCharacter_C_4 to BP_PlayerCharacter_C_{1,2,3}" -- confirming the
  downed-delegate retarget path fires correctly and repeatedly under real chaotic multi-monster load,
  and that every retarget's new pick still obeys the non-Tank filter. Also issued the plan's literal
  `ApplyTestDamage 150` against the real Control client (`UEDPIE_3`, cross-referenced via `PlayerId`)
  to exercise the manual path too -- confirmed `0.0/100.0` in the log, though Control had likely
  already been downed by organic fire moments earlier (no fresh 0-HP-transition retarget followed),
  so the organic evidence above is the stronger proof here, not a substitute needed for it.
  **Spawn timing confirmed server-driven** by code review, same basis as M10: `ACoopMonsterSpawner::
  BeginPlay`/`SpawnMonster`/`ScheduleNextSpawn` are timer-driven off `GetWorldTimerManager()` on the
  spawner actor itself, gated behind the spawner's own `HasAuthority()` check before the roster-poll
  timer ever starts -- no client-side path exists to trigger a spawn.
  Final log sweep across the whole session: no errors beyond pre-existing benign engine noise (missing
  profiler DLLs, the engine's own deliberate `LogTemp: Error test:` lines, an unrelated
  `GameFeatureData` asset-manager warning) -- nothing from this milestone's own code. `StopPIE`.
  **M11 fully verified and complete.** Monster HP depletion/death remains untested (Damage has no
  functional ability yet in Build 1) -- same deferral as before, not new. Next: M12 (Hold the Gate:
  Escalation Tuning, Win/Lose Integration, Full Playtest).

- **M12 -- resumed in a new session, found the full C++ side already written (by a prior session,
  uncommitted, already compiled into the running editor's binary), then completed the Verify pass
  and found/fixed a real, pre-existing content bug along the way.** Session started cold; `git
  status`/`git log` confirmed the working tree carries M7-M12 as one large uncommitted block (only
  M0-M6 are on any commit) -- consistent with this project's established practice of not committing
  until the user asks. Read `CoopHoldTheGateScene.h/.cpp` (win/lose state machine, `ResetScene()`,
  `CompleteScene()`), `CoopMonsterSpawner.h/.cpp` (`ResetSpawner()`, the escalation ramp now using
  the real `HoldTheGateSceneDurationSeconds` instead of M11's hardcoded 60s placeholder), and
  `CoopGameState.h/.cpp` (`CompleteMatch()`, `IncrementDownedCount()`'s wipe->`RequestSceneReset()`
  wiring) in full -- all matched the plan's M12 scope exactly, no gaps found on inspection.
  Confirmed via file timestamps that a full external rebuild had *already* happened after this code
  was last touched (`UnrealEditor-Unreal_first_Game.dll` at 17:07:52, all M12 source files last
  modified 16:28-16:30) -- no rebuild needed this session. `search_subclasses` confirmed no new
  UCLASSes here (`CompleteMatch`/`ResetSpawner` are new `UFUNCTION`s on already-existing classes),
  matching DECISIONS.md's Live-Coding-safe category, consistent with the rebuild having already
  landed cleanly. `get_properties` on the live `DA_GameConstants` CDO confirmed
  `HoldTheGateSceneDurationSeconds=90` at its correct C++ default, no manual `set_properties` needed.
  **Real, pre-existing bug found before any timing test could be trusted: 8 `ACoopPressurePlate`
  actors in the level, not 4.** First 5-client PIE session logged
  `ACoopHoldTheGateScene::BeginPlay: found 8 ACoopPressurePlate actor(s) in the level, expected 4` --
  a warning M10's own log claimed was resolved ("Removed all 6 actors and re-placed them in a
  scouted, verified-clear northeast quadrant"). Investigated rather than assumed stale: `find_actors`
  on both the PIE world and the base (non-PIE) `/Game/ThirdPerson/Lvl_ThirdPerson` level showed the
  same 8 plates in both, each labeled in two complete `SE`/`SW`/`NE`/`NW` sets -- one at the correct
  M10 coordinates (`750-1050` range), one at M10's *original, supposedly-removed* southwest-quadrant
  coordinates (`-1200`/`-1600`, `850`/`1150`). Since `AreAllPlatesOccupied()` requires *every*
  discovered plate occupied, this stray leftover set would have made the gate mathematically
  impossible to open with only 4 real players -- silently invalidating M10's own "verified" claim and
  blocking M12's win condition from ever firing in a real game, not just in this test. **Root-caused,
  not just patched around:** the stray 4 had no corresponding on-disk `__ExternalActors__` package
  (confirmed -- `save_actor` on one of the *kept* plates errored "Asset does not exist" for an
  unrelated external-actor path, and `git status` showed the same 14 untracked external-actor folders
  before and after removal, zero new/changed) -- these were pure in-memory leftovers in this
  long-running editor session (open across many prior sessions per the DLL-timestamp pattern already
  established in M5/M7/M8's logs) that M10's `remove_from_scene` call evidently never got a chance to
  actually persist. Removed the 4 stray actors (`remove_from_scene`, one at a time -- a first parallel
  attempt got two calls blocked by the permission classifier, corrected to sequential per the
  unreal-mcp skill's own "sequential, never parallel" rule), confirmed exactly 4 plates remain at the
  correct coordinates, `save_assets([])`. Re-ran PIE: no plate-count warning this time.
  **Verify, several fresh 5-client PIE sessions (temporarily shrinking `RoleSelectDurationSeconds`/
  `PrepArenaDurationSeconds`/`HoldTheGateSceneDurationSeconds` on the live `DA_GameConstants` for fast
  iteration, same established widen/shrink-then-restore pattern as M7-M9; restored to 30/60/90 and
  confirmed via `get_properties` before finishing):**
  - **Win path confirmed live, twice, independently.** Teleported 4 pawns onto the 4 plates via
    `set_actor_transform` (same method M10 established) -- two of five pawns needed 2-3 retries to
    stick per press (see the new methodology note below), all 4 eventually held simultaneously,
    `bGateOpen` flipped `true`. Left the hold in place; the shortened scene-duration timer elapsed
    while still held, and the log showed `ACoopHoldTheGateScene::CompleteScene: Hold the Gate
    complete -- the party held the gate for the full scene duration.` with `CurrentPhase` correctly
    reading `Complete` afterward. A second, independent win happened later the same session (see
    below) with an identical log line and phase transition -- not manufactured, a second genuine
    occurrence caught by continuing to check state rather than stopping at the first result.
  - **Duration-expiry-without-win -> reset confirmed live.** Fresh PIE session, deliberately did not
    occupy any plate, confirmed `CurrentPhase` stayed `HoldTheGate` (never falsely flipped `Complete`)
    through and past the shortened duration. `CompleteScene` has no log line for this branch (only
    fires on win), so used an indirect but unambiguous signal instead: every live
    `ACoopMonsterCharacter`'s `refPath` UObject index was recorded before and after the expiry point,
    and the entire population was replaced with a fresh, non-overlapping ID set (e.g. `256-259` ->
    `268-275`, matching 2 further spawn waves' worth of monsters) -- exactly `ResetScene()`'s
    documented behaviour ("destroy every live monster... restart each spawner's escalation ramp"),
    and inconsistent with any other code path in this project. `bGateOpen` stayed `false` throughout,
    as expected with nobody on a plate.
  - **Restore-window-expiry -> reset confirmed live -- the actual *new* M12 wiring, not just a
    repeat of M10's already-proven gate-closes behaviour.** Re-occupied all 4 plates (`bGateOpen`
    true), then broke one plate's occupancy and let `PlateRestoreWindowSeconds` (real 5.0s value,
    unchanged) expire unattended. `bGateOpen` flipped `false` (M10's already-proven half) *and* the
    same monster-population-replacement signal as above fired again (a third distinct ID set,
    `548-559`), confirming `OnRestoreWindowExpired`'s new `CoopGameState->RequestSceneReset()` call
    -- absent before M12 -- now actually reaches `ACoopHoldTheGateScene::ResetScene()` end to end via
    the `OnSceneResetRequested` broadcast, not just closing the gate bool as M10 alone would have.
  - **Genuine methodology finding, not a game bug:** a "step off the plate" attempt to a point only
    ~25 units past a plate's box edge (`(900,900)`, M10's own established "neutral center") initially
    read as still-occupied and looked like a fresh regression. Checked before concluding that:
    `get_actor_bounds` confirmed the target plate's real box is `625-875` in X/Y, and the Mannequin
    capsule's ~34-unit radius genuinely still overlaps a box edge only 25 units away -- not a bug,
    just insufficient clearance in this specific test point. Moving further out (`(400,400)`)
    resolved it immediately. Worth remembering for any future plate/trigger-adjacent reflection test
    in this project: "outside the box's raw AABB" is not the same as "outside the capsule's actual
    collision footprint" -- leave at least a capsule-radius margin, not just past the drawn edge.
  - **Real, general tooling/environment discovery, not specific to this milestone -- worth
    remembering for any future timing-sensitive verification in this project:** partway through this
    session, a shortened (16s) scene-duration timer failed to fire after 180+ real seconds of
    waiting, and zero monsters had spawned despite an early interval of 6s -- looked at first like a
    broken timer. Root-caused instead of worked around: `Editor > General > Performance >
    bThrottleCPUWhenNotForeground` was `true` (the editor's own default), which throttles *all* world
    ticking (PIE included) whenever the Editor application itself isn't the OS-focused window --
    exactly the state this whole session was in, since the terminal/chat window had focus throughout.
    Confirmed the fix worked before relying on it: flipped it off via
    `ConfigSettingsToolset.SetSectionProperties` (`Editor`/`General`/`EditorPerformanceSettings`),
    and monster spawns + phase timers immediately began advancing at a normal, roughly-1:1-with-wall-
    time rate. Restored it back to `true` afterward (an unrequested personal editor preference, not a
    project file, so left as found rather than silently kept changed) -- flagged to the user as a
    real option worth toggling off deliberately for any future solo agentic PIE testing session in
    this project, since 180+ real seconds for a 16-second in-game timer is a large, easy-to-misdiagnose
    tax otherwise.
  - **Also confirmed, as a byproduct of the above:** a scene win (`CompleteScene`, `CurrentPhase ==
    Complete`) does not stop the pressure-plate/gate/restore-window subsystem from continuing to run
    underneath -- a reset triggered *after* a win (as happened once in this session, purely from
    continuing to manipulate plates post-win while investigating the throttling issue) still clears
    monsters/respawns players/restarts spawners correctly, but leaves `CurrentPhase` stuck at
    `Complete` rather than reverting to `HoldTheGate`. `CompleteScene()`'s own existing code comment
    already flags "not stopping the spawners... on a win" as deliberate for Build 1 (no downstream
    Complete-phase content exists yet to break), so this is a natural, already-acknowledged
    consequence of that existing decision, not a new gap -- logged here as an observed behaviour, not
    changed, since nothing in Build 1 depends on Complete-phase plate state.
  Final log sweep: no errors beyond the known-benign engine noise every prior milestone's sessions
  have also shown. `StopPIE`, `IsPIERunning` confirmed `false`. `DA_GameConstants` confirmed restored
  to `RoleSelectDurationSeconds=30`/`PrepArenaDurationSeconds=60`/`HoldTheGateSceneDurationSeconds=90`,
  `save_assets([])`.
  **M12 fully verified: win path, both reset paths, and the specific new-this-milestone
  restore-window-to-scene-reset wiring are all confirmed live and correct**, plus a real pre-existing
  content bug (the M10 stray-plate leftover) was caught and fixed before it could silently block a
  future session's playtest. Checked off all three of M12's boxes -- the plan's own suggested
  human/solo pacing playtest (Shield-alone-sufficient -> insufficient curve) stays open per that
  document's explicit recommendation, same non-blocking treatment as M7/M8's ability-feel playtests.
  Next: M13 (Mini-Boss: Repeat -> Combine -> Rotate Fortress).

- **Parked M5 `WBP_AbilityCard` width bug fixed and definitively verified, triggered by the user
  asking to work through parked items using newly-available `unreal-mcp` tooling.** Explicitly did
  *not* touch M14 (the exit-criteria playtest) -- that's parked because it measures something only
  real humans produce (Discord talk time, CLAUDE.md §9), not because of a tooling gap, and no amount
  of new tooling changes that; declined to attempt it and explained why rather than silently
  reinterpreting the ask.
  **Real new capability found first, not assumed:** `list_toolsets` now shows `UMGToolSet` (read/
  write a widget's `WidgetTree` -- create, move, rename, wrap, bind events) and
  `SlateInspectorToolset` (live Slate snapshot/screenshot/click/type on the actual running Editor
  UI, including in-PIE UMG). Both are exactly the tooling gap M5's log named explicitly ("no tool to
  populate a widget's `WidgetTree`", "no input-injection tool exists"). `Plugins/` itself only holds
  the already-known `GameplayTestToolset` -- these two are new MCP-server-side toolsets, not a new
  UE plugin in the project.
  **Root cause was not what the parked note guessed, and inspecting the real tree rather than
  trusting the old screenshot-based diagnosis caught this before a wrong fix was applied:**
  `UMGToolSet.GetWidgets` on `WBP_AbilityCard` showed `Border_93 -> VerticalBox_0 ->
  {TextBlock_0 (name), TextBlock_1 (description)}` -- no `Size Box` anywhere in the tree, so the
  parked note's guessed fix ("widen the description Text Block's Size Box Width Override") could
  never have worked; there was nothing to widen. Instead, `WBP_PrepArenaHUD`'s own `CanvasPanelSlot`
  for each of the 4 `WBP_AbilityCard` instances was the actual culprit: `left/top/right/bottom`
  offsets under a single-point anchor (`min==max==(0,0)`) mean `right`/`bottom` are literally
  width/height, and all four were `100x40` -- comically undersized for a name label plus a wrapped
  multi-line description, and consistent with *both* previously-observed symptoms (unwrapped text
  overflowing into neighbouring cards before `autoWrapText` was enabled; wrapping at ~1-2 characters
  per line after, since `autoWrapText` computes its wrap width from the tiny allocated slot, not
  free-form). Widened all four card slots to `220x150` (`set_properties` on each
  `CanvasPanelSlot_2..5`'s `layoutData.offsets`, keeping `left`/`top` unchanged) -- checked the real
  gap between adjacent cards first (244-268px, read from each slot's `left`) to confirm 220 wouldn't
  reintroduce overlap. Also widened `WBP_TeamSynergiesPanel`'s own slot (`CanvasPanelSlot_6`, same
  100x40 default) to `400x60`, since it was the same class of bug on the hint-text panel the parked
  note's own "worth checking during the same pass" note flagged as unconfirmed.
  **A second, independent bug was found only because the widened cards could finally be read at
  all:** with the container now correctly sized, a screenshot showed each card as a blank white box
  with the description text spilling out *below* it -- `TextBlock_0`/`TextBlock_1` both read
  `colorAndOpacity = white (1,1,1,1)` and `Border_93.background` was the engine's unstyled default
  (`drawAs=Image`, `resourceObject=None`, `tintColor=white`) -- i.e. genuinely invisible white-on-
  white ability-name text, on a card that was never actually styled. This was unreachable by any
  previous method in this project (no way to read a widget's actual bound color/brush values before
  `UMGToolSet`/`ObjectTools` could target WidgetTree members). Fixed by darkening
  `Border_93.background.tintColor` to a near-black translucent `(0.05, 0.05, 0.05, 0.85)`, leaving
  the existing white text as-is -- standard dark-card-white-text HUD styling, one property changed,
  matches CLAUDE.md §5's "everything readable, nothing pretty."
  **Verified with two independent methods neither of which existed at the start of Build 1, not just
  a re-run of the old low-res desktop-screenshot method:**
  1. `SlateInspectorToolset.Screenshot` on the live PIE preview window, at the window's *native*
     resolution (646x520 per client, not a shrunk 1280x397 5-window composite) -- before/after
     comparison shows the fix directly: pre-fix, blank white boxes with description text spilling
     below in fragmented lines; post-fix, clean dark cards with the name label and a properly
     multi-line-wrapped description fully contained inside each card.
  2. `SlateInspectorToolset.Snapshot` (a full accessibility-tree text dump, not pixels) across *all
     five* live client windows simultaneously confirmed the exact rendered string for every card:
     Damage ("Execution"/"...physically vulnerable.", "Overload"/"...magically vulnerable."), Runner
     ("Dash"/"Carry"/"Chain" with their full descriptions), Tank ("Shield"/"Armor Break"), Control
     ("Stabilize"/"Mind Fracture"/"Channel") -- every string matches `docs/abilities.md` and
     `CoopAbilityCardWidget.cpp`'s hardcoded table exactly, and every `WBP_TeamSynergiesPanel`
     instance across all 5 windows shows the complete, untruncated sentence "Tank and Control share
     a bond. Talk to each other." -- proving the earlier screenshot's apparent mid-sentence cutoff
     was purely the small PIE preview window's own pixel width cropping the capture, not a real
     wrap/truncation bug (the accessibility tree has no such limit and shows the full string).
  Compiled both widget blueprints (`CompileWidgetBlueprint`, clean), `StopPIE`/`save_assets` before
  editing (per the unreal-mcp skill's "Mind PIE" rule -- blueprint CDO edits don't hot-reload into
  already-spawned PIE widget instances), then a fresh `StartPIE` to confirm the compiled changes
  live. `DA_GameConstants` temporarily shrunk `RoleSelectDurationSeconds` for fast iteration,
  restored to 30/60/90 and confirmed via `get_properties` afterward, `save_assets([])`.
  **Real methodology note for future MCP-driven work, not a game bug:** two `mcp__unreal-mcp__call_tool`
  calls issued together in one message got one blocked by the permission classifier while the other
  succeeded, during the earlier M12 pass -- confirms the unreal-mcp skill's own "sequential, never
  parallel" rule isn't just about avoiding game-thread deadlocks, it also avoids partial-completion
  states that are easy to miss if both calls are assumed to have the same outcome.
  Checked off M5's remaining Verify box (both the layout bug and the underlying data/wiring were
  already independently confirmed correct in the original M5 session). M5 is now **fully complete**,
  no open items. Still explicitly not exercised this session, since it wasn't what was asked and
  isn't itself a parked item: `SlateInspectorToolset.Click`-driven testing of actual `WBP_RoleSelect`
  button presses and the two-clients-claim-the-same-role UI race -- both are now genuinely possible
  for the first time in this project (the exact "no input-injection tool" gap M5's log named), worth
  picking up explicitly in a future session if that specific verification is wanted.
  **Real, serious tooling problem found and resolved before declaring this done -- the fix was
  correct in the live editor the whole time, but had never actually reached disk.** Routine
  double-checking (`AssetTools.is_dirty` after `save_assets`) found every save this session had
  silently failed: `is_dirty` kept reading `true` after `save_assets([])` reported `true`, and
  `.uasset` mtimes on disk hadn't moved in hours -- this affected `WBP_AbilityCard`,
  `WBP_PrepArenaHUD`, and even `DA_GameConstants`'s M12-session restoration. Calling `save_assets`
  with an explicit path (not the empty-list form) surfaced the real error in the Output Log:
  `Error Code 32` (Windows sharing violation) repeatedly failing to move `DA_GameConstants.uasset`
  into place. Root-caused, not worked around: three `UnrealEditor.exe` processes were running --
  the real editor plus what looked like an orphaned duplicate instance of this same project (same
  window title, spawned minutes later, far lower resource usage) left behind by an earlier
  `StartPIE`/`StopPIE` cycle this session. Flagged to the user rather than guessing and killing an
  unfamiliar process myself; user confirmed and closed the stray instance(s). Saves succeeded
  immediately afterward, confirmed three ways: `is_dirty` false, `.uasset` mtimes fresh, and
  `git status`/`git diff` showing the real changes (`WBP_AbilityCard.uasset`/
  `WBP_PrepArenaHUD.uasset` now genuinely modified; `DA_GameConstants.uasset` shows **no** diff,
  confirming its restored values round-tripped back to exactly the already-committed production
  defaults). Logged as a new DECISIONS.md entry ("`save_assets` can silently fail to reach disk")
  since this is a durable, project-wide risk any future multi-`StartPIE` session could hit again,
  not a one-off. Re-confirmed the widget fix survived the save/reload cycle via one final
  `get_properties` read on the live `Border_93` background color -- unchanged, as expected.
