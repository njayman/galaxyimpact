# Combat feel rework

**Status:** Approved, 2026-10-22. Inspired by the Pathogenic trailer: enemies reposition, tell briefly, then commit hard and fast at unpredictable moments. Built one item at a time, in this order.

1. **Chain Lightning:** long crackling, branching arcs that jump far (about 700 from the ship, 400 between targets) and linger a beat, instead of a short zap.
2. **Spawns, two tiers:**
   - **Swarm:** today's simple chasers, zigzaggers and biome natives. Lower HP, smaller, more of them, an unlimited stream.
   - **Specials:** dashers, snipers, turrets, bombers, mines and ricochet strafers. Rarer and tougher. At most 2 alive at wave 1, ramping to 6 in the late biomes; each type keeps its own cap (dashers and turret/snipers at 2).
   - Specials are what make the player fight instead of holding one direction.
3. **Special behaviours:** random timing, a short tell (0.3 to 0.5 s), then a very fast commit.
   - **Dasher:** circles at mid range, flashes a line, lunges through the player, chaining 2 or 3 lunges.
   - **Sniper:** repositions, then a thin tracking line locks 0.4 s before the shot; sometimes a double tap.
   - **Turret:** plants itself and fires bursts of rotating spiral or ring patterns, so the gaps move.
   - **Bomber:** dives across the player and drops delayed blasts along its path.
4. **Enemy movement:** turn-rate steering with speed variance; swarmers flow in curving, separated streams instead of beelining.
5. **Boss movement:** bosses glide around the player in sweeping arcs with turn-rate steering; wind-up lunges commit along a curve.

## As built

- Chain Lightning (2026-10-22): reaches 700, jumps 400, 2 + level strikes, never revisits an enemy, ends in a boss in reach; jagged bolts with two forks.
- Tiers (2026-10-22): specials are dashers, turrets and snipers, bombers, mines, spawners and laser fences: 2.5x HP, cap 2 rising by one every 20 waves to 6, 3 s between spawns. Swarm: 0.6x HP, 1.5x spawn count.
- Behaviours (2026-10-22): the Sniper keeps the shot QTE and glides to a new spot 450-600 from the player between shots. Turret, Solar Turret and Husk Sentinel no longer use the QTE: they rest 2.5-4.5 s, show a closing ring for 0.4 s, then fire a 1.4 s burst of either a turning four-armed spiral or gapped rings; their shots fly through other enemies. Dashers circle at 260, then chain 2 or 3 lunges with 0.3 s tells between. The Bomber dives through the player and 320 past after a 0.35 s line tell, dropping a bomb every 70 units (0.7 s fuse, 64 radius, 2 damage), then comes round again; it no longer uses the proximity fuse.
- Swarm movement (2026-10-22): chasers turn at most 3.2 rad/s toward an approach angled 40 degrees off the player (per enemy, either side), straightening inside 500; they push apart within 34 of each other; speed varies 0.85-1.15 per enemy with a 15% surge.
- Boss movement (2026-10-22): bosses steer a heading at a limited turn rate (1.4 rad/s; Wreckworm 2.2, director bosses 2.6 keeping their pattern speed), sweeping round the player at the engage distance with an orbit that slowly reverses. Charge dashes bend toward the player at 0.9 rad/s while the player is still ahead (not Beltbreaker plates).

Also from the trailer, after these: rotating spiral and ring patterns for boss barrages, a thin tell line before every beam, and short afterimage trails on fast movers.
