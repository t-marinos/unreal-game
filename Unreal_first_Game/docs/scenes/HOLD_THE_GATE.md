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
  that fixate on a random non-Tank plate-holder (retargeting to another when their target
  goes Downed). Each monster is a Mannequin-bodied `ACoopMonsterCharacter` that **walks in a
  straight line toward its fixated target** (`ACoopMonsterAIController`, `AddMovementInput`
  steering — deliberately *no* navmesh / behaviour tree / pathfinding, see the caveat under
  "Where authority resolves"). When it reaches melee range it **telegraphs a windup** — a
  flat red ground ring at the target's feet for `MonsterAttackWindupSeconds` — then strikes:
  direct damage **plus a knockback** shove directly away from the monster.
- **Why the knockback matters** — it's tuned strong enough to push a plate-holder **off
  their plate**, which trips the plate's own occupancy check and closes the gate. The
  monster threat attacks the *objective*, not just HP — which is exactly what makes
  Fortress's knockback-resist worth a second player's action.
- **Tank's job is not damage output — it's threat control:**
  - **Body-block** — a monster's straight-line path into the Tank's capsule is physically
    stopped (capsule vs capsule, no code); it never reaches melee range of the plate-holder
    behind the Tank. Standing in the lane is the play.
  - **Shield-shove** — raising `Shield` also `LaunchCharacter`s every monster in its forward
    cone away from the Tank (`ShieldShoveImpulse`). Shield is a repositioning tool, not just
    a damage filter.
  - Keep all four plate-holders alive and on their plates.
- **Shield** — Tank's directional damage-blocking volume (see `docs/abilities.md`); it
  negates incoming damage for its holder and shoves monsters (above). Early in the scene,
  Shield alone is enough to cover one threat direction at a time. It does **not** resist
  knockback — a Shielded plate-holder still gets shoved off their plate by a strike.
- **The complication** — near the end, the threat volume/frequency exceeds what a single
  facing of Shield can cover. Plain Shield is no longer sufficient.
- **Fortress** — Control casts `Stabilize` on Tank's active `Status.Shielded`, upgrading
  it to `Status.Fortress`: damage negation extends to protect multiple teammates in a
  radius at once (not just Tank's own facing), **and** a Fortress'd target keeps
  `(1 - FortressKnockbackResistPercent)` of a monster strike's knockback — i.e. it barely
  moves, so a Fortress'd plate-holder stays on their plate through a hit that would have
  dislodged a merely-Shielded one. This is the scene's actual teaching moment — protection
  becomes a synchronized two-player action, not a solo tank stat.

## Tags read/written

| Tag | Direction | Ability |
|---|---|---|
| `Status.Shielded` | written by Tank `Shield`, read by Control `Stabilize` **and by `CoopHealthComponent::ApplyDamage`** (negates the hit) | see `docs/abilities.md` |
| `Status.Fortress` | written by Control `Stabilize` (upgrade output); read by `ApplyDamage` (negates the hit) **and by the monster's `PerformStrike`** (scales knockback by `1 - FortressKnockbackResistPercent`) | see `docs/abilities.md` |

The monster's own "about to strike" windup is **not** a tag — it's transient AI state
(`bool bWindingUp` + a timer on `ACoopMonsterCharacter`), so nothing else reads it.

## Where authority resolves

- Plate-occupancy check (all four held simultaneously → gate opens): server-only,
  continuously evaluated, not a fire-once trigger.
- Monster spawn/targeting logic (fixate on plate-holders, retarget when the target goes
  Downed): server-only. This scene is the origin of the fixate/retarget monster pattern that
  Gravity Bridge later reuses. CLAUDE.md §6.5 records this as a deliberate, user-approved,
  one-off exception to the "boss has no AI" rule, scoped to this monster type only — not a
  general precedent for adaptive AI elsewhere. Full reasoning: `DECISIONS.md`'s "Monster
  combat inside Hold the Gate" entry (and its "Follow-up (2026-09-04): the monsters now move
  and attack in melee" subsection).
- Monster movement (`ACoopMonsterAIController`): server-driven `AddMovementInput` toward the
  fixated target, **straight-line steering only — no navmesh, no behaviour tree, no
  pathfinding**. The capsule position itself replicates via the default
  `CharacterMovementComponent` path (CLAUDE.md §4.2's one sanctioned client-prediction).
- Monster melee attack: server-only. `PerformAttackTick` gates on `MonsterMeleeRangeUnits`,
  then a `MonsterAttackWindupSeconds` telegraph (the `ACoopMonsterStrikeTelegraph` ring is
  server-spawned and replicated so all five clients see the same window at the same instant,
  §4.5), then `PerformStrike` re-checks range and applies damage + `LaunchCharacter`
  knockback. The knockback rides the same movement-replication path as normal movement.
- Shield's damage negation, its forward coverage cone, and its monster-shove
  (`ShieldShoveImpulse`): server-only.
- `CoopHealthComponent::ApplyDamage` negating a hit under `Status.Shielded` **or**
  `Status.Fortress`: server-only.
- Stabilize's upgrade check (`HasMatchingGameplayTag(Status.Shielded)`) and the resulting
  Fortress radius coverage + knockback-resist (`FortressKnockbackResistPercent`, read in the
  monster's `PerformStrike`): server-only.

## Buildable via unreal-mcp vs. requires C++

| MCP-buildable (content wiring) | Requires C++ |
|---|---|
| Gate room level layout, plate trigger-volume placement, gate actor placement | Plate-occupancy continuous check and gate-open logic |
| Monster spawner actor placement; `BP_MonsterCharacter` (Mannequin mesh/anim/tint) and `BP_MonsterStrikeTelegraph` (`M_TargetRing` ref) wiring | Monster spawn timing, targeting, fixate/retarget logic; `ACoopMonsterAIController` straight-line steering; the melee windup → `PerformStrike` (damage + knockback) split |
| Shield/Fortress visual representation (decal, mesh, or widget indicator); the red windup ring is content-agnostic (`M_TargetRing`, tinted in C++) | Shield's damage-blocking volume, its monster-shove loop, and Fortress's multi-teammate coverage/knockback-resist logic; `CoopHealthComponent` negating under `Status.Fortress` |
| Data asset tuning in `DA_GameConstants` (plate count fixed at 4; `MonsterMoveSpeed`, `MonsterMeleeRangeUnits`, `MonsterAttackWindupSeconds`, `MonsterKnockbackImpulse`, `ShieldShoveImpulse`, `FortressKnockbackResistPercent`, Shield/Fortress coverage radius) | `Stabilize`'s upgrade check and the Shield→Fortress tag transition |

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
