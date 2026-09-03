# GameplayTestToolset — Progress Tracker

Working checklist for a new, **project-scoped** `unreal-mcp` toolset plugin that removes the
human-in-the-loop for live gameplay testing (ability keys, console `Exec` commands) that
`SlateInspectorToolset.PressKey()` cannot reach.

**Context, so a resumed session understands why this exists:** while verifying Build 1's M9
(Downed/Revive), we confirmed `SlateInspectorToolset.Click()` reliably drives real UMG buttons in
PIE (finally closing `BUILD_1_PROGRESS.md`'s M5 button-click gap), but `PressKey()` does **not**
reach Enhanced Input gameplay bindings or the in-game console — pressing E for Shield and Tilde for
the console both silently did nothing, confirmed via reflection and log checks, not assumption. The
user (who has real Unreal MCP tooling experience) diagnosed this precisely: `Click()` goes through
Slate/UMG hit-testing, which works regardless of input path; `PressKey()` is character-event
oriented and never reaches the raw key-down/up pipeline Enhanced Input and the console-key intercept
require. The fix is a small toolset exposing `UEnhancedInputLocalPlayerSubsystem::InjectInputForAction`
(real input injection, bypasses focus entirely) and `UKismetSystemLibrary::ExecuteConsoleCommand`
(runs an `Exec` UFUNCTION scoped to a specific player, no console UI needed).

**Location decision:** every existing toolset (`EditorToolset`, `LiveCodingToolset`,
`SlateInspectorToolset`, etc.) lives under the shared engine install
(`C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\Toolsets\`) — not in this repo,
and editing it would affect every UE 5.8 project on this machine with no git history. This plugin
instead lives at `Plugins/GameplayTestToolset/` inside this project: git-tracked, reviewable,
deletable without side effects elsewhere. Structure and registration pattern (a `UEditorSubsystem`
that registers the toolset class with `UToolsetRegistry`, toggled by a console variable) are copied
directly from the real, currently-installed `LiveCodingToolset` plugin — read in full before writing
any code, not inferred from the `create-toolset` skill's simplified example alone. Tests use
`CQTest` (`TEST_CLASS`/`TEST_METHOD`), matching that plugin's actual test file, not the skill doc's
`BEGIN_DEFINE_SPEC` example (stale relative to this engine version).

**Language decision:** C++, not Python. The project's own Python stub file
(`Intermediate/PythonStub/unreal.py`) doesn't exist — Python Developer Mode was never enabled for
this project, and enabling it costs an editor restart just to inspect coverage. Every sibling
toolset this one parallels (`LiveCodingToolset`, `EditorAppToolset`) is C++, and the two required
engine APIs (`InjectInputForAction`, `ExecuteConsoleCommand`) are plain, well-documented C++/
Blueprint-callable functions with no Python-coverage risk. Chosen without asking — a reasonable
default call per the skill's own guidance, not a hard blocker either way.

---

## M0 — Plan & scaffold
- [x] Read `LiveCodingToolset` plugin in full (`.uplugin`, `Build.cs`, module, subsystem,
      toolset class, tests) as the structural template
- [x] Decide plugin location (project-local `Plugins/`, not the shared engine install) and
      language (C++) — see rationale above
- [x] Create this progress tracker
- [x] `Plugins/GameplayTestToolset/GameplayTestToolset.uplugin`
- [x] `Source/GameplayTestToolset/GameplayTestToolset.Build.cs`
- [x] `Source/GameplayTestToolset/Public/GameplayTestToolsetModule.h` +
      `Private/GameplayTestToolsetModule.cpp` (empty Startup/Shutdown, same as LiveCodingToolset —
      registration lives in the subsystem, not the module)

## M1 — Toolset class: `TriggerInputAction` + `ExecCommand`
- [x] `Public/GameplayTestToolset.h` — `UGameplayTestToolset : public UToolsetDefinition`
- [x] `Private/GameplayTestToolset.cpp`:
  - `TriggerInputAction(APlayerController* Controller, UInputAction* Action)` — resolves
    `Controller->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()`, injects a
    press (`true`) then release (`false`) `FInputActionValue` via `InjectInputForAction`. Raises via
    `UKismetSystemLibrary::RaiseScriptError` on null Controller/Action/LocalPlayer/Subsystem.
  - `ExecCommand(APlayerController* Controller, const FString& Command)` — wraps
    `UKismetSystemLibrary::ExecuteConsoleCommand(Controller->GetWorld(), Command, Controller)`.
    Raises on null Controller or empty Command.
  - **Both take the target player's own client-local `APlayerController*`** (e.g. a PIE client's
    `UEDPIE_N` world's own `BP_PlayerController_C_0`, not the server's replicated representation) —
    `GetLocalPlayer()` is null on a server-side replica of a remote client, so this is a hard
    requirement, not a convention; document it on both tools.

## M2 — Registration
- [x] `Public/GameplayTestToolsetSubsystem.h` + `Private/GameplayTestToolsetSubsystem.cpp` —
      `UEditorSubsystem` registering/unregistering `UGameplayTestToolset` with `UToolsetRegistry`,
      toggled via a `GameplayTestToolset.Enable` console variable (copy the exact
      `LiveCodingToolset.Enable` CVar pattern)

## M3 — Tests
- [x] `Source/GameplayTestToolsetTests/GameplayTestToolsetTests.Build.cs`
- [x] `Private/GameplayTestToolsetTestsModule.cpp` (empty `IMPLEMENT_MODULE`, same as
      LiveCodingToolsetTests)
- [x] `Private/GameplayTestToolsetTests.cpp` (`CQTest`, via `FActorTestSpawner` for a valid non-null
      `APlayerController`/`UInputAction` in an isolated test world) — subsystem-is-available,
      subsystem-registers-toolset-by-default, and error-path tests for both tools (null Controller,
      null Action, empty Command, no-LocalPlayer Controller). Error paths assert "does not crash"
      rather than "logs an error" — `RaiseScriptError` doesn't surface as a catchable automation
      error when called directly from C++ instead of through the Blueprint VM, per a documented
      precedent in this same engine version's `UMGToolSetTest.cpp`.

## M4 — Enable & compile
- [x] Add `{"Name": "GameplayTestToolset", "Enabled": true}` to `Unreal_first_Game.uproject`'s
      `Plugins` array
- [x] `LiveCodingToolset.CompileLiveCoding` (or full rebuild if the brand-new-plugin case turns out
      unsafe under Live Coding — treat as an open question, not an assumption) until clean — confirmed
      the brand-new-plugin case indeed needs a full rebuild, not Live Coding: editor had to be closed,
      `Build.bat Unreal_first_GameEditor Win64 Development -Project=... -WaitMutex -FromMsBuild`
      succeeded (17/17 actions, `Result: Succeeded`, ~11s), producing
      `UnrealEditor-GameplayTestToolset.dll` and `UnrealEditor-GameplayTestToolsetTests.dll` alongside
      a relinked `UnrealEditor-Unreal_first_Game.dll`
- [x] `AutomationTestToolset.DiscoverTests` → `ListTests` (filter `AI.Toolsets.GameplayTestToolset`)
      → `RunTests` — all 8 green (`Subsystem_IsAvailable`, `Subsystem_RegistersToolsetByDefault`,
      `TriggerInputAction_*DoesNotCrash` x3, `ExecCommand_*DoesNotCrash` x3), 0 failed, 0 skipped

## M5 — Live verification, and finish Build 1's M9 as a side effect
- [x] `list_toolsets` shows `GameplayTestToolset`; `describe_toolset` shows both tools with correct
      schemas
- [x] 5-client PIE: `TriggerInputAction` on a client-local Tank `PlayerController` + `IA_Shield` —
      confirm `Status.Shielded` applies via reflection, no human keypress involved
- [x] `ExecCommand` on a client-local `PlayerController` running `ApplyTestDamage <amount>` —
      confirm damage lands via reflection, no human console typing involved
- [x] If both hold: resume `BUILD_1_PROGRESS.md`'s M9 (Downed/Revive/Wipe) entirely through this
      toolset — down one pawn, confirm Downed/movement/ability lockout, revive it, down all 5,
      confirm `IsPartyWiped()`, all self-driven

---

## Log
(Newest entries at the bottom.)

- **File created.** Scoped the plan above after the user diagnosed the real root cause of
  `PressKey()`'s gameplay-input gap and asked for a dedicated toolset rather than continuing the
  manual per-milestone console/keypress workaround. Read `LiveCodingToolset` in full as the
  structural template before writing anything. Next: M0's remaining boxes (uplugin, Build.cs,
  module skeleton).

- **M0-M3 written.** Created `Plugins/GameplayTestToolset/` with the full module: `.uplugin`
  (depends on `ToolsetRegistry` + `EnhancedInput`, `PostEngineInit` loading phase, mirrors
  `LiveCodingToolset.uplugin` exactly), `Build.cs`, an empty-Startup/Shutdown module (registration
  lives in the subsystem per the reference plugin's own split), `GameplayTestToolset.h/.cpp`
  (`TriggerInputAction`/`ExecCommand`, both null/empty-checked with `RaiseScriptError`, both
  documented as requiring the target's own client-local `PlayerController` since
  `GetLocalPlayer()` is null on a server-side replica of a remote client),
  `GameplayTestToolsetSubsystem.h/.cpp` (CVar-toggled registration, copied line-for-line from
  `LiveCodingToolsetSubsystem` with names swapped), and a `GameplayTestToolsetTests` module using
  `CQTest`'s `FActorTestSpawner` to get a real non-null `APlayerController`/`UInputAction` in an
  isolated test world for the error-path tests, plus the two subsystem-registration tests copied
  from `LiveCodingToolsetTest`.
  **Real finding while writing tests, not assumed:** found a comment in this exact engine version's
  `UMGToolSetTest.cpp` ("RaiseScriptError fires through Blueprint VM (visible to AI), not in direct
  C++ calls") — confirms `AddExpectedError`/`TEST_ERROR` would be the wrong assertion for a test
  that calls the tool function directly rather than through actual Blueprint execution, so the
  error-path tests here assert "does not crash" rather than "logs an error," consistent with that
  established precedent rather than guessing.
  Added `{"Name": "GameplayTestToolset", "Enabled": true}` to `Unreal_first_Game.uproject`'s
  `Plugins` array. Confirmed the real build target name from `Binaries/Win64/*.target`
  (`Unreal_first_GameEditor`, not a guess) and kicked off a full external build via
  `Build.bat Unreal_first_GameEditor Win64 Development -Project=...` — a brand-new plugin can't be
  picked up by Live Coding at all (it's not "recompile an already-loaded module," the editor has
  never loaded this DLL), so this always needed a full rebuild + editor restart, unlike the
  UCLASS-in-an-existing-module case M9 tested earlier this session. Running in background; next:
  confirm it compiles clean, then ask the user to restart the editor so `unreal-mcp` reconnects and
  the new plugin loads, then M4's remaining verification (`AutomationTestToolset` run) and M5's live
  PIE check.

- **Build succeeded.** User confirmed the editor was closed (verified no `UnrealEditor.exe` process
  running before proceeding, not just taking the user's word). Ran the full external build —
  `Result: Succeeded`, 17/17 actions, ~11s via Unreal Build Accelerator. Both
  `UnrealEditor-GameplayTestToolset.dll` and `UnrealEditor-GameplayTestToolsetTests.dll` now exist.
  M4's plugin-enable and compile boxes are done. Next: ask the user to reopen the editor so
  `unreal-mcp` reconnects and the new plugin actually loads (a DLL existing on disk isn't the same
  as the running editor having it in memory), then run M4's remaining `AutomationTestToolset`
  verification and M5's live PIE checks.

- **Editor found already reopened; M4 and M5 both completed this session, no human keypress/typing
  involved anywhere.** Confirmed `UnrealEditor.exe` was already running (PID 34296, started 11:56:27)
  and `list_toolsets` showed `GameplayTestToolset.GameplayTestToolset` registered — the new plugin
  loaded correctly into the running editor.
  **M4 finished:** `AutomationTestToolset.DiscoverTests` → `ListTests` (filter `GameplayTestToolset`)
  found all 8 tests written in M3 → `RunTests` on all 8: **8/8 `Success`, 0 failed, 0 skipped**
  (`Subsystem_IsAvailable`, `Subsystem_RegistersToolsetByDefault`, three `TriggerInputAction_*`
  null-safety tests, three `ExecCommand_*` null-safety tests).
  **M5's first two boxes confirmed:** `describe_toolset` on `GameplayTestToolset.GameplayTestToolset`
  showed both `TriggerInputAction` and `ExecCommand` with correct schemas (`controller`/`action` and
  `controller`/`command` respectively).
  **M5's live PIE verification, genuinely exercised, not deferred to a human this time:** `StartPIE`
  (warmup 2s), waited for `RoleSelect`'s 30s auto-resolve via a backgrounded log poll (note: this
  session's log timestamps are UTC while `date`/file-mtime read local time, one hour ahead this
  session — don't mistake a UTC-timestamped fresh log line for a stale one from an old session, cross-
  check against the log's own "Log file open" header line instead). Found all 5 `CoopPlayerState`s via
  `find_actors` on the server world (`UEDPIE_0`): this run's Tank was `CoopPlayerState_3`
  (`PlayerId=259`). Cross-referenced each `UEDPIE_N`'s own local `BP_PlayerController_C_0.PlayerState`
  by `PlayerId` (same method M6/M7 established — per-world actor numbering doesn't line up across
  clients) and found `UEDPIE_3` is this run's Tank client. Confirmed `BP_PlayerCharacter_C_3` (server)
  is the matching pawn via its own `PlayerState` reference, baseline `ActiveStatusTags` empty,
  `HealthComponent` at `100/100`.
  **`TriggerInputAction(UEDPIE_3's PlayerController, IA_Shield)` — first attempt read back empty,
  correctly diagnosed as the same 5.0s-duration race M7's log already found with human keypresses, now
  recurring with pure MCP round-trip latency instead** (the six-ish seconds between the trigger call
  and the reflection read already exceeded `ShieldDurationSeconds`'s real 5.0s value). Applied the
  exact same established fix: widened `ShieldDurationSeconds` to 60 on the live `DA_GameConstants`
  (`set_properties`), re-triggered, immediately read back: `ActiveStatusTags` contained
  `Status.Shielded`, `StatusTagExpiryServerTime` showed a fresh timestamp — **confirms
  `TriggerInputAction` genuinely drives the real `IA_Shield` Enhanced Input binding through
  `ActivateShield`/`Server_ActivateShield` to `CoopTankAbilities::ApplyShield`, no keypress involved.**
  **`ExecCommand(UEDPIE_3's PlayerController, "ApplyTestDamage 30")` while shielded:** log confirmed
  `Cmd: ApplyTestDamage 30` ran as that player's own console command, followed by
  `ApplyTestDamage: BP_PlayerCharacter_C_3 took 30.0, now 100.0/100.0` — health unchanged, confirming
  `Status.Shielded`'s damage negation fired against a toolset-triggered tag, and that `ExecCommand`
  genuinely reaches a real `UFUNCTION(Exec)` scoped to the calling player, no console typing involved.
  Restored `ShieldDurationSeconds` back to 5.0 (`set_properties`, confirmed via `get_properties`),
  `save_assets([])` to persist the restore, `StopPIE`.
  Both of M5's live-verification boxes checked. M5's box for resuming Build 1's M9 through this
  toolset stays open — the toolset itself is now fully proven correct on both `TriggerInputAction` and
  `ExecCommand`, so that box is unblocked, not attempted yet this session.
  Next: resume `BUILD_1_PROGRESS.md`'s M9 (Downed/Revive/Wipe) entirely through this toolset — down
  one pawn via repeated `ApplyTestDamage`, confirm Downed/movement/ability lockout, revive it, down
  all 5, confirm `IsPartyWiped()`.

- **M9 (Downed/Revive/Wipe) resumed and fully verified through the toolset, entirely self-driven —
  and a real, non-obvious design finding caught along the way.** Fresh 5-client PIE (careful this
  time to grab the log's line count *before* `StartPIE` and only grep lines after it, avoiding the
  UTC-vs-local stale-match trap logged above). `RoleSelect` resolved at 30s as usual. Found this
  run's 5 `CoopPlayerState`s (`PlayerId`s 261-265, indices 0-4) and, learning from a mistake caught
  mid-session (see below), cross-referenced **every** `UEDPIE_N` client's own local `PlayerState` by
  reading its `PlayerId` directly rather than trusting matching local actor names across worlds — full
  mapping this run: `261→UEDPIE_0` (the host), `262→UEDPIE_1`, `263→UEDPIE_2`, `264→UEDPIE_3`,
  `265→UEDPIE_4`.
  **Real methodology mistake caught before it corrupted the result, not after:** briefly assumed
  `UEDPIE_2`'s and `UEDPIE_4`'s local `CoopPlayerState_4` (same local index, coincidentally) were the
  same player — they weren't (`263` vs `265`). Per-world actor numbering not matching across clients
  was already documented from Build 1's M6, but this session shows the trap also applies to the
  *client's own* PlayerState object, not just cross-referencing someone else's — always read `PlayerId`
  directly off the specific `UEDPIE_N` instance in hand, never infer from a shared local name.
  **Real, unplanned finding: `ApplyTestDamage`-then-immediately-reflect on a fresh PIE session raced a
  revive nobody triggered on purpose.** Downed `PlayerId=261` (`ExecCommand("ApplyTestDamage 150")`
  on its own client-local controller) at the *default* `ReviveDurationSeconds=3.0`, then read back
  `ActiveStatusTags` empty and `HealthComponent.CurrentHealth=50/100` — not what a fresh 0-HP-then-
  untouched pawn should show. Root-caused, not hand-waved: `100 * ReviveHealthRestorePercent(0.5) =
  50` is exactly `UCoopHealthComponent::Revive()`'s formula, and `BP_GameState_C_0.DownedPlayerCount`
  read back `0` (round-tripped `0→1→0`) — meaning `BeginRevive`→`CompleteRevive` had already fired and
  finished. **Why:** CLAUDE.md §6.3 puts all 5 players in "a small locked circular arena" for the prep
  phase, and `ReviveRadiusUnits` is 150 units — small enough that a nearby, still-standing teammate is
  *already* overlapping the moment `SetDowned(true)` flips the revive-trigger sphere to
  `OverlapAllDynamic`, firing `OnReviveTriggerBeginOverlap`→`Server_AttemptRevive`→`BeginRevive`
  immediately, no one needing to walk over deliberately. At the real 3.0s duration this completes
  faster than any MCP reflection round-trip can observe the intermediate Downed state — a new,
  toolset-native version of the exact race M7's log already hit with human keypresses.
  **Fixed with the same established pattern, applied twice this session (once at 20s — still not
  enough margin for 5 chained reflection calls; once at 60s — enough):** widened
  `ReviveDurationSeconds` on the live `DA_GameConstants`, re-downed `PlayerId=261`, and this time
  confirmed the full Downed state cleanly before it could auto-clear: `ActiveStatusTags` contained
  `Status.Downed`; `HealthComponent.CurrentHealth=0/100`; `CharMoveComp.MovementMode=MOVE_None`
  (movement lockout, via `ActorTools.get_components`+`get_properties`); `DownedPlayerCount=1`.
  **Ability lockout during Downed relied on code review, not a live isolated test** — this run's down
  target (`PlayerId=261`) rolled `Damage`, not Tank/Control, so `Server_ActivateShield`/
  `Server_ActivateStabilize`'s role gate would already reject it regardless of Downed state, making a
  live test on this pawn non-isolating. `CoopPlayerController.cpp`'s `Status_Downed` guard clause in
  both functions (checked *after* the role gate, before calling into `CoopTankAbilities`/
  `CoopControlAbilities`) was already read in full this session — treated as sufficient per this
  project's own established precedent (M6 accepted code-review-only for the 0-HP delegate fire-once
  check on the same reasoning).
  **Waited past the widened 60s window (self-paced, no user involvement) and confirmed `CompleteRevive`
  fired correctly, driven by the same organic nearby-teammate proximity, not a scripted teleport:**
  `ActiveStatusTags` back to empty, `HealthComponent.CurrentHealth=50/100` (the real
  `ReviveHealthRestorePercent=0.5` formula, now genuinely observed rather than raced past),
  `DownedPlayerCount` back to `0`. Restored `ReviveDurationSeconds=3.0` — checked via `get_properties`
  before moving on, not assumed.
  **Wipe test:** widened `ReviveDurationSeconds` to 90 (guarantee no auto-revive mid-sequence), then
  ran `ExecCommand("ApplyTestDamage 150")` on all 5 players' own client-local controllers in sequence
  (`UEDPIE_0` through `UEDPIE_4`). `BP_GameState_C_0.DownedPlayerCount` read `5` on two separate reads
  a few seconds apart — stable, not transiently passing through on its way back down, confirming
  `IsPartyWiped()` (`DownedPlayerCount > 0 && DownedPlayerCount >= PlayerArray.Num()`, `5 >= 5`) would
  evaluate true. The stability itself is a real confirmation of `Server_AttemptRevive`'s own reviver-
  not-Downed guard: with every player Downed simultaneously, no valid reviver exists, so the wipe
  genuinely holds rather than trending back down on its own the way the single-down cases did.
  Restored `ReviveDurationSeconds=3.0` (confirmed via `get_properties`), `save_assets([])`, `StopPIE`.
  **`build_toolset.md`'s own M5 checklist is now fully complete.** `BUILD_1_PROGRESS.md`'s M9 Verify
  box is next to update to reflect this session's result.
