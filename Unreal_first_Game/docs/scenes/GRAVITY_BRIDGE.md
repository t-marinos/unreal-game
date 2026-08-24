# Scene 1 — The Gravity Bridge

**Star role:** Runner · **Synergy taught:** Thousand Dashes (Support `Speed` + Runner `Dash`)

## The fantasy

"I can get us somewhere nobody else can."

## The room

A long bridge sits inside a gravity field. Anyone inside the field moves at roughly 10% of
normal speed — the field is the room's default state, not a trap that triggers. A lever
actor sits at the far end.

- **Gravity field** — a large trigger volume (`AGravityFieldVolume` or similar) covering
  the bridge. On overlap, applies a movement-speed multiplier to the entering actor; on
  exit, removes it. This is a movement modifier, not a gameplay tag — it doesn't need
  `FGameplayTag` machinery, just a server-authoritative speed multiplier read by
  `CharacterMovementComponent` (still within §4.2's "movement prediction stays on" carve-out,
  since it's a continuous modifier, not a discrete gameplay-critical event).
- **The lever** — an actor with a `Server_Activate()` RPC, following the same pattern as
  Build 0's exit-criteria test object (CLAUDE.md §7). Pulling it does two things
  server-side: disables the gravity field (removing the speed penalty for everyone still
  on the bridge) and triggers a monster spawner.
- **Thousand Dashes** — Support casts `Speed` on Runner before or during the crossing;
  Runner's `Dash` then resolves as Thousand Dashes per its tag check in `docs/abilities.md`,
  letting Runner reach the lever fast enough despite the field.
- **The complication** — pulling the lever releases monsters that had been held beneath
  the bridge by the field. They fixate on the Runner specifically — the same
  fixate/retarget-on-death pattern Hold the Gate already establishes (CLAUDE.md §6.5;
  see `docs/scenes/HOLD_THE_GATE.md`). Reused here, not re-litigated. Tank, Control,
  Support and Damage all have a role in keeping the Runner alive during this window, but
  none of them are the Star.
- **Objective is not a kill count.** The scene ends when the whole party has crossed the
  bridge, not when the monsters are cleared. The bridge itself has a secondary collapse
  timer (server-timestamped, §4.5) that runs independently of the monster fight — a slow
  party can still lose the bridge even with a clean fight.

## Tags read/written

| Tag | Direction | Ability |
|---|---|---|
| `Status.SpeedBuff` | written by Support `Speed`, read by Runner `Dash` | see `docs/abilities.md` |

No other tags are involved — the gravity field's speed penalty is a separate,
non-tag movement modifier (see above).

## Where authority resolves

- Gravity field membership and the resulting speed multiplier: server-only, replicated as
  a movement property.
- Lever activation, field disable, and spawner trigger: `Server_Activate()` RPC, asserts
  `HasAuthority()`.
- Monster fixation/retarget-on-death: server-only (matches Hold the Gate's existing
  precedent — see that scene's file for the CLAUDE.md §6.5 exception this pattern relies on).
- Bridge collapse timer: server time (`GetServerWorldTimeSeconds()`), never client
  `DeltaTime` accumulation.
- `Speed`/`Dash`/Thousand Dashes resolution: server-only per `docs/abilities.md`.

## Buildable via unreal-mcp vs. requires C++

| MCP-buildable (content wiring) | Requires C++ |
|---|---|
| Bridge level layout, lever actor placement, spawner actor placement | Gravity field volume class (speed-multiplier logic) |
| Visual telegraph for the collapsing bridge (decal/material) | Lever's `Server_Activate()` RPC and its two effects |
| Monster actor placement/blueprint wiring for appearance | Monster fixate/retarget-on-death logic (reused from Hold the Gate) |
| Data asset tuning (speed multiplier %, collapse timer duration) in `DA_GameConstants` | Bridge collapse timer and whole-party-crossed win check |

## Exit / fail criteria

- **Success:** all five players reach the far side before the bridge collapses.
- **Fail (wipe):** all five `DOWNED` simultaneously, or the bridge collapse timer expires
  before everyone crosses (§6.6). Instant restart from scene start, unlimited attempts —
  no attempt-limit system, per CLAUDE.md §8.

## Build reference

Build 2 (CLAUDE.md §7) — one of the four remaining scenes after Hold the Gate (Build 1)
and The Dying Room (built second, per the Build order priority note).
