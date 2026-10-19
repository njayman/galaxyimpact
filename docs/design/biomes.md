# Biome design spec

The design for each Biome of Galaxy Impact, detailed enough to build one Biome at a time. Terms follow `GLOSSARY.md`. Planned on the map [Map: biome identity rework](https://github.com/njayman/galaxyimpact/issues/1).

## How this spec works

- Each Biome section carries a **Status**: Draft, or Approved with a date. An Approved section changes only through a new ticket on the map.
- The spec is done when all five Biome sections are Approved.
- Numbers are starting values; the build may tune them.
- Visuals are described in words plus palette hex values; there are no art assets. Reworked items name their current code identifier.
- Recommended build order: Shattered Belt, Rustbloom, Solar Forge, Punctum, Banished realm, shipping one Biome fully before the next. The build effort makes the final call.

## Shared rules

### Scope

From [Biome rework scope: what is open to change](https://github.com/njayman/galaxyimpact/issues/2).

- The four Biomes keep their names, 25-wave stretches and order. The five Biome Bosses keep their names, Biomes and spawn waves.
- Inside a Biome every layer is open: Native Enemy look and behaviour, background, Biome Hazard, Anomaly variant, Wave Boss flavour, Biome Boss moves and attacks, Boss QTE.
- Native Enemies are reworked first. A Biome gets a brand-new Native Enemy only when its design needs a combat role none of its current ones fill, at most one per Biome. An enemy is cut only when a new one replaces it.
- Global Enemies keep their behaviour everywhere and may take the current Biome's tint. Native Enemies follow their Biome's palette.
- Wave timing, spawn pacing and boss wave numbers are fixed. HP, damage and speed of anything reworked are open.
- Ships, weapons, skills, Nerve and controls are not changed.

### Palette

- Danger red `#C43A3A` is shared by every Biome so threats always read the same (from [Rustbloom: theme, Native Enemies and background](https://github.com/njayman/galaxyimpact/issues/11)).

### Boss QTE

From [Shared Boss QTE rules](https://github.com/njayman/galaxyimpact/issues/3).

- **Cutscene contract:** HUD hidden with letterbox bars, world at 4% speed, player immune. One Prompt at a time from the shared input set: four directions, Shield, Dash, Parry (shield + dash), Nerve, and Mash. A wrong or late answer ends the cutscene and the threat lands live, still dodgeable. Only the threat and its choreography differ per Biome Boss.
- **Trigger:** a beat the Biome Boss already has and that recurs. Skipped while the player is grabbed or launched, or another cutscene runs. At least 20 s between Boss QTEs.
- **Flawless clear:** a 3 s Punish Window; the boss is dazed, cannot attack and takes +50% damage.
- **Fail:** the live hit only; damage already dealt sticks.
- **Difficulty by Biome order** (Prompt window / Prompt count / Parry share), exact values per Biome Boss:

  | Biome Boss  | Window | Prompts | Parry |
  | ----------- | ------ | ------- | ----- |
  | Beltbreaker | 0.9 s  | 8       | 15%   |
  | Wreckworm   | 0.8 s  | 9       | 20%   |
  | Slagmaw     | 0.7 s  | 10      | 25%   |
  | Kraken      | 0.6 s  | 12      | 30%   |

- **Mash** is used only where a grip or struggle fits the fiction.

### Biome Hazards

From [One signature Biome Hazard per Biome](https://github.com/njayman/galaxyimpact/issues/7).

- Every Biome has exactly one signature Biome Hazard.
- It applies movement pressure and only hurts when its tell is ignored: every damaging moment is telegraphed at least 1 s ahead and avoidable by moving. It never instakills, and it ramps gently across the Biome's 25 waves. Always-on or event-driven is per Biome.
- It pauses during a Boss QTE. During the rest of a Biome Boss fight it stays on unless that Biome Boss's section suppresses or weaponises it.
- A HUD element appears only when the hazard has hidden state (like the heat meter); otherwise all tells are in-world.

### Anomalies

From [Anomaly variants per Biome](https://github.com/njayman/galaxyimpact/issues/5).

- Both Anomalies (black hole, wormhole pair) take the current Biome's palette everywhere.
- Each Biome adds one behaviour twist to one featured Anomaly: its Anomaly Variant.
- An Anomaly Variant's twist stays a property of that object; it never changes a Biome-wide meter or state (that is a Biome Hazard's job).
- Spawn timing, lifetime and pull strength are identical in every Biome; a twist never makes Anomalies more frequent.

### Wave Bosses and Elite Hazards

From [Wave Boss flavour per Biome](https://github.com/njayman/galaxyimpact/issues/6).

- Each Biome draws its Wave Bosses from a themed pool of 4-5 of the existing 20 Wave Boss types, tinted with the Biome palette. The pool is named in that Biome's section.
- The moveset stays the shared random draw of 4 from the 9 generic attacks. No Biome-themed Wave Boss attacks: themed attacks belong to Biome Bosses.
- Wave Bosses never borrow a Biome Boss's silhouette, colour or attacks.
- Fact: in waves 1-100 only mini Wave Bosses appear (waves 5, 15, 35, 45, 55, 65, 85, 95); every 10th wave is a Biome Boss wave and the swarm trigger is effectively unreachable. Left as is (pacing locked).
- Elite Hazards (Warlord, Suppressor) stay global in behaviour and take the Biome tint.

## Biome section template

Every Biome section uses these headings in this order:

1. **Status**: Draft or Approved (date).
2. **Theme and palette**: one paragraph of fiction; hex for base, accent, danger, background.
3. **Native Enemies**: table of name, keep / rework / new, role, look, behaviour, counterplay, starting numbers.
4. **Global Enemy tint**
5. **Background**: layers, motion, parallax.
6. **Biome Hazard**
7. **Anomaly variant**
8. **Wave Boss flavour**
9. **Biome Boss**: phase list; attack table of name, tell, effect, counter, numbers.
10. **Boss QTE**: trigger beat, threat, choreography, Prompt window / count / mix, Mash use, Punish Window, fail outcome.
11. **Open for build**: what is deliberately left to tuning.

## Shattered Belt

**Status:** Approved, 2026-10-19. Waves 1-25, Biome Boss Beltbreaker.

### Theme and palette

The broken ring of a shattered planet: cold stone, amber dust lanes, the planet's split husk looming far behind. The opening Biome, where the basics are taught.

| Role       | Hex       | Note                                   |
| ---------- | --------- | -------------------------------------- |
| Base       | `#384254` | stone (`Palette::StructMid`)           |
| Accent     | `#D9A55B` | amber dust                             |
| Danger     | `#C43A3A` | (`Palette::Accent`)                    |
| Background | `#0A0C12` | void (`Palette::Void`)                 |

### Native Enemies

No new enemy: every role is covered and this Biome teaches the basics.

| Name | Status | Role | Look | Behaviour | Counterplay | Starting numbers |
| --- | --- | --- | --- | --- | --- | --- |
| `Rockhide` | rework | tank chaser, teaches dashing | boulder with plates; cracks glow amber during the tell | chases; every ~4 s a roll charge after a 0.6 s tell | dash sideways out of the roll line | 36 HP, 3 contact; roll 2.5x speed over 220 units, 3 contact |
| `Rock Debris` | keep (recolour) | stationary fodder | palette stone | stationary | shoot | unchanged |
| `Rubble Splitter` | keep (recolour) | chaser that splits | palette stone | chases, splits into 2 Rock Debris on death | kill away from your path | unchanged |
| `Ricochet Strafer` | keep | turret | brown/amber (already fits) | shots bounce off asteroids | watch rebound angles, use asteroids as cover with care | unchanged |
| `Beltcrawler` | keep (recolour) | orbiter | shield-blue to amber | orbits the nearest asteroid | catch it on the open side of the orbit | unchanged |

### Global Enemy tint

Light amber dust tint, about 15%.

### Background

Three layers, far to near:

- **Far** (parallax 0.05): the planet's huge split husk with a broken ring arc, dark silhouette with amber rim light.
- **Mid** (parallax 0.3): faint diagonal amber dust lanes drifting slowly. Debris Squalls arrive along the lane direction, so the background foreshadows them.
- **Near** (parallax 0.8): small drifting rock specks.

Described in words only; looks the build finds open can become a prototype ticket.

### Biome Hazard

**Debris Squall** (event): a few times per wave an edge warning shows, then a band of fast rocks sweeps across the field in one direction; move out of the band. Damages enemies too, so they can be lured in. Switched off during Beltbreaker fights.

### Anomaly variant

**Grinder** (black hole): loose rock caught in its pull orbits it as contact-damage shrapnel; shooting the orbit breaks it up. Details may be refined in this Biome's tickets.

### Wave Boss flavour

Mini Wave Bosses at waves 5 and 15. Pool: Shatter Prime, Fracture Queen, Ashen Sentinel, Barrier Colossus, Warbringer (stone and fracture names; Structural Wyrm left out as a Wreckworm echo).

### Biome Boss: Beltbreaker

Waves 10, 20, 25 with 4, 6, 8 plates. Skeleton kept: a core ringed by rotating plates that act as breakable armour (220 HP per tier each); fewer attached plates speed the core up (down to 0.4x attack interval); dashes deal 2.5x to the open core; generic enrage below 25% HP.

**Phases**

1. **Plated:** plates orbit and occasionally leave on harass excursions; the core attacks from the table below.
2. **Shield charge:** attached plates charge the core's shield over ~8 s (faster with more plates).
3. **Shield complete:** plates burst away and the Boss QTE starts. Clean clear: shield shatters into the Punish Window. Fail: the core stays shielded for its full duration (12 s + 2 s per plate above 4) while plates harass, then plates return.

**Attacks**

| Attack | Tell | Effect | Counter | Starting numbers |
| --- | --- | --- | --- | --- |
| Rubble Barrage (replaces `Barrage`) | core flashes amber 0.5 s | fan of stone shards, Barrage behaviour with Belt look | strafe through gaps | as `Barrage` |
| Ring Crush (replaces `GravityWell`) | plates contract tight round the core 0.8 s | plates fling outward as an expanding ring, then return | hug the core, stay far out, or dash through a broken plate's gap | ring speed 6, 2 damage per plate |
| Plate Hurl (`PlateHurl`, kept) | plate glows amber 0.6 s | one plate flies straight at the player, then returns | sidestep | as today |
| Overload Beam (`Beam`, kept, wave 20+) | as today | as today | as today | as today |

### Boss QTE: Plate Rush

- **Trigger:** shield completion (recurs each shield cycle, which naturally spaces QTEs past the 20 s minimum).
- **Threat and choreography:** plates exit off-screen, then rush the player one at a time, each from a side at least 70 degrees from the last.
- **Prompts:** 0.9 s window, 8 Prompts, 15% Parry share. No Mash (no grip in the fiction).
- **Answers:** directions and Dash dodge; Shield blocks, staggers and chips the plate (5% of its HP); Parry dash-shields into it (15%, stagger); Nerve bursts it (35%).
- **Clean clear:** the shield shatters; 3 s Punish Window on the exposed core (+50% damage, stacks with the 2.5x dash multiplier).
- **Fail:** the plate lands live (still dodgeable); the shield stays up for its full duration.

### Open for build

- Plate HP, Ring Crush ring speed and damage, Rubble Barrage shard count.
- Debris Squall frequency per wave and its ramp across waves 1-25.
- Rockhide roll charge cadence and distance.
- Exact amber tint strengths.

## Rustbloom

**Status:** Draft. Waves 26-50, Biome Boss Wreckworm.

### Theme and palette

A dead fleet's graveyard, eaten by a rust-feeding fungal bloom: hulls rot into spore pods, and everything that dies feeds the bloom.

| Role       | Hex       | Note                                   |
| ---------- | --------- | -------------------------------------- |
| Base       | `#967850` | rust haze (`Palette::RustbloomHaze`)   |
| Accent     | `#8CA546` | sickly green (`Palette::RustbloomAccent`) |
| Danger     | `#C43A3A` | shared danger red                      |
| Background | `#100E0A` | (`Palette::RustbloomVoid`)             |

### Native Enemies

No new enemy: fodder, turret, spawner, charger and slow are all covered. All organic; organic merging stays.

| Name | Status | Role | Look | Behaviour | Counterplay | Starting numbers |
| --- | --- | --- | --- | --- | --- | --- |
| `Spore Swarmling` | keep | fast fodder | green spore | chases | sweep with fire | unchanged |
| `Husk Sentinel` | keep (look) | turret | fungus-crusted wreck turret | fires every 2.5 s | break line of sight, close in | unchanged |
| `Hive Node` | keep | spawner | green pod cluster | spawns 2 Spore Swarmlings every 3 s | kill early | unchanged |
| `Boarder` | keep | charger | rust haze | charges; the step up from Rockhide's roll | sidestep | unchanged |
| `Carrion Drone` | keep (recolour) | slow | amber to rust haze with green spots | on contact slows the player 2 s and dies | shoot before contact; dash through | unchanged |

The Carrion Drone's slow overlaps the Blight Gate's on purpose: attrition is Rustbloom's flavour.

### Global Enemy tint

Green-brown tint, about 15%.

### Background

Rework of `drawRustbloomBackground` into three layers, far to near:

- **Far** (parallax 0.05): huge derelict capital-ship silhouettes furred with bloom.
- **Mid** (parallax 0.3): the existing hull fragments plus slow spore motes.
- **Near** (parallax 0.8): the existing pods and tendrils, pulsing.

### Biome Hazard

**Rust Patches** (always on): rust blooms where enemies die and slowly spreads; crossing it deals damage over time; shooting or dashing through burns it away. Affects the player only. Distinct from the Blight Gate (slow only, wormhole exits only).

### Anomaly variant

**Blight Gate** (wormhole): each exit leaves a short-lived spore cloud that slows the player, so teleporting has a cost. Details may be refined in this Biome's tickets.

### Wave Boss flavour

Mini Wave Bosses at waves 35 and 45. Pool: The Last Signal, Hollow Idol, Dim Reaver, Static Wraith (dead-fleet and ghost names; Structural Wyrm left out as a Wreckworm echo).

## Solar Forge

**Status:** Draft. Waves 51-75, Biome Boss Slagmaw.

### Biome Hazard

**Heat and caves** (always on, existing): the heat meter climbs outside open caves and drains inside; above the melt threshold it ticks damage. Kept as is; tuning notes only. Affects the player only; keeps its HUD meter.

### Anomaly variant

**Flare Gate** (wormhole): each exit fires a brief, telegraphed flame jet along its facing. Details may be refined in this Biome's tickets.

### Wave Boss flavour

Mini Wave Bosses at waves 55 and 65. Themed pool of 4-5 Wave Boss types: to be named in this Biome's design ticket.

## Punctum

**Status:** Draft. Waves 76-100, Biome Boss Kraken.

### Biome Hazard

**Thunder Strike** (event): the existing thunder flash becomes the tell; a target ring marks a spot near the player about 1 s before a bolt hits it for radius damage. Damages enemies too, so they can be lured in.

### Anomaly variant

**Collapse** (black hole): every few seconds its pull flips to an outward push, telegraphed by the dust reversing; builds on the Kraken portal look. Details may be refined in this Biome's tickets.

### Wave Boss flavour

Mini Wave Bosses at waves 85 and 95. Themed pool of 4-5 Wave Boss types: to be named in this Biome's design ticket.

## Banished realm

**Status:** Draft. Finale arena, not a Biome (from [Banished realm as a fifth Biome](https://github.com/njayman/galaxyimpact/issues/8)). Reached through the Kraken's portal after wave 100; Biome Boss Banished; defeating it leads to the ending.

Arena rules kept as is: horizontal wraparound strip, unlimited Nerve, unlimited dash charges. The only enemies are Banished's own summons.

### Theme and palette

Its own palette, not Punctum's (Punctum's look appears only as the entry portal). To be designed in the Banished realm's design ticket.

### Native Enemies

None (finale arena).

### Global Enemy tint

None (finale arena).

### Background

Its own background in place of today's empty black void. To be designed in the Banished realm's design ticket.

### Biome Hazard

None (finale arena).

### Anomaly variant

None (finale arena).

### Wave Boss flavour

None (finale arena).
