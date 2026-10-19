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

### Anomalies

From [Anomaly variants per Biome](https://github.com/njayman/galaxyimpact/issues/5).

- Both Anomalies (black hole, wormhole pair) take the current Biome's palette everywhere.
- Each Biome adds one behaviour twist to one featured Anomaly: its Anomaly Variant.
- An Anomaly Variant's twist stays a property of that object; it never changes a Biome-wide meter or state (that is a Biome Hazard's job).
- Spawn timing, lifetime and pull strength are identical in every Biome; a twist never makes Anomalies more frequent.

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

**Status:** Draft. Waves 1-25, Biome Boss Beltbreaker.

### Anomaly variant

**Grinder** (black hole): loose rock caught in its pull orbits it as contact-damage shrapnel; shooting the orbit breaks it up. Details may be refined in this Biome's tickets.

## Rustbloom

**Status:** Draft. Waves 26-50, Biome Boss Wreckworm.

### Anomaly variant

**Blight Gate** (wormhole): each exit leaves a short-lived spore cloud that slows the player, so teleporting has a cost. Details may be refined in this Biome's tickets.

## Solar Forge

**Status:** Draft. Waves 51-75, Biome Boss Slagmaw.

### Anomaly variant

**Flare Gate** (wormhole): each exit fires a brief, telegraphed flame jet along its facing. Details may be refined in this Biome's tickets.

## Punctum

**Status:** Draft. Waves 76-100, Biome Boss Kraken.

### Anomaly variant

**Collapse** (black hole): every few seconds its pull flips to an outward push, telegraphed by the dust reversing; builds on the Kraken portal look. Details may be refined in this Biome's tickets.

## Banished realm

**Status:** Draft. Biome Boss Banished.

### Anomaly variant

None by default (closed void arena); final call in [Banished realm as a fifth Biome](https://github.com/njayman/galaxyimpact/issues/8).
