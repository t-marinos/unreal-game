# Build 0 Progress Tracker

Working checklist for implementing Build 0 ("Five Mannequins in a Room"), per the approved plan.
Full context, rationale, and verification detail for each milestone lives in the plan file at
`C:\Users\t-mar\.claude\plans\plan-our-first-build-declarative-blum.md` — this file is just the
resumable checklist: what's done, what's next.

**If a session gets interrupted (e.g. runs out of credits), a new session should:**
1. Read this file to see the last checked/unchecked item.
2. Read the plan file above for the full detail on the next unchecked milestone.
3. Read `DECISIONS.md` for anything settled along the way.
4. Continue from the first unchecked box.

Scope for this pass (per session decisions): core plumbing only, verified solo via multiple
Play-In-Editor clients on one machine. Packaging + the real multi-friend Tailscale join test are a
**separate follow-up plan**, not tracked here.

---

## M0 — Repo hygiene
- [x] Add `.gitignore` (Binaries/, DerivedDataCache/, Intermediate/, Saved/, .vs/, *.VC.db, etc.)
- [x] `CLAUDE.md` §3: correct pinned engine version 5.8.2 → 5.8.1
- [x] `DECISIONS.md`: add entry recording the engine-version correction and why
- [x] `Config/DefaultEngine.ini`: disable Lumen / ray tracing / virtual shadow maps
- [x] `Config/DefaultGame.ini`: rename `ProjectName` away from template default
- [x] Verify: Editor still opens, PIE still plays exactly as before (just lighter)

## M1 — C++ module scaffold
- [x] `Source/Unreal_first_Game/Unreal_first_Game.Build.cs`
- [x] `Source/Unreal_first_Game/Unreal_first_Game.h` / `.cpp`
- [x] `Source/Unreal_first_Game.Target.cs` / `Source/Unreal_first_GameEditor.Target.cs`
- [x] Add `Modules` array entry to `Unreal_first_Game.uproject`
- [x] Confirm build path available (Visual Studio or UnrealBuildTool CLI)
- [x] Regenerate project files, confirm project compiles, Editor opens with C++ support
- [x] Verify: no PIE regressions

## M2 — Core C++ actor skeleton
- [x] `Source/Unreal_first_Game/Core/CoopGameMode.h/.cpp` (5-player cap via `PreLogin`)
- [x] `Source/Unreal_first_Game/Core/CoopGameState.h/.cpp` (empty skeleton)
- [x] `Source/Unreal_first_Game/Core/CoopPlayerState.h/.cpp` (empty skeleton)
- [x] `Source/Unreal_first_Game/Core/CoopPlayerController.h/.cpp` (empty skeleton)
- [x] `Source/Unreal_first_Game/Core/CoopCharacter.h/.cpp` (empty skeleton)
- [x] Wire in as project defaults (directly or via thin BP wrappers)
- [x] Verify: PIE behavior unchanged from before this milestone

## M3 — `DA_GameConstants`
- [x] `Source/Unreal_first_Game/Core/GameConstants.h/.cpp` (`UGameConstants : UDataAsset`)
- [x] Create `Content/Data/DA_GameConstants` asset instance
- [x] Create `BP_GameMode` wrapper holding the `DA_GameConstants` reference, set as `GlobalDefaultGameMode`
- [x] Verify: `MaxPlayers` read from data asset (6th-client rejection itself deferred to M10 -- see log)

## M4 — Character conversion + per-player colour tint
- [ ] Duplicate `BP_ThirdPersonCharacter` → `Content/Blueprints/Characters/BP_PlayerCharacter`, reparent to `ACoopCharacter`
- [ ] Deterministic per-player Dynamic Material Instance colour tint keyed off `PlayerState->PlayerId`
- [ ] Point `BP_GameMode`'s `DefaultPawnClass` at `BP_PlayerCharacter`
- [ ] Verify: 5 PIE clients, 5 distinct colours, consistent across all clients

## M5 — Local-only orbit camera
- [ ] `Source/Unreal_first_Game/Camera/CoopOrbitCamera.h/.cpp` (never replicated)
- [ ] Fixed high 3/4 default angle, right-click-drag orbit, replaces template follow camera
- [ ] Verify: per-client independence, camera never tracks a player's position

## M6 — Shared server-time timer
- [ ] Replicated `MatchStartServerTime` on `CoopGameState`, set via server time only
- [ ] UMG widget showing elapsed time since match start
- [ ] Set `NetUpdateFrequency` on `CoopGameState` from `DA_GameConstants`
- [ ] Verify: all 5 PIE clients show the same elapsed value

## M7 — Button/effect Server RPC
- [ ] One interactable actor (new or adapted from `LevelPrototyping/Interactable/`)
- [ ] `Server_PressButton()` RPC on `CoopPlayerController` (intent only)
- [ ] Replicated effect on `CoopGameState`, cosmetic response reads from replication
- [ ] Verify: all 5 PIE clients see the same effect at the same time

## M8 — `DumpGameState` exec command
- [ ] `UFUNCTION(Exec) DumpGameState()` on `CoopPlayerController`
- [ ] Dumps GameState + every PlayerState (name, PlayerId, location, velocity, ping)
- [ ] Verify: run on server and on a client, shared fields match

## M9 — Dev mode
- [ ] `Source/Unreal_first_Game/Dev/DummyAIController.h/.cpp` (Idle / FollowPlayer / StandOn(TargetActor))
- [ ] `bDevMode` flag on `CoopGameMode`, auto-fills empty slots with dummies up to 5
- [ ] Allow starting the run without waiting for 5 real connections
- [ ] `PossessDummy <Index>` exec command
- [ ] `SceneSkip` exec command (stub — no scenes exist yet)
- [ ] God mode / invuln replicated bool on `CoopPlayerState` (stub — no damage system yet)
- [ ] Verify: solo PIE (1 real client + 4 dummies), possess a dummy, stub commands run without error

## M10 — Full regression pass + network emulation
- [ ] Full 5-client PIE session exercising M2–M9 together
- [ ] Apply `Net.PktLag` / `Net.PktLagVariance` / `Net.PktLoss`, re-verify checklist still converges
- [ ] This is the "Build 0 core plumbing done" gate

---

## Deferred (not tracked here — separate follow-up plan)
- Packaging a Development-configuration Windows build
- Windows Firewall first-run prompt documentation
- Real multi-machine Tailscale join test with the other four friends

## Log
(Newest entries at the bottom. One line per completed step: what was done, anything notable.)

- **M0 done.** Added `.gitignore`; corrected CLAUDE.md's engine pin to 5.8.1 with a matching
  `DECISIONS.md` entry; disabled Lumen GI/reflections, ray tracing, and virtual shadow maps in
  `Config/DefaultEngine.ini`; renamed `ProjectName` in `Config/DefaultGame.ini`. Verified via the
  live Editor (unreal-mcp): started and stopped a PIE session with no errors after the edits — Note:
  the rendering cvar changes are in the `.ini` on disk but won't take visual effect in the currently
  *running* Editor process until it's restarted (cvars set at engine init are read at startup); this
  restart hasn't happened yet, so Lumen/RT are still visually active in the live session for now.
  Nothing else regressed. Next: M1 (C++ module scaffold) — needs a confirmed C++ build path
  (Visual Studio or UnrealBuildTool CLI) before starting.

- **M1 in progress — BLOCKED on toolchain, needs user action.** Created all module scaffold files
  (`Source/Unreal_first_Game.Target.cs`, `Source/Unreal_first_GameEditor.Target.cs`,
  `Source/Unreal_first_Game/Unreal_first_Game.Build.cs`/`.h`/`.cpp`) and added the `Modules` array
  entry to `Unreal_first_Game.uproject`. Confirmed a build path exists in principle: Visual Studio
  **Build Tools 2022** is installed at
  `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` (no full VS IDE), with MSVC
  toolset **14.37.32822**.
  **Ran the first compile** via
  `Engine\Build\BatchFiles\Build.bat Unreal_first_GameEditor Win64 Development <uproject> -waitmutex`
  and it **failed (exit code 6, "OtherCompilationError")**:
  > Visual Studio is installed, but is out of date or missing a valid C++ toolchain (minimum
  > version 14.38.33130, preferred version 14.50.35717). Please update Visual Studio to 17.8 or
  > later and verify that the "MSVC Build Tools v14.50 for x64/x86" component is selected in the
  > Visual Studio installation options.
  > Visual Studio x64 must be installed in order to build this target.

  **What's needed to unblock:** update Visual Studio Build Tools 2022 (via the Visual Studio
  Installer) to at least 17.8, with the "MSVC v143 Build Tools" / "MSVC Build Tools v14.50 for
  x64/x86" component selected, so the installed MSVC toolset reaches ≥14.38.33130 (UE 5.8's stated
  minimum). This needs the Visual Studio Installer GUI / an internet download — not something to
  run unattended. **No code changes are needed once this is fixed** — the scaffold files above are
  believed correct; just re-run the same `Build.bat` command above once the toolchain is updated.

- **MSVC toolchain updated by user** to 14.44.35207 (≥ the 14.38.33130 minimum) — confirmed via
  `vswhere`/checking `VC\Tools\MSVC`. Re-ran the same `Build.bat` command.

- **M1 still blocked — second, different toolchain gap.** The retry got past the MSVC check but
  failed at makefile-creation with `Result: Failed (RulesError)`:
  > Unable to instantiate module 'SwarmInterface': Could not find NetFxSDK install dir; this will
  > prevent SwarmInterface from installing. Install a version of .NET Framework SDK at 4.6.0 or
  > higher.

  Confirmed via registry check: no `HKLM\SOFTWARE\Microsoft\Microsoft SDKs\NETFXSDK` (or WOW6432Node
  equivalent) key exists at all — the .NET Framework **Developer Pack/targeting SDK** (distinct
  from the already-installed .NET Framework *runtime*, and distinct from the bundled .NET 10 SDK
  UBT itself runs on) has never been installed on this machine.
  **Fix found, not yet applied (needs user confirmation before installing software):** winget has
  `Microsoft.DotNet.Framework.DeveloperPack_4` (.NET Framework Developer Pack 4.8.1), which
  registers the NETFXSDK key UBT is looking for. Equivalent alternative: Visual Studio Installer →
  Individual Components → ".NET Framework 4.6.2 SDK" (or 4.7.2/4.8 SDK/targeting pack).
  Once installed, re-run the same `Build.bat` command again — still no code changes expected.

- **winget install attempted, failed — needs admin elevation this session doesn't have.**
  `winget install --id Microsoft.DotNet.Framework.DeveloperPack_4` downloaded
  `NDP481-DevPack-ENU.exe` successfully (hash verified) but the installer itself exited with code 1.
  Confirmed the Claude Code shell is running **non-elevated**
  (`WindowsPrincipal.IsInRole(Administrator)` = False) — the .NET Framework Developer Pack installer
  needs admin rights, and a UAC prompt can't be satisfied from this non-interactive session. The
  registry key still doesn't exist after the attempt, so nothing was actually installed.
  **Needs the user to do one of, from their own elevated session:**
  1. Open PowerShell/Terminal as Administrator, run:
     `winget install --id Microsoft.DotNet.Framework.DeveloperPack_4 --accept-package-agreements --accept-source-agreements`
  2. Or open the Visual Studio Installer normally (it self-elevates via UAC prompt) → Modify →
     Individual Components → check ".NET Framework 4.6.2 SDK" (or 4.7.2/4.8) → Modify/Install.
  Once either completes, re-run `Build.bat Unreal_first_GameEditor Win64 Development <uproject.
  path> -waitmutex` — still no code changes expected, this is purely an environment gap.

- **Still blocked — rechecked, NetFxSDK genuinely not present yet.** Re-ran `Build.bat`: identical
  `RulesError` / "Could not find NetFxSDK install dir" failure. Searched harder for it: no
  `C:\Program Files (x86)\Windows Kits\NETFXSDK` folder, no `NETFXSDKDir` machine env var, no
  `Microsoft.Net.Component.4.6.2.*` / `...4.8.SDK` package in the full `vswhere -include packages`
  listing. Visual Studio itself is now at 17.14.36414.8 (further updated since the earlier MSVC
  fix), so *something* was updated — but the specific ".NET Framework 4.6.2/4.8 SDK" **individual
  component** has not been added, and the earlier winget attempt never completed (blocked on
  elevation, see above). **Still needs**: either the VS Installer → Individual Components → add
  ".NET Framework 4.6.2 SDK" (or 4.7.2/4.8 SDK), or the winget command run from an elevated
  PowerShell. Re-run `Build.bat` after either one.

- **Both toolchain gaps now resolved.** User installed .NET Framework 4.8 SDK — confirmed via
  registry (`HKLM\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\NETFXSDK\4.8`,
  `KitsInstallationFolder=C:\Program Files (x86)\Windows Kits\NETFXSDK\4.8\`) and
  `C:\Program Files (x86)\Windows Kits\NETFXSDK` now exists on disk. Re-ran `Build.bat`.

- **M1 blocked a third time — Live Coding lock, not a missing tool.** Build got past both prior
  checks and reached UnrealHeaderTool, then failed:
  > Unable to build while Live Coding is active. Exit the editor and game, or press Ctrl+Alt+F11 if
  > iterating on code in the editor or game.

  The Unreal Editor is currently open (the same live session `unreal-mcp` has been talking to
  throughout this whole session) with Live Coding enabled, which locks module binaries and conflicts
  with an external `Build.bat` invocation. This is the module's **first-ever** build — it doesn't
  exist as a loaded module yet, so Live Coding's in-Editor Ctrl+Alt+F11 recompile can't create it
  either; the Editor genuinely needs to be closed for this one build.
  **Needs the user to close the Unreal Editor** (save any open work first) before the next retry —
  not something to close unattended, since it could discard unsaved Blueprint/level edits. Once
  closed, re-run the same `Build.bat` command; after it succeeds the Editor can be reopened (it will
  now load as a C++ project, with `unreal-mcp` presumably reconnecting once it's back up).

- **M1 done — build succeeded, Editor confirmed loading as a C++ project.** New session (Editor and
  all build processes were confirmed closed via `tasklist` first — the M1 blocker from last time was
  gone). Re-ran the exact same `Build.bat Unreal_first_GameEditor Win64 Development <uproject>
  -waitmutex` command: **`Result: Succeeded`**, 7/7 actions (resource compile, PCH, `Unreal_first_Game.cpp`,
  lib+dll link, metadata) in 89.4s, output `UnrealEditor-Unreal_first_Game.dll`. Engine located at
  `C:\Program Files\Epic Games\UE_5.8`.
  Regenerated Visual Studio project files via the engine's **bundled** .NET 10 runtime directly
  (`Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe` + `UnrealBuildTool.dll -projectfiles`)
  — note for future sessions: the machine's system-PATH `dotnet` only has .NET 6.0.x runtimes
  installed and fails with a version-resolution error against this engine's UBT; always invoke UBT
  through its own bundled dotnet, not the system one. `Unreal_first_Game.sln`/`.slnx` written
  successfully (VS2022 can't host the separate .NET-10 `Automation_*` C# project — harmless, doesn't
  affect the game module).
  Launched `UnrealEditor.exe` directly on the `.uproject` and waited on the log: **`LogLoad: (Engine
  Initialization) Total time: 60.97 seconds`**, no `Fatal error`/`Assertion failed`, no `Error:` lines
  in the startup log besides one benign `LogModelContextProtocol: Error: Call to unknown method
  "server/discover"` (an MCP handshake probe the plugin doesn't implement — not a real failure).
  Confirmed via `netstat` that the `unreal-mcp` server is listening on `127.0.0.1:8000` with
  established connections back to this session's own `claude.exe` process.
  **Known gap, not yet resolved:** despite that live, established connection, `unreal-mcp`'s tools
  (`list_toolsets`/`describe_toolset`/`call_tool`) do not appear in this session's tool search —
  almost certainly because this Claude Code session's tool list was built before the Editor (and its
  MCP server) was running, and a mid-session reconnect doesn't refresh it. **User decision: restart
  the Claude Code session** so it reconnects fresh and picks up the `unreal-mcp` tools, rather than
  verifying PIE manually or skipping the check. **Next session should:** confirm `unreal-mcp` tools
  are now discoverable, then use them to verify no PIE regressions (M1's last unchecked box) before
  starting M2.

- **M1 fully done — PIE regression check passed, unreal-mcp confirmed working.** New session after
  the Claude Code restart: `list_toolsets` returned all 17 toolsets immediately, confirming the
  reconnect fixed the tool-discovery gap. Started a PIE session via `EditorAppToolset.StartPIE`
  (Lvl_ThirdPerson, still the template's `BP_ThirdPersonGameMode_C` — expected, M2 hasn't wired in
  `ACoopGameMode` yet). The tool call itself returned an error ("PIE ended before warmup completed")
  but the log told a different story: `PIE: Server logged in` and `PIE: Play in editor total start
  time 0.25 seconds` with zero `Error`/`Fatal`/`Assert` lines around startup, and a follow-up
  `IsPIERunning` call returned `true` — so PIE was actually healthy and still running; the tool's own
  warmup-completion tracking threw a false negative (worth remembering for future sessions: trust the
  log over that specific error if `IsPIERunning` disagrees). Captured the viewport (had to pass
  explicit non-null `annotations`/`captureTransform` — the schema marks them optional but the toolset
  implementation errors without them) and visually confirmed the level geometry renders normally.
  Stopped PIE cleanly. **M1 is now fully closed — no PIE regressions from the C++ module scaffold.**
  Next: M2 (Core C++ actor skeleton — `CoopGameMode`/`CoopGameState`/`CoopPlayerState`/
  `CoopPlayerController`/`CoopCharacter`).

- **M2 done — Core C++ actor skeleton compiles, wired, no PIE regression.** Created five empty(ish)
  classes under `Source/Unreal_first_Game/Core/`: `ACoopGameState` (`AGameStateBase`),
  `ACoopPlayerState` (`APlayerState`), `ACoopPlayerController` (`APlayerController`), `ACoopCharacter`
  (`ACharacter`, default `CharacterMovementComponent` prediction left untouched per §4.2), and
  `ACoopGameMode` (`AGameModeBase`) — the one with real logic: its constructor points
  `GameStateClass`/`PlayerStateClass`/`PlayerControllerClass` at the four Coop skeletons, and it
  overrides `PreLogin` to reject a 6th connecting player once `GameState->PlayerArray.Num() >= 5`
  (hardcoded `MaxPlayers = 5` for now — M3 replaces this with a `DA_GameConstants` read).
  Deliberately did **not** touch `DefaultEngine.ini`'s `GlobalDefaultGameMode` (still
  `BP_ThirdPersonGameMode`) or `ACoopGameMode`'s `DefaultPawnClass` — per the plan, M3 is what
  actually makes `ACoopGameMode` (via a `BP_GameMode` wrapper) the live default, and M4 is what gives
  `ACoopCharacter` a real mesh; wiring either in early would have broken this milestone's own "PIE
  unchanged" verify step.
  **Found and fixed a real bug, not a tooling quirk:** the module has no `Public`/`Private` folder
  split (flat subfolders per CLAUDE.md's repo layout), so UBT never added the module root as an
  include search path. My `#include "Core/CoopGameMode.h"`-style includes (matching standard Unreal
  folder-prefixed convention, e.g. `GameFramework/Actor.h`) failed to resolve except from within the
  same folder. Fixed at the root in `Unreal_first_Game.Build.cs` by adding
  `PublicIncludePaths.Add(ModuleDirectory);` rather than degrading the include style — this will
  matter again for every future cross-folder include (M5 `Camera/`, M9 `Dev/`, Build 1's `Tags/`/
  `Abilities/`/`Scenes/`), so fixing it now instead of working around it once was deliberate.
  **Build path used:** external `Build.bat` failed again with the same Live Coding lock as M1 (editor
  was open) — this time, rather than closing the editor, the user triggered an in-editor Live Coding
  recompile (Ctrl+Alt+F11) instead, which is the faster/correct path once the module already exists
  and is loaded. First attempt hit the `Core/` include bug above; after the `Build.cs` fix, a second
  Ctrl+Alt+F11 succeeded (`LogLiveCoding: Error: Live coding failed...` in the log is from the first,
  pre-fix attempt — the second one, confirmed via a follow-up PIE check, was clean).
  **Verify:** started PIE via `unreal-mcp` (`StartPIE` itself threw its now-familiar false-negative
  "ended before warmup completed" — same as M1, trust `IsPIERunning`/the log over that specific
  error), confirmed `IsPIERunning` = true, log showed `Game class is 'BP_ThirdPersonGameMode_C'`
  (unchanged) and a clean `PIE: Server logged in` / `Play in editor total start time 0.131 seconds`
  with no errors around the session. Stopped PIE cleanly. **M2 done, no regressions.**
  Next: M3 (`DA_GameConstants`).

- **M3 done - DA_GameConstants live, BP_GameMode is the actual GlobalDefaultGameMode.** Added
  `Source/Unreal_first_Game/Core/GameConstants.h/.cpp` (`UGameConstants : UDataAsset`, `BlueprintType`)
  with Build 0's first three tunables: `MaxPlayers` (5), `GameStateNetUpdateFrequency` (30 - top of
  the §4.4 20-30Hz range, bandwidth is a non-issue at 5 players), and
  `MatchTimerDisplayUpdateIntervalSeconds` (0.1 - a local UI refresh throttle for M6's elapsed-time
  widget, not a replication rate; `MatchStartServerTime` itself only ever replicates once). Gave
  `ACoopGameMode` a `GameConstants` property and switched `PreLogin` to read `MaxPlayers` from it,
  with a hardcoded `FallbackMaxPlayers = 5` + a `UE_LOG` warning if the reference is ever left unset
  (fails loud, not silently).
  **Hit a UHT rule, not a tooling quirk:** `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ...)` on a
  private member fails to compile ("BlueprintReadOnly should not be used on private members").
  Dropped `BlueprintReadOnly` - `EditDefaultsOnly` alone is enough to expose it on `BP_GameMode`'s
  Class Defaults panel, and nothing needs Blueprint-graph read access to it.
  **Build path:** same as M2, external `Build.bat` blocked by the Live Coding lock (editor open), so
  the user ran Ctrl+Alt+F11 in-editor instead - confirmed succeeded. Re-checked at the user's
  prompt whether `unreal-mcp` can trigger a Live Coding compile itself: it cannot - `list_toolsets`
  shows no Live Coding toolset, and the one tool that might reach further
  (`ProgrammaticToolset.execute_tool_script`) only orchestrates the same registered toolsets via a
  sandboxed script that can't `import unreal` or run console commands (allowed modules are just
  `json`/`math`/`datetime`/`copy`/`re`/`time`). This matches CLAUDE.md §3.2 exactly - C++ compiles
  are a human/IDE action, full stop. Worth remembering so future sessions don't re-litigate it.
  **Content wiring via unreal-mcp:** created `/Game/Data/DA_GameConstants`
  (`DataAssetTools.create`) and confirmed its defaults via `ObjectTools.get_properties`. Created
  `/Game/Blueprints/BP_GameMode` (`BlueprintTools.create`, parented to `ACoopGameMode`), compiled it,
  fetched its CDO (`BlueprintTools.get_default_object`), and set its `GameConstants` property to
  reference `DA_GameConstants` (`ObjectTools.set_properties` - object-reference values are just the
  asset's content path as a string, e.g. `"/Game/Data/DA_GameConstants.DA_GameConstants"`).
  **Found a real gotcha, logged so it doesn't cost a future session the same detour:** setting
  `DefaultEngine.ini`'s `GlobalDefaultGameMode` was NOT enough to make `BP_GameMode` live in PIE, and
  neither was directly editing the live `UGameMapsSettings` CDO in memory
  (`/Script/EngineSettings.Default__GameMapsSettings`, which is possible via `ObjectTools` and avoids
  needing an editor restart for the project-wide default) - PIE kept reporting `Game class is
  'BP_ThirdPersonGameMode_C'` either way. Root cause: `Lvl_ThirdPerson`'s own `WorldSettings` actor
  has an explicit GameMode Override (`DefaultGameMode` property) pointing at `BP_ThirdPersonGameMode_C`,
  which wins over both the ini and the project-wide CDO default. Fixed by also setting
  `WorldSettings.DefaultGameMode` to `BP_GameMode_C` via `ObjectTools.set_properties` on
  `/Game/ThirdPerson/Lvl_ThirdPerson.Lvl_ThirdPerson:PersistentLevel.WorldSettings` - confirmed via a
  fresh PIE session (had to `StopPIE`/`StartPIE` again since the already-running session had the old
  class baked in) that the log now reads `Game class is 'BP_GameMode_C'`, with no `LogTemp: Warning`
  about a missing `GameConstants` reference (i.e. `MaxPlayers` really is coming from the data asset,
  not the hardcoded fallback). Saved all dirty assets afterward (`AssetTools.save_assets` with an
  empty list - saves everything dirty: the level, the new Blueprint, the new data asset) and
  confirmed the level is no longer dirty.
  **Deferred, not skipped:** the actual 6th-player rejection behavior wasn't live-tested this
  milestone - `StartPIE`'s options have no "number of clients" field, so a genuine 6-client PIE
  session needs a different mechanism (Editor Preferences / `ULevelEditorPlaySettings` multiplayer
  options, or the Editor's own "Number of Players" control). Left for M10's full 5-client regression
  pass, which was always the plan's real gate for this kind of multi-client behavior anyway.
  **M3 done.** Next: M4 (Character conversion + per-player colour tint).

- **M4 in progress - PAUSED mid-milestone, resume from here.** Session stopped to save progress
  before the C++ side was even compiled yet, so nothing below is verified in PIE. Exact state:

  **Done:**
  - Investigated the Mannequin's actual material setup via `unreal-mcp` (worth knowing for any
    future material work, not just this milestone): `BP_ThirdPersonCharacter`'s `Mesh` component
    uses `SKM_Quinn_Simple` with no per-instance override materials, so tinting must go on the
    mesh asset's own material slots. `SkeletalMeshTools.get_material_slots` found two slots,
    `Quinn_01` and `Quinn_02`, both assigned `MaterialInstanceConstant`s (`MI_Quinn_01`,
    `MI_Quinn_02` under `/Game/Characters/Mannequins/Materials/Quinn/`).
    `MaterialInstanceTools.list_parameters` on either one found a `Vector` parameter named exactly
    `"Paint Tint"` (with a space, case-sensitive) -- this is the one to drive per-player colour, not
    `LogoTint` (that's the chest-logo decal colour, a different thing).
  - Wrote the actual tint logic in `Source/Unreal_first_Game/Core/CoopCharacter.h/.cpp`:
    `BeginPlay()` (covers server + the locally-controlled client) and an overridden
    `OnRep_PlayerState()` (covers remote clients, since `PlayerState` replicates asynchronously to
    them) both call a private `ApplyPlayerColorTint()`, which reads `PlayerState->GetPlayerId()`,
    maps it through a hardcoded 5-entry `static const TArray<FLinearColor>` (Red/Blue/Green/
    Yellow/Purple, indexed by `PlayerId % 5`) via `GetColorForPlayerId()`, then calls
    `GetMesh()->CreateAndSetMaterialInstanceDynamic(SlotIndex)` for every material slot and sets
    the `"Paint Tint"` vector parameter on each. This is deliberately computed identically on every
    client from replicated data -- no Server RPC, purely cosmetic, matches CLAUDE.md
    §5/DECISIONS.md's "Dynamic Material Instance colour tint" approach exactly.
  - Confirmed (again) that `unreal-mcp` cannot compile this -- external `Build.bat` hit the same
    "Unable to build while Live Coding is active" lock as M2/M3 (editor still open). Asked the user
    to press Ctrl+Alt+F11. **Session was paused before their reply came back, so it is NOT confirmed
    whether this compile succeeded.**

  **NOT done yet -- pick up here:**
  1. Confirm the Ctrl+Alt+F11 compile above actually succeeded (ask the user, or have them retry it,
     before touching anything else -- don't assume).
  2. `AssetTools.duplicate` `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` to
     `/Game/Blueprints/Characters/BP_PlayerCharacter` (matches the repo layout's
     `Content/Blueprints/Characters/` convention).
  3. `BlueprintTools.set_parent` the duplicate to `/Script/Unreal_first_Game.CoopCharacter`, then
     `compile_blueprint`.
  4. Point `BP_GameMode`'s `DefaultPawnClass` to the new `BP_PlayerCharacter_C` via
     `ObjectTools.set_properties` on `/Game/Blueprints/BP_GameMode.Default__BP_GameMode_C`.
  5. `AssetTools.save_assets` (empty list = save everything dirty).
  6. Verify: the plan wants 5 PIE clients each seeing all 5 characters in 5 distinct colours,
     consistently across every client's view. `StartPIE`'s options (per its input schema) have no
     "number of clients" field, so a genuine multi-client PIE session needs a different mechanism --
     worth checking whether `ULevelEditorPlaySettings`'s CDO (same trick used in M3 for
     `GameMapsSettings`, likely something like
     `/Script/UnrealEd.Default__LevelEditorPlaySettings`, property probably `PlayNumberOfClients`)
     is reachable and settable via `ObjectTools`, the same way `GameMapsSettings` was. If that
     doesn't pan out, a single-client PIE check (does the local player render in a non-white/grey
     colour, i.e. `Paint Tint` is actually being overridden) is an acceptable interim check, with the
     full 5-distinct-colours claim deferred to M10 same as M3's 6th-player-rejection deferral.
  7. Update this file's M4 checkboxes and log a completion entry once verified, same format as
     M1-M3 above.

  Next after M4 completes: M5 (local-only orbit camera).

- **M4 resume attempt — blocked again, same tool-discovery gap as M1, needs session restart.**
  New session found the Unreal Editor **not running at all** (no `UnrealEditor.exe` process, port
  8000 not listening) — it must have been closed since the last session paused mid-M4, so the
  pending Ctrl+Alt+F11 compile confirmation (step 1 above) is moot; whatever state that compile was
  in didn't carry forward as a loaded process either way. Relaunched it directly
  (`UnrealEditor.exe Unreal_first_Game.uproject`), waited on `Saved/Logs/Unreal_first_Game.log`:
  `LogLoad: (Engine Initialization) Total time: 42.05 seconds`, no `Fatal error`/`Assertion failed`
  — clean startup. Confirmed via `netstat` that `unreal-mcp`'s server is listening on
  `127.0.0.1:8000` with established connections back to this session's `claude.exe` — identical
  signature to the healthy state from the end of M1's log. **But** `list_toolsets`/`call_tool` are
  not discoverable via this session's tool search, same as the M1 blocker: the session's tool list
  was built before the Editor/MCP server came up this time, and a mid-session reconnect doesn't
  refresh it. **Needs the user to restart the Claude Code session** (same fix as M1) so it
  reconnects fresh and picks up the `unreal-mcp` tools. **Next session should:** confirm
  `unreal-mcp` tools are discoverable, then resume exactly at M4 step 1 — verify the C++ tint code
  in `CoopCharacter.h/.cpp` actually compiles now (the Editor is freshly loaded from whatever was
  last built on disk, so this needs a real check — e.g. an in-editor Live Coding compile or
  `LiveCodingToolset.CompileLiveCoding` via unreal-mcp — not an assumption), then continue with
  steps 2–7 unchanged.
