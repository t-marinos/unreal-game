# Scene 4 — The Dying Room

**Star role:** Support · **Synergy taught:** Team Spirit / Life Network (Support `Link` + Control `Channel`)

Per CLAUDE.md's Build order priority note, this scene should be built **second**,
immediately after Hold the Gate — the Link/chain mechanic is the riskiest and most
technically fiddly thing in the whole design (distance checks, break conditions, chain
ordering, damage routing), so it's worth knowing early whether it's fun or just annoying.

## The fantasy

"I decide who gets what, and when."

## The room

The room drains life continuously while objectives must be completed elsewhere in it.

- **Life-drain volume** — a room-wide, continuous damage-over-time trigger volume (or a
  GameState-driven periodic tick affecting all actors in the room). This is the room's
  default state, always active, not a triggered hazard.
- **Resource pressure** — Support cannot simply spam Heal (Heal is a `TBD` ability per
  `docs/abilities.md` — this scene assumes some form of limited healing resource exists,
  even though its exact ability isn't specced yet). The design intent is that Support must
  allocate attention/resources across multiple needs rather than brute-force healing
  through the drain.
- **Link** — Support targets one teammate, creating a two-actor relationship
  (`Status.Linked`, see `docs/abilities.md`).
- **Channel** — Control casts on the active `Status.Linked` pair, upgrading it to
  **Team Spirit / Life Network** (`Status.TeamSpirit`): heal/damage-share/shield effects
  now route through the whole chain, not just the original two linked actors.
- **This build's scope, explicitly limited:** per CLAUDE.md's own risk callout (Build order
  priority note), the harder version of this mechanic — target rotation mid-fight, the
  chain breaking on bad positioning/distance — is a **Build 2+ concern**. This document
  specs the mechanic itself (Link → Channel → group-wide routing); it does not scope-creep
  into designing the rotation/break-condition layer prematurely. That's deliberately
  deferred, not forgotten.

## Tags read/written

| Tag | Direction | Ability |
|---|---|---|
| `Status.Linked` | written by Support `Link`, read by Control `Channel` | see `docs/abilities.md` |
| `Status.TeamSpirit` | written by Control `Channel` (upgrade output) | see `docs/abilities.md` |

## Where authority resolves

- Life-drain tick: server-only, applied per-actor on a fixed cadence off server time
  (§4.5) — never accumulated client `DeltaTime`.
- `Link`'s relationship state (who's linked to whom): server-owned; the visual beam/line
  effect is the only client-cosmetic piece.
- `Channel`'s upgrade check (`HasMatchingGameplayTag(Status.Linked)`) and the resulting
  group-wide routing table: server-only.
- Heal/damage-share/shield routing through the chain once `Status.TeamSpirit` is active:
  server-only — this is exactly the kind of "gameplay-critical state" §4.2 forbids any
  client-side prediction of.

## Buildable via unreal-mcp vs. requires C++

| MCP-buildable (content wiring) | Requires C++ |
|---|---|
| Room level layout, objective actor placement | Life-drain volume/tick logic |
| Link/Team Spirit beam visual effect wiring | `Link`'s relationship state and its replication |
| Data asset tuning (drain rate, resource pool sizes) in `DA_GameConstants` | `Channel`'s upgrade check and the group-wide heal/damage-share/shield routing logic |

## Exit / fail criteria

- **Success:** the room's objectives complete before the drain overwhelms the party.
- **Fail (wipe):** all five `DOWNED` simultaneously, or the room's own drain-completion
  fail condition triggers (§6.6 — "room drain completes" is explicitly named as a
  scene-specific wipe condition, distinct from the all-downed case). Instant restart from
  scene start, unlimited attempts.

## Build reference

Build 2 (CLAUDE.md §7), but built **second** in practice per the Build order priority
note — right after Hold the Gate (Build 1), ahead of the other three remaining scenes.
