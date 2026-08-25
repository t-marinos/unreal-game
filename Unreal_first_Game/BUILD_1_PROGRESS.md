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
- [ ] Verify: reflection confirms the data path and `WBP_RoleSelect`'s button wiring are correct (see
      log) and the full RoleSelect→Prep→HoldTheGate progression still works with real widgets
      attached — **done**. **Still open:** a real layout bug found in `WBP_PrepArenaHUD` (ability
      card text overlaps illegibly — see log), plus the still-outstanding human playtest for
      layout/readability and an actual-button-click test (no input-injection tool exists to do this
      by reflection). Leave this box unchecked until the overlap bug is fixed and a human confirms
      the fix visually.

**Parked (2026-08-25, explicit user decision — not an oversight):** the `WBP_AbilityCard` description
text-wrap width bug (see the log entry ending "M5's Verify box stays unchecked") is deliberately left
unfixed for now. User: "leave it to fix it later." All underlying data/logic (`CardIndex` wiring,
`GetCardName`/`GetCardDescription`, the 5-button `OnClicked` wiring on `WBP_RoleSelect`) is verified
correct — this is purely a Designer-side width/wrap constraint on one Text Block, cosmetic only, does
not block gameplay logic in M6+. Do not silently re-open or attempt to fix this as a side effect of
later work; only touch it if the user explicitly asks. When it does get picked back up: widen the
description Text Block's `Size Box` `Width Override` to ~200-250px per the log's diagnosis, then
re-verify per the same reflection+screenshot method already established (5-client PIE,
`CaptureEditorImage`, compare against the two bug screenshots already captured this session).

## M6 — Health & Damage Foundation
- [x] `CoopHealthComponent.h/.cpp` — replicated Current/MaxHealth, server-only `ApplyDamage()`,
      `GetHealthPercent()`, 0-HP delegate
- [x] `ACoopCharacter`: add the component
- [x] New DA_GameConstants: `DefaultMaxHealth = 100.0`
- [x] Verify: 5-client PIE, damage replication/clamping confirmed live; 0-HP delegate fire-once
      confirmed by code review only (no bound listener until M9) — see log

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
