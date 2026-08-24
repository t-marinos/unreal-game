# The Boss — "The Final Teamwork Exam"

Not one of the original five star scenes — added here because the design brief has
substantial boss-specific detail (the escalation table, the "HP is a teamwork meter"
philosophy) that doesn't belong inside any single star scene's file. Follows the same
template as the five scene docs for consistency.

**No star role, no single synergy** — the boss deliberately reuses the vocabulary taught
by all five scenes.

## The fantasy

A raid-boss final exam built from mechanics the team already learned in the dungeon, made
harder purely through timing, rotation and overlap — not new content.

## The room

**Design rule (explicit, from the source brief):** the five star scenes teach only the
first ~5% of each relationship. The boss makes those same five relationships repeat,
combine, rotate and overlap.

**CLAUDE.md §6.5 is explicit: the boss has no AI.** It is a scripted sequencer — a
hardcoded timeline that fires telegraphs and applies tags at fixed offsets. Do not build
behaviour trees, pathfinding, or adaptive difficulty, and do not reach for the project's
`GameplayStateTree` plugin to build one — a hardcoded timeline is the correct shape here
(§4.6's "no generic system" philosophy applied to the boss), matching the project's
general refusal to build a generalized ability/synergy engine.

*(The one logged exception to "no AI" is scoped narrowly to Hold the Gate's monsters —
see `docs/scenes/HOLD_THE_GATE.md` — and does not extend to the boss itself.)*

### Escalation layers

| Layer | What changes |
|---|---|
| LEARN | happens in the five star scenes, not here |
| REPEAT | boss asks for a recognizable version of one synergy so the team recalls the mechanic |
| COMBINE | two learned mechanics happen at the same time |
| ROTATE | targets, links, positions or responsibilities change mid-execution |
| OVERLAP | several mechanics active simultaneously, little breathing room |

**MVP scope note (CLAUDE.md §7 Build 2):** only **REPEAT → COMBINE → ROTATE** are in
scope. **OVERLAP and the final-5% compressed-orchestration phase are explicitly out of
MVP scope** — "if ROTATE isn't fun, OVERLAP won't rescue it." Named here for completeness
of the design record, not as something to build yet.

### Full intended escalation (post-MVP reference, from the source brief)

- 100–90%: obvious/clean versions of all five synergies
- 90–75%: two mechanics combine
- 75–50%: rotations begin
- 50–25%: mechanics overlap *(post-MVP)*
- 25–5%: continuous communication required *(post-MVP)*
- Final 5%: compressed orchestration, one failure breaks the chain *(post-MVP)*

### Boss HP philosophy

**Boss HP is a teamwork progress meter, not a damage race.** Meaningful chunks of HP are
earned by successful coordinated executions (a clean Fortress hold, a correct Mind
Fracture callout, a landed Execution/Overload), not by sustained DPS. This mirrors The
Heart's HP-threshold-per-execution pattern (`docs/scenes/THE_HEART.md`), scaled up to five
synergies instead of one.

## Tags read/written

No new tags — the boss timeline reuses every tag already defined in `docs/abilities.md`
(`Status.SpeedBuff`, `Status.Shielded`/`Status.Fortress`, `Status.Broken`, `Status.Linked`/
`Status.TeamSpirit`, `Status.Vulnerable.Physical`/`Status.Vulnerable.Magic`), applying them
via scripted timeline offsets instead of scene-specific triggers.

## Where authority resolves

- The entire timeline (which telegraph fires when, which tags get applied to which
  targets): server-only, driven by `GetServerWorldTimeSeconds()` offsets, never
  per-frame `DeltaTime` accumulation (§4.4/§4.5).
- ROTATE's target/link/position reassignment: server-only — this is exactly the kind of
  gameplay-critical state §4.2 forbids predicting client-side.
- Boss HP deduction on successful coordinated executions: server-only, using each
  synergy's existing resolution logic (no new damage system — reuses what each scene
  already established).

## Buildable via unreal-mcp vs. requires C++

| MCP-buildable (content wiring) | Requires C++ |
|---|---|
| Boss chamber level layout, boss actor placement/visual | The timeline sequencer class itself (fixed-offset telegraph/tag-application logic) |
| Telegraph decals/VFX-free visual cues for each phase | REPEAT/COMBINE/ROTATE phase logic and the HP-as-teamwork-meter deduction rule |
| Data asset tuning (phase HP thresholds, telegraph timing) in `DA_GameConstants` | Reuse/invocation of each scene's existing synergy-resolution logic from within the timeline |

## Exit / fail criteria

- **Success:** boss HP reaches the MVP's REPEAT→COMBINE→ROTATE completion threshold
  (OVERLAP and beyond are post-MVP, not part of the MVP win condition).
- **Fail (wipe):** all five `DOWNED` simultaneously (§6.6). Instant restart from the
  beginning of the current scene (the boss phase, in this case), unlimited attempts.

## Build reference

Build 2 (CLAUDE.md §7): "Full boss with REPEAT → COMBINE → ROTATE only. Boss HP tied to
successful coordinated executions." A separate mini-boss that repeats and combines the
Fortress mechanic specifically ships earlier, inside Build 1, as part of Hold the Gate's
exit criteria — see `docs/scenes/HOLD_THE_GATE.md` and CLAUDE.md §7 Build 1.
