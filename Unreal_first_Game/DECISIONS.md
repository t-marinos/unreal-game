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
