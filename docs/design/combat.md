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

Also from the trailer, after these: rotating spiral and ring patterns for boss barrages, a thin tell line before every beam, and short afterimage trails on fast movers.
