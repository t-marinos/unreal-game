# Scene 2 — Hold the Gate

**Star role:** Tank · **Synergy taught:** Fortress (Control `Stabilize` + Tank `Shield`)

This is Build 1's scene (CLAUDE.md §7) — the first real scene built, chosen deliberately
because it's the cheapest to build (trigger volumes, spawners, one carried collider) and
has the highest social density: all five players are under pressure simultaneously,
instead of four watching one star.

## The fantasy

"For the next 15 seconds, everyone trusts me."

## The room

A sealed gate opens only while four pressure plates stay occupied. Four of the five
players are pinned in position by this requirement; Tank is the only mobile one.

- **Pressure plates** — four trigger volumes. The gate stays closed unless all four are
  occupied simultaneously (server-checked continuously, not a one-time trigger).
- **Monster chambers** — open around the plates once the scene starts, spawning threats
  that target the pinned plate-holders. Tank's job is not damage output — it's threat
  control: block projectiles, knock enemies away, keep all four plate-holders alive.
- **Shield** — Tank's directional damage-blocking volume (see `docs/abilities.md`).
  Early in the scene, Shield alone is enough to cover one threat direction at a time.
- **The complication** — near the end, the threat volume/frequency exceeds what a single
  facing of Shield can cover. Plain Shield is no longer sufficient.
- **Fortress** — Control casts `Stabilize` on Tank's active `Status.Shielded`, upgrading
  it to `Status.Fortress`: coverage extends to protect multiple teammates at once, and it
  resists knockback. This is the scene's actual teaching moment — protection becomes a
  synchronized two-player action, not a solo tank stat.

## Tags read/written

| Tag | Direction | Ability |
|---|---|---|
| `Status.Shielded` | written by Tank `Shield`, read by Control `Stabilize` | see `docs/abilities.md` |
| `Status.Fortress` | written by Control `Stabilize` (upgrade output) | see `docs/abilities.md` |

## Where authority resolves

- Plate-occupancy check (all four held simultaneously → gate opens): server-only,
  continuously evaluated, not a fire-once trigger.
- Monster spawn/targeting logic (fixate on plate-holders): server-only. This scene is the
  origin of the fixate/retarget-on-death monster pattern that Gravity Bridge later reuses.
  CLAUDE.md §6.5 records this as a deliberate, user-approved, one-off exception to the
  "boss has no AI" rule, scoped to this monster type only — not a general precedent for
  adaptive AI elsewhere. §6.5 points to a `DECISIONS.md` entry ("Monster combat inside Hold
  the Gate") for the full reasoning; that file doesn't exist in the repo yet, so treat this
  note as the current best record until it's created.
- Shield's damage negation and coverage volume: server-only.
- Stabilize's upgrade check (`HasMatchingGameplayTag(Status.Shielded)`) and the resulting
  Fortress coverage/knockback-resist: server-only.

## Buildable via unreal-mcp vs. requires C++

| MCP-buildable (content wiring) | Requires C++ |
|---|---|
| Gate room level layout, plate trigger-volume placement, gate actor placement | Plate-occupancy continuous check and gate-open logic |
| Monster spawner actor placement, chamber-opening visual/blueprint wiring | Monster spawn timing, targeting, and fixate/retarget-on-death logic |
| Shield/Fortress visual representation (decal, mesh, or widget indicator) | Shield's damage-blocking volume and Fortress's multi-teammate coverage/knockback-resist logic |
| Data asset tuning (plate count already fixed at 4, spawn rate, Shield/Fortress coverage radius) in `DA_GameConstants` | `Stabilize`'s upgrade check and the Shield→Fortress tag transition |

## Exit / fail criteria

- **Success:** gate stays open long enough / the scripted threat sequence completes with
  all four plates continuously held, and the party proceeds through the gate.
- **Fail (wipe):** all five `DOWNED` simultaneously, or the gate closes because plate
  occupancy breaks and can't be restored in time (§6.6). Instant restart from scene start,
  unlimited attempts.

## Build reference

Build 1 (CLAUDE.md §7) — built first, before any other scene. Exit criteria for Build 1:
"five friends play it and talk continuously. If they don't, we stop and rethink the design
rather than building four more scenes."
