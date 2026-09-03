# Action Bar (WoW-style ability bar) — Implementation Plan & Progress Tracker

Resumable checklist for the bottom-screen ability action bar: coloured ability tiles, a keybind
badge, and a WoW-style radial cooldown sweep + seconds countdown. This doc **is** the plan — there
is no separate spec file; the design was agreed in chat on 2026-09-03.

> **STATUS (2026-09-03): FEATURE COMPLETE.** P1–P10 done and verified in 5-client PIE; the P9 C++
> deltas are now baked by a full external `Build.bat` rebuild from a closed editor (see the P9.8 /
> "rebuild done" log entries). Two real bugs were caught in P9 and fixed (widget self-hide freezes
> `NativeTick` → `RenderOpacity` toggle; placed sub-widget instances kept `gameConstants = None` →
> set per-instance) — see the P9 log entry and the `DECISIONS.md` "WoW-style action bar" entry.
> Nothing outstanding.

> Previous contents of this file (the "Q ability per role" feature and the RoleSelect screen
> feedback follow-on) are **complete**. Their design decisions and gotchas live permanently in
> `DECISIONS.md` ("The Q ability per role…", "`unreal-mcp` gotchas found building the Q abilities",
> "RoleSelect screen feedback is `NativeTick`-driven…"). This file was cleared and repurposed for
> the action bar at the user's explicit request.

**If a session is interrupted, a new session should:**
1. Read this file top to bottom, find the first unchecked `- [ ]`.
2. Read `docs/abilities.md` for the per-role ability list (source of truth for tile names).
3. Read `DECISIONS.md`'s entries listed above for the established widget/rebuild patterns.
4. Continue from the first unchecked box. Phases P1–P3 are all C++ and are built together in **one**
   rebuild (P4); do not rebuild between them.

---

## Design decisions — LOCKED (agreed 2026-09-03, do not re-litigate)

- **Slots shown = the role's full specced kit** from `docs/abilities.md` (2–3 tiles), not just the
  one working ability. Slot 0 is the implemented "Q ability" (Shield / Speed / Dash / Stabilize /
  Execution). Slots 1–2 are the specced-but-unbuilt kit — greyed tile, **no** keybind badge, **no**
  cooldown. They exist so the bar reads like a real kit and future ability work has an obvious seam.

  | Role | Slot 0 (live, **Q**) | Slot 1 | Slot 2 |
  |---|---|---|---|
  | TANK | Shield | Armor Break | — |
  | SUPPORT | Speed | Link | — |
  | RUNNER | Dash | Carry | Chain |
  | CONTROL | Stabilize | Mind Fracture | Channel |
  | DAMAGE | Execution | Overload | — |

- **Icons = coloured letter tiles.** Each slot is a solid-colour `Border` with the ability name
  centred on it. No textures, no art. Distinct colour per ability; greyed slots use a flat dark
  grey. This is CLAUDE.md §5's "everything readable, nothing pretty" applied literally, and partly
  fulfils §6.3's long-standing "ability cards with icon, name, explanation" (icons were always in
  the design; M5 skipped them).
- **Cooldown = WoW-style radial sweep + integer seconds.** A dark wedge covers the tile and unwinds
  **clockwise from 12 o'clock** as the cooldown expires; the whole-seconds-remaining number sits on
  top. Driven by one UI-domain material `M_CooldownSweep` with a single `Progress` (0–1) scalar,
  pushed every frame via a `UMaterialInstanceDynamic` (same DMI mechanism the mannequin colour tint
  already uses). This is the one element that brushes against §5's "no VFX / no post-processing" —
  it is *UI*, not scene VFX, so it is logged as a deliberate call in `DECISIONS.md` (P10), not a §5
  edit.
- **Visible during Prep + HoldTheGate**, collapsed in every other phase. `UCoopActionBarWidget`
  sets its own visibility from `NativeTick` off the replicated match phase — **no** UMG Designer
  "Bind Function" bindings anywhere in this feature (they can't be built via `unreal-mcp` —
  `DECISIONS.md`, RoleSelect follow-on). The prep-arena text ability cards (`WBP_AbilityCard`) are
  **untouched** and stay as the "what does it do" surface during Prep.
- **Slots are not mouse-click targets.** The whole bar is `HitTestInvisible` so the cursor always
  falls through to the camera drag (§5). Pressing **Q** stays the only way to cast; the slot just
  reflects the resulting cooldown. (Confirmed with the user.)
- **Keybind label is hardcoded `"Q"`** for slot 0 — every ability maps to Q by design
  (`DECISIONS.md`), so there is nothing to look up in the input mapping.
- **Cooldown state gets replicated.** The five `…CooldownEndServerTime` floats on `ACoopCharacter`
  are server-only today. They become `UPROPERTY(Replicated)` with `COND_OwnerOnly` — each client
  gets only its own pawn's cooldowns, still written server-side only.
- **A small per-role ability table is duplicated** into the slot widget (name + tile colour +
  implemented-flag). ~11 name strings also live in `CoopAbilityCardWidget`'s table. Deliberate:
  cheaper and lower-risk than refactoring a verified widget (CLAUDE.md §1 "hardcoded is correct",
  §4.8 "no unsolicited refactoring"). The card widget keeps the long descriptions; the bar doesn't
  need them.
- **No new `DA_GameConstants` fields.** Cooldown durations already exist (`ShieldCooldownSeconds`
  = 8, `StabilizeCooldownSeconds` = 10, `SpeedCooldownSeconds` = 8, `DashCooldownSeconds` = 4,
  `ExecutionCooldownSeconds` = 6). The slot widget reads them via a `UGameConstants*` reference set
  on `WBP_AbilitySlot`'s CDO (same content-wiring pattern as the PlayerController / scenes).

---

## File map

**C++ — new:**
- `Source/Unreal_first_Game/Core/CoopAbilitySlotWidget.h` / `.cpp` — one action-bar square. Per-
  instance `SlotIndex` 0–2. `NativeTick` drives tile colour / name / keybind / greyed state /
  cooldown sweep + number against `BindWidgetOptional` pointers. Owns the hardcoded per-role kit
  table and the two per-role "which cooldown getter / which duration constant" switches.
- `Source/Unreal_first_Game/Core/CoopActionBarWidget.h` / `.cpp` — the persistent container.
  `NativeTick` sets its own visibility (Prep/HoldTheGate → `HitTestInvisible`, else `Collapsed`).

**C++ — modified:**
- `Source/Unreal_first_Game/Core/CoopCharacter.h` — the 5 cooldown floats gain `UPROPERTY(Replicated)`;
  the "Not replicated" comment above the getters is corrected.
- `Source/Unreal_first_Game/Core/CoopCharacter.cpp` — `GetLifetimeReplicatedProps` gains 5
  `DOREPLIFETIME_CONDITION(…, COND_OwnerOnly)` lines.
- `Source/Unreal_first_Game/Core/CoopPlayerController.h` — `ActionBarWidgetClass` +
  `ActionBarWidget` fields (same shape as `RoleSelectWidgetClass`/`RoleSelectWidget`).
- `Source/Unreal_first_Game/Core/CoopPlayerController.cpp` — `BeginPlay` creates + adds the bar to
  viewport, after the `PrepArenaHUDWidget` block.

**Content — new (all via `unreal-mcp`):**
- `/Game/Materials/M_CooldownSweep` — Material, Material Domain = User Interface, Blend Mode =
  Translucent, Shading Model = Unlit. One scalar parameter `Progress` (default 0).
- `/Game/Blueprints/UI/WBP_AbilitySlot` — WidgetBlueprint, parent `UCoopAbilitySlotWidget`.
- `/Game/Blueprints/UI/WBP_ActionBar` — WidgetBlueprint, parent `UCoopActionBarWidget`.

**Content — modified (all via `unreal-mcp`):**
- `BP_PlayerController` CDO — `ActionBarWidgetClass` → `WBP_ActionBar_C`.
- `WBP_AbilitySlot` CDO — `GameConstants` → `/Game/Data/DA_GameConstants`.

---

## P1 — Replicate the 5 cooldown end-times  *(C++ DONE 2026-09-03, awaiting P4 rebuild)*

- [x] **Step 1.1** — `CoopCharacter.h`: change each of the 5 fields (currently around lines 184–188)
      from `float X = -1.0f;` to:
      ```cpp
      // Replicated to the owning client only (COND_OwnerOnly) so that client's own action bar
      // (UCoopAbilitySlotWidget) can draw a cooldown sweep. Still written server-side only, via the
      // Set...() setters called from the ability namespaces -- the owning client receives these
      // read-only and never writes them. Other players don't need each other's cooldowns.
      UPROPERTY(Replicated)
      float ShieldCooldownEndServerTime = -1.0f;
      UPROPERTY(Replicated)
      float StabilizeCooldownEndServerTime = -1.0f;
      UPROPERTY(Replicated)
      float SpeedCooldownEndServerTime = -1.0f;
      UPROPERTY(Replicated)
      float DashCooldownEndServerTime = -1.0f;
      UPROPERTY(Replicated)
      float ExecutionCooldownEndServerTime = -1.0f;
      ```
- [x] **Step 1.2** — `CoopCharacter.h`: fix the stale comment above `GetShieldCooldownEndServerTime()`
      (around line 82). Replace the sentence "Not replicated -- Build 1's ability cards have no
      cooldown-remaining display yet, so no client needs to see this." with:
      "Replicated to the owning client only (`COND_OwnerOnly`, see the field declarations below) so
      their action bar can draw the cooldown sweep. Still authored server-side only."
- [x] **Step 1.3** — `CoopCharacter.cpp`: in `GetLifetimeReplicatedProps` (around line 153), after
      the existing `DOREPLIFETIME(ACoopCharacter, ActiveStatusTags);` add:
      ```cpp
      // Owner-only: each player's client only needs its own cooldowns, for its own action bar.
      DOREPLIFETIME_CONDITION(ACoopCharacter, ShieldCooldownEndServerTime, COND_OwnerOnly);
      DOREPLIFETIME_CONDITION(ACoopCharacter, StabilizeCooldownEndServerTime, COND_OwnerOnly);
      DOREPLIFETIME_CONDITION(ACoopCharacter, SpeedCooldownEndServerTime, COND_OwnerOnly);
      DOREPLIFETIME_CONDITION(ACoopCharacter, DashCooldownEndServerTime, COND_OwnerOnly);
      DOREPLIFETIME_CONDITION(ACoopCharacter, ExecutionCooldownEndServerTime, COND_OwnerOnly);
      ```
      (`Net/UnrealNetwork.h` is already included; `COND_OwnerOnly` needs no extra include.)

---

## P2 — `UCoopAbilitySlotWidget` + `UCoopActionBarWidget`  *(C++ DONE 2026-09-03, awaiting P4 rebuild)*

- [x] **Step 2.1** — Create `Source/Unreal_first_Game/Core/CoopAbilitySlotWidget.h`:
      ```cpp
      #pragma once

      #include "CoreMinimal.h"
      #include "Blueprint/UserWidget.h"
      #include "Components/SlateWrapperTypes.h"
      #include "Core/CoopRoleTypes.h"
      #include "CoopAbilitySlotWidget.generated.h"

      class UBorder;
      class UImage;
      class UTextBlock;
      class UMaterialInstanceDynamic;
      class UGameConstants;

      // One square of the bottom-screen action bar (WoW-style). Reused 3x inside WBP_ActionBar with
      // a per-instance SlotIndex 0/1/2, exactly like UCoopAbilityCardWidget's CardIndex. All
      // feedback is driven from NativeTick against BindWidgetOptional pointers -- no Designer "Bind
      // Function" bindings, so the whole widget is buildable/verifiable through unreal-mcp (same
      // reasoning as the RoleSelect follow-on, DECISIONS.md).
      //
      // Slot 0 is the role's implemented "Q ability" (Shield/Speed/Dash/Stabilize/Execution): full
      // colour, a "Q" keybind badge, a live cooldown sweep. Slots 1-2 are the role's remaining
      // specced-but-unbuilt kit -- greyed, no keybind, no cooldown.
      UCLASS()
      class UNREAL_FIRST_GAME_API UCoopAbilitySlotWidget : public UUserWidget
      {
      	GENERATED_BODY()

      public:
      	// Which entry (0-2) of the local player's role kit this slot shows. Set per-instance in
      	// the WBP_ActionBar Designer.
      	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Bar")
      	int32 SlotIndex = 0;

      protected:
      	virtual void NativeConstruct() override;
      	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

      private:
      	EPlayerRole GetLocalPlayerRole() const;

      	// Cooldown end (absolute server time, CLAUDE.md §4.5) for THIS role's slot-0 ability, read
      	// off the local player's own pawn. -1 if unavailable. One explicit case per role -- no
      	// generic map (CLAUDE.md §4.6).
      	float GetSlotZeroCooldownEndServerTime(EPlayerRole Role) const;
      	float GetSlotZeroCooldownDurationSeconds(EPlayerRole Role) const;

      	// DA_GameConstants, set on WBP_AbilitySlot's CDO (same pattern as
      	// ACoopPlayerController::GameConstants and every scene's own reference).
      	UPROPERTY(EditDefaultsOnly, Category = "Game Constants")
      	TObjectPtr<UGameConstants> GameConstants;

      	// --- WBP_AbilitySlot children, matched by name ---
      	UPROPERTY(meta = (BindWidgetOptional))
      	TObjectPtr<UBorder> TileBorder;

      	UPROPERTY(meta = (BindWidgetOptional))
      	TObjectPtr<UTextBlock> NameText;

      	UPROPERTY(meta = (BindWidgetOptional))
      	TObjectPtr<UTextBlock> KeybindText;

      	// UImage whose brush material is M_CooldownSweep. NativeConstruct grabs one dynamic
      	// instance; NativeTick pushes the 0-1 "Progress" scalar every frame.
      	UPROPERTY(meta = (BindWidgetOptional))
      	TObjectPtr<UImage> CooldownImage;

      	UPROPERTY(meta = (BindWidgetOptional))
      	TObjectPtr<UTextBlock> CooldownSecondsText;

      	UPROPERTY(Transient)
      	TObjectPtr<UMaterialInstanceDynamic> CooldownMID;
      };
      ```
- [x] **Step 2.2** — Create `Source/Unreal_first_Game/Core/CoopAbilitySlotWidget.cpp` (added
      `#include "Styling/SlateColor.h"` beyond the plan draft, for the anon-namespace `FSlateColor`
      constants):
      ```cpp
      #include "Core/CoopAbilitySlotWidget.h"
      #include "Core/CoopPlayerState.h"
      #include "Core/CoopCharacter.h"
      #include "Core/GameConstants.h"
      #include "Components/Border.h"
      #include "Components/Image.h"
      #include "Components/TextBlock.h"
      #include "GameFramework/PlayerController.h"
      #include "GameFramework/GameStateBase.h"
      #include "Kismet/GameplayStatics.h"
      #include "Materials/MaterialInstanceDynamic.h"

      namespace
      {
      	struct FCoopAbilitySlotInfo
      	{
      		FText Name;
      		FLinearColor TileColor;
      		bool bImplemented = false;
      	};

      	// Per-role action-bar kit. Names duplicated from CoopAbilityCardWidget's table (which keeps
      	// the long descriptions -- the bar only needs name + tile colour + castable-flag). A
      	// deliberate ~11-string duplication, not a refactor of a verified widget (CLAUDE.md §4.8,
      	// §1). Only slot 0 of each role is bImplemented -- the one Q maps to.
      	const TArray<FCoopAbilitySlotInfo>& GetKitForRole(EPlayerRole Role)
      	{
      		static const TArray<FCoopAbilitySlotInfo> Tank = {
      			{ NSLOCTEXT("CoopAbilitySlot", "Shield", "Shield"),           FLinearColor(0.20f, 0.40f, 0.65f), true  },
      			{ NSLOCTEXT("CoopAbilitySlot", "ArmorBreak", "Armor Break"),  FLinearColor(0.55f, 0.30f, 0.12f), false },
      		};
      		static const TArray<FCoopAbilitySlotInfo> Support = {
      			{ NSLOCTEXT("CoopAbilitySlot", "Speed", "Speed"),             FLinearColor(0.15f, 0.52f, 0.42f), true  },
      			{ NSLOCTEXT("CoopAbilitySlot", "Link", "Link"),               FLinearColor(0.40f, 0.25f, 0.55f), false },
      		};
      		static const TArray<FCoopAbilitySlotInfo> Runner = {
      			{ NSLOCTEXT("CoopAbilitySlot", "Dash", "Dash"),               FLinearColor(0.42f, 0.52f, 0.15f), true  },
      			{ NSLOCTEXT("CoopAbilitySlot", "Carry", "Carry"),             FLinearColor(0.45f, 0.35f, 0.22f), false },
      			{ NSLOCTEXT("CoopAbilitySlot", "Chain", "Chain"),             FLinearColor(0.35f, 0.40f, 0.48f), false },
      		};
      		static const TArray<FCoopAbilitySlotInfo> Control = {
      			{ NSLOCTEXT("CoopAbilitySlot", "Stabilize", "Stabilize"),         FLinearColor(0.25f, 0.30f, 0.60f), true  },
      			{ NSLOCTEXT("CoopAbilitySlot", "MindFracture", "Mind Fracture"),  FLinearColor(0.55f, 0.20f, 0.45f), false },
      			{ NSLOCTEXT("CoopAbilitySlot", "Channel", "Channel"),             FLinearColor(0.15f, 0.45f, 0.55f), false },
      		};
      		static const TArray<FCoopAbilitySlotInfo> Damage = {
      			{ NSLOCTEXT("CoopAbilitySlot", "Execution", "Execution"),     FLinearColor(0.62f, 0.16f, 0.20f), true  },
      			{ NSLOCTEXT("CoopAbilitySlot", "Overload", "Overload"),       FLinearColor(0.45f, 0.20f, 0.55f), false },
      		};
      		static const TArray<FCoopAbilitySlotInfo> Empty;

      		switch (Role)
      		{
      			case EPlayerRole::Tank:    return Tank;
      			case EPlayerRole::Support: return Support;
      			case EPlayerRole::Runner:  return Runner;
      			case EPlayerRole::Control: return Control;
      			case EPlayerRole::Damage:  return Damage;
      			default:                   return Empty;
      		}
      	}

      	const FLinearColor GreyedTileColor(0.16f, 0.16f, 0.18f);
      	const FSlateColor  ImplementedTextColor(FLinearColor::White);
      	const FSlateColor  GreyedTextColor(FLinearColor(0.45f, 0.45f, 0.48f));
      }

      void UCoopAbilitySlotWidget::NativeConstruct()
      {
      	Super::NativeConstruct();

      	// One dynamic instance of M_CooldownSweep for this slot. Null if CooldownImage has no
      	// material brush yet -- tolerated, the tick just skips the sweep push.
      	if (CooldownImage)
      	{
      		CooldownMID = CooldownImage->GetDynamicMaterial();
      	}
      }

      EPlayerRole UCoopAbilitySlotWidget::GetLocalPlayerRole() const
      {
      	const APlayerController* PC = GetOwningPlayer();
      	const ACoopPlayerState* PS = PC ? PC->GetPlayerState<ACoopPlayerState>() : nullptr;
      	return PS ? PS->GetRole() : EPlayerRole::Unassigned;
      }

      float UCoopAbilitySlotWidget::GetSlotZeroCooldownEndServerTime(EPlayerRole Role) const
      {
      	const APlayerController* PC = GetOwningPlayer();
      	const ACoopCharacter* Char = PC ? Cast<ACoopCharacter>(PC->GetPawn()) : nullptr;
      	if (!Char)
      	{
      		return -1.0f;
      	}
      	switch (Role)
      	{
      		case EPlayerRole::Tank:    return Char->GetShieldCooldownEndServerTime();
      		case EPlayerRole::Support: return Char->GetSpeedCooldownEndServerTime();
      		case EPlayerRole::Runner:  return Char->GetDashCooldownEndServerTime();
      		case EPlayerRole::Control: return Char->GetStabilizeCooldownEndServerTime();
      		case EPlayerRole::Damage:  return Char->GetExecutionCooldownEndServerTime();
      		default:                   return -1.0f;
      	}
      }

      float UCoopAbilitySlotWidget::GetSlotZeroCooldownDurationSeconds(EPlayerRole Role) const
      {
      	if (!GameConstants)
      	{
      		return 0.0f;
      	}
      	switch (Role)
      	{
      		case EPlayerRole::Tank:    return GameConstants->ShieldCooldownSeconds;
      		case EPlayerRole::Support: return GameConstants->SpeedCooldownSeconds;
      		case EPlayerRole::Runner:  return GameConstants->DashCooldownSeconds;
      		case EPlayerRole::Control: return GameConstants->StabilizeCooldownSeconds;
      		case EPlayerRole::Damage:  return GameConstants->ExecutionCooldownSeconds;
      		default:                   return 0.0f;
      	}
      }

      void UCoopAbilitySlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
      {
      	Super::NativeTick(MyGeometry, InDeltaTime);

      	const EPlayerRole Role = GetLocalPlayerRole();
      	const TArray<FCoopAbilitySlotInfo>& Kit = GetKitForRole(Role);

      	// Nothing in this slot for this role (e.g. slot 2 on a 2-ability role) -> the square
      	// disappears, no empty tile.
      	if (!Kit.IsValidIndex(SlotIndex))
      	{
      		SetVisibility(ESlateVisibility::Collapsed);
      		return;
      	}
      	SetVisibility(ESlateVisibility::HitTestInvisible);

      	const FCoopAbilitySlotInfo& Info = Kit[SlotIndex];

      	if (NameText)
      	{
      		NameText->SetText(Info.Name);
      		NameText->SetColorAndOpacity(Info.bImplemented ? ImplementedTextColor : GreyedTextColor);
      	}
      	if (TileBorder)
      	{
      		TileBorder->SetBrushColor(Info.bImplemented ? Info.TileColor : GreyedTileColor);
      	}
      	if (KeybindText)
      	{
      		const bool bShowKey = (SlotIndex == 0 && Info.bImplemented);
      		KeybindText->SetText(FText::FromString(TEXT("Q")));
      		KeybindText->SetVisibility(bShowKey ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
      	}

      	// --- Cooldown sweep: implemented slot-0 only ---
      	float Progress = 0.0f;
      	int32 SecondsLeft = 0;
      	if (SlotIndex == 0 && Info.bImplemented)
      	{
      		const AGameStateBase* GS = UGameplayStatics::GetGameState(this);
      		const float Now = GS ? GS->GetServerWorldTimeSeconds() : 0.0f;
      		const float End = GetSlotZeroCooldownEndServerTime(Role);
      		const float Duration = GetSlotZeroCooldownDurationSeconds(Role);
      		const float Remaining = End - Now;
      		if (Remaining > 0.0f && Duration > 0.0f)
      		{
      			Progress = FMath::Clamp(Remaining / Duration, 0.0f, 1.0f);
      			SecondsLeft = FMath::CeilToInt(Remaining);
      		}
      	}

      	if (CooldownMID)
      	{
      		CooldownMID->SetScalarParameterValue(TEXT("Progress"), Progress);
      	}
      	if (CooldownSecondsText)
      	{
      		if (SecondsLeft > 0)
      		{
      			CooldownSecondsText->SetText(FText::AsNumber(SecondsLeft));
      			CooldownSecondsText->SetVisibility(ESlateVisibility::HitTestInvisible);
      		}
      		else
      		{
      			CooldownSecondsText->SetVisibility(ESlateVisibility::Collapsed);
      		}
      	}
      }
      ```
- [x] **Step 2.3** — Create `Source/Unreal_first_Game/Core/CoopActionBarWidget.h`:
      ```cpp
      #pragma once

      #include "CoreMinimal.h"
      #include "Blueprint/UserWidget.h"
      #include "Components/SlateWrapperTypes.h"
      #include "CoopActionBarWidget.generated.h"

      // C++ base for WBP_ActionBar -- the persistent bottom-screen ability bar. Created once in
      // ACoopPlayerController::BeginPlay alongside the other HUD widgets and left in the viewport;
      // NativeTick drives its own visibility off the replicated match phase (visible during Prep
      // and HoldTheGate, collapsed otherwise) -- same NativeTick-not-Designer-bindings approach as
      // the RoleSelect follow-on. The slots themselves are UCoopAbilitySlotWidget instances placed
      // in the WBP; this class only owns show/hide.
      UCLASS()
      class UNREAL_FIRST_GAME_API UCoopActionBarWidget : public UUserWidget
      {
      	GENERATED_BODY()

      protected:
      	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
      };
      ```
- [x] **Step 2.4** — Create `Source/Unreal_first_Game/Core/CoopActionBarWidget.cpp`:
      ```cpp
      #include "Core/CoopActionBarWidget.h"
      #include "Core/CoopGameState.h"
      #include "Core/CoopMatchPhase.h"
      #include "Kismet/GameplayStatics.h"

      void UCoopActionBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
      {
      	Super::NativeTick(MyGeometry, InDeltaTime);

      	const ACoopGameState* GameState = Cast<ACoopGameState>(UGameplayStatics::GetGameState(this));
      	const EMatchPhase Phase = GameState ? GameState->GetCurrentPhase() : EMatchPhase::WaitingForRoster;
      	const bool bShow = (Phase == EMatchPhase::Prep || Phase == EMatchPhase::HoldTheGate);

      	// HitTestInvisible (whole subtree) -- nothing in the bar is interactive, the mouse must
      	// always fall through to the camera drag (CLAUDE.md §5).
      	SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
      }
      ```

---

## P3 — Wire the bar into `CoopPlayerController`  *(C++ DONE 2026-09-03, awaiting P4 rebuild)*

- [x] **Step 3.1** — `CoopPlayerController.h`: after the `PrepArenaHUDWidgetClass`/`PrepArenaHUDWidget`
      pair (around line 165–169), add:
      ```cpp
      // The bottom-screen WoW-style ability bar (WBP_ActionBar). Same create-once-in-BeginPlay,
      // leave-in-viewport pattern as the widgets above; UCoopActionBarWidget hides itself outside
      // the Prep / HoldTheGate phases.
      UPROPERTY(EditDefaultsOnly, Category = "UI")
      TSubclassOf<UUserWidget> ActionBarWidgetClass;

      UPROPERTY()
      TObjectPtr<UUserWidget> ActionBarWidget;
      ```
- [x] **Step 3.2** — `CoopPlayerController.cpp`: in `BeginPlay`, right after the
      `if (PrepArenaHUDWidgetClass) { … }` block (around line 98), add:
      ```cpp
      if (ActionBarWidgetClass)
      {
      	ActionBarWidget = CreateWidget<UUserWidget>(this, ActionBarWidgetClass);
      	if (ActionBarWidget)
      	{
      		ActionBarWidget->AddToViewport();
      	}
      }
      ```

---

## P4 — One full rebuild from a closed editor  *(BUILD DONE 2026-09-03)*

New `UCLASS`es and new `UPROPERTY`s on the already-loaded `ACoopCharacter`/`ACoopPlayerController` →
**Live Coding is unsafe** (DECISIONS.md's confirmed-crash case). Full external build required.

- [x] **Step 4.1** — Editor was already fully closed (no `UnrealEditor` process — checked via
      `tasklist`).
- [x] **Step 4.2** — Confirmed no `UnrealEditor` process, ran
      `Build.bat Unreal_first_GameEditor Win64 Development -project=... -waitmutex` (2026-09-03).
      Reviewed all 4 files on disk against the plan first (2 new widgets + the `CoopCharacter`
      replication delta + `CoopPlayerController` `ActionBarWidgetClass`/BeginPlay delta) — all
      reference only symbols that exist (`EMatchPhase::Prep`/`HoldTheGate`; the 5 `…CooldownSeconds`
      GameConstants; `UImage::GetDynamicMaterial`/`UBorder::SetBrushColor`). No issues.
- [x] **Step 4.3** — **Result: Succeeded, exit 0**, ~66s, 21 actions. `CoopActionBarWidget.cpp` +
      `CoopAbilitySlotWidget.cpp` compiled as new files; `CoopCharacter.cpp`/`CoopPlayerController.cpp`
      recompiled for the deltas. UHT wrote 8 generated files, no errors.
      `UnrealEditor-Unreal_first_Game.dll` relinked → **773,120 bytes, Sep 3 01:01** (was 742,912).
      `Unreal_first_GameEditor.target` also rebuilt. **Zero warnings or errors in the build log.**
- [x] **Step 4.4** — Editor reopened; `unreal-mcp` reconnected. `list_toolsets` + `AgentSkillToolset.ListSkills`
      + multiple `ObjectTools` calls all succeed (2026-09-03).
- [x] **Step 4.5** — New symbols confirmed loaded: `ObjectTools.search_subclasses` on
      `/Script/UMG.UserWidget` (filter "Coop") returns both `CoopAbilitySlotWidget` and
      `CoopActionBarWidget`. `BP_PlayerController` CDO (`Default__BP_PlayerController_C`) shows
      `actionBarWidgetClass = None` (not-yet-wired, as planned) and the other three widget-class refs
      (`matchTimerWidgetClass`/`roleSelectWidgetClass`/`prepArenaHUDWidgetClass`) + `gameConstants`
      all still correctly set — no silent reset from the rebuild.

---

## P5 — `M_CooldownSweep` material  *(all `unreal-mcp`, `MaterialTools`)*

Radial clockwise wipe. `angle01` = 0 at 12 o'clock, increasing clockwise, in `[0,1)`. The tile is
**dark where `angle01 >= (1 - Progress)`** — at `Progress=1` the whole tile is dark; as the cooldown
finishes (`Progress → 0`) the dark wedge shrinks toward 12 o'clock, sweeping clockwise. Verified
against WoW behaviour: at 25% CD remaining, the top-left ~quarter (9→12 o'clock) stays dark.

- [x] **Step 5.1** — `/Game/Materials/M_CooldownSweep` created (folder already existed — `M_CoopButton`
      lives there). `materialDomain = MD_UI`, `blendMode = BLEND_Translucent`, `shadingModel = MSM_Unlit`
      confirmed via read-back.
- [x] **Step 5.2** — `MaterialExpressionScalarParameter` "Progress", `defaultValue 0.0`, `group "Cooldown"`,
      slider min/max 0–1. `list_parameters` confirms one Scalar param `Progress`.
- [x] **Step 5.3** — Angle chain built exactly as planned: `TextureCoordinate_0` → `Subtract_0`
      (B = `Constant2Vector_0` `(0.5, 0.5)`) → two `ComponentMask` (R→`_0`, G→`_1`). `Arctangent2_0`:
      Y = `ComponentMask_0` (cx), X = `Multiply_0` (cy × ConstB −1). `Divide_0` (ConstB `6.283185307`) →
      `Frac_0` = angle01. All input wiring re-read and verified.
- [x] **Step 5.4** — Mask: `OneMinus_0`(Progress) → threshold. `If_0`: A = `Frac_0`, B = `OneMinus_0`,
      `A > B` and `A == B` → `Constant_0` (1.0), `A < B` → `Constant_1` (0.0). `Multiply_1` (ConstB `0.55`).
- [x] **Step 5.5** — `Constant3Vector_0` `(0,0,0)` → `MP_EmissiveColor`; `Multiply_1` → `MP_Opacity`.
      Both output connections verified via `get_property_input`.
- [x] **Step 5.6** — `layout_expressions` + `recompile` — no error raised (tool raises on shader
      failure), log clean of `LogMaterial` errors. `save_assets` + `is_dirty == false` + on-disk
      `.uasset` mtime confirmed (14,743 B, Sep 3 01:13).
- [x] **Step 5.7** — Sanity-checked via a scratch `MI_CooldownSweep_ScratchTest` MIC + `CaptureAssetImage`
      at Progress 1.0 / 0.75 / 0.25 / 0.0: 1.0 → whole tile ~55%-dark; 0.0 → fully transparent;
      0.25 → only the top-left quarter (9→12 o'clock) dark; 0.75 → dark everywhere except the top-right
      quarter (12→3 o'clock). Clockwise-from-12 unwind confirmed, matches WoW. Scratch MIC **deleted**
      after verification.

---

## P6 — `WBP_AbilitySlot`  *(all `unreal-mcp`, `UMGToolSet` + `ObjectTools`)*

- [x] **Step 6.1** — `WBP_AbilitySlot` created at `/Game/Blueprints/UI`, parent
      `/Script/Unreal_first_Game.CoopAbilitySlotWidget` (confirmed via `GetWidgets` `Info.ParentClass`).
- [x] **Step 6.2** — Tree built via `UMGToolSet.AddWidget` (`list_properties` first on every widget/slot):
      `RootSizeBox` (76×76, both overrides on) → `RootOverlay` → in order: `TileBorder` (Fill/Fill in
      overlay, `brushColor (0.16,0.16,0.18,1)`, padding 0) with child `NameText` (`Text "Shield"`,
      `Justification Center`, `AutoWrapText true`, `Font.Size 12`, white); `CooldownImage` (Fill/Fill,
      `Brush.ImageSize (76,76)`, `Brush.ResourceObject → M_CooldownSweep`, `Visibility HitTestInvisible`);
      `CooldownSecondsText` (Center/Center, `Font.Size 24` Bold white, `Text ""`, `Collapsed`);
      `KeybindText` (Right/Top, padding `(0,2,3,0)`, `Font.Size 12` white, `Text "Q"`, `Collapsed`).
      All 5 `ToggleWidgetAsVariable true`. `GetWidgetDescription` re-read — tree matches.
- [x] **Step 6.3** — `CompileWidgetBlueprint` → `true`; `LogBlueprint` shows only the "Compiling
      Blueprint" lines, no BindWidget/type/graph errors.
- [x] **Step 6.4** — `save_assets` + `is_dirty == false` + on-disk `WBP_AbilitySlot.uasset` (35,310 B,
      Sep 3 01:22).

---

## P7 — `WBP_ActionBar`  *(all `unreal-mcp`)*

- [x] **Step 7.1** — `WBP_ActionBar` created, parent `/Script/Unreal_first_Game.CoopActionBarWidget`.
- [x] **Step 7.2** — Tree: `RootCanvas` (CanvasPanel) → `SlotBox` (HorizontalBox) with
      `CanvasPanelSlot` anchors min/max `(0.5, 1.0)`, alignment `(0.5, 1.0)`, `bAutoSize true`,
      offsets `(0, -28)`. Three `WBP_AbilitySlot_C` children `Slot0`/`Slot1`/`Slot2`, each
      `HorizontalBoxSlot.Padding` `(4,0,4,0)`.
- [x] **Step 7.3** — `slotIndex` set per instance: `Slot0 = 0`, `Slot1 = 1`, `Slot2 = 2`
      (`ObjectTools.set_properties` on each `WidgetTree.SlotN` node). Re-read after compile —
      1 and 2 survived (0 is the C++ default so it doesn't show as an override).
- [x] **Step 7.4** — `CompileWidgetBlueprint` → `true`, `LogBlueprint` clean, `GetWidgetDescription`
      re-read. `save_assets` + `is_dirty == false` + on-disk `WBP_ActionBar.uasset` (32,854 B, Sep 3 01:25).

---

## P8 — CDO wiring  *(all `unreal-mcp`)*

- [x] **Step 8.1** — `Default__WBP_AbilitySlot_C` `gameConstants` → `/Game/Data/DA_GameConstants.DA_GameConstants`.
      Compiled + re-read: `{"gameConstants": ".../DA_GameConstants", "slotIndex": 0}` — survived.
- [x] **Step 8.2** — `Default__BP_PlayerController_C` `actionBarWidgetClass` →
      `/Game/Blueprints/UI/WBP_ActionBar.WBP_ActionBar_C`. Post-`compile_blueprint` re-read confirms
      `matchTimerWidgetClass` / `roleSelectWidgetClass` / `prepArenaHUDWidgetClass` / `gameConstants`
      all unchanged — no silent reset.
- [x] **Step 8.3** — `compile_blueprint` on `BP_PlayerController`; `save_assets` for all four;
      `is_dirty == false` for all four; `BP_PlayerController.uasset` / `WBP_AbilitySlot.uasset` /
      `WBP_ActionBar.uasset` all re-written Sep 3 01:27, `M_CooldownSweep.uasset` unchanged since 01:13
      (not dirty).

---

## P9 — 5-client PIE verification

> **A real bug in the planned C++ was caught here and fixed — see the "P9 bug" log entry.**
> `UCoopActionBarWidget` / `UCoopAbilitySlotWidget` self-`SetVisibility(Collapsed)` in `NativeTick`
> froze the widget: Slate stops ticking a widget the frame its own visibility leaves the "visible"
> family (**both `Collapsed` and `Hidden` stop `NativeTick`** — confirmed with instrumentation), so
> the bar, created during RoleSelect, never un-hid for Prep. Fix: the container stays
> `HitTestInvisible` forever and toggles its own `RenderOpacity` (0 hidden / 1 shown) instead; the
> slot does the same for the transient Unassigned-role case, keeping `Collapsed` only for the
> permanent "slot index past this role's kit" case (no empty 3rd tile). Both are function-body-only
> changes, Live-Coding-safe.

- [x] **Step 9.1** — 5-client PIE (`PlayMode_InEditorFloating`, `PIE_ListenServer`, one process).
      RoleSelect auto-resolved each run to 5 distinct roles (e.g. Control/Runner/Tank/Support/Damage
      on the run used for the screenshots).
- [x] **Step 9.2** — **Bar hidden during RoleSelect:** instrumented `NativeTick` logged
      `Phase=1 (RoleSelect) bShow=0 Opacity=0.00` every tick for the whole 30 s window; the first two
      runs' RoleSelect-phase screenshots showed no tiles. (A `RenderOpacity=0` widget is fully
      transparent — stronger than a screenshot check.)
- [x] **Step 9.3** — **Bar appears in Prep, correct per role** (screenshots cropped/upscaled):
      - Control → **3 tiles**: `Stabilize` (periwinkle `(0.25,0.30,0.60)`, white text, **"Q"** badge
        top-right) + `Mind Fracture` (flat grey `(0.16,0.16,0.18)`, greyed text, no badge) + `Channel`.
      - Runner → **3 tiles**: `Dash` (olive `(0.42,0.52,0.15)`, "Q" badge) + `Carry` + `Chain` greyed.
      - Support → **exactly 2 tiles, no empty 3rd**: `Speed` (teal `(0.15,0.52,0.42)`, "Q" badge) +
        `Link` greyed. The `Collapsed`-slot path for a 2-ability role is exercised every tick and
        gives a clean 2-tile bar.
- [x] **Step 9.4** — **Cooldown set on cast:** `TriggerInputAction(IA_Speed)` on the Support host set
      `speedCooldownEndServerTime = 163.84` on its pawn (widened `SpeedCooldownSeconds` to 90; cast
      at server-time ≈ 73.8 → `73.8 + 90 = 163.8`, exact). `TriggerInputAction(IA_Dash)` on the
      Runner's own remote client set `dashCooldownEndServerTime = 287.32` (`197.3 + 90`, exact).
      Reflection reads needed the 5 `…CooldownEndServerTime` fields to gain `VisibleInstanceOnly`
      (they were `UPROPERTY(Replicated)`-only → invisible to `ObjectTools`) — a small deviation from
      the plan, logged; CLAUDE.md §4.3 wants these printable anyway.
- [x] **Step 9.5** — **Owner-only replication confirmed exactly.** Runner (pid 277) cast Dash from
      its own client. `dashCooldownEndServerTime` for pawn-277 read across all 5 PIE worlds:
      `UEDPIE_0` (server) `287.32`, `UEDPIE_1` (Runner's own client) `287.32`, `UEDPIE_2/3/4` (other
      clients) `-1`. Value reaches the server + the one owning client and no other client —
      textbook `DOREPLIFETIME_CONDITION(…, COND_OwnerOnly)`.
- [x] **Step 9.6** — **Radial visual:** first attempt showed no sweep/number on any cast tile.
      Diagnostic (`CDLog`) found `Dur=0.0` — **the 3 `WBP_AbilitySlot` instances inside `WBP_ActionBar`
      had `gameConstants = None`.** Root cause: P7 created the slot instances *before* P8.1 wired the
      `WBP_AbilitySlot` CDO, and a placed widget instance snapshots the CDO's value at placement time
      — the later CDO edit doesn't propagate (same shape as the M6/P7.3 per-instance `slotIndex` need).
      Fixed by `set_properties` `gameConstants` on `Slot0`/`Slot1`/`Slot2` directly (+ recompile +
      save). **Re-verified after PIE restart** — `TriggerInputAction(IA_Stabilize)` on the Control host
      set `stabilizeCooldownEndServerTime ≈ Now + 90`; the widget then showed a white **"55"** number
      + a dark radial wedge covering all but the top-right ~40% of the tile; a screenshot ~29 s later
      showed **"26"** and the wedge shrunk to a small top-left corner — sweep unwinds clockwise from
      12 o'clock exactly like WoW. `STABDIAG` logging confirmed `Server_ActivateStabilize` →
      `ResolveStabilize` fires with role/pawn/GameConstants all valid.
- [x] **Step 9.7** — **Bar persists into HoldTheGate:** instrumented `NativeTick` logged
      `Phase=3 (HoldTheGate) bShow=1 MyVis=3 (HitTestInvisible) Opacity=1.00` continuously; the bar
      stays `HitTestInvisible` the whole time (mouse falls through to the camera drag by construction).
- [x] **Step 9.8** — Cleanup done: `StopPIE`; `DA_GameConstants` restored
      (`stabilize`/`speed`/`dash` `CooldownSeconds` back to 10 / 8 / 4);
      `bThrottleCPUWhenNotForeground` + `bAllowSlateThrottling` restored to `true` (project default);
      all TEMP diagnostic `UE_LOG`s removed from `CoopActionBarWidget.cpp` / `CoopAbilitySlotWidget.cpp`
      / `CoopPlayerController.cpp` and a final clean Live Coding compile run (patch_6). Saved:
      `DA_GameConstants` (restored), `WBP_ActionBar` (the Slot0/1/2 `gameConstants` fix). All
      `is_dirty == false`.
- [x] **Bake rebuild done (2026-09-03).** Full external `Build.bat Unreal_first_GameEditor Win64
      Development` from a confirmed-closed editor. **Succeeded, exit 0**, ~38s, 20 actions,
      `CoopActionBarWidget.cpp` / `CoopAbilitySlotWidget.cpp` / `CoopCharacter.cpp` recompiled, DLL
      relinked → **774,144 B, Sep 3 09:31**. No warnings/errors. No UHT re-run needed — the editor's
      Live Coding pass had already regenerated `CoopCharacter.gen.cpp` (01:58) for the
      `VisibleInstanceOnly, Category="Cooldowns"` change; confirmed all 5 `…CooldownEndServerTime`
      fields + their `_MetaData`/`Cooldowns` category are present in that generated file, so the
      reflection is genuinely baked, not just the function bodies. The `RenderOpacity` widget fix is
      function-body-only and linked in the same pass.

---

## P10 — `DECISIONS.md` entry

- [x] **Step 10.1** — `DECISIONS.md` entry "WoW-style action bar (bottom-screen ability bar)" added,
      covering: the locked design; the `CoopAbilityCardWidget.cpp` "icon art out of scope" override +
      partial §6.3 "cards with icon" fulfilment; `M_CooldownSweep` as a deliberate logged call vs §5's
      "no VFX / no post-processing" (UI, not scene VFX); the 5 `…CooldownEndServerTime` fields now
      `COND_OwnerOnly`-replicated + `VisibleInstanceOnly`; the `NativeTick`-not-Designer-binding rule;
      "slots not click targets, Q only".
- [x] **Step 10.2** — Both P9 deviations recorded in the same entry: (a) the "a widget can't hide
      itself via `SetVisibility` in its own `NativeTick` — `Collapsed` *and* `Hidden` both freeze the
      tick" gotcha and the `RenderOpacity` fix; (b) the "placed sub-widget instance snapshots its
      class CDO at placement time" gotcha (`gameConstants = None` on the slots) and the set-per-instance
      fix. (The full `Build.bat` bake rebuild those deltas needed is now done — 2026-09-03, see P9.8.)

---

## Self-review against the design (done 2026-09-03 at plan-write time)

- **Slots = full kit:** P2 table has 2–3 entries per role, only slot 0 `bImplemented`. ✓
- **Coloured letter tiles:** P2 `FLinearColor` per ability + `Border` `BrushColor` in P6. ✓
- **Radial sweep + number:** P5 material + P2 `Progress` push + `CooldownSecondsText`. ✓
- **Prep + combat visibility, cards untouched:** P2.4 `NativeTick` phase check; no `WBP_AbilityCard`
  edits anywhere in the plan. ✓
- **Not click targets:** `HitTestInvisible` in P2.4 and on every P6 sub-widget. ✓
- **Keybind hardcoded "Q":** P2.2 `NativeTick`. ✓
- **Cooldown replication:** P1 (`COND_OwnerOnly`), verified in P9.4/P9.5. ✓
- **Duplicated table, card widget untouched:** P2.2 comment + no card-widget file in the map. ✓
- **No new GameConstants:** P8.1 wires the existing asset; no `GameConstants.h` edit in the map. ✓
- Placeholder scan: no "TBD"/"handle edge cases"/"similar to" — every C++ step has full code. ✓
- Type consistency: `GetKitForRole` / `FCoopAbilitySlotInfo` / `GetSlotZeroCooldown*` /
  `CooldownMID` / the 5 `Get…CooldownEndServerTime()` getters all named identically across P1/P2. ✓

---

## Log
(Newest at the bottom. One line per completed step.)

- **Plan written (2026-09-03).** Design agreed in chat (4 decisions: full kit / coloured letter
  tiles / radial sweep + number / Prep+combat, cards stay; slots not clickable). This file cleared
  and replaced with the plan above. Next: P1–P3 (all C++), then P4 (one rebuild).
- **P1–P3 C++ done (2026-09-03).** All in one uncommitted batch:
  - `CoopCharacter.h`/`.cpp` — 5 `…CooldownEndServerTime` floats now `UPROPERTY(Replicated)` +
    `DOREPLIFETIME_CONDITION(… COND_OwnerOnly)`; stale "Not replicated" comment corrected.
  - `CoopAbilitySlotWidget.h`/`.cpp` (new) — per-role kit table (2–3 entries, only slot 0
    `bImplemented`), `NativeConstruct` grabs the cooldown DMI, `NativeTick` drives tile colour /
    name / `"Q"` badge / greyed state / `Progress` scalar + `CooldownSecondsText`. One explicit
    `switch(Role)` each for the cooldown getter and the duration constant (§4.6).
  - `CoopActionBarWidget.h`/`.cpp` (new) — `NativeTick` sets own visibility `HitTestInvisible`
    during Prep/HoldTheGate, `Collapsed` otherwise.
  - `CoopPlayerController.h`/`.cpp` — `ActionBarWidgetClass`/`ActionBarWidget` + `BeginPlay`
    create-and-add-to-viewport.
  Verified against the tree: `EMatchPhase` has `Prep`/`HoldTheGate`; Build.cs already has
  UMG/SlateCore/Engine; all includes resolved. **Next: P4 — user closes the editor, then the full
  `Build.bat` rebuild.**
- **P4 rebuild done (2026-09-03).** Resumed session, confirmed no `UnrealEditor` process, re-read all
  4 files on disk (not just the plan text) — `CoopAbilitySlotWidget.h`/`.cpp`,
  `CoopActionBarWidget.h`/`.cpp` match the plan; `CoopCharacter` has the 5 `UPROPERTY(Replicated)` +
  `DOREPLIFETIME_CONDITION(… COND_OwnerOnly)` and the corrected comment; `CoopPlayerController` has
  `ActionBarWidgetClass`/`ActionBarWidget` + the BeginPlay create-and-add block. Ran `Build.bat`:
  **Succeeded, exit 0**, ~66s, DLL relinked to 773,120 B (Sep 3 01:01), no warnings/errors. **Next:
  P4.4/P4.5 need the user — reopen the editor so `unreal-mcp` reconnects and the new
  `CoopAbilitySlotWidget`/`CoopActionBarWidget` symbols are visible; then P5 (`M_CooldownSweep`
  material) onward is all `unreal-mcp`.**
- **P4.4 / P4.5 / P5 done (2026-09-03).** Editor reopened, `unreal-mcp` reconnected.
  `search_subclasses` on `/Script/UMG.UserWidget` confirms both `CoopAbilitySlotWidget` and
  `CoopActionBarWidget` are loaded; `BP_PlayerController` CDO shows `actionBarWidgetClass = None`
  (not-yet-wired, expected) with the other 3 widget-class refs + `gameConstants` unchanged (no
  rebuild silent-reset). Built `M_CooldownSweep` in full via `MaterialTools`: MD_UI / Translucent /
  Unlit, `Progress` scalar param (group "Cooldown"), the `atan2`-based clockwise-from-12 angle chain
  and the `angle01 >= (1-Progress)` step mask feeding `MP_Opacity` at 0.55, black `MP_EmissiveColor`.
  Recompiled clean. Visually verified with a scratch MIC + `CaptureAssetImage` at Progress 1.0/0.75/
  0.25/0.0 — the dark wedge unwinds clockwise from 12 o'clock exactly as WoW does (P=0.25 → only
  top-left quarter dark; P=0.75 → only top-right quarter clear). Scratch MIC deleted. **Next: P6 —
  `WBP_AbilitySlot` widget blueprint (`UMGToolSet` + `ObjectTools`).**
- **P6 / P7 / P8 done (2026-09-03).** `WBP_AbilitySlot` (SizeBox 76×76 → Overlay → `TileBorder`+`NameText`,
  `CooldownImage` (M_CooldownSweep brush), `CooldownSecondsText`, `KeybindText`; all 5 as variables)
  and `WBP_ActionBar` (CanvasPanel → bottom-centre autosize `SlotBox` HorizontalBox → 3
  `WBP_AbilitySlot_C` `Slot0/1/2` with `slotIndex` 0/1/2) built via `UMGToolSet`; both compile clean.
  CDO wiring: `WBP_AbilitySlot` CDO `gameConstants` → `DA_GameConstants`; `BP_PlayerController` CDO
  `actionBarWidgetClass` → `WBP_ActionBar_C` (other widget-class refs re-verified, no silent reset).
  All saved, `is_dirty` false, `.uasset` mtimes checked. **Next: P9 (5-client PIE).**
- **P9 done — two real bugs caught and fixed, one plan-assumption corrected (2026-09-03).**
  - **Bug 1 (widget freezes hidden).** `UCoopActionBarWidget` / `UCoopAbilitySlotWidget` self-set
    `SetVisibility(Collapsed)` in `NativeTick`. The bar is created (in `ACoopPlayerController::BeginPlay`)
    during RoleSelect, so its first tick hides it — and Slate stops calling `Tick` on a widget whose
    own visibility leaves the "visible" family. **`Hidden` stops it too, not just `Collapsed`**
    (confirmed with a throttled `UE_LOG`: the tick fired exactly once then never again). Fix: the
    container stays `HitTestInvisible` forever and toggles its own `RenderOpacity` (0/1) — a
    RenderOpacity-0 widget is invisible but keeps ticking. The slot does the same for the transient
    `Unassigned` role, keeping `Collapsed` only for the *permanent* "slot past this role's kit" case
    (so a 2-ability role still shows exactly 2 tiles). Both are function-body-only → 3 clean Live
    Coding compiles.
  - **Bug 2 (`gameConstants = None` on the slot instances).** The cooldown sweep drew nothing on any
    cast tile. A `CDLog` diagnostic showed `Dur=0.0` — the 3 `WBP_AbilitySlot` instances inside
    `WBP_ActionBar` had `gameConstants = None`: P7 placed them **before** P8.1 wired the
    `WBP_AbilitySlot` CDO, and a placed widget instance snapshots the CDO at placement time (same
    class of gotcha as P7.3's per-instance `slotIndex`). Fixed by `set_properties` `gameConstants`
    on `Slot0/1/2` directly. **The plan's P8.1 should have set this per-instance, or P7 should
    follow P8** — logged for P10.
  - **Plan assumption corrected.** P9.4/9.5 assume the replicated `…CooldownEndServerTime` is
    reflection-readable; it wasn't (`UPROPERTY(Replicated)` with no edit/visible specifier →
    invisible to `ObjectTools`). Added `VisibleInstanceOnly, Category="Cooldowns"` to all 5 (a
    header change; Live Coding compiled it clean with the benign packaging warning). This also
    serves CLAUDE.md §4.3 ("state must always be printable").
  - **Verified:** 9.1 5-client PIE, 5 distinct auto-resolved roles. 9.2 bar `Opacity=0` through the
    whole RoleSelect phase. 9.3 Control → 3 tiles (Stabilize periwinkle + Q badge, Mind Fracture /
    Channel greyed); Support/Runner → 2 / 3 tiles, colours + badge + greying + count all correct,
    no empty tile on 2-ability roles. 9.4 `IA_Speed`/`IA_Dash`/`IA_Stabilize` casts each set
    `…CooldownEndServerTime = casttime + duration` exactly. 9.5 `COND_OwnerOnly` exact — Runner's
    Dash cooldown read `287.32` on the server world + the Runner's own client, `-1` on the other 3
    clients. 9.6 sweep + number animate (`55` → `26`, dark wedge shrinks clockwise from 12). 9.7 bar
    persists into HoldTheGate, still `HitTestInvisible`.
  - **9.8 cleanup:** diagnostics removed, `DA_GameConstants` cooldowns restored (10/8/4), editor
    throttle settings restored, final clean Live Coding compile. **Next: P10.**
- **Bake rebuild done (2026-09-03).** Resumed session, confirmed no `UnrealEditor` process, re-read
  the P9 deltas on disk (`CoopActionBarWidget.cpp` / `CoopAbilitySlotWidget.cpp` `RenderOpacity`
  fix; `CoopCharacter.h` 5× `UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Cooldowns")`).
  Ran full external `Build.bat`: **Succeeded, exit 0**, ~38s, DLL relinked to 774,144 B (Sep 3
  09:31), no warnings/errors. Verified the reflection change is baked (not just function bodies) by
  checking `CoopCharacter.gen.cpp` — all 5 cooldown fields + `_MetaData`/`Cooldowns` category
  present. **The action bar feature is now fully complete with nothing outstanding.**
