# Scene 3 — The False King

**Star role:** Control · **Synergy taught:** Mind Fracture (Tank `Armor Break` → target `BROKEN` → Control `Mind Fracture`)

## The fantasy

"I see what the others cannot. Listen to me."

## The room

An enemy multiplies into several identical copies; only one is real. Attacking the wrong
copy incurs a penalty (more enemies, damage, lost time).

- **Clone actors** — N identical actors spawned at scene start. Exactly one is flagged,
  server-side only, as the real target. This flag is a hidden actor property, never
  replicated to any client before a reveal fires — if a client could inspect it, the
  scene's entire test collapses.
- **Armor Break** — Tank "tests" a clone by using Armor Break on it. This applies
  `Status.Broken` to whichever clone it's used on, real or fake — Armor Break itself
  reveals nothing (see `docs/abilities.md`).
- **Mind Fracture** — Control casts it on a `Broken` target. Server checks whether that
  target is the real one:
  - **If real:** fires a reveal cue (see below) and opens a short window for the team to
    act on the correct target.
  - **If fake:** no reveal fires — proposed behavior is a minor penalty consistent with
    "attacking the wrong copy incurs a penalty" (extra enemy, small damage tick, or time
    lost), exact effect `TBD`.
- **The value is not the spell alone.** The PDF is explicit that the test is interpretation
  + callout + timing: Control must read the reveal cue and then verbally direct the team
  ("REAL ONE BACK LEFT!" per CLAUDE.md's own target Discord-moment quote) — the ability
  does the sensing, the player does the deciding.

### Proposed resolution for the non-perspective-dependent tell (flagged, not locked)

CLAUDE.md §6.4 is explicit and stricter than the original design here: because the camera
is only high-angle *by default* and each player can freely orbit their own view (§5),
there is no longer a guaranteed shared viewing angle to lean on for "which clone is real."
The tell must work identically regardless of where a given player's camera happens to be
pointed.

**Proposal:** combine two channels, neither of which is angle-dependent:
1. A **HUD/UI marker** (e.g. a screen-space arrow or highlight on the real target),
   replicated to all clients only after the reveal fires — this reads correctly no matter
   which direction a player is looking, because it's UI space, not world space.
2. An **audio cue** (a distinct sound played from the real target's location, or a
   non-positional stinger) as a redundant channel for players who are looking elsewhere
   entirely at the moment of the reveal.

This is a design proposal for review, not a final decision — a ground-plane VFX ring was
considered and rejected here specifically because a ring is still world-space and can be
partly occluded depending on camera angle/geometry, which is the exact failure mode §6.4
warns against.

## Tags read/written

| Tag | Direction | Ability |
|---|---|---|
| `Status.Broken` | written by Tank `Armor Break`, read by Control `Mind Fracture` | see `docs/abilities.md` |

## Where authority resolves

- Which clone is real: server-only hidden state, set at scene start, never replicated
  pre-reveal.
- `Armor Break`'s `Status.Broken` application: server-only, applies uniformly regardless
  of real/fake (this is intentional — it must not leak information).
- `Mind Fracture`'s real/fake check and reveal-firing: server-only.
- The reveal cue's replication to clients: happens only *after* the server-side check
  succeeds, never before.

## Buildable via unreal-mcp vs. requires C++

| MCP-buildable (content wiring) | Requires C++ |
|---|---|
| Room layout, clone actor placement (visually identical meshes/materials) | Server-side "which clone is real" flag and its non-replication guarantee |
| Reveal HUD widget (UMG) and audio cue asset wiring | `Armor Break`'s uniform `Status.Broken` application logic |
| Data asset tuning (clone count, reveal window duration, wrong-target penalty) in `DA_GameConstants` | `Mind Fracture`'s real/fake check and reveal-trigger logic |

## Exit / fail criteria

- **Success:** the team correctly identifies and defeats the real King within the
  attempt's constraints.
- **Fail (wipe):** all five `DOWNED` simultaneously, or repeated wrong-target penalties
  compound into an unrecoverable state (exact fail condition `TBD` alongside the penalty
  design above). Instant restart from scene start, unlimited attempts.

## Build reference

Build 2 (CLAUDE.md §7).
