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

- **Cutscene contract:** HUD hidden with letterbox bars, world at 4% speed, player immune. One Prompt at a time from the shared input set: four directions, Shield, Dash, Parry (shield + dash), Nerve, and Mash. A late answer, or a wrong one once the chain is live, ends the cutscene and the threat lands live, still dodgeable. The chain goes live on the first correct answer: until then wrong presses are ignored, since the player is usually mid-fight on other controls when a QTE pops. Only the threat and its choreography differ per Biome Boss.
- **Trigger:** a beat the Biome Boss already has and that recurs. Skipped while the player is grabbed or launched, or another cutscene runs. At least 20 s before the same Boss QTE can start again (per QTE, so a boss's Rush does not lock out its Constrict).
- **Damage:** every hit on an enemy or boss while a Boss QTE runs deals 3x damage.
- **Finisher:** a clean chain ends with controls locked while the ship turns on the boss, gathers a charge and fires one heavy shot. Each answer charges it by 2% of the boss's max health (5% Parry, 10% Nerve, 3% for a completed Mash), tripled by the damage rule. Only then does the boss's own clean-clear payoff (Punish Window and the like) follow. Answers no longer strike the boss one by one.
- **Every boss:** any boss's ChargeDash while it is enraged (25% health or less) starts the Rush QTE (3 prompts as it passes); a clean clear ends in the finisher and stuns it 3 s (no moves, +50% damage). The Kraken (Dragged Under) and the Banished (Juggled) have their own QTEs; the Storm Serpent has none.
- **Flawless clear:** a 3 s Punish Window; the boss is dazed, cannot attack and takes +50% damage.
- **Fail:** the live hit only; damage already dealt sticks.
- **Difficulty by Biome order** (Prompt window / Prompt count / Parry share), exact values per Biome Boss:

  | Biome Boss  | Window | Prompts | Parry |
  | ----------- | ------ | ------- | ----- |
  | Beltbreaker | 0.9 s  | 8       | 15%   |
  | Wreckworm   | 0.8 s  | 9       | 20%   |
  | Slagmaw     | 0.7 s  | 10      | 25%   |
  | Kraken      | 0.6 s  | 12      | 30%   |
  | Banished    | 0.5 s  | 14      | 35%   |

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

- **Far**: broken planets. The arena is cut into an equal 3x3 grid of zones. Each run, every zone has a 45% chance of a planet, rolled fresh, so some stretches are open space. A planet sits at screen centre when you are at its zone's centre, drifts against your movement so it slides just off screen as you reach the zone edge, and only fades in the last few percent. Each is a big, faint, low-poly faceted sphere, about 90% of screen height across and drawn at about 5% strength behind the stars, so it reads as distant. Orientation, light direction and rock colour (ash, rust, bone, slate, dusk, ochre) are random per run. A fracture zone around a break point lifts the facets apart onto the core. About half the planets have a live core (orange, red, cyan or violet) glowing through the seams with a slow pulse; the rest have a dark core. The facets nearest the break fly off as shards, and a tilted debris ring runs along the dust-lane angle. Each run also starts at a random point in the inner 70% of the arena.
- **Mid** (parallax 0.3): a few faint, irregularly spaced amber dust lanes drifting slowly. Debris Squalls arrive along the lane direction, so the background foreshadows them.
- No near layer: the real asteroids already fill that role. Kept minimal after playtest feedback (too much on screen, evenly spaced parallel lines).

Described in words only; looks the build finds open can become a prototype ticket.

### Biome Hazard

**Debris Squall** (event): a few times per wave an edge warning shows, then a band of fast rocks sweeps across the field in one direction; move out of the band. Damages enemies too, so they can be lured in. Switched off during Beltbreaker fights.

Each rock hurts on contact and can be shot: a bullet pops it with a small spark burst (2 score), clearing that piece of the band. Starting numbers: one squall every 18 s at wave 1, ramping linearly to every 10 s at wave 25; 1.2 s warning (band outline plus chevrons at the screen edge it enters from); band 180 units wide, centred within 60 units of the player, sweeping along the dust-lane angle at 1500 units/s; 1 wave-scaled damage to the player on rock contact (normal immunity frames apply), 12 damage every 0.25 s to enemies touching a rock.

### Anomaly variant

**Grinder** (black hole): loose rock caught in its pull orbits it as contact-damage shrapnel; shooting the orbit breaks it up. Details may be refined in this Biome's tickets.

As built: asteroids (single rocks and Belt rock clusters) that enter the pull lock into an orbit between 1.8x the hole's radius and 70% of its influence radius, turning at 140°/s, instead of being swallowed. They keep the normal asteroid rules: contact damage, and they break when shot. When the hole closes they fly off along the tangent. Both Anomalies take the amber accent in the Belt: amber dust in the black hole and amber wormhole rims.

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

As built: both replacements keep the `Barrage` and `GravityWell` attack slots and branch on the Beltbreaker core. Rubble Barrage keeps Barrage's aimed stream (no fan) with stone-shard projectiles. Ring Crush: plates pull in to 0.7x their orbit during the tell, then fly out to 600 units at speed 6 and come back. Each attached plate carries an amber arc covering 80% of its slot, so a missing or detached plate leaves a gap. The ring does no damage within 50 units of the hull, so hugging the core is safe.

### Boss QTE: Plate Rush

- **Trigger:** shield completion (recurs each shield cycle, which naturally spaces QTEs past the 20 s minimum).
- **Threat and choreography:** plates exit off-screen, then rush the player one at a time, each from a side at least 70 degrees from the last.
- **Prompts:** 0.9 s window, 8 Prompts, 15% Parry share. No Mash (no grip in the fiction).
- **Answers:** directions and Dash dodge; Shield blocks, staggers and chips the plate (5% of its HP); Parry dash-shields into it (15%, stagger); Nerve bursts it (35%).
- **Clean clear:** the shield shatters; 3 s Punish Window on the exposed core (+50% damage, stacks with the 2.5x dash multiplier).
- **Fail:** the plate lands live (still dodgeable); the shield stays up for its full duration.
- As built: during the Punish Window the plates stay parked off screen and fly home when it ends. On a fail, the other plates go back to harassing until the shield's return lead time. A ring of pulsing gold shield fragments marks the open core.
- Finisher: while any plate is alive, the clean chain's finisher shot hits the healthiest plate (its charge share of that plate's max HP, tripled) instead of the core; with no plates left it hits the core.

### Open for build

- Plate HP, Ring Crush ring speed and damage, Rubble Barrage shard count.
- Debris Squall frequency per wave and its ramp across waves 1-25.
- Rockhide roll charge cadence and distance.
- Exact amber tint strengths.

## Rustbloom

**Status:** Approved, 2026-10-19. Waves 26-50, Biome Boss Wreckworm.

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

Rework of `drawRustbloomBackground` into three layers, far to near, kept faint and sparse behind gameplay:

- **Far** (parallax 0.05, `drawRustbloomDerelicts`, drawn before the stars like the Belt planets): huge derelict capital ships (about 650-900 px long) on a 1100 px layer grid, about 60% of cells filled, placement varied per run by `backdropSeed`. Each is a faceted hull, every section split along the keel into a lit and a shadowed facet, snapped in two with the stern half knocked askew; plating rotted away ahead of the break bares the interior and a few ribs, with torn chunks adrift in the gap. Superstructure blocks cast short shadows, a stepped bridge sits aft, dead engines at the stern. Bloom furs the outer edges in patches (swaying slightly) and thickly along both cut faces, with a few pods rooted near the break. Drawn about 10% of the way from `RustbloomVoid` to full colour; at this parallax ships only ever slide in and out at the screen edges, never fading.
- **Mid** (parallax 0.3): the existing hull fragments, thinned and toned toward the void, plus sparse spore motes, each circling slowly in place with a soft halo.
- **Near** (parallax 0.8): the existing pods and tendrils, slightly sparser and fainter; pods breathe in size and core glow, and each tendril pulses with the blend of its two pods.

### Biome Hazard

**Rust Patches** (always on): rust blooms where enemies die and slowly spreads; crossing it deals damage over time; shooting or dashing through burns it away. Affects the player only. Distinct from the Blight Gate (slow only, wormhole exits only). Stays on during Wreckworm fights; destroyed and shed segments feed Rust Patches like any death.

Starting numbers (`rustPatch*` in `include/update_constants.hpp`; Start/End ramp linearly from wave 26 to wave 50):

| Value | Start | End | Note |
| --- | --- | --- | --- |
| Bloom chance per enemy death | 35% | 60% | Wreckworm segments always bloom; a death inside a patch grows it by 8 px instead |
| Spread rate (radius growth) | 2 px/s | 3.5 px/s | from 20 px at bloom |
| Max radius | 46 px | 62 px | |
| Bud interval at max radius | 10 s | 6 s | buds a new patch at its edge |
| Patch cap | 32 | 32 | patches beyond 1400 px from the player wither |

- **Tell:** a new patch is a dashed, pulsing outline for 1 s and harmless until it ripens; the rim turns danger red while the player stands in it.
- **Damage:** 1 base damage (scaled by `enemyDamage`) 0.35 s after stepping in, then every 1 s; player only.
- **Counter:** bullets passing over a patch burn 160 px/s of radius each and pass through; a patch the player stands in cannot be shot down from inside. Dashing through any patch burns it away at once.

### Anomaly variant

**Blight Gate** (wormhole): each exit leaves a short-lived spore cloud that slows the player, so teleporting has a cost. Details may be refined in this Biome's tickets.

As built: every time the player comes out of a wormhole mouth, that mouth leaves an anchored spore cloud (radius 70, 4 s; a second exit through the same mouth refreshes it instead of stacking). While inside, the player moves at half speed (the shared slow), and the slow lingers 0.3 s after leaving. Spore clouds deal no damage and, unlike other gas clouds, cannot be cleared by a shield dash (that would cancel the toll). Only the player's exits leave spores; enemies and bullets pass through freely. Both Anomalies take the Rustbloom palette: lichen-green and rust dust in the black hole and lichen-green wormhole rims. Wormhole timing, lifetime and behaviour are unchanged.

### Wave Boss flavour

Mini Wave Bosses at waves 35 and 45. Pool: The Last Signal, Hollow Idol, Dim Reaver, Static Wraith (dead-fleet and ghost names; Structural Wyrm left out as a Wreckworm echo).

### Biome Boss: Wreckworm

Waves 30, 40, 50. Skeleton kept: a head trailing armoured segments (armour takes 35% damage) that fire volleys every 3-5 s.

**Phases**

1. **Armoured:** full length, attacks from the table below.
2. **Molt:** at 50% HP (waves 30, 40) or at 66% then 33% (wave 50, in that order). Sheds two tail segments as asteroids, +15% speed, 1.5 s lull.
3. **Burrow:** Burrow Charge joins the moveset from wave 40.

**Attacks**

| Attack | Tell | Effect | Counter | Starting numbers |
| --- | --- | --- | --- | --- |
| Spore Spit (replaces `Barrage`) | head flares green 0.5 s | green spore spray, Barrage behaviour with Rustbloom look | strafe through gaps | as `Barrage` |
| Coil Clamp (`CoilClamp`, kept) | body assembles a ring around the player 1.5 s | ring closes over 2.6 s from 260 to 40 radius, 90 degree gap | escape through the gap; caught inside starts the Boss QTE | as today |
| Dash Rush (`DashRush`, kept) | as today | off-screen passes | sidestep | as today |
| Cloud Breath (`CloudBreath`, kept) | as today | gas clouds | keep clear | as today |
| Tail Wrap (`TailWrap`, kept) | as today | circles and chases | outrun, break line | as today |
| Burrow Charge (`BurrowCharge`, kept, wave 40+) | as today | as today | as today | as today |

As built: Spore Spit keeps the `Barrage` attack slot and branches on the Wreckworm head (not the Kraken's snake summon, which keeps its fluid blobs). The tell is 0.5 s: the head pulses pale green inside a pulsing green ring. It then fires Barrage's aimed stream, but each shot is a spinning hexagonal green spore with eight spines and a short puff trail. Skeleton audit found no drift: armoured segments take 35%, molts at 50% (waves 30, 40) and at 66% then 33% (wave 50) each shed the two rearmost segments as asteroids, add 0.15 to the head's speed multiplier and give a 1.5 s lull, and Burrow Charge joins the moveset from wave 40.

### Boss QTE: Constrict

- **Trigger:** Coil Clamp closes with the player still inside (recurs; 20 s minimum gap). Escaping through the gap means no QTE.
- **Choreography:** the coil tightens; a **Mash** Prompt to pry it open (fill the bar within 1.6 s), then 8 segment strikes, each lunging from a different point on the coil.
- **Prompts:** 9 total (1 Mash + 8 strikes), strikes at 0.8 s window, 20% Parry share; strikes use directions, Shield, Dash, Parry, Nerve.
- **Clean clear:** the coil springs open; the head is stunned for the 3 s Punish Window (+50%), and armoured segments take full damage during it.
- **Fail:** the coil crushes for one hit and releases the player.
- As built: the Mash takes 8 presses in the 1.6 s window (5 a second, leaving room to react); it is mashed on the Nerve input (SPACE, or the NERVE touch button) and other inputs are ignored. The crush is 2 base damage, scaled like other enemy hits (as heavy as a plate dash or Ring Crush). The coil catches the player when its closing ring reaches them outside the gap, then rides with the ship and keeps tightening during the Mash. Strikes come from segments that have not struck yet. Clean or failed, the clamp skips to its release and the segments rejoin the chain; during the Punish Window the head stops moving, starts no attacks, segments hold their volleys, and a ring of pulsing fragments marks the stunned head. The 20 s minimum gap is per Boss QTE, so the Wreckworm's Rush does not lock out its Constrict.

### Boss QTE: Rush

- **Trigger:** a Burrow Charge or Dash Rush head closing within 340 units of the player (recurs; 20 s minimum gap). Not on the Storm Serpent.
- **Threat and choreography:** the head keeps crawling through the slowed world at 420 units a second, so it and then the body pass the ship.
- **Prompts:** 3, 0.8 s each, 0.3 s apart, the usual mix with 20% parry. Each answer charges the shared finisher shot that ends a clean chain.
- **Clean clear:** the head ploughs to a stop and the rush ends; Punish Window as Constrict (3 s, +50% damage, no movement or attacks).
- **Fail:** the cutscene ends and the rush carries on live, still dodgeable.

### Open for build

- Spore Spit spray density.
- Rust Patch spread rate and ramp across waves 26-50.

## Solar Forge

**Status:** Approved, 2026-10-19. Waves 51-75, Biome Boss Slagmaw.

### Theme and palette

A forge world crusted over a dying star: basalt caves the only shade, molten seams and ash drifting through solar glare.

Today's palette is red on red and its accent collides with the shared danger red; it moves so danger is the only true red here.

| Role       | Hex       | Note                                              |
| ---------- | --------- | ------------------------------------------------- |
| Base       | `#3A1C16` | charred basalt (replaces `Palette::SolarForgeHaze` use) |
| Accent     | `#FFB23A` | molten gold (replaces `Palette::SolarForgeAccent` use); brighter and more saturated than the Belt's amber |
| Danger     | `#C43A3A` | shared danger red                                 |
| Background | `#100404` | (`Palette::SolarForgeVoid`)                       |

### Native Enemies

No new enemy. The Vent Crawler rework ends its overlap with Forgeborn and ties it to the heat hazard.

| Name | Status | Role | Look | Behaviour | Counterplay | Starting numbers |
| --- | --- | --- | --- | --- | --- | --- |
| `Cinder Wisp` | keep (recolour) | exploder | gold-cored star | chases, explodes on death | kill at range | unchanged (2 damage, radius 50) |
| `Forgeborn` | keep (look) | heavy chaser | molten golem, basalt with gold seams | chases | kite, dash damage | unchanged |
| `Solar Turret` | keep (recolour) | turret | basalt with gold lens | fires every 3.5 s | cover, close in | unchanged |
| `Vent Crawler` | rework | cave flusher | basalt with glowing vents (was danger red) | creeps along cave edges; after a 0.6 s gold glow tell vents a flame jet into the cave | leave the jet line or kill it before it vents | jet 160 units, 2 damage, every ~4 s; other numbers unchanged |

### Global Enemy tint

Ember-orange tint, about 15%.

### Background

Three layers, far to near:

- **Far** (parallax 0.02): a dim, vast stellar corona glaring from one edge.
- **Mid** (parallax 0.3): drifting ash and heat shimmer.
- **Near** (world-locked): the existing cave rock and embers from `drawSolarForgeCaveBackground`, recoloured to basalt and gold. Shapes untouched: they define the safe caves.

### Biome Hazard

**Heat and caves** (always on, existing): the heat meter climbs outside open caves and drains inside; above the melt threshold it ticks damage. Kept as is; tuning notes only. Affects the player only; keeps its HUD meter. Stays on during Slagmaw fights: Devour can drag the player out of a cave into the heat.

As built: the rock outside the caves is a dense gel. Inside it the ship moves at 30% speed (dashes at 60%), the gel parts round the ship, trails behind its movement and jiggles back when it stops, with a splash on entry.

### Anomaly variant

**Flare Gate** (wormhole): each exit fires a brief, telegraphed flame jet along its facing. Details may be refined in this Biome's tickets.

### Wave Boss flavour

Mini Wave Bosses at waves 55 and 65. Pool: Ember Titan, Cinder Duke, Beamforge, Lightbound Custodian.

### Biome Boss: Slagmaw

Waves 60, 70, 75. Skeleton kept: a huge slow mass (0.32x speed), now basalt with molten gold seams (was `Palette::SolarForgeAccent` red). Death behaviour by wave: wave 60 dies; wave 70 shatters into 14 tough meteor drifters; wave 75 splits into halves that keep splitting by generation.

**Phases**

1. **Molten:** attacks from the table below; Devour gives the recurring QTE beat.
2. **Death break:** per wave as above. Wave 75 halves cannot Devour (too small).

**Attacks**

| Attack | Tell | Effect | Counter | Starting numbers |
| --- | --- | --- | --- | --- |
| Slag Spray (replaces `Barrage`) | body flares gold 0.5 s | molten spray, Barrage behaviour | strafe through gaps | as `Barrage` |
| Magma Ram (replaces `ChargeDash`) | as ChargeDash | ChargeDash plus a burning trail | sidestep, avoid the trail | trail lasts 2 s, 1 damage per 0.5 s inside |
| Meteor Hell (`MeteorHell`, kept) | as today | as today | as today | as today |
| Meteor Swarm (`MeteorSwarm`, kept) | as today | as today | as today | as today |
| Seeker Embers (replaces `HomingBarrage`, wave 70+) | as HomingBarrage | homing embers, ember look | outrun, shoot down | as `HomingBarrage` |
| Devour (new, all waves) | maw glows gold 1 s | inhale cone pulls the player toward the maw for 2 s; reaching the maw starts the Boss QTE | dash across or leave the cone; caves do not protect | pull strength tuned in build |

### Boss QTE: Swallowed

- **Trigger:** Devour pulls the player into the maw (recurs; 20 s minimum gap). Escaping the cone means no QTE.
- **Choreography:** a **Mash** Prompt to brace against the jaws, then 9 Prompts as Slagmaw chews and whips about, spitting slag gobs from shifting angles.
- **Prompts:** 10 total (1 Mash + 9), 0.7 s window, 25% Parry share.
- **Clean clear:** the player bursts out and Slagmaw chokes: 3 s Punish Window (+50%) with the maw exposed as a weak point.
- **Fail:** the player is spat out and takes one burn hit.

- As built: Slagmaw is a faceted basalt mass lit from the upper left, warm gold seams running back from a maw ringed by uneven fangs that turns to track the player; tells heat the seams and open the maw. Slag Spray keeps Barrage's aimed stream with a 0.5 s gold flare tell, spraying jagged slag gobs from the maw within 14 degrees. Magma Ram drops a pool every 70 units of the charge (64 radius, 2 s, 1 damage per 0.5 s, ignoring hit immunity; a dash crosses unharmed). Seeker Embers are spinning gold diamonds that shed sparks. Devour: 1 s tell, then a 2 s inhale along a cone (35 degrees each side, 760 long) locked to where the maw faced, pulling 300 units a second; a dash rides through the pull; reaching the maw while the QTE is on its gap takes a 2 damage bite and throws the player clear. Swallowed: Mash 8 presses in 1.6 s, then 9 prompts (0.7 s, 25% Parry) with a slag gob flying in from each prompt's angle while the boss shakes and the maw whips; a clean chain ends in the shared finisher shot. Wave 75 halves keep the old moveset (no Devour).

### Planned rework: Lava Mole (idea, 2026-10-22, not built)

- **Look:** keep the current design language (basalt, gold seams, fanged maw), but the body becomes a thinner lava mole instead of a planet-like mass.
- **Spawn:** Slagmaw floods the whole map with the Solar Forge lava gel (it slows the player), except the area the screen shows at that moment. The player can't easily escape and has to fight inside that clearing.
- **Open:** whether the clearing stays fixed or shifts; whether the gel clears on death; whether the mole burrows through the gel; how Devour and Magma Ram fit a thinner body.

### Open for build

- Devour pull strength and cone width; maw weak-point multiplier.
- Magma Ram trail damage; Slag Spray density.
- Vent Crawler jet cadence; heat ramp across waves 51-75.
- Exact basalt and gold shading of the cave layer.
- Colour note (from [Punctum: theme, Native Enemies and background](https://github.com/njayman/galaxyimpact/issues/15)): `Meteor Chunk`, Slagmaw's wave-70 drifter, moves from the old red to basalt and molten gold.

## Punctum

**Status:** Approved, 2026-10-20. Waves 76-100, Biome Boss Kraken.

### Theme and palette

The region around the Punctum, a pinprick tear in space: a dark storm nebula crackling with lightning, starlight bending toward the point. The last Biome, the most hostile and the strangest.

The old yellow accent clashed with Solar Forge's gold and the Belt's amber; it moves to the lightning's cyan.

| Role       | Hex       | Note                                       |
| ---------- | --------- | ------------------------------------------ |
| Base       | `#C896E6` | lavender (`Palette::PunctumHaze`)          |
| Accent     | `#7FE3FF` | electric cyan (replaces `Palette::PunctumAccent` yellow) |
| Danger     | `#C43A3A` | shared danger red                          |
| Background | `#06060A` | (`Palette::PunctumVoid`)                   |

### Native Enemies

No new enemy: the three do not overlap and Global Enemies cover ranged. `Meteor Chunk` is Slagmaw's summon, not a Punctum enemy.

| Name | Status | Role | Look | Behaviour | Counterplay | Starting numbers |
| --- | --- | --- | --- | --- | --- | --- |
| `Null Crawler` | keep (recolour) | phasing chaser | lavender ring with cyan phase shimmer | chases, cycles in and out of phase | hit it while solid | unchanged (24 HP, speed 1.3) |
| `Anomaly Warden` | keep (tell) | area denial | octagon, cyan | stationary; pulses every 4 s; new 0.6 s cyan charge-up ring as the tell | leave the ring before it fires | unchanged (90 HP, 3 damage, radius 90) |
| `Fused Wreck` | keep (look) | mega-tank | wreckage cluster fused by lightning | slow chase | kite, focus fire | unchanged (260 HP, 4 contact) |

### Global Enemy tint

Lavender tint, about 15%.

### Background

Three layers, far to near:

- **Far** (fixed): the Punctum itself, a tiny blinding point ringed by bent light, always in the same direction.
- **Mid** (parallax 0.3): a lavender storm nebula lit from inside by each Thunder Strike flash.
- **Near** (parallax 0.8): faint star streaks bent toward the Punctum.

### Biome Hazard

**Thunder Strike** (event): the existing thunder flash becomes the tell; a target ring marks a spot near the player about 1 s before a bolt hits it for radius damage. Damages enemies too, so they can be lured in.

### Anomaly variant

**Collapse** (black hole): every few seconds its pull flips to an outward push, telegraphed by the dust reversing; builds on the Kraken portal look. Details may be refined in this Biome's tickets.

### Wave Boss flavour

Mini Wave Bosses at waves 85 and 95. Pool: Void Herald, Nullpoint Marauder, Stormcaller, Judgment Spire.

### Biome Boss: Kraken

Waves 80, 90, 100 (encounters 1, 2, 3). Skeleton kept; new look: lavender body with cyan lightning veins, staggered tentacles crackle cyan.

As built (2026-10-22 rework): the body is a third of its old size (175 across) and the silhouette is carried by the tentacles: eight thin resting tentacles, six to seven body-lengths long, writhe slowly around it. Swipe tentacles are thin and 640 long, open from portals 260 to 380 from the player and sweep 110 degrees; their whole length hits, not just the tip. Staggered tentacles hang 260 out from the body. The Snatch grip reaches from a portal 320 away.

**Phases**

1. **Storm:** attacks from the table below. Tentacles (1 / 2 / 4 active by encounter) emerge from portals near the player and swipe; shielding a swipe staggers that tentacle; staggering every active tentacle exposes the core for 8 s, once per fight. Snatch gives the recurring QTE beat.
2. **Limbs (wave 90+):** pods pull enemies out of portals and hurl them.
3. **Summons by encounter:** wave 80, 8 enemies every 9 s; wave 90, 3 tough enemies plus an Elite Hazard every 12 s; wave 100, one Storm Serpent after 4 s.
4. **Exit:** at 0 HP on waves 80 and 90 the Kraken dives into a small tear portal (was: swims off-screen). Wave 100 plays the outro below.

**Attacks**

| Attack | Tell | Effect | Counter | Starting numbers |
| --- | --- | --- | --- | --- |
| Static Burst (replaces `Barrage`) | as Barrage | cyan bolt spray, Barrage behaviour | strafe through gaps | as `Barrage` |
| Arc Lance (replaces `Beam`) | as Beam | forked lightning beam, Beam behaviour | step out of the line | as `Beam` |
| Riptide (replaces `GravityWell`) | as GravityWell | pull toward a small tear, GravityWell behaviour | thrust away | as `GravityWell` |
| Ball Lightning (replaces `HomingBarrage`, wave 90+) | as HomingBarrage | homing lightning orbs | outrun, shoot down | as `HomingBarrage` |
| Tentacle swipe (kept) | portal opens, 0.4 s emerge | 140° swipe | shield to stagger | 2 damage |
| Limb hurl (kept, wave 90+) | as today | thrown enemy | dodge | 3 damage |
| Snatch (new, all encounters) | cyan ring opens under the player 1 s | a tentacle grabs the player from the portal; a catch starts the Boss QTE | leave the ring | uses an idle, unstaggered tentacle slot |

**Snatch rules:** never during core exposure, the outro, or a Storm Serpent Coil Clamp hold; a fully staggered Kraken cannot Snatch. Inside the 20 s QTE gap a catch is a plain grab: a short hold, then the fail outcome, no cutscene.

**Storm Serpent** (wave 100 summon, was the off-palette red snake): lavender segments with cyan crackle; behaviour unchanged. Its Coil Clamp is a plain grab and never starts a Boss QTE (those belong to Biome Bosses only).

**Thunder Strike** stays on during the fight, unchanged, and still hits summons. Paused in the Boss QTE; suppressed during the outro.

**Outro (wave 100):** kept beat for beat in cyan and lavender: the Kraken approaches, opens the portal ("Follow me, if you dare."), and the trap pulls the player into the Banished realm.

### Boss QTE: Dragged Under

- **Trigger:** Snatch catches the player (recurs; 20 s minimum gap). Leaving the ring means no QTE.
- **Choreography:** a **Mash** Prompt to break the grip, then 11 Prompts as the Kraken drags the player portal to portal through the tear while other tentacles lash in; thunder flashes are visual only.
- **Prompts:** 12 total (1 Mash + 11), 0.6 s window, 30% Parry share.
- **Clean clear:** the player severs the gripping tentacle: 3 s Punish Window (+50%) with the core exposed as a weak point. Does not use up the once-per-fight 8 s core exposure.
- **Fail:** flung out of a portal at a random nearby spot; one tentacle hit (2 damage).

As built: Snatch tries every 8 to 10 s (rolled) when an active tentacle slot is idle and unstaggered; its ring is 70 px and the plain grab holds 0.8 s. A caught ship is gripped by a tentacle from a portal 180 px away; after the Mash it is hauled toward that portal and, after every 4 answers, pulled through to a new spot beside the Kraken with a visual-only thunder flash. A clean clear ends in the finisher into the core (its charge damage, tripled), then the grip is severed and the Punish Window is the shared 3 s QTE stun (`qteStunTimer`): the core takes weapon damage at +50% and no new tentacle swipes start.

### Open for build

- Snatch ring radius, cadence and the plain-grab hold length; core weak-point multiplier.
- Static Burst density; Arc Lance fork count; Riptide pull strength.
- Tear-portal dive timing at waves 80 and 90.
- Storm Serpent exact shading.

## Banished realm

**Status:** Approved, 2026-10-20. Finale arena, not a Biome (from [Banished realm as a fifth Biome](https://github.com/njayman/galaxyimpact/issues/8)). Reached through the Kraken's portal after wave 100; Biome Boss Banished; defeating it leads to the ending.

Arena rules kept as is: horizontal wraparound strip, unlimited Nerve, unlimited dash charges. Banished fights alone: no summons.

### Theme and palette

The prison beyond the tear, where Banished was sealed away. No stars and no light of its own: the only light is Banished's pale eye and the broken seals that once held it. Defeating Banished leads to the ending. Punctum's look appears only as the entry portal and the sealed tear behind.

| Role       | Hex       | Note                                       |
| ---------- | --------- | ------------------------------------------ |
| Base       | `#CFC8B8` | ash bone (seals and chains)                |
| Accent     | `#FFE2AA` | pale eye-gold (`Palette::Crit`, Banished's eye) |
| Danger     | `#C43A3A` | shared danger red                          |
| Background | `#0E0A14` | bruise-violet black (replaces the empty `Palette::Void`) |

### Native Enemies

None (finale arena).

### Global Enemy tint

None (finale arena).

### Background

Three layers, far to near, each tiling seamlessly along the wraparound strip so the wrap never shows a seam:

- **Far** (fixed): the sealed tear, a lavender and cyan scar; the way in, now closed. On arrival the entry portal visibly collapses into it.
- **Mid** (parallax 0.3): huge broken seal rings and chain fragments in ash bone, drifting slowly; they glow pale gold whenever Banished's eye opens.
- **Near** (parallax 0.8): ash motes drifting upward.

### Biome Hazard

None (finale arena).

### Anomaly variant

None (finale arena).

### Wave Boss flavour

None (finale arena).

### Biome Boss: Banished

Skeleton kept. Look: black body with an ash-bone rim and seal-chain marks so it reads against the realm's dark background; staggered tentacles crack ash bone; eye and beam in pale eye-gold. Only eye bursts damage it: weapon fire just flashes.

**Phases**

1. **Tentacles:** 12 tentacles strike from the body (near) or from portals near the player (far), 0.6 s windup, 0.5 s strike; 3x faster while Banished is offscreen. Shielding a strike staggers that tentacle.
2. **Eye open:** once all 12 are staggered, the eye chases the player for 5 s (Eye Chase); dashing into it or hitting it with Nerve lands an eye burst. Then Doom Gaze fires for 1 s; shield-dashing through it lands a smaller burst. Tentacles then reset. One burst per opening.
3. **Escalation by cycle:** three eye bursts win (a dash-in or Nerve burst takes 1/3 of its health, a Doom Gaze shield-dash 1/6). Cycle 1 as described; cycle 2 adds more far-portal strikes and Chain Lashes; cycle 3 Doom Gaze sweeps after the player for its 1 s and Bind is more frequent. The background seal rings crack further after each burst.
4. **Defeat:** Banished flees slightly slower than the player; a shield-dash into it plays the ending (and unlocks infinite mode).

**Attacks**

| Attack | Tell | Effect | Counter | Starting numbers |
| --- | --- | --- | --- | --- |
| Seal Spike (jab) | 0.6 s windup | straight tentacle strike | shield to stagger, or dodge | 4 damage |
| Chain Lash (thrash) | 0.6 s windup | 110° sweep | shield to stagger, or dodge | 7 damage |
| Bind (grab, near only) | 0.6 s windup | a catch starts the Boss QTE | shield to stagger, or dodge | 15% of near strikes; more in cycle 3 |
| Eye Chase | eye opens | eye follows the player for 5 s | dash into it or Nerve for a burst | 5 s |
| Doom Gaze | after Eye Chase | 1 s beam; sweeps in cycle 3 | shield-dash through for a smaller burst | 6 damage per s |

### Boss QTE: Juggled

- **Trigger:** Bind catches the player (recurs; 20 s minimum gap). Inside the gap a catch is a plain grab: a short hold, then the fail outcome, no cutscene. The old live aim-cancel juggle is removed.
- **Choreography:** a **Mash** Prompt to break the coil, then 13 Prompts across three throws between tentacles, about 4 or 5 per flight.
- **Prompts:** 14 total (1 Mash + 13), 0.5 s window, 35% Parry share.
- **Clean clear:** in place of the +50% Punish Window (only eye bursts hurt Banished), the eye snaps open for 3 s: one free burst, skipping a stagger cycle. Tentacles keep their stagger state; a burst counts toward the three.
- **Fail:** thrown clear, one Chain Lash hit (7 damage).

As built: the clean clear ends in the finisher: the eye snaps open and the beam lands one full eye burst (1/3 health), counted toward the three; there is no extra 3 s window. Throws start at 13, 9 and 5 prompts left (4, 4, 5 per flight); each throw hands the ship to another unstaggered tentacle, which swings it 90 to 150 degrees round the body. Other tentacles hold still while Juggled runs. Inside the gap, Bind is a 0.8 s plain hold, then the fail outcome. The old live juggle (aim-cancel launches) is removed.

### Open for build

- Cycle 2 and 3 strike-mix weights; Doom Gaze sweep speed.
- Plain-grab hold length; QTE throw distances and camera.
- Seal-ring crack stages and the ending's visual beat.
