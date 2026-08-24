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
