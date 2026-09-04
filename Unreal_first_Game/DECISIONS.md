# DECISIONS.md — Case Law

Anything settled in a past session that future sessions must respect, logged here per
CLAUDE.md §10 ("Update DECISIONS.md with anything settled during a session that future
sessions must respect. Do not let knowledge live only in chat history.").

## Pawn visuals: stock Mannequin instead of bare capsules

**Decision:** Player characters use the ThirdPerson template's stock Unreal Mannequin
(Manny/Quinn) as-is, instead of the originally-planned bare capsule primitives. Per-player
and per-role identification is done with a Dynamic Material Instance colour tint on the
Mannequin's base material, not a new mesh or custom art.

**Why:** The Mannequin ships pre-rigged and pre-animated (idle/walk/run/jump) for free —
using it costs zero additional art or animation production time, so it doesn't reopen the
scope-creep risk that the original "capsules and coloured primitives" rule (CLAUDE.md §5)
was written to avoid, especially for a team with no prior game-dev experience (§2). This
is a narrow visual-fidelity upgrade, not a reversal of §1's "disposable prototype, ugly is
correct" philosophy — custom character art, custom animation/rigging work, VFX,
post-processing, and MetaHumans specifically all remain out of scope (§8).

**Still true / unchanged:** no custom character models, no bespoke animation or rigging
work, no VFX, no post-processing, no MetaHumans. Status-condition bars, camera behaviour,
and every other §5 rule are unaffected — only the base mesh and its per-player colouring
changed.

## Engine version pin corrected: 5.8.2 → 5.8.1

**Decision:** CLAUDE.md §3 now pins Unreal Engine 5.8.1, not 5.8.2.

**Why:** A Build 0 planning session (2026-08-24) surveyed the repo and found the engine
that has actually been opening and running this project (per `Saved/Logs`) is 5.8.1, not
the 5.8.2 the doc specified. Corrected the doc to match reality rather than reinstalling
the engine — there was no reason to believe 5.8.2 specifically was needed, and matching
the doc to the already-working install avoids losing time to a reinstall for no benefit.

**Still true / unchanged:** the pin-exact-patch-version rule itself (§3) is unchanged —
every teammate's install must still match, only the target number moved.

## `.uproject` EngineAssociation must be a plain version string, not a GUID

**Decision:** `Unreal_first_Game.uproject`'s `EngineAssociation` field must be the plain
Launcher-tracked version string `"5.8"`, not a custom-build GUID.

**Why:** A Build 0 session (2026-08-24) regenerated Visual Studio project files directly via
`UnrealBuildTool.dll -projectfiles` (bypassing the Epic Games Launcher's own "Generate project
files" flow). That path registered the engine as a *custom build*, writing a GUID
(`{3F1E7983-4073-5CC1-C16D-B58739E08990}`) into `EngineAssociation` and a matching entry under
`HKCU\SOFTWARE\Epic Games\Unreal Engine\Builds` in the registry. That registry entry never
persisted (didn't survive to the next session/reboot — exact mechanism not confirmed, not worth
chasing further since the fix is simple and robust either way). Confirmed via
`C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat` that the Launcher already tracks
this exact install (`C:\Program Files\Epic Games\UE_5.8`) correctly under `AppName: "UE_5.8"`, so
a GUID/custom-build association was never necessary here — pointing `EngineAssociation` at the
plain `"5.8"` string lets the Editor resolve the engine directly through the Launcher's own
tracked install list, with no registry dependency to go stale.

**Symptom to recognize this by:** opening the `.uproject` (double-click, or launching
`UnrealEditor.exe <uproject>`) fails or prompts that the project "was created with a different
version," even though the correct engine is installed and was working in a prior session.

**Still true / unchanged:** the engine itself is still 5.8.1 exactly per the entry above — this
was never a wrong-engine-installed problem, only a stale association-metadata problem. If
project files ever need regenerating again, prefer the Epic Games Launcher's own right-click →
"Generate Visual Studio project files" over calling `UnrealBuildTool.dll -projectfiles` directly,
to avoid re-triggering the custom-build GUID path.

## Monster combat inside Hold the Gate, ahead of Build 2's general system

**Decision:** Scene 2 (Hold the Gate) gets real monster combat — a spawner, monster HP, and an
AI targeting behaviour that fixates on a random plate-holder and retargets to another
plate-holder when its current target dies or goes Downed — built as part of Build 1, even
though CLAUDE.md §6.5 says "the boss has no AI" and reserves general monster/boss systems for
Build 2.

**Why:** Hold the Gate is Build 1's chosen first scene specifically because it's "the cheapest
to build... and has the highest social density — all five players are under pressure
simultaneously" (CLAUDE.md §7). That scene's own design (§6.4) requires monsters that threaten
the four pinned plate-holders while the Tank is the only mobile player — without real monster
combat, the scene has no actual threat for Shield/Fortress to defend against, so the mechanic
this build exists to test (the Fortress synergy) can't be exercised at all. Building a minimal,
scene-local spawner/targeting system now is cheaper than either faking the threat or blocking
all of Build 1 on Build 2's general monster/boss system landing first.

**Scope boundary:** this is scoped to Hold the Gate only. The targeting *behavior*
(fixate-then-retarget) is written as a small reusable component per the Build 1 plan, since
Gravity Bridge (Scene 1) is expected to reuse the same pattern later — but spawn choreography
(timing, escalation curve, which plate is targeted) stays local to this scene's own class, not a
general system. Build 2 still owns the general monster/boss framework and the boss's own
scripted-timeline sequencer (§6.5) — this entry does not authorize adaptive AI, behavior trees,
or pathfinding anywhere, and does not extend to the mini-boss's own phase logic beyond reusing
the same monster/targeting base classes.

**Still true / unchanged:** CLAUDE.md §6.5's "the boss has no AI" rule stands for the actual
boss and for Build 2's general system — this is a narrow, logged, user-approved exception for
one scene's trash monsters, not a reversal of that rule.

**Follow-up (2026-09-04): the monsters now move and attack in melee.** `MONSTER_ENEMIES_PROGRESS.md`
covers the full plan; the load-bearing decisions:

- **`ACoopMonsterCharacter` was reparented `AActor` → `ACharacter`, in place** (same class/name/file,
  so every `Cast<ACoopMonsterCharacter>` call site kept working). The custom cone `UStaticMeshComponent`
  root is gone; the body is the stock Mannequin (`SKM_Quinn_Simple` + `ABP_Unarmed`, wired on
  `BP_MonsterCharacter`, mirroring `BP_PlayerCharacter`) with a dark-red `"Paint Tint"` MID on every
  mesh slot. One-liner in `BeginPlay` re-blocks `ECC_Visibility` on the inherited capsule (the
  reparent dropped the cone's `BlockAllDynamic` mesh) so monsters stay click-targetable for Armor
  Break / Execution / Overload — the same fix `ACoopCharacter::BeginPlay` already applies for players.
- **New `ACoopMonsterAIController : AAIController`** (in `Core/`, not `Dev/` — this is real gameplay
  AI now). It `Tick()`s, reads `TargetingComponent->GetCurrentTarget()`, and while the target is
  beyond melee range does `AddMovementInput((TargetLoc - PawnLoc).GetSafeNormal2D())`. **Straight-line
  steering only — no `MoveToActor`, no navmesh, no behaviour tree, no pathfinding.** This is a
  deliberate, in-bounds extension of the carve-out above: that scope boundary forbade *pathfinding*,
  not *motion*. `bOrientRotationToMovement = true`, so the monster faces where it walks. Body-blocking
  the Tank is free (capsule vs capsule) — no code. Movement rides the default
  `CharacterMovementComponent` replication (CLAUDE.md §4.2's one sanctioned client-prediction).
- **The attack is melee-range-gated with a telegraphed windup.** `PerformAttackTick` (the existing
  repeating `MonsterAttackIntervalSeconds` timer) only proceeds when the target is within
  `MonsterMeleeRangeUnits`; on a passing tick it sets `bWindingUp`, spawns a flat red ground ring
  (`ACoopMonsterStrikeTelegraph` — a new small replicated `AActor`, engine Plane + `M_TargetRing`
  forced red, mirroring `ACoopTargetRing` minus its local-only cursor Tick), and arms a
  `MonsterAttackWindupSeconds` one-shot → `PerformStrike`. `PerformStrike` re-checks range (a
  body-block / Shield-shove / the target walking away in the window = a whiff), then `ApplyDamage`
  + `LaunchCharacter` knockback directly away in XY. "About to hit" is transient AI state
  (`bool bWindingUp`), **not** an `FGameplayTag` — no `docs/abilities.md` change for it.
- **Knockback is tuned to shove a plate-holder off their plate** (user decision): the push moves the
  character out of `ACoopPressurePlate`'s overlap band → the plate's own `OnOccupancyChanged` fires →
  the gate closes. **No plate code changed.** This makes the monster threat attack the *objective*,
  not just HP, and is what makes Fortress's knockback-resist matter. `MonsterKnockbackImpulse` is
  the first real consumer of nothing new; `FortressKnockbackResistPercent` (added in an earlier
  milestone with "no consumer yet") gets its **first consumer** here — a `Status.Fortress` target is
  launched only `(1 - FortressKnockbackResistPercent)` as far.
- **The `Status.Fortress` damage-negation gap was fixed as part of this.** `Stabilize` wrote
  `Status.Fortress` and the UI showed it, but `CoopHealthComponent::ApplyDamage` only early-returned
  on `Status.Shielded` — nothing read Fortress for defense (invisible until real monster damage
  existed). `ApplyDamage` now early-returns on `Status.Shielded || Status.Fortress`. `Status.Shielded`
  keeps negating damage and deliberately does **not** resist knockback — that stays the real
  Shield → Fortress upgrade distinction (Fortress = damage negation across its radius **plus**
  knockback resist).
- **`CoopTankAbilities::ApplyShield` now also shoves monsters.** A parallel
  `TActorIterator<ACoopMonsterCharacter>` loop after the existing teammate loop, same cone test,
  `LaunchCharacter`s each matched monster away from the Tank by `ShieldShoveImpulse` (new). Shield
  becomes a repositioning tool, not just a damage filter (`docs/scenes/HOLD_THE_GATE.md`'s "knock
  enemies away"). Fortress adds no shove of its own — its upgrade value stays multi-teammate damage
  coverage + the knockback resist.
- **Two closed-editor `Build.bat` rebuilds** (one per phase — a base-class change + new UCLASSes are
  Live-Coding-unsafe per the entry below), both `exit 0`, no warnings. Verification: Phase A + the
  telegraph/strike/knockback/Downed-retarget path are confirmed in solo dev-mode PIE; the Fortress
  negation, Shield-shove, and plate-dislodge checks need a real 5-player playtest (dev dummies idle
  on a raised platform out of monster melee reach) — tracked in `MONSTER_ENEMIES_PROGRESS.md` B9.

**Still in bounds:** no generic ability/effect system (CLAUDE.md §4.6) — the windup/strike/knockback
is hand-written on `ACoopMonsterCharacter`, the Fortress read is one `||`, the Shield-shove is one
loop. No navmesh / behaviour tree / pathfinding / adaptive difficulty anywhere. Still scoped to
Hold the Gate's trash monsters; Build 2 still owns the general framework and the boss's sequencer.

## Role assignment is player-chosen, not random

**Decision:** CLAUDE.md §6.1 and §6.3's original wording ("Roles are assigned **randomly** at
run start") is overridden. Roles are instead selected by each player: once the 5-player roster
is complete, a RoleSelect phase opens in which each real player claims any still-unclaimed role
for themselves (first claim wins, server-authoritative via `Server_ClaimRole`). A
`RoleSelectDurationSeconds` timeout auto-assigns any remaining unclaimed roles at random to
players (or dev-mode dummies) who haven't picked, so the session can't stall indefinitely on one
AFK player. The prep arena's own 60-second countdown does not start until role selection
resolves.

**Why:** explicit user instruction during Build 1 planning (2026-08-24): "Dont make the role
assignment random. Make it possible for the player to choose which role he wants to play
first." Per CLAUDE.md's own top-level rule ("If a request in a session conflicts with a rule in
this file, stop and say so rather than silently working around it"), this conflict was flagged
to the user before being implemented, and the user's instruction stands as a deliberate,
explicit override — CLAUDE.md §6.1/§6.3 have been updated to match so the doc and the
implementation don't disagree.

**Still true / unchanged:** everything else about role structure is unchanged — still exactly 5
roles (TANK/SUPPORT/RUNNER/CONTROL/DAMAGE), still exactly one player per role, still temporary
to the run rather than persistent. Only the assignment *mechanism* changed, from random shuffle
to player claim with a randomized timeout fallback.

## Live Coding must not be used to add a new UCLASS/UPROPERTY/UFUNCTION

**Decision:** Live Coding (Ctrl+Alt+F11 / the in-editor "Compile" button) is only safe for
editing the *body* of functions on classes that already exist and are already loaded. Adding a
new `UCLASS`/`USTRUCT`/`UENUM`, adding a new `UPROPERTY`/`UFUNCTION` to an existing reflected
class, or changing a class's inheritance requires closing the editor and doing a full build
(Visual Studio/Rider Build, or `Engine\Build\BatchFiles\Build.bat`) before reopening it.

**Why:** during Build 1 (2026-08-24), adding `UCoopAbilityCardWidget` (a new `UUserWidget`
subclass with new `UPROPERTY`/`UFUNCTION` members) and then triggering a Live Coding compile
crashed the editor: `EXCEPTION_ACCESS_VIOLATION` inside the freshly-linked `patch_0` DLL's
`DllMain`, in the dynamic initializer for a UMG stat counter (`StatPtr_STAT_CreateWidget`,
`UserWidget.h:1816`). The editor log showed the crash landed within seconds of the patch DLL
finishing linking — `Starting Live Coding compile` → `link.exe` creates `patch_0.lib/.exp` →
`RequestExit(..., EngineUnhandledExceptionFilter)`. Live Coding's patch DLL runs its static
initializers inside `DllMain` under the Windows loader lock; that's tolerant of function-body-only
changes (confirmed working all day — 20 successful patches, `patch_0`...`patch_19`, in the prior
session) but not of new reflected types/members, which is a known Live Coding limitation, not a
bug in the widget code itself. No source was lost — the crash happened after the edited `.cpp`/
`.h` files were already saved to disk and successfully linked; only the in-memory patch load
failed.

**Still true / unchanged:** built-in movement prediction, replication, and every other engine
behavior are unaffected — this is purely an editor-workflow rule about when Live Coding vs. a
full rebuild is required, not an architecture change.

**Addendum (2026-08-25, M8): the crash risk is narrower than the rule above assumes — new
UFUNCTIONs/UPROPERTYs on an already-loaded non-widget UCLASS compiled clean via Live Coding.**
During M8, the user triggered an in-editor Live Coding compile (not the full external rebuild
this session had asked for) after adding two new `UFUNCTION`s (`ActivateStabilize`/
`Server_ActivateStabilize`) to the already-loaded `ACoopPlayerController`, plus five new
`UPROPERTY` fields to the already-loaded `UGameConstants`. The log shows a fully clean reload —
`LogClass` listed every new/modified function and both reloaded classes individually, ending in
`Reload/Re-instancing Complete: 1 package changed, 2 classes changed, 13 classes unchanged, 3
enums unchanged, 36 functions remapped` and the same benign `data type changes may cause
packaging to fail` warning every prior milestone's Live Coding compile has also produced — no
`EXCEPTION_ACCESS_VIOLATION`, no crash, editor stayed fully responsive afterward (confirmed via a
follow-up `IsPIERunning` call succeeding immediately). This is a real counterexample to the
original entry's blanket claim that new `UPROPERTY`/`UFUNCTION` additions require a full rebuild.

**Why the original crash likely doesn't generalize:** the documented crash's own stack trace
was inside a **UMG stat counter's dynamic initializer** (`StatPtr_STAT_CreateWidget`,
`UserWidget.h:1816`), triggered while adding `UCoopAbilityCardWidget` — a **brand-new
`UUserWidget`-derived `UCLASS`**, not just new members on an existing one. `ACoopPlayerController`
and `UGameConstants` are neither new classes nor UMG widgets, so this session's clean result is
consistent with the real trigger being specific to first-time UMG widget class registration
under Live Coding's patch-DLL loader-lock, not new members on an arbitrary already-loaded class.

**Revised guidance:** Live Coding remains **confirmed unsafe for adding a brand-new `UCLASS`**
(especially a `UUserWidget` subclass) — treat that case exactly as the original entry says: close
the editor, full external rebuild. For adding new `UFUNCTION`/`UPROPERTY` members to an
**already-existing** reflected class, Live Coding has now worked cleanly at least once (M7 avoided
testing it out of caution; M8 tested it, by accident, and it held) — still worth defaulting to a
full rebuild when in doubt (one success isn't proof for every class shape), but this is no longer
an automatic, uninvestigated "must rebuild" — a Live Coding attempt for member-only additions on
an existing non-widget class is reasonable to try first, checking the log for a clean
`Reload/Re-instancing Complete` line with no exception before trusting it, same evidence bar used
here.

## PIE world time can run far slower than wall-clock time while the Editor is unfocused — check this before concluding a timer is broken

**Decision:** Before concluding a server-time-based timer (cooldown, telegraph, scene duration —
anything under CLAUDE.md §4.4/§4.5) isn't firing during solo agentic `unreal-mcp` testing, check
`Editor > General > Performance > bThrottleCPUWhenNotForeground` (`Editor`/`General`/
`EditorPerformanceSettings` via `ConfigSettingsToolset`). If `true` (the engine's own default), the
Editor throttles *all* world ticking — PIE included — whenever the Editor application itself isn't
the OS-focused window, which is the normal state for an agentic session driving PIE purely through
MCP tool calls from a separate terminal/chat window.

**Why:** during M12 (2026-08-25), a shortened 16-second test timer failed to fire after 180+ real
seconds, and zero monsters had spawned despite a 6-second spawn interval — looked exactly like a
broken timer. Root cause was this setting, not the game code: with it `true`, in-game server time
advanced at a small fraction of wall-clock time for the whole session (a supposedly-10-second timer
in an earlier test in the same session took ~260 real seconds to fire). Flipping it off via
`ConfigSettingsToolset.SetSectionProperties` made timers advance at a normal, roughly 1:1-with-wall-
time rate immediately.

**How to apply:** for any future solo agentic PIE session doing timing-sensitive verification,
check/flip this setting first rather than waiting out increasingly long real-time delays and
suspecting the game code. It's a personal Editor preference, not a project file — restore it to
however you found it when done, don't leave it silently changed. (If a future session's PIE
timers are inexplicably slow even with this off, checking `bAllowSlateThrottling` in the same
section is the next thing to try — not yet confirmed necessary, but the same category of setting.)

**Still true / unchanged:** this is a testing/tooling note, not an architecture change — CLAUDE.md
§4.4/§4.5's server-time discipline is unaffected; the underlying timers were correct the whole time
in the M12 case that surfaced this.

## `save_assets`/`AssetTools.save_assets` can silently fail to reach disk — always confirm with `is_dirty`, not just the return value

**Decision:** After any `unreal-mcp` asset save (`AssetTools.save_assets`, or the equivalent on any
toolset), confirm the save actually landed by calling `AssetTools.is_dirty` on the same asset path
afterward, and/or checking the `.uasset`'s on-disk mtime. Do not trust `save_assets([])`'s ("save
all dirty assets") return value alone — it can report `true` while leaving assets genuinely dirty
and unwritten to disk.

**Why:** during a 2026-08-25 session, every `.uasset` change made across a multi-hour span
(`DA_GameConstants` tuning, `WBP_AbilityCard`/`WBP_PrepArenaHUD` widget edits) appeared to save
successfully (`save_assets([])` consistently returned `true`) but never actually reached disk —
`is_dirty` on the same assets kept reading `true` afterward, and file mtimes hadn't moved. Calling
`save_assets` with an **explicit path** (not the empty-list "save all" form) surfaced the real
error: the Output Log showed `LogFileManager: Error: Error moving file '...DA_GameConstants.uasset'`
with repeated `Error Code 32` (Windows `ERROR_SHARING_VIOLATION`) retries, then a failed save. An
independent PowerShell exclusive-open test on the same file confirmed it was genuinely locked by
something outside the editor's own save operation.
**Root cause:** three `UnrealEditor.exe` processes were running simultaneously — the real editor,
plus what looked like an orphaned duplicate instance of this same project (identical window title,
spawned ~2.5 minutes later, far lower CPU/memory) and a generic "Unreal Engine 5.8" window — almost
certainly a stale process left behind by an earlier `StartPIE`/`StopPIE` cycle in the same session
that didn't fully tear down. Killing the stray processes (done by the user after being flagged,
not done unilaterally — killing an unfamiliar Editor-titled process without confirming it's truly
orphaned risks losing real unsaved work in a window that looks the same) released the lock; saves
succeeded immediately afterward with matching disk-mtime changes.

**How to apply:** if a `unreal-mcp` session runs multiple `StartPIE`/`StopPIE` cycles, periodically
sanity-check for stray `UnrealEditor.exe`/`CrashReportClientEditor.exe` processes (e.g. via
PowerShell `Get-Process -Name UnrealEditor,CrashReportClientEditor`), especially before trusting
that a batch of asset edits actually persisted. Never kill an Editor-titled process without
confirming with the user first — a duplicate title alone isn't proof it's safe to close, only a
strong signal worth flagging. This project also lives inside a OneDrive-synced folder, which is a
plausible *contributing* factor to transient file locks in general (OneDrive briefly locks files
mid-sync) even though the specific incident here traced to a stale process, not OneDrive itself —
worth ruling out either way if this recurs.

**Still true / unchanged:** every fix made during the session this happened in was independently
re-verified correct in the live editor (properties read back, screenshots, accessibility-tree text
dumps) before the save issue was even discovered — the underlying widget/data fixes were never in
doubt, only whether they had reached disk. No gameplay logic or architecture is affected.

## No `unreal-mcp` tool can set a UMG Designer "Bind Function" property binding — human-in-the-loop is required

**Decision:** When a widget's `Text`/`ColorAndOpacity`/`Visibility` (or any other property with a
Designer "Bind" button) needs to be bound to a `BlueprintPure` function, there is no `unreal-mcp`
tool path to do it. Plan for a short human-in-the-loop step (a few clicks per binding in the UMG
Designer) rather than assuming a reflection-based `set_properties` call will work.

**Why:** confirmed during the 2026-08-26 status badge session (`STATUS_BADGE_PROGRESS.md`). The
Designer's "Bind" mechanism writes to `UWidgetBlueprint::Bindings` (a `TArray<FDelegateEditorBinding>`
that lives on the **Blueprint editor asset itself**, not on the generated class or its CDO). Every
`unreal-mcp` `ObjectTools` call against a Blueprint asset's bare refPath (`get_class`,
`list_properties`, even after routing through `AssetTools.load_asset` first) auto-resolves to the
generated class's **CDO** — confirmed by `get_class` returning `..._C` every time — so there is no
reachable object whose reflected properties include `Bindings`. Separately confirmed
`UMGToolSet.BindToEventProperty` is unrelated: it wires a Blueprint graph node to a *multicast
delegate event* (`OnClicked` etc.), not a Designer-style property binding, and requires a matching
delegate `UPROPERTY` that a plain `BlueprintPure` getter doesn't have.

**How to apply:** when a plan calls for binding a widget property to a C++ `BlueprintPure` function,
budget a short pause for a human to open the widget in the Designer and click "Bind" → select the
function for each property (three clicks took under a minute in the case that surfaced this). Don't
spend further tool-search effort hunting for a reflection path — this one's confirmed absent, not
just undiscovered. Verify the binding afterward through `SlateInspectorToolset.Snapshot`/`Screenshot`
in live PIE (the rendered text/color is proof the binding fired), not by trying to read the Blueprint
asset's `Bindings` array — that path doesn't exist either.

**Still true / unchanged:** `unreal-mcp`'s other UMG tooling (widget creation, tree manipulation,
CDO property wiring) is unaffected — this is narrowly about the Designer's per-property function
binding mechanism.

## A status/ability duration widened on `GameConstants` mid-PIE only affects *casts made after* the edit — the timer captures the value, not a live reference

**Decision:** When temporarily widening a duration on `DA_GameConstants` to survive a slow
verification round-trip (the established M7/M8/etc. pattern), the widened value only takes effect
for an ability *cast after* the edit. A cast made before the edit keeps running on whatever duration
was already captured into its timer — widening the constant does not retroactively extend an
in-flight timer. Re-cast after widening, and keep the widen → cast → verify sequence tight (avoid
large exploratory tool calls — big `list_properties` dumps, screenshot capture/decode — between the
cast and the check), or the newly-widened duration can still run out from real wall-clock time spent
mid-verification.

**Why:** confirmed during the 2026-08-26 status badge session. `ACoopCharacter::ApplyStatusTag`
(`CoopCharacter.cpp`) calls
`GetWorldTimerManager().SetTimer(Handle, Delegate, DurationSeconds, false)` — `DurationSeconds` is a
plain `float` parameter captured **by value** at the moment the ability resolves, not a live pointer
back into `GameConstants`. Repeatedly widening `ShieldDurationSeconds`/`FortressDurationSeconds`
(60s, then 600s) on the live `DA_GameConstants` asset and then still catching `ActiveStatusTags`
empty on a later read was mis-read at first as a possible replication or binding bug; reading the
source directly showed the widened value simply hadn't been in effect yet at the moment of the
already-completed cast, and/or several large tool calls in between had genuinely burned through even
the widened window.

**How to apply:** for any future ability-duration verification, widen the constant *immediately*
before the cast that needs the longer window, not once at the start of a long exploratory session —
and keep the gap between that cast and the property read as short as possible.

**Still true / unchanged:** this is a testing-methodology note, not a bug — CLAUDE.md §4.5's
server-time discipline and the ability code itself are both correct as written; only the *live-tuning
during PIE* technique has this one sharp edge.

## Hold the Gate: pressure plates require standing on them, not merely being nearby/airborne above them

**Decision:** `ACoopPressurePlate`'s occupancy trigger (`TriggerVolume`) is a thin world-space band
starting at the plate's own origin and extending `DA_GameConstants::PlateTriggerCatchHeightUnits`
(default 30 units) upward — not the ~200-unit-tall symmetric column it used to be. Occupancy now
requires a character's capsule to actually reach down to the plate's surface.

**Why:** the original `TriggerVolume` box (`FVector(125, 125, 100)`, centered on the plate's own
origin) extended 100 world units above and below the plate — tall enough that a standing-nearby or
airborne character overlapped it well before their feet reached the plate, and stayed "occupying" it
through most of a jump's arc. Reported 2026-09-02: this read as activating by proximity, not by
physically stepping on the plate.

**A second, deeper bug fixed at the same time:** `TriggerVolume` is parented to `Mesh` (the actor's
root, scaled `(2.5, 2.5, 0.2)` in the constructor to make a flat plate read as a plate, not a cube).
`UBoxComponent::SetBoxExtent`/`SetRelativeLocation` take *local*, pre-scale units that get multiplied
by that inherited scale — the original `125`/`125` XY values were written as if they were already
world-space, so the real overlap footprint was ~2.5x the plate's visible size (625 world units wide
against a 250-unit-wide plate) even before the height bug. `ACoopPressurePlate::ApplyTriggerVolumeSize`
now divides desired world-space numbers by `GetActorScale3D()` to keep the box's real-world dimensions
correct regardless of the plate's scale.

**Level content fixed in the same pass:** `Lvl_ThirdPerson` had two entire separate 4-plate arenas —
one live (near `BP_Gate`/the 4 `BP_MonsterSpawner` actors, world ~X:750-1050, Y:750-1050) and one
orphaned (no gate or spawners nearby, inconsistent Z heights 100/125/275, likely abandoned early
placement work). `ACoopHoldTheGateScene::BeginPlay` iterates every `ACoopPressurePlate` in the level
via `TActorIterator`, so with 8 plates present the "all plates held simultaneously" gate condition was
mathematically impossible for 5 players. The orphaned 4 were deleted (2026-09-02, user-confirmed). The
live 4 also had a per-instance scale override of `(1,1,1)` instead of the class's intended
`(2.5, 2.5, 0.2)` (rendering as full 1m cubes) and sat at world Z=100 while the room's `Floor` actor's
top surface is at world Z=0 (floating 75 units above ground) — both corrected so each plate's mesh
bottom is flush with the floor.

**How to apply:** if a future session adds more `ACoopPressurePlate` instances (this scene or a new
one reusing the class), place them with the class's default scale — don't override it — and verify
`get_actor_bounds`' min Z matches the local floor's top surface. If `ACoopHoldTheGateScene` ever logs
its "found N, expected `PlateCount`" warning again, search the whole level for stray
`ACoopPressurePlate` instances before assuming the code is wrong.

**Still true / unchanged:** `PlateCount` stays 4 (`DA_GameConstants`); the gate's "all plates held
simultaneously" logic (`ACoopHoldTheGateScene::AreAllPlatesOccupied`) is unchanged — only what counts
as "occupying" one plate changed.

## Characters spawn/reset at distinct PlayerStarts in a line, not all stacked on one point

**Decision:** `Lvl_ThirdPerson` now has 5 `PlayerStart` actors laid out in a straight horizontal line
(X = -300/-150/0/150/300, Y=0, Z≈302, 150 units apart) instead of one shared `PlayerStart`.
`ACoopGameMode::FillEmptySlotsWithDummies` and `ACoopHoldTheGateScene::ResetScene` both now gather
every `APlayerStart` in the level (sorted by X) and assign each character/dummy `Starts[Index %
Starts.Num()]` instead of reusing a single point for everyone.

**Why:** reported 2026-09-02 — with every character spawning/teleporting to the exact same point,
capsule interpenetration was supposed to resolve itself (the prior `ResetScene` comment called this
"a deliberate ugly-is-correct simplification... purely cosmetic"), but in practice one character (the
2nd color, blue) could end up boxed in by the other four with nowhere to separate to, unable to move.
Real players spawning via the engine's own `RestartPlayer`/`ChoosePlayerStart` flow were never
affected by this (that path already avoids occupied `PlayerStart`s when more than one exists, and
needed no code change) — only the two places that bypassed it by calling `FindPlayerStart(nullptr)`
once and reusing that single transform for every character.

**How to apply:** if a future session adds a real prep-arena spawn ring (§6.3) or moves this line
closer to the Hold the Gate room, keep the "gather + sort by X + modulo index" pattern rather than
reverting to a single shared spawn point — it's intentionally not a generalized spawn-point system
(§4.6), just the same small fix duplicated at both call sites.

**Still true / unchanged:** dummies/real players are still capped at `GameConstants->MaxPlayers`; this
only changed *where* each one lands, not roster/role logic.

## Camera follows the player

**Decision:** `ACoopOrbitCamera`'s pivot now tracks whichever pawn its owning controller currently
possesses (read fresh every `Tick`, not cached), instead of staying fixed on a world-space
`ArenaCenterLocation`. `UGameConstants::ArenaCenterLocation` was deleted (its only consumer was this
camera's old fixed-pivot `Initialize` logic). Orbit angle (right-click-drag) is unchanged — still
purely local input state, still clamped pitch, still a WoW-style high 3/4 angle, not third-person.

**Why:** requested 2026-09-02 ("the camera needs to move with the player"). This directly reverses
CLAUDE.md §5's original "the camera still never follows a player" rule — flagged as a conflict before
implementing, per this file's own "stop and say so" instruction. User's explicit choice, presented
against the alternative of just fixing the pivot's location (which was separately, genuinely stale —
`ArenaCenterLocation` still pointed at `(0,0,302)`, the ThirdPerson template's original single spawn
point, never updated after the Hold the Gate room was built out near `(900,900)` — see this file's
"Characters spawn/reset at distinct PlayerStarts in a line" entry from the same session). That
staleness was likely *part of* why the fixed camera felt disconnected from gameplay, but the user's
request was for follow behavior specifically, not just a pivot fix, so this goes further than that
alone would have.

**How to apply:** don't reintroduce a fixed arena-center pivot without re-reading this entry and
CLAUDE.md §5 first. If a future session wants a shared/spectator view (e.g. all five players seeing
the same framing for a cinematic beat), that's a different camera mode layered on top of this, not a
reversion — `ACoopOrbitCamera` is still local-only and per-player per §5.

**Still true / unchanged:** the camera is still local-only, never replicated, never affects what any
other player sees (§5); still a high 3/4 clamped-pitch angle, not third-person/over-the-shoulder; the
`bAutoManageActiveCameraTarget = false` setting in `ACoopPlayerController`'s constructor is unchanged
(still needed so a dev-mode `Possess()` swap doesn't rip the view target away from our own camera
actor to the engine's default).

## Movement direction follows the camera's actual yaw, not `ControlRotation`; WoW-style backpedal added

**Decision:** `BP_PlayerCharacter`'s `Move` function (Blueprint, inherited from the ThirdPerson
template) now computes its forward/right basis from `Camera|GetCameraRotation` (via
`Game|GetPlayerCameraManager`) instead of `Pawn|GetControlRotation`. It also scales the backward
(negative Y Axis) input by `ACoopCharacter::GetBackpedalSpeedMultiplier()` (reads
`DA_GameConstants::BackpedalSpeedMultiplier`, default 0.5) via a `SelectFloat` node, so backpedaling
is slower than walking forward/strafing, same as WoW.

**Why:** reported 2026-09-02 — holding right-click to orbit the camera and then pressing W could run
the character *toward* the camera. Root cause: the ThirdPerson template's stock `Move` function was
never rewritten for this project's custom `ACoopOrbitCamera` (CLAUDE.md §5) — it still computed
movement direction from `GetControlRotation()`, but our camera's right-click-drag orbit
(`ACoopOrbitCamera::OrbitYaw`/`OrbitPitch`) is purely local actor state that never feeds back into the
`PlayerController`'s `ControlRotation` (by design — `bUsePawnControlRotation = false` on the camera's
SpringArm/Camera components). `ControlRotation` therefore stayed frozen at whatever it was on spawn,
so "forward" was a fixed world-space direction with no relationship to where the camera had been
dragged. `PlayerCameraManager::GetCameraRotation()` returns the actual current view rotation
regardless of `ControlRotation`, since it tracks whatever the active view target (our orbit camera) is
actually doing — the correct source for a camera-relative movement scheme when the camera is a
separate, self-driven actor rather than something driven by `AddControllerYawInput`.

**How to apply:** if a future session touches `BP_PlayerCharacter`'s `Move`/`Aim` functions, keep
using `GetCameraRotation()` for movement direction, not `GetControlRotation()` — the latter will
silently drift out of sync with the visible camera angle again the moment anything changes the
camera's yaw without also calling `AddControllerYawInput`. `Aim` (touch-only thumbstick look) still
uses `AddControllerYawInput`/`AddControllerPitchInput` and was left alone.

**Still true / unchanged:** `CharacterMovementComponent`'s default movement prediction (CLAUDE.md
§4.2) — this only changed which direction counts as "forward," not how movement replicates.

**First follow-up same session (superseded by the third follow-up below — kept for the reasoning
trail):** fixing movement direction exposed a second issue — `CharMoveComp` still had
`bOrientRotationToMovement = true` (the ThirdPerson template default), which auto-rotates the capsule
to face whatever direction it's currently moving. Once backward movement was actually correct, this
meant pressing S spun the character 180° to face away from the camera and "walked forward" from that
flipped orientation — reported as "facing the wrong way and just walking in front." First fix attempt:
set `bOrientRotationToMovement = false` unconditionally and always call
`SetActorRotation(MakeRotator(0, 0, cameraYaw))` in `Move`, locking facing to the camera at all times.
This worked for S, but see the next entry for why it didn't stay this way.

**Second follow-up same session (reverted — kept for the reasoning trail):** with facing always locked
to the camera, pure-left strafe (`A`) started visually reading as a backward run while pure-right
(`D`) looked correct. Verified with a temporary on-screen debug print (added to `ABP_Unarmed`'s
`EventBlueprintUpdateAnimation`, later removed) that `Direction` was exactly `-90°` while holding `A`
— the precise, correct sample point for `MF_Unarmed_Walk_Left`/`MF_Unarmed_Jog_Left`, not anywhere
near the `±135°`/`180°` backward samples. So the direction math and blend space sampling were correct;
the dedicated `_Left` clips themselves just don't read as a clean mirror of `_Right`. First attempted
fix: point the blend space's two `x=-90` samples at `MF_Unarmed_Walk_Right`/`MF_Unarmed_Jog_Right`
with `bMirror=true` instead of the separately-authored `_Left` clips. **This didn't work and was
reverted** — confirmed no `UMirrorDataTable` asset exists anywhere in the project, and
`FAnimNode_BlendSpacePlayer` (verified via its full property list) has no `MirrorDataTable` pin at
all, so a sample's `bMirror` flag has nothing to mirror against in this animgraph and is a silent
no-op. The actual runtime effect was just playing the unmirrored `_Right` clip while translating left
— still visually wrong (reads as moonwalking), matching the user's "still looks backward" report.
Building working mirroring would require creating a `UMirrorDataTable` asset (Content Browser only —
no `unreal-mcp` tool creates this asset class) plus an `FAnimNode_Mirror` wrapping the
`BlendSpacePlayer` in the "Walk / Run" state. Not pursued — see the third follow-up instead.

**Third follow-up same session — the actual fix, replacing both attempts above:** the real insight was
realizing *why* `A` used to look fine before any of this session's changes: `ABP_Unarmed`'s own
`EventBlueprintUpdateAnimation` (stock template logic, not ours) clamps `Direction` to `[-45, 45]`
**only while `bOrientRotationToMovement` is true** — and while it's true, the capsule also
auto-rotates to face its own movement direction. Together those two effects meant `Direction` was
never anything but ~0° for *any* input before this session, regardless of which key was pressed — the
character just turned to face wherever it was going and played the forward animation. The broken
`_Left` samples were always there, just never selected. Restored
`bOrientRotationToMovement = true` (reverting the first follow-up) and reverted the blend space's
`x=-90` samples back to their original `_Left` clips (reverting the second follow-up), then made the
camera-facing lock in `Move` **conditional on `Y Axis < 0`** (reuses the same `< Y Axis 0.0` comparison
node already computed for the backpedal-speed `SelectFloat`, via a `Branch` wired so both the True path
— `SetActorRotation` then continue — and the False path — skip straight to the movement calls —
converge on the same `AddMovementInput` calls). Net result: pressing S (or S+A/S+D) locks facing to the
camera and backpedals correctly; W, pure A, pure D, and W+A/W+D are back to exactly their original,
user-confirmed-good behavior (`CharacterMovementComponent` auto-turns the capsule to face movement,
`Direction` stays clamped near 0°, the broken `_Left` samples are never exercised again).

**How to apply:** don't re-enable an unconditional camera-facing lock in `Move` without re-reading this
whole chain of entries — it will re-expose the `_Left` blend space samples' authoring problem. If a
future session wants strafing (A/D moving sideways while facing stays camera-locked, closer to true
WoW mouselook behavior) instead of the current auto-turn-on-A/D behavior, that specifically requires
first building the `UMirrorDataTable` + `FAnimNode_Mirror` setup described in the reverted second
follow-up above — it is not optional plumbing, `bMirror` alone does nothing in this animgraph.

## "The Q ability" per role, cast animations, and the `ApplyTestVulnerable` dev stub

**Decision (2026-09-02):** Each of the 5 roles now has one keyboard-activated ability, all on the
**Q** key:

| Role | Q ability | Resolves in | Reads | Writes |
|---|---|---|---|---|
| TANK | Shield | `CoopTankAbilities::ApplyShield` | — | `Status.Shielded` |
| SUPPORT | Speed | `CoopSupportAbilities::ApplySpeed` | — | `Status.SpeedBuff` (nearest other `ACoopCharacter`) |
| RUNNER | Dash | `CoopRunnerAbilities::ResolveDash` | `Status.SpeedBuff` (checks, doesn't consume) | — (`LaunchCharacter` impulse; boosted if buff held) |
| CONTROL | Stabilize | `CoopControlAbilities::ResolveStabilize` | `Status.Shielded` | `Status.Fortress` |
| DAMAGE | Execution | `CoopDamageAbilities::ResolveExecution` | `Status.Vulnerable.Physical` (consumes) | — (deals `ExecutionDamageAmount` to the **click-selected** `ACoopMonsterCharacter` if it holds the tag & is in range) |

- **Execution was retrofitted 2026-09-03 to take a click-selected target** instead of its original
  "nearest `ACoopMonsterCharacter` holding the tag" auto-search — `Server_ActivateExecution(AActor*
  Target)` now, gated client-side on `GetCurrentTargetActor()`. Speed and Stabilize were deliberately
  **not** retrofitted (they keep their auto-search). See "Target-required abilities need a
  click-selected target" below for the full reasoning.
- **"Q ability" = each role's first-listed ability in `docs/abilities.md`.** That doc has no keybind
  notation anywhere; this mapping was inferred from listing order and confirmed with the user.
- **Tank's Shield moved from E to Q** (explicit user choice). `E` is now mapped to nothing in
  `IMC_Default`. The `IA_Shield`→`ActivateShield` wiring is bound to the *action*, not the key, so
  only `IMC_Default`'s key mapping changed.
- **`IMC_Default` deliberately maps all 5 actions (`IA_Shield`/`IA_Speed`/`IA_Dash`/`IA_Stabilize`/
  `IA_Execution`) to the same `Q` key.** Safe and intentional: every `Activate*` Server RPC is
  role-gated on `ACoopPlayerController` and no-ops for the wrong role, so one physical key means a
  different thing per role with no conflict. This is the standard Enhanced Input pattern for
  "same key, different meaning per class."

**Cast animations — the `PlayCastMontage` / `DefaultSlot` mechanism (reuse this for any future
ability animation):**
- `ACoopCharacter::PlayCastMontage(UAnimMontage*)` is **server-only**; it calls a
  `NetMulticast, Reliable` `Multicast_PlayCastMontage` that runs `Montage_Play` on every client's
  copy (including the listen-host). This is server-initiated cosmetic feedback, NOT client
  prediction — matches CLAUDE.md §4.2. It no-ops if the montage is null.
- Every ability plays its montage on **any cast that clears the cooldown gate**, even if the deeper
  targeting/whiff check then fails — consistent "I pressed my button and my character did something"
  feedback. Applied uniformly to all 5 (Shield/Stabilize retrofitted).
- Montages play through `ABP_Unarmed`'s already-wired `DefaultSlot` `AnimGraphNode_Slot` — **no
  AnimBP changes needed**, the slot was already there.
- The 5 montages live in `/Game/Characters/Mannequins/Anims/Unarmed/AbilityMontages/`
  (`MM_Shield_Montage` … `MM_Execution_Montage`), each a single-segment wrapper around a stock
  Mannequin gesture clip (`MM_Attack_01/02/03`, `MM_ChargedAttack`, `MM_Dash`). No new animation
  content was authored. They're wired onto `BP_PlayerCharacter`'s CDO
  (`shieldCastMontage`/`speedCastMontage`/`dashCastMontage`/`stabilizeCastMontage`/`executionCastMontage`).

**`ApplyTestVulnerable` is a dev-only Exec command, not a real ability.** `docs/abilities.md` says
`Status.Vulnerable.Physical` is written by "The Heart" (Scene 5, Build 2 — doesn't exist yet).
Without a tag-writer, Damage's Execution would be permanently uncastable and untestable. So
`ACoopPlayerController::ApplyTestVulnerable` / `Server_ApplyTestVulnerable` (no role gate) grants the
nearest `ACoopMonsterCharacter` within `TestVulnerableRangeUnits` the tag for
`TestVulnerableDurationSeconds` — same "ApplyTestDamage" precedent already in the controller. **Delete
this when Scene 5 lands** and a real mechanic writes the tag.

- **Addendum (2026-09-03): a twin `ApplyTestVulnerableMagic` / `Server_ApplyTestVulnerableMagic` Exec
  pair was added** for the same reason, granting `Status.Vulnerable.Magic` so Damage's new **Overload**
  ability is testable before "The Heart" exists. **Delete BOTH stubs together when Scene 5 lands.**

**Monsters got minimal tag support for this.** `ACoopMonsterCharacter` gained
`HasStatusTag`/`ApplyStatusTag`/`RemoveStatusTag` (a trimmed copy of `ACoopCharacter`'s mechanism —
a replicated `FGameplayTagContainer` + expiry timers — NOT a shared base class, per CLAUDE.md §4.6),
because Execution needs a taggable enemy target type and monsters are the only enemy actor that
exists.

**Still true / unchanged:** no generic ability system (CLAUDE.md §4.6) — each of the 5 abilities is
its own explicit namespace function + role-gated RPC pair, mirroring the existing
`ApplyShield`/`ResolveStabilize` shape exactly. Movement for Dash still rides standard
`CharacterMovementComponent` prediction (§4.2); only the cooldown gate / impulse-strength decision is
server-authoritative.

## `unreal-mcp` gotchas found building the Q abilities (2026-09-02)

Narrow tooling notes, not architecture. Logged so a future session doesn't rediscover them:

- **`ObjectTools` property names are camelCase**, not the C++ PascalCase. `get_properties` /
  `set_properties` on `ShieldCastMontage` silently fails ("could not be read"); `shieldCastMontage`
  works. `list_properties` first is the reliable way to get exact names.
- **`ObjectTools.set_properties` on an array rejects a simultaneous element-change + size-change**
  ("ArrayAdd: elements changed alongside the size change; insertion points are ambiguous"). Split
  into two writes: one that only modifies existing elements in place (size unchanged), then one that
  only appends (existing elements unchanged). Used for the `IMC_Default` Shield-rebind + 3-new-mapping
  edit.
- **CDO writes must target the CDO object, not the generated class.** `set_properties` on
  `/Game/.../BP_Foo.BP_Foo_C` silently rejects; use
  `BlueprintTools.get_default_object` → `/Game/.../BP_Foo.Default__BP_Foo_C`.
- **An `AnimMontage`'s `sequenceLength` is not settable and goes stale** if you only edit
  `slotAnimTracks` via reflection (stays at whatever the duplicated-from montage had). Force a
  recompute with `EditorAppToolset.OpenEditorForAsset` on the montage, then re-read to confirm.
- **`EditorToolset.LogsToolset.GetLogEntries` needs `category: ""`** to search all categories — the
  param defaults to the literal string `"LogsToolset"` which then errors as an unknown category.
- Writing instanced-subobject refs back through `set_properties` (the `IMC_Default` input modifiers,
  `.../IMC_Default:InputModifierSwizzleAxis_3`) logs `LogUObjectGlobals: Warning: Failed to find
  object 'Class ...'` during the write — harmless fallback-resolution noise; the read-back confirmed
  every modifier stayed correctly attached.

## RoleSelect screen feedback is `NativeTick`-driven, not Designer bindings

**Decision (2026-09-02):** `WBP_RoleSelect`'s on-screen feedback — which roles are taken, which one
is yours, the countdown header — is driven entirely from `UCoopRoleSelectWidget::NativeTick` writing
to `BindWidgetOptional` C++ pointers every frame. It is **not** built from UMG Designer "Bind
Function" property bindings. Do not re-do it as Designer bindings.

**Why:** clicking a role button already worked end to end
(`OnClicked`→`ClaimX`→`Server_ClaimRole`→`ACoopGameMode::TryClaimRole`→`SetRole`, first-claim-wins,
replicated) but produced **zero visible change** — `WBP_RoleSelect` was 5 role labels + 5 empty
buttons. `unreal-mcp` has no tool to create a Designer "Bind Function" binding (see the entry above /
BUILD_1 M5 log), so `NativeTick` against `BindWidgetOptional` members is the one feedback mechanism
this project's tooling can actually build *and* verify. Every pointer is null-checked, so a
renamed/missing child widget degrades gracefully instead of hard-failing the `BindWidget` contract.

**The naming contract** (`NativeTick`/`UpdateRoleSlot` match these by exact name):
- Buttons: `TankButton` `SupportButton` `RunnerButton` `ControlButton` `DamageButton`
- Caption `TextBlock` inside each button: `TankButtonLabel` … `DamageButtonLabel`
- Header `TextBlock`: `RoleSelectHeaderText`

Renaming a button in the WBP auto-fixes-up its `OnClicked`→`ClaimX` graph handler (`RenameWidget`
does this), but the C++ name-match is silent if it breaks — rename via the same names or lose the
feedback.

**Behaviour rules baked in:**
- **No click-to-release.** Your own claimed role's button shows `YOURS` and is *disabled*. To switch,
  click a different still-available role — `TryClaimRole` frees your previous pick server-side
  automatically. There is deliberately no "unclaim" button.
- **Tint / caption per state** (mid-tone backgrounds so the always-white caption reads on every
  state — "ugly is correct", no per-state text-colour juggling): available → blue / `CLAIM`;
  yours → green / `YOURS`; taken by another → red / `TAKEN` + disabled. Header text:
  `PICK YOUR ROLE  -  <N> SEC LEFT`, `<N>` recomputed from `GetServerWorldTimeSeconds()` each tick
  (never a locally-ticked countdown, CLAUDE.md §4.5).
- The panel's *root visibility* (show only during the `RoleSelect` phase) is a separate gate that
  predates this feature and is unchanged — verified still working (panel is gone the instant the
  phase resolves, doesn't linger over the prep arena).
- **Mouse cursor + input mode.** ~~The match otherwise runs in `FInputModeGameOnly` with a hidden
  cursor... `NativeTick` flips the **local** player into `FInputModeGameAndUI` + visible cursor...
  and back... when it resolves.~~ **STALE as of 2026-09-03 — see "Cursor + click-to-target, target
  frame, party frames" below.** This per-phase toggle block was **deleted** from
  `UCoopRoleSelectWidget::NativeTick`: the cursor-targeting feature makes the cursor + `GameAndUI`
  the whole-match default (owned by `ACoopPlayerController::BeginPlay`), so RoleSelect's buttons
  stay clickable with no local toggle. The `if (!bRoleSelectActive) return;` feedback bail is all
  that remains of this block. Original rationale kept for history: added 2026-09-02 after the first
  playtest ("I cant see my mouse to click a role"), used `bShowMouseCursor` as its own
  already-switched sentinel.

**Verified (2026-09-02, 5-client PIE):** claim on a client sets that client's server
`CoopPlayerState.PlayerRole` and flips its button to `YOURS`/disabled; every other client's matching
button shows `TAKEN`/disabled within a tick; switching frees the old role on every client; header
counts down from server time; RoleSelect still resolves to Prep on timeout with the two non-claiming
players auto-assigned the leftover roles. Exact tint *colours* on screen not separately eyeballed
(`SlateInspectorToolset` reads caption text + enabled state, not `SetBackgroundColor`) — same
cosmetic-feel carve-out as every other widget milestone.

## WoW-style action bar (bottom-screen ability bar)

**Decision (2026-09-03):** the game now has a persistent bottom-centre ability bar
(`WBP_ActionBar` / `UCoopActionBarWidget`, containing 3 `WBP_AbilitySlot` / `UCoopAbilitySlotWidget`
tiles), created once in `ACoopPlayerController::BeginPlay` alongside the other HUD widgets. Locked
design, agreed in chat and not to be re-litigated:

- **Each tile is a solid-colour `Border` with the ability name centred on it** — a coloured *letter*
  tile, no texture, no art. One distinct colour per ability; greyed tiles use flat dark grey
  `(0.16,0.16,0.18)`. This **overrides `CoopAbilityCardWidget.cpp`'s "icon art is out of scope" note**
  and partly fulfils CLAUDE.md §6.3's long-standing "ability cards with icon, name, explanation"
  (M5 shipped the cards without icons). It is CLAUDE.md §5's "everything readable, nothing pretty"
  applied literally.
- **Slots shown = the role's full specced kit** from `docs/abilities.md` (2–3 tiles), not just the
  one working ability. Slot 0 is the implemented "Q ability" (Shield / Speed / Dash / Stabilize /
  Execution): full colour, a hardcoded `"Q"` badge, a live cooldown sweep. Slots 1–2 are the
  specced-but-unbuilt kit — greyed, no badge, no cooldown. A 2-ability role shows exactly 2 tiles
  (the third `Collapse`s), never an empty third.
- **Cooldown = WoW-style radial sweep + integer seconds.** A dark wedge covers the tile and unwinds
  **clockwise from 12 o'clock** as the cooldown expires, with the whole-seconds-remaining number on
  top. Driven by one **UI-domain material `M_CooldownSweep`** (`MD_UI` / Translucent / Unlit, single
  `Progress` 0–1 scalar) pushed every frame via a `UMaterialInstanceDynamic`. **This is the one
  element that brushes against CLAUDE.md §5's "no VFX / no post-processing" — logged here as a
  deliberate judgement call: it is *UI*, not scene VFX, the same category as any other UMG widget.**
- **Visible only during the Prep and HoldTheGate phases**, invisible otherwise. The prep-arena text
  ability cards (`WBP_AbilityCard`) are **untouched** and remain the "what does it do" surface during
  Prep.
- **Slots are not mouse-click targets** — the whole bar is `HitTestInvisible` so the cursor always
  falls through to the right-click camera drag (§5). Pressing **Q** stays the only way to cast; the
  tile just reflects the resulting cooldown. The keybind label is a hardcoded `"Q"` (every ability
  maps to Q by design — see the "Q ability per role" entry).
- **The 5 `ACoopCharacter::…CooldownEndServerTime` floats are now replicated** —
  `DOREPLIFETIME_CONDITION(…, COND_OwnerOnly)`, plus `VisibleInstanceOnly` so they are
  reflection-readable for debugging (CLAUDE.md §4.3). Previously server-only. Still written
  server-side only, via the `Set…()` setters the ability namespaces call; each client receives only
  its own pawn's cooldowns.
- **All widget feedback is `NativeTick`-driven against `BindWidgetOptional` pointers — no UMG
  Designer "Bind Function" bindings** anywhere in this feature, same reason as the RoleSelect
  follow-on (`unreal-mcp` can't author Designer bindings).

**Gotcha found in P9 — a widget cannot hide *itself* via `SetVisibility` in its own `NativeTick`.**
Slate stops calling `Tick` on a widget the frame its own visibility leaves the "visible" family, and
**both `Collapsed` *and* `Hidden` stop it** (only `Visible` / `HitTestInvisible` /
`SelfHitTestInvisible` keep ticking). A widget that hides itself this way on its first tick — which
`WBP_ActionBar` did, because it is created during the RoleSelect phase — freezes and never comes
back. The project's other phase-gated HUD widgets dodge this with Designer Visibility bindings
(re-evaluated even while collapsed); this feature can't author those. **The fix used here:** the
container stays `HitTestInvisible` forever and toggles its own `RenderOpacity` between 0 and 1
(RenderOpacity 0 is fully invisible but keeps the widget ticking, and it stays non-interactive
either way). A child slot does the same for the transient `Unassigned`-role window, and keeps a
plain `Collapsed` only for the *permanent* "this slot index is past this role's kit" case.

**Second gotcha found in P9 — a widget instance placed inside another Widget Blueprint snapshots the
placed class's CDO at placement time.** The 3 `WBP_AbilitySlot` instances inside `WBP_ActionBar`
were placed (P7) *before* the `WBP_AbilitySlot` CDO's `GameConstants` was wired (P8), so they kept
`GameConstants = None` and the cooldown-duration lookup silently returned 0 (no sweep drawn). Same
class of gotcha as the per-instance `SlotIndex` values that P7.3 *did* set explicitly. **How to
apply:** when a placed sub-widget needs a value from its class CDO, set it **on each placed
instance** (or wire the CDO before placing the instances), don't rely on later CDO edits
propagating.

**Verified (2026-09-03, 5-client PIE):** bar hidden through the whole RoleSelect phase
(`RenderOpacity` logged at 0 every tick); appears in Prep with the correct per-role kit — Control 3
tiles (Stabilize coloured + "Q" badge, Mind Fracture / Channel greyed), Support 2 tiles, Runner 3
tiles, colours + badge + greying + count all correct; `IA_*` cast sets
`…CooldownEndServerTime = casttime + duration` exactly; `COND_OwnerOnly` exact (Runner's Dash
cooldown reached the server world + the Runner's own client and `-1` on the other three clients);
the radial sweep + seconds number animate (screenshotted `55` → `26`, dark wedge shrinking clockwise
from 12); bar persists into HoldTheGate still `HitTestInvisible`. Exact wedge *shape* is the usual
human-eyeball cosmetic carve-out.

**Still owed:** a full external `Build.bat` rebuild (editor closed, as in this feature's own P4) to
bake the P9 C++ deltas — the `RenderOpacity` visibility fix in both widgets and the
`VisibleInstanceOnly` on the 5 cooldown fields — permanently; they are live in the current editor
through Live Coding patches only.

## Team Synergies panel removed from the prep arena (deviation from CLAUDE.md §6.3)

**Decision (2026-09-03):** the "Team Synergies" hint panel is **removed entirely**, at the user's
explicit direction. Deleted: the `WBP_TeamSynergiesPanel` instance inside `WBP_PrepArenaHUD`, the
`WBP_TeamSynergiesPanel` widget asset itself, and its C++ base `UCoopSynergyHintWidget`
(`CoopSynergyHintWidget.h/.cpp`). The prep-arena HUD now shows only the countdown text and the four
`WBP_AbilityCard` tiles.

**This is a knowing deviation from CLAUDE.md §6.3**, which mandates "a Team Synergies panel showing
*that* a relationship exists with another role — but not the full solution" as part of the
"first teamwork test is communication itself" beat. It was flagged to the user as a §6.3 conflict
before the change was made; the user chose "remove the panel entirely" over blanking the text or
rewording it. The hint string it displayed was `"Tank and Control share a bond. Talk to each
other."` (the sole synergy hint in Build 1).

**Why:** user call — the panel wasn't wanted on screen. No rationale beyond that was given; treat
§6.3's synergy-panel requirement as set aside for now, not permanently deleted from the design. If a
synergy hint is wanted back, `docs/abilities.md` still has the synergy list and this entry records
exactly what was torn out.

**How to apply:**
- Do **not** re-add a synergy panel to `WBP_PrepArenaHUD` or recreate `UCoopSynergyHintWidget`
  without explicit user go-ahead — this was a deliberate removal, not an oversight.
- `UCoopSynergyHintWidget` is deleted from source but still lives in the currently-loaded editor
  DLL; a future full rebuild drops it. Harmless until then (nothing references it —
  `CoopAbilityCardWidget.h` only mentions it in a comment).
- `docs/abilities.md`'s synergy definitions are unchanged and remain the source of truth for the
  five relationships themselves.

## RoleSelect: CLAIM button moved below the role name

**Decision (2026-09-03):** in `WBP_RoleSelect`, each role column now stacks the role-name
`TextBlock` above its CLAIM `Button` in the column's `VerticalBox` (button 6px below the name,
left-aligned), instead of the previous name-and-button side-by-side `HorizontalBox`. The per-column
`HorizontalBox`es were deleted and each column's `CanvasPanelSlot` set to auto-size. Purely a layout
change — the five `OnClicked → ClaimTank/Control/Runner/Support/Damage` bindings and the
`NativeTick`/`UpdateRoleSlot` name contract (see "RoleSelect screen feedback is `NativeTick`-driven"
above) are unchanged and were verified intact in 5-client PIE.

## Cursor + click-to-target, target frame, party frames (WoW-style)

**Decision (2026-09-03):** the game now has mouse-cursor unit selection and two always-on
`UUserWidget` frames, built per `cursor_progress.md` (that file is the plan and the build log;
this entry is the durable summary). User-requested mini-feature outside strict build order — same
category as monster combat / the "Q ability" / the WoW-style action bar / the status badge (logged
deviations, not scope creep). `docs/abilities.md` untouched — no new tag, no new ability.

**What it is:**
- **A visible OS cursor for the whole match.** `ACoopPlayerController::BeginPlay` (inside its
  `IsLocalController()` block) sets `SetShowMouseCursor(true)` + `FInputModeGameAndUI`
  (`DoNotLock`, `SetHideCursorDuringCapture(false)`). This **replaces** `UCoopRoleSelectWidget`'s
  old per-phase cursor toggle — that block is deleted (see the strike-through in "RoleSelect screen
  feedback is `NativeTick`-driven"). The `DoNotLock` + `SetHideCursorDuringCapture(false)` combo is
  what lets `ACoopOrbitCamera`'s right-click-drag orbit (raw mouse delta in its `Tick`) keep
  working with the cursor shown — RoleSelect already proved that combo.
- **The cursor hides while right-click is held** (WoW-style: it vanishes while you turn the camera,
  reappears where it was the instant you let go). `ACoopOrbitCamera::Tick` already reads the RMB
  state every frame for the orbit, so it edge-detects RMB there and calls
  `OwningController->SetShowMouseCursor(false/true)` on the two edges only. Cursor visibility is
  really a controller concern, but co-locating it with the identical RMB check the orbit already
  does beat adding a `Tick` to the controller for one toggle — and it stays the *only* other writer
  of `bShowMouseCursor` besides `BeginPlay`'s one-time whole-match "on" (added 2026-09-03 from the
  first playtest). **Second playtest (2026-09-03) showed hiding alone is not enough:** with
  `GameAndUI` + `DoNotLock` the *hidden* OS cursor still tracks the physical mouse during the drag,
  so it reappeared wherever the drag ended, not where it started. Fixed by snapshotting the cursor's
  viewport position (`GetMousePosition`) on the press edge and warping it back (`SetMouseLocation`)
  on the release edge. The cursor is still not *locked* during the drag, so a very large sweep can
  push it to the viewport edge and briefly stall the orbit — left as a noted limitation (a per-tick
  re-centre) rather than fixed pre-emptively.
- **Click a unit to target it.** LMB → `IA_Select` (new Input Action, Pressed-only trigger) →
  mapped in `IMC_Default` → `BP_PlayerCharacter`'s EventGraph (`IA_Select` event → `GetController`
  → `CastToCoopPlayerController` → `SelectTargetUnderCursor`, same BP-wiring shape as every
  ability). `ACoopPlayerController::SelectTargetUnderCursor()` does
  `GetHitResultUnderCursor(ECC_Visibility, ...)`, accepts only `ACoopCharacter` /
  `ACoopMonsterCharacter`, else clears. Empty click / `Esc` → `ClearTarget()` (WoW keeps the
  target; clearing is the simpler prototype rule).
- **A targetable actor must BLOCK the `ECC_Visibility` trace channel** — that's what
  `GetHitResultUnderCursor(ECC_Visibility)` collides against. Monsters already did
  (`ACoopMonsterCharacter`'s static `Mesh` uses the `BlockAllDynamic` profile). **Teammates did
  not**: `ACharacter`'s capsule is the `Pawn` profile (`Visibility → Ignore`) and
  `BP_PlayerCharacter` stores that as an explicit `Custom` override on `CollisionCylinder`, and
  `CharacterMesh0` is `QueryOnly` ignoring every trace channel — so a click on an ally passed
  straight through to the floor and cleared the target (bug found 2026-09-03, first thing the user
  hit: *"I should be able to click on and target allies ... same as enemies"*). **Fix:**
  `ACoopCharacter::BeginPlay()` calls
  `GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block)`. Placed in
  `BeginPlay`, **not** the constructor — `BP_PlayerCharacter`'s serialized component data re-applies
  its `Visibility → Ignore` override on top of any constructor setting, so a runtime call is the
  last word; `BeginPlay` also covers the dev-mode dummy pawns (they spawn from `DefaultPawnClass` =
  `BP_PlayerCharacter`). The **capsule** is the click volume, WoW-style — the skeletal mesh stays
  non-colliding. Camera is unaffected: `ACoopOrbitCamera`'s spring arm probes `ECC_Camera`, which
  the capsule still ignores. Verified 2026-09-03 in PIE: all 5 live characters' capsules block
  Visibility after `BeginPlay` (the `Ignore` override cleared → reverts to default Block); a
  straight-down `trace_world` at a character now stops at capsule-top (origin + 90), not the floor.
- **`CurrentTargetActor` is `TWeakObjectPtr<AActor>` — NOT replicated, no `DOREPLIFETIME`, no RPC.**
  Selection is "what this local player is looking at", CLAUDE.md §4.2's local/cosmetic/client-only
  category — each of the five players targets independently, nobody's target touches anyone else's
  screen (same guarantee as `ACoopOrbitCamera`). A destroyed target silently weak-null-resolves.
- **`GetCurrentTargetActor()` is the clean seam for target-driven abilities** (a *future* phase,
  not built): a later phase routes it into the `Server_Activate*` RPCs as *intent*, server
  re-validates. v1 changes no ability code — the five abilities keep their own implicit
  nearest-X targeting.
- **One reusable widget class, `UCoopUnitFrameWidget`** (`Core/CoopUnitFrameWidget.h/.cpp`), drives
  **both** the single top-left target frame **and** each of the 5 party-stack rows. A per-instance
  `EUnitFrameSource { CurrentTarget, PartyMember }` + `int32 PartyMemberIndex` decide which actor
  it reads each `NativeTick`. `CurrentTarget` → `GetOwningPlayer<ACoopPlayerController>()
  ->GetCurrentTargetActor()`; `PartyMember` → `GameState->PlayerArray[PartyMemberIndex]`'s pawn.
  `static HealthOf` / `ActorHasTag` handle `ACoopCharacter` **and** `ACoopMonsterCharacter` with an
  explicit `Cast` each — no shared base (§4.6).
- **All feedback is `NativeTick` → null-checked `BindWidgetOptional` pointers** (`RootBorder`,
  `NameText`, `TypeText`, `HealthBar` `UProgressBar`, `HealthText`, `StatusText`). **No Designer
  "Bind Function" bindings** anywhere (`unreal-mcp` can't author them — see the entry above). Same
  approach as `UCoopRoleSelectWidget` / `UCoopActionBarWidget`.
- **Show/hide is a `RenderOpacity` 0/1 toggle, never the widget's own `Visibility`.** The frames
  are created during the RoleSelect phase with nothing to show; a widget that leaves the "visible"
  family in its own `NativeTick` freezes forever (the P9 action-bar gotcha). They stay
  `HitTestInvisible` always. `RenderOpacity` 0 when: phase ∉ {Prep, HoldTheGate}, OR no subject
  actor, OR the subject has no health component.
- **The top-left target frame is never interactive; party-stack rows are left-clickable.** The
  target-frame `UCoopUnitFrameWidget` (`Source == CurrentTarget`) stays `HitTestInvisible` — the
  cursor falls straight through it. **Party rows** (`Source == PartyMember`) got click-to-target
  (2026-09-03, from the first playtest — *"when clicking on the party frames, the selected team
  member needs to be shown in the top left"*), which **revises the original "frames are never
  interactive / rows are display-only" call:**
  - `UCoopUnitFrameWidget::NativeOnMouseButtonDown` — **left**-click on a party row with a live
    subject → `ACoopPlayerController::SetCurrentTarget(that teammate's pawn)` → `FReply::Handled`.
    **Right-click and every other case return `Unhandled`**, so the right-click-drag orbit camera
    still works with the mouse anywhere over the party frame — decision #7's *intent* (frames never
    steal the camera drag) is preserved, only its *mechanism* changed.
  - `NativeTick` sets a party row `Visible` (hit-testable) only while it has a subject, else
    `HitTestInvisible` — an empty slot never eats a click. `Visible`↔`HitTestInvisible` are both in
    Slate's visible family, so this never trips the P9 self-hide freeze; show/hide stays
    `RenderOpacity`.
  - `WBP_PartyFrame`'s `RootCanvas` is `SelfHitTestInvisible` (was `HitTestInvisible`) so clicks
    reach the rows but the empty canvas area still falls through; `PartyStack` VBox is already
    `SelfHitTestInvisible`.
  - `ACoopPlayerController::SetCurrentTarget(AActor*)` — a direct setter (no cursor trace, the row
    already knows the actor), same local-only / no-RPC / not-replicated contract and the same
    `ACoopCharacter`/`ACoopMonsterCharacter` accept-filter as `SelectTargetUnderCursor`; a
    null/rejected arg is a no-op (does **not** clear).
- **Party stack tints the local player's own row** (warm yellow `RootBorder` brush) so a player
  can pick themselves out. Enemy targets / other rows stay neutral dark.
- **Aesthetic: coloured bars + text, no art.** `UProgressBar` HP bar tinted green (ally) / red
  (enemy), an HP number, name text, role text ("TANK".."DAMAGE" or "ENEMY"), one status-text line
  (concatenated short tag names — Downed / Fortress / Shielded / Speed / Vulnerable). "Everything
  readable, nothing pretty" (§5).
- **The ground ring** — `ACoopTargetRing` (`Core/CoopTargetRing.h/.cpp`), a local-only `AActor`
  (`bReplicates = false`), spawned per-client in `BeginPlay` behind `IsLocalController()` exactly
  like `ACoopOrbitCamera`. One engine `Plane` mesh + `M_TargetRing` (a **`MD_Surface`**, unlit,
  translucent material — `OneMinus(Saturate(Abs(Distance(UV,(.5,.5)) - .42)/.07))` → soft ring
  band → Opacity; one `Color` vector param → EmissiveColor). `Tick` snaps it to the current
  target's feet, tints the MID green (`ACoopCharacter`) / red (else), hides it when there's no
  target. This is CLAUDE.md §5's "a coloured ring on the ground ... is a spell effect" — the
  sanctioned stand-in for a WoW selection **outline**, which §5 forbids (it needs post-processing).
  Class is content-wired via `BP_TargetRing`'s CDO (`RingMaterial` → `M_TargetRing`); if
  `ACoopPlayerController::TargetRingClass` is ever left unset the ring simply never spawns.

**Tunables:** `TargetRingRadiusUnits` (90) + `TargetRingGroundOffsetUnits` (88) in
`GameConstants.h` (`Category = "Targeting"`). All colours stay hardcoded-cosmetic (the
`GetColorForPlayerId` / `GetStatusColor` precedent — §10 is about gameplay numbers).

**Rebuild discipline:** the two new UCLASSes (`UCoopUnitFrameWidget` = new `UUserWidget` subclass,
`ACoopTargetRing` = new `AActor`) were added with a **full external `Build.bat` from a closed
editor** (Live Coding is confirmed unsafe for a brand-new `UUserWidget` subclass — see the Live
Coding entry). All the rest is content, wired through `unreal-mcp`. Two follow-up rounds of C++
then landed from playtest feedback: (1) the `ACoopCharacter::BeginPlay` capsule-collision fix;
(2) the cursor-hide + party-row-click changes (`ACoopOrbitCamera` Tick + member,
`ACoopPlayerController::SetCurrentTarget`, `UCoopUnitFrameWidget::NativeOnMouseButtonDown` override
+ `NativeTick`) — the widget gets a new virtual **override** (reuses the base's existing vtable
slot, no layout change, but it's a `UUserWidget` subclass so treated as rebuild-only to be safe).
**Both (1) and (2) were baked by one closed-editor `Build.bat` on 2026-09-03** (Succeeded, exit 0,
DLL 799,744 B, UHT 0 generated files — pure C++, no reflection change; no warnings/errors). The
`WBP_PartyFrame` `SelfHitTestInvisible` change was already saved content.
Then (3) a **third** playtest round (2026-09-03): the cursor-hide fix above didn't restore the
cursor's *position* on release — `ACoopOrbitCamera` gains 3 plain members (`bHasSavedCursorPos`,
`SavedCursorX/Y`) + edge-handler logic in `Tick` (`GetMousePosition` on press, `SetMouseLocation`
on release). **Baked by a closed-editor `Build.bat` on 2026-09-03** (Succeeded, exit 0, DLL
799,744 B @ 14:25, UHT 0 generated files, no warnings/errors). **Nothing is owed a rebuild now.**

**Verified (2026-09-03, solo agentic 5-client PIE — the second pass was against the *baked* DLL after
the two follow-up rounds):** all assets compile/save/load; the game runs with the new widgets with
**zero runtime errors / `Accessed None` / script warnings** across a full RoleSelect → Prep →
HoldTheGate session; the **party frame** shows 5 rows (name / role / HP bar / "N / 100") with
consistent replicated values across 3 clients and the local row **yellow-tinted**; HP **drops live**
and `StatusText` shows **"DOWNED"** as monsters down players (frames read replicated state, not a
snapshot); the **target frame stays invisible** (`RenderOpacity` 0) with no target.
Post-bake additions: **left-click a party row → the target frame populates** with that teammate
(name / role / green HP bar / "100 / 100"), verified on two clients via `SlateInspector`;
**client isolation** — one client's target left another client's target frame untouched;
**the ground ring** (`BP_TargetRing_C_0`, checked via `ObjectTools`) `bHidden` `true`→`false` on
target set, snaps to the target pawn's location minus `TargetRingGroundOffsetUnits` (88), carries the
`TargetRingRadiusUnits`-derived scale (90/50 = 1.8), and its `M_TargetRing` MID `Color` param goes
`AllyRingColor` green for an `ACoopCharacter` target.

**Still needs a human playtest (pure visual, or a positioned world-click the MCP tools can't
simulate):** the cursor is visible from match start and **hides while right-click is held** (reappears
on release); a **world-click** on a teammate/monster (`SelectTargetUnderCursor` + the capsule
`ECC_Visibility` fix — the fix itself was trace-verified) populates the frame (ally green / "ENEMY"
red) and click-empty-ground clears; **targeting a monster** shows red bar + red ring (the proven
tick's `else` branch); right-click-drag camera still works with the mouse over the party frame
(rows return `Unhandled` for RMB); RoleSelect buttons still clickable after the cursor-ownership move.

## Target-required abilities need a click-selected target

**Decision (2026-09-03):** three abilities are now **target-required** — they only fire when the local
player has a click-selected target (`ACoopPlayerController::CurrentTargetActor`, from the cursor /
party-row selection feature above). Pressing one with no target selected shows a centre-screen
**"Please choose a target"** toast and **sends no RPC**. This is the first consumer of the
`GetCurrentTargetActor()` seam that the "Cursor + click-to-target" entry parked as a future phase
(*"a later phase routes it into the `Server_Activate*` RPCs as intent, server re-validates"*).

**The three, and why only these three:**
- **Execution** (Damage, `Q`) — **retrofitted** from its old "nearest `ACoopMonsterCharacter` holding
  `Status.Vulnerable.Physical`" auto-search.
- **Armor Break** (Tank, **`E`**, new) — applies `Status.Broken` to the targeted monster.
- **Overload** (Damage, **`E`**, new) — the magic twin of Execution, keyed to
  `Status.Vulnerable.Magic`.

All three target an **enemy** (`ACoopMonsterCharacter`), so the server-side "valid target" check is
uniformly `Cast<ACoopMonsterCharacter>` + range + required-tag.

**Speed and Stabilize were deliberately NOT retrofitted** — explicit user call (2026-09-03: *"retrofit
only Execution, leave Speed and Stabilize auto for now"*). They keep their "nearest ally / nearest
Shielded Tank in range" auto-search. This keeps **Hold the Gate / the Fortress synergy flow entirely
untouched** — that scene is verified working and Stabilize's auto-target is load-bearing for it.
Revisit only if a later scene needs ally/Tank click-targeting.

**The gate is client-first, server re-validates (CLAUDE.md §4.1):** the thin
`BlueprintCallable` wrapper on `ACoopPlayerController` (`ActivateExecution` / `ActivateArmorBreak` /
`ActivateOverload`, called from `BP_PlayerCharacter`'s EventGraph) reads `GetCurrentTargetActor()`.
Null → `ShowToast(...)`, **`return` before the RPC**. Non-null → `Server_Activate*(Target)`, and the
server casts + re-checks type / range / required tag — it never trusts the client's actor. RPC
signatures changed: `Server_ActivateExecution()` → `Server_ActivateExecution(AActor* Target)`, plus
new `Server_ActivateArmorBreak(AActor*)` / `Server_ActivateOverload(AActor*)`. `AActor*` replicates
fine in an RPC (both target types are replicated actors the server already has).

**`Status.Broken` has no reader yet — accepted.** Armor Break applies it (real/fake target is
irrelevant — Armor Break reveals nothing itself). Nothing consumes it until Control's Mind Fracture +
the False King clones (Build 2); until then it just shows on the target frame's status line
(`BROKEN`) and expires. Same "built before its consumer" situation as Stabilize was.

**`Status.Vulnerable.Magic` is dev-granted until The Heart** — via the new `ApplyTestVulnerableMagic`
Exec command (twin of `ApplyTestVulnerable`). Both stubs are deleted together when Scene 5 lands.

**The toast — `UCoopToastWidget` (`Core/CoopToastWidget.h/.cpp` + `WBP_Toast`):** a reusable
centre-anchored transient message. Purely local cosmetic UI (§4.2) — its `NativeTick` only *reads*
two **plain (non-`UPROPERTY`) C++ members** on the owning controller (`PendingToastText` /
`PendingToastStartTime`, set by `ShowToast`) and fades itself. Fade is `elapsed = GetWorld()->
GetTimeSeconds() - StartTime` (one stored stamp, **not** a `DeltaTime` accumulation — §4.4), full
opacity for the first 60% of `ToastDurationSeconds` then linear to 0. **Never self-hides via
`Visibility`** (the P9 action-bar freeze — a widget that leaves the "visible" family in its own tick
freezes forever); it stays `HitTestInvisible` and only toggles `RenderOpacity`. `MessageText` is a
`BindWidgetOptional` written from `NativeTick` — **no Designer "Bind Function" bindings** (`unreal-mcp`
can't author them). `NativeTick` does **not** clear `MessageText` after the fade, so the text stays in
the Slate tree at opacity 0 — a `SlateInspector.WaitFor` text-gone check can't see the fade.

**Deferred, not built (need scenes that don't exist):** Link, Channel, Mind Fracture stay
specced-not-built until The Dying Room / The False King. Carry, Chain, Taunt are out — Carry/Chain
target *objects* (the click-select system only accepts `ACoopCharacter`/`ACoopMonsterCharacter`, and
there are no carryable props), Taunt is undesigned.

**`CoopDamageAbilities` — two explicit functions, no `TFunctionRef` helper.** `ResolveExecution` and
`ResolveOverload` are fully spelled-out twins (each: authority/world guard → cooldown gate →
`Set…CooldownEndServerTime` + `PlayCastMontage` on any gate-cleared cast → `Cast<ACoopMonsterCharacter>`
→ range check → required-tag check → `RemoveStatusTag` + `ApplyDamage`), mirroring `ApplyShield` /
`ResolveArmorBreak`'s shape. The plan sketched a shared `TFunctionRef` helper; §4.6 / §2 favour the
explicit copy.

**Rebuild discipline:** new `UCoopToastWidget` UCLASS + 2 new `UPROPERTY` cooldown fields on the
already-loaded `ACoopCharacter` (`ArmorBreakCooldownEndServerTime` / `OverloadCooldownEndServerTime`,
`Replicated` + `VisibleInstanceOnly` + `COND_OwnerOnly`, identical shape to the existing 5) + a
changed `Server_ActivateExecution` signature + 4 new RPCs → **Live Coding unsafe** (see the Live
Coding entry). All C++ landed in one batch, then one full external `Build.bat` from a closed editor
(**Succeeded, exit 0, DLL 830,464 B @ 2026-09-03 16:00, no warnings/errors**). Content (2 `IA_*`
assets, `IMC_Default` `E` mappings, 7 new `DA_GameConstants` fields, `WBP_Toast`, CDO wiring, 2
`BP_PlayerCharacter` EventGraph chains, cast montages) all via `unreal-mcp` afterward.

**New keys — `E` for both new abilities, role-gated.** `IMC_Default` maps `E` → `IA_ArmorBreak`
*and* `E` → `IA_Overload`; every `Server_Activate*` is role-gated and no-ops for the wrong role, so
one physical key means Armor Break for a Tank and Overload for Damage with no conflict — the same
pattern that puts all 5 first abilities on `Q`.

**Verified (2026-09-03, agentic 5-window PIE smoke pass):** full RoleSelect → Prep → HoldTheGate loop
runs with **zero script errors**; `IA_Execution` pressed with no target → the "Please choose a target"
toast renders (`SlateInspector.WaitFor` false→true); `IA_ArmorBreak` + `IA_Overload` also fire clean.
**Still owed a hands-on pass:** targeted casts on a real monster (needs an LMB world-click the MCP
tools can't position), the toast's opacity fade (visual), `E`-on-Tank-vs-Damage disambiguation,
cooldown `COND_OwnerOnly` replication across the 5 client worlds, and the 2 new action-bar tiles
(Tank slot 1 Armor Break / Damage slot 1 Overload, `E` badge + cooldown sweep). Speed/Stabilize +
the Fortress flow to be re-confirmed unchanged.

## Ability-bar UX: cooldown toast + hover tooltips

**Decision (2026-09-03):** two small ability-UX additions, built per `ABILITY_UX_PROGRESS.md` (that
file is the plan + build log; this entry is the durable summary). Both are **local, pre-RPC UI
feedback** (CLAUDE.md §4.2) — no gameplay prediction, no replication, no new tunables (colours /
offsets stay hardcoded-cosmetic, the `GetColorForPlayerId` precedent).

**1 — "Ability not ready" toast on cooldown.** All 7 `Activate*()` wrappers on
`ACoopPlayerController` (Shield / Speed / Dash / Stabilize / Execution / Armor Break / Overload) now
gate client-side on the owning pawn's replicated `*CooldownEndServerTime` before sending their
`Server_Activate*` RPC. Not ready → centre-screen `UCoopToastWidget` **"Ability not ready"** (new
`NSLOCTEXT("CoopAbilities", "AbilityNotReady", ...)`), **no RPC**. New private helper
`ACoopPlayerController::IsAbilityReady(float CooldownEndServerTime) const` — plain method (not a
`UFUNCTION`), `GetWorld()->GetGameState()->GetServerWorldTimeSeconds() >= end`. A wrong-role player's
cooldown field for an ability they don't have is always `-1`, so the toast is naturally suppressed
for them on the 4 auto-target wrappers. **Every `Server_Activate*_Implementation` keeps its own
authoritative cooldown re-check** in the `Resolve*`/`Apply*` namespace call — unchanged; this gate
only saves a wasted RPC and gives feedback.

**Client-side ROLE gate added to the 3 target-required wrappers** (Execution / Armor Break /
Overload): `GetPlayerState<ACoopPlayerState>()->GetRole() != <thisAbilityRole>` → **silent `return`**
(no toast, no RPC). This fixes a pre-existing wart: `IA_Execution` / `IA_Overload` share the `Q` / `E`
keys with the other roles' first abilities, so a Tank/Runner/etc. pressing `Q` for *their* ability
used to flash a spurious "Please choose a target" from the Execution chain. Order in those 3
wrappers: **role gate → cooldown gate → target gate → RPC** (a not-ready or wrong-role ability is a
more fundamental blocker than a missing target). The server-side role gate in each
`_Implementation` is unchanged and stays the real guard.

**2 — Hover tooltips above the action bar.** Hovering a `WBP_AbilitySlot` tile shows a LoL/WoW-style
panel **above** the bar with the ability **name + one-sentence description** — all slots, greyed
("coming later") ones included; a slot index past the local role's kit shows nothing.

- **Pure geometry poll, NOT Slate hit-testing.** The bar and every slot stay `HitTestInvisible` (the
  WoW-action-bar decision's core guarantee — cursor falls through to the right-click camera drag /
  click-to-target). Each `UCoopAbilitySlotWidget::NativeTick` sets
  `bCursorOver = MyGeometry.IsUnderLocation(UWidgetLayoutLibrary::GetMousePositionOnPlatform())`
  (UMG, already a dep — **not** `FSlateApplication::Get().GetCursorPos()`, which needs the unlisted
  `Slate` module; **no `Build.cs` change**). `UCoopActionBarWidget::NativeTick` reads its 3 slots
  (`Slot0/1/2`, new `BindWidgetOptional`) and drives one shared panel. **Zero input-routing change
  anywhere.**
- **Tooltip content is duplicated into `CoopAbilitySlotWidget.cpp`'s kit table** — a new
  `FText Description` on `FCoopAbilitySlotInfo`, ~11 strings lifted verbatim from
  `CoopAbilityCardWidget.cpp`. Continues that file's existing deliberate name-duplication (CLAUDE.md
  §4.8 / §1 "hardcoded is correct") rather than refactoring a verified widget.
- **The panel lives in `WBP_ActionBar`** as `TooltipRoot` (a `Border`, dark `(0.05,0.05,0.07,0.92)`,
  `HitTestInvisible`) → `TooltipBox` (`VerticalBox`) → `TooltipNameText` (bold 16) +
  `TooltipDescText` (regular 12, wrapped at 320), anchored bottom-centre in `RootCanvas`, offset
  `top -100`, `bAutoSize`. **Show/hide via `RenderOpacity` only** (never its own `Visibility` — the
  P9 self-hide-freeze). All `BindWidgetOptional` + `NativeTick`, no Designer bindings.

**Rebuild discipline:** Feature 1 is function bodies + one plain helper + one `NSLOCTEXT` (Live-Coding
safe alone). Feature 2 adds **6 new `UPROPERTY(meta=(BindWidgetOptional))` members** to the
already-loaded `UUserWidget`-derived `UCoopActionBarWidget` → **Live Coding unsafe** (see the Live
Coding entry — new reflected members on a widget class fall on the cautious side). One full external
`Build.bat` from a closed editor covered both (**Succeeded, exit 0, DLL 840,704 B @ 2026-09-03
18:15, no warnings/errors**). `WBP_ActionBar`'s `TooltipRoot`/`TooltipBox`/`TooltipNameText`/
`TooltipDescText` wired via `unreal-mcp` afterward.

**`UCoopToastWidget::NativeTick` still does not clear `MessageText` after the fade** (noted in the
target-required entry) — once any toast has shown, its text stays in the Slate tree at opacity 0, so
`SlateInspector.WaitFor("<toast text>")` returning true only means that text was *ever* shown, not
that it is currently visible. Verify a cooldown toast by pairing it with a cooldown-field read (a
widened cooldown + a byte-identical `*CooldownEndServerTime` across the 2nd press = no re-cast), not
by text presence alone.

**Verified (2026-09-03, agentic 5-window PIE):**
- Full RoleSelect → Prep → HoldTheGate loop, **zero `Error` / `Accessed None` / script warnings** for
  any new code.
- **Cooldown toast** (Control / Stabilize, `StabilizeCooldownSeconds` widened to 120 for the round
  trip): cast sets `stabilizeCooldownEndServerTime`; an immediate 2nd press left it **byte-identical**
  (`480.20114135742188` before and after) and put "Ability not ready" in the Slate tree → the client
  gate fired, no RPC. The other 6 wrappers are the same 3-line pattern with a different getter,
  reviewed on disk, not each PIE-tested.
- **Client role gate:** Control / Runner / Support / Tank pressing `IA_Execution` with no target →
  **no** "Please choose a target" (silent return); the Damage client → the toast **does** show
  (unchanged). Confirmed across all 5 client worlds.
- **Hover tooltip:** hovering the Tank's Shield tile → panel above the bar reads "Shield" + its
  description; moving to the Armor Break tile → swaps to "Armor Break" + its description; hovering
  off the bar → both texts clear. The geometry poll works under `SlateInspector.Hover` and the bar
  never became hit-testable.

**Not verified via MCP:** right-click-drag camera orbit / click-to-target *while the cursor is over
the bar* (P4.5 — needs a real drag). Low risk: zero input-routing change, and the hover test proves
the bar stays `HitTestInvisible` (the tooltip is a geometry poll, not a Slate hover event). Worth a
one-minute human eyeball.

**Observed, not caused by this feature (now addressed — see follow-up below):** in the first PIE run
the prep-arena ability-card HUD (`WBP_PrepArenaHUD`) stayed on screen after HoldTheGate started.
Nothing in the cooldown-toast / hover-tooltip work touched that widget's lifecycle.

### Follow-up (2026-09-03): prep-arena ability cards deleted

Once the hover tooltips (above) covered the same "what does this ability do" surface, the prep-arena
`WBP_AbilityCard` ×4 (icon + name + one-sentence description, shown during Prep and *lingering* into
HoldTheGate) were redundant. User directive: *"Remove the other tooltip that is showing when you
select a class, leave only the hovers"* — and, asked how far to go, *"Strip + delete the assets."*

- **Deviation from CLAUDE.md §6.3** (which mandates "3–4 ability cards with icon, name, one-sentence
  explanation" in the prep arena) — flagged to the user before acting, same class of deliberate,
  logged deviation as the earlier "Team Synergies panel removed" entry. `docs/abilities.md` + the
  action-bar slot-kit table in `CoopAbilitySlotWidget.cpp` are now the sole homes of the ability
  name/description strings.
- **Done:** the 4 `WBP_AbilityCard` instances removed from `WBP_PrepArenaHUD` (compiled + saved —
  tree is now `CanvasPanel_40` → the countdown `TextBlock_60` + an empty leftover `HorizontalBox_4`
  from the old Team Synergies slot, left in place, minimal-touch). `WBP_AbilityCard.uasset` deleted.
  `Source/Unreal_first_Game/Core/CoopAbilityCardWidget.h`/`.cpp` `git rm`'d — no code referenced
  `UCoopAbilityCardWidget`, only comments (3 stale "like the card widget" analogy comments in
  `CoopAbilitySlotWidget.h`/`.cpp` + `CoopStatusBarWidget.h` cleaned).
- **One closed-editor `Build.bat`** (dropping a UCLASS from the module is not Live-Coding-safe — see
  the "Live Coding must not be used to add a new UCLASS…" entry): Succeeded, exit 0, UHT logged
  "source file removed", `UnrealEditor-Unreal_first_Game.dll` shrank 840,704 → **822,784 B**, zero
  warnings/errors.
- **Verified (2026-09-03, agentic 5-window PIE):** `WBP_PrepArenaHUD` compiles clean against the
  rebuilt module — no missing-class / missing-parent (its parent `UCoopPrepCountdownWidget` is
  untouched); `UCoopAbilityCardWidget` is gone from the loaded module. Full RoleSelect → Prep →
  HoldTheGate loop with **zero script errors / Accessed-None / UMG warnings** (only the pre-existing
  benign `FindTeleportSpot` spawn-overlap + `Not enough login credentials` PIE warnings). Prep-phase
  HUD shows **only the countdown** (live number) alongside the party/target frames and action bar —
  no cards; HoldTheGate shows the same minus a live number. Hover tooltips unaffected — code path
  untouched (only comment edits), tooltip panel widgets still present and bound in the live tree; not
  re-exercised live because the geometry poll reads the OS cursor, which the MCP synthetic hover
  can't move (same harness gap as P4.5's camera drag).
- **Still not done (out of scope, flagged):** `WBP_PrepArenaHUD` has no phase-gate wired, so the
  now-bare countdown panel still lingers into HoldTheGate. `UCoopPrepCountdownWidget::
  GetPrepArenaVisibility()` exists for a Designer "Bind" on the panel root (`unreal-mcp` can't author
  it) or a C++ `NativeTick` self-gate. A bare "0" is far less intrusive than the old cards, so parked.
