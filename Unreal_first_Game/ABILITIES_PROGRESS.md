# Ability Kit Expansion — Armor Break, Overload & target-required casting — Plan & Progress Tracker

Resumable checklist for the next ability pass: add **Armor Break** (Tank) and **Overload** (Damage),
and introduce **target-required casting** — an ability that needs a target can only fire when the
local player has a click-selected target, otherwise a centre-screen **"Please choose a target"**
toast shows and nothing is sent. This doc **is** the plan — there is no separate spec file (same
convention as `cursor_progress.md`); the design was agreed in chat on 2026-09-03.

> **STATUS (2026-09-03): P1–P3 DONE, P5 DONE. P4.1 + P4.2-core DONE (agentic smoke pass); P4.3–P4.8
> await a hands-on PIE session.** All C++ built (`Build.bat` exit 0, DLL 830,464 B @ 16:00) +
> symbol-load verified; all content wired via `unreal-mcp` — every asset saved + `is_dirty == false`
> + on disk. `DECISIONS.md` + `docs/abilities.md` updated. A 5-window PIE session ran the full
> RoleSelect→Prep→HoldTheGate loop with **zero errors**; the no-target toast fires; the 2 new input
> actions + EventGraph chains + C++ wrappers all execute clean.
>
> **The only work left is the hands-on P4.3–P4.8 verification** — targeted casts on a monster
> (needs a real LMB world-click the MCP tools can't position), cooldown `COND_OwnerOnly` replication
> across the 5 client worlds, action-bar tile visuals, and confirming Speed/Stabilize + the Fortress
> flow are unchanged. Then tick P4.9/P4.10 (P4.10 = rebuild only if that pass surfaces a C++ fix).
>
> P1.6 was implemented as **two fully spelled-out functions** (`ResolveExecution` + `ResolveOverload`),
> NOT the `TFunctionRef` helper the plan sketched — §4.6 / §2 favour the explicit copy. See the Log.

> Previous contents of this file (the **WoW-style action bar** feature — P1–P10) are **complete and
> verified** (5-client PIE, baked DLL). Its design decisions and gotchas live permanently in
> `DECISIONS.md` ("WoW-style action bar (bottom-screen ability bar)"). This file was cleared and
> repurposed for the ability-kit expansion at the user's explicit request — same as when it was
> cleared for the action bar.

**If a session is interrupted, a new session should:**
1. Read this file top to bottom, find the first unchecked `- [ ]`.
2. Read `docs/abilities.md` — Armor Break and Overload are both already fully specced there; the
   tags `Status.Broken` and `Status.Vulnerable.Magic` are already in its glossary. **Nothing new is
   invented** by this plan.
3. Read `DECISIONS.md`'s entries: "The Q ability per role…", "WoW-style action bar…", "Cursor +
   click-to-target, target frame, party frames…", "Live Coding must not be used to add a new
   UCLASS…", "No `unreal-mcp` tool can set a UMG Designer Bind Function…". This plan reuses every
   pattern in those.
4. Continue from the first unchecked box. **P1 is all C++ in one batch — do not rebuild between its
   steps. P2 is the single rebuild.**

---

## Design decisions — LOCKED (agreed 2026-09-03, do not re-litigate)

- **Scope this pass = Armor Break + Overload only.** The other greyed action-bar slots stay greyed:
  - **Link, Channel, Mind Fracture** — deferred. Their real payoffs need scenes that don't exist
    (The Dying Room / The False King). Built when those scenes are. `docs/abilities.md` keeps them
    as specced-not-built.
  - **Carry, Chain, Taunt** — out of this pass. Carry/Chain target *objects*, which the click-select
    system (`ACoopPlayerController::CurrentTargetActor`) doesn't accept, and there are no carryable
    props yet. Taunt is undesigned (`docs/abilities.md` TBD).
  - Rationale for stopping here: user chose "only build what fully works today". Armor Break's
    `Status.Broken` and Overload's damage are both fully functional in isolation; the rest aren't.

- **Target-required abilities this pass: Execution (retrofit), Armor Break (new), Overload (new).**
  All three target an **enemy** (`ACoopMonsterCharacter`), so the server-side "valid target" check
  is uniformly `Cast<ACoopMonsterCharacter>`.

- **Speed and Stabilize are NOT retrofitted.** They keep their existing server-side "nearest ally /
  nearest Tank in range" auto-search. Explicit user call (2026-09-03): *"retrofit only Execution,
  leave Speed and Stabilize auto for now"*. This keeps **Hold the Gate / the Fortress synergy flow
  completely untouched** — that scene is verified working and Stabilize's auto-target is load-bearing
  for it. Revisit if a later scene makes ally/Tank click-targeting necessary.

- **The gate is client-side first, server re-validates.** The thin `Activate*()` wrapper on
  `ACoopPlayerController` (the `BlueprintCallable` that `BP_PlayerCharacter` calls) checks
  `GetCurrentTargetActor()`. Null → `ShowToast("Please choose a target")`, **return without sending
  the RPC**. Non-null → `Server_Activate*(Target)`. The server then re-validates the target it
  receives (type, range, required tag) — intent, re-checked (CLAUDE.md §4.1). This is exactly the
  seam `cursor_progress.md` decision #6 and the `DECISIONS.md` "Cursor + click-to-target" entry
  anticipated: *"a later phase routes `GetCurrentTargetActor()` into the `Server_Activate*` RPCs as
  intent, server re-validates."*

- **`Server_Activate*` RPC signatures change** for the three target-required abilities:
  `Server_ActivateExecution()` → `Server_ActivateExecution(AActor* Target)`, plus new
  `Server_ActivateArmorBreak(AActor* Target)` / `Server_ActivateOverload(AActor* Target)`. `AActor*`
  replicates fine in an RPC — both target types are replicated actors, and the server has every
  actor. The server casts + validates; it never trusts the client's type.

- **"Please choose a target" is a centre-screen toast** via a new reusable `UCoopToastWidget`
  (`Core/CoopToastWidget.h/.cpp` + `WBP_Toast`). Centre-anchored, created once in
  `ACoopPlayerController::BeginPlay`, `NativeTick`-driven, fades `RenderOpacity` 1→0 over
  `ToastDurationSeconds`. **Never self-hides via `Visibility`** (the P9 action-bar freeze — a widget
  that leaves the "visible" family in its own tick freezes forever); stays `HitTestInvisible`,
  toggles `RenderOpacity`. Reusable for any future "X failed" message.

- **Toast fade is a delta from one stored timestamp** (`GetWorld()->GetTimeSeconds() - StartTime`),
  **not** a `DeltaTime` accumulation (CLAUDE.md §4.4) — same shape as the action bar's cooldown
  sweep (`End - Now`). It's local cosmetic UI, so game time (not server time) is correct and
  sufficient here.

- **New keys: `E` for both new abilities, role-gated.** `IMC_Default` maps `E` → `IA_ArmorBreak`
  *and* `E` → `IA_Overload`. Same "one physical key, different meaning per role, no conflict because
  every `Server_Activate*` is role-gated and no-ops for the wrong role" pattern that already puts all
  5 first abilities on `Q` (`DECISIONS.md` "The Q ability per role"). `E` is currently unmapped
  (Shield moved E→Q).

- **New tags — declared only, not invented.** `Status.Broken` and `Status.Vulnerable.Magic` are
  already in `docs/abilities.md`'s tag glossary (rows for Tank Armor Break → Control Mind Fracture,
  and The Heart → Damage Overload). This plan only adds the `UE_DECLARE`/`UE_DEFINE` lines to
  `CoopGameplayTags.h/.cpp`. No `docs/abilities.md` tag edit.

- **`Status.Broken` has no reader yet — accepted.** Armor Break applies it to the targeted monster
  (real/fake is irrelevant — `docs/abilities.md`: "Armor Break itself does not reveal anything");
  nothing consumes it until Control's Mind Fracture + the False King clones (Build 2). Until then it
  simply shows on the target frame's status line and expires. This is the same "built before its
  consumer" situation as Stabilize (built before Hold the Gate's monsters).

- **`Status.Vulnerable.Magic` is dev-granted until The Heart.** New Exec command
  `ApplyTestVulnerableMagic` (mirror of the existing `ApplyTestVulnerable`, which grants
  `.Physical`). Both get **deleted when Scene 5 lands** — same note as the existing one carries.

- **Two new cooldown fields on `ACoopCharacter`** (`ArmorBreakCooldownEndServerTime`,
  `OverloadCooldownEndServerTime`) — identical `UPROPERTY(Replicated, VisibleInstanceOnly)` +
  `COND_OwnerOnly` shape as the existing five (`DECISIONS.md` "WoW-style action bar" P1). Every
  `ACoopCharacter` carries every ability's cooldown regardless of the player's current role — role is
  a runtime PlayerState value, not a subclass. **This header change → full rebuild (P2).**

- **New `DA_GameConstants` fields** (CLAUDE.md §10 — no magic numbers): `ArmorBreakCooldownSeconds`,
  `ArmorBreakCastRangeUnits`, `BrokenDurationSeconds`, `OverloadCooldownSeconds`,
  `OverloadCastRangeUnits`, `OverloadDamageAmount`, `ToastDurationSeconds`. Defaults in
  `GameConstants.h`, mirrored into the `DA_GameConstants` asset in P3.

- **Action bar: two slots go live.** `CoopAbilitySlotWidget.cpp`'s kit table flips `bImplemented`
  to `true` for Tank slot 1 (Armor Break) and Damage slot 1 (Overload). The slot widget's keybind
  badge generalises from hardcoded `"Q"` / `SlotIndex == 0` to `"Q"` for slot 0, `"E"` for slot 1;
  the cooldown sweep generalises from `GetSlotZeroCooldown*(Role)` to
  `GetSlotCooldown*(Role, SlotIndex)`. Colours for those tiles already exist in the table.

- **`NativeTick` + `BindWidgetOptional`, no Designer "Bind Function" bindings** anywhere in the new
  widget — `unreal-mcp` can't author them (`DECISIONS.md`). Same as every widget in the project.

- **One full external `Build.bat` rebuild from a closed editor (P2).** New `UCoopToastWidget`
  UCLASS + new `UPROPERTY`s on the already-loaded `ACoopCharacter` + new/changed `UFUNCTION` RPC
  signatures → Live Coding is unsafe (`DECISIONS.md` "Live Coding must not be used to add a new
  UCLASS…"). All C++ lands in P1, one rebuild in P2, then content in P3.

---

## File map

**C++ — new:**
- `Source/Unreal_first_Game/Core/CoopToastWidget.h` / `.cpp` — `UUserWidget` base for `WBP_Toast`.
  `NativeTick` reads the owning `ACoopPlayerController`'s `GetPendingToastText()` /
  `GetPendingToastStartTime()`, fades a `BindWidgetOptional` `MessageText`. Its own
  `EditDefaultsOnly UGameConstants*` (set on `WBP_Toast`'s CDO) for `ToastDurationSeconds`, same
  pattern as `WBP_AbilitySlot`.

**C++ — modified:**
- `Source/Unreal_first_Game/Tags/CoopGameplayTags.h` / `.cpp` — declare + define `Status_Broken`,
  `Status_Vulnerable_Magic`.
- `Source/Unreal_first_Game/Core/GameConstants.h` — 7 new fields (`Category = "Abilities"` / `"UI"`).
- `Source/Unreal_first_Game/Core/CoopCharacter.h` — 2 new cooldown fields + getters/setters, 2 new
  `*CastMontage` `EditDefaultsOnly` refs + getters.
- `Source/Unreal_first_Game/Core/CoopCharacter.cpp` — 2 new `DOREPLIFETIME_CONDITION(… COND_OwnerOnly)`.
- `Source/Unreal_first_Game/Abilities/CoopTankAbilities.h` / `.cpp` — add
  `ResolveArmorBreak(ACoopCharacter* Tank, AActor* Target, const UGameConstants*)`.
- `Source/Unreal_first_Game/Abilities/CoopDamageAbilities.h` / `.cpp` — `ResolveExecution` gains an
  `AActor* Target` param and loses its `TActorIterator` nearest-search; add `ResolveOverload(…)`.
- `Source/Unreal_first_Game/Core/CoopPlayerController.h` / `.cpp`:
  - `ShowToast(const FText&)` + `PendingToastText` / `PendingToastStartTime` fields + getters.
  - `ToastWidgetClass` / `ToastWidget` pair + create-and-add-to-viewport in `BeginPlay`.
  - `ActivateExecution()` gains the client gate; `Server_ActivateExecution` gains `AActor* Target`
    + re-validation.
  - new `ActivateArmorBreak()` / `Server_ActivateArmorBreak(AActor* Target)`,
    `ActivateOverload()` / `Server_ActivateOverload(AActor* Target)`.
  - new `ApplyTestVulnerableMagic()` / `Server_ApplyTestVulnerableMagic()` Exec pair.
- `Source/Unreal_first_Game/Core/CoopUnitFrameWidget.cpp` — target-frame status line adds
  `BROKEN` + `VULNERABLE-M`.
- `Source/Unreal_first_Game/Core/CoopAbilitySlotWidget.h` / `.cpp` — kit table `bImplemented` flips;
  `GetSlotZeroCooldown*` → `GetSlotCooldown*(Role, SlotIndex)`; keybind badge `"Q"`/`"E"` per slot.

**Content — new (all via `unreal-mcp`):**
- `/Game/Input/Actions/IA_ArmorBreak`, `/Game/Input/Actions/IA_Overload` — Input Actions, Boolean,
  `Pressed` trigger only (duplicate `IA_Shield`).
- `/Game/Blueprints/UI/WBP_Toast` — WidgetBlueprint, parent `UCoopToastWidget`.

**Content — modified (all via `unreal-mcp`):**
- `/Game/Input/IMC_Default` — append `E → IA_ArmorBreak` and `E → IA_Overload` (append-only write,
  see the array-size gotcha in `DECISIONS.md` "unreal-mcp gotchas found building the Q abilities").
- `/Game/Data/DA_GameConstants` — set the 7 new fields.
- `BP_PlayerController` CDO — `toastWidgetClass` → `WBP_Toast_C`.
- `WBP_Toast` CDO — `gameConstants` → `/Game/Data/DA_GameConstants`.
- `BP_PlayerCharacter` EventGraph — 2 new `EnhancedInputAction` chains (`IA_ArmorBreak` /
  `IA_Overload` → `GetController` → `Cast To BP_PlayerController` → `ActivateArmorBreak` /
  `ActivateOverload`), built with `create_node` / `connect_pins` mirroring the `IA_Shield` chain
  exactly (**not** `write_graph_dsl` — `DECISIONS.md`).
- *(Optional, non-blocking)* `BP_PlayerCharacter` CDO — `armorBreakCastMontage` /
  `overloadCastMontage` → reuse an existing `MM_*` gesture montage. Left `None` is a valid state
  (`PlayCastMontage` no-ops).

**Docs — modified:**
- `docs/abilities.md` — one "Prototype status" line each on Armor Break and Overload.
- `DECISIONS.md` — new entry "Target-required abilities need a click-selected target".

---

## P1 — All C++ (one batch; do NOT rebuild between steps — P2 is the single rebuild)

> **✅ P1 COMPLETE (2026-09-03) — every step below is done and self-reviewed on disk. A resuming
> session should skip straight to P2.** The per-step code blocks below are kept as the reference of
> what was written. Deviations from the sketch: P1.6 is two explicit functions, not a `TFunctionRef`
> helper; P1.2's `ToastDurationSeconds` was placed next to `MatchTimerDisplayUpdateIntervalSeconds`
> (the existing `Category = "UI"` field), not at the end; a stale "no explicit targeting UI" comment
> on the `TestVulnerable*` constants was updated to mention `ApplyTestVulnerableMagic`. Full account
> in the Log.

### P1.1 — Tags

- [x] `CoopGameplayTags.h` — after `Status_Vulnerable_Physical`:
      ```cpp
      // Ability kit expansion: Tank Armor Break writes this on a monster target. Control's Mind
      // Fracture (Build 2, needs The False King) is the eventual reader -- nothing reads it yet, so
      // for now it only shows on the target frame's status line and expires. Already in
      // docs/abilities.md's glossary.
      UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Broken);

      // Ability kit expansion: Damage Overload reads this on a monster target (magic branch of
      // Execution/Overload). Real writer is "The Heart" (Scene 5, Build 2); until then it is
      // dev/test-granted via ACoopPlayerController::ApplyTestVulnerableMagic. Already in
      // docs/abilities.md's glossary.
      UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Vulnerable_Magic);
      ```
- [x] `CoopGameplayTags.cpp` — after the `Status_Vulnerable_Physical` line:
      ```cpp
      UE_DEFINE_GAMEPLAY_TAG(Status_Broken, "Status.Broken");
      UE_DEFINE_GAMEPLAY_TAG(Status_Vulnerable_Magic, "Status.Vulnerable.Magic");
      ```

### P1.2 — `GameConstants.h` fields

- [x] Add, after `ExecutionDamageAmount` (keep the existing `Category = "Abilities"` grouping):
      ```cpp
      // Ability kit expansion: Tank Armor Break (CoopTankAbilities::ResolveArmorBreak). Cooldown,
      // cast range against the click-selected ACoopMonsterCharacter, and how long Status.Broken
      // persists once applied. docs/abilities.md calls Broken a "short window" -- shorter than
      // Fortress's 8s.
      UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
      float ArmorBreakCooldownSeconds = 10.0f;

      UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
      float ArmorBreakCastRangeUnits = 800.0f;

      UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
      float BrokenDurationSeconds = 6.0f;

      // Ability kit expansion: Damage Overload (CoopDamageAbilities::ResolveOverload) -- the magic
      // branch of Execution/Overload. Mirrors Execution's numbers: same cooldown, cast range against
      // a Status.Vulnerable.Magic-holding ACoopMonsterCharacter, and flat damage on a hit.
      UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
      float OverloadCooldownSeconds = 6.0f;

      UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
      float OverloadCastRangeUnits = 400.0f;

      UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
      float OverloadDamageAmount = 100.0f;

      // Ability kit expansion: how long UCoopToastWidget shows a centre-screen message
      // ("Please choose a target") before it has fully faded. A local cosmetic display duration,
      // not a gameplay timer -- but it's a tunable "duration", so it lives here per CLAUDE.md §10.
      UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
      float ToastDurationSeconds = 2.0f;
      ```

### P1.3 — `CoopCharacter` cooldown + montage fields

- [x] `CoopCharacter.h` — after `GetExecutionCooldownEndServerTime` / `SetExecutionCooldownEndServerTime`:
      ```cpp
      float GetArmorBreakCooldownEndServerTime() const { return ArmorBreakCooldownEndServerTime; }
      void SetArmorBreakCooldownEndServerTime(float ServerTime) { ArmorBreakCooldownEndServerTime = ServerTime; }

      float GetOverloadCooldownEndServerTime() const { return OverloadCooldownEndServerTime; }
      void SetOverloadCooldownEndServerTime(float ServerTime) { OverloadCooldownEndServerTime = ServerTime; }
      ```
- [x] `CoopCharacter.h` — after `GetExecutionCastMontage`:
      ```cpp
      UAnimMontage* GetArmorBreakCastMontage() const { return ArmorBreakCastMontage; }
      UAnimMontage* GetOverloadCastMontage() const { return OverloadCastMontage; }
      ```
- [x] `CoopCharacter.h` — after `ExecutionCooldownEndServerTime`, matching the exact
      `UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")` shape of the existing five:
      ```cpp
      UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")
      float ArmorBreakCooldownEndServerTime = -1.0f;
      UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")
      float OverloadCooldownEndServerTime = -1.0f;
      ```
- [x] `CoopCharacter.h` — after `ExecutionCastMontage`:
      ```cpp
      UPROPERTY(EditDefaultsOnly, Category = "Abilities")
      TObjectPtr<UAnimMontage> ArmorBreakCastMontage;

      UPROPERTY(EditDefaultsOnly, Category = "Abilities")
      TObjectPtr<UAnimMontage> OverloadCastMontage;
      ```
- [x] `CoopCharacter.cpp` — in `GetLifetimeReplicatedProps`, after the 5 existing
      `DOREPLIFETIME_CONDITION(…, COND_OwnerOnly)`:
      ```cpp
      DOREPLIFETIME_CONDITION(ACoopCharacter, ArmorBreakCooldownEndServerTime, COND_OwnerOnly);
      DOREPLIFETIME_CONDITION(ACoopCharacter, OverloadCooldownEndServerTime, COND_OwnerOnly);
      ```

### P1.4 — `CoopToastWidget` (new)

- [x] Create `Source/Unreal_first_Game/Core/CoopToastWidget.h`:
      ```cpp
      #pragma once

      #include "CoreMinimal.h"
      #include "Blueprint/UserWidget.h"
      #include "CoopToastWidget.generated.h"

      class UTextBlock;
      class UGameConstants;

      // Centre-screen transient message ("Please choose a target", and any future "X failed" cue).
      // Purely local, cosmetic UI (CLAUDE.md §4.2): it only READS two non-replicated fields on the
      // owning ACoopPlayerController (PendingToastText + PendingToastStartTime) and fades itself.
      //
      // Same pattern as UCoopActionBarWidget / UCoopUnitFrameWidget: all feedback is NativeTick
      // against a BindWidgetOptional pointer -- no Designer "Bind Function" bindings (unreal-mcp
      // can't author them -- DECISIONS.md). Show/hide is a RenderOpacity toggle, NEVER this widget's
      // own Visibility: it is created (in ACoopPlayerController::BeginPlay) with nothing to show, and
      // a widget that leaves the "visible" family in its own NativeTick freezes forever (the P9
      // action-bar gotcha, DECISIONS.md).
      UCLASS()
      class UNREAL_FIRST_GAME_API UCoopToastWidget : public UUserWidget
      {
      	GENERATED_BODY()

      protected:
      	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

      private:
      	// DA_GameConstants, set on WBP_Toast's CDO -- same content-wiring pattern as WBP_AbilitySlot.
      	// Only used for ToastDurationSeconds; a null ref falls back to 2.0s.
      	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
      	TObjectPtr<UGameConstants> GameConstants;

      	// WBP_Toast child, matched by name.
      	UPROPERTY(meta = (BindWidgetOptional))
      	TObjectPtr<UTextBlock> MessageText;
      };
      ```
- [x] Create `Source/Unreal_first_Game/Core/CoopToastWidget.cpp`:
      ```cpp
      #include "Core/CoopToastWidget.h"
      #include "Core/CoopPlayerController.h"
      #include "Core/GameConstants.h"
      #include "Components/TextBlock.h"
      #include "Engine/World.h"

      void UCoopToastWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
      {
      	Super::NativeTick(MyGeometry, InDeltaTime);

      	// Stay HitTestInvisible forever -- never Collapsed/Hidden from our own tick (P9 freeze), and
      	// the cursor must fall straight through to the camera drag anyway (CLAUDE.md §5).
      	SetVisibility(ESlateVisibility::HitTestInvisible);

      	const ACoopPlayerController* PC = GetOwningPlayer<ACoopPlayerController>();
      	if (!PC)
      	{
      		SetRenderOpacity(0.0f);
      		return;
      	}

      	const FText Message = PC->GetPendingToastText();
      	const float StartTime = PC->GetPendingToastStartTime();
      	const float Duration = GameConstants ? GameConstants->ToastDurationSeconds : 2.0f;

      	// Local cosmetic fade: elapsed = now - one stored stamp, NOT a DeltaTime accumulation
      	// (CLAUDE.md §4.4) -- same shape as the action bar's cooldown sweep (End - Now).
      	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
      	const float Elapsed = Now - StartTime;

      	if (Message.IsEmpty() || StartTime < 0.0f || Elapsed >= Duration || Duration <= 0.0f)
      	{
      		SetRenderOpacity(0.0f);
      		return;
      	}

      	if (MessageText)
      	{
      		MessageText->SetText(Message);
      	}

      	// Full opacity for the first 60% of the window, then linear fade to 0.
      	const float FadeStart = Duration * 0.6f;
      	const float Opacity = (Elapsed <= FadeStart)
      		? 1.0f
      		: FMath::Clamp(1.0f - (Elapsed - FadeStart) / (Duration - FadeStart), 0.0f, 1.0f);
      	SetRenderOpacity(Opacity);
      }
      ```

### P1.5 — `CoopTankAbilities::ResolveArmorBreak`

- [x] `CoopTankAbilities.h` — add `class ACoopMonsterCharacter;` to the forward declares (kept for
      symmetry even though the signature takes `AActor*` — see the `.cpp`), and after `ApplyShield`:
      ```cpp
      // Ability kit expansion. Tank's second ability (docs/abilities.md: "Armor Break -- Mind Fracture
      // input"). Target is the local player's click-selected actor, forwarded from
      // ACoopPlayerController::Server_ActivateArmorBreak and re-validated here: it must be an
      // ACoopMonsterCharacter within ArmorBreakCastRangeUnits. On a valid hit, applies Status.Broken
      // for BrokenDurationSeconds -- real or fake target, Armor Break reveals nothing itself
      // (docs/abilities.md). Nothing READS Status.Broken yet (Control's Mind Fracture + the False
      // King clones are Build 2); for now it only shows on the target frame's status line. Cooldown
      // consumes on any cast that clears the gate, same "opens a window" philosophy as every ability
      // here.
      void ResolveArmorBreak(ACoopCharacter* Tank, AActor* Target, const UGameConstants* GameConstants);
      ```
- [x] `CoopTankAbilities.cpp` — add the include `#include "Core/CoopMonsterCharacter.h"` and:
      ```cpp
      void ResolveArmorBreak(ACoopCharacter* Tank, AActor* Target, const UGameConstants* GameConstants)
      {
      	if (!Tank || !Tank->HasAuthority() || !GameConstants || !Tank->GetWorld())
      	{
      		return;
      	}

      	const AGameStateBase* GameState = Tank->GetWorld()->GetGameState();
      	const float Now = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
      	if (Now < Tank->GetArmorBreakCooldownEndServerTime())
      	{
      		return;
      	}

      	Tank->SetArmorBreakCooldownEndServerTime(Now + GameConstants->ArmorBreakCooldownSeconds);
      	Tank->PlayCastMontage(Tank->GetArmorBreakCastMontage());

      	// Re-validate the intent (CLAUDE.md §4.1): the client only ever sends something it clicked,
      	// but the server owns the decision. Armor Break only affects an ACoopMonsterCharacter.
      	ACoopMonsterCharacter* MonsterTarget = Cast<ACoopMonsterCharacter>(Target);
      	if (!MonsterTarget)
      	{
      		return;
      	}

      	const float DistSq = FVector::DistSquared(Tank->GetActorLocation(), MonsterTarget->GetActorLocation());
      	if (DistSq > FMath::Square(GameConstants->ArmorBreakCastRangeUnits))
      	{
      		return;
      	}

      	MonsterTarget->ApplyStatusTag(CoopGameplayTags::Status_Broken, GameConstants->BrokenDurationSeconds);
      }
      ```

### P1.6 — `CoopDamageAbilities`: Execution retrofit + Overload

- [x] `CoopDamageAbilities.h` — replace the `ResolveExecution` declaration + comment, and add
      `ResolveOverload`:
      ```cpp
      class ACoopCharacter;
      class UGameConstants;

      namespace CoopDamageAbilities
      {
      	// Damage's Execution finisher (docs/abilities.md's physical branch of Execution/Overload).
      	// Target is the local player's click-selected actor, forwarded from
      	// ACoopPlayerController::Server_ActivateExecution and re-validated here: it must be an
      	// ACoopMonsterCharacter within ExecutionCastRangeUnits currently holding
      	// Status.Vulnerable.Physical. On a valid hit, consumes that tag and deals ExecutionDamageAmount.
      	// Whiffs silently (cooldown still consumes) otherwise. As of now the only writer of
      	// Status.Vulnerable.Physical is the dev-only ACoopPlayerController::ApplyTestVulnerable
      	// (Scene 5 "The Heart" doesn't exist yet).
      	//
      	// Retrofitted 2026-09-03 from an implicit "nearest vulnerable monster in range" search to an
      	// explicit target (DECISIONS.md "Target-required abilities need a click-selected target").
      	// Speed / Stabilize were deliberately NOT retrofitted -- they keep their auto-search.
      	void ResolveExecution(ACoopCharacter* DamageDealer, AActor* Target, const UGameConstants* GameConstants);

      	// Ability kit expansion. Damage's second ability -- the magic branch, identical to
      	// ResolveExecution but keyed to Status.Vulnerable.Magic and the Overload* constants. Reading
      	// which branch (Physical vs Magic) is open and picking the right key is The Heart's actual
      	// test (docs/abilities.md) -- not exercised until that scene, but the ability is fully
      	// functional now against a dev-granted tag.
      	void ResolveOverload(ACoopCharacter* DamageDealer, AActor* Target, const UGameConstants* GameConstants);
      }
      ```
- [x] `CoopDamageAbilities.cpp` — **DONE, implemented as two fully spelled-out functions** (no
      `TFunctionRef` helper — §4.6 / §2 favour the explicit copy; the two Damage functions now
      mirror `ApplyShield` / `ResolveArmorBreak`'s shape exactly). Each: null/authority/world guard →
      cooldown gate → `Set…CooldownEndServerTime` + `PlayCastMontage` on any gate-cleared cast →
      `Cast<ACoopMonsterCharacter>(Target)` re-validation → range check → required-tag check →
      `RemoveStatusTag` + `ApplyDamage`. `ResolveExecution` keyed to `Status.Vulnerable.Physical` +
      the `Execution*` constants; `ResolveOverload` to `Status.Vulnerable.Magic` + `Overload*`.
- [x] `CoopDamageAbilities.cpp` — confirm includes cover `CoopMonsterCharacter.h`,
      `CoopHealthComponent.h`, `GameConstants.h`, `CoopGameplayTags.h`, `GameStateBase.h`,
      `Animation/AnimMontage.h` (already mostly present — it currently includes the first four and
      `EngineUtils.h`).

### P1.7 — `CoopPlayerController`: toast, Execution gate, Armor Break / Overload, dev exec

- [x] `CoopPlayerController.h` — add to the `public:` section near the other ability wrappers:
      ```cpp
      // Ability kit expansion. Bound to IA_ArmorBreak (E) in BP_PlayerCharacter's EventGraph, same
      // thin-wrapper-around-a-Server-RPC shape as ActivateShield. Target-required: reads
      // GetCurrentTargetActor(); null -> ShowToast + return (no RPC), non-null -> Server RPC.
      UFUNCTION(BlueprintCallable, Category = "Abilities")
      void ActivateArmorBreak();

      UFUNCTION(Server, Reliable)
      void Server_ActivateArmorBreak(AActor* Target);

      // Ability kit expansion. Bound to IA_Overload (E). Damage-only. Same target-required shape.
      UFUNCTION(BlueprintCallable, Category = "Abilities")
      void ActivateOverload();

      UFUNCTION(Server, Reliable)
      void Server_ActivateOverload(AActor* Target);

      // Ability kit expansion. Local, cosmetic (CLAUDE.md §4.2): shows a centre-screen message via
      // UCoopToastWidget. Called from the target-required ability wrappers when the ability is
      // pressed with no target selected. Not replicated -- each client's own missed-input feedback.
      void ShowToast(const FText& Message);

      FText GetPendingToastText() const { return PendingToastText; }
      float GetPendingToastStartTime() const { return PendingToastStartTime; }

      // Dev/test only (CLAUDE.md §7), mirror of ApplyTestVulnerable: grants the nearest
      // ACoopMonsterCharacter Status.Vulnerable.Magic so Overload can be tested before "The Heart"
      // (Scene 5) exists. DELETE both when that scene lands, alongside ApplyTestVulnerable.
      UFUNCTION(Exec)
      void ApplyTestVulnerableMagic();

      UFUNCTION(Server, Reliable)
      void Server_ApplyTestVulnerableMagic();
      ```
- [x] `CoopPlayerController.h` — **change** the existing `Server_ActivateExecution` declaration:
      ```cpp
      UFUNCTION(Server, Reliable)
      void Server_ActivateExecution(AActor* Target);
      ```
- [x] `CoopPlayerController.h` — add to the `private:` section near the other widget pairs:
      ```cpp
      // Ability kit expansion: WBP_Toast, same create-once-in-BeginPlay / leave-in-viewport pattern
      // as every other HUD widget. UCoopToastWidget reads PendingToast* below and fades itself.
      UPROPERTY(EditDefaultsOnly, Category = "UI")
      TSubclassOf<UUserWidget> ToastWidgetClass;

      UPROPERTY()
      TObjectPtr<UUserWidget> ToastWidget;

      // Local, non-replicated toast state -- what UCoopToastWidget::NativeTick reads. StartTime is
      // GetWorld()->GetTimeSeconds() at ShowToast(); -1 means "nothing shown yet".
      FText PendingToastText;
      float PendingToastStartTime = -1.0f;
      ```
- [x] `CoopPlayerController.cpp` — includes: add `#include "Core/CoopToastWidget.h"` (only if a
      forward decl isn't enough — the `CreateWidget<UUserWidget>` call doesn't need the concrete
      type, so likely no include needed; add `#include "Engine/World.h"` if `GetTimeSeconds` isn't
      already transitively available).
- [x] `CoopPlayerController.cpp` `BeginPlay` — after the `ActionBarWidget` create block (or anywhere
      in the widget-creation run):
      ```cpp
      // Ability kit expansion: centre-screen toast ("Please choose a target"). Self-gates via
      // RenderOpacity in its own NativeTick -- created here with nothing to show.
      if (ToastWidgetClass)
      {
      	ToastWidget = CreateWidget<UUserWidget>(this, ToastWidgetClass);
      	if (ToastWidget)
      	{
      		ToastWidget->AddToViewport();
      	}
      }
      ```
- [x] `CoopPlayerController.cpp` — add `ShowToast`:
      ```cpp
      void ACoopPlayerController::ShowToast(const FText& Message)
      {
      	PendingToastText = Message;
      	PendingToastStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
      }
      ```
- [x] `CoopPlayerController.cpp` — **replace** `ActivateExecution` + `Server_ActivateExecution_Implementation`:
      ```cpp
      void ACoopPlayerController::ActivateExecution()
      {
      	// Target-required (DECISIONS.md "Target-required abilities need a click-selected target").
      	// Gate client-side: no point sending an RPC the server will only reject for a missing target.
      	AActor* Target = GetCurrentTargetActor();
      	if (!Target)
      	{
      		ShowToast(NSLOCTEXT("CoopAbilities", "ChooseTarget", "Please choose a target"));
      		return;
      	}
      	Server_ActivateExecution(Target);
      }

      void ACoopPlayerController::Server_ActivateExecution_Implementation(AActor* Target)
      {
      	// Execution is Damage-only.
      	const ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>();
      	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(GetPawn());
      	if (!CoopPS || !CoopCharacter || CoopPS->GetRole() != EPlayerRole::Damage)
      	{
      		return;
      	}

      	// Build 1, M9: Downed characters can't use abilities (CLAUDE.md §6.6).
      	if (CoopCharacter->HasStatusTag(CoopGameplayTags::Status_Downed))
      	{
      		return;
      	}

      	// Target came from the client as intent -- the namespace re-validates type/range/tag.
      	CoopDamageAbilities::ResolveExecution(CoopCharacter, Target, GameConstants);
      }
      ```
- [x] `CoopPlayerController.cpp` — add `ActivateArmorBreak` / `Server_ActivateArmorBreak` (role gate
      = Tank), `ActivateOverload` / `Server_ActivateOverload` (role gate = Damage). Both wrappers are
      byte-for-byte the `ActivateExecution` shape (gate → `ShowToast` → return, else Server RPC);
      both `_Implementation`s are the `Server_ActivateExecution_Implementation` shape with their own
      role check, calling `CoopTankAbilities::ResolveArmorBreak` / `CoopDamageAbilities::ResolveOverload`.
- [x] `CoopPlayerController.cpp` — add `ApplyTestVulnerableMagic` / `Server_ApplyTestVulnerableMagic`
      as a copy of `ApplyTestVulnerable` / `Server_ApplyTestVulnerable`, swapping
      `Status_Vulnerable_Physical` → `Status_Vulnerable_Magic` and the log string.

### P1.8 — `CoopUnitFrameWidget` status line

- [x] `CoopUnitFrameWidget.cpp` `NativeTick` — in the `if (StatusText)` block, after the
      `Status_Vulnerable_Physical` line:
      ```cpp
      if (ActorHasTag(Subject, CoopGameplayTags::Status_Broken))            { Parts.Add(TEXT("BROKEN")); }
      if (ActorHasTag(Subject, CoopGameplayTags::Status_Vulnerable_Magic))  { Parts.Add(TEXT("VULNERABLE-M")); }
      ```
      (and rename the existing `Status_Vulnerable_Physical` label from `"VULNERABLE"` to
      `"VULNERABLE-P"` so the two branches read distinctly — a 1-word cosmetic change).

### P1.9 — `CoopAbilitySlotWidget` generalisation

- [x] `CoopAbilitySlotWidget.h` — rename the two getters and add the `SlotIndex` param:
      ```cpp
      // Cooldown end (absolute server time) / duration for the slot-SlotIndex ability of Role, read
      // off the local player's own pawn. -1 / 0 if this slot has no implemented ability. One explicit
      // (Role, SlotIndex) case each -- no generic map (CLAUDE.md §4.6).
      float GetSlotCooldownEndServerTime(EPlayerRole Role, int32 InSlotIndex) const;
      float GetSlotCooldownDurationSeconds(EPlayerRole Role, int32 InSlotIndex) const;
      ```
- [x] `CoopAbilitySlotWidget.cpp` — kit table: flip `bImplemented` `false → true` on
      `Tank[1]` (`Armor Break`) and `Damage[1]` (`Overload`).
- [x] `CoopAbilitySlotWidget.cpp` — `GetSlotCooldownEndServerTime`: for Tank add
      `InSlotIndex == 1 → Char->GetArmorBreakCooldownEndServerTime()`; for Damage add
      `InSlotIndex == 1 → Char->GetOverloadCooldownEndServerTime()`; every other (Role, index) that
      isn't an implemented ability returns `-1`. Same structure for `GetSlotCooldownDurationSeconds`
      against `ArmorBreakCooldownSeconds` / `OverloadCooldownSeconds`.
- [x] `CoopAbilitySlotWidget.cpp` `NativeTick` — keybind badge:
      ```cpp
      if (KeybindText)
      {
      	// Slot 0 -> Q, slot 1 -> E (DECISIONS.md "The Q ability per role" + this pass). Only shown
      	// for an implemented slot.
      	const bool bShowKey = Info.bImplemented && (SlotIndex == 0 || SlotIndex == 1);
      	KeybindText->SetText(FText::FromString(SlotIndex == 0 ? TEXT("Q") : TEXT("E")));
      	KeybindText->SetVisibility(bShowKey ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
      }
      ```
- [x] `CoopAbilitySlotWidget.cpp` `NativeTick` — cooldown block: change the guard
      `if (SlotIndex == 0 && Info.bImplemented)` → `if (Info.bImplemented)` and the two calls to
      `GetSlotCooldownEndServerTime(Role, SlotIndex)` / `GetSlotCooldownDurationSeconds(Role, SlotIndex)`.

### P1.10 — Re-read every touched file on disk, verify symbols

- [x] Re-read all P1 files **on disk** (not just this plan). Confirm: `EMatchPhase` unchanged;
      `CoopMonsterCharacter::HasStatusTag`/`ApplyStatusTag`/`RemoveStatusTag`/`GetHealthComponent`
      all exist (they do — used by the existing Execution); `UCoopHealthComponent::ApplyDamage`
      exists; `Build.cs` already has `UMG` / `GameplayTags` / `SlateCore` / `Engine` (it does — the
      action bar and cursor features needed the same). No `Build.cs` change.

---

## P2 — One full external rebuild from a closed editor  *(✅ DONE 2026-09-03 — build + symbol-load both verified)*

New `UCoopToastWidget` UCLASS + 2 new `UPROPERTY`s on the already-loaded `ACoopCharacter` + a
**changed** `Server_ActivateExecution` UFUNCTION signature + 4 new UFUNCTION RPCs →
**Live Coding is unsafe** (`DECISIONS.md` "Live Coding must not be used to add a new UCLASS…").
Same procedure as this file's previous P4 and `cursor_progress.md` P2.

- [x] **P2.1** — Confirmed no `UnrealEditor` process (`tasklist`), 2026-09-03.
- [x] **P2.2** — Re-read every P1 delta on disk against the plan: `CoopToastWidget.h`/`.cpp` (new,
      `NativeTick` reads `GetPendingToastText`/`GetPendingToastStartTime`, `RenderOpacity` fade);
      tags declare+define; `GameConstants.h` +7 fields; `CoopCharacter` +2 `UPROPERTY(Replicated,
      VisibleInstanceOnly)` cooldowns + 2 montage refs + 2 `DOREPLIFETIME_CONDITION`; `CoopTankAbilities`
      `ResolveArmorBreak(AActor*)`; `CoopDamageAbilities` `ResolveExecution(AActor*)` retrofit +
      `ResolveOverload(AActor*)` (two explicit functions, `EngineUtils.h` dropped); `CoopPlayerController`
      3 target-gated wrappers (`AActor* Target = GetCurrentTargetActor()` → `ShowToast` or Server RPC),
      `Server_ActivateExecution` signature change, `ShowToast`, `ApplyTestVulnerableMagic`;
      `CoopUnitFrameWidget` status line +`BROKEN`/`VULNERABLE-P`/`VULNERABLE-M`; `CoopAbilitySlotWidget`
      `GetSlotCooldown*(Role, SlotIndex)` + kit-table flips + Q/E badge. All includes present. No issues.
- [x] **P2.3** — Ran `Build.bat Unreal_first_GameEditor Win64 Development -project=... -waitmutex`.
      **Result: Succeeded, exit 0**, ~69s, 16 actions. All 12 changed `.cpp`s compiled (incl.
      `CoopToastWidget.cpp` new); UHT wrote **8 generated files** — `CoopToastWidget.generated.h`
      (3,465 B) + `.gen.cpp` (10,629 B) fresh @ 15:59, plus `CoopCharacter`/`CoopPlayerController`
      regenerated for the new reflected members. `UnrealEditor-Unreal_first_Game.dll` relinked →
      **830,464 B, Sep 3 16:00** (was 799,744). `.target` rebuilt. **Zero warnings or errors.** Native
      tags emit no `.gen.cpp`, but `CoopGameplayTags.cpp` + the two ability `.cpp`s that reference
      `Status_Broken`/`Status_Vulnerable_Magic` compiled and linked with no undefined-symbol error —
      proof the declare/define pair is correct.
- [x] **P2.4** — Editor already reopened by the user; `unreal-mcp` connected (`list_toolsets` returned
      the full roster, `IsPIERunning` → false). Confirmed 2026-09-03.
- [x] **P2.5** — New symbols confirmed loaded (2026-09-03): `ObjectTools.search_subclasses` on
      `/Script/UMG.UserWidget` filter "Toast" → `[/Script/Unreal_first_Game.CoopToastWidget]`;
      `GameplayTagsToolset.ListTags("Status")` → includes `Status.Broken`, `Status.Vulnerable.Magic`
      (plus the pre-existing `Status.Vulnerable` implicit parent, `.Physical`, etc.);
      `BP_PlayerController` CDO `toastWidgetClass = "None"` (expected — P3.5 wires it) with all 7 prior
      widget-class refs (`matchTimer`/`roleSelect`/`prepArenaHUD`/`actionBar`/`targetFrame`/
      `partyFrame`/`targetRing`) + `gameConstants = DA_GameConstants` intact.

---

## P3 — Content (all `unreal-mcp`)

- [x] **P3.1** (2026-09-03) — `IA_ArmorBreak` + `IA_Overload` duplicated from `IA_Shield` to
      `/Game/Input/Actions/`. `valueType` already `Boolean` (inherited); `triggers` trimmed from
      `[InputTriggerPressed_0, InputTriggerReleased_0]` → `[InputTriggerPressed_0]` on each (pure
      size-decrease `set_properties`, no element change — no array gotcha). Saved, `is_dirty == false`
      for both, `.uasset`s on disk @ 16:35 (1488 / 1478 B, matching `IA_Select`'s trimmed size).
- [x] **P3.2** (2026-09-03) — `IMC_Default.defaultKeyMappings.mappings`: read full array (18 entries),
      confirmed **no `E` mapping** (Shield/Stabilize/Speed/Dash/Execution all on `Q`, Select on LMB).
      Appended `E → IA_ArmorBreak` + `E → IA_Overload` via a `ProgrammaticToolset` read-deepcopy-
      append-write (append-only, no element change). Re-read: `count 18 → 20`, **all 18 prior entries
      byte-identical** (`prior_diff: null` — every `InputModifierSwizzleAxis`/`Negate`/`DeadZone`/
      `Scalar` subobject refPath intact), 2 new entries exactly as intended. Saved, `is_dirty == false`,
      on disk @ 16:37.
- [x] **P3.3** (2026-09-03) — `DA_GameConstants` (direct `GameConstants` C++ instance): the 7 fields
      already *read* at target (inherited from the P1/P2-built C++ CDO defaults). `set_properties`
      wrote them as explicit instance overrides anyway (mirror-to-asset per CLAUDE.md §10) →
      `armorBreakCooldownSeconds` 10, `armorBreakCastRangeUnits` 800, `brokenDurationSeconds` 6,
      `overloadCooldownSeconds` 6, `overloadCastRangeUnits` 400, `overloadDamageAmount` 100,
      `toastDurationSeconds` 2. Saved, `is_dirty == false`, re-read confirms, on disk @ 16:37.
- [x] **P3.4** (2026-09-03) — `WBP_Toast` created (`/Game/Blueprints/UI/WBP_Toast`, parent
      `CoopToastWidget`). Tree: `RootCanvas` (CanvasPanel, `Visibility = HitTestInvisible` — whole
      subtree non-hit-testable) → `MessageText` (TextBlock; canvas `layoutData` anchors min/max
      `(0.5,0.5)` + alignment `(0.5,0.5)` + offsets 0, `bAutoSize = true`; `Justification = Center`,
      `Font` Roboto Bold size 28, `outlineSettings.outlineSize = 2` black + `ShadowOffset (2,2)` /
      `ShadowColorAndOpacity` black α0.75 for readability, `Text = ""`). `ToggleWidgetAsVariable true`
      → `MessageText` now `bIsVariable + bInherited` (resolves the C++ `BindWidgetOptional MessageText`).
      `CompileWidgetBlueprint → true`, `LogBlueprint` clean (no errors/warnings). Saved,
      `is_dirty == false`, on disk @ 16:40.
- [x] **P3.5** (2026-09-03) — CDO wiring done:
      - `Default__WBP_Toast_C.gameConstants` → `/Game/Data/DA_GameConstants.DA_GameConstants` (read
        back confirmed).
      - `Default__BP_PlayerController_C.toastWidgetClass` → `/Game/Blueprints/UI/WBP_Toast.WBP_Toast_C`.
      - `compile_blueprint` on `BP_PlayerController`, then re-read CDO: `toastWidgetClass` now set;
        `matchTimer` / `roleSelect` / `prepArenaHUD` / `actionBar` / `targetFrame` / `partyFrame` /
        `targetRing` widget-class refs + `gameConstants` **all unchanged** (no silent reset).
      - Both assets saved, `is_dirty == false`, on disk @ 16:41.
- [x] **P3.6** (2026-09-03) — `BP_PlayerCharacter` EventGraph: 2 new chains built via `create_node` /
      `connect_pins` (a `ProgrammaticToolset` batch of those two tools — **not** `write_graph_dsl`),
      each a byte-for-byte mirror of the existing `IA_Execution` chain (the target-required template):
      `Input|EnhancedActionEvents|IA_ArmorBreak` `.Triggered` → `CastToCoopPlayerController`
      (`Object` ← `Pawn|GetController` ← `Variables|Getareferencetoself`) `.then`/`.AsCoopPlayerController`
      → `Abilities|ActivateArmorBreak`; same for `IA_Overload` → `Abilities|ActivateOverload`.
      `CastFailed` left open on both. Both chains re-verified via `get_connected_subgraph` (all 5
      links per chain match the Execution template pin-for-pin). `compile_blueprint`
      (`warnings_as_errors: true`) clean — `LogBlueprint` / `LogK2Compiler` no errors/warnings. New
      nodes: `EnhancedInputAction_10/11`, `Self_6/7`, `CallFunction_24/26` (GetController),
      `DynamicCast_6/7`, `CallFunction_25/27` (Activate). Saved, `is_dirty == false`, on disk @ 16:45.
- [x] **P3.7** (2026-09-03) — `BP_PlayerCharacter` CDO `armorBreakCastMontage` + `overloadCastMontage`
      both wired to `/Game/Characters/Mannequins/Anims/Unarmed/AbilityMontages/MM_Execution_Montage`
      (only 5 ability montages exist — reusing the Execution strike for both new offensive
      target-abilities; Overload literally mirrors Execution). `compile_blueprint` clean, re-read
      confirms, saved with P3.8. The existing 5 (`shield`/`speed`/`dash`/`stabilize`/`execution`)
      untouched.
- [x] **P3.8** (2026-09-03) — `save_assets([])` + per-asset `is_dirty` check: all 7 touched assets
      `false` — `IA_ArmorBreak` / `IA_Overload` (@16:35), `IMC_Default` / `DA_GameConstants` (@16:37),
      `WBP_Toast` / `BP_PlayerController` (@16:41), `BP_PlayerCharacter` (@16:46). All on disk with
      fresh mtimes.

> **P3 COMPLETE (2026-09-03).** All content wired via `unreal-mcp`, every asset compiled + saved +
> `is_dirty == false` + mtime-verified. Next: **P4 — 5-client PIE verification.**

---

## P4 — 5-client PIE verification

Solo agentic pass via `unreal-mcp` where possible; a few items need a human eyeball (flagged).
Editor throttle (`bThrottleCPUWhenNotForeground` / `bAllowSlateThrottling`) **off** for the run,
**restored** after (`DECISIONS.md`). Widen the relevant `*CooldownSeconds` on `DA_GameConstants`
*before* the casts to be verified, restore after (the "timer captures the value" gotcha).

> **P4 STATUS (2026-09-03): P4.1 done, P4.2 core done. P4.3–P4.8 need a hands-on PIE session** —
> they require selecting a *monster* as the target (party-row click can't; needs a real LMB
> world-click, which the agentic tools can't position) plus reading replicated cooldown state
> across the 5 separate PIE client worlds and eyeballing the action-bar tiles / toast fade. The
> agentic smoke pass proved the plumbing (PIE boots 5-up, full phase loop clean, all new input
> actions + EventGraph chains + C++ wrappers fire with zero errors, the toast renders). What's left
> is behavioural verification best done by a human driving the 5 windows. `DA_GameConstants`
> cooldowns were **not** widened (no targeted casts were run); nothing to restore there.

- [x] **P4.1** (2026-09-03, agentic smoke pass) — `StartPIE` on `Lvl_ThirdPerson` launched a full
      **5-window PIE** (`Preview [NetMode: Server 0]` + `Client 1..4` — the project's own
      `LevelEditorPlaySettings` drives the 5-client listen-server setup; the "Not enough login
      credentials" warning is non-fatal, all 5 launched). Log: `OnRosterComplete` → RoleSelect (10s)
      → `ResolveRoleSelection: Prep phase started (10s)` → `OnPrepPhaseExpired: HoldTheGate phase
      started` — clean progression. Roles auto-resolved to **5 distinct**: C_0 Runner / C_1 Tank /
      C_2 Control / C_3 Damage / C_4 Support. **Zero `Error:` / `Accessed None` / blueprint script
      errors** across the session for any new code (only the DECISIONS.md-documented harmless
      `LogUObjectGlobals: Failed to find object 'Class …IA_ArmorBreak…InputTriggerPressed_0'`
      fallback-resolution noise from the P3.1/P3.2 subobject-ref writes, plus pre-existing
      `GameFeatureData` config notes). Throttle flipped off for the run, **restored to `true`/`true`**
      after; PIE stopped; no stray `UnrealEditor` processes.
- [x] **P4.2** (2026-09-03, **partial** — core confirmed, RPC-negative + fade need a hands-on pass) —
      On the listen-server client (`BP_PlayerController_C_0`, Runner — the client wrapper has **no**
      role gate, so any role exercises the toast path), `SlateInspector.WaitFor("Please choose a
      target")` → **`false` before**, **`true` after** `TriggerInputAction(IA_Execution)` with no
      target selected. Confirms the whole client chain: `IA_Execution` EventGraph binding →
      `ActivateExecution()` → `GetCurrentTargetActor()` null → `ShowToast()` sets `PendingToastText`
      → `UCoopToastWidget::NativeTick` reads it → `MessageText` renders in the viewport.
      `TriggerInputAction(IA_ArmorBreak)` + `(IA_Overload)` on the same client also fired with **zero
      errors** — proving the 2 new `IA_*` assets, the 2 new `BP_PlayerCharacter` EventGraph chains,
      and the `ActivateArmorBreak`/`ActivateOverload` C++ wrappers all load and execute.
      **Still to verify by hand:** "no `Server_Activate*` RPC sent" (no log signal — `ShowToast`
      doesn't log; needs a breakpoint or a temp log), and the `RenderOpacity` 1→0 fade over ~2 s
      (`NativeTick` never clears `MessageText`, so `WaitFor` text-gone can't see the fade — needs a
      screenshot/eyeball). `PendingToastText`/`PendingToastStartTime` are **plain C++ members, not
      `UPROPERTY`** (and the getters aren't `UFUNCTION`), so they are **not reflection-readable** —
      the plan's "reflection read" check for P4.2 isn't possible as-built; `SlateInspector.WaitFor`
      on the rendered text is the substitute.
- [ ] **P4.3** — **Execution with a target.** Grant a monster `Status.Vulnerable.Physical`
      (`ApplyTestVulnerable`), click it (party-row click can't select a monster — use a world-click
      or a direct `SetCurrentTarget` via `unreal-mcp` if a positioned click can't be simulated),
      `IA_Execution` → monster loses the tag + takes `ExecutionDamageAmount` + `executionCooldownEndServerTime`
      set. Same cast with the monster **out of `ExecutionCastRangeUnits`** → cooldown consumes, no
      damage (silent whiff).
- [ ] **P4.4** — **Armor Break.** Tank client, target a monster, `IA_ArmorBreak` (E) → monster gains
      `Status.Broken` for `BrokenDurationSeconds` → **target frame status line shows `BROKEN`** →
      tag expires on its own. `armorBreakCooldownEndServerTime` set; second press within cooldown →
      no-op. No target → toast, no RPC.
- [ ] **P4.5** — **Overload.** `ApplyTestVulnerableMagic` grants the nearest monster
      `Status.Vulnerable.Magic` → target frame shows `VULNERABLE-M` → Damage client targets it,
      `IA_Overload` (E) → tag consumed + `OverloadDamageAmount` dealt. `E` on a **Tank** does nothing
      (role gate); `E` on **Damage** does Overload, `E` on **Tank** does Armor Break — one key, two
      meanings, no conflict.
- [ ] **P4.6** — **Cooldown replication (`COND_OwnerOnly`).** Read `armorBreakCooldownEndServerTime` /
      `overloadCooldownEndServerTime` for a casting pawn across all 5 PIE worlds: real value on the
      server world + that pawn's own client, `-1` on the other 3. (Same check the action-bar P9.5
      did for the original 5.)
- [ ] **P4.7** — **Action bar.** Tank bar shows **2 live tiles** — `Shield` (Q badge) + `Armor Break`
      (**E badge**, own cooldown sweep on cast). Damage bar — `Execution` (Q) + `Overload` (E).
      Other roles unchanged (Support 2 tiles, Runner 3, Control 3, only slot 0 live).
- [ ] **P4.8** — **Speed / Stabilize untouched.** Support `IA_Speed` still buffs the nearest ally
      with no target selected; Control `IA_Stabilize` still upgrades the nearest Shielded Tank. Run
      one Hold-the-Gate Fortress execution end-to-end to confirm the scene flow is unchanged.
- [~] **P4.9** — Cleanup (partial, done for the smoke pass): `StopPIE` ✓; editor throttle restored to
      `true`/`true` ✓; no stray `UnrealEditor` processes ✓; no TEMP `UE_LOG`s were added; no function
      bodies touched. `DA_GameConstants` cooldowns were never widened. **Re-run the full checklist
      after the hands-on P4.3–P4.8 session.**
- [ ] **P4.10** — **Bake rebuild** (closed editor `Build.bat`) **only if P4 produced C++ changes**.
      P1's C++ is already baked by P2; P3 is pure content. If P4 surfaced a fix, it needs its own
      closed-editor rebuild — then, not pre-emptively.

**Needs a human (can't simulate a positioned world-click / pure visual):** the toast actually
reads legibly centre-screen and fades smoothly; a real LMB world-click on a monster selects it and
Armor Break / Overload / Execution then fire; `E` feels right as the second-ability key.

---

## P5 — `DECISIONS.md` + `docs/abilities.md`

> **P5 COMPLETE (2026-09-03).** Docs written against the locked-and-implemented design; independent
> of the outstanding hands-on P4 items (those verify behaviour, not decisions).

- [x] **P5.1** (2026-09-03) — New `DECISIONS.md` entry **"Target-required abilities need a
      click-selected target"** appended (end of file). Covers: the client-first gate + server
      re-validation (§4.1) as the first use of the parked `GetCurrentTargetActor()` seam; the 3
      abilities (Execution retrofit / Armor Break / Overload) & why only these 3; Speed + Stabilize
      **deliberately not** retrofitted (Hold the Gate / Fortress is load-bearing); `Server_Activate*
      (AActor* Target)` signature changes; `Status.Broken` has no reader yet (accepted);
      `Status.Vulnerable.Magic` dev-granted via `ApplyTestVulnerableMagic` (delete both stubs at
      Scene 5); `UCoopToastWidget` (`RenderOpacity` fade off one stamp, never self-hides via
      Visibility, `BindWidgetOptional`, `NativeTick` doesn't clear the text); Link/Channel/Mind
      Fracture + Carry/Chain/Taunt deferred/out; **two explicit `CoopDamageAbilities` functions, no
      `TFunctionRef` helper**; the `E`-key role-gating pattern; the one closed-editor `Build.bat`;
      and the smoke-pass verification + what's still owed a hands-on pass.
- [x] **P5.2** (2026-09-03) — `DECISIONS.md` "The Q ability per role": Execution table row updated to
      "**click-selected** `ACoopMonsterCharacter` … if it holds the tag & is in range" (was
      "nearest … holding the tag"); added a bullet noting the 2026-09-03 retrofit + that Speed/
      Stabilize were not; added an addendum to the `ApplyTestVulnerable` paragraph for the twin
      `ApplyTestVulnerableMagic` stub (delete both together at Scene 5).
- [x] **P5.3** (2026-09-03) — `docs/abilities.md`: **"Prototype status (2026-09-03)"** italic lines
      added to **Armor Break** (Tank/`E`, target-required, `Status.Broken`/`BROKEN`, no consumer
      until The False King), **Overload** (Damage/`E`, target-required, mirrors Execution,
      `Status.Vulnerable.Magic` dev-granted until The Heart), and **Execution** (Damage/`Q`,
      retrofitted auto→click-select). No spec or tag-glossary changes — all three abilities and both
      tags were already fully specced.

---

## Self-review against the design (done 2026-09-03 at plan-write time)

- **Scope = Armor Break + Overload only:** P1.5 / P1.6 build exactly those two; Link/Channel/Mind
  Fracture appear only in P5.1 as "deferred"; Carry/Chain/Taunt absent entirely. ✓
- **Execution retrofit, Speed/Stabilize untouched:** P1.6 changes `ResolveExecution`'s signature +
  drops its loop; `CoopSupportAbilities` / `CoopControlAbilities` are not in the file map at all. ✓
- **Client gate + server re-validation:** P1.7 `ActivateExecution` returns before the RPC on null
  target; `Server_ActivateExecution_Implementation` + the namespaces re-cast/range/tag-check. ✓
- **Toast never self-hides via Visibility:** P1.4 `NativeTick` sets `HitTestInvisible` unconditionally,
  only ever touches `RenderOpacity`. ✓ (P9 gotcha)
- **No Designer bindings:** `MessageText` is `BindWidgetOptional`, written from `NativeTick`. ✓
- **No new magic numbers:** P1.2 puts all 7 tunables in `GameConstants.h`; P3.3 mirrors to the
  asset. Colours/labels stay hardcoded-cosmetic (the `GetColorForPlayerId` precedent). ✓
- **Tags added to the doc first:** `Status.Broken` + `Status.Vulnerable.Magic` are already in
  `docs/abilities.md`'s glossary — P1.1 only declares them. ✓ §4.6
- **No generic ability system:** `ResolveArmorBreak` / `ResolveOverload` are explicit namespace
  functions, role-gated RPC pairs, mirroring `ApplyShield` / `ResolveExecution` exactly. P1.6 was
  implemented as two fully spelled-out functions (the `TFunctionRef` helper the plan sketched was
  dropped — §4.6 / §2 favour the explicit copy). ✓ §4.6
- **Rebuild discipline:** new UCLASS + new UPROPERTYs + changed RPC signature → P2 is one full
  external `Build.bat` from a closed editor; all C++ batched in P1. ✓
- **Placeholder scan:** every C++ step has full code or an exact, unambiguous snippet + location;
  the two "same shape as X" steps (P1.7 Armor Break/Overload wrappers, P1.7 dev exec) name the exact
  function they copy. No "TBD" / "handle edge cases". ✓
- **Hold the Gate risk:** P4.8 explicitly re-runs a Fortress execution to confirm Speed/Stabilize
  and the scene are unaffected. ✓

---

## Log
(Newest at the bottom. One line per completed step.)

- **Plan written (2026-09-03).** Design agreed in chat: 4 clarifying answers — scope = 5 synergy
  abilities narrowed by "only build what fully works today" to **Armor Break + Overload**; no new
  Support Heal; centre-screen toast; **retrofit only Execution**, leave Speed/Stabilize auto. This
  file cleared (its action-bar contents are complete, preserved in `DECISIONS.md`) and replaced with
  the plan above. Next: P1 (all C++, one batch), then P2 (one rebuild).

- **P1 done — all C++ in one uncommitted batch (2026-09-03).** Self-reviewed on disk; not yet built.
  - **`CoopGameplayTags.h/.cpp`** — `Status_Broken` ("Status.Broken") + `Status_Vulnerable_Magic`
    ("Status.Vulnerable.Magic") declared + defined. Both were already in `docs/abilities.md`'s
    glossary — nothing invented.
  - **`GameConstants.h`** — 7 new fields: `ArmorBreakCooldownSeconds` 10, `ArmorBreakCastRangeUnits`
    800, `BrokenDurationSeconds` 6, `OverloadCooldownSeconds` 6, `OverloadCastRangeUnits` 400,
    `OverloadDamageAmount` 100 (`Category = "Abilities"`), `ToastDurationSeconds` 2 (`Category = "UI"`,
    placed beside `MatchTimerDisplayUpdateIntervalSeconds`). Also updated the stale `TestVulnerable*`
    comment to mention `ApplyTestVulnerableMagic`.
  - **`CoopCharacter.h/.cpp`** — `ArmorBreakCooldownEndServerTime` / `OverloadCooldownEndServerTime`
    (`UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")` + getters/setters + 2
    `DOREPLIFETIME_CONDITION(… COND_OwnerOnly)`), and `ArmorBreakCastMontage` / `OverloadCastMontage`
    (`EditDefaultsOnly` + getters) — exact shape of the existing five.
  - **`CoopToastWidget.h/.cpp`** (new) — `UUserWidget` base; `NativeTick` stays `HitTestInvisible`,
    reads the controller's `GetPendingToastText()` / `GetPendingToastStartTime()`, fades
    `RenderOpacity` (full for the first 60% of `ToastDurationSeconds`, linear to 0 over the last 40%)
    off `GetWorld()->GetTimeSeconds() - StartTime`. Own `EditDefaultsOnly UGameConstants*` for the
    duration (wired on `WBP_Toast`'s CDO in P3), null → 2.0s fallback.
  - **`CoopTankAbilities.h/.cpp`** — `ResolveArmorBreak(ACoopCharacter*, AActor* Target, const
    UGameConstants*)`: authority/world guard → cooldown gate → `SetArmorBreakCooldownEndServerTime`
    + `PlayCastMontage` on any gate-cleared cast → `Cast<ACoopMonsterCharacter>` re-validation →
    range check → `ApplyStatusTag(Status_Broken, BrokenDurationSeconds)`. `+#include CoopMonsterCharacter.h`.
  - **`CoopDamageAbilities.h/.cpp`** — `ResolveExecution` signature changed to take `AActor* Target`,
    `TActorIterator` nearest-search **removed**; new `ResolveOverload` as an explicit twin (keyed to
    `Status_Vulnerable_Magic` + the `Overload*` constants). **Two fully spelled-out functions, no
    helper.** `-#include EngineUtils.h` (search loop gone).
  - **`CoopPlayerController.h/.cpp`**:
    - `ActivateExecution()` — reads `GetCurrentTargetActor()`; null → `ShowToast("Please choose a
      target")` + return (no RPC); else `Server_ActivateExecution(Target)`.
    - `Server_ActivateExecution(AActor* Target)` — signature changed; role/Downed gate unchanged,
      forwards `Target` to `ResolveExecution` which re-validates.
    - new `ActivateArmorBreak` / `Server_ActivateArmorBreak(AActor*)` (Tank), `ActivateOverload` /
      `Server_ActivateOverload(AActor*)` (Damage) — byte-for-byte the `ActivateExecution` shape.
    - `ShowToast(const FText&)` + `PendingToastText` / `PendingToastStartTime` (non-replicated) +
      getters; `ToastWidgetClass` / `ToastWidget` pair + create-and-add-to-viewport in `BeginPlay`
      (after the party frame block).
    - `ApplyTestVulnerableMagic` / `Server_ApplyTestVulnerableMagic` Exec pair — explicit copy of
      `ApplyTestVulnerable`, grants `Status_Vulnerable_Magic`. Delete both with `ApplyTestVulnerable`
      when The Heart lands.
  - **`CoopUnitFrameWidget.cpp`** — target-frame status line adds `BROKEN` + `VULNERABLE-M`, and the
    existing `VULNERABLE` label renamed `VULNERABLE-P` so the two branches read distinctly.
  - **`CoopAbilitySlotWidget.h/.cpp`** — kit table `bImplemented` flipped `true` for Tank[1]
    (Armor Break) + Damage[1] (Overload); `GetSlotZeroCooldown{EndServerTime,DurationSeconds}(Role)`
    → `GetSlotCooldown…(Role, InSlotIndex)` with explicit `InSlotIndex == 0` / `== 1` switches
    (slot 1 only Tank/Damage); keybind badge `"Q"` slot 0 / `"E"` slot 1, shown for any implemented
    slot; cooldown-sweep guard `SlotIndex == 0 && bImplemented` → `bImplemented`.
  - **No `Build.cs` change** (`UMG`/`GameplayTags`/`SlateCore`/`Engine` already cover it). No other
    C++ callers of `ResolveExecution` / `Server_ActivateExecution` (grep-confirmed). Speed / Stabilize
    (`CoopSupportAbilities` / `CoopControlAbilities`) **untouched**.
  - **Next: P2 — user closes the editor, then the full `Build.bat` rebuild** (new `UCoopToastWidget`
    UCLASS + new `UPROPERTY`s + changed RPC signature → Live Coding unsafe).

- **P2 rebuild done (2026-09-03).** Resumed session, confirmed no `UnrealEditor` process, re-read
  every P1 delta on disk against the plan (P2.2 above lists them) — all consistent, all referenced
  symbols + includes present (`Cast<ACoopMonsterCharacter>`, `GetHitResultUnderCursor`,
  `UProgressBar` methods, `GetWorld()->GetTimeSeconds`, the 7 new `GameConstants` fields, the 2 new
  `CoopCharacter` getters). Ran full external `Build.bat`: **Succeeded, exit 0**, ~69s, 16 actions,
  all 12 changed `.cpp`s compiled (`CoopToastWidget.cpp` new). UHT wrote 8 generated files —
  `CoopToastWidget.generated.h`/`.gen.cpp` (@15:59) + `CoopCharacter`/`CoopPlayerController` regen'd
  for the 2 new cooldown `UPROPERTY`s / the changed `Server_ActivateExecution` signature / 4 new
  RPCs. DLL relinked → **830,464 B, Sep 3 16:00** (was 799,744). No warnings/errors. `Status.Broken`
  / `Status.Vulnerable.Magic`: native tags emit no `.gen.cpp`, but `CoopGameplayTags.cpp` + the two
  ability `.cpp`s referencing them compiled + **linked** clean — proof the declare/define is right.
  **Next: P2.4/P2.5 need the user — reopen the editor so `unreal-mcp` reconnects and
  `search_subclasses("Toast")` / the `Status.Broken`+`Status.Vulnerable.Magic` tag list confirm the
  new symbols; then P3 (all `unreal-mcp`: `IA_ArmorBreak`/`IA_Overload`, `IMC_Default` E-mappings,
  `DA_GameConstants` 7 fields, `WBP_Toast`, CDO wiring, `BP_PlayerCharacter` EventGraph 2 chains).**

- **P2.4/P2.5 + P3 done (2026-09-03, resumed session — editor already reopened by the user, PID 25180,
  `unreal-mcp` connected, PIE not running).**
  - **P2.5** — `CoopToastWidget` subclass present (`search_subclasses` on `UserWidget`); `Status.Broken`
    + `Status.Vulnerable.Magic` both in `GameplayTagsToolset.ListTags("Status")`; `BP_PlayerController`
    CDO `toastWidgetClass = None` with all 7 prior widget refs + `gameConstants` intact — no reset.
  - **P3.1** — `IA_ArmorBreak` / `IA_Overload` duplicated from `IA_Shield`, `triggers` trimmed to
    `[InputTriggerPressed_0]` only. Saved, `is_dirty == false`, on disk.
  - **P3.2** — `IMC_Default.defaultKeyMappings.mappings` 18 → 20 via a `ProgrammaticToolset`
    read-deepcopy-append-write: appended `E → IA_ArmorBreak` + `E → IA_Overload`; **all 18 prior
    entries byte-identical** (`prior_diff: null`, every input-modifier subobject refPath intact).
  - **P3.3** — `DA_GameConstants`: 7 fields already read at target (inherited from the P2-built C++
    CDO defaults); `set_properties` wrote them as explicit instance overrides anyway (10 / 800 / 6 /
    6 / 400 / 100 / 2). Saved.
  - **P3.4** — `WBP_Toast` (parent `CoopToastWidget`): `RootCanvas` (CanvasPanel,
    `Visibility = HitTestInvisible`) → `MessageText` (TextBlock, canvas `layoutData` centre anchors +
    alignment `(0.5,0.5)` + `bAutoSize`, `Justification Center`, Roboto Bold 28, black outline +
    shadow, `Text ""`), `ToggleWidgetAsVariable true` (now `bInherited` → resolves the C++
    `BindWidgetOptional`). `CompileWidgetBlueprint → true`, `LogBlueprint` clean.
  - **P3.5** — `Default__WBP_Toast_C.gameConstants` → `DA_GameConstants`;
    `Default__BP_PlayerController_C.toastWidgetClass` → `WBP_Toast_C`; post-compile CDO re-read: all
    other widget-class refs + `gameConstants` unchanged.
  - **P3.6** — `BP_PlayerCharacter` EventGraph: 2 new chains via a `create_node`/`connect_pins` batch
    (**not** `write_graph_dsl`), each a pin-for-pin mirror of the existing `IA_Execution` chain:
    `IA_ArmorBreak.Triggered` → `CastToCoopPlayerController` (`Object` ← `GetController` ← `Self`) →
    `Abilities|ActivateArmorBreak`; same for `IA_Overload` → `ActivateOverload`. `CastFailed` open.
    Both re-verified via `get_connected_subgraph`; `compile_blueprint` (`warnings_as_errors`) clean.
    (`Variables|Getareferencetoself` is the create type_id for the Self node — `Variables|Self-Reference`
    is only the display title and fails `create_node`.)
  - **P3.7** — `armorBreakCastMontage` + `overloadCastMontage` CDO → `MM_Execution_Montage` (reuse;
    only the 5 ability montages exist).
  - **P3.8** — all 7 touched assets saved, `is_dirty == false`, mtimes on disk (16:35–16:46).

- **P4.1 + P4.2-core done (2026-09-03, same session — agentic smoke pass).** Flipped editor throttle
  (`bThrottleCPUWhenNotForeground` / `bAllowSlateThrottling`) off, `StartPIE` on `Lvl_ThirdPerson`.
  The project's `LevelEditorPlaySettings` launched a full **5-window PIE** (Server 0 + Client 1–4).
  Log showed a clean `OnRosterComplete → RoleSelect(10s) → Prep(10s) → HoldTheGate` progression;
  roles auto-resolved 5-distinct (C_0 Runner, C_1 Tank, C_2 Control, C_3 Damage, C_4 Support).
  **Zero blueprint script errors / `Accessed None`** for any new code — only the DECISIONS.md-known
  harmless `LogUObjectGlobals: Failed to find object 'Class …IA_ArmorBreak…InputTriggerPressed_0'`
  subobject-ref-write noise, plus pre-existing `GameFeatureData` config notes. On `BP_PlayerController_C_0`
  (Runner; the client wrapper has no role gate): `SlateInspector.WaitFor("Please choose a target")`
  → `false` before / `true` after `TriggerInputAction(IA_Execution)` with no target — the toast
  fires end-to-end. `TriggerInputAction(IA_ArmorBreak)` + `(IA_Overload)` on the same client also
  fired with zero errors (new `IA_*` assets + new EventGraph chains + new C++ wrappers all load &
  execute). `StopPIE`; throttle **restored to `true`/`true`**; no stray editor processes.
  Discovered: `PendingToastText`/`PendingToastStartTime` are plain (non-`UPROPERTY`) C++ members —
  not reflection-readable, so the plan's "reflection read" P4.2 check is impossible as-built;
  `SlateInspector.WaitFor` on the rendered text is the substitute.
  - **Next: a hands-on 5-window PIE session for P4.3–P4.8** — each needs a real LMB world-click to
    select a *monster* as target (agentic tools can't position a world-click) + reads of replicated
    cooldown state across the 5 client worlds + eyeballing the action-bar tiles and the toast fade.
    `DA_GameConstants` cooldowns were **not** widened (no targeted casts run) — nothing to restore.

- **P5 done (2026-09-03, same session).** Written against the locked + implemented design — no
  dependency on the outstanding hands-on P4 items.
  - **`DECISIONS.md`** — new end-of-file entry **"Target-required abilities need a click-selected
    target"** (the full design: 3 target-required abilities & why only 3; Speed/Stabilize left auto;
    client-first gate + server re-validation; `Server_Activate*(AActor*)` signature changes; the
    `UCoopToastWidget` mechanism incl. the non-`UPROPERTY` fields + no-text-clear-on-fade note;
    `Status.Broken` reader-less / `Status.Vulnerable.Magic` dev-granted; deferred abilities; two
    explicit `CoopDamageAbilities` functions; `E`-key role-gating; one closed-editor `Build.bat`;
    smoke-pass results + what's owed). Plus: "The Q ability per role" Execution row + retrofit
    bullet + `ApplyTestVulnerableMagic` addendum.
  - **`docs/abilities.md`** — "Prototype status (2026-09-03)" italic lines on **Armor Break**,
    **Overload**, **Execution**. No spec / tag-glossary changes.

- **Where this leaves the feature:** P1 (C++) + P2 (build) + P3 (content) + P5 (docs) are **done and
  verified**. P4 is a smoke-pass green (plumbing proven end-to-end) with the behavioural half
  (P4.3–P4.8) parked for a human-driven 5-window PIE session — everything there needs a positioned
  world-click on a monster and/or cross-client-world state reads that the agentic tooling can't do.
