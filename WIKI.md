# Galaxy Impact — Wiki

A top-down, bullet-heaven space shooter. Survive escalating waves, build out your loadout through level-up picks, and take down bosses that draw from a shared pool of moves. This wiki documents every system in the game as implemented.

## Table of contents

- [Controls](#controls)
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
| Aim (Forward Shot / Beam Sweep) | Mouse position |
| Dash | Left Click |
| Shield | Right Click |
| Pause | `Esc` |
| Fullscreen toggle | `F11` |
| Menu navigate | `W`/`S`/`Up`/`Down`, or mouse hover |
| Menu confirm | `Enter` / `Space`, or Left Click |
| Settings adjust | `A`/`D`/`Left`/`Right` |

Movement and aiming are independent — you walk in one direction while your Forward Shot and Beam Sweep both fire toward the mouse cursor. The ship is camera-locked to screen center; the world scrolls under you.

---

## Player

- **Health**: shown as a row of ship-icon "lives." Starts at 5.
- **Shield** (Right Click): a temporary invulnerability bubble. Costs one charge. While active, it blocks all contact and beam damage. If you're standing in a boss beam when your shield expires, you take one heavy hit instead of the beam's usual gradual chip damage — the shield buys you time to move, not a way to facetank the whole attack.
- **Dash** (Left Click): a short burst of speed in your aim direction. Costs one charge. While dashing you have contact immunity and plow through enemies, dealing damage to each one you pass through (once per enemy per dash). Dashing while your Shield is also active turns every enemy you plow through into an instant kill.
  - **Dash-refund-on-kill**: killing an enemy mid-dash knocks time off your charge-regen timer, rewarding chaining dashes through a crowd instead of a single hit-and-run.
- **Charges**: a shared pool of 2, used by both Dash and Shield. Regenerate over time (faster with the Barrier Mastery skill, and further sped up by kills via Nerve).
- **Shield Stacks** (0–3): a separate resource from the Shield ability — rare drops that each absorb exactly one hit (from any source) with no cooldown. Shown as small ring pips in the HUD.
- **Life Orbs**: two orbs collected convert into +1 max health (fully healed). A single collected orb shows as a faint half-filled health icon until the second arrives.
- **Arena boundary**: the play area is a large circle (radius 20,000 units) — you're clamped to it, though in practice it reads as effectively open.

---

## Weapons

You start with **Forward Shot** at level 1. Level-up picks grant new weapons or level up ones you already have. All weapons auto-fire on their own cooldown — there's no manual fire button.

| Weapon | Behavior | Notes |
|---|---|---|
| **Forward Shot** | Fires toward your cursor. | Levels add extra simultaneous shots in a spread. |
| **Orbit Blades** | Periodic damage pulse to anything circling you. | Radius grows with level. |
| **Homing Missiles** | Auto-fires at the nearest enemy. | Fire-and-forget; missiles retarget in flight. |
| **Mine Layer** | Drops mines that scatter near you. | See [Mines](#mines) below — base and evolved mines behave differently. |
| **Beam Sweep** | Fires a piercing beam toward your cursor. | Length grows with level. |
| **Shockwave** | A slow, heavy-hitting pulse around you. | Evolved version also heals 1 HP per pulse. |

Every weapon's cooldown shortens with level and with the Overclock skill, and evolved weapons fire 20% faster and hit harder. Standing near an Elite Hazard **Suppressor** applies a cooldown penalty to all your weapons for as long as you're in its aura.

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
| Sprawner | Stationary (or slow), periodically spawns more enemies |
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
| **Homing** | Launches 3 homing projectiles. |
| **Spread** | A fan burst of projectiles; windup time before the burst adds more shots (up to +12). |
| **Slam** | A screen-covering expanding shockwave from the boss's position. |
| **WormholeBeam** | Summons a temporary wormhole and fires its beam from a flanking point near you instead of from itself — same beam mechanic, unpredictable origin. |
| **MineDrop** | Drops 4 stationary mines around itself that sit and threaten anyone who touches them. |
| **ChargeDash** | Telegraphs, then rams a straight line through the arena at high speed. |
| **SummonAdds** | Spawns 2 regular enemies. |
| **ShockwaveStomp** | An instant area pulse around the boss. |
| **Barrage** | Sustained rapid-fire single shots at you for several seconds. |
| **GravityWell** | Pulls you steadily toward the boss for several seconds. |

**Boss attacks always destroy** every asteroid and enemy they touch (Beam, Slam, ShockwaveStomp) — they don't stop for obstacles, and there's no line-of-sight blocking; an asteroid in a beam's path gets destroyed but doesn't shield you from it. Boss-fired Homing/Spread/Barrage projectiles likewise pass through and destroy anything they touch rather than being stopped by it — the only way to stop one is sustained direct fire (they have their own small HP pool, scaled by wave and difficulty).

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
| Shield | +1 shield stack (max 3) | 18s |
| Life Orb | Half of a max-health increase (2 orbs = +1 max HP) | 18s |

All pickups magnet toward you within a radius (increased by the Tractor Beam skill) and **expire if left uncollected** — they blink in their last 2.5 seconds as a despawn warning. Anything that drifts into a black hole is destroyed instead of collected.

---

## Waves & difficulty

- A new wave begins every 25 seconds.
- **Enemy spawn rate** cycles in blocks of 10 waves: the first 5 waves of each block spawn at a steady rate, the last 5 spawn faster. The baseline rate itself also ratchets up once per 10-wave block, so the game keeps escalating indefinitely.
- **Difficulty** (Easy / Normal / Hard) scales enemy health, enemy damage, and spawn rate, and gates some content (asteroids on Easy, Elite Hazard cap, boss moveset size).

---

## Nerve

An aggression meter (0–100) that climbs 6 points per kill and bleeds away on its own over time — it doesn't bank indefinitely if you idle. Getting hit resets it to zero immediately. While high, Nerve grants:

- Up to **+50% weapon damage**
- Up to **+20% move speed**

The "hesitation is defeat" identity: staying aggressive and landing kills keeps your damage and speed climbing, but one mistake (getting hit) wipes it out — not just the HP cost, but the momentum too.

---

## Damage meter

A live readout next to the Nerve bar showing **this frame's outgoing damage only** — recalculated fresh every frame, reading blank when nothing was hit. When you deal damage, it shows the total plus a breakdown by source (Forward / Orbit / Homing / Mine / Beam / Shock / Dash), so you can directly watch how Nerve's damage bonus affects your numbers in real time.

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
| `B` | Spawn a boss |
| `K` | Clear the board (enemies, asteroids, projectiles, mines, bosses) |
| `L` | Open the level-up picker on demand |
| `H` | Full heal + max shield stacks + max Nerve |
| `R` | Reset weapons/skills back to starting Forward Shot only |

---

## Score & high scores

Score comes from kills (enemies, bosses, asteroids) and accumulates for the run. On death, your run's score is compared against the top 5 all-time high scores (persisted to disk) and inserted if it qualifies. Shown on the title screen and game-over screen.
