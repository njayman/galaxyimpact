# Galaxy Impact — Wiki

A top-down, bullet-heaven space shooter. Survive escalating waves, build out your loadout through level-up picks, and take down bosses that draw from a shared pool of moves. This wiki documents every system in the game as implemented.

## Table of contents

- [Controls](#controls)
- [Ships](#ships)
- [Player](#player)
- [Weapons](#weapons)
- [Skills & leveling up](#skills--leveling-up)
- [Enemies](#enemies)
- [Elite Hazards](#elite-hazards)
- [Bosses](#bosses)
- [Space hazards](#space-hazards)
- [Pickups & loot](#pickups--loot)
- [Waves & difficulty](#waves--difficulty)
- [Nerve](#nerve)
- [Damage meter](#damage-meter)
- [Music](#music)
- [Settings](#settings)
- [Sandbox mode](#sandbox-mode)
- [Score & high scores](#score--high-scores)

---

## Controls

| Action | Input |
|---|---|
| Move | `W` `A` `S` `D` |
| Aim (Forward Shot / Beam Sweep / Overdrive Burst) | Mouse position |
| Dash | Left Click |
| Shield | Right Click |
| Overdrive Burst | Hold `Space` (once Nerve is full) |
| Pause | `Esc` |
| Fullscreen toggle | `F11` |
| Menu navigate | `W`/`S`/`Up`/`Down`, or mouse hover |
| Menu confirm | `Enter` / `Space`, or Left Click |
| Settings adjust | `A`/`D`/`Left`/`Right` |

Movement and aiming are independent — you walk in one direction while your Forward Shot, Beam Sweep, and Overdrive Burst all fire toward the mouse cursor. The ship is camera-locked to screen center; the world scrolls under you.

---

## Ships

Picking "Start" from the title screen (or "New Game" from the pause or game-over menus) opens a ship-select screen before the run begins. Each ship has its own stats, dash behavior, starting weapon, a passive weapon specialty, and its own [Overdrive Burst](#nerve). Your choice persists between runs (saved to disk) until you pick a different one.

| Ship | HP | Armor | Damage | Shield Stacks | Dash | Starting weapon | Specialty |
|---|---|---|---|---|---|---|---|
| **Bastion** | 8 | 2 | ×0.8 | 4 | Shorter reach; shoves enemies/bosses aside instead of damaging them | Orbit Blades | Orbit blades spin 2.2× faster |
| **Ranger** | 5 | 1 | ×1.0 | 3 | Normal reach; damages *and* shoves (half of each) | Forward Shot | Bullets fly 1.4× faster |
| **Interceptor** | 4 | 0 | ×1.3 | 2 | Longer reach; damages everything in its path | Beam Sweep | Laser reaches 1.5× further |

- **Armor** flatly reduces incoming damage (always chips at least 1 HP — it can't grant full immunity).
- **Damage** multiplies all weapon damage output (Forward Shot, Orbit Blades, Homing Missiles, Mines, Beam Sweep, Shockwave alike).
- **Shield Stacks** cap replaces a single fixed number — see [Player](#player).
- **Dash** behavior ties into the shared dash mechanic below; see [Player → Dash](#player) for how the push/damage/hybrid quirks work in combat.
- Each ship also has its own distinct hull silhouette (Bastion: wide hexagon hull with wing pods; Ranger: the classic triangle plus a diamond plate; Interceptor: a slim dart with swept fins) and color.

---

## Player

- **Health**: shown as a single bar rather than discrete hearts. Max HP, armor, and starting stats all come from your chosen [ship](#ships); Life Orbs and the Hull Plating skill add to it directly, so partial HP is normal and visible, not rounded to whole hearts.
- **Shield** (Right Click): a temporary invulnerability bubble. Costs one charge. While active, it blocks all contact and beam damage. If you're standing in a boss beam when your shield expires, you take one heavy hit instead of the beam's usual gradual chip damage — the shield buys you time to move, not a way to facetank the whole attack.
- **Dash** (Left Click): a short burst of speed in your aim direction, whose distance is scaled by your ship (Bastion shorter, Interceptor longer). Costs one charge. While dashing you have contact immunity, and what happens to anything you plow through depends on your ship's dash quirk:
  - **Damage** (Interceptor): full damage to every enemy/boss you pass through, once per target per dash.
  - **Push** (Bastion): no damage — instead shoves enemies and bosses away from you.
  - **Hybrid** (Ranger): half damage plus a smaller shove.
  - Dashing while your Shield is also active still turns every enemy you plow through into an instant kill, regardless of ship.
  - **Dash-refund-on-kill**: killing an enemy mid-dash knocks time off your charge-regen timer, rewarding chaining dashes through a crowd instead of a single hit-and-run.
- **Charges**: a shared pool of 2, used by both Dash and Shield. Regenerate over time (faster with the Barrier Mastery skill), and further sped up by [Charge Feed](#nerve) whenever you're missing one and have Nerve banked.
- **Shield Stacks**: a separate resource from the Shield ability — rare drops that each absorb exactly one hit (from any source) with no cooldown. Shown as small ring pips in the HUD; the cap is ship-dependent (2/3/4 — see [Ships](#ships)) rather than a fixed number.
- **Life Orbs**: each orb collected heals +0.5 HP directly (capped at max HP) — no more two-orbs-per-heal-point pairing, since HP is now a continuous bar.
- **Arena boundary**: the play area is a large circle (radius 20,000 units) — you're clamped to it, though in practice it reads as effectively open.

---

## Weapons

You start with your [ship](#ships)'s default weapon at level 1 (Bastion: Orbit Blades, Ranger: Forward Shot, Interceptor: Beam Sweep). Level-up picks grant new weapons or level up ones you already have. Most weapons auto-fire on their own cooldown; Orbit Blades and Beam Sweep are always active instead (see below) — there's no manual fire button for anything.

| Weapon | Behavior | Notes |
|---|---|---|
| **Forward Shot** | Fires toward your cursor. | Levels add extra simultaneous shots in a spread; bullet speed is ship-dependent (Ranger's fly 1.4× faster). |
| **Orbit Blades** | Always-spinning blades that damage on contact, plus periodic ranged shots. | See [Orbit Blades](#orbit-blades) below. |
| **Homing Missiles** | Auto-fires at the nearest enemy. | Fire-and-forget; missiles retarget in flight. |
| **Mine Layer** | Drops mines that scatter near you. | See [Mines](#mines) below — base and evolved mines behave differently. |
| **Beam Sweep** | An always-on piercing laser toward your cursor. | See [Beam Sweep](#beam-sweep) below. |
| **Shockwave** | A slow, heavy-hitting pulse around you. | Evolved version also heals 1 HP per pulse. |

Every weapon's cooldown shortens with level and with the Overclock skill, and evolved weapons fire 20% faster and hit harder. Standing near an Elite Hazard **Suppressor** applies a cooldown penalty to all your weapons for as long as you're in its aura.

### Orbit Blades

Unlike other weapons, Orbit Blades don't fire on a cooldown — they're always active. A ring of blades (count grows with level) spins around you, and anything a blade actually touches takes damage: first contact lands an instant hit, and staying in contact ticks extra damage per second on top of that. This now damages bosses too, which it never used to.

Separately, roughly every 2.2 seconds (faster with the Cooldown skill), one or more blades launch off the ring toward your cursor as a piercing shot that punches through everything in its path — enemies, elite hazards, bosses, asteroids — until it leaves the camera's view. The number launched at once scales with how many blades you currently have, and each launches from wherever that blade actually sits on the ring (not the ship's center), with a replacement blade visibly growing back out from the ship to refill the vacated slot.

### Beam Sweep

A continuous laser toward your cursor rather than a periodic pulse. Like Orbit Blades, anything it touches takes an instant hit on first contact, then continues taking damage per second for as long as it stays in the beam. Length grows with level (and 1.5× further on Interceptor). Its per-target damage is intentionally much lower than a single-target weapon's, since a full-length beam can be touching an entire crowd at once — full single-target damage per target there would delete crowds instantly.

### Mines

- **Base mines**: mostly sit where they were dropped. They only home toward *enemies* (never asteroids) and only detonate once they're right on top of one — they're a "wait for something to wander close" tool, not an aggressive area-denial one.
- **Evolved mines**: seek both enemies and asteroids, and detonate across their full blast radius.

### Weapon evolution

Once a weapon and its linked passive skill both reach level 3, an **Evolve** choice can appear on the level-up screen. Evolving fuses the weapon and its passive into a single super-weapon slot with a unique name and stronger, flashier effects:

| Weapon | Evolved name |
|---|---|
| Forward Shot | Photon Cannon |
| Orbit Blades | Aegis Ring |
| Homing Missiles | Seeker Swarm |
| Mine Layer | Cluster Charges |
| Beam Sweep | Lance Sweep |
| Shockwave | Bulwark Pulse |

---

## Skills & leveling up

Killing enemies drops XP; filling the XP bar levels you up and opens the level-up picker (gameplay pauses while you choose). You're offered up to 3 skill picks, plus a guaranteed 4th **Evolve** option whenever any weapon is eligible.

There are 12 skills, split into 6 weapon-grant skills (leveling one grants/levels the matching weapon) and 6 linked passives:

| Weapon skill | Linked passive | Passive effect |
|---|---|---|
| Forward Shot | Warhead Tuning | +15%/level damage, all weapons (max level 5) |
| Orbit Blades | Barrier Mastery | +shield duration, faster charge regen (max level 3) |
| Homing Missiles | Overclock | −10%/level weapon cooldowns (max level 5) |
| Mine Layer | Tractor Beam | +20%/level pickup magnet radius (max level 5) |
| Beam Sweep | Thrusters | +10%/level move speed (max level 5) |
| Shockwave | Hull Plating | +1 max health per level, fully healed (max level 5) |

All 6 weapon-grant skills max at level 8.

**Ability slots**: only 6 skills (weapon or passive) can be "equipped" at once — shown as icons in the HUD. A passive that's been fused into an evolved weapon no longer counts against this limit; new skill offers are limited to ones you already own once all 6 slots are full. Once every slot is full *and* every owned skill is maxed, the level-up picker instead offers a straight Life Orb or Shield reward (1/2/3 of either), plus a small permanent damage bonus that stacks every time this happens — so power progression never fully caps out even after your build is "complete."

---

## Enemies

17 enemy kinds unlock progressively by wave number, each following one of 7 movement patterns:

| Pattern | Behavior |
|---|---|
| Chase | Beelines straight at you |
| Zigzag | Chases with a wobbling side-to-side weave |
| Charge | Telegraphs, then dashes in a straight line |
| Orbit | Circles you, slowly spiraling inward |
| Turret | Stationary, fires projectiles at range |
| Spawner | Stationary (or slow), periodically spawns more enemies |
| Stationary | Doesn't move — a pure contact hazard |

A random 6% of spawns roll as **Elite** — double health, faster, and worth double score. Special kind traits include splitting on death, exploding on death, phasing in and out of tangibility, and leeching (slows and heavily damages you on contact but heavily damages itself when it does).

All enemy stats (health, contact damage) scale up with wave number and with the difficulty setting.

---

## Elite Hazards

A rare, tough field hazard distinct from the regular enemy roster — capped at 1/2/3 concurrent depending on difficulty (Easy/Normal/Hard), spawning on a long random timer (45–90s).

- **Doesn't chase.** It holds a position near the edge of your screen and slowly orbits, drifting to follow you rather than closing in — always visible at the periphery, applying pressure without engaging directly.
- **Two roles**, rolled at spawn:
  - **Warlord**: buffs the speed of regular enemies within its aura.
  - **Suppressor**: applies a weapon cooldown penalty to you while you're within its aura.
- **HP**: sits strictly between a regular enemy and a miniboss — a real fight, not a throwaway kill.
- **Reward**: killing one guarantees a Shield or Life Orb drop plus a score bonus.
- Still deals contact damage if you touch it, and can be shot down with your weapons like anything else.

---

## Bosses

### Cadence

- A **miniboss** appears every 5 waves.
- A **mega boss** appears every 10 waves (supersedes the miniboss on waves divisible by both).
- Every **50th boss spawn** (mini or mega, counted together) is a **swarm of 3** bosses at once instead of 1.

### 20 types, one shared move pool

Bosses come in 20 flavors (name, color, and health/size multipliers) — purely cosmetic variation. What actually makes each fight different is its **moveset**: at spawn, every boss randomly samples a handful of moves from one shared pool of 11, sized by difficulty (Easy: 2 moves, Normal: 3, Hard: 4). This means fight variety comes from combinatorics, not hand-authored movesets, and any move added to the pool in the future enriches every boss type for free.

### The move pool

| Move | Effect |
|---|---|
| **Beam** | A long, telegraphed beam. See below for its full mechanic. |
| **Spread** | Multiple rounds of a fan burst — 10/30/60 total bullets by Easy/Normal/Hard, each round re-aimed at your current position. |
| **Slam** | A screen-covering expanding shockwave from the boss's position. |
| **WormholeBeam** | Summons a temporary wormhole and fires its beam from a flanking point near you instead of from itself — same beam mechanic, unpredictable origin. |
| **MineDrop** | Drops 4 stationary mines around itself that sit and threaten anyone who touches them. |
| **ChargeDash** | Telegraphs, then dashes edge-to-edge across the arena at high speed. |
| **SummonAdds** | Spawns 2/3/4 regular enemies by Easy/Normal/Hard. |
| **ShockwaveStomp** | An instant area pulse around the boss. |
| **Barrage** | Sustained rapid-fire straight shots at your current position, at a speed matching your own move speed so a direct retreat can't just outrun it — not homing, so moving off-line still dodges it. |
| **GravityWell** | Pulls you steadily toward the boss for several seconds. |
| **HomingBarrage** | Sustained rapid-fire *homing* shots — continuously re-aims in flight, so it can't be outrun the way Barrage can, but it's dodgeable by breaking line of sight or juking at close range. |

**Boss attacks always destroy** every asteroid and enemy they touch (Beam, Slam, ShockwaveStomp) — they don't stop for obstacles, and there's no line-of-sight blocking; an asteroid in a beam's path gets destroyed but doesn't shield you from it. On Normal/Hard, enemies killed this way (not by your own damage) drop no score/XP/pickups — only Easy difficulty and the boss's own death shockwave pay out loot for kills of opportunity. Boss-fired Spread/Barrage/HomingBarrage projectiles likewise pass through and destroy anything they touch rather than being stopped by it — the only way to stop one is sustained direct fire (they have their own small HP pool, scaled by wave and difficulty).

### Beam mechanic

The Beam attack telegraphs with a slow-blinking ring on *your* position (not a line — you know a beam is coming, not its exact trajectory) during windup, then fires for several seconds. Get out of it, or shield through it — but if you shield and then the shield expires while you're still standing in the beam, you take one heavy hit instead of the usual small chip damage per second. It's a "shield buys time to move" mechanic, not a way to stand still and tank it.

### Enrage

Once a boss drops below 25% health, its idle cooldown and attack telegraphs speed up — it gets noticeably more aggressive in its last stretch.

### Boss ramming

Physically colliding with a boss deals heavy damage to both sides — risky if you're caught off guard, but a viable (if costly) source of extra damage.

---

## Space hazards

### Asteroids

Spawn in tiers (Large → Medium → Small); destroying a non-small asteroid shatters it into 3 smaller ones flying outward. Locked out entirely on Easy until wave 10. Spawn rate and cap both scale with difficulty.

### Black holes

Spawn periodically near you and pull in anything within their influence radius — player, asteroids, and loot alike. Anything that reaches the core is destroyed. **Loot pulled into a black hole is destroyed, not collected** — it's a real strategic cost to fighting near one.

### Wormholes

A linked pair of portal mouths that spawn/despawn on a timer like black holes. Each mouth independently faces one of 4 cardinal directions. Anything that touches a mouth — you, bullets, boss projectiles, enemies — is teleported out the other mouth, with its velocity rotated to match the facing difference between the two ends. It's a real portal transform (a beam entering one mouth exits the other still moving, redirected), not a blink-teleport.

---

## Pickups & loot

| Pickup | Effect | Lifetime |
|---|---|---|
| XP orb | Adds to your XP bar | 10s |
| Shield | +1 shield stack (cap is ship-dependent — see [Ships](#ships)) | 18s |
| Life Orb | +0.5 max-health-capped healing | 18s |

All pickups magnet toward you within a radius (increased by the Tractor Beam skill) and **expire if left uncollected** — they blink in their last 2.5 seconds as a despawn warning. Anything that drifts into a black hole is destroyed instead of collected.

---

## Waves & difficulty

- A new wave begins every 25 seconds.
- **Enemy spawn rate** cycles in blocks of 10 waves: the first 5 waves of each block spawn at a steady rate, the last 5 spawn faster. The baseline rate itself also ratchets up once per 10-wave block, so the game keeps escalating indefinitely.
- **Difficulty** (Easy / Normal / Hard) scales enemy health, enemy damage, and spawn rate, and gates some content (asteroids on Easy, Elite Hazard cap, boss moveset size).

---

## Nerve

Nerve (0–100) fills +6 per kill. Unlike its old design, it no longer decays on its own and no longer grants a passive damage/speed buff — instead it's spent on two concrete effects:

### Charge Feed (passive)

Whenever you're missing a Dash/Shield charge and have any Nerve banked, it starts draining after a short 0.6s delay (so a momentary charge dip doesn't instantly start feeding) at ~12/sec, tripling the speed of that charge's regeneration. It stops the instant the charge is full again or Nerve hits 0. The charge pip being fed visibly glows differently while this is active. This replaces the old "free emergency dash/shield at 70% Nerve" backdoor with something continuous and visible.

### Overdrive Burst (active)

Once Nerve reaches 100 it stops draining and visibly pulses to signal it's ready, with a flashing `SPACE` hint near the ability slots. Hold `Space` to begin a ~0.35s charge windup (your ship glows, a rising-pitch tone plays); it fires automatically the moment the windup completes, consuming the full meter. Taking a hit while charging cancels it instead — a descending fizzle tone plays and Nerve drops to 0. This is now the *only* way a hit resets Nerve; taking damage while not mid-charge no longer touches it at all.

The burst itself is ship-specific:

| Ship | Burst |
|---|---|
| **Bastion** | A ring of blades (count matches your current Orbit Blade count) spins around a center that travels forward in your aim direction like a thrown top — pierces everything along its path, including bosses. |
| **Ranger** | A large ball thrown forward that pierces everything in its way as it travels. |
| **Interceptor** | An instant long-range piercing beam toward your cursor. |

The Bastion spiral and Ranger ball both keep travelling in whatever direction you were aiming the instant you released — they don't re-steer if you move the mouse afterward — and persist until they actually leave the camera's view rather than expiring on a fixed timer or range.

---

## Damage meter

A live readout next to the Nerve bar showing your outgoing damage, broken down by source (Forward / Orbit / Homing / Mine / Beam / Shock / Dash / Nerve). Defaults to `DMG: 0`. A hit refreshes the reading and holds it for a short moment before it decays back to 0, instead of flickering blank between frames.

---

## Music

Three synchronized, seamlessly-looping synthesized music layers, mixed by volume alone so they always stay harmonically locked:

- **Drone** (always audible): a warm, slow A-root chord — root, true octave, and a gently detuned unison for chorus — plus one quiet, higher dissonant color tone for an "eerie" edge without ever beating against the bass.
- **Intensity** (fades in with enemy count): a rhythmic pulse on the drone's perfect fifth.
- **Upgrade** (fades in, and stays, once you get your first weapon upgrade): a bright plucked arpeggio built on the same root/fifth/octave, elevated two octaves — same harmonic DNA as the drone, reading as a reward layered on top.

Killing a boss briefly calms the intensity layer for a few seconds, unless enemies have already swarmed back to a heavy count, in which case the calm is cut short.

---

## Settings

Accessible from the title screen or the pause menu.

| Setting | Options |
|---|---|
| Resolution | 1280×720, 1600×900, 1920×1080, 2560×1440, 3840×2160 |
| Difficulty | Easy / Normal / Hard |
| BGM | On / Off |
| Sound | On / Off |
| FPS Cap | 60 / 120 / 240 |
| Display Mode | Windowed / Fullscreen |

All gameplay logic is frame-rate independent (delta-time scaled, not frame-counted), so raising the FPS cap is purely a smoothness/input-latency choice — it doesn't change simulation speed. Settings persist to disk between sessions (except display mode, which always starts windowed).

---

## Sandbox mode

Launch with `-sandbox` (or `--sandbox`) for a clean, empty arena with no waves or auto-spawns — built for testing.

| Key | Action |
|---|---|
| `[` / `]` | Cycle the enemy kind to spawn |
| `E` | Spawn one of the selected enemy kind |
| `B` (hold `Shift` for miniboss) | Spawn a boss |
| `Y` | Spawn a 3-boss swarm |
| `K` | Clear the board (enemies, asteroids, projectiles, mines, bosses) |
| `L` | Open the level-up picker on demand |
| `H` | Full heal + max shield stacks + max Nerve |
| `R` | Reset weapons/skills back to the current ship's starting weapon |
| `I` / `Shift+I` | Cycle ship forward/backward (applies immediately via a full reset) |
| `G` | Toggle death (god mode) — off by default, so damage doesn't matter until you turn it on |
| `,` / `.` | Cycle the boss attack to command |
| `O` | Force the nearest boss to immediately wind up and use the selected attack |
| `N` | Spawn a black hole |
| `M` | Spawn a wormhole pair |
| `U` (hold `Shift` for Suppressor) | Spawn an Elite Hazard (Warlord by default) |
| `=` / `-` | Increase/decrease the wave number |

Current sandbox state (selected enemy kind, god-mode on/off, selected boss attack) is always shown at the bottom of the screen.

---

## Score & high scores

Score comes from kills (enemies, bosses, asteroids) and accumulates for the run. On death, your run's score is compared against the top 5 all-time high scores (persisted to disk) and inserted if it qualifies. Shown on the title screen and game-over screen.
