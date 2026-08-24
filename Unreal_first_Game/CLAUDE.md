# CLAUDE.md — Game (Prototype)

This file is read at the start of every session. It defines what we are building, what we are
deliberately **not** building, and the architectural rules that must not drift.

If a request in a session conflicts with a rule in this file, **stop and say so** rather than
silently working around it.

**Companion files — read these when relevant:**
- `docs/abilities.md` — full ability list per role, tag glossary, synergy resolution logic
- `docs/scenes/*.md` — one file per scene, written as each scene is built. Source of truth for *why*
  that scene works the way it does, not just what the code does — read the relevant one before
  changing a scene's systems. `docs/scenes/HOLD_THE_GATE.md` is the first.
- `Content/Data/DA_GameConstants` — every tunable number, as a `UDataAsset`. Single source of truth.
  Editable by hand in the Unreal Editor, or via the `unreal-mcp` tools (§3.2).
- `DECISIONS.md` — case law. Anything settled in a past session.
- `docs/*.pdf` — the original concept and dungeon design briefs, for background on intent.

---

## 1. What this project is

A five-player cooperative dungeon prototype. Five friends receive five unfamiliar temporary roles,
learn five collaboration relationships across five short scenes, and are then tested on all five by
a boss that repeats, combines, rotates and overlaps them.

**Core promise:** 5 friends. 5 temporary roles. 5 star scenes. 1 boss that requires all five
relationships. Nobody can carry alone.

The underlying goal is social, not mechanical. The game exists to bring a fragmented adult friend
group back together for one intense shared evening. It is not trying to be anyone's main game — it
is trying to be *the game our group plays together*. Difficulty must come from communication,
interpretation and coordination, not from mechanical skill or hours played, so that a competitive
player and a casual player can matter equally inside the same run.

**The target moment** is five people on Discord shouting over each other:
`"RUNNER GO!"` → `"FORTRESS NOW!"` → `"REAL ONE BACK LEFT!"` → `"LINK IS ROTATING!"` → `"EXECUTION — DO IT!"`

If that moment does not happen, nothing else about this project matters.

### This prototype is disposable by design

We are buying an answer to "does this feeling work," not a codebase. Do not optimise for
extensibility, reusability, or future-proofing. Optimise for **speed to a playable Friday test** and
for **being easy to debug**. Ugly is correct. Hardcoded is correct.

This still holds after the engine switch below. Do not let "Unreal has a proper way to do this"
pull us toward building generalised systems the disposable-prototype philosophy explicitly rejects
(see §4.6).

---

## 2. Team context (important)

- **Nobody on the team has game development experience**, and the engine switch (§3) adds a real
  learning curve on top of that: Unreal's editor, its replication/authority model, and either C++ or
  Blueprint as an authoring language are all new. Budget time for this explicitly — it is not
  overhead, it is the job now.
- The project is being built primarily with AI assistance.
- Therefore: prefer boring, explicit, heavily-commented code over clever or idiomatic code.
- Explain networking and game-loop concepts when they come up rather than assuming familiarity —
  this now includes explaining Unreal-specific concepts (Actor/Component replication, RPCs, the
  Role/RemoteRole authority model) that have no equivalent in a from-scratch server, not just
  general networking theory.
- When there is a simple approach and a correct-but-complex approach, take the simple one and note
  the tradeoff in a comment.
- Unreal's authority model has more surface area — and more footguns (default client-side movement
  prediction, multicast RPCs, replication conditions) — than a single hand-rolled state schema did.
  §10's "one human must understand the authority model" rule is *more* important now, not less.

---

## 3. Tech stack — LOCKED

| Layer | Choice |
|---|---|
| Engine | **Unreal Engine 5.8.1.** Pin this exact patch version for every teammate's install — opening the project in a different minor version can silently version-up save files and break for everyone else. |
| Language | **Hybrid, split by concern (see §3.2 for why):** C++ for core systems — replication, authority, the tag system, ability resolution, timers, boss timeline. Blueprint for content wiring — actor placement, materials, UI, simple input bindings. |
| Networking | Unreal's built-in Actor/Component replication and RPCs. Server-authoritative (§4.1). |
| Session model | **Listen Server** — one player's client is also the host. No dedicated server. |
| Transport / join | Players join over a **Tailscale** mesh VPN by direct IP (§3.1). No Steam, no Epic Online Services, no accounts beyond Tailscale itself. |
| Delivery | A **packaged Development-configuration Windows build**, shared as a file (not browser-hosted). Players install it once; no engine or editor install required to play. |
| Voice | Discord. We do not build voice chat. |

**Do not propose Colyseus, Three.js, Node.js, WebSockets, or any browser-based rewrite.** This
project was originally scoped as a browser game (TypeScript + Colyseus + Three.js) and that choice
is superseded — the rest of this file has been rewritten for Unreal. If you find a stray reference
to the old stack anywhere in this repo that this rewrite missed, flag it, don't silently follow it.

**Why Unreal, given the earlier rejection:** an earlier version of this document rejected Unreal
specifically because "Blueprints are not text (invisible to AI assistance) and Unreal C++ is a poor
target for generated code." That reasoning is only partly overridden, not wrong: see §3.2 for how we
mitigate it. This is a deliberate, informed decision to move ahead anyway, not an oversight.

**Do not add plugins** (Marketplace or engine-bundled) **without flagging it** and explaining why
built-in Unreal functionality is insufficient.

### 3.1 Hosting and joining

**For now, everything runs on a developer's local PC** as a Listen Server. Do not set up dedicated
server infrastructure, cloud hosting, containers, orchestration, CI/CD, or a managed backend. There
is no deployment pipeline and we do not want one yet.

**How five friends get into the same session:**

1. Every player installs [Tailscale](https://tailscale.com) (free) once and joins the same private
   tailnet. This gives every device a stable `100.x.x.x` address that behaves like it's on the same
   LAN, without port forwarding or a public IP.
2. The host starts the packaged build, which opens a Listen Server (`?listen`), and shares their
   Tailscale IP.
3. The other four connect via Unreal's default IP netdriver — direct-IP connect to
   `<host-tailscale-ip>:<port>`. No online subsystem, no Steam/EOS session interface, no accounts
   beyond Tailscale.

This is the direct replacement for the old "one shared URL, no builds, no downloads" rule. Be honest
about the tradeoff: it is not as frictionless as a browser link. Every player now needs (a) the
packaged build shared with them and (b) a one-time Tailscale install. That is the accepted, minimum
necessary cost of leaving the browser — see §8's Known design risks for the full accounting.

**Local development:** run two or more instances via Unreal's "Number of Players" / Play-In-Editor
multiplayer options to simulate several clients without needing Tailscale or a packaged build at all.
Combined with dev mode (§7), one person iterates alone without needing four friends online.

- **Never suggest port forwarding**, whether as the primary join method or as a "fix" when Tailscale
  connectivity misbehaves. It exposes the home network, requires router access, and may be
  impossible anyway if the ISP uses CGNAT — this is exactly why Tailscale was chosen.
- **Windows Firewall will prompt on first run** for the packaged executable — players must allow it,
  or connections silently time out with no error client-side. Document this in the playtest
  instructions, don't assume it's obvious.
- **Never hardcode a join IP.** The host's Tailscale IP is stable per-device but is still specific to
  that machine — read it fresh each session (`tailscale ip` or the Tailscale app) rather than baking
  a previous session's address into a config file or shortcut.
- Unlike the old Cloudflare quick-tunnel URL, **the Tailscale IP does not change on restart** — this
  is a genuine improvement over the browser-era hosting story, not just a lateral move.

**Latency:** the team and playtesters are in Cyprus, so local hosting keeps latency low. Tailscale
prefers direct peer-to-peer connections (WireGuard) over relaying through its coordination servers,
so this holds even across Cyprus-to-Cyprus home connections. Bandwidth is a non-issue at five
players; set `NetUpdateFrequency` deliberately (see §4.4) rather than trusting Unreal's per-class
defaults, so replication rate is a known, debuggable number.

**The host has zero ping.** Whoever runs the Listen Server has the smoothest possible experience and
is therefore the *least* reliable judge of whether a timing window is generous enough. Timing
judgements come from the player on the worst connection, not the host.

**Artificial latency toggle:** package Build 0 as a **Development** configuration, not Shipping —
Shipping strips console access, which we need for this and for the debug dump (§4.3) and dev mode
(§7). Use Unreal's built-in network emulation (`Net.PktLag`, `Net.PktLagVariance`, `Net.PktLoss`
console variables, or an Editor Preferences → Network Emulation profile) so timing can be tested at
realistic ping during solo local development, rather than discovering timing problems on a Friday
night. This replaces the old `SIMULATED_LATENCY_MS` env var — same intent, engine-native mechanism.

**When to move off Tailscale, to EOS or a dedicated server** (roughly Build 2, not before). Trigger
conditions:
- We want to drop the "everyone installs Tailscale" requirement for new playtesters outside the core
  five friends
- We want to test with players who can't easily be added to the tailnet, or want public/matchmaking
  session flow (still out of scope per §8 even then — this is about *reach*, not about building
  matchmaking)
- A developer's PC being asleep or updating cancels a playtest, and we want a persistent host

At that point: Epic Online Services (EOS) is free and ships an official Unreal plugin
(`OnlineSubsystemEOS`) built for exactly this — session creation/discovery and NAT traversal without
per-player IP knowledge. This is flagged as a parked upgrade, not solved now, same treatment the old
doc gave "when to move to a VPS."

### 3.2 AI tooling: the `unreal-mcp` server, and why it changes the C++/Blueprint call

This project has a local `unreal-mcp` server (see `.mcp.json`) connected to a running Unreal Editor
instance. It exposes structured, inspectable tools for actors, Blueprints, materials, data
assets/tables, static/skeletal meshes, the content browser, and viewport/PIE control — but **no
tools for writing or compiling C++ source.** That file editing still goes through normal
Read/Write/Edit on `.h`/`.cpp` files, followed by a human- or IDE-triggered build.

This directly informs the hybrid split in §3:
- **Blueprints are not actually "invisible to AI" in this project**, the way the old doc assumed for
  Unreal in general — they can be read and edited through `unreal-mcp`'s Blueprint/Object/Actor
  tools, and verified visually via `CaptureViewport`/`CaptureAssetImage`. Use them for content
  wiring: placing actors, assigning materials, laying out UMG widgets, wiring simple input.
- **C++ has no equivalent AI-editor bridge here** — it's plain text edited normally, then built
  outside this tool's reach. Use it for anything that needs to be reliably correct, diffable, and
  covered by the DECISIONS.md paper trail: replication, authority-sensitive logic, the tag system,
  ability/synergy resolution, timers, the boss timeline.
- If a future session finds `unreal-mcp` (or an equivalent) gains real C++ authoring/build tools,
  this split is worth revisiting — it was chosen because of today's tooling gap, not as a permanent
  philosophical stance.

---

## 4. Architecture rules — NON-NEGOTIABLE

These exist because desync bugs produce no stack trace, and nobody on this team can debug them by
intuition. Every rule below exists to make state inspectable. Unreal gives us more of this for free
than a from-scratch server would (replication and the Role/Authority model are built in) — the rules
below are about not undermining that, not about rebuilding it.

### 4.1 The server owns everything

- All **gameplay-critical** state — role assignment, ability cooldowns, tags, synergy resolution,
  damage, objective/scene state, boss timeline — lives on the server (the Listen Server's Actors:
  GameMode, GameState, PlayerState) and is written only where `HasAuthority()` is true.
- Clients send *intent* via Server RPCs (`"pressed Q"`, `"moving north"`), never results (`"I dealt
  40 damage"`).
- Every ability, synergy check, damage calculation, status application and objective state change
  resolves **server-side only**, and must be traceable to a `Server_*` RPC or a function that asserts
  `HasAuthority()`.

### 4.2 Client-side prediction: allowed for movement, forbidden for gameplay

This narrows the old blanket "no client prediction" rule, and the narrowing is deliberate — read the
reasoning before changing it.

Unreal's `CharacterMovementComponent` predicts and reconciles movement by default; that behaviour is
free, load-bearing for how the engine's netcode is built, and fighting it would cost this team far
more than it saves. **Leave default movement prediction on.** Our design is coordination on 500ms+
windows, not twitch aim or frame-perfect platforming, so movement-smoothing prediction being "a
little ahead" of the server is never the thing a scene depends on.

What must **never** be predicted client-side, full stop: ability activation results, tag
application/expiry, damage, synergy resolution, objective/scene state, boss telegraphs. These
resolve on the server and reach clients only through replication — a client may show an optimistic
*local* animation/VFX cue on button press for feel, but the actual game-state change happens only
when the server says so.

**If a mechanic seems to require sub-500ms reaction to gameplay-critical state, the mechanic is
wrong, not the netcode.** This sentence is unchanged from the original rule — only the movement
carve-out above is new. Treat that carve-out as reversible: if capsule movement ever needs to be
made non-predicted (e.g. some scene turns out to need frame-exact positional sync), that's a
contained change to the movement component config, not a rearchitecture.

### 4.3 State must always be printable, even though it's not one object

Unreal spreads state across GameState, PlayerStates, and per-actor tag containers rather than one
schema object — that's an engine-imposed change from the old single-Colyseus-Schema model, not a
relaxation of the underlying rule.

- Every player's role, tags (with expiry), cooldowns; boss state; scene/objective state; timers must
  all be reachable from GameState or a PlayerState.
- Build a console `Exec` command (e.g. `DumpGameState`) that walks GameState + every PlayerState +
  their tag containers and logs the full picture as structured text (JSON-shaped is fine). This must
  work in a Development packaged build, which is why §3.1 requires Development, not Shipping.
- If you cannot run one command and see the truth of the game, the architecture has drifted. Fix it.

### 4.4 Deliberate, debuggable timing — not a fixed tick, but the same discipline

Unreal doesn't run one fixed global simulation tick the way a hand-rolled 20Hz server loop would.
Translate the underlying discipline instead of trying to force a literal fixed-tick loop:

- Compute cooldowns, telegraph windows, and the boss timeline from **absolute server timestamps**
  (`GetServerWorldTimeSeconds()`), never by accumulating per-frame `DeltaTime` — this avoids drift
  entirely rather than just bounding it.
- Set `NetUpdateFrequency` explicitly (don't leave it at each class's engine default) on GameState
  and any Actor whose replication matters for gameplay feel — pick a number (e.g. 20–30Hz), write it
  down, and treat changing it as a tuning decision, not an accident.
- No frame-rate-dependent gameplay logic. If something reads `DeltaTime` for anything other than
  local, cosmetic interpolation, that's a bug.

### 4.5 All timing uses server time

Every countdown, telegraph, cast bar and warning window is timestamped using
`GetServerWorldTimeSeconds()`. Never derive a telegraph from a client's local clock or local
`DeltaTime` accumulation. `"FORTRESS NOW!"` only works if all five players see the same window at the
same instant.

### 4.6 Tags, timers, and a hardcoded interaction table

The entire game's logic is expressible as: *actors hold tags; abilities check tags; abilities apply
tags; the boss timeline applies tags.*

- Use Unreal's native `FGameplayTag` / `FGameplayTagContainer` for status conditions — it's built
  into the engine (not a third-party dependency), replicates, and is readable/writable from both C++
  and Blueprint, which fits the hybrid split in §3.2. Pair each tag with an expiry timestamp
  (server time, §4.5) rather than relying on a duration ticked down client-side.
- The canonical tag list lives in `docs/abilities.md` — **do not invent new tag names; add them
  there first.**
- **Do NOT build a generic ability/synergy/effect system**, and do not reach for
  GameplayAbilitySystem (GAS) as a way to get this "for free" — it is a large, general framework and
  would encode an abstraction we don't yet know is right, exactly the trap §1 warns against. Write
  five explicit conditionals, one per synergy, in C++.
- Example of the correct shape:
  ```cpp
  // Synergy 1: Support Speed + Runner Dash -> Thousand Dashes
  if (Ability == EAbilityId::Dash && Actor->HasMatchingGameplayTag(Tag_SpeedBuff))
  {
      ResolveThousandDashes(Actor);
  }
  else if (Ability == EAbilityId::Dash)
  {
      ResolveNormalDash(Actor);
  }
  ```

### 4.7 Joining a session

Five friends need to reach the same session with as little friction as this engine allows. The
mechanism (full detail in §3.1):

- One Listen Server, one host, direct IP connect over Tailscale.
- `GameMode`'s max player count caps the session at 5. The run starts when 5 players are present (or
  when a host presses start in dev mode).
- No lobby browser, no matchmaking, no accounts beyond Tailscale. Players type a display name and
  that is all.

### 4.8 No unsolicited refactoring

Do not restructure folders, rename files/classes, reformat unrelated code, or "clean up" code (or
Blueprints, or content) you were not asked to touch. Architectural drift across sessions is the
primary failure mode for this project. If you believe a refactor is needed, say so and wait.

---

## 5. Presentation rules

- **High 3/4-angle camera in a 3D scene, starting fixed, now player-rotatable.** Not third-person.
  Not top-down 2D. We keep real 3D space (body-blocking, hiding behind the shield, positioning for
  the link chain are all gameplay). Each player can hold right-click and drag to orbit their own
  camera around the arena center, WoW-style — implemented as a dedicated, **local-only** camera
  actor/spring-arm setup (`Source/Unreal_first_Game/Camera/`), never replicated, never affecting what
  any other player sees, and the camera still never *follows* a player (only the viewing angle
  changes, not what it's centered on). The project starts from Unreal's ThirdPerson template, whose
  default follow-camera is **not** what we want — replacing it is part of Build 0, not a later pass.
  This reverses an earlier "fully fixed, no camera engineering at all" rule — see `DECISIONS.md` for
  why, and treat it as reversible: if free rotation turns out to hurt readability once more scenes
  exist, going back to a truly fixed camera is a small, contained change, not a rearchitecture.
  **Because the angle is no longer guaranteed-fixed, any future "this only reads correctly from a
  specific viewing angle" tell (e.g. Scene 3/The False King's "which clone is real" reveal) is
  stricter than before, not more lenient** — it must never depend on perspective, full stop, since
  there's no longer a single perspective everyone shares.
- **The stock Unreal Mannequin (Manny/Quinn), not bare capsules.** The ThirdPerson template ships
  this skeletal mesh pre-rigged and pre-animated (idle/walk/run/jump) for free — using it as-is costs
  zero extra art or animation production time, so we keep it rather than stripping it down to a
  primitive capsule. This reverses the earlier "capsules and coloured primitives, no character
  models" rule — see `DECISIONS.md`. Facing, which a bare capsule needed a cone/arrow hack to show,
  is now inherent to the model. Per-player/per-role identification uses a Dynamic Material Instance
  colour tint on the Mannequin's base material (a parameter change, not custom art or a new mesh) —
  still zero art-production cost, and doable via `unreal-mcp` as content wiring. **Full MetaHuman
  characters stay out of scope** (§8): MetaHuman is a separate, much heavier photorealistic pipeline
  that the template does not ship by default — don't confuse it with the Mannequin.
- **No VFX, no particles, no post-processing.** A coloured ring on the ground (an Unreal Decal actor
  or a flat unlit plane) is a spell effect. The template ships with Lumen and other default visual
  fidelity turned on — turn it off (or use a lightweight/unlit rendering path) rather than accepting
  it by default; it costs iteration speed on a non-gaming dev PC for zero benefit to this prototype.
- Status conditions are shown as coloured bars above each character — a 3D Widget Component (UMG)
  attached to the actor and billboarded to camera.
- UI is built in **UMG**, Unreal's native UI system — no third-party UI plugin unless it earns
  itself. Keep layouts simple; this replaces the old "plain HTML/CSS over canvas" rule.
- Telegraphs are flat decals on the ground. Attacks are damage volumes (simple collision
  shapes/traces), not particle-driven hit detection.
- Everything readable, nothing pretty. If it looks good, we over-invested.

---

## 6. Game design specification

### 6.1 Roles

Five roles, assigned **randomly** at run start. Roles are temporary — they belong to the run, not
to the player. Each role has **3–4 active abilities**, not a rotation.

`TANK` · `SUPPORT` · `RUNNER` · `CONTROL` · `DAMAGE`

**Full ability definitions live in `docs/abilities.md`.** Do not invent abilities. If something is
missing, propose it as an edit to that file first.

### 6.2 The five synergies

These are the spine of the game. Each is taught in one scene, then stressed by the boss.

| # | Synergy | Type | Formula |
|---|---|---|---|
| 1 | **Thousand Dashes** | Enhancement | Support `Speed` + Runner `Dash` |
| 2 | **Fortress** | Enhancement | Control `Stabilize` + Tank `Shield` |
| 3 | **Mind Fracture** | Activation | Tank `Armor Break` → target becomes `BROKEN` → Control `Mind Fracture` |
| 4 | **Team Spirit / Life Network** | Sequence | Support `Link` + Control `Channel` |
| 5 | **Execution / Overload** | Sequence | Team-created `VULNERABLE` + Support buff → Damage finisher |

**Synergy types:**
- *Enhancement* — a teammate transforms or upgrades your ability.
- *Activation* — a teammate creates a state that unlocks your stronger action.
- *Sequence* — several players execute in the correct order.

### 6.3 The 60-second preparation arena

Five players spawn in a small locked circular arena. Roles assigned randomly. A visible 60-second
countdown starts immediately — there is no relaxed loading period.

Each player sees:
- 3–4 ability cards with icon, name, one-sentence explanation.
- Their role label.
- A **Team Synergies** panel showing *that* a relationship exists with another role — **but not the
  full solution.**
- The always-visible countdown.

The design intent: the UI reveals that a relationship exists but forces players to talk to work out
how to use it. Silence wastes preparation time. The first teamwork test is communication itself.

### 6.4 The five star scenes

Each scene has one **Star Player** whose responsibility gates progress. The other four still
contribute. Each scene teaches one synergy.

| # | Scene | Star | Teaches | Summary |
|---|---|---|---|---|
| 1 | **The Gravity Bridge** | Runner | Thousand Dashes | Gravity field slows everyone to ~10% speed. Support buffs Runner, who crosses and pulls a lever. Lever releases monsters that prioritise the Runner; bridge starts collapsing. Objective is to get the *whole party across*, not to kill everything. |
| 2 | **Hold the Gate** | Tank | Fortress | A gate opens only while four pressure plates stay occupied. Four players are pinned; Tank is the only mobile one. Tank controls threat, blocks projectiles, keeps plate-holders alive. Near the end, Shield alone is insufficient — Control's `Stabilize` converts it to `Fortress`. |
| 3 | **The False King** | Control | Mind Fracture | One enemy multiplies into identical copies; only one is real. Attacking the wrong copy incurs penalties. Tank tests with `Armor Break`; on `BROKEN`, Control has a short window to cast `Mind Fracture`, which reveals the truth. Control must then *call it out*. Value is interpretation + callout + timing. |
| 4 | **The Dying Room** | Support | Team Spirit | The room drains life while objectives must be completed. Support cannot spam Heal — limited energy must be allocated across Heal/Speed/Link/Empower. Support creates a `Link`; Control `Channel`s into it to create a group-wide network. |
| 5 | **The Heart** | Damage | Execution | A Heart that ignores normal damage. The other four manufacture a short vulnerability window using learned relationships. Damage must read whether the vulnerability is Physical or Magic and land the correct finisher within a few seconds. |

**Note for Scene 3:** the "which clone is real" tell must **not** be perspective-dependent. This is
now stricter than when it was written: the camera is only high-angle by *default* and each player can
freely rotate their own view (§5), so there is no longer even a guaranteed shared fixed angle to lean
on. Use an audio cue, a UI marker, or a ground effect — not something only visible from a particular
angle, and not something that assumes every player is looking from roughly the same direction.

### 6.5 The boss

The five scenes teach only the first 5% of each relationship. The boss uses the *same vocabulary*
and escalates through four layers:

| Layer | What changes |
|---|---|
| **LEARN** | (happens in the scenes) |
| **REPEAT** | Boss asks for a recognisable version so the team recalls the mechanic |
| **COMBINE** | Two learned mechanics happen at the same time |
| **ROTATE** | Targets, links, positions or responsibilities change mid-execution |
| **OVERLAP** | Several mechanics active simultaneously, little breathing room |

Intended full escalation (post-MVP):
- 100–90%: obvious versions of all five synergies
- 90–75%: two mechanics combine
- 75–50%: rotations begin
- 50–25%: mechanics overlap
- 25–5%: continuous communication required
- Final 5%: compressed orchestration; one failure breaks the chain

**Boss HP is a teamwork progress meter.** Meaningful chunks of HP are earned by successful
coordinated executions, not by sustained DPS.

**The boss has no AI.** It is a scripted sequencer: a timeline that fires telegraphs and applies
tags at fixed offsets. Do not build behaviour trees, pathfinding, or adaptive difficulty — and do not
reach for the project's `GameplayStateTree` plugin to build one either; a hardcoded timeline (§4.6's
philosophy applied to the boss) is correct here, not a general-purpose behavior system.

*(Exception, logged not standing: Hold the Gate now also has monsters that fixate on a random
player and retarget on death — a deliberate, user-approved, one-off deviation from this rule and
from Build 1's scope below. See DECISIONS.md's "Monster combat inside Hold the Gate" entry for the
full reasoning. Treat the rule above as still governing any *new* work — this is the one place it
was knowingly set aside, not a revision to the rule itself.)*

### 6.6 Failure, downed state and retry

Decided rules for the prototype. Tension should come from interdependence, not from punishment.

- Players do not die outright. At 0 HP a player becomes `DOWNED`: immobile, cannot use abilities,
  still visible and still able to talk. This matters — a dead player who can't act stops
  contributing to the Discord conversation, which is the thing we are measuring.
- Any teammate can revive a `DOWNED` player by standing adjacent for a few seconds. Reviving is
  itself a costly decision under pressure, which is good.
- **Wipe** = all five `DOWNED` simultaneously, or the scene's own fail condition (bridge fully
  collapses, gate closes, room drain completes).
- On wipe: **instant restart from the beginning of the current scene. Unlimited attempts.**
  No attempt limits, no lockouts, no penalties in the prototype. We want them saying *"again, again"*
  within two seconds of failing — anything that slows that down is working against the test.
- The limited-failure / elimination system from the original concept is **out of scope** (see §8).

---

## 7. The MVP — what we are actually building

Three builds. **Do not work ahead.** Each build answers a question before the next begins.

*(Logged exception: monster combat — spawning, HP, player-fixation, Strike-kill — was added inside
Hold the Gate ahead of Build 2, as a deliberate user-approved deviation. See DECISIONS.md. This does
not loosen "do not work ahead" for anything else.)*

### Build 0 — "Five Mannequins in a room" (target: 1–2 weeks)

The only goal is to prove the plumbing and prove that five friends can actually get in.

- [ ] Add a C++ module to the project (currently Blueprint-only — no `Source/` folder exists yet)
- [ ] Listen Server with `GameMode` capped at exactly 5 players
- [ ] Level with a fixed high 3/4 camera (replacing the ThirdPerson template's follow camera), five
      stock Mannequin characters, colour-tinted per player via a Dynamic Material Instance (§5)
- [ ] Networked movement (server-authoritative via default `CharacterMovementComponent` replication —
      built-in prediction accepted per §4.2, no custom prediction code written)
- [ ] One shared visible timer, replicated from server time (§4.5)
- [ ] One button/actor that produces an effect all five players can see, resolved via a Server RPC
- [ ] Packaged as a **Development** build (§3.1); host runs it, other four connect by direct IP over
      Tailscale
- [ ] Never a hardcoded join IP — read the host's current Tailscale IP each session (§3.1)
- [ ] Debug `Exec` command: full state dump (§4.3), ping, replication rate
- [ ] **Dev mode (see below)**
- [ ] Artificial latency toggle via Unreal's network emulation (§3.1)

**Exit criteria:** five friends in different houses, each having installed the shared build and
Tailscale once, join by direct IP and see each other move, with no live support required beyond that
one-time setup.

#### Dev mode — build this in Build 0, not later

You cannot summon five friends every time you change a line of code. Without a solo iteration path,
every change requires a Discord call and the project stalls. Dev mode must provide:

- Empty player slots filled with **dummy players** — simple AI-controlled pawns that hold position,
  occupy pressure plates, take damage, and can be scripted with simple behaviours (`StandOn(Plate)`,
  `FollowPlayer`, `Idle`). Use a plain `AAIController` with hardcoded logic — not a general
  behavior-tree AI system (§6.5's "no adaptive difficulty" principle applies here too).
- The ability to **possess** any dummy from the dev client, to test another role's abilities —
  Unreal's `Possess`/`UnPossess` maps directly onto this.
- A **scene skip** `Exec` command to jump straight to any scene or boss phase.
- A **god mode / invuln** toggle.
- Start the run without waiting for 5 players.

This is the highest-leverage thing in Build 0. Treat it as a feature, not a debug afterthought.

### Build 1 — one relationship, end to end (target: 3–4 weeks)

The smallest artifact that tests the actual thesis: *teach a relationship, then stress it.*

- [ ] 60-second prep arena: random role assignment, ability cards (UMG), synergy panel, countdown
- [ ] **Scene 2 — Hold the Gate** (pressure plates, spawners, Tank shield collider, Fortress synergy)
- [ ] One mini-boss that repeats the Fortress mechanic, then **combines** and **rotates** it
- [ ] Downed/revive, wipe, instant retry

**Scene 2 is first on purpose:** it is the cheapest to build (trigger volumes, spawners, one carried
collider) and has the highest social density — all five players are under pressure simultaneously
rather than four watching one star. If the Discord energy does not appear here, it will not appear
anywhere, and we will know in a month instead of six.

**Exit criteria:** five friends play it and talk continuously. If they don't, we stop and rethink
the design rather than building four more scenes.

### Build 2 — full loop (target: 5–6 weeks)

- [ ] Remaining four scenes
- [ ] Full boss with **REPEAT → COMBINE → ROTATE only**
- [ ] Boss HP tied to successful coordinated executions

`OVERLAP` and the final-5% orchestration are **out of MVP scope**. If ROTATE isn't fun, OVERLAP
won't rescue it.

### Build order priority note

**Scene 4 (The Dying Room) should be built second**, immediately after Build 1. The Link/chain
mechanic is the riskiest and most technically fiddly thing in the design — distance checks, break
conditions, chain ordering, damage routing. We want to know early whether it is fun or just annoying.

---

## 8. Explicitly OUT OF SCOPE

Do not build, scaffold, or "prepare for" any of the following. If asked to, push back and reference
this section.

- Accounts, login, persistence, databases
- Cloud infrastructure, dedicated servers, containers, orchestration, CI/CD, deployment pipelines.
  The server runs as a Listen Server on a local PC, reached over Tailscale (see §3.1). Do not set up
  more than that.
- Steam or EOS integration — parked as a future upgrade path (§3.1), not to be built now
- Progression, XP, unlocks, cosmetics
- Matchmaking beyond "five friends join one Listen Server by IP"
- Tournament structure, qualification windows, scheduled live events, leaderboards
- Attempt limits, elimination, competitive failure states
- Prizes, sponsorships, monetisation
- Voice chat (we use Discord)
- Custom character art, custom animation/rigging work, VFX, post-processing, MetaHumans. (§5: the
  ThirdPerson template's stock Mannequin, used as-is, is now in scope — reusing free, already-authored
  content costs nothing; producing new character art or animation does, and MetaHuman specifically
  stays out of reach given its separate, heavier pipeline.)
- A generalised ability/synergy system, or the GameplayAbilitySystem plugin (§4.6)
- Sound design beyond functional cues
- Anti-cheat or input validation for adversarial players (these are friends; the threat model is
  jitter, not cheating)
- Mobile, controller, or accessibility support
- Procedural generation of any kind
- Host migration or reconnection recovery — if the run breaks, we restart
- Gameplay-critical client-side prediction (see §4.2 — movement prediction is the one accepted
  exception, and only for movement)
- **Carrying other players.** Attaching one player to another over the network is a known source of
  jitter and desync. Runner's `Carry` handles objects only; `Chain` applies an impulse, not a
  parent-child attachment.

### Known design risks — parked, not solved

Documented here so they are not forgotten, but **not to be addressed in the MVP**:

1. **The hard requirement of exactly five players.** Our target audience is adults with unreliable
   schedules; a design that dies when one of five cancels is aimed at exactly the people least able
   to guarantee five. We will need a 3–4 player answer (role merging, AI-held role, scaled variant)
   before this is a real product.
2. **Scheduled fixed-time events contradict the premise.** The original concept proposes a weekly
   appointment, which is the same friction that pushed these players out of raiding. Likely
   resolution: always-available runs with an *optional* synchronised event.
3. **Interdependent failure plus mixed skill is a blame engine.** "One person's mistake wipes the
   group" can produce resentment rather than laughter. Mitigations to explore later: recoverable
   rather than binary failures, no death-cam that identifies the culprit, and star scenes that don't
   hard-wall when random assignment gives the star role to the weakest player.
4. **Content burn rate.** "Unknown content" with kits that change every event means bespoke design
   per event, and one solved puzzle is spoiled globally. Out of scope entirely for now.
5. **Distribution friction, newly introduced by the engine switch.** Unreal requires a packaged build
   (hundreds of MB to low GB) shared before each playtest, plus a one-time Tailscale install per
   friend — real friction the old browser-URL design explicitly avoided. Mitigate for now by keeping
   the build shared via one stable link (cloud drive folder) and only re-sharing when it changes;
   revisit if this is still annoying friends by Build 2.
6. **Tailscale as a shared dependency.** Fine for a closed group of five trusted friends; becomes the
   wrong tool the moment we want to test with people outside that circle. See §3.1's EOS migration
   trigger conditions.

---

## 9. How we know it worked

The success criteria are social, so we measure them socially.

- **Record the Discord audio for every playtest.**
- Check **talk time per player**. If one of the five is quiet through a whole run, that scene failed
  for that role — regardless of whether the team cleared it.
- Other signals: every player has at least one moment where the group depended on them; synergy
  activations feel satisfying; the boss creates controlled chaos; after a wipe the group immediately
  wants another attempt.

Clear rate is **not** a success metric. Talking is.

---

## 10. Working conventions

### Repo layout

```
/
├── CLAUDE.md                        ← this file (constitution)
├── DECISIONS.md                     ← case law, updated every session
├── docs/
│   ├── abilities.md                 ← abilities, tags, synergy resolution
│   ├── scenes/                      ← one file per scene, why it works the way it does
│   └── *.pdf                        ← original design briefs
├── Unreal_first_Game.uproject
├── Config/                          ← DefaultEngine.ini, DefaultGame.ini — net update rates,
│                                        maps, project settings
├── Source/
│   └── Unreal_first_Game/           ← does not exist yet — created in Build 0 (§7)
│       ├── Unreal_first_Game.Build.cs
│       ├── Core/                    ← GameMode, GameState, PlayerState, PlayerController
│       ├── Tags/                    ← gameplay tag definitions
│       ├── Abilities/               ← one file per role, hardcoded synergy conditionals (§4.6)
│       ├── Scenes/                  ← one file per scene's server-side logic/timeline
│       ├── Dev/                     ← dummy pawns, scene skip, god mode, DumpGameState command
│       └── Camera/                  ← local-only orbit camera actor (§5)
└── Content/
    ├── Blueprints/
    │   ├── Characters/               ← BP wrappers over C++ pawn/character base classes
    │   ├── UI/                       ← UMG widgets
    │   └── Levels/                   ← per-scene levels
    └── Data/
        └── DA_GameConstants          ← every tunable number (replaces shared/constants.ts)
```

`DerivedDataCache/`, `Intermediate/`, and `Saved/` are engine-generated and must stay gitignored.

### Rules

- **Every tunable number lives in the `DA_GameConstants` DataAsset** (`Content/Data/`). No magic
  numbers in C++ or in Blueprint graphs — movement speeds, cooldowns, durations, ranges, HP values,
  arena dimensions, window lengths. This is what makes tuning a Friday-night activity rather than a
  code change, and it's editable both in the Unreal Editor and via `unreal-mcp` (§3.2).
- Small files, named by scene or system.
- Comment *why*, not *what* — especially anything touching authority or timing.
- When adding a mechanic, state explicitly in the code where it resolves (`HasAuthority()`) and what
  tags it reads and writes.
- **Update `DECISIONS.md`** with anything settled during a session that future sessions must
  respect. Do not let knowledge live only in chat history — future sessions cannot see it.
- Prefer deleting code — and unused Blueprints/assets — to commenting out or leaving orphaned
  content. Orphaned `.uasset` files bloat the project and confuse the content browser in a way dead
  code in a text repo doesn't.

### When something desyncs

The predicted failure mode for this project is state divergence that produces no error. Debugging
order:

1. Run the `DumpGameState` command (§4.3) on the server — this is the authoritative truth.
2. Run the same command (or a client-side equivalent) on the disagreeing client.
3. Find the first field that disagrees.
4. Ask: *does this Actor/property have `HasAuthority()` true only on the server?* The answer should
   always be "yes, and only the server wrote it." If a client-side Blueprint node or a
   `CharacterMovementComponent` quirk changed something gameplay-critical, that's the bug — see
   §4.2's boundary between accepted movement prediction and forbidden gameplay prediction.

### Team rule (not a code rule)

One human on this team must genuinely understand the authority model — what state exists, who has
authority over it (`HasAuthority()`/Role/RemoteRole), when it replicates, what clients may decide
locally versus only request via RPC. Roughly a week of study, more given Unreal's larger surface area
(§2). Everything else can be delegated to AI assistance; this cannot, because it is the one thing
that cannot be recovered from a stack trace.

---

## 11. First milestone

> **5 FRIENDS · 5 STAR MOMENTS · 5 SYNERGIES · 1 BOSS · EVERYONE MATTERS**
