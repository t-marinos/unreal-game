# Ability UX pass — cooldown toast + action-bar hover tooltips — Plan & Progress Tracker

Resumable checklist for two small ability-UX features, agreed in chat 2026-09-03 (same
convention as `ABILITIES_PROGRESS.md` / `cursor_progress.md` — this doc **is** the plan, no
separate spec file):

1. **"Ability not ready" toast on cooldown.** Pressing an ability key while that ability is still
   on cooldown shows the centre-screen `UCoopToastWidget` message **"Ability not ready"** and
   sends no RPC. Applies to **all 7** abilities (Shield / Speed / Dash / Stabilize / Execution /
   Armor Break / Overload).
2. **Hover tooltips above the action bar.** Hovering the mouse over an action-bar tile shows a
   LoL/WoW-style panel **above the bar** with the ability **name + one-sentence description**.
   **All** slots, greyed ("coming later") ones included.

> **STATUS (2026-09-03): FEATURE COMPLETE — main feature + Follow-up F1 both fully done and
> agentic-PIE verified.** P1–P5 done (cooldown toast + client role gate + hover tooltips). **F1
> (prep-arena ability cards deleted) F1.1–F1.7 all DONE:** cards stripped from `WBP_PrepArenaHUD`,
> `WBP_AbilityCard.uasset` + `CoopAbilityCardWidget.h/.cpp` gone, closed-editor `Build.bat`
> (DLL 840,704 → 822,784 B), reopen-verify (compile clean, no missing class, PIE shows only the
> countdown in Prep + HoldTheGate, no cards), `DECISIONS.md` extended.
>
> **Two items owed to a human eyeball (both low-risk, no code change needed — noted in `DECISIONS.md`):**
> 1. **P4.5** — right-click-drag camera orbit / click-to-target *while the cursor sits over the action
>    bar*. MCP can't do a real drag; the hover tests prove the bar stays `HitTestInvisible`.
> 2. **Hover tooltip live re-check** — the `GetMousePositionOnPlatform()` geometry poll reads the OS
>    cursor, which `SlateInspector.Hover` doesn't warp, so the tooltip can't be triggered through MCP.
>    Verified structurally (panel widgets present + bound, slots tick with correct names, F1 changed
>    only comments) and functionally back in P4.4. Just move the real mouse over a tile.
>
> **`WBP_PrepArenaHUD` phase-gate still not wired** (out of F1 scope, flagged in `DECISIONS.md`): the
> now-bare countdown panel lingers into HoldTheGate. Fix = a Designer "Bind" of
> `UCoopPrepCountdownWidget::GetPrepArenaVisibility()` on the panel root, or a C++ `NativeTick`
> self-gate. A bare "0" is far less intrusive than the old cards were.
>
> **P2.0 non-issue:** the reopened editor loaded `DA_GameConstants` with `brokenDurationSeconds` 6 /
> `armorBreakCooldownSeconds` 10 already correct on disk (`is_dirty` false) — the P4.4-attempt widen
> never reached disk. Editor throttle confirmed restored to `true`/`true`.
>
> Deviation from the plan sketch: the slot-widget cursor read uses
> `UWidgetLayoutLibrary::GetMousePositionOnPlatform()` (UMG, already a dep) instead of
> `FSlateApplication::Get().GetCursorPos()` — the latter needs the `Slate` module which this project's
> `Build.cs` does not list. **No `Build.cs` change.**

**If interrupted, a new session should:**
1. Read this file top to bottom, find the first unchecked `- [ ]`.
2. Read `DECISIONS.md`'s entries: "WoW-style action bar…", "Target-required abilities need a
   click-selected target", "Live Coding must not be used to add a new UCLASS…", "No `unreal-mcp`
   tool can set a UMG Designer Bind Function…". This plan reuses every pattern in those.
3. Continue from the first unchecked box. **P1 is all C++ in one batch — do not rebuild between
   its steps. P2 is the single rebuild.**

---

## Design decisions — LOCKED (agreed 2026-09-03, do not re-litigate)

### Feature 1 — cooldown toast

- **All 7 abilities**, text **"Ability not ready"** (flat, no seconds countdown — matches "Please
  choose a target"). New `NSLOCTEXT("CoopAbilities", "AbilityNotReady", "Ability not ready")`.
- **Client-side gate only, in the `Activate*()` wrappers on `ACoopPlayerController`** — the same
  place and shape as the existing target-null check. Reads the owning pawn's
  `Get<Ability>CooldownEndServerTime()` (already replicated `COND_OwnerOnly`, the action-bar sweep
  reads the same field) vs. `GetServerWorldTimeSeconds()`. **Every `Server_Activate*_Implementation`
  keeps its own authoritative cooldown re-check inside the `Resolve*`/`Apply*` namespace call —
  unchanged.** This is pre-RPC UI feedback (CLAUDE.md §4.2), not gameplay prediction.
- **New private helper** `bool ACoopPlayerController::IsAbilityReady(float CooldownEndServerTime)
  const` — `Now >= end`. Plain method, **not** a `UFUNCTION` (called only within the .cpp).
- **Order of checks in the 3 target-required wrappers:** role gate → cooldown gate → target gate →
  RPC. The cooldown toast fires *before* "Please choose a target" (a not-ready ability is the more
  fundamental blocker).
- **Client-side ROLE gate added to the 3 target-required wrappers only** (Execution / Armor Break
  / Overload): `GetPlayerState<ACoopPlayerState>()->GetRole() != <thisAbilityRole>` → silent
  `return` (no toast, no RPC). Fixes a pre-existing wart: `IA_Execution` shares the `Q` key with
  the other four first-abilities, so today a Tank/Runner/etc. pressing `Q` for *their* ability
  flashes a spurious "Please choose a target" from the Execution chain. The 4 auto-target wrappers
  (Shield/Speed/Dash/Stabilize) get **no** client role gate — a wrong-role player's cooldown field
  for that ability is always `-1` so the toast is naturally suppressed, and their existing
  server-side role gate is the real guard (unchanged behaviour, just now also cooldown-checked).

### Feature 2 — hover tooltips

- **Hover detection is a pure geometry poll, NOT Slate hit-testing.** The action bar and every slot
  stay `HitTestInvisible` (the "WoW-style action bar" decision's core guarantee — cursor falls
  through to the right-click camera drag and click-to-target). Each `UCoopAbilitySlotWidget::
  NativeTick` sets `bCursorOver = MyGeometry.IsUnderLocation(FSlateApplication::Get().GetCursorPos())`
  (both absolute desktop space). `UCoopActionBarWidget::NativeTick` reads its 3 slots and drives one
  shared tooltip panel. **No input-routing change anywhere** → zero risk to the camera drag /
  targeting. Same `NativeTick`-poll idiom as RoleSelect / the party frames.
- **Tooltip content = name + description.** Descriptions are **copied** into
  `CoopAbilitySlotWidget.cpp`'s kit table (a new `FText Description` on `FCoopAbilitySlotInfo`, ~11
  strings lifted verbatim from `CoopAbilityCardWidget.cpp`'s table). This continues that file's
  existing, deliberate "~11-string duplication, not a refactor of a verified widget (CLAUDE.md §4.8,
  §1)" note — it already duplicates the names for the same reason.
- **All slots**, greyed included — their descriptions already exist and the tooltip usefully signals
  "this is coming". A slot index past the role's kit shows nothing (`HasAbilityEntry()` false).
- **The tooltip panel lives inside `WBP_ActionBar`** as a child of `RootCanvas`, anchored
  bottom-centre, offset above `SlotBox`. `HitTestInvisible`. **Show/hide via `RenderOpacity` only**
  (never its own `Visibility` — the P9 self-hide-freeze applies; the bar's own comment explains it).
- **`NativeTick` + `BindWidgetOptional`, no Designer "Bind Function" bindings** — same as every
  widget in the project (`unreal-mcp` can't author them, `DECISIONS.md`).
- **No new tunables** — nothing here is a gameplay number (CLAUDE.md §10 is about gameplay values;
  colours / offsets / the tooltip's position stay hardcoded-cosmetic, the `GetColorForPlayerId`
  precedent).

### Rebuild discipline

- **Feature 1**: function bodies + one new **plain** (non-reflected) helper method + one
  `NSLOCTEXT` → Live-Coding-safe on its own.
- **Feature 2**: `UCoopActionBarWidget` gains **6 new `UPROPERTY(meta=(BindWidgetOptional))`
  members** on a `UUserWidget` subclass → **Live Coding is unsafe** (`DECISIONS.md` "Live Coding
  must not be used to add a new UCLASS…" — new reflected members on a widget class fall on the
  cautious side of the addendum). `UCoopAbilitySlotWidget` adds only plain methods + a plain bool +
  an anon-struct field → no reflection change there.
- → **One full external `Build.bat` from a closed editor covers both** (P2). All C++ lands in P1.

---

## File map

**C++ — modified:**
- `Source/Unreal_first_Game/Core/CoopPlayerController.h` — declare `bool IsAbilityReady(float) const`
  in `private:`.
- `Source/Unreal_first_Game/Core/CoopPlayerController.cpp` — define `IsAbilityReady`; add the
  cooldown gate to all 7 `Activate*()` wrappers and the role gate to the 3 target-required ones;
  `+#include "GameFramework/GameStateBase.h"`.
- `Source/Unreal_first_Game/Core/CoopAbilitySlotWidget.h` — `public`: `GetAbilityName()` /
  `GetAbilityDescription()` / `HasAbilityEntry()` / `IsCursorOver()`. `private`: `bool bCursorOver`.
- `Source/Unreal_first_Game/Core/CoopAbilitySlotWidget.cpp` — `FText Description` on
  `FCoopAbilitySlotInfo` + the 11 description strings in the 5 kit tables; `bCursorOver` poll in
  `NativeTick`; the 4 getter definitions; `+#include "Framework/Application/SlateApplication.h"`.
- `Source/Unreal_first_Game/Core/CoopActionBarWidget.h` — 6 `BindWidgetOptional` members
  (`Slot0/1/2` as `UCoopAbilitySlotWidget*`, `TooltipRoot` as `UWidget*`, `TooltipNameText` /
  `TooltipDescText` as `UTextBlock*`); forward decls.
- `Source/Unreal_first_Game/Core/CoopActionBarWidget.cpp` — tooltip drive at the end of
  `NativeTick`; `+#include "Core/CoopAbilitySlotWidget.h"`, `+#include "Components/TextBlock.h"`.

**Content — modified (all via `unreal-mcp`, P3):**
- `/Game/Blueprints/UI/WBP_ActionBar` — add `TooltipRoot` (Border, dark bg, `HitTestInvisible`) →
  `VerticalBox` → `TooltipNameText` (TextBlock, bold) + `TooltipDescText` (TextBlock, wrapped,
  smaller) as a child of `RootCanvas`, canvas slot anchored `(0.5, 1)` / alignment `(0.5, 1)` /
  offset top ≈ `-100` (above the bar) / `bAutoSize`. `ToggleWidgetAsVariable true` on the 3.
  `Slot0/1/2` already exist as WBP variables → the new C++ `BindWidgetOptional`s bind by name
  automatically. `CompileWidgetBlueprint` → `true`, `LogBlueprint` clean. Save.

**Docs — modified (P5):**
- `DECISIONS.md` — new entry "Ability-bar UX: cooldown toast + hover tooltips".

---

## P1 — All C++ (one batch; do NOT rebuild between steps — P2 is the single rebuild)

> **✅ P1 COMPLETE (2026-09-03) — all edits on disk, self-reviewed, not yet built. Skip to P2.**
> The per-step boxes below are the reference of what was written.

### P1.1 — `CoopPlayerController` cooldown + role gates

- [ ] `CoopPlayerController.h` — in `private:` (near `GetCurrentTargetActor` / the other helpers),
      add:
      ```cpp
      // Client-side "is this ability off cooldown?" check for the pre-RPC "Ability not ready"
      // toast. Reads the owning pawn's COND_OwnerOnly-replicated *CooldownEndServerTime vs.
      // GetServerWorldTimeSeconds(); the server still enforces the cooldown authoritatively.
      bool IsAbilityReady(float CooldownEndServerTime) const;
      ```
- [ ] `CoopPlayerController.cpp` — `+#include "GameFramework/GameStateBase.h"`.
- [ ] `CoopPlayerController.cpp` — define, near `ShowToast`:
      ```cpp
      bool ACoopPlayerController::IsAbilityReady(float CooldownEndServerTime) const
      {
      	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
      	const float Now = GS ? GS->GetServerWorldTimeSeconds() : 0.0f;
      	return Now >= CooldownEndServerTime;
      }
      ```
- [ ] `CoopPlayerController.cpp` — **each of the 4 auto wrappers** (`ActivateShield`,
      `ActivateStabilize`, `ActivateSpeed`, `ActivateDash`): before the `Server_Activate*()` call,
      insert (swap the getter per ability):
      ```cpp
      // Client-side cooldown gate: "Ability not ready" instead of a wasted RPC. No client ROLE gate
      // here -- a wrong-role player's cooldown for this ability is always -1 (never set), so this is
      // a silent no-op for them; Server_Activate*'s own role gate stays the real guard.
      if (const ACoopCharacter* C = Cast<ACoopCharacter>(GetPawn()))
      {
      	if (!IsAbilityReady(C->GetShieldCooldownEndServerTime()))  // GetSpeed…/GetDash…/GetStabilize…
      	{
      		ShowToast(NSLOCTEXT("CoopAbilities", "AbilityNotReady", "Ability not ready"));
      		return;
      	}
      }
      ```
- [ ] `CoopPlayerController.cpp` — **each of the 3 target-required wrappers** (`ActivateExecution`
      → Damage, `ActivateArmorBreak` → Tank, `ActivateOverload` → Damage): prepend the role gate
      and cooldown gate *before* the existing target check:
      ```cpp
      // Client-side ROLE gate: IA_Execution/Overload (Q/E) and IA_ArmorBreak (E) share keys with
      // other roles' abilities, so this wrapper fires on every role's keypress. Bail silently for
      // the wrong role -- without this, a non-Damage player pressing Q sees a spurious "Please
      // choose a target". Server_Activate*_Implementation still role-gates authoritatively.
      const ACoopPlayerState* PS = GetPlayerState<ACoopPlayerState>();
      if (!PS || PS->GetRole() != EPlayerRole::Damage)  // EPlayerRole::Tank for ArmorBreak
      {
      	return;
      }

      // Client-side cooldown gate -- fires before the target check (a not-ready ability is the more
      // fundamental blocker).
      if (const ACoopCharacter* C = Cast<ACoopCharacter>(GetPawn()))
      {
      	if (!IsAbilityReady(C->GetExecutionCooldownEndServerTime()))  // GetArmorBreak…/GetOverload…
      	{
      		ShowToast(NSLOCTEXT("CoopAbilities", "AbilityNotReady", "Ability not ready"));
      		return;
      	}
      }
      ```

### P1.2 — `CoopAbilitySlotWidget`: descriptions + hover poll + getters

- [ ] `CoopAbilitySlotWidget.h` — `public:` add:
      ```cpp
      // Read by UCoopActionBarWidget each tick to drive the shared hover tooltip. Name/description
      // come from this slot's (local role, SlotIndex) kit entry. IsCursorOver() is a pure geometry
      // poll (NativeTick) -- the bar stays HitTestInvisible, no Slate hover (the WoW action bar
      // decision / CLAUDE.md §5). HasAbilityEntry() is false only for a slot past the role's kit.
      FText GetAbilityName() const;
      FText GetAbilityDescription() const;
      bool HasAbilityEntry() const;
      bool IsCursorOver() const { return bCursorOver; }
      ```
- [ ] `CoopAbilitySlotWidget.h` — `private:` add `bool bCursorOver = false;` (plain, not a
      `UPROPERTY` — transient local UI state).
- [ ] `CoopAbilitySlotWidget.cpp` — `+#include "Framework/Application/SlateApplication.h"`.
- [ ] `CoopAbilitySlotWidget.cpp` — `FCoopAbilitySlotInfo` gets a trailing field:
      ```cpp
      struct FCoopAbilitySlotInfo
      {
      	FText Name;
      	FLinearColor TileColor;
      	bool bImplemented = false;
      	FText Description;   // NEW -- lifted verbatim from CoopAbilityCardWidget.cpp's table.
      };
      ```
- [ ] `CoopAbilitySlotWidget.cpp` — append the 4th init element to all 11 kit entries (verbatim
      from `CoopAbilityCardWidget.cpp`, `NSLOCTEXT` namespace `"CoopAbilitySlot"`, keys
      `"<Ability>Desc"`):
      - Tank/Shield: `"Raise a barrier in front of you that blocks damage from that direction."`
      - Tank/Armor Break: `"Mark a target, opening a brief window for Control to act on it."`
      - Support/Speed: `"Grant a teammate a burst of movement speed."`
      - Support/Link: `"Bond yourself to a teammate -- the seed of a wider network."`
      - Runner/Dash: `"A short dash. Stronger if a teammate has buffed your speed."`
      - Runner/Carry: `"Pick up and carry an object."`
      - Runner/Chain: `"Fire a tether that pulls you or a target."`
      - Control/Stabilize: `"Cast on a shielded teammate to upgrade their shield for the whole team."`
      - Control/Mind Fracture: `"Cast on a marked target to reveal the truth."`
      - Control/Channel: `"Cast on a bonded pair to spread the link to the whole team."`
      - Damage/Execution: `"A finishing strike -- only lands while the target is physically vulnerable."`
      - Damage/Overload: `"A finishing strike -- only lands while the target is magically vulnerable."`
- [ ] `CoopAbilitySlotWidget.cpp` — in `NativeTick`, near the top (after `Super::`):
      ```cpp
      // Pure geometry poll for the hover tooltip -- the bar is HitTestInvisible so there is no
      // Slate hover event; UCoopActionBarWidget reads bCursorOver each tick. GetCursorPos() is
      // absolute desktop space, which is what FGeometry::IsUnderLocation expects.
      bCursorOver = MyGeometry.IsUnderLocation(FSlateApplication::Get().GetCursorPos());
      ```
- [ ] `CoopAbilitySlotWidget.cpp` — add the 4 definitions (reuse the anon-namespace `GetKitForRole`
      + the existing `GetLocalPlayerRole()`):
      ```cpp
      FText UCoopAbilitySlotWidget::GetAbilityName() const
      {
      	const TArray<FCoopAbilitySlotInfo>& Kit = GetKitForRole(GetLocalPlayerRole());
      	return Kit.IsValidIndex(SlotIndex) ? Kit[SlotIndex].Name : FText::GetEmpty();
      }
      FText UCoopAbilitySlotWidget::GetAbilityDescription() const
      {
      	const TArray<FCoopAbilitySlotInfo>& Kit = GetKitForRole(GetLocalPlayerRole());
      	return Kit.IsValidIndex(SlotIndex) ? Kit[SlotIndex].Description : FText::GetEmpty();
      }
      bool UCoopAbilitySlotWidget::HasAbilityEntry() const
      {
      	return GetKitForRole(GetLocalPlayerRole()).IsValidIndex(SlotIndex);
      }
      ```

### P1.3 — `CoopActionBarWidget`: tooltip members + drive

- [ ] `CoopActionBarWidget.h` — forward-declare `class UCoopAbilitySlotWidget; class UWidget;
      class UTextBlock;` and add to a new `private:` section:
      ```cpp
      // The 3 placed WBP_AbilitySlot instances -- bound by name (Slot0/Slot1/Slot2 already exist as
      // WBP_ActionBar variables). Polled each tick for IsCursorOver() to drive the hover tooltip.
      UPROPERTY(meta = (BindWidgetOptional))
      TObjectPtr<UCoopAbilitySlotWidget> Slot0;
      UPROPERTY(meta = (BindWidgetOptional))
      TObjectPtr<UCoopAbilitySlotWidget> Slot1;
      UPROPERTY(meta = (BindWidgetOptional))
      TObjectPtr<UCoopAbilitySlotWidget> Slot2;

      // Hover tooltip panel above the bar (LoL/WoW style). Added to WBP_ActionBar's RootCanvas in
      // P3. Shown via RenderOpacity only -- never its own Visibility (P9 self-hide freeze).
      UPROPERTY(meta = (BindWidgetOptional))
      TObjectPtr<UWidget> TooltipRoot;
      UPROPERTY(meta = (BindWidgetOptional))
      TObjectPtr<UTextBlock> TooltipNameText;
      UPROPERTY(meta = (BindWidgetOptional))
      TObjectPtr<UTextBlock> TooltipDescText;
      ```
- [ ] `CoopActionBarWidget.cpp` — `+#include "Core/CoopAbilitySlotWidget.h"`,
      `+#include "Components/TextBlock.h"`.
- [ ] `CoopActionBarWidget.cpp` — at the end of `NativeTick` (after the `SetRenderOpacity(bShow…)`):
      ```cpp
      // Hover tooltip: whichever of the 3 slots the cursor is over (and that has a kit entry)
      // drives one shared name+description panel above the bar. Geometry-poll + RenderOpacity
      // toggle -- bar and slots stay HitTestInvisible so the cursor still falls through to the
      // camera drag (CLAUDE.md §5 / the WoW action bar decision).
      const UCoopAbilitySlotWidget* Hovered = nullptr;
      if (bShow)
      {
      	const UCoopAbilitySlotWidget* Slots[] = { Slot0, Slot1, Slot2 };
      	for (const UCoopAbilitySlotWidget* S : Slots)
      	{
      		if (S && S->IsCursorOver() && S->HasAbilityEntry()) { Hovered = S; break; }
      	}
      }
      if (TooltipNameText) { TooltipNameText->SetText(Hovered ? Hovered->GetAbilityName() : FText::GetEmpty()); }
      if (TooltipDescText) { TooltipDescText->SetText(Hovered ? Hovered->GetAbilityDescription() : FText::GetEmpty()); }
      if (TooltipRoot)     { TooltipRoot->SetRenderOpacity(Hovered ? 1.0f : 0.0f); }
      ```

### P1.4 — Re-read every touched file on disk, verify symbols

- [ ] Re-read all P1 files on disk. Confirm: the 7 `Get*CooldownEndServerTime()` getters exist on
      `ACoopCharacter` (they do — `CoopCharacter.h:86-116`); `EPlayerRole` + `ACoopPlayerState::
      GetRole()` are usable in the controller cpp (they are — the `Server_*_Implementation`s use
      them); `GetKitForRole` / `GetLocalPlayerRole` reachable from the new slot getters (same .cpp);
      `FGeometry::IsUnderLocation` + `FSlateApplication::Get().GetCursorPos()` signatures;
      `Build.cs` already has `Slate`/`SlateCore`/`UMG` (it does). No `Build.cs` change.

---

## P2 — One full external rebuild from a closed editor  *(BUILD DONE 2026-09-03 — P2.4/P2.5 need the user)*

New `UPROPERTY`s on the already-loaded `UCoopActionBarWidget` (`UUserWidget` subclass) → Live
Coding unsafe (`DECISIONS.md`). Same procedure as `ABILITIES_PROGRESS.md` P2 / `cursor_progress.md`
P2.

- [~] **P2.0** — **DEFERRED to P2.4/P3 (needs the editor + `unreal-mcp`).** The editor was already
      closed when this session picked up P2, so `DA_GameConstants` couldn't be saved. On disk it may
      still carry the P4.4-attempt widened `brokenDurationSeconds` / `armorBreakCooldownSeconds` —
      re-check both against 6/10 and `save_assets` once the editor is reopened. Not a blocker for the
      C++ build.
- [x] **P2.1** — Confirmed no `UnrealEditor` process (`tasklist`), 2026-09-03.
- [x] **P2.2** — Re-read every P1 delta on disk against the plan: `CoopPlayerController.h`
      (`IsAbilityReady(float) const` in `private:`); `.cpp` (`IsAbilityReady` defined near `ShowToast`,
      `#include "GameFramework/GameStateBase.h"` present at line 21; the 4 auto wrappers gain a
      cooldown gate before `Server_Activate*`; the 3 target-required wrappers gain role gate →
      cooldown gate → existing target gate; `NSLOCTEXT("CoopAbilities","AbilityNotReady",...)`).
      `CoopAbilitySlotWidget.h` (4 public getters + `bool bCursorOver`); `.cpp`
      (`#include "Blueprint/WidgetLayoutLibrary.h"`, `FText Description` on `FCoopAbilitySlotInfo` + 11
      verbatim strings, `bCursorOver = MyGeometry.IsUnderLocation(UWidgetLayoutLibrary::
      GetMousePositionOnPlatform())` in `NativeTick`, 3 getter defs reusing `GetKitForRole`/
      `GetLocalPlayerRole`). `CoopActionBarWidget.h` (6 `BindWidgetOptional` `UPROPERTY`s + forward
      decls); `.cpp` (`#include`s for `CoopAbilitySlotWidget.h` + `Components/TextBlock.h`; tooltip
      drive at the `NativeTick` tail — first `Slot0/1/2` with `IsCursorOver() && HasAbilityEntry()`
      → push name/desc, `TooltipRoot->SetRenderOpacity`). All includes present, all symbols resolve.
      No issues.
- [x] **P2.3** — Ran `Build.bat Unreal_first_GameEditor Win64 Development -project=... -waitmutex`.
      **Result: Succeeded, exit 0**, ~62s, 11 actions. All 7 changed `.cpp`s compiled
      (`CoopActionBarWidget` / `CoopAbilitySlotWidget` / `CoopPlayerController` for this pass, plus
      the abilities-expansion files still in the working set). UHT wrote **3 generated files** —
      `CoopActionBarWidget.generated.h` (3,581 B) fresh @ 18:15; verified `Slot0`/`Slot1`/`Slot2`/
      `TooltipRoot`/`TooltipNameText`/`TooltipDescText` all present in `CoopActionBarWidget.gen.cpp`
      (the 6 `BindWidgetOptional` `UPROPERTY`s reflected). `UnrealEditor-Unreal_first_Game.dll`
      relinked → **840,704 B, Sep 3 18:15** (was 830,464). `.target` rebuilt @ 18:16. **Zero
      warnings or errors.**
- [x] **P2.4** — Editor reopened; `unreal-mcp` full roster back, `IsPIERunning` → false (2026-09-03).
      Deferred P2.0: `DA_GameConstants` `brokenDurationSeconds`/`armorBreakCooldownSeconds` read 6/10
      on disk, `is_dirty` false — the P4.4-attempt widen never persisted, nothing to save.
- [x] **P2.5** — `WBP_ActionBar` `CompileWidgetBlueprint` → `true` against the rebuilt C++ parent
      (proof the 840,704 B DLL loaded and `UCoopActionBarWidget` is valid). Tree intact: `RootCanvas`
      / `SlotBox` / `Slot0-2` (all 3 `bIsVariable`, so the new `BindWidgetOptional`s bind by name).
      The 6 `BindWidgetOptional` members were already confirmed reflected in `CoopActionBarWidget.gen.cpp`
      at build time (P2.3); they're not `EditAnywhere` so `list_properties` on the CDO doesn't surface
      them — a clean compile is the right bar.

---

## P3 — Content (all `unreal-mcp`)

- [x] **P3.1** — `WBP_ActionBar` under `RootCanvas` (all via `UMGToolSet` + `ObjectTools`):
      - `TooltipRoot` — `Border`, `visibility = HitTestInvisible`, `renderOpacity 0`,
        `brushColor (0.05,0.05,0.07,0.92)` (same "set brushColor on a plain Border" pattern as
        `WBP_UnitFrame`'s `RootBorder`), `padding (10,6,10,6)`, `HAlign_Fill`/`VAlign_Fill`. Canvas
        slot: anchors `(0.5,1)`/`(0.5,1)`, alignment `(0.5,1)`, offset `top -100`, `bAutoSize true`.
      - `TooltipBox` — `VerticalBox` in `TooltipRoot`.
      - `TooltipNameText` — `TextBlock`, `font.typefaceFontName "Bold"` size 16, white,
        `HitTestInvisible`, `text ""`.
      - `TooltipDescText` — `TextBlock`, Regular 12, `autoWrapText true`, `wrapTextAt 320`,
        `(0.8,0.8,0.8)`, `HitTestInvisible`, `text ""`, slot `padding top 3`.
      `ToggleWidgetAsVariable true` on `TooltipRoot` / `TooltipNameText` / `TooltipDescText`
      (`Slot0/1/2` already variables — C++ binds by name).
- [x] **P3.2** — `CompileWidgetBlueprint` → `true`. Tree re-read: `SlotBox` + `Slot0-2` unchanged,
      the 4 new widgets present, the 3 named ones `bIsVariable`. Saved (explicit path),
      `is_dirty == false`, `WBP_ActionBar.uasset` mtime 2026-09-03 19:11 on disk.

---

## P4 — PIE verification

Editor throttle **off** for the run, **restored to `true`/`true`** after (`DECISIONS.md`). Widen
the relevant `*CooldownSeconds` on `DA_GameConstants` *before* a cast to be verified, restore after.

- [x] **P4.1** — Agentic 5-window PIE (server = Control + 4 preview clients = Tank/Runner/Support/
      Damage), all 5 roles auto-assigned, RoleSelect → Prep → HoldTheGate ran clean. **No `Error` /
      `Accessed None` / `LogScript: Warning`** for any new code (only benign `PIE: login credentials`
      + `FindTeleportSpot` spawn-overlap warnings, both pre-existing).
- [x] **P4.2 — cooldown toast** — verified on Control / Stabilize with `StabilizeCooldownSeconds`
      widened to 120 for the round trip. Cast set `stabilizeCooldownEndServerTime`; an immediate
      2nd press left it **byte-identical** (`480.20114135742188` before and after) and put "Ability
      not ready" in the Slate tree → client gate fired, no re-cast, no RPC. First (unwidened) attempt
      re-cast because the 10 s window expired between MCP round trips (the DECISIONS.md "keep the
      widen→cast→verify tight" hazard) — the widened re-test is the clean one. The other 6 wrappers
      are the identical 3-line pattern with a different getter; reviewed on disk, not each PIE-tested
      (the lingering-`MessageText` issue makes a text-only check unreliable per role).
- [x] **P4.3 — role gate, both directions.** Control (server) + Runner + Support + Tank clients each
      pressing `IA_Execution` with no target → **no** "Please choose a target" (silent return at the
      role gate). The Damage client (UEDPIE_4), same press → the toast **does** show (unchanged).
- [x] **P4.4 — hover tooltip** — via `SlateInspector.Hover` (the geometry poll picks up the platform
      cursor move). Hover the Tank's Shield tile → panel above the bar reads "Shield" + "Raise a
      barrier in front of you that blocks damage from that direction."; move to the Armor Break tile
      → swaps to "Armor Break" + its description; hover off the bar → both texts clear to empty. Bar
      never became hit-testable.
- [~] **P4.5 — camera drag + targeting with the cursor over the bar** — NOT verified via MCP (needs a
      real right-click-drag in the game viewport). Low risk: zero input-routing change, and P4.4
      proves the bar/slots stay `HitTestInvisible` (tooltip is a geometry poll, cursor falls
      through). **Owed: a one-minute human eyeball** — right-click-drag orbit + LMB-target a unit
      while the mouse sits over a tile.
- [x] **P4.6 — cleanup.** `StopPIE`; `DA_GameConstants` cooldowns restored to 8/8/4/10
      (Shield/Speed/Dash/Stabilize), saved (explicit path), `is_dirty == false`, `.uasset` mtime
      2026-09-03 19:23; editor throttle restored to `true`/`true`; SlateInspector observers removed.
      No TEMP logs were added.
- [x] **P4.7 — bake rebuild** — N/A, P4 produced no C++ changes.

---

## P5 — `DECISIONS.md`

- [x] **P5.1** — Added `DECISIONS.md` entry **"Ability-bar UX: cooldown toast + hover tooltips"**
      (after "Target-required abilities need a click-selected target"): the client-side cooldown gate
      in all 7 `Activate*` wrappers (+ `IsAbilityReady` helper, + "server still authoritative"); the
      client role gate on the 3 target-required wrappers and the spurious "choose a target" it fixes;
      the geometry-poll hover (bar stays `HitTestInvisible`, no input-routing change); descriptions
      duplicated into the slot kit table; the `WBP_ActionBar` panel (`RenderOpacity`, no Designer
      bindings); the lingering-`MessageText` verification caveat; one closed-editor `Build.bat`; and
      the full agentic-PIE verification result.

---

## Log
(Newest at the bottom. One line per completed step.)

- **Plan written (2026-09-03).** Two features agreed in chat: (1) "Ability not ready" toast on
  cooldown for all 7 abilities; (2) LoL/WoW-style hover tooltips (name + description) above the
  action bar for all slots. Design locked above. Next: P1 (all C++, one batch), then P2 (one
  rebuild). `DA_GameConstants` `brokenDurationSeconds`/`armorBreakCooldownSeconds` restored to 6/10
  in-memory; disk save owed (P2.0, `save_assets` fails during PIE).

- **P1 done — all C++ in one uncommitted batch (2026-09-03).** Self-reviewed on disk; not built.
  - **`CoopPlayerController.h/.cpp`** — `bool IsAbilityReady(float) const` helper
    (`Now >= end`, `GetServerWorldTimeSeconds()`, `+#include GameFramework/GameStateBase.h`). All 4
    auto wrappers (`ActivateShield/Stabilize/Speed/Dash`): `Cast<ACoopCharacter>(GetPawn())` →
    `!IsAbilityReady(Get<X>CooldownEndServerTime())` → `ShowToast("Ability not ready")` + return,
    no client role gate (wrong-role CD is always -1). All 3 target-required wrappers
    (`ActivateExecution/ArmorBreak/Overload`): client **role gate** (`GetPlayerState<ACoopPlayerState>
    ()->GetRole() != Damage/Tank` → silent return) → cooldown gate → existing target gate → RPC.
    New `NSLOCTEXT("CoopAbilities", "AbilityNotReady", "Ability not ready")`.
  - **`CoopAbilitySlotWidget.h/.cpp`** — `public`: `GetAbilityName()` / `GetAbilityDescription()` /
    `HasAbilityEntry()` / `IsCursorOver()`. `private`: `bool bCursorOver`. `FCoopAbilitySlotInfo`
    gets a trailing `FText Description`; all 11 kit entries get the description verbatim from
    `CoopAbilityCardWidget.cpp` (`NSLOCTEXT` ns `"CoopAbilitySlot"`, keys `"<Ability>Desc"`).
    `NativeTick`: `bCursorOver = MyGeometry.IsUnderLocation(UWidgetLayoutLibrary::
    GetMousePositionOnPlatform())` (`+#include Blueprint/WidgetLayoutLibrary.h` — **not**
    `FSlateApplication`, which would need the unlisted `Slate` module). 3 getter defs reuse the
    anon-ns `GetKitForRole` + existing `GetLocalPlayerRole()`.
  - **`CoopActionBarWidget.h/.cpp`** — 6 `BindWidgetOptional` `UPROPERTY`s (`Slot0/1/2`
    `UCoopAbilitySlotWidget*`, `TooltipRoot` `UWidget*`, `TooltipNameText`/`TooltipDescText`
    `UTextBlock*`) + forward decls. `NativeTick` tail: find the first of `Slot0/1/2` with
    `IsCursorOver() && HasAbilityEntry()` (only when `bShow`), push name/desc to the two text
    blocks, `TooltipRoot->SetRenderOpacity(hovered ? 1 : 0)`. `+#include CoopAbilitySlotWidget.h`,
    `+#include Components/TextBlock.h`.
  - **No `Build.cs` change.** The 7 `Get*CooldownEndServerTime` getters, `EPlayerRole`/`GetRole()`,
    `GetKitForRole`, `FGeometry::IsUnderLocation`, `UWidgetLayoutLibrary` all resolve against
    existing includes/deps (`UMG`/`SlateCore` already listed).
  - **Next: P2 — user stops PIE + closes the editor, then the full `Build.bat`** (6 new
    `BindWidgetOptional` `UPROPERTY`s on the `UUserWidget`-derived `UCoopActionBarWidget` → Live
    Coding unsafe).

- **P2 rebuild done (2026-09-03).** Resumed session, editor already closed (no `UnrealEditor`
  process), so P2.0's `DA_GameConstants` save is deferred to when the editor's back (flagged in the
  STATUS block). Re-read all 6 P1 deltas on disk vs the plan — all consistent, all includes present
  (`GameStateBase.h` for `GetServerWorldTimeSeconds`; `WidgetLayoutLibrary.h` for the cursor poll;
  `CoopAbilitySlotWidget.h` + `TextBlock.h` in the bar widget). Ran full external `Build.bat`:
  **Succeeded, exit 0**, ~62s, 11 actions, all 7 changed `.cpp`s compiled. UHT wrote 3 generated
  files; `CoopActionBarWidget.gen.cpp` has all 6 `BindWidgetOptional` members (`Slot0/1/2`,
  `TooltipRoot`, `TooltipNameText`, `TooltipDescText`). DLL → **840,704 B, Sep 3 18:15** (was
  830,464). No warnings/errors. **Next: P2.4/P2.5 need the user — reopen the editor (also do the
  deferred `DA_GameConstants` save), then P3 (`WBP_ActionBar` gets the `TooltipRoot`/`TooltipNameText`/
  `TooltipDescText` panel under `RootCanvas`), P4 PIE, P5 `DECISIONS.md`.**

- **P2.4/P2.5 → P5 done — FEATURE COMPLETE (2026-09-03).** Editor reopened by the user;
  `unreal-mcp` full roster, PIE off. **P2.0 was a non-issue** — `DA_GameConstants` already had
  `brokenDurationSeconds` 6 / `armorBreakCooldownSeconds` 10 on disk (`is_dirty` false); the
  P4.4-attempt widen never persisted. **P2.5:** `WBP_ActionBar` compiles clean against the 840,704 B
  DLL's `UCoopActionBarWidget`. **P3:** added `TooltipRoot` (`Border`, `brushColor
  (0.05,0.05,0.07,0.92)`, `HitTestInvisible`, `renderOpacity 0`, canvas-anchored bottom-centre, offset
  `top -100`, `bAutoSize`) → `TooltipBox` (`VerticalBox`) → `TooltipNameText` (Bold 16) +
  `TooltipDescText` (Regular 12, wrapped 320) — all `bIsVariable`; compiled + saved, `.uasset` on
  disk. **P4 (agentic 5-window PIE):** clean RoleSelect→Prep→HoldTheGate (no script errors); cooldown
  toast verified on Control/Stabilize with a 120 s widen (2nd press → toast + `stabilizeCooldownEnd`
  byte-identical); role gate verified both directions across all 5 clients (non-Damage `IA_Execution`
  → silent, Damage → "Please choose a target"); hover tooltip verified via `SlateInspector.Hover`
  (Shield tile → name+desc above bar, → Armor Break tile swaps, → off-bar clears). **P4.5**
  (camera-drag-while-cursor-over-bar) left for a one-minute human eyeball — low risk, no
  input-routing change. **P4.6:** PIE stopped, `DA_GameConstants` cooldowns restored to 8/8/4/10 +
  saved + clean, editor throttle back to `true`/`true`. **P4.7:** N/A. **P5:** `DECISIONS.md` entry
  "Ability-bar UX: cooldown toast + hover tooltips" added.
  Observed (not this feature's bug): the prep-arena `WBP_PrepArenaHUD` ability cards stayed on screen
  into HoldTheGate — flagged for a separate look.

- **Follow-up F1 started — prep-arena ability cards being deleted (2026-09-03).** User: "leave only
  the hovers". F1.1–F1.4 done: 4 `WBP_AbilityCard` stripped from `WBP_PrepArenaHUD` (compiled,
  saved), `WBP_AbilityCard.uasset` deleted (PIE had to be stopped first), `CoopAbilityCardWidget.h/
  .cpp` `git rm`'d, 3 dangling comments fixed. §6.3 deviation flagged (like the Team Synergies
  removal). **Owed: F1.5 rebuild (closed editor) → F1.6 verify → F1.7 `DECISIONS.md`.**

- **F1.5 rebuild done (2026-09-03).** Resumed session, confirmed no `UnrealEditor` process and that
  no `Source/` code still references `CoopAbilityCardWidget` (only one explanatory comment in
  `CoopAbilitySlotWidget.cpp`). Ran full external `Build.bat`: **Succeeded, exit 0**, ~63s, 8
  actions. UHT: `Invalidating makefile ... (source file removed)`, 4 generated files, module
  regenerated without the class. `CoopActionBarWidget`/`CoopAbilitySlotWidget`/`CoopCharacter`/
  `CoopStatusBarWidget` recompiled (F1.4 comment edits). DLL **shrank 840,704 → 822,784 B (Sep 3
  20:22)** — the `UCoopAbilityCardWidget` UCLASS is gone. No warnings/errors, no undefined symbols.

- **F1.6 + F1.7 done — FOLLOW-UP F1 COMPLETE, WHOLE PLAN COMPLETE (2026-09-03).** Editor was already
  open on the F1.5 DLL. **F1.6:** `search_subclasses(UUserWidget,"Coop")` → `UCoopAbilityCardWidget`
  gone, `CoopAbilitySlotWidget`/`CoopActionBarWidget` present. `WBP_PrepArenaHUD` compiles → `true`,
  tree = `CanvasPanel_40` → `TextBlock_60` + empty `HorizontalBox_4`, no card instances, parent
  `UCoopPrepCountdownWidget` intact. `WBP_AbilityCard` no longer in `ListWidgetBlueprints`. Agentic
  5-window PIE (`bDevMode` toggled true on the BP_GameMode CDO for the run): full RoleSelect → Prep →
  HoldTheGate, **zero script/UMG/Blueprint errors or warnings** for touched code. Prep HUD
  (`PrepArenaDurationSeconds` widened to 120 for the capture) screenshotted → **only the countdown**
  + party/target frames + action bar, **no cards**; HoldTheGate identical minus the live number.
  Hover tooltip not re-triggerable via MCP (geometry poll reads the un-warpable OS cursor) —
  structurally intact (panel bound in tree, slots tick correct names, F1.4 = comments only), P4.4
  stands. **Cleanup:** PIE stopped; `bDevMode` → `false` (`BP_GameMode.uasset` byte-identical to git
  HEAD, not modified); `PrepArenaDurationSeconds` → 10; `DA_GameConstants` re-saved (logical no-op,
  already `M` pre-session); all three assets `is_dirty == false`; observers/temp-logs clean.
  **F1.7:** `DECISIONS.md` "Ability-bar UX…" entry extended with the "Follow-up (2026-09-03):
  prep-arena ability cards deleted" subsection + updated its stale "Observed…" note.
  **Owed to a human (low-risk, no code): P4.5 camera-drag-over-bar, and a real-mouse hover-tooltip
  glance. `WBP_PrepArenaHUD` phase-gate still unwired — bare countdown lingers into HoldTheGate,
  parked.**

---

## Follow-up F1 — delete the prep-arena ability cards (2026-09-03)

**User directive:** *"Remove the other tooltip that is showing when you select a class, leave only the
hovers."* → the prep-arena ability cards (`WBP_AbilityCard` ×4 in `WBP_PrepArenaHUD`, showing name +
one-sentence description during the Prep phase, and *lingering* into HoldTheGate) are redundant now
that the action-bar hover tooltips cover the same "what does this ability do" surface. User picked
**"Strip + delete the assets"** when asked how far to go.

**Deviation from CLAUDE.md §6.3** (which mandates "3–4 ability cards with icon, name, one-sentence
explanation" in the prep arena) — flagged to the user before acting, same class of deliberate
deviation as the earlier "Team Synergies panel removed" entry in `DECISIONS.md`. The
`docs/abilities.md` ability list + the action-bar slot kit table (`CoopAbilitySlotWidget.cpp`) remain
the homes of the name/description text.

### Steps

- [x] **F1.1** — `WBP_PrepArenaHUD`: `RemoveWidget` on all 4 `WBP_AbilityCard` instances
      (`WBP_AbilityCard` / `_0` / `_1` / `_2`). Tree now = `CanvasPanel_40` → `TextBlock_60`
      (countdown) + `HorizontalBox_4` (empty leftover from the old Team Synergies slot — **left in
      place**, minimal-touch; user was told). `CompileWidgetBlueprint` → `true`, saved (mtime
      2026-09-03 19:51). *(Done while a user PIE session was live — editor-side asset edit, persisted
      fine; the live session kept showing the cards from its pre-edit class instance, as expected.)*
- [x] **F1.2** — Deleted `/Game/Blueprints/UI/WBP_AbilityCard` (`AssetTools.delete` → `true`,
      `exists` → `false`, no redirector — `find_assets "AbilityCard"` empty). Needed PIE stopped
      first (`delete` is refused in play mode: *"The Editor is currently in a play mode."*).
- [x] **F1.3** — `git rm Source/Unreal_first_Game/Core/CoopAbilityCardWidget.h` + `.cpp`. No code
      referenced the class — only comments. Full-repo grep over `*.{cpp,h,cs,ini,uproject}` now
      clean except one deliberate explanatory comment in `CoopAbilitySlotWidget.cpp`.
- [x] **F1.4** — Cleaned 3 stale comments that used the deleted widget as an analogy:
      `CoopAbilitySlotWidget.h` (`SlotIndex` "like CardIndex" → "set in the Designer's Details
      panel"), `CoopAbilitySlotWidget.cpp` (kit table "duplicated from CoopAbilityCardWidget's
      table" → "the sole home of the ability name + description strings now that the cards are
      deleted"), `CoopStatusBarWidget.h` (2 spots — the `WBP_AbilityCard` contrast + the "same
      mechanism WBP_AbilityCard uses" bind-function reference → `WBP_RoleSelect`/`WBP_PrepArenaHUD`).
- [x] **F1.5 — rebuild DONE (2026-09-03).** Confirmed no `UnrealEditor` process; confirmed the only
      remaining `CoopAbilityCardWidget` mention in `Source/` is one explanatory comment in
      `CoopAbilitySlotWidget.cpp` (not a code reference). Ran full external `Build.bat`: **Succeeded,
      exit 0**, ~63s, 8 actions. UHT logged `Invalidating makefile ... (source file removed)`, wrote
      4 generated files (module regenerated without `CoopAbilityCardWidget`). `CoopActionBarWidget` /
      `CoopAbilitySlotWidget` / `CoopCharacter` / `CoopStatusBarWidget` recompiled (the F1.4 comment
      cleanup). `UnrealEditor-Unreal_first_Game.dll` **shrank 840,704 → 822,784 B (Sep 3 20:22)** —
      the dropped UCLASS. **Zero warnings/errors**, no undefined-symbol errors. (Orphaned
      `Intermediate/.../CoopAbilityCardWidget.cpp.obj` + `.gen.*` artifacts remain on disk — harmless,
      not in the relinked module; a future clean rebuild sweeps them.)
- [x] **F1.6 — reopen + verify DONE (2026-09-03).** Editor was already open on the F1.5-rebuilt
      822,784 B DLL — `search_subclasses(UUserWidget, "Coop")` confirms `UCoopAbilityCardWidget` is
      **gone** from the loaded module (`CoopAbilitySlotWidget`/`CoopActionBarWidget` present).
      `WBP_PrepArenaHUD` `CompileWidgetBlueprint` → **true**; tree = `CanvasPanel_40` →
      `TextBlock_60` (countdown) + empty `HorizontalBox_4` — no card instances, no missing-parent
      (`UCoopPrepCountdownWidget` untouched). `ListWidgetBlueprints /Game/Blueprints/UI` no longer
      lists `WBP_AbilityCard`. **Agentic 5-window PIE** (dev mode toggled on the BP_GameMode CDO for
      the run, restored after): full RoleSelect → Prep → HoldTheGate, **zero `Error`/`Accessed None`/
      `LogScript`/`LogUMG`/`LogBlueprint` warnings** for any touched code (only the pre-existing
      benign `FindTeleportSpot` + `Not enough login credentials` warnings). Prep-phase HUD (widened
      `PrepArenaDurationSeconds` to 120, restored to 10 after) screenshotted: **only the countdown**
      (live number) + party/target frames + action bar — **no cards**; HoldTheGate shows the same
      minus a live number (bare countdown still lingers — known, out of scope). Hover tooltip **not**
      re-exercised live (the `GetMousePositionOnPlatform()` geometry poll reads the OS cursor, which
      `SlateInspector.Hover`'s synthetic move doesn't warp — same harness gap as P4.5); code path is
      comment-only-changed by F1.4, tooltip panel widgets present + bound in the live tree, slot
      widgets tick with correct per-role names → structurally intact, P4.4's functional verification
      stands. **Cleanup:** PIE stopped, `bDevMode` back to `false` (BP_GameMode.uasset byte-identical
      to git HEAD → not modified), `PrepArenaDurationSeconds` back to 10, DA_GameConstants re-saved
      (logical no-op; was already `M` in git pre-session), all assets `is_dirty == false`, no
      SlateInspector observers left, no TEMP logs.
- [x] **F1.7 — `DECISIONS.md` DONE (2026-09-03).** Extended the "Ability-bar UX: cooldown toast +
      hover tooltips" entry with a **"Follow-up (2026-09-03): prep-arena ability cards deleted"**
      subsection: the user directive + "strip + delete" scope, the §6.3 deviation (logged, same
      class as "Team Synergies panel removed"), slot-kit table now the sole home of the name/desc
      strings, `WBP_AbilityCard` + `UCoopAbilityCardWidget` gone, the one closed-editor `Build.bat`
      (DLL 840,704 → 822,784 B), the full agentic-PIE verification, and the still-parked
      `WBP_PrepArenaHUD` phase-gate. Also updated that entry's "Observed, not caused by this feature"
      note to point at the follow-up.

### Not in scope for F1

- The empty `HorizontalBox_4` in `WBP_PrepArenaHUD` (old Team Synergies slot) — left as-is unless
  the user asks.
- `WBP_PrepArenaHUD`'s own phase-gating: it has `UCoopPrepCountdownWidget::GetPrepArenaVisibility()`
  (Visible only during `EMatchPhase::Prep`) available for a Designer "Bind" on its root, but that
  binding isn't wired, which is why the whole panel (now just the countdown) still lingers into
  HoldTheGate. Wiring it needs a human "Bind" click (`unreal-mcp` can't author it) or a C++
  `NativeTick` self-gate. **Flagged, not done** — the user's directive was about the cards, and the
  bare countdown lingering is far less intrusive.
