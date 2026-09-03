# Cursor Targeting, Target Frame & Party Frames (WoW-style) — Plan & Progress Tracker

Resumable checklist for a mouse-cursor unit-selection system: a visible cursor, click a teammate or
an enemy to "target" it, show that unit's frame (name, role, HP bar, status) in the top-left, and
always show a stacked party frame for all five players — like WoW's target frame + party/raid
frames. This doc **is** the plan — there is no separate spec file (same convention as
`ABILITIES_PROGRESS.md`).

> **STATUS (2026-09-03): PLANNED, not started. All design decisions RESOLVED (2026-09-03, user:
> "yes to all", with party frames pulled into v1).** Ready to build. Start at the first unchecked
> `- [ ]` in P1. Phases: P1 C++ (one batch) → P2 one full rebuild → P3 content via `unreal-mcp` →
> P4 5-client PIE → P5 `DECISIONS.md`.

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

- [ ] **1.1** `CoopPlayerController`: `CurrentTargetActor` + `GetCurrentTargetActor()` +
      `SelectTargetUnderCursor()` (trace `ECC_Visibility`, accept `ACoopCharacter` /
      `ACoopMonsterCharacter`, empty → clear) + `ClearTarget()`. Local-only, commented, no RPC.
- [ ] **1.2** `CoopPlayerController::BeginPlay`: cursor on + `FInputModeGameAndUI` inside the
      `IsLocalController()` block.
- [ ] **1.3** `CoopPlayerController`: `TargetFrameWidgetClass`/`TargetFrameWidget` +
      `PartyFrameWidgetClass`/`PartyFrameWidget` + create-and-add-to-viewport; set the target
      instance's `Source` after create.
- [ ] **1.4** New `CoopUnitFrameWidget.h/.cpp` — `EUnitFrameSource`, per-instance fields,
      `NativeTick` subject resolution + child writes, `RenderOpacity` gating, "this is me" tint,
      the three `static` helpers.
- [ ] **1.5** `CoopRoleSelectWidget.cpp` — remove the cursor block, keep the feedback bail, fix the
      comment.
- [ ] **1.6** New `CoopTargetRing.h/.cpp` + `GameConstants.h` `Targeting` fields + spawn in
      `CoopPlayerController::BeginPlay`.
- [ ] **1.7** Read every changed/new file on disk against this plan — confirm each referenced
      symbol exists (`GetHitResultUnderCursor`, `ECC_Visibility`, `FInputModeGameAndUI`,
      `UProgressBar`, `PlayerArray`, `EMatchPhase`, the health/tag/role getters).

## P2 — One full external rebuild from a closed editor

New `UUserWidget` subclass (`UCoopUnitFrameWidget`) + new `AActor` UCLASS (`ACoopTargetRing`) →
**Live Coding unsafe** (`DECISIONS.md`). Same procedure as `ABILITIES_PROGRESS.md` P4.

- [ ] **2.1** Confirm no `UnrealEditor` process.
- [ ] **2.2** `Build.bat Unreal_first_GameEditor Win64 Development -project=... -waitmutex`.
- [ ] **2.3** Result: Succeeded, exit 0, zero warnings/errors. DLL relinked (note size/time).
- [ ] **2.4** Reopen editor; `unreal-mcp` reconnects.
- [ ] **2.5** `search_subclasses` (`UserWidget`, "Unit") → `CoopUnitFrameWidget`; (`Actor`, "Ring")
      → `CoopTargetRing`. `BP_PlayerController` CDO keeps its existing widget-class refs.

## P3 — Content (all `unreal-mcp`)

- [ ] **3.1** `IA_Select` (duplicate `IA_Shield`); verify via read-back.
- [ ] **3.2** `IMC_Default`: confirm LMB unbound, append `LeftMouseButton → IA_Select`, re-verify
      all prior mappings intact.
- [ ] **3.3** `M_TargetRing` via `MaterialTools` — unlit, translucent, one `Color` param. Save +
      `is_dirty == false` + on-disk mtime.
- [ ] **3.4** `WBP_UnitFrame` (parent `UCoopUnitFrameWidget`) — build the tree via
      `UMGToolSet.AddWidget` (`list_properties` first on every widget/slot); all children as
      variables. `CompileWidgetBlueprint` clean. Save + verify on disk.
- [ ] **3.5** `WBP_TargetFrame` (plain `UUserWidget`) — top-left `SizeBox` → one `WBP_UnitFrame`,
      `Source = CurrentTarget`. Compile + save.
- [ ] **3.6** `WBP_PartyFrame` (plain `UUserWidget`) — top-left `VerticalBox` → 5 `WBP_UnitFrame`,
      per-instance `Source = PartyMember` + `PartyMemberIndex` 0-4 (set on each placed instance
      **after** any CDO wiring — the `WBP_ActionBar` P9 `gameConstants`-snapshot lesson). Compile +
      save.
- [ ] **3.7** `BP_PlayerController` CDO: `targetFrameWidgetClass` → `WBP_TargetFrame_C`,
      `partyFrameWidgetClass` → `WBP_PartyFrame_C`, ring class ref. `compile_blueprint`, re-verify
      other CDO refs survived, save.
- [ ] **3.8** `BP_PlayerCharacter` EventGraph: `IA_Select` event → `Get Controller` → `Cast To
      BP_PlayerController` → `SelectTargetUnderCursor`. `compile_blueprint` clean, `read_graph_dsl`
      to confirm, save.

## P4 — 5-client PIE verification

- [ ] **4.1** 5-client PIE, roles auto-resolve. Cursor visible from match start on every client.
- [ ] **4.2** **Party frame**: top-left, 5 rows, each showing a player's name + role + full green
      HP bar + "100 / 100". The local player's own row is visibly tinted. In a <5-player run the
      empty rows collapse (test via a dev-mode partial roster if practical, else note).
- [ ] **4.3** **Target a teammate** (click their character): target frame shows that player's name,
      role, green HP bar, HP number. Click another → updates. Click empty ground → clears.
- [ ] **4.4** Scene-skip to HoldTheGate. **Target a monster**: frame shows "ENEMY" (no role), red
      HP bar, HP number.
- [ ] **4.5** **Live HP**: monster damages a targeted/partied teammate (or `ExecCommand
      ApplyTestDamage`) → that unit's bar drops **live** in both the target frame and its party row
      — proves the frames read replicated health, not a snapshot. Shield/Fortress/Downed show in
      `StatusText`.
- [ ] **4.6** **On a remote client** (not host): party + target frames correct; the client's clicks
      do **not** change the host's target frame (spot-check `CurrentTargetActor` is not
      `UPROPERTY(Replicated)` / has no `DOREPLIFETIME`).
- [ ] **4.7** Right-click-drag orbit still rotates the camera with the cursor visible and the mouse
      over the frames (all `HitTestInvisible`).
- [ ] **4.8** RoleSelect buttons still clickable; no stuck cursor / input-mode glitch at phase edges
      (decision #1's deleted block).
- [ ] **4.9** **Ring**: appears under the targeted unit, correct colour, follows it, vanishes on
      clear/death.
- [ ] **4.10** Target a unit, then it dies (monster) / goes Downed (teammate) → target frame
      degrades gracefully (weak-ptr null → `RenderOpacity` 0, or "DOWNED" for the teammate — pick
      in 1.4 and verify); its party row keeps showing (Downed state) or greys out.
- [ ] **4.11** Cleanup: `StopPIE`, restore any widened `DA_GameConstants`, remove TEMP `UE_LOG`s,
      one clean Live Coding compile, all `is_dirty == false`.
- [ ] **4.12** Bake rebuild: full external `Build.bat` from a closed editor for the P4 C++ deltas
      (the "still owed" step every prior feature has).

## P5 — `DECISIONS.md` + `CLAUDE.md`

- [ ] **5.1** New `DECISIONS.md` entry "Cursor + click-to-target, target frame, party frames
      (WoW-style)": net-new interaction (§5 covers camera + overhead bars, not a cursor/click);
      local-only, no replication, display-only (§4.2), abilities unchanged; ground ring not an
      outline (§5 no post-processing); cursor now always-on for the whole match, replacing
      RoleSelect's per-phase toggle; deviation from build order (user-requested, like monster
      combat / action bar / status badge); future seam for target-driven abilities is
      `GetCurrentTargetActor()`.
- [ ] **5.2** Update `DECISIONS.md` "RoleSelect screen feedback is `NativeTick`-driven" — its
      "Mouse cursor + input mode" bullet is now stale (block deleted); point it at 5.1.
- [ ] **5.3** Flag to the user a one-line `CLAUDE.md` §5 addition (cursor + click-to-target + party
      frames now exist, see `DECISIONS.md`) — same treatment as the camera-follow / Mannequin
      reversals; don't edit §5 unprompted.

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
  Next: P1 (all C++ in one batch), then P2 (one rebuild).
