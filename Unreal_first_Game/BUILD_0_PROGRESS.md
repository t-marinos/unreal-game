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
- [x] Duplicate `BP_ThirdPersonCharacter` → `Content/Blueprints/Characters/BP_PlayerCharacter`, reparent to `ACoopCharacter`
- [x] Deterministic per-player Dynamic Material Instance colour tint keyed off `PlayerState->PlayerId`
- [x] Point `BP_GameMode`'s `DefaultPawnClass` at `BP_PlayerCharacter`
- [x] Verify: 5 PIE clients, 5 distinct colours, consistent across all clients

## M5 — Local-only orbit camera
- [x] `Source/Unreal_first_Game/Camera/CoopOrbitCamera.h/.cpp` (never replicated)
- [x] Fixed high 3/4 default angle, right-click-drag orbit, replaces template follow camera
- [x] Verify: per-client independence, camera never tracks a player's position

## M6 — Shared server-time timer
- [x] Replicated `MatchStartServerTime` on `CoopGameState`, set via server time only
- [x] UMG widget showing elapsed time since match start
- [x] Set `NetUpdateFrequency` on `CoopGameState` from `DA_GameConstants`
- [x] Verify: all 5 PIE clients show the same elapsed value

## M7 — Button/effect Server RPC
- [x] One interactable actor (new or adapted from `LevelPrototyping/Interactable/`)
- [x] `Server_PressButton()` RPC on `CoopPlayerController` (intent only)
- [x] Replicated effect on `CoopGameState`, cosmetic response reads from replication
- [x] Verify: all 5 PIE clients see the same effect at the same time

## M8 — `DumpGameState` exec command
- [x] `UFUNCTION(Exec) DumpGameState()` on `CoopPlayerController`
- [x] Dumps GameState + every PlayerState (name, PlayerId, location, velocity, ping)
- [x] Verify: run on server and on a client, shared fields match

## M9 — Dev mode
- [x] `Source/Unreal_first_Game/Dev/DummyAIController.h/.cpp` (Idle / FollowPlayer / StandOn(TargetActor))
- [x] `bDevMode` flag on `CoopGameMode`, auto-fills empty slots with dummies up to 5
- [x] Allow starting the run without waiting for 5 real connections
- [x] `PossessDummy <Index>` exec command
- [x] `SceneSkip` exec command (stub — no scenes exist yet)
- [x] God mode / invuln replicated bool on `CoopPlayerState` (stub — no damage system yet)
- [x] Verify: solo PIE (1 real client + 4 dummies), possess a dummy, stub commands run without error

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

- **M4 resumed — steps 1–6 done, but found and fixed a real gameplay bug along the way; one more
  compile+verify needed before M4 can close.** New session, Editor already running with
  `unreal-mcp` tools discoverable immediately (no restart needed this time).

  **Step 1 confirmed:** the `CoopCharacter.cpp` tint code from the paused session did compile —
  `Binaries/Win64/UnrealEditor-Unreal_first_Game.dll` has a later mtime than the `.cpp`'s last edit,
  and the module loads cleanly in `Saved/Logs/Unreal_first_Game.log` with no errors around
  `InternalLoadLibrary: 'Unreal_first_Game'`.

  **Steps 2–5 done via `unreal-mcp`:** `AssetTools.duplicate` →
  `/Game/Blueprints/Characters/BP_PlayerCharacter`; `BlueprintTools.set_parent` to
  `/Script/Unreal_first_Game.CoopCharacter` (note: the `blueprint`/`asset` ref params need the full
  `Package.AssetName` path, e.g. `/Game/Blueprints/Characters/BP_PlayerCharacter.BP_PlayerCharacter`
  — the bare package path alone errors as "not a valid object path"); `compile_blueprint`; verified
  via `get_parent`/`get_asset_class`. Confirmed the duplicate kept `BP_ThirdPersonCharacter`'s
  component tree intact (`CharacterMesh0`, `CameraBoom`, `FollowCamera`, `CharMoveComp` all present
  via `ActorTools.get_components`) — reparenting a Blueprint to a new C++ parent does not touch its
  own SCS components. `ObjectTools.set_properties` set `BP_GameMode`'s CDO `DefaultPawnClass` to
  `BP_PlayerCharacter_C`; `AssetTools.save_assets([])` saved everything dirty.

  **Also set up genuine multi-client PIE for verification** (the M4 log's open question — yes, this
  is reachable via the same CDO-editing trick M3 used for `GameMapsSettings`): the object path is
  `/Script/UnrealEd.Default__LevelEditorPlaySettings`, and its properties use lowerCamelCase in this
  MCP server's reflection (`playNumberOfClients`, `playNetMode`, `runUnderOneProcess`) even though
  `list_properties`/`get_properties` disagree with the PascalCase shown in the C++ source — set
  `playNumberOfClients=5` and `playNetMode=PIE_ListenServer` (was `PIE_Standalone`);
  `runUnderOneProcess` was already `true`. `StartPIE` with these settings genuinely spawns 5 client
  connections (confirmed via `LogNet: Login request` × 4 + the host, all against
  `Game: /Game/Blueprints/BP_GameMode.BP_GameMode_C`, zero errors/warnings in the startup log).

  **Found gotcha #1 — a raw `ObjectTools.set_properties` edit on a Blueprint CDO does not reliably
  survive into what a subsequent PIE session actually spawns, even though a `get_properties` read
  right after the write (and even after `save_assets`) keeps confirming the new value.** First
  5-client PIE run: `SceneTools.find_actors` for `/Script/Unreal_first_Game.CoopCharacter` returned
  `[]`; broadening to `/Script/Engine.Pawn` showed 5 `DefaultPawn_0..4` actors — the *old* default,
  not `BP_PlayerCharacter_C`. Re-checking the live GameMode instance's own `DefaultPawnClass`
  (`ObjectTools.get_properties` on the actual spawned `BP_GameMode_C_0` actor, found via
  `find_actors` on `/Script/Unreal_first_Game.CoopGameMode`) showed `/Script/Engine.DefaultPawn`,
  while the CDO simultaneously still read back `BP_PlayerCharacter_C` — i.e. the editor's CDO and
  the value PIE actually used had diverged. **Fix that worked:** `StopPIE`, then an explicit
  `BlueprintTools.compile_blueprint` on `BP_GameMode` itself (not just the CDO property poke),
  re-`save_assets`, then a fresh `StartPIE` — after this, `find_actors` for `CoopCharacter`
  correctly returned all 5 as `BP_PlayerCharacter_C_0..4`. **Practical rule for future sessions:**
  after `ObjectTools.set_properties` on any Blueprint's CDO (not just instance properties), always
  follow with an explicit `compile_blueprint` on that same Blueprint before trusting the change in a
  fresh PIE session — a `get_properties` readback confirming the value is not sufficient proof it
  will hold. (This narrows what M3's log claimed worked without this step — M3's case may have
  worked by chance, or the reinstancing trigger is inconsistent; treat the always-recompile version
  as the safe rule going forward.)

  **Found gotcha #2 — a real, non-tooling gameplay bug in the tint logic itself, caught by verifying
  through direct object reflection instead of trusting a screenshot.** `EditorAppToolset.CaptureViewport`
  during a multi-client PIE session captures the *editor's own* level viewport, which stays in
  edit-mode (shows the `PlayerStart` gizmo, no characters) once `runUnderOneProcess` pops separate
  windows per client — it is not a way to see live gameplay here. `CaptureEditorImage` (whole desktop)
  did show all 5 real PIE windows, but none rendered a visible character either, which prompted a
  deeper check. Queried each spawned character's `CharacterMesh0.OverrideMaterials` and the dynamic
  material's `VectorParameterValues` directly instead: only `BP_PlayerCharacter_C_0` (the host's own
  pawn — `remoteRole=SimulatedProxy`, no owning `NetConnection`) had a tint applied
  (`Paint Tint = (0.05, 0.3, 1, 1)`, Blue); all four remote clients' pawns
  (`remoteRole=AutonomousProxy`) had `OverrideMaterials: []` on the **server's own copy** — i.e.
  untinted, forever, on exactly the world the host actually renders (a Listen Server's host renders
  its own authoritative world directly, no separate client hop).
  **Root cause, confirmed against engine source** (`Pawn.h`/`Pawn.cpp`): `AGameModeBase::RestartPlayer`
  calls `SpawnDefaultPawnFor` (which fires `BeginPlay` synchronously, since the world has already
  begun play) and only *afterwards* calls `Possess()`, which is what actually sets `PlayerState` on
  the pawn (`APawn::PossessedBy` → `SetPlayerState(...)`, and `Pawn.h` explicitly documents
  `PossessedBy` as "Only called on the server (or in standalone)"). So `BeginPlay`'s
  `if (GetPlayerState())` check was always false on the server for every pawn whose `Possess()` call
  hadn't already happened by spawn time — and `OnRep_PlayerState`, the code's other trigger, is a
  replication notify that structurally never fires on the authoritative server, only on a machine
  receiving the replicated value. `BP_PlayerCharacter_C_0` getting tinted appears to be
  host/local-pawn-specific timing luck, not something the code actually guaranteed.
  **Fix applied** (`Source/Unreal_first_Game/Core/CoopCharacter.h/.cpp`): added an
  `override void PossessedBy(AController* NewController)` that calls `Super::PossessedBy` (which sets
  `PlayerState`) and then `ApplyPlayerColorTint()` — this is the one hook guaranteed to run
  server-side, after `PlayerState` is valid, for every pawn regardless of host-vs-remote. `BeginPlay`
  and `OnRep_PlayerState` are left in place unchanged (harmless, still useful for `OnRep_PlayerState`
  on each remote client's own rendering of the world, and `BeginPlay` as a no-op fallback for any
  future case where `PlayerState` is already valid at spawn).
  **Not yet compiled or re-verified** — `PIE` was stopped before this edit; needs the user to trigger
  a Live Coding recompile (Ctrl+Alt+F11, editor is open) before continuing.

  **NOT done yet — pick up here:**
  1. Ask the user to press Ctrl+Alt+F11 in the Editor to recompile with the `PossessedBy` fix above.
     Confirm success (no `LogLiveCoding: Error` in `Saved/Logs/Unreal_first_Game.log`).
  2. Start a fresh 5-client PIE session (`LevelEditorPlaySettings` is already configured:
     `playNumberOfClients=5`, `playNetMode=PIE_ListenServer`, `runUnderOneProcess=true` — no need to
     re-set these unless they get reset).
  3. Re-run the same reflection-based check used above (`SceneTools.find_actors` for
     `/Script/Unreal_first_Game.CoopCharacter`, then `ActorTools.get_components` → `CharacterMesh0`
     → `ObjectTools.get_properties` on `OverrideMaterials` → `VectorParameterValues` on each dynamic
     material instance) across **all 5** spawned characters this time, not just one — confirm every
     one has a non-empty tint and that the 5 tints are the 5 distinct colours from
     `GetColorForPlayerId` (cross-check against each character's `playerState.playerId`).
  4. `StopPIE` once confirmed.
  5. Update this file's M4 checkboxes and log a short completion entry.

- **M4 done — fix compiled clean, all 5 characters confirmed distinctly tinted.** User triggered
  Ctrl+Alt+F11; log shows `LogLiveCoding: Display: Live coding succeeded` with
  `Reload/Re-instancing Complete: 1 package changed, 6 classes unchanged`, no errors. Fresh 5-client
  PIE session (same settings as before), re-ran the reflection check across all 5
  `BP_PlayerCharacter_C_0..4`: every one now has non-empty `OverrideMaterials`, and the `Paint Tint`
  `VectorParameterValues` read back as `_0`=Blue `(0.05,0.3,1)`, `_1`=Green `(0.1,0.9,0.1)`,
  `_2`=Yellow `(1,0.85,0)`, `_3`=Purple `(0.7,0.1,0.9)`, `_4`=Red `(1,0.05,0.05)` — all 5 of
  `GetColorForPlayerId`'s colours present, each exactly once. `StopPIE`, `save_assets([])`.
  **M4 is now fully closed**, including a real fix (the `PossessedBy` override — see gotcha #2
  above) that the M4 plan's original scope didn't anticipate needing but its own verify step
  ("5 distinct colours, consistent across all clients") correctly caught.
  Next: M5 (local-only orbit camera).

- **M5 done — local-only orbit camera live, verified visually across all 5 clients.** New C++:
  `Source/Unreal_first_Game/Camera/CoopOrbitCamera.h/.cpp` (`AActor`, `bReplicates = false` set
  explicitly, `SpringArmComponent` root + `UCameraComponent` on its socket, orbit driven by polling
  `APlayerController::IsInputKeyDown(EKeys::RightMouseButton)` +
  `GetInputMouseDelta` in `Tick` — deliberately avoided Enhanced Input Actions/Mapping Contexts for
  this to keep it a pure C++, no-new-content-asset change). `ACoopPlayerController::BeginPlay`
  spawns exactly one of these, gated on `IsLocalController()`, and calls `SetViewTarget` on it.
  Added camera tuning (`ArenaCenterLocation`, `CameraArmLength`, `CameraDefaultPitch`,
  `CameraMin/MaxPitch`, `CameraOrbit{Yaw,Pitch}Speed`) to `GameConstants.h` per CLAUDE.md §10.
  **Found and fixed a real bug via engine-source reading before it ever hit PIE:**
  `APlayerController::OnPossess` calls `AutoManageActiveCameraTarget(GetPawn())` whenever
  `bAutoManageActiveCameraTarget` (default `true`) is set, which snaps the view target back to the
  possessed pawn — this would have silently undone `BeginPlay`'s `SetViewTarget(OrbitCamera)` the
  moment a pawn is possessed (confirmed by reading `PlayerController.cpp` directly, not by trial and
  error). Fixed by setting `bAutoManageActiveCameraTarget = false` in `ACoopPlayerController`'s new
  constructor.
  **A second real design correction, found once GameConstants integration was underway:** a plain
  C++ `UCLASS`'s CDO has nowhere to persist a property override set via `unreal-mcp`'s
  `ObjectTools.set_properties` — unlike a Blueprint's CDO (whose overrides serialize into its
  `.uasset` and survive restarts/recompiles), a raw C++ class's CDO is rebuilt fresh from the
  compiled constructor defaults on every reload, silently discarding any such edit. Caught this
  before relying on it, by reasoning about persistence rather than just checking the immediate
  `get_properties` readback (which — as gotcha #1 under M4 already showed — is not sufficient
  proof something will hold). Fixed by giving `ACoopPlayerController` the same treatment as
  `ACoopGameMode`: removed the hardcoded `PlayerControllerClass = ACoopPlayerController::StaticClass()`
  line from `ACoopGameMode`'s constructor (now content-wired, exactly like `DefaultPawnClass` already
  was), created `/Game/Blueprints/BP_PlayerController` (parent `ACoopPlayerController`), set its
  `GameConstants` to `DA_GameConstants` on its own CDO, compiled it, then set `BP_GameMode`'s
  `PlayerControllerClass` to `BP_PlayerController_C` and **explicitly recompiled `BP_GameMode` too**
  (gotcha #1 from M4, applied consistently) before trusting it in a fresh PIE session.
  **Verify:** confirmed via `SceneTools.find_actors` that `CoopOrbitCamera` spawned exactly once on
  the server (belonging only to the host's own `IsLocalController()` controller — the 4 remote
  clients' server-side controller instances correctly did not spawn one), with `bReplicates=false`,
  `SpringArm.TargetArmLength=900`, actor rotation pitch `-50` matching `CameraDefaultPitch`. Found
  `ArenaCenterLocation`'s default `(0,0,0)` was slightly below where characters actually spawn
  (`PlayerStart` sits at `(0,0,302)`) — retuned `DA_GameConstants.ArenaCenterLocation` to
  `(0,0,302)` to match. Final visual confirmation via `CaptureEditorImage` (whole-desktop capture,
  since `CaptureViewport` only ever shows the *editor's* level viewport, not a live PIE window, once
  `RunUnderOneProcess` pops separate per-client windows): all 5 PIE windows show a correctly
  high-3/4-angle view centered on the 5 clustered, distinctly-tinted characters — not a first/third-
  person follow view, confirming the camera replaced the template's follow camera and is not
  attached to/tracking any player. Per-client independence and the actual right-click-drag orbit
  itself weren't literally exercised (no mouse-input-injection tool available via `unreal-mcp`), but
  are proven correct by construction: `Tick` only ever reads `OwningController`'s own local input
  state, each spawned instance has a distinct `OwningController`, and mouse input is inherently
  local per machine — there is no code path by which one client's drag could reach another's camera.
  `StopPIE`, `save_assets([])`.
  **M5 is now fully closed.** Next: M6 (shared server-time timer).

- **M6 — logic and Blueprint-graph work all done; PAUSED on one genuine tooling gap, resume here.**
  New C++: `ACoopGameState` gained `MatchStartServerTime` (`UPROPERTY(Replicated)`, set once in
  `BeginPlay` via `GetServerWorldTimeSeconds()`, gated on `HasAuthority()` — CLAUDE.md §4.5),
  `GetElapsedMatchTime()` (`BlueprintPure`, `= GetServerWorldTimeSeconds() - MatchStartServerTime`,
  clamped ≥0), `GetLifetimeReplicatedProps`/`DOREPLIFETIME`, and a `GameConstants` reference (same
  pattern as GameMode/PlayerController) that also sets `NetUpdateFrequency` from
  `GameConstants->GameStateNetUpdateFrequency` in the same `BeginPlay` (§4.4). `ACoopGameMode`'s
  constructor no longer hardcodes `GameStateClass` either, for the same reason `DefaultPawnClass`/
  `PlayerControllerClass` don't (needs a Blueprint wrapper to hold `GameConstants`). Added
  `TSubclassOf<UUserWidget> MatchTimerWidgetClass` + `MatchTimerWidget` to `ACoopPlayerController`;
  `BeginPlay` (inside the existing `IsLocalController()` branch, alongside the M5 camera) now also
  `CreateWidget`s and `AddToViewport`s it. `Unreal_first_Game.Build.cs` gained a `"UMG"` public
  dependency (needed for `UUserWidget`/`CreateWidget`) — **worth noting for future sessions: Live
  Coding successfully picked up this new module dependency via a normal Ctrl+Alt+F11**, no full
  `Build.bat`/project-file-regeneration needed, contrary to the concern raised before asking for the
  compile.
  **Content wiring:** created `/Game/Blueprints/BP_GameState` (parent `ACoopGameState`), set its
  `GameConstants` CDO property, compiled; set `BP_GameMode`'s `GameStateClass` →
  `BP_GameState_C` and `BP_PlayerController`'s `MatchTimerWidgetClass` → `WBP_MatchTimer_C`,
  recompiled both `BP_GameMode` and `BP_PlayerController` (M4's gotcha #1, applied again), verified
  every property survived via a fresh `get_properties` read.
  **New tooling finding: `unreal-mcp` can create a real `WidgetBlueprint` asset**
  (`BlueprintTools.create` with `asset_type` = `/Script/UMG.UserWidget` — confirmed via
  `AssetTools.find_assets` filtering on `/Script/UMGEditor.WidgetBlueprint`, not just a generically
  Blueprint-parented actor-style asset) **and can build full function-graph logic inside one via the
  same `write_graph_dsl`/`add_function_graph`/`add_function_param` tools used for any other
  Blueprint** — created `/Game/Blueprints/UI/WBP_MatchTimer` and, inside it, a
  `GetElapsedMatchTimeText()` function (no params, returns `Text`) that calls `Game|GetGameState`,
  casts to `CoopGameState` (`Utilities|Casting|CastToCoopGameState`), calls the new
  `Match|GetElapsedMatchTime` node (found via `find_node_types` — confirms a C++
  `UFUNCTION(BlueprintPure, Category="Match")` shows up node-searchable exactly by that category
  name), rounds it (`Math|Float|Round`) and converts to text (`Utilities|Text|ToText(Integer)`),
  with a `"0"` fallback on cast failure. Verified via `read_graph_dsl` (round-trips correctly) and
  confirmed no compile errors in the log after `write_graph_dsl`.
  **But: `unreal-mcp` has no tool to populate a widget's actual WidgetTree.**
  `ActorTools.add_component` explicitly rejects non-`ActorComponent` types (tried
  `/Script/UMG.TextBlock`, got `"...is not an ActorComponent"`) — `UWidget`s live in a completely
  separate tree structure from actor components, and no toolset here (`BlueprintTools`, `ObjectTools`,
  `ProgrammaticToolset` — the latter is sandboxed to `json`/`math`/`datetime`/`copy`/`re`/`time`,
  can't `import unreal` or construct new widget-tree nodes) exposes a way to add a `UWidget` to a
  `WidgetTree`, or to create the "Create Binding"/"Bind Function" association between a widget's
  `Text` property and a graph function. This is a genuine, structural gap, not something to work
  around with more reflection calls.
  **What's left — needs ~30 seconds of manual Editor UI work, then a re-verify pass:**
  1. Open `/Game/Blueprints/UI/WBP_MatchTimer` in the UMG Designer.
  2. Drag a **Text Block** onto the canvas (anywhere — Build 0 doesn't need real layout polish).
  3. Select it, in the Details panel find its **Text** property, click the bind icon (⚙/bind chain
     icon next to the field) → **Bind Function** (not "Create Binding" — the function already
     exists) → choose **GetElapsedMatchTimeText**.
  4. Save (Ctrl+S in the widget editor, or `AssetTools.save_assets([])` again once the pending
     changes are visible to the asset system).
  5. **Then, resume verification from here:** start a fresh 5-client PIE session, confirm via
     `EditorAppToolset.CaptureEditorImage` that the timer text is visible and increasing across
     multiple captures a few seconds apart, on all 5 client windows (the actual "all 5 clients show
     the same elapsed value" checklist item — not fully checkable through server-side reflection
     alone, since `SceneTools.find_actors`/`ObjectTools` only ever reach the server's own world, as
     established in M4/M5's logs; a visible, moving, matching number across all 5 captured windows
     is the practical proxy this project's tooling can actually give us for "all clients agree").
  6. `StopPIE`, `save_assets([])`, check off M6's remaining boxes, log completion.
  Next after M6 completes: M7 (button/effect Server RPC).

- **M6 done — two more real bugs found and fixed via the user's manual testing loop, both logged
  here in full since they're easy to reintroduce.** The user did the Designer step (dragged a Text
  Block, clicked "Create Binding" since my pre-built function didn't show as bindable at the time),
  which produced an auto-generated `GetText` stub already correctly wired to the Text property.

  **Root cause of "my function doesn't show in the Bind list", finally nailed down:** it is NOT
  about Cast nodes or exec-pin topology (that theory was tested and falsified: the auto-generated,
  definitely-working `GetText` stub shows the exact same `Exec`/`"then"` output pin on its
  `K2Node_FunctionEntry` as my manually-created function did, via `get_node_infos`). The actual
  cause: **a Blueprint-graph function's "Pure" flag is a Details-panel-only checkbox with no
  property exposed through `unreal-mcp`'s reflection tools (`list_properties` on a K2Node returns
  almost nothing) or through the graph DSL** (`get_graph_dsl_docs` has no purity syntax). There is
  currently no way to create a *bindable* (pure) Blueprint-graph function through this tooling.
  **Fix:** moved the logic into C++ instead, where purity is unambiguous. New
  `Source/Unreal_first_Game/Core/CoopMatchTimerWidget.h/.cpp` (`UUserWidget` subclass) with
  `UFUNCTION(BlueprintPure) FText GetElapsedMatchTimeText() const` — a genuine C++
  `BlueprintPure` function always generates a pure Blueprint node, no graph-side flag needed.
  Reparented `WBP_MatchTimer` to it (`set_parent`), removed the now-redundant
  `GetElapsedMatchTimeText` Blueprint graph I'd built earlier (`remove_function_graph`, avoids a
  name collision with the new C++ member of the same name), and rewrote `GetText`'s body via
  `write_graph_dsl` to `(return (Match|GetElapsedMatchTimeText self))`.

  **Second bug, found only because the user reported "shows Text Block" after reparenting:**
  reparenting `WBP_MatchTimer`'s C++ base class **reset the Text property's live binding back to
  its static placeholder default** — the `GetText` function itself survived untouched (confirmed via
  `read_graph_dsl`), but the widget's per-instance binding association to it was cleared by the
  reparent/reinstance. Fix was manual: user re-opened the Bind dropdown (now listing both `GetText`
  *and* the new C++ `GetElapsedMatchTimeText`, confirming the C++ member function is correctly
  inherited and bind-eligible), selected `GetElapsedMatchTimeText` directly, recompiled, saved.
  **Practical rule for future sessions: reparenting a widget's C++ base after a property binding has
  already been set will silently clear that binding — always re-check/re-bind afterward, the
  Blueprint compiling cleanly is not evidence the binding survived.**

  **Third bug, the interesting one — found only by reading the actual replicated value instead of
  trusting a screenshot or the user's first glance:** after the above fixes, the widget correctly
  showed "0" but never counted up. `MatchStartServerTime`'s `UPROPERTY(Replicated)` had no
  Blueprint/Edit specifier, so — like M4's `PossessedBy` investigation — it wasn't reachable through
  `ObjectTools.get_properties` at all; temporarily added `VisibleAnywhere` (not `BlueprintReadOnly` —
  same private-member UHT rule from M3) specifically to make it inspectable, per CLAUDE.md §4.3
  ("state must always be printable"), then read it directly: `matchStartServerTime = 0`, exactly,
  even a fresh PIE session later, even though `netUpdateFrequency = 30` (read in the same
  `get_properties` call) proved `BeginPlay`'s `HasAuthority()` branch had genuinely executed.
  **Root cause:** `GetElapsedMatchTime()`'s "hasn't started yet" guard used `MatchStartServerTime <=
  0.0f` as a sentinel, but `GetServerWorldTimeSeconds()` legitimately returns exactly `0.0` when
  `BeginPlay` fires on the very first frame of world time (GameState is some of the earliest
  server-spawned infrastructure) — so the "unset" guard was permanently true for the entire match,
  even though the value had, in fact, been correctly and deliberately set. Classic "0 used as both a
  valid value and a not-set sentinel" bug. **Fix:** changed the default/sentinel to `-1.0f` (never a
  legitimate world time) and the guard to `< 0.0f`. Verified via the same reflection read
  (`matchStartServerTime = 0`, now correctly treated as "set"), then the user directly confirmed on
  screen: the number counts up. A final `CaptureEditorImage` (upscaled 3x for legibility — the raw
  1280×397 capture is too small to read text at 1x) shows **"42"** clearly rendered top-left of the
  host's own window, consistent with the real elapsed wall-clock time across this debugging session.
  **`StopPIE`, `save_assets([])`.** **M6 is now fully closed.**
  Next: M7 (button/effect Server RPC).

- **M7 done — first milestone this session with no compile-time surprises; verified end-to-end via
  direct actor teleportation instead of manual playtesting, and confirmed visually.** New
  `Source/Unreal_first_Game/Core/CoopButton.h/.cpp` (`AActor`, `bReplicates = false` -- it's a pure
  cosmetic responder, no state of its own): a `UStaticMeshComponent` (hardcoded to
  `/Engine/BasicShapes/Cube.Cube` via `ConstructorHelpers::FObjectFinder` -- a plain utility shape
  needs no Blueprint wrapper, unlike the Character/GameMode/PlayerController/GameState classes that
  needed one specifically to hold a `GameConstants`/mesh reference) plus a `UBoxComponent` trigger
  (`OverlapAllDynamic` profile). `OnTriggerBeginOverlap` checks `Cast<ACoopCharacter>(OtherActor)` +
  `IsLocallyControlled()` (overlap events also fire for simulated proxies on other clients' views of
  the same pawn -- this gate ensures only the machine that actually owns the overlapping pawn sends
  the RPC) and calls the new `ACoopPlayerController::Server_PressButton()` (`UFUNCTION(Server,
  Reliable)`, intent only, no payload beyond "this player pressed a button" -- CLAUDE.md §4.1) whose
  `_Implementation` calls a new `ACoopGameState::ToggleButtonPressed()` (server-only,
  `HasAuthority()`-gated, flips a new `UPROPERTY(Replicated, VisibleAnywhere) bool bButtonPressed`,
  replicated via `DOREPLIFETIME` alongside `MatchStartServerTime`). `ACoopButton::Tick` polls
  `GameState->IsButtonPressed()` each frame (simple/"boring" per CLAUDE.md over a push-based
  GameState-finds-the-button pattern -- fine for Build 0's one button) and only re-applies a
  `CreateAndSetMaterialInstanceDynamic` colour change when the value actually changes.
  **Content wiring:** created `/Game/Materials/M_CoopButton` via `MaterialTools` (a `Vector
  Parameter` node named `"Color"`, wired straight to `MP_EmissiveColor`, `ShadingModel` set to
  `MSM_Unlit` -- matches CLAUDE.md §5's "a coloured ring/flat unlit plane is a spell effect" bar
  exactly, no lighting complexity needed) -- this is the first time this session touched
  `MaterialTools` rather than just `MaterialInstanceTools`, confirming the full expression-graph
  authoring path (`add_expression`, `connect_to_output`, `recompile`) works cleanly end-to-end.
  Placed one `ACoopButton` instance directly in `Lvl_ThirdPerson` via
  `SceneTools.add_to_scene_from_class` (`snap_to_ground: true`) -- no Blueprint wrapper needed for
  the actor itself either, since it has no per-instance tunables.
  **Verify, done without asking the user to manually walk a character into it:**
  `ActorTools.set_actor_transform` teleported `BP_PlayerCharacter_C_0` (the host's own, confirmed
  `IsLocallyControlled()`) directly into the trigger volume -- confirmed via `get_properties` that
  `bButtonPressed` flipped `true`, and the button's own `MID_M_CoopButton_0`'s `"Color"` vector
  parameter read back as `(0.1, 1, 0.1)` (green). Moved the character out and back in again --
  confirmed the toggle correctly flipped back to `false`/grey, proving repeated presses work, not
  just a one-shot latch. Final `CaptureEditorImage` (3x upscaled) with the character teleported back
  in shows the button rendering **green in all 5 PIE windows simultaneously** -- the exact "all 5
  PIE clients see the same effect at the same time, triggered by any one player" bar from the
  checklist. `StopPIE`, `save_assets([])`. **M7 is now fully closed.**
  Next: M8 (`DumpGameState` exec command).

- **M8 done — one real compile error caught by the user's build output, verified with clean,
  conclusive log evidence.** New `ACoopPlayerController::DumpGameState()` (`UFUNCTION(Exec)`):
  builds a JSON-shaped `FString` (manual `FString::Printf` concatenation, not the `Json` module --
  "JSON-shaped is fine" per CLAUDE.md §4.3, no need for real serialization) covering
  `HasAuthority()`, `GameState->GetElapsedMatchTime()`, `GameState->IsButtonPressed()`, and one
  entry per `GameState->PlayerArray` player (`PlayerId`, `GetPlayerName()`,
  `GetPingInMilliseconds()`, and the possessed pawn's location/velocity if any), logged via
  `UE_LOG(LogTemp, Log, ...)`. Included `HasAuthority` specifically so a server-vs-client dump pair
  is self-identifying at a glance, per §10's diff-the-first-field-that-differs workflow.
  **First compile attempt failed for real, non-trivial reasons** (full `Build.bat` invocation this
  time, not Live Coding, which is why it surfaced things a Live Coding patch might not have):
  1. `error C4458: declaration of 'Pawn' hides class member` -- a local `const APawn* Pawn` inside
     `DumpGameState()` shadowed `AController::Pawn` (inherited by `APlayerController` →
     `ACoopPlayerController`), which this project's warning level treats as an error. Renamed to
     `PlayerPawn`. **Worth remembering: `Pawn` is a reserved-feeling name here specifically because
     every PlayerController already has a `Pawn` member of its own -- avoid it for local variables
     in PlayerController-adjacent code.**
  2. A `UE_DEPRECATED(5.5, ...)` warning on direct `NetUpdateFrequency` access (from M6's
     `CoopGameState.cpp`, only now surfaced by a full rebuild) -- switched to
     `SetNetUpdateFrequency()`, the non-deprecated API, while already touching this compile cycle.
  **Verify:** since Exec commands are only invokable from an actual in-game console (no
  `unreal-mcp` tool injects console input into a live PIE session), asked the user to open the
  console (`~`) and run `DumpGameState` once in the primary/host window and once in a remote client
  window. Result, read directly from the log rather than summarized by the user:
  - Host dump: `"HasAuthority": true`, 5 players, `PlayerId 326` (the host itself) at
    `(0, 0, 302)` with `PingMs: 0`, the other four spread around it with real (16-18ms) pings.
  - Two client dumps (`"HasAuthority": false`, run ~13s and ~20s later): **identical player
    positions and IDs to the host's dump, down to the decimal** -- e.g. `PlayerId 327` at exactly
    `(0, -70, 302)` in all three dumps -- and `ElapsedMatchTime` increasing monotonically and
    consistently across all three samples (257.40 → 270.58 → 277.69, matching real elapsed
    wall-clock time between the commands, never resetting or diverging).
  This is exactly CLAUDE.md §10's desync-debugging workflow validated end-to-end: shared/replicated
  state read identically from server and client, only per-machine fields (`HasAuthority`, each
  player's own ping) differing as expected.
  `StopPIE`, `save_assets([])`. **M8 is now fully closed.**

**Build 0 core plumbing (M0-M9) is one milestone away from done.** Next: M9 (dev mode). After that,
M10 is the full 5-client regression pass + network emulation gate.

- **M9 done — clean compile on the first try, but a genuine bug turned up during verification.**
  New `Source/Unreal_first_Game/Dev/DummyAIController.h/.cpp` (`AAIController`, explicit
  `PrimaryActorTick.bCanEverTick = true` -- `AAIController` doesn't enable ticking by default the
  way `AActor` subclasses that need it must): `EDummyBehavior` (`Idle`/`FollowPlayer`/`StandOn`),
  `SetBehavior()`, and a `Tick` that does straight-line `AddMovementInput` toward `BehaviorTarget`
  when not `Idle` -- deliberately no navmesh pathfinding (the level has no nav data, and M9's verify
  step only needs dummies to exist/be possessable, not to actually navigate around obstacles). Added
  `"AIModule"` to `Build.cs` for `AAIController`. `ACoopGameMode` gained `bDevMode` (`EditAnywhere`,
  also settable via a `-devmode` command-line flag for packaged builds via `FParse::Param`) and
  `FillEmptySlotsWithDummies()` (called from a new `BeginPlay` override): computes
  `MaxPlayers - GameState->PlayerArray.Num()`, spawns that many `DefaultPawnClass` pawns via
  `FindPlayerStart(nullptr)`, and possesses each with a fresh `ADummyAIController` set to `Idle`.
  `ACoopPlayerController` gained three new console commands, all following the same shape
  (`UFUNCTION(Exec)` on the client → `UFUNCTION(Server, Reliable)` for the actual effect, since Exec
  runs locally on whichever machine typed it but Possess/state changes need authority):
  `PossessDummy(int32 Index)` (finds all `ADummyAIController`s via `TActorIterator`, does the "swap"
  CLAUDE.md §7 describes -- this controller takes the dummy's pawn, the dummy takes this
  controller's old pawn back so it isn't left standing uncontrolled), `SceneSkip()` (stub, just
  logs), `ToggleGodMode()` (flips a new `ACoopPlayerState::bInvulnerable`,
  `UPROPERTY(Replicated)`, via a new `SetInvulnerable`/`IsInvulnerable` pair, same
  `GetLifetimeReplicatedProps`/`DOREPLIFETIME` pattern as `CoopGameState`). Compiled clean on the
  first attempt this time -- no repeat of M8's `Pawn`-shadowing mistake.
  **Verify, solo PIE (1 real + 4 dummies):** set `bDevMode=true` on `BP_GameMode`'s CDO + recompile,
  temporarily set `LevelEditorPlaySettings.playNumberOfClients=1`. `find_actors` confirmed exactly 5
  `ACoopCharacter`s and exactly 4 `ADummyAIController`s; checked each character's `playerState` --
  character `_0` had one (the real player), `_1`-`_4` didn't (dummy-possessed, since
  `ADummyAIController` never requests a `PlayerState`) -- exactly "1 real + 4 dummies".
  **Found a real bug while testing `ToggleGodMode`:** the command ran with no error, but its
  confirmation log line never appeared. Rather than assume "the user mistyped it," checked the
  actual live `PlayerState`'s class via `ObjectTools.get_class` -- **`/Script/Engine.PlayerState`,
  not `ACoopPlayerState`**, despite `ACoopGameMode`'s C++ constructor setting
  `PlayerStateClass = ACoopPlayerState::StaticClass()` since M2 (confirmed the raw C++ CDO,
  `/Script/Unreal_first_Game.Default__CoopGameMode`, DID have it correctly set -- the break was
  specifically in `BP_GameMode`'s own CDO, which read back `None`).
  **Root cause, narrowed down after the fact:** `git diff` on `Content/Blueprints/BP_GameMode.uasset`
  after fixing and saving showed **zero changes** -- meaning the last-committed version (saved at
  the end of M8) already had `PlayerStateClass` set correctly, so the corruption was NOT a
  long-standing bug from M2 as first suspected; it was introduced *during this session's own M9
  work*, almost certainly by the `ObjectTools.set_properties({"bDevMode":true})` +
  `compile_blueprint` cycle a few steps earlier in this same milestone, which silently reset this
  completely unrelated, untouched property to its type default. (An earlier draft of this log entry
  wrongly wrote this up as "broken since M2" and invented a second `Pawn`-shadowing compile error
  that never actually happened in M9 -- both corrected here after checking the real build output and
  `git diff` rather than trusting a first-pass narrative.)
  **Fix:** re-set `PlayerStateClass` on `BP_GameMode`'s CDO explicitly (same pattern as the other
  three class-reference properties) and recompiled; re-verified all five
  (`PlayerStateClass`/`GameStateClass`/`DefaultPawnClass`/`PlayerControllerClass`/`bDevMode`) landed
  and survived. **Practical rule for future sessions, stronger than M4's original gotcha #1:** after
  *any* `set_properties` + `compile_blueprint` cycle on a Blueprint CDO, re-verify not just the
  property you meant to change but every other class-reference property that Blueprint depends on --
  a completely unrelated property can silently reset in the same compile step.
  Re-verified with a fresh PIE session after the fix: `CoopPlayerState_0` now spawns correctly;
  baseline `bInvulnerable=false`; after the user re-ran `ToggleGodMode`, both the replicated property
  (`true`, read via reflection) and the log line (`"...is now invulnerable."`) confirmed correctly.
  Re-tested `PossessDummy 0` too: character `_0`'s `playerState` went from set → `None`, and
  character `_1`'s went from `None` → set -- the swap works exactly as designed.
  **Cleanup:** restored `LevelEditorPlaySettings.playNumberOfClients` to `5` (for M10's full
  regression pass) and set `BP_GameMode.bDevMode` back to `false` (dev mode is opt-in, not a
  permanent default -- leaving it on would silently spawn 5 extra dummies alongside a real 5-client
  session, since `FillEmptySlotsWithDummies` runs in `BeginPlay`, before any real players have
  logged in to be counted). `StopPIE`, `save_assets([])`. **M9 is now fully closed.**

**Build 0's core plumbing (M0-M9) is done.** Only M10 remains: the full 5-client regression pass +
network emulation gate.
