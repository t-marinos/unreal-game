# Status Badge Progress Tracker

Resumable checklist for the overhead Shield/Fortress status badge feature. Full design/rationale
lives in the approved plan at `C:\Users\t-mar\.claude\plans\i-want-to-do-resilient-metcalfe.md` —
this file is just the checklist: what's done, what's next.

**If a session gets interrupted, a new session should:**
1. Read this file to see the last checked/unchecked item.
2. Read the plan file above for full design detail on the next unchecked item.
3. Continue from the first unchecked box.

---

## Code
- [x] New `Source/Unreal_first_Game/Core/CoopStatusBarWidget.h`/`.cpp` — `UUserWidget` base with
      `SetOwningCharacter`/`GetStatusText`/`GetStatusColor`/`GetStatusVisibility`, reads
      `Status.Shielded`/`Status.Fortress` from the owning `ACoopCharacter`.
- [x] `CoopCharacter.h`/`.cpp`: new `StatusBarWidgetComponent` (`UWidgetComponent`, Screen space,
      `SetDrawAtDesiredSize(true)`), new `GameConstants` reference, `BeginPlay` sets the height
      offset and calls `SetOwningCharacter`.
- [x] `GameConstants.h`: new `StatusBarHeightOffsetUnits = 180.0f` (`Category = "Status"`).
- [x] `Unreal_first_Game.Build.cs`: added `"SlateCore"` to `PublicDependencyModuleNames` — found
      necessary after the first build attempt failed (see Log below); `FSlateColor` lives in
      SlateCore, which `UMG` only depends on privately, so it never linked before.

## Build
- [x] **Full external rebuild from a fully closed editor** (not Live Coding — `UCoopStatusBarWidget`
      is a brand-new `UUserWidget` subclass, DECISIONS.md's confirmed-unsafe case, and the
      `Build.cs` dependency change can't be picked up by Live Coding at all regardless). First
      attempt (see Log) went through Live Coding while the editor was still open and failed with an
      unresolved `FSlateColor` symbol — root-caused and fixed (`Build.cs` above). User closed the
      editor and ran the real external rebuild; confirmed clean (see Log).
- [x] Reopen editor, confirm `unreal-mcp` reconnects.
- [x] Confirm the new class loaded cleanly: `ObjectTools.search_subclasses(UUserWidget, "CoopStatusBar")`
      finds `UCoopStatusBarWidget`, log sweep shows no UHT/module-load errors.

## Content (all via `unreal-mcp`, per the plan's Content steps section)
- [x] `UMGToolSet.CreateWidgetBlueprint` → `WBP_StatusBar` (parented to `UCoopStatusBarWidget`)
- [x] `UMGToolSet.AddWidget` → one root `TextBlock`
- [x] Wire the three property bindings (Text/ColorAndOpacity/Visibility → `GetStatusText`/
      `GetStatusColor`/`GetStatusVisibility`) — no `unreal-mcp` tool path exists (confirmed); done via
      the plan's documented fallback, a human doing the 3 "Bind Function" clicks in the Designer.
- [x] Wire `BP_PlayerCharacter`'s CDO: `StatusBarWidgetComponent.WidgetClass` →
      `/Game/Blueprints/UI/WBP_StatusBar.WBP_StatusBar_C`, `GameConstants` →
      `/Game/Data/DA_GameConstants.DA_GameConstants`. Compile, then **re-verify both properties
      survived the compile** (the M6 CDO-persistence lesson) — both survived.

## Verification
- [x] Trigger Shield/Stabilize on a live Tank/Control pair (`GameplayTestToolset`), confirm
      `ActiveStatusTags` shows `Status.Shielded` then `Status.Fortress` (never both).
- [x] Reflection-read `ActiveStatusTags` on the Tank and on covered teammates — confirmed via
      `ActiveStatusTags` reflection (see Verification plan step 1) plus the accessibility-tree text
      dump below for the widget's actual bound/rendered output (`GetUserWidgetObject()`'s live
      instance isn't reflectable via `ObjectTools`, same class of limitation as M11's
      `TWeakObjectPtr` finding — `SlateInspectorToolset` substitutes cleanly, per the plan's own
      Verification steps 3/4).
- [x] `SlateInspectorToolset.Screenshot` — visual confirmation on a live client window (Tank's,
      `Client 4`): blue `"SHIELDED"` badge, then gold `"FORTRESS"` badges.
- [x] `SlateInspectorToolset.Snapshot` — accessibility-tree text dump confirms exact rendered string.
- [x] Let `ShieldDurationSeconds`/`FortressDurationSeconds` expire, confirm the badge collapses.
- [x] `AssetTools.is_dirty` false + `.uasset` mtimes actually moved after `save_assets` — per the
      documented "save_assets can silently fail to reach disk" risk (DECISIONS.md).

**Status badge feature complete — all boxes checked.**

---

## Log
(Newest entries at the bottom.)

- **Code written.** `CoopStatusBarWidget.h`/`.cpp` created; `CoopCharacter.h`/`.cpp` and
  `GameConstants.h` updated per the approved plan. Asked the user to close the editor so a full
  external rebuild could run.
- **First rebuild attempt failed — root-caused, not worked around.** Build output showed
  `Result: Succeeded` for compiling the `.obj` files, but then a `"Building patch...for Live coding
  module"` step followed and failed: `LNK2019: unresolved external symbol
  Z_Construct_UScriptStruct_FSlateColor`, referenced from `GetStatusColor`'s generated reflection
  code. The `"patch_0"`/"Live coding module" phrasing is Live Coding's own linking mechanism (same
  as the crash DECISIONS.md already documents for a different reason), and `unreal-mcp` was still
  reachable immediately after — confirming the editor was never actually closed and this ran through
  Live Coding, not a genuine external rebuild. Checked `Unreal_first_Game.Build.cs`: `SlateCore` was
  never a dependency (only `UMG`, which depends on `SlateCore` privately, not publicly) — this is
  the first place in this project's C++ to use `FSlateColor` directly, so the gap was invisible
  until now. Added `"SlateCore"` to `PublicDependencyModuleNames`. Re-asked the user to actually
  close the editor this time (Build.cs changes can't be picked up by Live Coding at all, on top of
  the pre-existing new-widget-class restriction), so a real full external rebuild can run.
  Next: closed-editor rebuild, then the Content steps above.
- **Full external rebuild confirmed clean.** Saved all dirty editor assets first
  (`AssetTools.save_assets`, empty list). User closed the editor and ran the real external rebuild
  (not Live Coding). Verified this wasn't just another Live Coding pass hiding behind a stale
  process: `Get-Process -Id <pid>` shows `UnrealEditor.exe` restarted at `00:08:56`, after the
  rebuilt `UnrealEditor-Unreal_first_Game.dll`'s `00:07` mtime — a genuinely new process. Log sweep
  of `Saved/Logs/Unreal_first_Game.log` (fresh session log, starting `Log file open, 08/26/26
  00:08:57`) shows zero `LNK`/`UHT`/fatal/crash entries. `ObjectTools.search_subclasses(UUserWidget,
  "CoopStatusBar")` returns `/Script/Unreal_first_Game.CoopStatusBarWidget` — the new class loaded
  cleanly, confirming the `SlateCore` `Build.cs` fix worked. `unreal-mcp` reconnected without issue.
  Next: the Content steps (create `WBP_StatusBar`, add the `TextBlock`, wire the three property
  bindings, wire `BP_PlayerCharacter`'s CDO).
- **Content steps started.** `UMGToolSet.CreateWidgetBlueprint` created `WBP_StatusBar` parented to
  `UCoopStatusBarWidget` in one step (confirmed no separate `set_parent` needed).
  `UMGToolSet.AddWidget` added a `TextBlock` ("StatusText") as the root widget (`parent: "None"`
  confirms root placement).
  **Resolved the plan's flagged uncertainty on property bindings — no tool path exists.**
  `ObjectTools.list_properties`/`get_class` on the Blueprint asset's refPath both auto-resolve to
  the generated class's CDO (`WBP_StatusBar_C`) regardless of whether reached via the bare asset
  path or `AssetTools.load_asset` first — there is no way through `ObjectTools` to reach the raw
  `UWidgetBlueprint` editor object that actually owns the `Bindings` array (a `UWidgetBlueprint`-only
  property, not present on the generated class or its CDO). Confirmed `UMGToolSet.BindToEventProperty`
  is multicast-delegate-only (`OnClicked` etc., description explicitly requires "the matching delegate
  UPROPERTY"), not a match for Designer-style property binding. **Falling back to the plan's documented
  fallback: a human does the three "Bind Function" clicks in the Designer.** Next: human does the
  bindings, then CDO wiring on `BP_PlayerCharacter`.
- **Bindings done by the user; CDO wired; full live verification complete — feature done.**
  Confirmed `gameConstants`/`StatusBarWidgetComponent.WidgetClass` both unset on `BP_PlayerCharacter`'s
  CDO beforehand, set both via `ObjectTools.set_properties`, `compile_blueprint`, re-verified both
  survived (M6 CDO-persistence check, non-skippable per the plan). `save_assets`.
  **5-client PIE, real role auto-assign (no dev-mode dummies — waited out the 30s
  `RoleSelectDurationSeconds` timeout):** Control=PlayerId 256/`UEDPIE_0` (host, embedded in the main
  editor viewport — no separate "Client 0" preview window), Runner=257/`UEDPIE_1`, Damage=258/`UEDPIE_2`,
  Support=259/`UEDPIE_3`, Tank=260/`UEDPIE_4` — cross-referenced via the established M7 `PlayerId`
  method (server-world `CoopPlayerState`'s role+id, matched against each `UEDPIE_N`'s own local
  controller's `PlayerState.PlayerId`). This run's server-world actor numbering happened to be a clean
  1:1 index match (`BP_PlayerController_C_N`/`CoopPlayerState_N`/`BP_PlayerCharacter_C_N` all share
  index `N` for the same player) — convenient this session, not to be assumed automatically next time.
  **Whiff-then-hit sequence confirmed exactly like M8:** Stabilize against an unshielded Tank wrote
  nothing; Tank's Shield applied `Status.Shielded`; Control's Stabilize upgraded it to `Status.Fortress`
  or, cast too early, correctly whiffed on the 10s `stabilizeCooldownSeconds`/**most confusingly, on
  Shield's own duration racing out from under the test** (see gotcha below) rather than any binding or
  targeting bug — confirmed via a direct read of `CoopControlAbilities::ResolveStabilize`'s source,
  not guesswork.
  **Real methodology gotcha hit and root-caused, not worked around:** widened `ShieldDurationSeconds`/
  `FortressDurationSeconds` (60s, then 600s) on `DA_GameConstants` same as M7/M8's established
  round-trip-survival pattern, but repeatedly still caught `ActiveStatusTags` empty on later checks.
  Root cause, confirmed by reading `ACoopCharacter::ApplyStatusTag` (`CoopCharacter.cpp`): the expiry
  timer's `DurationSeconds` is captured **by value** at the moment `ApplyStatusTag` runs (
  `GetWorldTimerManager().SetTimer(Handle, Delegate, DurationSeconds, false)`), so widening the
  DataAsset only affects *casts made after* the edit — a cast made before it, or enough wall-clock time
  spent on slow tool calls (large `list_properties` dumps, screenshot capture/base64-decode/view) after
  a cast, still expires on its originally-captured duration. Fixed by keeping the widen → recast →
  verify sequence tight (no large exploratory calls in between), which produced clean, repeatable
  results. Worth remembering for any future ability-duration test.
  **Verified on live PIE, Tank's own client window (`Client 4`, `w25`) via `SlateInspectorToolset`**
  (`GetUserWidgetObject()`'s runtime instance isn't reflectable through `ObjectTools` — same class of
  gap as M11's `TWeakObjectPtr` finding — so visual/accessibility tools substituted cleanly, matching
  the plan's own Verification steps 3/4):
  - Shielded: `Snapshot` showed a `generic "SHIELDED"` node above the Tank; `Screenshot` confirmed it
    rendered in blue.
  - Fortress: `Snapshot` showed **five** `generic "FORTRESS"` nodes (one per teammate within
    `FortressCoverageRadiusUnits`, all clustered together in the small prep arena) — confirms
    multi-teammate coverage, not just the Tank's own upgrade; `Screenshot` confirmed gold/orange color.
  - Expiry: shrank durations to 5s, cast a fresh Shield→Stabilize pair, waited past expiry —
    `ActiveStatusTags` empty and `Snapshot` showed the same five nodes collapsed back to `size=0,0`
    with no text label, matching the pre-cast baseline exactly.
  Restored `ShieldDurationSeconds`→5.0/`FortressDurationSeconds`→8.0, confirmed via `get_properties`.
  `StopPIE`, confirmed `IsPIERunning` false. `save_assets([])`, then `AssetTools.is_dirty` false on
  `DA_GameConstants`/`WBP_StatusBar`/`BP_PlayerCharacter` plus a direct `.uasset` mtime check on disk
  (all three moved) — the documented "`save_assets` can silently fail to reach disk" risk, ruled out.
  **Status badge feature fully verified and complete.**
