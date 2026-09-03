# Cursor Targeting, Target Frame & Party Frames (WoW-style) — Plan & Progress Tracker

Resumable checklist for a mouse-cursor unit-selection system: a visible cursor, click a teammate or
an enemy to "target" it, show that unit's frame (name, role, HP bar, status) in the top-left, and
always show a stacked party frame for all five players — like WoW's target frame + party/raid
frames. This doc **is** the plan — there is no separate spec file (same convention as
`ABILITIES_PROGRESS.md`).

> **STATUS (2026-09-03): P1–P3 DONE, P4 mostly done (post-bake pass verified 4.2/4.3-ally/4.6/4.9;
> a few pure-visual items still need a human), P5 DONE + all THREE playtest-feedback rounds baked.
> Feature is code-complete, self-checked, and re-verified against the baked DLL.** All C++
> built and baked by closed-editor `Build.bat` (latest DLL 799,744 B @ 14:25 — P2's rebuild plus the
> three feedback rounds; nothing owed). All content wired via `unreal-mcp` and verified on disk.
> Post-bake 5-client PIE confirmed: zero runtime errors/AccessedNone/script warnings all session;
> party frame = 5 rows (name/role/HP bar/HP number), replicated values consistent across clients,
> local row tinted, HP + "DOWNED" update live; target frame invisible with no target; **party-row
> click populates the target frame** (Client 1) with **client isolation** intact (Client 2
> unaffected); **ground ring** unhides / follows (pawn − 88) / greens for an ally / carries the
> radius-90 scale. `DECISIONS.md` entry written + the stale RoleSelect bullet fixed (P5.1/5.2).
> **PLAYTEST-FEEDBACK ROUND (2026-09-03, post-P5). 3 C++ deltas + 1 content edit — NOW BAKED
> (closed-editor `Build.bat`, exit 0, DLL 799,744 B @ 13:50, no warnings/errors):**
> 1. **Allies not click-targetable** — root cause: `BP_PlayerCharacter` capsule + mesh both
>    `Visibility → Ignore` (monsters work via `BlockAllDynamic`). Fix: `ACoopCharacter::BeginPlay()`
>    blocks `ECC_Visibility` on the capsule. (Live-Coding-patched + runtime-verified; now baked.)
> 2. **Cursor should hide while right-click held** — `ACoopOrbitCamera::Tick` edge-detects RMB and
>    toggles `SetShowMouseCursor`. (New `bWasOrbiting` member.) Now baked.
> 3. **Click a party-frame row → target that teammate** — `UCoopUnitFrameWidget::NativeOnMouseButtonDown`
>    (left-click a `PartyMember` row → `ACoopPlayerController::SetCurrentTarget`; RMB/else → `Unhandled`
>    so camera drag still falls through). `NativeTick` makes rows `Visible` only when they have a
>    subject. `WBP_PartyFrame` `RootCanvas` → `SelfHitTestInvisible` (saved content, was never a rebuild).
>    Revises plan decisions #4 (rows were display-only) and #7 (frames non-interactive). Now baked.
>
> **POST-BAKE PIE PASS (2026-09-03, editor reopened PID 11684 @ 13:54, running the baked
> 799,744 B DLL). Fresh solo 5-client agentic PIE on the *baked* artifact — throttle off for the
> run, restored to `true` after; StopPIE clean, no stray processes:**
> - Both new UCLASSes (`CoopUnitFrameWidget`, `CoopTargetRing`) confirmed loaded in the baked editor.
> - RoleSelect → Prep → HoldTheGate all progressed; **zero `Error:` / `Accessed None` / script
>   warnings / exceptions in the entire session log** (only the 3 pre-existing benign startup lines).
> - Party frame + target-frame-hidden re-confirmed on 3 clients (w1 host / w5 / w6), live HP + DOWNED.
> - **Party-row left-click → target (revised decision #4, checklist 4.3 ally path) VERIFIED end-to-end:**
>   left-clicked the TANK party row on Client 1 (`WBP_UnitFrame::NativeOnMouseButtonDown` →
>   `SetCurrentTarget`) → target frame flipped from "Text Block" placeholders (RenderOpacity 0) to
>   `Theo-…F2B… / TANK / [bar] / 100 / 100`. Repeated on Client 0 (host).
> - **Client isolation (4.6) VERIFIED:** Client 1 setting a target left Client 2's target frame
>   still showing placeholder text — untouched. Plain `TWeakObjectPtr`, no replication, confirmed live.
> - **Ground ring (4.9) VERIFIED** via `ObjectTools` on `BP_TargetRing_C_0` (host PIE world): `bHidden`
>   `true`→`false` on target set; location `(300,0,214)` = TANK pawn `(300,0,302)` − `TargetRingGroundOffsetUnits`
>   88 (follows + ground offset); scale `1.8` = `TargetRingRadiusUnits` 90 / 50; MID `M_TargetRing`
>   `Color` param = `(0.15,0.85,0.20)` = `AllyRingColor` green (target is an `ACoopCharacter`). CDO
>   `ringMaterial` ref intact.
> - Tooling note: SlateInspector `Click` on the **full editor viewport** (host) needs window-focus +
>   a viewport pre-click first; on the dedicated **preview windows** (Clients 1-4) it works directly.
>
> **STILL NEEDS A HUMAN (pure visual / can't simulate a positioned world-click):** cursor visible from
> match start (4.1); cursor **hides while RMB held** + reappears on release; **target a monster** →
> "ENEMY" + red bar/ring (4.4 — could not click a monster via a row; it's the trivial `else` branch of
> the same, now-proven, `NativeTick` / ring `Tick`); right-click-drag camera works with the mouse over
> the party frame (4.7); a **world-click on a character** via `SelectTargetUnderCursor` + the
> capsule-collision fix (trace-verified in the earlier session); RoleSelect buttons still clickable
> (4.8 — auto-resolved before a click could be sent this run).
>
> **PLAYTEST-FEEDBACK ROUND 3 (2026-09-03) — cursor position not restored on RMB release. NOW BAKED
> (closed-editor `Build.bat`, exit 0, DLL 799,744 B @ 14:25, no warnings/errors).** User:
> *"When you hold right click to rotate the camera and then you let go, the mouse cursor needs to be
> in the same location where it was first right clicked, now it ends up elsewhere."* Root cause: the
> round-2 comment's assumption was wrong — with `GameAndUI` + `DoNotLock` the *hidden* OS cursor
> still tracks the physical mouse during the drag, so `SetShowMouseCursor(true)` on release just
> reveals it wherever it drifted to. **Fix (`ACoopOrbitCamera` only):** snapshot the cursor's
> viewport position (`GetMousePosition`) on the RMB press edge → warp it back (`SetMouseLocation`) on
> the release edge. 3 new plain members (`bHasSavedCursorPos`, `SavedCursorX/Y`) + edge-handler
> logic in `Tick`. Comment corrected in code + `DECISIONS.md`. Known limitation left un-fixed: cursor
> isn't *locked* mid-drag, so a very large sweep can push it to the viewport edge and stall the orbit
> — a per-tick re-centre if a later playtest hits it.
>
> **NEXT: user reopens the editor → resume the playtest** (all C++ baked; nothing owed a rebuild).
> Commit still held until the playtest passes (user's call — one commit for the lot).
> P5.3 `CLAUDE.md` §5 line — **DONE** (user approved).

**If a session picks this up:** read this file top to bottom, then read `DECISIONS.md`'s entries
"RoleSelect screen feedback is `NativeTick`-driven", "No `unreal-mcp` tool can set a UMG Designer
Bind Function", "Live Coding must not be used to add a new UCLASS", and "WoW-style action bar" —
this plan reuses all four patterns.

---

## Is it possible? — Yes

- **Cursor + click-to-select**: `APlayerController::GetHitResultUnderCursor()` deprojects the mouse
  through the active view (our `ACoopOrbitCamera`, which is the view target) and returns the actor
  under it. Filter the hit to `ACoopCharacter` / `ACoopMonsterCharacter`. Built-in, no plugin.
- **The frames**: UMG widgets, same `UUserWidget` + C++ base + `NativeTick` pattern as
  `WBP_ActionBar` / `WBP_RoleSelect`.
- **The data** is already there and already replicated:
  - HP: `UCoopHealthComponent` on both `ACoopCharacter` and `ACoopMonsterCharacter`
    (`GetHealthPercent()`, `GetCurrentHealth()`, `GetMaxHealth()` — both `Replicated`).
  - Role: `ACoopPlayerState::GetRole()` (replicated `EPlayerRole`).
  - Name: `APlayerState::GetPlayerName()`.
  - Status: `HasStatusTag()` on both classes (replicated `ActiveStatusTags`); teammate Downed via
    `ACoopCharacter::GetDownedComponent()->IsDowned()`.
  - Party roster: `AGameStateBase::PlayerArray` (replicated).
- **No new networking.** Selection is *what the local player is looking at*, not gameplay state —
  CLAUDE.md §4.2's local, cosmetic, client-only category. Each of the five players targets
  independently; nobody's target affects anyone else's screen (same guarantee as `ACoopOrbitCamera`).

The one real constraint: **CLAUDE.md §5 forbids post-processing**, so no WoW-style selection
outline/glow. A flat coloured ring decal on the ground is the §5-sanctioned substitute
("A coloured ring on the ground ... is a spell effect").

---

## Where this sits vs. the build plan

Not in any build's checklist. `BUILD_1_PROGRESS.md` is at M13; Build 1 is Hold the Gate + Fortress.
This is a **user-requested mini-feature outside strict build order** — same category as monster
combat, the "Q ability per role", the WoW-style action bar, and the status badge (all logged
deviations, not scope creep). It forward-enables real design: `docs/abilities.md` specs Armor Break
/ Link / Mind Fracture / Channel / Speed as **explicitly targeted** ("Tank targets one enemy
actor", "Support targets one teammate"), and the code has repeated "no crosshair yet" notes. This
builds the crosshair.

---

## Decisions — LOCKED (resolved 2026-09-03, do not re-litigate)

1. **Cursor is visible for the whole match.** Ownership moves to `ACoopPlayerController::BeginPlay`
   (`SetShowMouseCursor(true)` + `FInputModeGameAndUI` with `DoNotLock` +
   `SetHideCursorDuringCapture(false)`). `UCoopRoleSelectWidget`'s per-phase cursor/input-mode
   toggle is **deleted** — no longer needed.
2. **Click empty ground clears the current target.** `Esc` also clears (`ClearTarget()`). (WoW's
   actual default is to keep it; clearing is the simpler prototype behaviour.)
3. **Ground selection ring is in scope** — a flat coloured ring under the targeted unit (green =
   ally, red = enemy). It is the **last** build item; the frames ship first and the ring can slip
   without blocking.
4. **Party frames are in v1** — a top-left vertical stack of up to five rows, one per player in
   `PlayerArray`: name, role, HP bar, status. Always visible during Prep + HoldTheGate. The local
   player's own row is tinted to stand out (no separate "player frame" widget). **Rows are
   display-only / `HitTestInvisible`** in v1 (you target by clicking the character in the world —
   keeps decision-#7 and the camera-drag fall-through intact); click-a-row-to-target is a noted
   follow-on.
5. **Targeting works in every phase** (Prep included — harmless, and helps players learn the roster).
6. **This will eventually drive abilities** (future phase, not v1): a later phase routes
   `ACoopPlayerController::GetCurrentTargetActor()` into the `Server_Activate*` RPCs as *intent*,
   server re-validates. v1 leaves that seam clean and changes no ability code.
7. **Frames are not interactive.** All frame widgets (`HitTestInvisible`) — the cursor always falls
   through to the game so right-click-drag camera works with the mouse anywhere on screen.
8. **Show/hide via `RenderOpacity`, never a widget's own `Visibility`** from its own `NativeTick`
   (the P9 action-bar freeze gotcha — a widget that leaves the "visible" family in its own tick
   never ticks again). Stay `HitTestInvisible` always, toggle `RenderOpacity` 0/1.
9. **`NativeTick` + `BindWidgetOptional`, no Designer "Bind Function" bindings** anywhere (can't be
   authored via `unreal-mcp` — `DECISIONS.md`). Same as `UCoopRoleSelectWidget` /
   `UCoopActionBarWidget`.
10. **Coloured bar/text aesthetic, no art.** Name text, role text, a `UProgressBar` HP bar tinted
    green (ally) / red (enemy), an HP number, one status-text line (concatenated tag short-names,
    the `UCoopStatusBarWidget` approach). "Everything readable, nothing pretty" (§5).
11. **One reusable unit-frame widget class** (`UCoopUnitFrameWidget`) drives *both* the target
    frame and each party row — a small `EUnitFrameSource { CurrentTarget, PartyMember }` +
    `int32 PartyMemberIndex` per instance decide which actor it reads. Keeps this to **one** new
    `UUserWidget` subclass (plus the ring `AActor`).
12. **Tunables in `DA_GameConstants`** (ring radius, ring ground-offset); colours stay
    hardcoded-cosmetic (the `GetColorForPlayerId` / `GetStatusColor` precedent). CLAUDE.md §10.
13. **No new gameplay tag.** This only *reads* existing tags — no `docs/abilities.md` edit.

## Non-goals for v1 (explicitly deferred — do not build)

- Abilities consuming the selected target (decision #6 is a *future* phase).
- Click-a-party-row-to-target (rows are display-only in v1).
- Target-of-target, focus frame, tab-target cycling, hover highlight, nameplates over every unit.
- Any selection outline/glow (CLAUDE.md §5 — post-processing is out).
- Controller/gamepad targeting (CLAUDE.md §8).

---

## What already exists (Phase 1 findings)

- **Camera** (`Camera/CoopOrbitCamera.*`): local-only `AActor`, `bReplicates = false`, spawned
  per-client in `ACoopPlayerController::BeginPlay` behind `IsLocalController()`, set as view target.
  Right-click-drag orbit is read **directly** in `Tick` via
  `Controller->IsInputKeyDown(EKeys::RightMouseButton)` + `GetInputMouseDelta` — **not** Enhanced
  Input. Keeps working with a visible cursor / `GameAndUI` given `DoNotLock` +
  `SetHideCursorDuringCapture(false)` (RoleSelect already proves this).
- **Cursor / input mode**: normally `FInputModeGameOnly` + hidden. `UCoopRoleSelectWidget::NativeTick`
  (`CoopRoleSelectWidget.cpp` ~lines 98-113) flips to `FInputModeGameAndUI` + cursor during
  RoleSelect and back, using `bShowMouseCursor` as its "already switched" sentinel — its own comment
  says that's safe "only because nothing else in the project touches `bShowMouseCursor`."
  Decision #1 deletes this block.
- **Character** (`Core/CoopCharacter.h`): `ACharacter`. Capsule root + skeletal mesh.
  `GetHealthComponent()`, `HasStatusTag(FGameplayTag)`, `GetDownedComponent()`.
- **Monster** (`Core/CoopMonsterCharacter.h`): plain `AActor`, root `UStaticMeshComponent` ("Mesh")
  with collision profile `BlockAllDynamic` (blocks `Visibility` → a cursor trace on `ECC_Visibility`
  hits it). `GetHealthComponent()`, `HasStatusTag(FGameplayTag)`.
- **Health** (`Core/CoopHealthComponent.h`): `GetHealthPercent()`, `GetCurrentHealth()`,
  `GetMaxHealth()`; `CurrentHealth`/`MaxHealth` `DOREPLIFETIME`.
- **PlayerState** (`Core/CoopPlayerState.h`): `GetRole()` → `EPlayerRole`
  (`Unassigned/Tank/Support/Runner/Control/Damage`), replicated.
- **GameState** (`Core/CoopGameState.h`): `GetCurrentPhase()` → `EMatchPhase`
  (`WaitingForRoster/RoleSelect/Prep/HoldTheGate/Complete`). `PlayerArray` from the base class.
- **Tags** (`Tags/CoopGameplayTags.h`): `Status_Shielded`, `Status_Fortress`, `Status_Downed`,
  `Status_SpeedBuff`, `Status_Vulnerable_Physical`.
- **Widget patterns**: `UCoopStatusBarWidget` (reads an actor's tags → text/colour/visibility),
  `UCoopActionBarWidget` (phase-gated `RenderOpacity` toggle from `NativeTick`),
  `UCoopRoleSelectWidget` (`NativeTick` writes null-checked `BindWidgetOptional` pointers, never
  cached), `WBP_ActionBar` (a container WBP holding N placed sub-widget instances each with a
  per-instance index property — the model for `WBP_PartyFrame`).
- **Input assets** (`Content/Input/Actions/`): `IA_Move/Look/MouseLook/Jump/Shield/Stabilize/Speed/Dash/Execution`.
  **No `IA_Select`.** LMB almost certainly unbound — verify against `IMC_Default` in P3. Abilities
  are BP-wired: an `EnhancedInputAction IA_*` event node in **`BP_PlayerCharacter`'s EventGraph** →
  `Get Controller` → `Cast To BP_PlayerController` → call a `BlueprintCallable` on the controller.
- **Build.cs**: `UMG`, `GameplayTags`, `SlateCore`, `AIModule` present. **No `EnhancedInput`** —
  and not needed (the click stays BP-wired like every ability).
- **Live Coding rule** (`DECISIONS.md`): a brand-new `UUserWidget` subclass (and a brand-new
  `AActor` UCLASS) = **unsafe** under Live Coding → close the editor, full external `Build.bat`.
  P2 is that rebuild, exactly like `ABILITIES_PROGRESS.md` P4.

---

## File map

**C++ — new:**
- `Core/CoopUnitFrameWidget.h` / `.cpp` — the one reusable frame. `UUserWidget` base for
  `WBP_UnitFrame`. Per-instance `EUnitFrameSource Source` (`CurrentTarget` | `PartyMember`) +
  `int32 PartyMemberIndex`. `NativeTick`:
  - resolves the subject actor: `CurrentTarget` → `GetOwningPlayer<ACoopPlayerController>()->GetCurrentTargetActor()`;
    `PartyMember` → `GameState->PlayerArray[PartyMemberIndex]` then its pawn.
  - writes `BindWidgetOptional` children (`NameText`, `TypeText`, `HealthBar` (`UProgressBar`),
    `HealthText`, `StatusText`, and `RootBorder` for the "this is me" tint).
  - `RenderOpacity` 0 when there's no subject (no target / party index past the roster).
  - `static` helpers: `HealthOf(AActor*)`, `ActorHasTag(AActor*, FGameplayTag)`, `RoleTextOf(AActor*)`
    — explicit `Cast<ACoopCharacter>` / `Cast<ACoopMonsterCharacter>` per class, no shared base
    (CLAUDE.md §4.6).
- `Core/CoopTargetRing.h` / `.cpp` — local-only `AActor` (`bReplicates = false`), mirrors
  `ACoopOrbitCamera`'s per-client lifecycle. One `UDecalComponent` (or flat unlit
  `UStaticMeshComponent` disc) with `M_TargetRing`. `Tick` reads
  `OwningController->GetCurrentTargetActor()`, snaps to that actor's ground position, tints
  green/red by faction, `SetActorHiddenInGame(true)` when there's no target.

**C++ — modified:**
- `Core/CoopPlayerController.h` / `.cpp`:
  - `TWeakObjectPtr<AActor> CurrentTargetActor;` + `AActor* GetCurrentTargetActor() const;`
  - `UFUNCTION(BlueprintCallable, Category = "Targeting") void SelectTargetUnderCursor();` —
    `GetHitResultUnderCursor(ECC_Visibility, false, Hit)`; accept only `ACoopCharacter` /
    `ACoopMonsterCharacter`; empty hit → `ClearTarget()` (decision #2). Local-only, no RPC — every
    write to `CurrentTargetActor` commented per §4.1/§4.2.
  - `UFUNCTION(BlueprintCallable, Category = "Targeting") void ClearTarget();`
  - `BeginPlay` (inside the `IsLocalController()` block): cursor on + `FInputModeGameAndUI`
    (`DoNotLock`, `SetHideCursorDuringCapture(false)`).
  - `TargetFrameWidgetClass` / `TargetFrameWidget` and `PartyFrameWidgetClass` / `PartyFrameWidget`
    pairs + create-and-add-to-viewport in `BeginPlay` (the `ActionBarWidget` shape). After creating
    the target-frame instance, set its `Source = CurrentTarget` in code (CDO-snapshot gotcha).
  - Spawn `ACoopTargetRing` in `BeginPlay`.
- `Core/CoopRoleSelectWidget.cpp` — delete the cursor / input-mode block in `NativeTick`
  (~lines 98-113); keep `if (!bRoleSelectActive) return;`. Update the class comment.
- `Core/GameConstants.h` — `TargetRingRadiusUnits` (+ `TargetRingGroundOffsetUnits` if needed),
  `Category = "Targeting"`.

**Content — new (all via `unreal-mcp`):**
- `/Game/Input/Actions/IA_Select` — Input Action, `ValueType = Boolean`, `Pressed` trigger
  (duplicate `IA_Shield`).
- `/Game/Blueprints/UI/WBP_UnitFrame` — WidgetBlueprint, parent `UCoopUnitFrameWidget`. Tree:
  `RootBorder` → `HorizontalBox` → (`VerticalBox`: `NameText`, `TypeText`) + (`VerticalBox`:
  `HealthBar` `UProgressBar`, `HealthText`) + `StatusText`. All as `BindWidget` variables. Built
  with `UMGToolSet`.
- `/Game/Blueprints/UI/WBP_TargetFrame` — WidgetBlueprint (plain `UUserWidget` parent). A
  top-left-anchored `SizeBox` (slightly larger) holding one `WBP_UnitFrame` instance;
  `Source = CurrentTarget` set on that instance / in the controller after `CreateWidget`.
- `/Game/Blueprints/UI/WBP_PartyFrame` — WidgetBlueprint (plain `UUserWidget` parent). Top-left
  `VerticalBox` holding **five** `WBP_UnitFrame` instances, per-instance `Source = PartyMember` and
  `PartyMemberIndex` 0-4 (the `WBP_ActionBar` per-instance-`SlotIndex` pattern — set on each placed
  instance, not just the CDO).
- `/Game/Materials/M_TargetRing` — unlit / translucent ring material (like `M_CooldownSweep`), one
  `Color` vector param.

**Content — modified (all via `unreal-mcp`):**
- `/Game/Input/IMC_Default` — append `LeftMouseButton` → `IA_Select` (read the whole
  `DefaultKeyMappings` array, confirm LMB unbound, append, write back, re-verify prior entries).
- `BP_PlayerController` CDO — `targetFrameWidgetClass` → `WBP_TargetFrame_C`,
  `partyFrameWidgetClass` → `WBP_PartyFrame_C`, ring class ref.
- `WBP_UnitFrame` CDO / placed instances — `Source` / `PartyMemberIndex` per instance.
- `BP_PlayerCharacter` EventGraph — new `EnhancedInputAction IA_Select` event → `Get Controller` →
  `Cast To BP_PlayerController` → `SelectTargetUnderCursor` (buildable via `BlueprintTools` nodes).

---

## P1 — C++ (one batch; do NOT rebuild between sub-steps — P2 is the single rebuild)

- [x] **1.1** `CoopPlayerController`: `CurrentTargetActor` (`TWeakObjectPtr<AActor>`, not a
      UPROPERTY) + `GetCurrentTargetActor()` + `SelectTargetUnderCursor()` (trace `ECC_Visibility`,
      accept `ACoopCharacter` / `ACoopMonsterCharacter`, empty → `ClearTarget()`) + `ClearTarget()`.
      Local-only, commented, no RPC. Added `#include "Engine/EngineTypes.h"` + `"Engine/HitResult.h"`.
- [x] **1.2** `CoopPlayerController::BeginPlay`: `SetShowMouseCursor(true)` + `FInputModeGameAndUI`
      (`DoNotLock`, `SetHideCursorDuringCapture(false)`) inside the `IsLocalController()` block,
      right after the camera spawn.
- [x] **1.3** `CoopPlayerController`: `TargetFrameWidgetClass`/`TargetFrameWidget` +
      `PartyFrameWidgetClass`/`PartyFrameWidget` + create-and-add-to-viewport. (No "set `Source`
      after create" needed — `EUnitFrameSource::CurrentTarget` is the C++ default, so
      `WBP_TargetFrame`'s instance uses it as-is; only party rows override.)
- [x] **1.4** New `CoopUnitFrameWidget.h/.cpp` — `EUnitFrameSource` (`CurrentTarget` |
      `PartyMember`), per-instance `Source` + `PartyMemberIndex`, `NativeTick` resolves the subject
      (target actor / `PlayerArray[Index]`'s pawn), writes `BindWidgetOptional` children
      (`RootBorder`/`NameText`/`TypeText`/`HealthBar`/`HealthText`/`StatusText`), `RenderOpacity`
      0 when no subject **or** phase ∉ {Prep, HoldTheGate}, "this is me" tint on party rows, the
      `static` helpers `HealthOf` / `ActorHasTag` (explicit `Cast` per class, §4.6).
- [x] **1.5** `CoopRoleSelectWidget.cpp` — removed the cursor/input-mode block from `NativeTick`,
      kept the `if (!bRoleSelectActive) return;` feedback bail, replaced the comment.
- [x] **1.6** New `CoopTargetRing.h/.cpp` (local-only `AActor`, `bReplicates = false`, engine Plane
      mesh, `RingMaterial` as `EditDefaultsOnly` for a `BP_TargetRing` CDO, `Tick` follows
      `GetCurrentTargetActor()` and tints green/red) + `GameConstants.h` `TargetRingRadiusUnits` /
      `TargetRingGroundOffsetUnits` (`Category = "Targeting"`) + spawn in
      `CoopPlayerController::BeginPlay` guarded by `if (TargetRingClass)`.
- [x] **1.7** Re-read all 8 files on disk. No `Build.cs` change needed (`UMG`/`GameplayTags`/
      `SlateCore`/`Engine` cover everything; the click stays BP-wired so no `EnhancedInput` in C++).
      All referenced symbols verified present.

## P2 — One full external rebuild from a closed editor

New `UUserWidget` subclass (`UCoopUnitFrameWidget`) + new `AActor` UCLASS (`ACoopTargetRing`) →
**Live Coding unsafe** (`DECISIONS.md`). Same procedure as `ABILITIES_PROGRESS.md` P4.

- [x] **2.1** Confirmed no `UnrealEditor` process (via `tasklist`).
- [x] **2.2** Ran `Build.bat Unreal_first_GameEditor Win64 Development -project=... -waitmutex`
      (2026-09-03). Re-read all 8 files on disk against the plan first — every referenced symbol
      (`GetHitResultUnderCursor`/`ECC_Visibility`, `UProgressBar::SetPercent`/`SetFillColorAndOpacity`,
      `ACoopPlayerState::GetRole`, `EMatchPhase::Prep`/`HoldTheGate`, engine `Plane` mesh) present.
- [x] **2.3** **Result: Succeeded, exit 0**, ~34s, 15 actions. `CoopUnitFrameWidget.cpp` /
      `CoopTargetRing.cpp` / `CoopPlayerController.cpp` / `CoopRoleSelectWidget.cpp` /
      `GameConstants.cpp` all recompiled; `GameplayTestToolset` relinked (it pulls the main module's
      headers). UHT generated `CoopUnitFrameWidget.generated.h` (4,164 B) + `CoopTargetRing.generated.h`
      (3,233 B) @ 11:12 — verified `Z_Construct_UClass_UCoopUnitFrameWidget` / `_ACoopTargetRing`, the
      `EUnitFrameSource` enum, and the `SelectTargetUnderCursor` UFUNCTION thunk are all in the
      generated `.gen.cpp`s. `UnrealEditor-Unreal_first_Game.dll` relinked → **798,208 B, Sep 3 11:12**
      (was 774,144). `Unreal_first_GameEditor.target` rebuilt. **Zero warnings or errors.**
- [x] **2.4** Editor reopened (PID 39192), `unreal-mcp` reconnected — `list_toolsets` returns the
      full 30+ toolset roster (2026-09-03, resumed session).
- [x] **2.5** `search_subclasses(UMG.UserWidget, "Unit")` → `/Script/Unreal_first_Game.CoopUnitFrameWidget`;
      `search_subclasses(Engine.Actor, "CoopTargetRing")` → `/Script/Unreal_first_Game.CoopTargetRing`.
      Both new UCLASSes visible to the editor. No project Agent Skills registered
      (`AgentSkillToolset.ListSkills` = engine/plugin skills only).

## P3 — Content (all `unreal-mcp`)

- [x] **3.1** `IA_Select` created by duplicating `IA_Shield` (`/Game/Input/Actions/IA_Select`),
      `ValueType = Boolean`. Trimmed the duplicated `triggers` array from `[Pressed, Released]` down
      to just `InputTriggerPressed_0` (a click-select wants one fire on press, not a second on
      release). Saved, `is_dirty == false`, on disk 11:21.
- [x] **3.2** `IMC_Default`: confirmed no `LeftMouseButton` mapping existed (17 entries, none LMB).
      Appended `LeftMouseButton → IA_Select` via a `ProgrammaticToolset` read-append-write (no manual
      transcription of the 17-entry array). Re-read: all 17 prior entries byte-identical incl. every
      `InputModifier*` subobject refPath (W/S/A/Up/Down/... swizzle+negate all intact), new entry #18
      is `LeftMouseButton → IA_Select`. Saved, `is_dirty == false`, on disk 11:22.
- [x] **3.3** `M_TargetRing` (`/Game/Materials/M_TargetRing`) via `MaterialTools`. **`MD_Surface`,
      NOT `MD_UI`** — M_CooldownSweep is `MD_UI` (renders in a UMG widget); this one renders on a
      world `UStaticMeshComponent` plane, so it must stay Surface. `BLEND_Translucent` + `MSM_Unlit`
      + `TwoSided`. Graph: `Distance(TexCoord, (0.5,0.5))` → `d`; `OneMinus(Saturate(Abs(d - 0.42) /
      0.07))` → soft ring band → `MP_Opacity`; `VectorParameter "Color"` (default green
      0.15/0.85/0.20) `.RGB` → `MP_EmissiveColor` (`ACoopTargetRing::Tick` swaps this param
      green↔red via the MID). Wiring re-read node-by-node, `recompile` clean (no shader error),
      `layout_expressions`, saved, `is_dirty == false`. Thumbnail confirms a green ring.
- [x] **3.4** `WBP_UnitFrame` (`/Game/Blueprints/UI/WBP_UnitFrame`, parent `UCoopUnitFrameWidget`).
      Tree: `RootBorder`(Border, root) → `BodyRow`(HorizontalBox) → [`NameCol`(VBox): `NameText`,
      `TypeText`] + [`HealthCol`(VBox): `HealthBarBox`(SizeBox 150×12) → `HealthBar`(ProgressBar),
      `HealthText`] + `StatusText`. `GetWidgets` confirms **all 6** `BindWidgetOptional` names
      (`RootBorder`/`NameText`/`TypeText`/`HealthBar`/`HealthText`/`StatusText`) resolved
      (`bInherited: true`, `inheritedWidgetCount: 6`). Styled: RootBorder dark 65%-alpha bg + 6/3
      padding + Fill/Fill; NameCol slot Fill; fonts 13/9/9/9; HealthBar green fill, percent 1.
      `CompileWidgetBlueprint` = true, saved, `is_dirty == false`, on disk 11:29.
- [x] **3.5** `WBP_TargetFrame` (`/Game/Blueprints/UI/WBP_TargetFrame`, plain `UUserWidget`).
      `RootCanvas`(CanvasPanel, `visibility = HitTestInvisible` — whole subtree non-hit-testable, so
      the cursor falls through to the camera drag, decisions #7/#8) → `FrameBox`(SizeBox, width 300,
      canvas slot anchored top-left, `bAutoSize`, offset (24,20)) → `TargetUnitFrame`
      (`WBP_UnitFrame` instance, `Source` stays `CurrentTarget` default — readback confirms).
      `CompileWidgetBlueprint` = true, saved, on disk 11:31.
- [x] **3.6** `WBP_PartyFrame` (`/Game/Blueprints/UI/WBP_PartyFrame`, plain `UUserWidget`).
      `RootCanvas`(CanvasPanel, `HitTestInvisible`) → `PartyStack`(VerticalBox, canvas slot top-left
      `bAutoSize` offset (24,96)) → **5× `WBP_UnitFrame`** `Member0..Member4`, per-instance
      `Source = PartyMember` + `PartyMemberIndex` 0-4 set **on each placed instance** and read back
      to confirm (the `WBP_ActionBar` P9 CDO-snapshot lesson), 3px inter-row padding.
      `CompileWidgetBlueprint` = true, saved, on disk 11:31.
- [x] **3.7** `BP_TargetRing` (`/Game/Blueprints/BP_TargetRing`, C++ child of `ACoopTargetRing`)
      created; CDO `RingMaterial` → `M_TargetRing` (read back). `BP_PlayerController` CDO (via
      `Default__BP_PlayerController_C`): `TargetFrameWidgetClass` → `WBP_TargetFrame_C`,
      `PartyFrameWidgetClass` → `WBP_PartyFrame_C`, `TargetRingClass` → `BP_TargetRing_C`. Re-read
      confirms all 3 new + all 5 prior CDO refs (MatchTimer/RoleSelect/PrepArenaHUD/ActionBar/
      GameConstants) intact. Both compiled, saved, on disk 11:33.
- [x] **3.8** `BP_PlayerCharacter` EventGraph: mirrored the `IA_Shield` chain exactly via
      `create_node`/`connect_pins` (**not** `write_graph_dsl` — DECISIONS.md, avoids clobbering the
      graph). 5 nodes: `Input|EnhancedActionEvents|IA_Select` event, `Self`, `Pawn|GetController`,
      `CastToCoopPlayerController`, `Targeting|SelectTargetUnderCursor`. Wired
      `Self→GetController.self`, `IA_Select.Triggered→Cast.execute`,
      `GetController.ReturnValue→Cast.Object`, `Cast.then→Call.execute`,
      `Cast.AsCoopPlayerController→Call.self` (`CastFailed` left open, same as IA_Shield).
      Connections re-verified on the live nodes via `get_node_infos`; `read_graph_dsl` still renders
      the event body empty (the known non-default-exec-pin limitation, not a wiring bug).
      `compile_blueprint` clean (no `LogBlueprint` errors after the "Compiling" line), saved, on
      disk 11:36. `find_node_types` didn't list `IA_Select` (stale action DB) but `create_node`
      accepted the type_id anyway.

## P4 — 5-client PIE verification

**Partial solo pass done (2026-09-03, agentic PIE via `unreal-mcp`).** Editor's Play settings
already spawn 5 PIE clients. `bThrottleCPUWhenNotForeground` flipped off for the run, restored
after (DECISIONS.md). RoleSelect → Prep → HoldTheGate all progressed; monsters spawned and downed
players (pre-existing Hold-the-Gate behaviour, unrelated). **No blueprint runtime errors, no
"Accessed None", no cast failures in the log** for any of the new code.

- [~] **4.1** 5-client PIE, roles auto-resolved ✓ (`ResolveRoleSelection: ... 5 player(s)
      auto-assigned`). **Cursor-visible-from-start still needs a human eyeball** (C++ sets
      `SetShowMouseCursor(true)` in `BeginPlay`; not verifiable from the Slate tree).
- [x] **4.2** **Party frame** confirmed via `SlateInspector` snapshot + screenshot on 3 clients:
      top-left, 5 rows, each `Name / ROLE / HP bar / "N / 100"`, **identical values across all 3
      clients** (CONTROL 90, RUNNER 40, DAMAGE 65, SUPPORT 65, TANK 100 at time of capture → reads
      replicated state, not a local snapshot). Local player's own row **visibly yellow-tinted**
      ("this is me"). <5-player collapse not tested (5-client run).
- [x] **4.2b** **Target frame hidden when no target** — the `WBP_TargetFrame` `WBP_UnitFrame`
      instance is present in the tree with its default "Text Block" placeholder text but is **not
      visible on screen** (`RenderOpacity` 0 via the `!Subject` early-return). ✓
- [x] **4.3** **Target a teammate → target frame populates.** VERIFIED (2026-09-03 post-bake pass) via
      the party-row click path: left-clicking the TANK party row on Client 1 flipped the target frame
      from RenderOpacity-0 placeholders to `Theo-…F2B… / TANK / [green bar] / 100 / 100`; repeated on
      Client 0. The **world-click** path (`SelectTargetUnderCursor` cursor trace) still needs a human,
      but its blocker is fixed: teammates were not hittable by the `ECC_Visibility` trace
      (`BP_PlayerCharacter` capsule + mesh both `Visibility → Ignore`; monsters work via
      `BlockAllDynamic`); `ACoopCharacter::BeginPlay()` now blocks `ECC_Visibility` on the capsule —
      baked, and previously runtime-verified (down-trace stops at capsule-top). See the `DECISIONS.md` entry.
- [~] **4.4** Target a monster → "ENEMY", red bar/ring. **NOT runtime-clicked** — party rows are players
      only and a positioned world-click on a monster can't be simulated. It is the trivial `else`
      branch of the same `UCoopUnitFrameWidget::NativeTick` / `ACoopTargetRing::Tick` whose ally path
      is now fully proven (`bIsAlly = Cast<ACoopCharacter>` false → `EnemyHealthColor` / "ENEMY" /
      `EnemyRingColor`). Monster `GetHealthComponent`/`HasStatusTag` were exercised by the Execution
      ability work. Left for the human playtest.
- [x] **4.5** **Live HP / status** — during the run, party rows showed teammates' HP **dropping
      live** and `StatusText` showing **"DOWNED"** as monsters downed them → frames read replicated
      health + tags, not a snapshot. ✓ (target-frame side not exercised — no target was set.)
- [x] **4.6** Remote-client isolation of `CurrentTargetActor`. VERIFIED (2026-09-03 post-bake pass):
      Client 1 setting a target (TANK row) left Client 2's target frame still showing placeholder
      "Text Block" text — completely unaffected. Plain `TWeakObjectPtr`, no `DOREPLIFETIME`, no RPC.
- [ ] **4.7** Right-click-drag orbit with cursor visible / mouse over frames. **NOT verified** —
      needs a human. (All frame roots are `HitTestInvisible` → whole subtree non-hit-testable, so
      the fall-through is structurally correct.)
- [ ] **4.8** RoleSelect buttons still clickable after the cursor-ownership move. **NOT verified** —
      needs a human. (RoleSelect ran and resolved without a stuck-input log.)
- [x] **4.9** **Ring** appears/colours/follows/vanishes. VERIFIED (2026-09-03 post-bake pass) via
      `ObjectTools` on `BP_TargetRing_C_0` in the host PIE world: `bHidden` `true` (no target) →
      `false` after clicking the TANK party row; actor location `(300,0,214)` = TANK pawn `(300,0,302)`
      minus `TargetRingGroundOffsetUnits` 88 (follows + ground-offset); mesh scale `1.8` =
      `TargetRingRadiusUnits` 90 / 50; MID `M_TargetRing` `Color` vector param = `(0.15,0.85,0.20)` =
      `AllyRingColor` green (target is an `ACoopCharacter`). The **vanish-on-clear** path is the same
      `if (!Target) SetActorHiddenInGame(true)` branch that produced the initial `bHidden=true`.
      On-screen *visual* (translucency, band shape, red for a monster) is the human-eyeball carve-out.
- [x] **4.10** Target-goes-Downed graceful degrade — **partly**: a party row whose player went
      Downed kept showing (row stays, `StatusText` = "DOWNED", bar at 0/100). Target-frame
      weak-ptr-null path not exercised (no target set).
- [x] **4.11** Cleanup: `StopPIE` done, `bThrottleCPUWhenNotForeground` restored to `true`, no
      TEMP `UE_LOG`s were added, no `DA_GameConstants` widening was needed, all 9 touched assets
      `is_dirty == false`.
- [x] **4.12** Bake rebuild — **not owed for this feature.** P1's C++ was already baked by a full
      external `Build.bat` at P2 (DLL 798,208 B, editor closed). P3 was pure content (`unreal-mcp`),
      no C++. P4's solo pass produced **zero C++ changes**. The current DLL *is* the baked artifact.
      (If the human P4 playtest surfaces a C++ fix, that fix then needs its own closed-editor
      rebuild — but only then.)

## P5 — `DECISIONS.md` + `CLAUDE.md`

- [x] **5.1** Added `DECISIONS.md` entry **"Cursor + click-to-target, target frame, party frames
      (WoW-style)"** (end of file). Covers: local-only `TWeakObjectPtr` target / no replication /
      §4.2; `GetCurrentTargetActor()` as the future target-driven-ability seam; one reusable
      `UCoopUnitFrameWidget` for both frame uses; `RenderOpacity` toggle + `HitTestInvisible`
      subtree; ground ring = §5's "coloured ring", not the forbidden outline; cursor now
      whole-match, replacing RoleSelect's toggle; build-order deviation; the full-rebuild
      discipline for the 2 new UCLASSes; a "verified so far" / "still to verify (needs human)"
      split.
- [x] **5.2** Updated the "RoleSelect screen feedback is `NativeTick`-driven" entry — struck
      through its "Mouse cursor + input mode" bullet, marked it STALE, and pointed it at 5.1.
- [x] **5.3** **DONE (2026-09-03, user approved).** Added a `CLAUDE.md` §5 bullet after the "UI is
      built in UMG" line: cursor visible whole match + click-to-select (local-only, never replicated);
      top-left target frame + always-on 5-row party frame (name/role/HP bar/status); flat coloured
      ground ring (green ally / red enemy) = §5's "coloured ring", not the forbidden outline; points
      at the `DECISIONS.md` "Cursor + click-to-target, target frame, party frames (WoW-style)" entry;
      flagged reversible like the camera/Mannequin reversals.

---

## Self-review against the constraints

- **Local-only / no desync:** `CurrentTargetActor` is a plain `TWeakObjectPtr`, no
  `UPROPERTY(Replicated)`, no RPC, written only in `SelectTargetUnderCursor()` on the local
  controller. Frames read only already-replicated state. ✓ §4.1/§4.2
- **No Designer bindings:** `NativeTick` + `BindWidgetOptional`. ✓
- **No widget freeze:** `RenderOpacity` toggle, never `Collapsed`/`Hidden` from own tick. ✓ (P9)
- **No post-processing:** ground ring decal only. ✓ §5
- **No plugin / no GAS:** built-in deprojection + UMG. ✓ §3, §4.6
- **Tunables in `DA_GameConstants`:** ring radius/offset; colours hardcoded-cosmetic (the
  `GetColorForPlayerId` precedent). ✓ §10
- **Minimal refactor:** one existing-code change (delete RoleSelect's cursor block, required by
  decision #1, logged). `docs/abilities.md` untouched. ✓ §4.8
- **Rebuild discipline:** new `UUserWidget` + new `AActor` → one full external rebuild at P2. ✓
- **One new widget class** for two frame uses (`EUnitFrameSource`), not three. ✓

---

## Log
(Newest at the bottom. One line per completed step.)

- **Plan written (2026-09-03).** Phase 1 exploration complete. First pass had 6 open decisions;
  user answered "yes to all" and pulled **party frames into v1** (decision #4). Plan revised:
  decisions all LOCKED, one reusable `UCoopUnitFrameWidget` drives both the target frame and the 5
  party rows, `WBP_TargetFrame` + `WBP_PartyFrame` containers added to the file map and P1/P3/P4.

- **P1 done (2026-09-03).** All C++ in one uncommitted batch:
  - `CoopUnitFrameWidget.h/.cpp` (new) — `EUnitFrameSource` enum + the reusable frame;
    `NativeTick` resolves target-actor / party-member pawn, writes 6 `BindWidgetOptional` children,
    gates `RenderOpacity` on phase ∈ {Prep, HoldTheGate} && a valid subject with health, tints the
    local player's own party row. `static HealthOf` / `ActorHasTag` handle `ACoopCharacter` **and**
    `ACoopMonsterCharacter` with an explicit `Cast` each (§4.6).
  - `CoopTargetRing.h/.cpp` (new) — local-only `AActor` (`bReplicates=false`), engine Plane mesh,
    `Tick` snaps to `GetCurrentTargetActor()`'s feet and tints green (ally) / red (enemy); hidden
    when no target. `RingMaterial` is `EditDefaultsOnly` for a `BP_TargetRing` CDO.
  - `CoopPlayerController.h/.cpp` — `CurrentTargetActor` (`TWeakObjectPtr<AActor>`, **not**
    replicated), `GetCurrentTargetActor()`, `SelectTargetUnderCursor()` (`GetHitResultUnderCursor`
    on `ECC_Visibility`, accept player/monster, else clear), `ClearTarget()`; `BeginPlay` now sets
    `bShowMouseCursor` + `FInputModeGameAndUI` for the whole match and creates
    `TargetFrameWidget` / `PartyFrameWidget` + spawns `TargetRing` (guarded on `TargetRingClass`).
  - `CoopRoleSelectWidget.cpp` — deleted the per-phase cursor/input-mode block (cursor is always on
    now); kept the feedback bail.
  - `GameConstants.h` — `TargetRingRadiusUnits` (90) + `TargetRingGroundOffsetUnits` (88).
  No `Build.cs` change. **Next: P2 — user closes the editor, then the full `Build.bat` rebuild**
  (new `UUserWidget` subclass + new `AActor` UCLASS → Live Coding unsafe, DECISIONS.md).

- **P2 rebuild done (2026-09-03).** Resumed session, confirmed no `UnrealEditor` process, re-read all
  8 files on disk (`CoopUnitFrameWidget.h`/`.cpp`, `CoopTargetRing.h`/`.cpp`, `CoopPlayerController.h`/
  `.cpp`, `CoopRoleSelectWidget.cpp`, `GameConstants.h`) against the plan — all consistent, all
  referenced symbols present. Ran full external `Build.bat`: **Succeeded, exit 0**, ~34s, DLL
  relinked to 798,208 B (Sep 3 11:12), no warnings/errors. UHT generated both new UCLASSes'
  `.generated.h`/`.gen.cpp` and the module linked against them (registration symbols verified).
  **Next: P2.4/P2.5 need the user — reopen the editor so `unreal-mcp` reconnects; then P3 (all
  `unreal-mcp`: `IA_Select`, `IMC_Default` LMB mapping, `M_TargetRing`, `WBP_UnitFrame` /
  `WBP_TargetFrame` / `WBP_PartyFrame`, CDO wiring, `BP_PlayerCharacter` EventGraph).**

- **P2.4/P2.5 + P3 done (2026-09-03, resumed session, editor already reopened — PID 39192).**
  `unreal-mcp` reconnected (full toolset roster). `search_subclasses` confirmed both new UCLASSes
  visible. Then all P3 content via `unreal-mcp`, each asset compiled + saved + `is_dirty == false` +
  mtime-verified on disk:
  - **`IA_Select`** — duplicated `IA_Shield`, trimmed `triggers` to just `Pressed`.
  - **`IMC_Default`** — appended `LeftMouseButton → IA_Select` via a read-append-write script; all
    17 prior mappings (incl. every input-modifier subobject) byte-identical on re-read.
  - **`M_TargetRing`** — `MD_Surface` (not `MD_UI` — it's a world mesh, not a widget),
    `BLEND_Translucent`/`MSM_Unlit`/`TwoSided`. `OneMinus(Saturate(Abs(Distance(UV,(.5,.5)) - .42)
    / .07))` → soft ring band → Opacity; `VectorParameter "Color"` → EmissiveColor. Recompiled
    clean; thumbnail shows a green ring.
  - **`WBP_UnitFrame`** (parent `UCoopUnitFrameWidget`) — Border→HBox→[NameText/TypeText] +
    [SizeBox→HealthBar / HealthText] + StatusText. `GetWidgets` confirms all 6 `BindWidgetOptional`
    names resolved (`inheritedWidgetCount: 6`).
  - **`WBP_TargetFrame`** / **`WBP_PartyFrame`** (plain `UUserWidget`) — CanvasPanel root
    `HitTestInvisible` (whole subtree non-hit-testable → cursor falls through, decisions #7/#8),
    top-left anchored. TargetFrame holds 1 `WBP_UnitFrame` (`Source` = `CurrentTarget` default);
    PartyFrame holds 5, per-instance `Source = PartyMember` + `PartyMemberIndex` 0-4 set on each
    placed instance and read back.
  - **`BP_TargetRing`** (child of `ACoopTargetRing`) — CDO `RingMaterial` → `M_TargetRing`.
  - **`BP_PlayerController` CDO** — `TargetFrameWidgetClass`/`PartyFrameWidgetClass`/`TargetRingClass`
    wired; all 5 prior refs intact.
  - **`BP_PlayerCharacter` EventGraph** — `IA_Select` → Cast → `SelectTargetUnderCursor` chain
    mirroring `IA_Shield` exactly, built with `create_node`/`connect_pins`, connections re-verified
    on live nodes, clean compile.
  **Next: P4 — 5-client PIE verification. Solo checks 4.1-4.10 via `unreal-mcp`; the real 5-friend
  test needs the user. Then P4.12 bake rebuild (editor closed, `Build.bat`) for the P1 C++ deltas,
  then P5 `DECISIONS.md` + `CLAUDE.md` flag.**

- **P4 solo pass + P5 docs done (2026-09-03, same resumed session).** Ran 5-client agentic PIE
  (`bThrottleCPUWhenNotForeground` off for the run, restored after). Progressed RoleSelect → Prep →
  HoldTheGate; roles auto-assigned. **No blueprint runtime errors / "Accessed None" / cast failures
  for any new code.** Via `SlateInspector` snapshot + screenshot on 3 clients: **party frame** = 5
  rows `Name / ROLE / HP bar / "N / 100"`, values **identical across clients** (→ reads replicated
  state), local player's row **yellow-tinted**; HP **dropped live** + `StatusText` showed **"DOWNED"**
  as monsters downed players; **target frame invisible** (`RenderOpacity` 0) with no target. Not
  verifiable via MCP (a positioned world-click can't be simulated): cursor-visible-from-start, actual
  LMB targeting, ground ring, camera-drag-with-cursor, RoleSelect clickability — all handed to the
  user. **P4.12 bake rebuild is NOT owed** — P1's C++ was baked by P2's full external `Build.bat`
  (editor closed); P3 was pure content; P4 produced zero C++ deltas; the current DLL is the artifact.
  **P5:** `DECISIONS.md` gained the "Cursor + click-to-target, target frame, party frames (WoW-style)"
  entry; the stale "Mouse cursor + input mode" bullet in the RoleSelect entry struck through +
  redirected. `CLAUDE.md` §5 left untouched — one-line addition drafted in P5.3 for the user to
  apply.
  **Next: user runs one real playtest (checklist in P4 above / the `DECISIONS.md` entry's "still to
  verify" list); then applies the P5.3 §5 line if wanted; then commit.**

- **BUGFIX — allies not click-targetable (2026-09-03, same session, user's first report:**
  *"I should be able to click on and target allies ... same as enemies"*). Systematic debugging:
  read `BP_PlayerCharacter`'s serialized collision — `CollisionCylinder` = `Custom` profile with
  `Visibility → ECR_Ignore`, `CharacterMesh0` = `QueryOnly` ignoring every trace channel.
  `SelectTargetUnderCursor`'s `GetHitResultUnderCursor(ECC_Visibility)` therefore never hit a
  teammate — trace passed through to the floor → `Cast<ACoopCharacter>` failed → `ClearTarget()`.
  Monsters work only because `ACoopMonsterCharacter`'s mesh uses `BlockAllDynamic`. **Fix:**
  `ACoopCharacter::BeginPlay()` +
  `GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block)` (+ `#include
  "Components/CapsuleComponent.h"`). In `BeginPlay` not the ctor — the BP's serialized override
  would shadow a ctor setting; `BeginPlay` also covers dev dummy pawns (`DefaultPawnClass` =
  `BP_PlayerCharacter`). Spring-arm camera probes `ECC_Camera` (still ignored) → no regression.
  **`LiveCodingToolset.CompileLiveCoding` → "Live coding succeeded", no errors** (pure
  function-body change, the safe LC category). Verified in PIE: all 5 live characters' capsules
  now block `ECC_Visibility` (`Ignore` override cleared → default Block), and a straight-down
  `trace_world` at a character stops at capsule-top (origin + 90). PIE stopped, throttle restored.
  **Owes one closed-editor `Build.bat`** to bake the live patch permanently (logged in the
  `DECISIONS.md` entry's rebuild-discipline paragraph). Actual click still needs the user's
  playtest, but the confirmed blocker is gone.

- **PLAYTEST FEEDBACK ROUND 2 (2026-09-03): cursor-hide + party-row-click.** Brainstormed (bounded),
  design approved, implemented:
  - **Cursor hides while RMB held.** `ACoopOrbitCamera::Tick` edge-detects the right mouse button
    (new `bWasOrbiting` member in the `.h`) and calls `Controller->SetShowMouseCursor(!bOrbiting)`
    on the two edges only. Co-located with the orbit's own RMB read; stays the only writer of
    `bShowMouseCursor` besides `BeginPlay`.
  - **Left-click a party row → target that teammate.** `UCoopUnitFrameWidget::NativeOnMouseButtonDown`
    (new virtual override): `Source == PartyMember` + left button + live subject →
    `ACoopPlayerController::SetCurrentTarget(subjectPawn)` → `Handled`; RMB / everything else →
    `Unhandled` (camera drag still falls through the party frame). `NativeTick` restructured: rows
    are `SetVisibility(Visible)` only when `Source == PartyMember && bHasSubject`, else
    `HitTestInvisible` (target frame always `HitTestInvisible`). New
    `ACoopPlayerController::SetCurrentTarget(AActor*)` — direct setter, same accept-filter /
    local-only contract as `SelectTargetUnderCursor`, null-arg is a no-op.
  - **`WBP_PartyFrame` `RootCanvas`: `HitTestInvisible` → `SelfHitTestInvisible`** (via `unreal-mcp`;
    `PartyStack` VBox was already `SelfHitTestInvisible`). Compiled, saved, `is_dirty == false`, on
    disk 13:14. **This is the only part not needing the rebuild.**
  Revises plan decisions #4 / #7 (logged in `DECISIONS.md`). **Next: one closed-editor `Build.bat`
  bakes this round + the collision fix together, then playtest.**

- **Bake rebuild done (2026-09-03).** Resumed session; the editor was still open (PID 39192) so
  asked the user to close it, then confirmed no `UnrealEditor` process. Re-read all 3 deltas on
  disk: `CoopCharacter.cpp` (`#include "Components/CapsuleComponent.h"` + `BeginPlay` capsule
  `SetCollisionResponseToChannel(ECC_Visibility, ECR_Block)`), `CoopOrbitCamera.h`/`.cpp`
  (`bWasOrbiting` + RMB-edge `SetShowMouseCursor` toggle), `CoopUnitFrameWidget.h`/`.cpp`
  (`#include "InputCoreTypes.h"`, `NativeOnMouseButtonDown` override, `NativeTick`
  Visible-vs-HitTestInvisible restructure) + `CoopPlayerController` `SetCurrentTarget(AActor*)` —
  all consistent. Ran full external `Build.bat`: **Succeeded, exit 0**, ~81s, 10 actions, all 6
  changed `.cpp`s recompiled, UHT wrote **0 generated files** (pure C++, no reflection change), DLL
  relinked → **799,744 B, Sep 3 13:50** (was 798,208). No warnings/errors. Also updated the
  `DECISIONS.md` entry's rebuild-discipline paragraph — the owed bake is now done. **Next: user
  reopens the editor, then the human playtest checklist; then P5.3 / commit.**

- **Post-bake PIE verification pass (2026-09-03).** Editor was already reopened (PID 11684 @ 13:54,
  after the 13:50 bake — confirmed running the 799,744 B DLL); `unreal-mcp` reconnected (full toolset
  roster); `search_subclasses` confirmed `CoopUnitFrameWidget` + `CoopTargetRing` loaded. Ran a fresh
  solo 5-client agentic PIE on the **baked** artifact (`bThrottleCPUWhenNotForeground` off for the
  run, restored to `true` after; StopPIE clean, no stray `UnrealEditor`/`CrashReportClient` procs):
  - RoleSelect (auto-resolved) → Prep → HoldTheGate progressed; monsters spawned/downed/retargeted as
    normal. **Whole-session log: zero `Error:` / `Accessed None` / script warnings / exceptions** —
    only the 3 pre-existing benign startup lines (GameFeatureData ×2, MCP `server/discover`).
  - Party frame + "target frame hidden with no target" re-confirmed on 3 clients (host w1 / w5 / w6),
    live HP + "DOWNED".
  - **4.3 (ally) + party-row-click (revised decision #4): VERIFIED end-to-end.** Left-clicked the
    TANK party row on Client 1 → target frame flipped from RenderOpacity-0 "Text Block" placeholders
    to `Theo-…F2B… / TANK / [green bar] / 100 / 100`. Repeated on Client 0.
  - **4.6 (client isolation): VERIFIED.** Client 1's target left Client 2's target frame untouched
    (still placeholder text).
  - **4.9 (ground ring): VERIFIED** via `ObjectTools`/`ActorTools` on `BP_TargetRing_C_0` (host PIE
    world): `bHidden` `true`→`false` on target set; location `(300,0,214)` = TANK pawn `(300,0,302)`
    − `TargetRingGroundOffsetUnits` 88 (follows + ground offset); scale `1.8` = `TargetRingRadiusUnits`
    90 / 50; MID `M_TargetRing` `Color` param = `(0.15,0.85,0.20)` = `AllyRingColor` green; CDO
    `ringMaterial` ref intact.
  - Tooling note logged: SlateInspector `Click` on the **full editor viewport** (host) needs
    window-focus + a viewport pre-click first; the dedicated **preview windows** (Clients 1-4) take
    clicks directly.
  - **Still needs a human** (pure visual): cursor visible from match start; cursor hides while RMB
    held + reappears on release; target a **monster** → "ENEMY"/red; RMB-drag camera with the mouse
    over the party frame; a **world-click** on a character (`SelectTargetUnderCursor` + capsule fix,
    trace-verified earlier); RoleSelect buttons still clickable.
  **Next: the short human playtest for those bullets; then P5.3 `CLAUDE.md` §5 line if wanted; then
  commit. Nothing owed a rebuild.**

- **P5.3 §5 line added (2026-09-03, user approved).** New `CLAUDE.md` §5 bullet after "UI is built in
  UMG": cursor visible whole match + click-to-select (local-only, never replicated); top-left target
  frame + always-on 5-row party frame; flat coloured ground ring (green ally / red enemy) = §5's
  "coloured ring", not the forbidden outline; points at the `DECISIONS.md` entry; flagged reversible.

- **Playtest-feedback round 3 (2026-09-03): cursor position not restored on RMB release.** User ran
  their own PIE playtest and hit it: releasing right-click leaves the cursor wherever the drag ended,
  not where it started. Diagnosed: round 2's `SetShowMouseCursor(false)` hides the cursor but with
  `GameAndUI` + `DoNotLock` the OS cursor keeps moving with the mouse while hidden — the round-2
  code comment's "keeps its screen position while hidden" claim was simply wrong. **Fix, contained to
  `ACoopOrbitCamera`:** `.h` gains `bHasSavedCursorPos` + `SavedCursorX/Y`; `Tick`'s RMB edge handler
  now calls `GetMousePosition` on the press edge and `SetMouseLocation(saved)` on the release edge
  (before re-showing the cursor). Corrected the code comment + the `DECISIONS.md` "cursor hides while
  right-click is held" bullet + its rebuild-discipline paragraph. Did **not** add a per-tick cursor
  lock — a big sweep can still push the hidden cursor to the viewport edge and stall the orbit;
  logged as a known limitation to fix only if a playtest finds it annoying.
  **Owes one closed-editor `Build.bat`** (plain-member layout change on an existing non-widget
  `AActor` — same as round 2's `bWasOrbiting`; PIE was running when the report came in so no Live
  Coding attempt). **Next: user stops PIE + closes editor → `Build.bat` → reopen → resume playtest.**

- **Round-3 bake rebuild done (2026-09-03).** Resumed session, confirmed no `UnrealEditor` process,
  re-read the `CoopOrbitCamera.h`/`.cpp` diff — `.h` gains `bHasSavedCursorPos` + `SavedCursorX/Y`
  (plain, alongside round 2's `bWasOrbiting`); `Tick`'s RMB press edge does
  `bHasSavedCursorPos = Controller->GetMousePosition(SavedCursorX, SavedCursorY)` then
  `SetShowMouseCursor(false)`, and the release edge does
  `SetMouseLocation(RoundToInt(SavedCursorX), RoundToInt(SavedCursorY))` (guarded on
  `bHasSavedCursorPos`) then `SetShowMouseCursor(true)`. All plain C++ on an existing `AActor`. Ran
  full external `Build.bat`: **Succeeded, exit 0**, ~155s, 6 actions (`CoopOrbitCamera.cpp` +
  `CoopPlayerController.cpp` — the latter includes `CoopOrbitCamera.h` — + module + link), UHT wrote
  **0 generated files**, DLL relinked → **799,744 B, Sep 3 14:25**. No warnings/errors. Updated the
  `DECISIONS.md` rebuild-discipline paragraph. **Next: user reopens the editor, resumes the human
  playtest; then commit. Nothing is owed a rebuild.**
