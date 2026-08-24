# Abilities, Tags & Synergy Resolution

Canonical source for every role's ability kit and every `FGameplayTag` the game uses.

**Rule (from CLAUDE.md §4.6): do not invent new tag names in code. Add them here first.**
Tags use dot-hierarchical `FGameplayTag` naming (`Status.SpeedBuff`, not `SpeedBuff` or a
loose string). Every status tag pairs with a server-timestamped expiry (`GetServerWorldTimeSeconds()`
+ duration) per §4.5 — never a client-ticked countdown.

Source material: `docs/Andreas_Game_Social_Gaming_Concept.pdf` (why) and
`docs/Andreas_Game_Prototype_Dungeon_v0.2_Visual_Brief.pdf` (what). Cross-reference:
CLAUDE.md §6.1 (roles), §6.2 (synergy table), §6.4 (scenes).

**Roster status:** only the abilities required by the 5 named synergies are fully specced
below. Every role has 1–2 remaining slots marked `TBD` — those are a later design pass,
not decided here. Do not treat a `TBD` line as permission to invent details ad hoc; bring
it back to a design discussion first.

---

## Tag glossary

| Tag | Written by | Read by | Notes |
|---|---|---|---|
| `Status.SpeedBuff` | Support `Speed` | Runner `Dash` | Enables Thousand Dashes resolution |
| `Status.Shielded` | Tank `Shield` | Control `Stabilize` | Baseline block state |
| `Status.Fortress` | Control `Stabilize` (upgrade output) | Fortress protection logic | Multi-teammate, knockback-resistant |
| `Status.Broken` | Tank `Armor Break` | Control `Mind Fracture` | Short window, target-specific |
| `Status.Linked` | Support `Link` | Control `Channel` | Two-actor relationship (Support ↔ target) |
| `Status.TeamSpirit` | Control `Channel` (upgrade output) | Life Network heal/buff/damage-share routing | Group-wide, routes through the link chain |
| `Status.Vulnerable.Physical` | Team-created (The Heart's mechanic) | Damage `Execution` | Consumed on use |
| `Status.Vulnerable.Magic` | Team-created (The Heart's mechanic) | Damage `Overload` | Consumed on use |
| `Status.Downed` | 0 HP (§6.6) | Revive logic | Not tied to a synergy — core failure-state tag |

---

## TANK

**Fantasy:** "For the next 15 seconds, everyone trusts me."

### Shield — *Fortress input*
- **Effect:** Raises a directional damage-blocking volume in front of Tank, absorbing/negating incoming damage from that facing.
- **Writes:** `Status.Shielded` (self, or the actor(s) currently standing behind it — exact coverage shape is a Hold the Gate implementation detail, see `docs/scenes/HOLD_THE_GATE.md`).
- **Reads:** none.
- **Taught in:** Hold the Gate (Scene 2).
- **Server authority:** the blocking volume's damage negation resolves server-side only; the visual raise/lower is the only client-local cosmetic.

### Armor Break — *Mind Fracture input*
- **Effect:** Tank targets one enemy actor (used against False King clones to "test" them).
- **Writes:** `Status.Broken` on whichever target it's used on — **regardless of whether that target is the real one**. Armor Break itself does not reveal anything; it only opens Mind Fracture's window. See `docs/scenes/THE_FALSE_KING.md` for the full resolution design (flagged there as a proposal, not locked).
- **Reads:** none.
- **Taught in:** The False King (Scene 3).
- **Server authority:** tag application and the target's real/fake identity are server-only state; no client ever learns which clone is real before a reveal fires.

### TBD (1–2 slots)
- No full spec. PDF prose (Gravity Bridge's narrative: "Tank taunts, Control creates/manipulates safe space...") hints at a **Taunt** (aggro/threat redirect) ability, but it isn't part of a named synergy formula — recorded here as a candidate, not designed.

---

## SUPPORT

**Fantasy:** "I decide who gets what, and when."

### Speed — *Thousand Dashes input*
- **Effect:** Targeted movement-speed buff.
- **Writes:** `Status.SpeedBuff` on target, server-timestamped expiry.
- **Reads:** none.
- **Taught in:** The Gravity Bridge (Scene 1).
- **Server authority:** matches CLAUDE.md §4.6's own worked example exactly — this is the canonical "how a synergy resolves" reference case.

### Link — *Team Spirit input*
- **Effect:** Support targets one teammate, creating a two-actor relationship between Support and that target.
- **Writes:** `Status.Linked` (both actors, or a paired ID stored on GameState — implementation detail for `docs/scenes/THE_DYING_ROOM.md`).
- **Reads:** none.
- **Taught in:** The Dying Room (Scene 4).
- **Server authority:** the link relationship (who's linked to whom) is server-owned state; only its visual (a beam/line effect) is client-cosmetic.

### TBD (1–2 slots)
- No full spec. The Dying Room's PDF text describes Support allocating a resource pool across "Heal, Shield, Speed, offensive buffs" — only Speed is a named synergy ability. Heal and any offensive buff are candidates, not designed.
- **Open question, not resolved here:** The Heart's synergy formula (Execution/Overload) calls for "Support buff" as one of its inputs. Whether that reuses `Speed` or needs a distinct ability is undecided — resolve when designing `docs/scenes/THE_HEART.md` in more depth than this pass covers.

---

## RUNNER

**Fantasy:** "I can get us somewhere nobody else can."

### Dash — *Thousand Dashes input/output*
- **Effect:** Normal case — a short-range dash. If the actor holds `Status.SpeedBuff` at cast time, resolves instead as **Thousand Dashes** (extended distance and/or multiple charges — exact numbers are a DA_GameConstants tuning value, not decided here).
- **Reads:** `Status.SpeedBuff`.
- **Writes:** none (consumes the buff's presence but doesn't necessarily clear it early — TBD whether Dash consumes the tag or just checks it; default to "checks, doesn't consume" unless a scene needs otherwise).
- **Taught in:** The Gravity Bridge (Scene 1).
- **Server authority:** this is the literal `if (Ability == EAbilityId::Dash && Actor->HasMatchingGameplayTag(Tag_SpeedBuff))` example from CLAUDE.md §4.6 — Dash resolution is server-only; the movement itself rides on the standard `CharacterMovementComponent` prediction already sanctioned by §4.2.

### Carry — *established in CLAUDE.md §8, not synergy-linked*
- **Effect:** Picks up and carries an **object** (never a player — CLAUDE.md §8 explicitly forbids player-carrying attachment as a jitter/desync risk).
- **Reads/writes:** none tag-related; object-attachment state, not a status tag.
- **Numbers (range, carry speed penalty, drop conditions):** `TBD`/DA_GameConstants.

### Chain — *established in CLAUDE.md §8, not synergy-linked*
- **Effect:** Applies a physics **impulse** — not a parent-child attachment (CLAUDE.md §8 is explicit on this constraint). Proposed reading, not locked: fires a tether that impulses either Runner toward an anchor point, or a target object toward Runner; resolved server-side.
- **Numbers:** `TBD`/DA_GameConstants.

### TBD (1 slot)
- No hint in either PDF.

---

## CONTROL

**Fantasy:** "I see what the others cannot. Listen to me."

### Stabilize — *Fortress output*
- **Effect:** Cast on a Tank actor currently holding `Status.Shielded`. Upgrades that shield to **Fortress**: multi-teammate coverage (not just Tank's own facing) and knockback resistance.
- **Reads:** `Status.Shielded`.
- **Writes:** `Status.Fortress` (replaces/upgrades `Status.Shielded` for the covered duration).
- **Taught in:** Hold the Gate (Scene 2).
- **Server authority:** the upgrade check (does the target currently hold `Status.Shielded`?) and the resulting protection radius/knockback-resist are server-only.

### Mind Fracture — *Mind Fracture output*
- **Effect:** Cast on a target currently holding `Status.Broken`. Server checks whether that target is the True King's real actor; if so, fires a reveal cue (see `docs/scenes/THE_FALSE_KING.md` for the proposed non-perspective-dependent cue design).
- **Reads:** `Status.Broken`.
- **Writes:** a reveal-state flag/cue trigger (not itself a persistent status tag on the target).
- **Taught in:** The False King (Scene 3).
- **Server authority:** the real/fake determination and reveal-firing are server-only; the reveal cue then replicates to all clients.

### Channel — *Team Spirit output*
- **Effect:** Cast on a pair currently holding `Status.Linked`. Upgrades the link to **Team Spirit / Life Network**: group-wide heal/damage-share/shield routing through the chain, not just the original two-actor link.
- **Reads:** `Status.Linked`.
- **Writes:** `Status.TeamSpirit` (group-wide, routed through the chain).
- **Taught in:** The Dying Room (Scene 4).
- **Server authority:** chain membership and routing are server-owned; §6.4's later-boss rotation/break-on-distance behavior is out of scope for this pass (Build 2+ concern, noted in `docs/scenes/THE_DYING_ROOM.md`).

### TBD (0–1 slot, optional)
- Control already reaches its full 3–4 range from the three synergy abilities above. A 4th slot is optional and undesigned.

---

## DAMAGE

**Fantasy:** "The team created the shot. I have to land it."

### Execution — *physical branch of Execution/Overload*
- **Effect:** Finisher, usable only while the target holds `Status.Vulnerable.Physical`. Deals a large, threshold-based chunk of damage (PDF example: ~5% of boss HP on a good execution).
- **Reads:** `Status.Vulnerable.Physical`.
- **Writes:** consumes the tag on use (or on window expiry, whichever comes first).
- **Taught in:** The Heart (Scene 5).
- **Server authority:** the window check, damage resolution, and tag consumption are all server-only.

### Overload — *magic branch of Execution/Overload*
- **Effect:** Same pattern as `Execution`, but keyed to `Status.Vulnerable.Magic` instead. Reading which branch is open (Physical vs. Magic) and picking correctly under time pressure is the scene's actual test — see `docs/scenes/THE_HEART.md`.
- **Reads:** `Status.Vulnerable.Magic`.
- **Writes:** consumes the tag on use (or on window expiry).
- **Taught in:** The Heart (Scene 5).
- **Server authority:** same as `Execution`.

### TBD (1–2 slots)
- Neither PDF describes Damage's baseline attack kit — only the finisher moment is specced. At least one remaining slot needs to be a normal, non-conditional damage ability so Damage has something to do outside the Execution/Overload window; not designed here.
