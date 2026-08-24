# Scene 5 — The Heart

**Star role:** Damage · **Synergy taught:** Execution / Overload (team-created `VULNERABLE` + Support buff → Damage finisher)

## The fantasy

"The team created the shot. I have to land it."

## The room

The final pre-boss chamber holds a Heart-like target that ignores normal damage entirely.

- **The Heart** — a target actor immune to standard damage instances; only consumes
  damage through the `Execution`/`Overload` finishers, and only during a vulnerability
  window.
- **Manufacturing the window** — the other four roles use their learned relationships to
  open a short window on The Heart, applying either `Status.Vulnerable.Physical` or
  `Status.Vulnerable.Magic` (server-timed, §4.5). Exactly which combination of abilities
  produces which vulnerability type is a scene-specific design detail beyond this pass's
  scope — this file documents the tag contract (something upstream writes one of the two
  `Vulnerable` tags; `Execution`/`Overload` read them), not the precise four-player
  choreography that produces it.
- **The read** — Damage must identify, within the window, whether the exposed
  vulnerability is Physical or Magic, and use the matching finisher (`Execution` for
  Physical, `Overload` for Magic — see `docs/abilities.md`). A good execution removes a
  meaningful HP threshold (PDF example: ~5%).
- **Support's role in this synergy is still open.** `docs/abilities.md` flags as
  unresolved whether the "Support buff" input to this synergy reuses `Speed` or needs a
  distinct ability — not decided in this pass.

## Tags read/written

| Tag | Direction | Ability |
|---|---|---|
| `Status.Vulnerable.Physical` | written by team mechanic (TBD choreography), read by Damage `Execution` | see `docs/abilities.md` |
| `Status.Vulnerable.Magic` | written by team mechanic (TBD choreography), read by Damage `Overload` | see `docs/abilities.md` |

## Where authority resolves

- The Heart's damage-immunity outside a vulnerability window: server-only.
- Whichever team mechanic opens a `Vulnerable.*` window: server-only, server-timestamped
  expiry — the window closing is a clock the server owns, not something a client can
  extend by acting "fast enough" locally.
- `Execution`/`Overload`'s window check, damage resolution, and tag consumption:
  server-only, per `docs/abilities.md`.

## Buildable via unreal-mcp vs. requires C++

| MCP-buildable (content wiring) | Requires C++ |
|---|---|
| Chamber level layout, Heart actor placement/visual | Heart's damage-immunity logic and its `Execution`/`Overload`-only damage path |
| Vulnerability-window visual telegraph (decal/material swap for Physical vs. Magic) | The team mechanic that writes `Status.Vulnerable.Physical`/`Status.Vulnerable.Magic`, and its server-timed expiry |
| Data asset tuning (window duration, HP-threshold-per-execution) in `DA_GameConstants` | `Execution`/`Overload`'s window check, damage resolution, and tag consumption |

## Exit / fail criteria

- **Success:** The Heart's HP threshold is depleted via successful Execution/Overload
  reads, opening the boss chamber.
- **Fail (wipe):** all five `DOWNED` simultaneously (§6.6). No scene-specific fail
  condition beyond the standard wipe rule is named in the source material for this scene.
  Instant restart from scene start, unlimited attempts.

## Build reference

Build 2 (CLAUDE.md §7).
