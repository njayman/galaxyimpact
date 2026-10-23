# Progression: achievement points and the skill tree

**Status:** Built, 2026-10-23. Layout open for review.

- **Achievements give points, not unlocks.** Every achievement is worth exactly 1 point. Nothing unlocks directly from an achievement any more.
- **Skill tree.** 1 point unlocks 1 node; nodes have no other cost. A node is buyable once all its prerequisites are owned.
- **Start.** Only the Ranger with Forward Shot. Everything else comes from the tree.
- **Ships.** The Interceptor and Bastion are nodes in the tree. Unlocking a ship also unlocks its signature weapon and that weapon's Nerve burst; there is no separate unlock.
- **Order along each branch:** weapons, then upgrade helpers (passives), then upgrades (evolutions), then the ship.

## As built

- **Achievements (47):** 500 kills with each weapon (16), max each weapon (16), dash-kill a Turret, an Orbiter and a Laser Fence, 200 dash or Nerve kills, reach wave 5/10/25/50/75/100, defeat each of the five bosses. Earned the moment they happen and saved at once, with a "+1 skill point" toast.
- **Tree (31 nodes), Ranger hub with three branches:**
  - **Offence (right, toward the Interceptor):** Homing, Railgun, Sniper, Flak, Flamethrower, Chain Lightning, Beam; Damage, Cooldown; Forward and Homing evolutions; Interceptor; then the Laser Shots evolution (also needs Move Speed).
  - **Defence (left, toward the Bastion):** Mine, Shock, Turret Deploy, Follower Drone, Laser Drone, Ricochet; Barrier, Max HP; Shock evolution; Bastion; then the Orbit evolution.
  - **Utility (below):** Move Speed, Pickup Radius, weapon slots 4, 5 and 6, Mine evolution.
  - Passives that are not in the tree are always available.
- **Screen:** Skill Tree on the title menu only (not in a run). Hover or the arrow keys pick a node; click or Enter buys it. Owned nodes are lit, buyable ones pulse, long cross-branch links are dotted.
- **Sandbox:** every gate is open. The sandbox menu has "Skill Tree (free)", which buys without points, and "Reset Progress", which clears earned achievements and owned nodes.
- **Saves:** the v3 save holds earned achievements and owned nodes. Old saves keep their stats (kills, highest wave) but lose their unlocks.

## Open

- Node placement and which nodes lead where.
- Can points be refunded outside the sandbox.
