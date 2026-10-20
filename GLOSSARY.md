# Galaxy Impact

A wave-survival space shooter whose run crosses a sequence of biomes, each ending in signature boss fights.

## World

**Biome**:
A themed stretch of waves (Shattered Belt, Rustbloom, Solar Forge, Punctum) with its own look, enemies and hazards.
_Avoid_: zone, level, area

**Banished realm**:
The finale arena reached through the Kraken's portal after Punctum, where only Banished is fought; not a Biome.
_Avoid_: fifth biome, Banished biome

**Biome Hazard**:
An environmental effect tied to one biome that is not an entity you fight, such as Solar Forge heat.
_Avoid_: hazard (alone), environment effect

**Anomaly**:
A black hole or wormhole: a space phenomenon that can appear in any biome.
_Avoid_: hazard, portal

**Anomaly Variant**:
A Biome's take on one Anomaly: its palette plus one behaviour twist that stays local to the Anomaly.
_Avoid_: biome anomaly, special black hole

## Foes

**Native Enemy**:
An enemy that spawns only in its own biome and follows that biome's palette.
_Avoid_: exclusive enemy, signature enemy

**Global Enemy**:
An enemy that can spawn in any biome; it keeps its behaviour everywhere but may take the local biome's tint.

**Elite Hazard**:
A special global enemy (Warlord, Suppressor) that buffs or disrupts the fight; an enemy, not a Biome Hazard.

**Biome Boss**:
One of the five signature bosses (Beltbreaker, Wreckworm, Slagmaw, Kraken, Banished) bound to its biome, or for Banished, to the Banished realm.
_Avoid_: main boss, signature boss

**Wave Boss**:
A generic mini, mega or swarm boss with a random moveset that appears on other milestone waves.
_Avoid_: generic boss, miniboss (as a category)

**Boss QTE**:
A cutscene reaction sequence owned by a Biome Boss, during which the player only answers prompts.
_Avoid_: QTE event, reaction event

**Prompt**:
One asked input within a Boss QTE, answered right, wrong or late.
_Avoid_: button sequence, QTE step

**Mash**:
A Prompt answered by pressing one shown button repeatedly to fill a bar before time runs out.

**Punish Window**:
The short dazed spell after a Boss QTE is cleared without a miss, when the Biome Boss cannot attack and takes extra damage.
