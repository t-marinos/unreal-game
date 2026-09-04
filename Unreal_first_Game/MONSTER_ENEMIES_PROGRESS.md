# Monsters: cones → moving Mannequin enemies — Plan & Progress Tracker

Resumable checklist for turning Hold the Gate's trash monsters from stationary red cones into
proper moving enemies: a Mannequin body, straight-line pursuit of their fixated target, a
melee-range-gated attack with a telegraphed windup, knockback on hit (strong enough to shove a
plate-holder off their plate), and the Tank counterplay that makes body-blocking and Shield matter.
Agreed in chat 2026-09-04. This doc **is** the plan — there is no separate spec file (same
convention as `ABILITY_UX_PROGRESS.md` / `ABILITIES_PROGRESS.md` / `cursor_progress.md`).

> **STATUS (2026-09-04): ALL PHASES COMPLETE (A + B + C). No open plan work.** Remaining: the user
> commits (Phase A/B source, the two content assets, the doc updates), and a real 5-player playtest
> covers the 4 deferred B9 checks (B9.3 plate-dislodge, B9.4 Fortress negation + resist, B9.5
> Shield-shove, B9.6 Shield regression) that solo dev-mode PIE structurally can't reach.
>
> Phase B recap: `BP_MonsterStrikeTelegraph` created over `ACoopMonsterStrikeTelegraph`, `M_TargetRing`
> wired on `ringMaterial`, `strikeTelegraphClass` set on `BP_MonsterCharacter`'s CDO, all compiled +
> saved (`is_dirty == false`, on disk 10:55). The 3 new `DA_GameConstants` fields read their C++
> defaults (`monsterAttackWindupSeconds` 1.0 / `monsterKnockbackImpulse` 650 / `shieldShoveImpulse`
> 900) with no per-asset override — same as A7. B9 solo dev-mode PIE **verified**: clean RoleSelect→
> Prep→HoldTheGate loop, no new script errors; `BP_MonsterStrikeTelegraph_C` actors spawn during
> windups (15+ observed, transient ~1s); strike lands (`MonsterAttackDamage` 5/hit, dummy 80→0);
> knockback launches the target (`MonsterKnockbackImpulse` — a dummy dropped into the swarm was flung
> ~1000 units); target Downed at 0 HP → ~10 monsters retargeted off it (also covers the A8.5 gap).
> **B9.3 / B9.4 / B9.5 / B9.6 still deferred to a real 5-player playtest** (plate-dislodge needs a
> dummy on a plate; Fortress negation / Shield-shove / Shield-still-negates need a live Tank+Control) —
> same dev-mode limitation A8 hit. Editor left with `BP_GameMode` + `Lvl_ThirdPerson` dirty in-memory
> only (bDevMode on/off toggle + a temp-actor visual check); both unchanged on disk / in git — discard
> on close, do not save.
>
> Phase A recap: monsters spawn as moving red Mannequins possessed by `ACoopMonsterAIController`,
> straight-line-chase their non-Tank fixate target, melee-range gate holds. A8.3-pos/A8.4/A8.5/A8.6
> blocked by a dev-mode limitation (idle dummies float at Z≈302) — verify in a real 5-player playtest.
>
> **Phase design (historical, for context):** Phase A = movement + look (reparent
> `ACoopMonsterCharacter` `AActor`→`ACharacter`, Mannequin body, new `ACoopMonsterAIController`,
> straight-line chase, melee-range gate on the existing attack — no windup/knockback). Phase B =
> telegraph + knockback + the `Status.Fortress` `ApplyDamage` fix + Shield-shove. Phase C = docs.
> **One closed-editor `Build.bat` per phase** (base-class change + new UCLASSes → not Live-Coding-safe,
> `DECISIONS.md` "Live Coding must not be used to add a new UCLASS"). Both builds ran `exit 0`; the
> plan-gap `ECC_Visibility` re-block fix (added before the A5 build so monsters stay click-targetable
> after losing the cone's `BlockAllDynamic` mesh) is in `ACoopMonsterCharacter::BeginPlay`.

**If a future session picks this up** (only the deferred B9 playtest checks + the commit remain):
1. The 4 deferred checks (B9.3–B9.6) need a **real 5-player playtest** — solo dev-mode PIE can't put
   a dummy on a plate or field a live Tank + Control. Don't burn time re-attempting them solo.
2. Read `DECISIONS.md`'s **"Monster combat inside Hold the Gate"** entry incl. its
   "Follow-up (2026-09-04)" subsection before touching any monster code — straight-line steering is
   in bounds, navmesh / behaviour trees / pathfinding are not.
3. Uncommitted at hand-off: Phase A/B source (`CoopMonsterAIController.*`, `CoopMonsterStrikeTelegraph.*`,
   modified `CoopMonsterCharacter.*` / `CoopHealthComponent.cpp` / `CoopTankAbilities.cpp` /
   `GameConstants.h`), `M BP_MonsterCharacter.uasset`, `?? BP_MonsterStrikeTelegraph.uasset`, and
   the doc updates (`DECISIONS.md`, `docs/scenes/HOLD_THE_GATE.md`, `docs/abilities.md`, this file).

---

## Design decisions — LOCKED (agreed 2026-09-04, do not re-litigate)

### The enemy actor

- **`ACoopMonsterCharacter` reparents `AActor` → `ACharacter`, in place.** Same class, same name,
  same file — so every existing `Cast<ACoopMonsterCharacter>` call site (`CoopMonsterSpawner`,
  `CoopTankAbilities::ResolveArmorBreak`, `CoopDamageAbilities::ResolveExecution`/`ResolveOverload`,
  `CoopHoldTheGateScene::ResetScene`, `CoopHealthComponent::ApplyDamage`'s `Cast<ACoopCharacter>`
  which correctly will *not* match a monster) keeps working untouched.
- **Drop the custom cone `UStaticMeshComponent` root.** Use `ACharacter`'s inherited
  `CapsuleComponent` (root) + `GetMesh()` (skeletal) + `CharacterMovementComponent`. The `Mesh`
  member is deleted (it collided in spirit with `ACharacter::GetMesh()`); the constructor's
  `ConeMeshFinder` / `MonsterMaterialFinder` / `SetActorScale3D` all go.
- **Body = the stock Mannequin**, wired on `BP_MonsterCharacter` (content, A6), exactly mirroring
  `BP_PlayerCharacter`'s mesh component:
  - `SkeletalMesh` = `/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple`
  - `AnimClass` = `/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed` (the same locomotion
    AnimBP players + dummies use — free idle/walk/run)
  - mesh relative location `Z = -89`, rotation `yaw = 270` (standard capsule offset)
- **Red tint** = the same idea already in `BeginPlay`, retargeted from the cone material to the
  Mannequin's materials: iterate `GetMesh()->GetNumMaterials()`, `CreateAndSetMaterialInstanceDynamic(i)`,
  `SetVectorParameterValue(TEXT("Paint Tint"), FLinearColor(0.55f, 0.03f, 0.03f))` — the Mannequin
  materials' tint param is **`"Paint Tint"`** (not `"Color"`), per `ACoopCharacter::ApplyPlayerColorTint`.
- `HealthComponent`, `TargetingComponent`, replicated `ActiveStatusTags` + the whole status-tag
  API, `HandleHealthDepleted` → `Destroy()`, `InitializeMonster` — **unchanged** except
  `InitializeMonster` also sets `GetCharacterMovement()->MaxWalkSpeed` from `GameConstants` (A2).
- **`Destroyed()` override** (new): destroy the possessing AI controller before `Super::Destroyed()`
  — covers both death (`HandleHealthDepleted`) and the scene-reset path
  (`CoopHoldTheGateScene::ResetScene` calls `Monster->Destroy()` directly), so no controller leaks
  per dead monster.

### Movement

- **New `ACoopMonsterAIController : public AAIController`** (`Source/Unreal_first_Game/Core/`,
  alongside the monster it drives — not `Dev/`, this is real gameplay AI now, not dev tooling).
  A near-copy of `ADummyAIController`: `PrimaryActorTick.bCanEverTick = true`; `Tick()` gets its
  pawn as `ACoopMonsterCharacter`, reads `TargetingComponent->GetCurrentTarget()`, and if the
  target is beyond `StopDistance` does `AddMovementInput((TargetLoc - PawnLoc).GetSafeNormal2D())`.
  **No `MoveToActor`, no navmesh, no behaviour tree** — straight-line steering only, identical to
  the dummy pattern. `DECISIONS.md`'s "Monster combat" entry forbids *pathfinding*; this is not that.
- **`StopDistance`** tracks the melee range: `ACoopMonsterCharacter` exposes
  `float GetMeleeRangeUnits() const` (reads `GameConstants->MonsterMeleeRangeUnits`, fallback
  constant if unset); the controller stops ~a hair inside that so `PerformAttackTick`'s own range
  check passes. If the pawn/target/GameConstants is missing the controller no-ops that tick.
- Monster `CharacterMovement` config (constructor): `MaxWalkSpeed` = a default (~350, overridden
  from `GameConstants->MonsterMoveSpeed` in `InitializeMonster`), `bOrientRotationToMovement = true`,
  `bUseControllerRotationYaw = false` — it faces where it walks. Default movement replication (the
  §4.2 exception).
- **`AIControllerClass = ACoopMonsterAIController::StaticClass()`** + `AutoPossessAI =
  EAutoPossessAI::PlacedInWorldOrSpawned` in the monster constructor → the engine spawns + possesses
  the controller during `SpawnActor`, before `InitializeMonster` runs. All server-side.
- **Body-block is free** — capsule vs capsule. A monster walking its straight line into the Tank's
  capsule is physically stopped and never reaches melee range of its fixated plate-holder. No code.
  Monsters also block each other (a clumping red wall — acceptable; the fallback if it reads badly
  is one collision-channel tweak so they block players but overlap each other, **not built now**).

### Attack + telegraph  *(Phase B)*

- `PerformAttackTick` (the existing repeating `MonsterAttackIntervalSeconds` timer) gains a
  **range gate**: only proceeds when `DistToTarget <= GameConstants->MonsterMeleeRangeUnits`
  (Phase A adds this gate; the attack still fires instantly on a passing tick in Phase A).
- **Phase B** turns a passing tick into a **windup**: set `bWindingUp`, spawn a flat red ground
  ring at the target's feet (a small new `ACoopMonsterStrikeTelegraph` actor — engine `Plane` +
  `M_TargetRing`, `"Color"` param red, scaled — copied from `ACoopTargetRing`'s shape; or reuse
  `ACoopTargetRing` directly if it's clean to), set a one-shot `MonsterAttackWindupSeconds` timer.
- Windup fires → **`PerformStrike`**: re-check target still in melee range + not Downed. In range →
  `ApplyDamage(MonsterAttackDamage)` + knockback (below). Out of range → whiff (telegraph beaten).
  Either way clear `bWindingUp` + destroy the telegraph.
- The telegraph is internal monster state (`bool bWindingUp` + timer), **not a `FGameplayTag`** —
  "about to hit" is transient AI state, not a status condition anything reads. No `docs/abilities.md`
  tag change.

### Knockback + the Fortress gap  *(Phase B)*

- **On a landed strike:** in addition to `ApplyDamage`, `LaunchCharacter` the player target away
  from the monster — `(TargetLoc - MonsterLoc).GetSafeNormal2D() * GameConstants->MonsterKnockbackImpulse`,
  `bXYOverride = true`. Server-side; rides the §4.2 movement-replication exception.
- **Tuned strong enough to shove a plate-holder off their plate** (user decision 2026-09-04): the
  push moves the character out of `ACoopPressurePlate`'s thin overlap band → its own
  `OnOccupancyChanged` fires → the gate closes → `PlateRestoreWindowSeconds` starts. **No plate
  code changes** — the plate's existing overlap logic does all of it. This makes the monster threat
  attack the *objective*, not just HP, and is what makes Fortress's knockback-resist matter.
- **The Fortress gap (must fix this pass):** `Status.Fortress` is written by `Stabilize` and shown
  on the status badge / unit frame, but **nothing reads it for defense** —
  `CoopHealthComponent::ApplyDamage` only early-returns on `Status.Shielded`. Until real damage
  existed (now) that was invisible. Fix:
  - `ApplyDamage` also early-returns (negates) when the owner `HasStatusTag(Status_Fortress)` —
    one `||` added to the existing `Status_Shielded` check.
  - When knocking back a player who `HasStatusTag(Status_Fortress)`, scale the impulse by
    `(1.0f - GameConstants->FortressKnockbackResistPercent)` — **first consumer** of that constant
    (it was added in an earlier milestone with "no consumer yet — no ability applies knockback
    until Hold the Gate's monsters exist").
  - `Status.Shielded` keeps negating damage and does **not** resist knockback — that stays a real
    Shield → Fortress upgrade distinction.

### Tank counterplay  *(Phase B)*

- **Shield-shove.** `CoopTankAbilities::ApplyShield` already sweeps a forward cone over
  `ACoopCharacter` actors. Add a parallel `TActorIterator<ACoopMonsterCharacter>` loop with the
  same cos-angle + radius test: every monster in the cone takes a one-time
  `LaunchCharacter` directly away from the Tank, `GameConstants->ShieldShoveImpulse` (new). This is
  `docs/scenes/HOLD_THE_GATE.md`'s "knock enemies away" — Shield becomes a repositioning tool, not
  just a damage filter.
- **Fortress does not add its own shove.** Its upgrade value stays: multi-teammate damage coverage
  (the radius loop in `ResolveStabilize`, now actually load-bearing once `ApplyDamage` reads the
  tag) + the player-side knockback resist above. Keeps this change contained to one loop in one
  function.

### Rebuild discipline

- **Phase A** = a base-class change on `ACoopMonsterCharacter` (`AActor` → `ACharacter`) + a whole
  new UCLASS (`ACoopMonsterAIController`) + new members → **Live Coding unsafe** (`DECISIONS.md`
  "Live Coding must not be used to add a new UCLASS…"). One full external `Build.bat` from a closed
  editor (A5).
- **Phase B** = new `GameConstants` fields (a `UDataAsset`, not a live UCLASS instance concern in
  the same way, but new `UPROPERTY`s all the same) + a new small `ACoopMonsterStrikeTelegraph`
  UCLASS + method bodies. **Also closed-editor `Build.bat`** (B7) — same caution.
- → **Two external builds total, one per phase.** Phase C (docs) is text-only, no build.

---

## File map

**C++ — new:**
- `Source/Unreal_first_Game/Core/CoopMonsterAIController.h` / `.cpp` — server-only straight-line
  chase controller (Phase A).
- `Source/Unreal_first_Game/Scenes/CoopMonsterStrikeTelegraph.h` / `.cpp` — flat red ground ring
  for the attack windup, `ACoopTargetRing`'s shape without the per-player cursor logic (Phase B).
  *(Decide at B2 whether a new actor is really cheaper than reusing `ACoopTargetRing`.)*

**C++ — modified:**
- `Source/Unreal_first_Game/Core/CoopMonsterCharacter.h` — `: public ACharacter`;
  `#include "GameFramework/Character.h"`; drop the `Mesh` member + `UStaticMeshComponent` fwd decl;
  add `GetMeleeRangeUnits()` (public), `Destroyed()` override, and (Phase B) `bWindingUp` +
  `PerformStrike()` + `WindupTimerHandle` + a `TWeakObjectPtr` to the live telegraph.
- `Source/Unreal_first_Game/Core/CoopMonsterCharacter.cpp` — rewrite the constructor (capsule/mesh/
  movement config, `AIControllerClass`, `AutoPossessAI`); retarget the tint to `GetMesh()` +
  `"Paint Tint"`; `InitializeMonster` sets `MaxWalkSpeed`; `PerformAttackTick` gets the range gate;
  `Destroyed()` kills the controller. Phase B: windup/strike split, knockback in `PerformStrike`.
- `Source/Unreal_first_Game/Core/GameConstants.h` — Phase A: `MonsterMoveSpeed`,
  `MonsterMeleeRangeUnits`. Phase B: `MonsterAttackWindupSeconds`, `MonsterKnockbackImpulse`,
  `ShieldShoveImpulse`. (`FortressKnockbackResistPercent`, `MonsterAttackDamage`,
  `MonsterAttackIntervalSeconds`, `MonsterHealth` already exist.)
- `Source/Unreal_first_Game/Core/CoopHealthComponent.cpp` — Phase B: `ApplyDamage` also negates
  under `Status_Fortress`.
- `Source/Unreal_first_Game/Abilities/CoopTankAbilities.cpp` — Phase B: monster-cone shove loop in
  `ApplyShield`; `+#include "Core/CoopMonsterCharacter.h"` is already there.
- `Source/Unreal_first_Game/Abilities/CoopControlAbilities.cpp` *(maybe)* — only if the
  Fortress-knockback-resist read needs a helper; likely untouched (the resist is read in the
  monster's `PerformStrike`).

**Content — modified (all via `unreal-mcp`):**
- `/Game/Blueprints/Scenes/BP_MonsterCharacter` — after the A5 rebuild: confirm/repair the reparent
  (base is now a `Character`), set the `Mesh` component's `SkeletalMesh` / `AnimClass` / relative
  transform, capsule size (~34 r / 88 hh, Mannequin default), `GameConstants` already wired.
  `AIControllerClass` is set in C++ so nothing to do there unless it needs overriding.
- `/Game/Blueprints/Scenes/BP_MonsterStrikeTelegraph` *(Phase B)* — new BP over the telegraph class,
  `M_TargetRing` on its plane, `"Color"` red.
- `/Game/Data/DA_GameConstants` — set the new fields (A7 for the two Phase-A ones, B1 for Phase B).
- `BP_MonsterSpawner` — no change expected (spawns `MonsterClass` at its own transform; capsule
  spawn-collision is already `AdjustIfPossibleButAlwaysSpawn`). Verify only.

**Docs — modified (Phase C):**
- `DECISIONS.md` — extend "Monster combat inside Hold the Gate".
- `docs/scenes/HOLD_THE_GATE.md` — drop "stationary ranged harassers"; document movement,
  body-block, Shield-shove, Fortress-resist, plate-knockback.
- `docs/abilities.md` — Shield now also shoves monsters in its cone; Fortress now actually negates
  damage in its radius + resists knockback (Stabilize + Shield entries, tag glossary notes).

---

## Phase A — movement + look  *(C++ A1–A3 in one batch; A5 is the single rebuild)*

> **✅ A1–A4 + the A7 `.h` fields DONE + A5 BUILD DONE (2026-09-04, exit 0, DLL 837,632 B @ 00:36,
> no warnings — plus a plan-gap Visibility-block fix, see the STATUS block). Skip to A5.4 / A6.**
> What was actually written, as the reference:
>
> - **`Core/CoopMonsterAIController.h/.cpp` (new UCLASS)** — `: public AAIController`,
>   `bCanEverTick = true`, `Tick()` casts `GetPawn()` → `ACoopMonsterCharacter`, reads
>   `GetTargetingComponent()->GetCurrentTarget()`, and while `ToTarget.SizeSquared2D() >
>   Square(Monster->GetMeleeRangeUnits())` does `AddMovementInput(ToTarget.GetSafeNormal2D())`.
>   No `MoveToActor` / navmesh / BT. (Dropped the sketch's `FallbackStopDistanceUnits` — the
>   monster's own `GetMeleeRangeUnits()` fallback covers the no-GameConstants case.) Includes:
>   `CoopMonsterCharacter.h`, `CoopFixateRetargetComponent.h`, `GameFramework/Pawn.h`.
> - **`Core/CoopMonsterCharacter.h`** — `#include "GameFramework/Character.h"`, `: public ACharacter`;
>   removed the `UStaticMeshComponent` fwd decl + the `Mesh` `UPROPERTY`; added public
>   `float GetMeleeRangeUnits() const` and protected `virtual void Destroyed() override`. Class
>   comment rewritten.
> - **`Core/CoopMonsterCharacter.cpp`** — constructor: `InitCapsuleSize(34, 88)`,
>   `MaxWalkSpeed = 350` + `bOrientRotationToMovement = true` + `bUseControllerRotationYaw = false`,
>   `AIControllerClass = ACoopMonsterAIController::StaticClass()` + `AutoPossessAI =
>   PlacedInWorldOrSpawned`. `BeginPlay`: **`GetCapsuleComponent()->SetCollisionResponseToChannel(
>   ECC_Visibility, ECR_Block)`** (the plan-gap fix — see the STATUS block), then the tint loop moved
>   to `GetMesh()` over all material slots with param **`"Paint Tint"`** (Mannequin param, not
>   `"Color"`), colour `(0.55, 0.03, 0.03)`.
>   `InitializeMonster` sets `GetCharacterMovement()->MaxWalkSpeed = GameConstants->MonsterMoveSpeed`.
>   `GetMeleeRangeUnits()` = `GameConstants ? MonsterMeleeRangeUnits : 160`. `Destroyed()` =
>   `if (AController* C = GetController()) C->Destroy(); Super::Destroyed();`. `PerformAttackTick`
>   gained the melee-range gate (`FVector::DistSquared(...) > Square(GetMeleeRangeUnits())` → return)
>   before the damage. Includes: `+CoopMonsterAIController.h`, `+Components/CapsuleComponent.h`,
>   `+Components/SkeletalMeshComponent.h`, `+GameFramework/CharacterMovementComponent.h`,
>   `+GameFramework/Controller.h`; dropped `Components/StaticMeshComponent.h`,
>   `UObject/ConstructorHelpers.h`.
> - **`Core/GameConstants.h`** — `MonsterMoveSpeed = 350.0f`, `MonsterMeleeRangeUnits = 160.0f`
>   under `Category = "Monster"` (part of the A5 build — it's a `.h` change).
> - **A4 review:** every `ACoopMonsterCharacter` call site checked (`CoopMonsterSpawner`,
>   `CoopTankAbilities`, `CoopDamageAbilities`, `CoopHoldTheGateScene::ResetScene`,
>   `CoopUnitFrameWidget`, `CoopPlayerController` dev cmds + click-target trace,
>   `CoopHealthComponent::ApplyDamage`) — all use `Cast<>` / `GetActorLocation` /
>   `GetHealthComponent` / the tag API; none broken. `CoopHealthComponent`'s `Cast<APawn>(GetOwner())`
>   now matches a monster (was null when monster was an `AActor`) but `GetPlayerState()` → null so
>   the invuln branch is still skipped — no behaviour change. `AIModule` already in `Build.cs`.

### A5 — One full external `Build.bat` from a closed editor  *(BUILD DONE 2026-09-04 — A5.4 needs the user)*

- [x] **A5.1** — Confirmed no `UnrealEditor` process (`tasklist`), 2026-09-04.
- [x] **A5.2** — Re-read every A1–A3 delta on disk vs. the plan. `CoopMonsterAIController.h`/`.cpp`
      (`: public AAIController`, `Tick` straight-line `AddMovementInput`, no navmesh/BT; includes
      `AIController.h`/`CoopMonsterCharacter.h`/`CoopFixateRetargetComponent.h`/`Pawn.h`).
      `CoopMonsterCharacter.h` (`: public ACharacter` + `#include "GameFramework/Character.h"`, `Mesh`
      member + fwd decl dropped, `GetMeleeRangeUnits()` + `Destroyed()` added). `CoopMonsterCharacter.cpp`
      (constructor: `InitCapsuleSize`, movement config, `AIControllerClass` + `AutoPossessAI`; `BeginPlay`
      tint → `GetMesh()` + `"Paint Tint"`; `InitializeMonster` sets `MaxWalkSpeed`; `PerformAttackTick`
      melee-range gate; `Destroyed()` kills the controller; includes swapped
      static-mesh→capsule/skeletal/charactermovement/controller). `GameConstants.h` (`MonsterMoveSpeed`
      350, `MonsterMeleeRangeUnits` 160). **Found + fixed a plan gap** (see the STATUS block): added
      `GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block)` to
      `BeginPlay` so monsters stay click-targetable after losing the `BlockAllDynamic` cone mesh. All
      includes present, all symbols resolve.
- [x] **A5.3** — Ran `Build.bat Unreal_first_GameEditor Win64 Development -project=... -waitmutex`.
      **Result: Succeeded, exit 0**, ~71s, 7 actions (`CoopMonsterAIController.cpp` /
      `CoopMonsterCharacter.cpp` / `GameConstants.cpp` + module + link). UHT wrote **6 generated
      files** — `CoopMonsterAIController.generated.h` (3,492 B) fresh @ 00:35; `CoopMonsterCharacter.gen.cpp`
      now references `ACharacter` (the reparent is baked into the reflection data, not just the
      header). `UnrealEditor-Unreal_first_Game.dll` relinked → **837,632 B, Sep 4 00:36** (was
      822,784). `.target` rebuilt. **Zero warnings or errors** — the `AActor`→`ACharacter` base
      change + the new UCLASS broke no call site.
- [x] **A5.4** — Editor was already open on resume (2026-09-04). `unreal-mcp` connected,
      `IsPIERunning` → false. `search_subclasses(AIModule.AIController, "CoopMonster")` →
      `/Script/Unreal_first_Game.CoopMonsterAIController`; `search_subclasses(Engine.Character,
      "CoopMonster")` → `/Script/Unreal_first_Game.CoopMonsterCharacter` (reparent took, DLL loaded).
      `Default__CoopMonsterCharacter` CDO has a valid `mesh` (`CharacterMesh0`) + `capsuleComponent`
      (`CollisionCylinder`) — the C++ side is clean.

### A6 — Content: `BP_MonsterCharacter` (all `unreal-mcp`)

- [x] **A6.1 — RECREATED FRESH (reparent was corrupt).** The reparented `BP_MonsterCharacter`
      compiled but its CDO's `mesh` read `None` (vs. `CharacterMesh0` on both the C++ CDO and
      `BP_PlayerCharacter`). Root cause in the log: `USimpleConstructionScript::FixupRootNodeParentReferences()
      - Couldn't find native parent component 'Mesh' for 'DefaultSceneRoot'` — the old AActor-era BP
      had an SCS root node parented to the old cone's native `UStaticMeshComponent Mesh`, which the
      reparent deleted, leaving a stale, un-fixable record (a round-trip reparent to native
      `ACharacter` and back did not clear it). Path taken: nulled `BP_MonsterSpawner.monsterClass`
      → compiled + saved the spawner → `AssetTools.delete BP_MonsterCharacter` → `BlueprintTools.create`
      fresh over `/Script/Unreal_first_Game.CoopMonsterCharacter` at the same path. Fresh CDO has a
      valid `mesh` (`CharacterMesh0`). BP has no custom graph/vars (only the 3 disconnected template
      stub events), so nothing was lost.
- [x] **A6.2** — `CharacterMesh0`: `skeletalMeshAsset` = `SKM_Quinn_Simple`, `animClass` =
      `ABP_Unarmed_C`, `relativeLocation` `(0,0,-89)`, `relativeRotation` `(pitch 0, yaw 270, roll 0)`
      — read off `BP_PlayerCharacter`'s `CharacterMesh0` and mirrored exactly.
- [x] **A6.3** — Capsule inherited from C++ `InitCapsuleSize`: `capsuleRadius 34`, `capsuleHalfHeight
      88` (confirmed on the BP's inherited `CollisionCylinder`). `gameConstants` set to
      `/Game/Data/DA_GameConstants` (fresh BP started with it `None`). `aIControllerClass` =
      `CoopMonsterAIController` + `autoPossessAI` = `PlacedInWorldOrSpawned` inherited from C++, no
      wiring needed.
- [x] **A6.4** — Both BPs `compile_blueprint` clean (no errors/warnings in the log after the
      recreate). Saved; `is_dirty == false` for `BP_MonsterCharacter`, `BP_MonsterSpawner`,
      `DA_GameConstants`. On-disk mtimes updated. `BP_MonsterSpawner.monsterClass` re-pointed to
      `/Game/Blueprints/Scenes/BP_MonsterCharacter.BP_MonsterCharacter_C`.

### A7 — New `DA_GameConstants` fields

- [x] `GameConstants.h` — `MonsterMoveSpeed = 350.0f` + `MonsterMeleeRangeUnits = 160.0f` under
      `Category = "Monster"`, after `MonsterAttackIntervalSeconds`. **Done — part of the A5 build.**
- [x] `DA_GameConstants` — `monsterMoveSpeed` reads `350`, `monsterMeleeRangeUnits` reads `160`
      (C++ defaults, no per-asset override). Left as-is; asset not dirty. Revisit only if A8 tuning
      says otherwise.

### A8 — PIE verification

> **PARTIAL (2026-09-04). Core Phase-A behaviour verified in solo dev-mode PIE (2 runs, `StartPIE`
> auto-ran a 5-client session). A8.3-positive / A8.4 / A8.5 / A8.6 are BLOCKED by a dev-mode
> limitation, not a Phase-A bug — see A8.3 below. Two content fixes were made during A8, no C++
> touched (A9 = N/A).**
>
> **Content fixes made during A8 (both `unreal-mcp`, no rebuild):**
> 1. **`BP_MonsterCharacter.HealthComponent.gameConstants`** was unset on the fresh recreate (A6
>    only wired the actor-level one) → `LogTemp: Warning: UCoopHealthComponent::BeginPlay:
>    GameConstants not set on BP_MonsterCharacter…, falling back to 100.0` on every spawn.
>    Wired it to `DA_GameConstants` (mirrors `BP_PlayerCharacter.HealthComponent`), compiled, saved.
>    Warning gone on the next run. (`InitializeMonster`'s `SetMaxHealth(MonsterHealth)` had been
>    masking the functional impact — health still ended at 50 — but the plan wants a clean log.)
> 2. **`Lvl_ThirdPerson`'s 4 placed `BP_MonsterSpawner` actors had `monsterClass` == `None`** in
>    PIE (no monsters spawned at all on the first run). Cause: deleting the old `BP_MonsterCharacter`
>    in A6 nulled the reference *in memory* for the loaded level actors (the CDO re-point doesn't
>    reach placed instances, and `monsterClass` is `EditDefaultsOnly` so it can't be set per-instance).
>    On-disk the external-actor `.uasset`s still stored the path `/Game/…/BP_MonsterCharacter.BP_MonsterCharacter_C`
>    — which the fresh recreate reoccupies — so **`SceneTools.load_level` (reload, no save) re-resolved
>    all 4 to the new class**. No level save needed; `git status` shows `Lvl_ThirdPerson` unmodified.
>    (If a future session deletes+recreates a BP that placed actors reference, reload the level after.)

- [x] **A8.1** — Solo dev-mode PIE, `StartPIE` brought up a 5-client session (`UEDPIE_0..4`).
      RoleSelect → Prep → HoldTheGate ran clean across 2 runs. **No `Error` / `Accessed None` /
      `LogScript` / `LogBlueprint` for `CoopMonsterCharacter` / `CoopMonsterAIController`.** The one
      `UCoopHealthComponent::BeginPlay: GameConstants not set` warning was fixed (see fix #1 above)
      and confirmed gone on the second run.
- [x] **A8.2 — movement** — monsters spawn as `ACoopMonsterCharacter` (now `ACharacter`), each
      auto-possessed by a spawned `ACoopMonsterAIController`, and walk via straight-line steering:
      positions moved 100s of units between reads (spawn `~(600,900)` → `~(234,567)` → converging on
      the target cluster), `LogCharacterMovement` velocity / "is stuck" lines confirm the movement
      component is actively consuming `AddMovementInput`, `bOrientRotationToMovement` yaw tracks
      heading. Mannequin `SKM_Quinn_Simple` + `ABP_Unarmed_C` wired (verified via properties).
      **Fixate correctly excludes the Tank** — every spawn wave fixated on the 4 non-Tank characters
      (`BP_PlayerCharacter_C_0/1/3/4` when `_2` was Tank), never the Tank. Red "Paint Tint" **not
      visually screenshotted** — `CaptureViewport` renders the editor scene, not PIE-spawned actors;
      the tint code is the proven `ApplyPlayerColorTint` pattern (`"Paint Tint"` over all mesh slots).
- [x] **A8.3 — melee gate, negative half** — no target ever lost HP from a monster that had not
      reached it. Every plate-holder `HealthComponent.currentHealth` read stayed `100/100` across
      ~2 min of dozens of monsters pursuing across the room. The old cross-room ranged harassment is
      gone.
- [ ] **A8.3-positive / A8.4 / A8.5 / A8.6 — BLOCKED in solo dev mode (not a Phase-A bug).** The
      dev-mode dummy plate-holders (`ADummyAIController`, `EDummyBehavior::Idle`) never leave their
      PlayerStarts, which sit on a raised spawn platform at **Z ≈ 302**; ground monsters are at
      **Z ≈ 90**, so `PerformAttackTick`'s `FVector::DistSquared` (3D) gate can never pass and no hit
      ever lands on a dummy. Nothing in HoldTheGate sets dummies to `StandOn` a plate. So: no landed
      hit to observe (A8.3-pos), no HP-drop to stop with a body-block (A8.4), no natural down to
      trigger a retarget (A8.5), and Execution-kill needs a click-selected target this session
      couldn't drive (A8.6). **Verify these in a real 5-player playtest** (players actually stand on
      the plates, same floor as the monsters), or with manual pawn-driving. Follow-up worth
      considering (out of this plan's scope, §4.8): give dev-mode dummies a `StandOn(nearest plate)`
      behaviour for HoldTheGate so solo testing exercises the scene.
- [x] **A8.7 — cleanup** — `StopPIE`; `bDevMode` → `false` (verified); editor throttle
      `bThrottleCPUWhenNotForeground` / `bAllowSlateThrottling` → `true`/`true` (verified);
      `DA_GameConstants` spawn intervals restored to `6.0`/`2.5` and saved, `is_dirty == false`;
      `BP_GameMode` `is_dirty == false`; `BP_MonsterSpawner` `is_dirty == false`; no asset editors
      open. `git status`: only `BP_MonsterCharacter.uasset` changed in `Content/` (the A6 recreate),
      no level / spawner / GameMode / GameConstants diff.

### A9 — Bake rebuild

- [x] **N/A** — A8's two fixes were content-only (`unreal-mcp`), no C++ changed. No rebuild.

---

## Phase B — telegraph + knockback + Fortress + Shield-shove  *(B2–B6 one C++ batch; B7 the rebuild — BUILT 2026-09-04)*

> **STATUS (2026-09-04): B1–B7 DONE — B7 `Build.bat` Succeeded, exit 0, DLL 858,624 B @ 10:40, no
> warnings/errors (new `ACoopMonsterStrikeTelegraph` UCLASS + windup/strike/knockback + Fortress
> negation + Shield-shove all compiled + linked). Next: user reopens the editor, then B8 (content:
> `BP_MonsterStrikeTelegraph`, wire `StrikeTelegraphClass` on `BP_MonsterCharacter`, the 3 new
> `DA_GameConstants` fields), then B9 PIE.**
>
> **What was written, as the reference:**
> - **`Core/GameConstants.h`** — `+MonsterAttackWindupSeconds = 1.0f`, `+MonsterKnockbackImpulse =
>   650.0f` (Category `"Monster"`, after `MonsterMeleeRangeUnits`); `+ShieldShoveImpulse = 900.0f`
>   (Category `"Abilities"`, after `ShieldCoverageRadiusUnits`). Updated the stale
>   `FortressKnockbackResistPercent` "no consumer yet" comment.
> - **`Scenes/CoopMonsterStrikeTelegraph.h/.cpp` (new UCLASS)** — `AActor`, engine `Plane`
>   `UStaticMeshComponent` root, `NoCollision`/no shadow/no overlap, `bReplicates = true`.
>   `Initialize(float RadiusUnits)` (server) sets a replicated
>   `TelegraphRadius`; `ApplyTelegraphVisual()` (called from `BeginPlay` on every machine **and**
>   `Initialize` on the server) scales the plane `radius/50` and forces the `M_TargetRing` MID
>   `"Color"` to red `(0.90, 0.10, 0.10)`. No Tick. Mirrors `ACoopTargetRing`'s constructor shape,
>   minus the local-only flag + cursor Tick.
> - **`Core/CoopMonsterCharacter.h`** — `+void PerformStrike()`, `+FTimerHandle WindupTimerHandle`,
>   `+bool bWindingUp`, `+UPROPERTY() TWeakObjectPtr<AActor> ActiveTelegraph`,
>   `+UPROPERTY(EditDefaultsOnly, Category="Monster") TSubclassOf<AActor> StrikeTelegraphClass`.
> - **`Core/CoopMonsterCharacter.cpp`** — `+#include "Scenes/CoopMonsterStrikeTelegraph.h"`,
>   `+"Tags/CoopGameplayTags.h"`, `+"Engine/World.h"`. `PerformAttackTick` now: `bWindingUp` re-entry
>   guard → target/range checks (unchanged) → `bWindingUp = true`, spawn `StrikeTelegraphClass` at
>   the target's feet (`Z -= 88`), `Cast<ACoopMonsterStrikeTelegraph>()->Initialize(GetMeleeRangeUnits())`,
>   `SetTimer(WindupTimerHandle, &PerformStrike, MonsterAttackWindupSeconds)`. New `PerformStrike`:
>   clears `bWindingUp` + destroys `ActiveTelegraph` (always), then server-only re-check target
>   valid + not Downed + still in melee range → `ApplyDamage(MonsterAttackDamage)` + `LaunchCharacter`
>   away in XY (`bXYOverride=true`, `bZOverride=false`), impulse `MonsterKnockbackImpulse` scaled by
>   `(1 - FortressKnockbackResistPercent)` if the target `HasStatusTag(Status_Fortress)`; degenerate
>   same-XY case shoves along the monster's facing. `Destroyed()` + `HandleHealthDepleted` also clear
>   `WindupTimerHandle` + destroy `ActiveTelegraph`.
> - **`Core/CoopHealthComponent.cpp`** — `ApplyDamage`'s `Status_Shielded` early-return now also
>   fires on `Status_Fortress` (`||`). Comment rewritten (Shield/Fortress both negate; the
>   distinction is Fortress's radius coverage + knockback resist, both elsewhere).
> - **`Abilities/CoopTankAbilities.cpp`** — `ApplyShield` gains a parallel
>   `TActorIterator<ACoopMonsterCharacter>` loop after the teammate loop: same
>   `ShieldCoverageRadiusUnits` + `CoverageAngleCos` cone test, on a match `LaunchCharacter` the
>   monster directly away from `TankLocation` in XY by `ShieldShoveImpulse`. `#include
>   "Core/CoopMonsterCharacter.h"` was already present.
>
> **Self-review notes:** `PerformStrike`/`PerformAttackTick` are plain member fns (SetTimer's
> member-ptr overload — no `UFUNCTION` needed, same as the existing `PerformAttackTick`). Telegraph
> reads `TelegraphRadius` in client `BeginPlay` (initial replicated props land before `BeginPlay` on
> a newly-replicated actor). `ACoopMonsterCharacter` is an `ACharacter` (Phase A) so `LaunchCharacter`
> resolves in both the monster strike and the Shield-shove loop. No new `Build.cs` module dep
> (StaticMesh/Materials are `Engine`). `Status_Fortress` already declared in `CoopGameplayTags.h`.

### B1 — New `DA_GameConstants` fields  *(part of the B7 build — `.h` change)*

- [x] `GameConstants.h` — `MonsterAttackWindupSeconds = 1.0f` + `MonsterKnockbackImpulse = 650.0f`
      (`Category = "Monster"`); `ShieldShoveImpulse = 900.0f` (`Category = "Abilities"`). **Done —
      part of the B7 build.** `DA_GameConstants` fields get set in B8 (defaults are fine to start).

### B2 — Windup telegraph

- [x] Decision: **new `ACoopMonsterStrikeTelegraph`** (not reuse `ACoopTargetRing`, which is
      `bReplicates = false` local-only with per-player cursor Tick).
- [x] `Source/Unreal_first_Game/Scenes/CoopMonsterStrikeTelegraph.h/.cpp` — `AActor`, engine `Plane`
      root, `NoCollision`, `bReplicates = true`, `Initialize(float RadiusUnits)` → replicated
      `TelegraphRadius` → `ApplyTelegraphVisual()` scales the plane + forces `M_TargetRing` MID
      `"Color"` red. No Tick.
- [ ] `BP_MonsterStrikeTelegraph` over it (content, B8), `M_TargetRing` wired.
- [x] `CoopMonsterCharacter` — `TSubclassOf<AActor> StrikeTelegraphClass` `EditDefaultsOnly`;
      `TWeakObjectPtr<AActor> ActiveTelegraph`; `bool bWindingUp`; `FTimerHandle WindupTimerHandle`.

### B3 — Windup / strike split

- [x] `PerformAttackTick` — `bWindingUp` re-entry guard; on a passing range check: `bWindingUp = true`,
      spawn the telegraph at the target's feet, `SetTimer(WindupTimerHandle, this,
      &ACoopMonsterCharacter::PerformStrike, MonsterAttackWindupSeconds, false)`.
- [x] `PerformStrike` (new) — clears `bWindingUp` + destroys `ActiveTelegraph` always; then
      server-only re-check target valid + in melee range + not Downed → `ApplyDamage(MonsterAttackDamage)`
      + knockback (B4). Whiff otherwise.
- [x] `HandleHealthDepleted` + `Destroyed` — also clear `WindupTimerHandle` + destroy `ActiveTelegraph`.

### B4 — Knockback on strike

- [x] In `PerformStrike`, after `ApplyDamage`: `LaunchCharacter` the target away in XY by
      `MonsterKnockbackImpulse`, scaled by `(1 - FortressKnockbackResistPercent)` if the target
      holds `Status.Fortress`. Degenerate same-XY → shove along the monster's facing.
      `#include "Tags/CoopGameplayTags.h"` added.

### B5 — `CoopHealthComponent`: `Status.Fortress` negation

- [x] `ApplyDamage` — `if (... Status_Shielded || ... Status_Fortress) return;`. Comment rewritten.

### B6 — Shield-shove

- [x] `CoopTankAbilities::ApplyShield` — parallel `TActorIterator<ACoopMonsterCharacter>` loop after
      the teammate loop, same distance + cone test, `LaunchCharacter` each matched monster away from
      `TankLocation` in XY by `ShieldShoveImpulse`.

### B7 — One full external `Build.bat` from a closed editor  *(DONE 2026-09-04)*

- [x] Confirmed no `UnrealEditor` process. Re-reviewed every B1–B6 delta on disk:
      `CoopMonsterStrikeTelegraph.h`/`.cpp` (new `AActor`, `Plane` root, `bReplicates=true`,
      `DOREPLIFETIME(TelegraphRadius)`, `Initialize(float)`, idempotent `ApplyTelegraphVisual`, no
      Tick); `CoopMonsterCharacter.h`/`.cpp` (`PerformStrike` + `WindupTimerHandle` + `bWindingUp` +
      `ActiveTelegraph` `TWeakObjectPtr` + `StrikeTelegraphClass` `TSubclassOf`; `PerformAttackTick`
      → windup: `bWindingUp` guard, spawn telegraph with `AlwaysSpawn`, `SetTimer → PerformStrike`;
      `PerformStrike` → clear state + telegraph, re-check range, `ApplyDamage`, `LaunchCharacter`
      knockback with `Status_Fortress` resist + a zero-dir guard; `HandleHealthDepleted` + `Destroyed`
      both clear `WindupTimerHandle` + telegraph). `CoopHealthComponent.cpp` (`ApplyDamage` early-out
      now `Status_Shielded || Status_Fortress`). `CoopTankAbilities.cpp` (`TActorIterator<ACoopMonsterCharacter>`
      shove loop in `ApplyShield`, same cone test, `LaunchCharacter(dir * ShieldShoveImpulse, true,
      false)` — `#include "Core/CoopMonsterCharacter.h"` + `EngineUtils.h` already present).
      `GameConstants.h` (`MonsterAttackWindupSeconds` 1.0, `MonsterKnockbackImpulse` 650,
      `ShieldShoveImpulse` 900). All includes present.
- [x] Ran `Build.bat Unreal_first_GameEditor Win64 Development -project=... -waitmutex`. **Result:
      Succeeded, exit 0**, ~38s, 10 actions. UHT wrote **5 generated files** —
      `CoopMonsterStrikeTelegraph.generated.h` (3,862 B) fresh @ 10:40, plus `CoopMonsterCharacter` /
      `GameConstants` regenerated for the new reflected members. Recompiled `CoopHealthComponent` +
      `CoopTankAbilities`. `UnrealEditor-Unreal_first_Game.dll` relinked → **858,624 B, Sep 4 10:40**
      (was 837,632). **Zero warnings or errors.**
- [x] **User reopened the editor** — `unreal-mcp` reconnected; `search_subclasses(Actor, "Monster")`
      shows `/Script/Unreal_first_Game.CoopMonsterStrikeTelegraph`; `BP_MonsterCharacter` CDO showed
      `strikeTelegraphClass = None` (not-yet-wired, expected). Then B8.

### B8 — Content  *(DONE 2026-09-04)*

- [x] `DA_GameConstants` — the B1 fields read their C++ defaults (`monsterAttackWindupSeconds` 1.0 /
      `monsterKnockbackImpulse` 650 / `shieldShoveImpulse` 900), **no per-asset override** — same
      call as A7. Asset not dirty. Revisit only if a real playtest wants different tuning.
- [x] `BP_MonsterStrikeTelegraph` — created fresh over `/Script/Unreal_first_Game.CoopMonsterStrikeTelegraph`
      at `/Game/Blueprints/Scenes/`. `ringMaterial` = `/Game/Materials/M_TargetRing` (read off
      `BP_TargetRing`'s CDO and mirrored — the ring shape / `radius/50` scale / `"Color"`-red are all
      in C++ `ApplyTelegraphVisual()`, so the BP only needs the material ref). Compiled clean
      (`warnings_as_errors`).
- [x] `BP_MonsterCharacter` CDO — `strikeTelegraphClass` = `BP_MonsterStrikeTelegraph_C` (was `None`).
      Compiled clean.
- [x] Saved both BPs; `is_dirty == false` for `BP_MonsterStrikeTelegraph`, `BP_MonsterCharacter`,
      `DA_GameConstants`, `BP_MonsterSpawner`. On disk 10:55 (`BP_MonsterStrikeTelegraph.uasset`
      30,568 B new; `BP_MonsterCharacter.uasset` updated). `git`: `M BP_MonsterCharacter.uasset` +
      `?? BP_MonsterStrikeTelegraph.uasset`, nothing else in `Content/`.

### B9 — PIE verification

> **PARTIAL (2026-09-04) — same dev-mode limitation as A8.** Solo dev-mode PIE (`bDevMode` on,
> throttle off per DECISIONS.md M12, both restored after). 1 real client + 4 idle dummies, one
> `UEDPIE_0` world. Verified everything the telegraphed-strike path can be driven to solo; the
> Fortress / Shield-shove checks still need a real Tank + Control and are deferred.

- [x] **B9.1** ✅ — RoleSelect (10s) → Prep (10s) → HoldTheGate ran clean. ~130 monsters spawned
      across the run as `BP_MonsterCharacter_C`, auto-possessed, fixating on the 4 non-Tank dummies;
      **no** `Error` / `Accessed None` / `LogScript`/`LogBlueprint` warning for `CoopMonsterCharacter`
      / `CoopMonsterStrikeTelegraph` / `CoopMonsterAIController`.
- [x] **B9.2 — telegraph** ✅ — `BP_MonsterStrikeTelegraph_C` actors spawn during windups (caught
      `_13`/`_14` live; suffix ≥15 means many had already cycled), each transient
      ~`MonsterAttackWindupSeconds` then destroyed on strike-resolve. **The red ring itself was not
      screenshotted** — `ApplyTelegraphVisual` runs in `BeginPlay` (an editor-placed instance shows
      M_TargetRing's default tint, not the `(0.90,0.10,0.10)` runtime override) and `CaptureViewport`
      doesn't render PIE actors (A8.2's carve-out). The visual is `ACoopTargetRing`'s proven Plane +
      `M_TargetRing` + `radius/50` scale + `"Color"` param, byte-for-byte.
- [x] **strike + damage** ✅ — dummy `BP_PlayerCharacter_C_1` went 80 → 0 `currentHealth` in 5-point
      steps (`MonsterAttackDamage` = 5) once in the swarm. No monster that had *not* reached melee
      range dealt damage (dummies at the raised PlayerStart line held full HP for ~90s until one was
      manually dropped into the cluster).
- [x] **B9 knockback (B4)** ✅ — that same dummy was flung from `(560,460)` to `(1441,987)` (~1000
      units) by repeated strikes. `LaunchCharacter` + `MonsterKnockbackImpulse` fires and moves the
      target hard. Un-Fortress'd → full impulse (the resist path is B9.4).
- [x] **Downed + retarget** ✅ (bonus — this is the A8.5 gap) — at 0 HP the dummy went Downed and
      ~10 monsters logged `OnTargetDowned: ... retargeted from BP_PlayerCharacter_C_1 to ...`.
- [ ] **B9.3 — knockback dislodges a plate-holder → gate closes.** DEFERRED to a real 5-player
      playtest — nothing makes a dev dummy `StandOn` a plate, so there's no plate-holder to shove
      off. The knockback *force* is confirmed (B4); whether it reliably clears `ACoopPressurePlate`'s
      overlap band is the untested part. If not, tune `monsterKnockbackImpulse` up.
- [ ] **B9.4 — Fortress negation + knockback resist.** DEFERRED — needs a Control player
      `Stabilize`-ing a Shielded Tank so `Status.Fortress` lands on a plate-holder, then a strike.
- [ ] **B9.5 — Shield-shove.** DEFERRED — needs a Tank raising Shield facing a monster cluster.
- [ ] **B9.6 — Shield still negates damage but not knockback** (regression). DEFERRED — needs a
      Shielded (not Fortress'd) plate-holder taking a monster hit.
- [x] **B9.7 — cleanup** — `StopPIE` (confirmed `IsPIERunning` false); `BP_GameMode` `bDevMode`
      → `false` (value confirmed; BP left **dirty in-memory** from the on/off toggle — disk
      unchanged, not in `git`, discard on close); `EditorPerformanceSettings`
      `bThrottleCPUWhenNotForeground` / `bAllowSlateThrottling` → `true`/`true`; `DA_GameConstants` /
      `BP_MonsterCharacter` / `BP_MonsterStrikeTelegraph` / `BP_MonsterSpawner` all `is_dirty ==
      false`; `Lvl_ThirdPerson` left dirty-in-memory (a temp telegraph actor added then removed for a
      visual check — disk unchanged, `git` clean, no stray OFPA file). No C++ touched → no rebuild.

---

## Phase C — docs  *(text only, no build — DONE 2026-09-04)*

### C1 — `DECISIONS.md`

- [x] Extended **"Monster combat inside Hold the Gate"** with a **"Follow-up (2026-09-04): the
      monsters now move and attack in melee"** subsection: reparent `AActor`→`ACharacter` + Mannequin
      + `ECC_Visibility` re-block; `ACoopMonsterAIController` = straight-line `AddMovementInput`
      steering, **still no navmesh / behaviour tree / pathfinding** (explicitly in-bounds — the
      original carve-out forbade *pathfinding*, not *motion*); body-block is free; melee-range-gated
      attack → `MonsterAttackWindupSeconds` telegraph (`ACoopMonsterStrikeTelegraph`, replicated) →
      `PerformStrike` (damage + knockback); knockback tuned to dislodge a plate-holder → the plate's
      own overlap logic closes the gate, no plate code changed; first consumer of
      `FortressKnockbackResistPercent`; the `Status.Fortress` `ApplyDamage` gap fixed here;
      `ApplyShield` gains a monster-cone `LaunchCharacter` shove loop (`ShieldShoveImpulse`); two
      closed-editor builds; the PIE-verification split (Phase A + telegraph/strike/knockback/Downed-
      retarget confirmed solo; Fortress negation / Shield-shove / plate-dislodge need a real
      playtest). Plus a "Still in bounds" paragraph (no generic system, no adaptive AI).

### C2 — `docs/scenes/HOLD_THE_GATE.md`

- [x] "The room": rewrote the "Monster chambers" bullet (straight-line chase → melee windup ring →
      strike + knockback), added a "Why the knockback matters" bullet (dislodges plate-holders →
      gate closes) and a "Tank's job" bullet with **body-block** + **Shield-shove** sub-bullets;
      Shield bullet notes it doesn't resist knockback; Fortress bullet spells out radius negation +
      `(1 - FortressKnockbackResistPercent)`. "Tags read/written" table: both rows now list the
      `ApplyDamage` / `PerformStrike` readers + a note that the windup is not a tag. "Where authority
      resolves": replaced the stale "that `DECISIONS.md` file doesn't exist yet" note with a real
      cross-ref, added movement / melee-attack / shield-shove / `ApplyDamage`-negation authority
      lines, kept the "no navmesh / BT / pathfinding — straight-line steering only" caveat. Updated
      the MCP-buildable-vs-C++ table.  *(The doc never actually contained the phrase "stationary
      ranged harassers" — it said "spawning threats"; that wording is now fully replaced.)*

### C3 — `docs/abilities.md`

- [x] **Shield** entry — added the monster-cone shove (`ShieldShoveImpulse`, server-only, one-time
      `LaunchCharacter` on raise), clarified `Status.Shielded` negates damage but not knockback,
      added a `Q`-key prototype-status line.
- [x] **Stabilize / Fortress** entry + **both** tag-glossary rows (`Status.Shielded` and
      `Status.Fortress`) — Fortress now actually negates damage via `ApplyDamage` across
      `FortressCoverageRadiusUnits` (was written-but-never-read until this pass) **and** resists
      knockback by `FortressKnockbackResistPercent` (read in `ACoopMonsterCharacter::PerformStrike`);
      `Status.Shielded` negates damage, not knockback. Added the `Q`-key + "needs a real Tank+Control
      playtest for B9.4" prototype-status line.

---

## Log
(Newest at the bottom. One line per completed step.)

- **Plan written (2026-09-04).** Enemies (currently stationary red cones) → moving Mannequin
  enemies for Hold the Gate. Design agreed in chat: reparent `ACoopMonsterCharacter`
  `AActor`→`ACharacter`, stock Mannequin + red "Paint Tint", new `ACoopMonsterAIController`
  (straight-line `AddMovementInput`, no navmesh — extends the `DECISIONS.md` "Monster combat"
  carve-out, which only forbade pathfinding), melee-range-gated attack, Phase B adds a windup
  telegraph + knockback (tuned to dislodge plate-holders, per user) + the `Status.Fortress`
  damage-negation fix (`CoopHealthComponent` never read the tag) + `FortressKnockbackResistPercent`
  as its first consumer + Shield-shove in `ApplyShield`. Two closed-editor builds (one per phase),
  Phase C docs. Next: Phase A C++ (A1–A3 + the A7 `.h` fields) in one batch, then A5 rebuild.

- **Phase A C++ done — one uncommitted batch (2026-09-04).** Not built. `CoopMonsterAIController.h/.cpp`
  written (new UCLASS, straight-line steer, no navmesh/BT — mirrors `ADummyAIController`).
  `CoopMonsterCharacter` reparented `AActor`→`ACharacter`: capsule `34/88`, Mannequin body deferred
  to `BP_MonsterCharacter` (A6), `MaxWalkSpeed`/`bOrientRotationToMovement`, `AIControllerClass` +
  `AutoPossessAI::PlacedInWorldOrSpawned`, tint retargeted to `GetMesh()` + `"Paint Tint"`,
  `InitializeMonster` overrides `MaxWalkSpeed` from `GameConstants`, new `GetMeleeRangeUnits()` +
  `Destroyed()` (kills the possessing controller — covers death + scene-reset), melee-range gate in
  `PerformAttackTick`. `GameConstants.h` +`MonsterMoveSpeed`/`MonsterMeleeRangeUnits`. A4 call-site
  sweep clean (details in the A1–A4 block above). **Next: A5 — user closes the editor, then the
  external `Build.bat`** (new UCLASS + base-class change → Live Coding unsafe, `DECISIONS.md`).

- **A5 rebuild done (2026-09-04).** Resumed session; the editor was still open so asked the user to
  close it, then confirmed no `UnrealEditor` process. Re-reviewed all Phase-A deltas on disk and
  **caught a plan gap:** the reparent drops the cone's `BlockAllDynamic` mesh → the inherited
  `Pawn`-profile capsule ignores `ECC_Visibility` → monsters would stop being click-selectable,
  breaking Armor Break / Execution / Overload targeting. User approved the one-line fix
  (`GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block)` in `BeginPlay`,
  mirroring `ACoopCharacter::BeginPlay`); added it before the build. Ran full external `Build.bat`:
  **Succeeded, exit 0**, ~71s, 7 actions. UHT: 6 generated files, `CoopMonsterCharacter.gen.cpp` now
  reflects `ACharacter`. DLL → **837,632 B, Sep 4 00:36** (was 822,784). No warnings/errors — the
  base-class change broke no call site. **Next: A5.4 needs the user — reopen the editor so
  `unreal-mcp` reconnects and `search_subclasses` confirms `CoopMonsterAIController` (an
  `AAIController`) + `CoopMonsterCharacter` (now a `Character`); then A6 (`BP_MonsterCharacter`:
  confirm/repair the reparent, wire the Mannequin mesh/anim), A7, A8 PIE.**

- **A5.4 + A6 + A7 done (2026-09-04).** Resumed; editor already open, `unreal-mcp` connected, PIE
  off. `search_subclasses` confirmed both new/reparented classes are live (DLL loaded). A6: the
  reparented `BP_MonsterCharacter` was **corrupt** — CDO `mesh` read `None` because the old
  AActor-era BP had an SCS root parented to the deleted cone `Mesh` component
  (`FixupRootNodeParentReferences` warning in the log; a native→`ACharacter`→back round-trip didn't
  clear it). Recreated fresh per the plan's A6.1 fallback: nulled + saved `BP_MonsterSpawner.monsterClass`,
  deleted `BP_MonsterCharacter`, `create`d it new over `CoopMonsterCharacter`. Fresh CDO has
  `CharacterMesh0`. Wired `SKM_Quinn_Simple` + `ABP_Unarmed_C` + loc `(0,0,-89)` / yaw `270` on
  the mesh (mirrored from `BP_PlayerCharacter`), `gameConstants` = `DA_GameConstants`; capsule 34/88
  + `AIControllerClass`/`AutoPossessAI` inherited from C++. Re-pointed `BP_MonsterSpawner.monsterClass`.
  Both BPs compile clean, saved, `is_dirty == false`. A7: `DA_GameConstants` `monsterMoveSpeed 350` /
  `monsterMeleeRangeUnits 160` (C++ defaults, no override).

- **A8 PIE verification — partial, Phase A closed (2026-09-04).** Solo dev-mode PIE (`StartPIE`
  auto-ran a 5-client session), throttle off then restored, `bDevMode` on then restored. **Verified:**
  monsters spawn as `ACoopMonsterCharacter`/`ACharacter`, auto-possessed by `ACoopMonsterAIController`,
  straight-line-chase their fixate target (positions moved 100s of units/read; `LogCharacterMovement`
  velocity + "is stuck" confirm active steering), fixate correctly **excludes the Tank** every wave,
  clean log (no Monster-class errors/warnings), and the **melee gate holds** — no target ever lost
  HP from an un-arrived monster (old cross-room harassment gone). **Two content fixes during A8** (no
  C++): (1) wired `BP_MonsterCharacter.HealthComponent.gameConstants` — the fresh recreate had lost
  it, causing a `GameConstants not set` warning per spawn; (2) the 4 placed `BP_MonsterSpawner`
  actors had `monsterClass == None` (deleting the old BP in A6 nulled the in-memory ref on loaded
  level actors; `EditDefaultsOnly` blocks a per-instance re-set) → fixed by `load_level` reload (disk
  still stored the path, which the recreate reoccupies; no level save, `git` shows level unmodified).
  **Blocked (dev-mode limitation, not a Phase-A bug):** A8.3-positive / A8.4 body-block / A8.5
  retarget-on-down / A8.6 death+no-orphan — dev dummies idle at Z≈302 (raised spawn platform),
  monsters at Z≈90, so the 3D melee gate never passes on a dummy and no hit lands. Verify in a real
  5-player playtest. A9 = N/A (no C++ touched). **Next: Phase B.**

- **Phase B C++ (B1–B6) done — one uncommitted batch (2026-09-04). NOT built.** New UCLASS
  `Scenes/CoopMonsterStrikeTelegraph.h/.cpp` (replicated flat red `Plane` ground ring, `Initialize`
  + `ApplyTelegraphVisual`, no Tick — mirrors `ACoopTargetRing` minus the local-only/cursor bits).
  `GameConstants.h` +`MonsterAttackWindupSeconds 1.0`/`MonsterKnockbackImpulse 650` (Monster) +
  `ShieldShoveImpulse 900` (Abilities); `FortressKnockbackResistPercent` comment de-staled.
  `CoopMonsterCharacter` +`PerformStrike`/`bWindingUp`/`WindupTimerHandle`/`ActiveTelegraph`/
  `StrikeTelegraphClass`: `PerformAttackTick` now starts a telegraphed windup instead of hitting
  instantly; `PerformStrike` re-checks range then `ApplyDamage` + `LaunchCharacter` knockback
  (Fortress-resist-scaled); `Destroyed`/`HandleHealthDepleted` clear the windup + telegraph.
  `CoopHealthComponent::ApplyDamage` now also negates under `Status.Fortress` (the gap where nothing
  read the tag). `CoopTankAbilities::ApplyShield` +monster-cone `LaunchCharacter` shove loop.
  Self-reviewed (member-ptr timers need no `UFUNCTION`; client reads replicated `TelegraphRadius` in
  `BeginPlay`; `ACoopMonsterCharacter` is an `ACharacter` so `LaunchCharacter` resolves; no new
  `Build.cs` dep). **Next: B7 — user closes the editor, then the external `Build.bat`** (new UCLASS
  → Live Coding unsafe, `DECISIONS.md`).

- **B7 rebuild done (2026-09-04).** Resumed session, confirmed no `UnrealEditor` process, re-reviewed
  every B1–B6 delta on disk vs. the plan (details in the B7 checklist above) — all consistent, all
  includes present. Ran full external `Build.bat`: **Succeeded, exit 0**, ~38s, 10 actions, all 6
  changed `.cpp`s compiled (`CoopMonsterStrikeTelegraph.cpp` new). UHT wrote 5 generated files;
  `CoopMonsterStrikeTelegraph.generated.h` fresh @ 10:40. DLL → **858,624 B, Sep 4 10:40** (was
  837,632). No warnings/errors. **Next: B8 needs the user — reopen the editor so `unreal-mcp`
  reconnects, then: create `BP_MonsterStrikeTelegraph` (over `ACoopMonsterStrikeTelegraph`, wire
  `M_TargetRing` on the plane), set `BP_MonsterCharacter` CDO `strikeTelegraphClass`, set the 3 new
  `DA_GameConstants` fields; then B9 PIE, then Phase C docs.**

- **B8 + B9 + Phase C done — Phase B closed (2026-09-04).** Editor was already open; `unreal-mcp`
  connected, `search_subclasses` confirmed `CoopMonsterStrikeTelegraph` loaded from the 10:40 DLL.
  **B8:** created `BP_MonsterStrikeTelegraph` fresh over the C++ class, wired `ringMaterial` =
  `M_TargetRing` (mirrored from `BP_TargetRing`'s CDO — everything else is in C++
  `ApplyTelegraphVisual`), set `BP_MonsterCharacter.strikeTelegraphClass` = `BP_MonsterStrikeTelegraph_C`,
  both compiled + saved (`is_dirty == false`, on disk 10:55). The 3 new `DA_GameConstants` fields
  read their C++ defaults (1.0 / 650 / 900) with no override — same as A7. `git`: `M
  BP_MonsterCharacter.uasset` + `?? BP_MonsterStrikeTelegraph.uasset` only. **B9:** solo dev-mode
  PIE (throttle off per M12, restored after). Verified: clean RoleSelect→Prep→HoldTheGate loop, no
  new script errors; `BP_MonsterStrikeTelegraph_C` actors spawn during windups (transient ~1s);
  strike + `ApplyDamage` lands (dummy 80→0 in 5-dmg steps); `LaunchCharacter` knockback flung a
  dummy dropped into the swarm ~1000 units; Downed-at-0 → ~10 monsters retargeted (also covers the
  A8.5 gap). **B9.3 (plate-dislodge) / B9.4 (Fortress negation + resist) / B9.5 (Shield-shove) /
  B9.6 (Shield regression) remain deferred to a real 5-player playtest** — same dev-dummy limitation
  A8 hit (nothing puts a dummy on a plate; no live Tank/Control). **Phase C:** `DECISIONS.md`
  "Monster combat inside Hold the Gate" got a "Follow-up (2026-09-04)" subsection; `docs/scenes/HOLD_THE_GATE.md`
  and `docs/abilities.md` (Shield + Stabilize/Fortress entries + both tag-glossary rows) updated.
  Editor left with `BP_GameMode` + `Lvl_ThirdPerson` **dirty in-memory only** (bDevMode toggle + a
  temp visual-check actor) — both unchanged on disk / in `git`; discard on close, do **not** save.
  **All plan work complete. Next: user commits (source from Phases A/B + the two content assets +
  the doc updates), then a real 5-player playtest for the deferred B9 items.**
