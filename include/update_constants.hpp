#pragma once

// Shared gameplay-tuning constants for the update/* subsystems (weapons, bosses, enemies,
// pickups, player abilities...). Split out of update.cpp so entities/boss.cpp, entities/enemy.cpp,
// entities/player.cpp etc. can all see the same numbers without re-declaring them. Pure constexpr
// data (internal linkage per translation unit, safe to include everywhere) - no function
// definitions live here, those stay in whichever .cpp actually uses them.

#include "update.hpp"
#include <array>
#include <cstdint>

constexpr float frameScale = UpdateConstants::frameScale;

constexpr float projectileSpeed = 10;
constexpr float projectileSize = 7;
constexpr int32_t bossRamDamage = 50;
// Large enough that armor reduction (damagePlayer) can never make an intended-lethal hit
// survivable.
constexpr int32_t instakillDamage = 999999;
constexpr float blackHolePull = 2;
constexpr float blackHoleSlow = 2.5F;
constexpr float blackHoleAsteroidPull = 1.5F;
constexpr float blackHoleChaseSpeed = 0.3F;

constexpr float homingProjSpeed = 6.5F;
constexpr float spreadProjSpeed = 6;
constexpr int32_t baseBulletDamage = 6;
constexpr float pickupMagnetSpeed = 7;
constexpr float dashSpeed = 22;
constexpr float dashDuration = 0.18F;
constexpr int32_t dashDamage = 15;
constexpr float dashPushDistance = 60.0F;
// DPS while a continuous weapon (orbit blades, beam) stays in contact, as a multiple of its
// one-time first-contact hit.
// REBALANCE (tools/balance_sim): was 1.2 — Orbit/Beam were the two weakest weapons in the roster
// (16-46 dps vs. a ~80 dps band for everything else at equal investment). Raised so continuous
// weapons land in the same band as cooldown-gated ones instead of being structurally undertuned.
constexpr float continuousDpsMultiplier = 2.1F;
constexpr float orbitBladeHitRadius = 10.0F;
// REBALANCE: was 0.35 — an arbitrary "crowds get hit too so nerf it" discount that, combined with
// the old continuousDpsMultiplier, left Beam as the single worst weapon in the game (~16 dps at
// max level). Beam's crowd exposure is already accounted for in the sim's separate crowd-dps
// metric; removing this discount just brings its single-target dps back to parity.
constexpr float beamDamageMult = 1.0F;

constexpr float orbitProjectileSpeed = 11.0F;
constexpr float orbitProjectileRadius = 7.0F;
constexpr float orbitLaunchInterval = 2.2F;
constexpr float orbitRegrowDuration = 0.35F;
constexpr float nerveSpiralSpinSpeed = 14.0F;
constexpr float cameraDespawnMargin = 80.0F;
constexpr float freezeSlowMult = 0.5F;
constexpr float confuseWanderInterval = 1.0F;
// Elemental debuffs are a helping hand, not a primary damage source.
constexpr float baseBurnDps = 3.0F;
// Duration for temporary player-side pickup effects (elemental Infusion/Field, Regen).
constexpr float pickupEffectDuration = 60.0F;
constexpr float enemyHealthMult = 1.2F;
constexpr float enemyDamageMult = 1.15F;
constexpr float elementalFieldRadius = 90.0F;
constexpr float regenRate = 0.4F;
constexpr float overchargeDuration = 8.0F;
constexpr float overdriveDuration = 8.0F;
// Shattered Belt's rock-cluster hazard (see spawnRockCluster) - degrees/sec.
constexpr float rockClusterRotationSpeed = 12.0F;
constexpr float overchargeDamageMult = 1.5F;
constexpr float dashTrailRadius = 24.0F;
constexpr int32_t dashTrailDamage = 4;
constexpr float dashTrailParticleLife = 0.9F;
constexpr float magnetPulseSnapDistance = 40.0F;
constexpr float dangerDowngradeDuration = 10.0F;
constexpr int32_t dangerDowngradeLevels = 2;
// Out of 1000 rolls, independent of (and rarer than) the "good" rare bonus roll above.
constexpr int32_t dangerPickupChance = 2;

// M26: Second Wind is the catalog's only remaining "permanent" (non-timed) effect - a one-shot
// revive held in reserve until it's actually needed, which a duration would just undermine. Every
// other buff (Dash Trail included, as of this session) is timed. Gets offered more often at
// higher wave numbers instead of adding new pickup types.
// Out of 1000 rolls: very rare, on top of (not instead of) the existing shield/life-orb rolls.
constexpr int32_t rareBonusDropChance = 3;
constexpr int32_t rareBonusCategoryCount =
    6; // Regen/DashTrail/MagnetPulse/Overcharge/SecondWind/Elemental
constexpr float dashKillChargeRefund = 0.6F;
constexpr float shieldKillChargeRefund = 0.6F;
constexpr float bossBodyLingerLimit = 1.0F;
constexpr float damageMeterHoldDuration = 1.5F;

constexpr float chargeTelegraphDuration = 0.5F;
constexpr float enemyChargeDashDuration = UpdateConstants::enemyChargeDashDuration;

constexpr float xpPickupLifetime = 10.0F;
constexpr float bonusPickupLifetime = 18.0F;
constexpr float shieldBaseDuration = 1.2F;
constexpr float entityDespawnRadius = 1400;
constexpr float asteroidDespawnRadius = 900;
constexpr float turretFireRange = 700;
constexpr int32_t crossfireProjectileDamage = 5;
constexpr int32_t shieldDropChance = 15;
constexpr int32_t lifeOrbDropChance = 40;
constexpr int32_t settingsRowCount = 7;
constexpr float nerveKillGain = 6;
constexpr float chargeFeedDelay = 0.6F;
constexpr float chargeFeedDrainRate = 12.0F;
constexpr float chargeFeedRegenMult = 3.0F;
constexpr float nerveBurstLength = 900.0F;
constexpr int32_t nerveBurstDamage = 260;
constexpr float maxHealthPerLevel = 0.4F;
constexpr float pickupRadiusPerLevel = 1.5F;
constexpr float postCapDamageBonusPerLevel = 0.05F;
// REBALANCE: was uncapped — the single biggest cause of "boss dies in a second once you're
// strong". Every level past a maxed-out 6-slot build used to add +5% damage forever with no
// ceiling; now it tops out at +100% (20 levels), which the boss-HP curve above is tuned against.
constexpr int32_t postCapDamageLevelsCap = 20;
constexpr float mineLifetime = 5.0F;
constexpr float mineHomeSpeed = 3.5F;
constexpr float mineSeekRadius = 320;
constexpr float bossEngageDistance = 380;
constexpr float bossKillCalmDuration = 6.0F;
constexpr float bgmDuckDuration = 0.35F;
constexpr float bgmDuckStrength = 0.5F;

constexpr int miniBossWaveInterval = 5;
constexpr int megaBossWaveInterval = 10;
constexpr int finalBossWaveInterval = 100;
constexpr int bossSwarmInterval = 50;
constexpr float miniBossHealthMult = 0.5F;
constexpr float miniBossSizeMult = 0.7F;
constexpr float megaBossHealthMult = 1.5F;
constexpr float megaBossSizeMult = 1.3F;

constexpr float miniBossXpMult = 1.5F;
constexpr float megaBossXpMult = 2.2F;

constexpr int spawnRateCycleLength = 10;
constexpr float spawnRateCycleStep = 0.1F;
constexpr float spawnRateSecondHalfMultiplier = 0.6F;
constexpr float spawnIntervalFloor = 0.15F;

constexpr float enrageHealthFrac = 0.25F;
constexpr float enrageSpeedMult = 0.5F;
// M12 dodge-window fix: forced-still period after an attack resolves, before the boss resumes
// strafing. See Boss::recoveryTimer / updateBossMovement.
constexpr float bossRecoveryDuration = 0.5F;

constexpr int32_t baseProjectileHealth = 3;

// The Beltbreaker (Shattered Belt signature boss). Each plate is a real Boss entry
// (isBeltbreakerPlate) orbiting the core; see Boss::plateAttached in boss.hpp for the full cycle.
constexpr float beltbreakerRotationSpeed = 18.0F; // degrees/sec
// Full shield generation takes plateCount/beltbreakerShieldGenRate seconds when every plate is
// alive and attached - killing plates directly slows this (see updateBeltbreakerCore).
constexpr float beltbreakerShieldGenRate = 1.0F / 8.0F;
constexpr float beltbreakerReturnLeadTime = 2.0F;
constexpr float beltbreakerPlateReturnSpeed = 7.5F;
constexpr float beltbreakerReturnArriveDist = 50.0F;
// A detached plate is a "mini boss," not a full one - it used the same chase speed and the same
// full-strength Barrage/ChargeDash as the core itself, which read as too fast and too strong for
// something the player is meant to be able to shrug off individually while the core is the real
// threat. Slows its chase/dash and softens its attack; see updateBossMovement/updateBoss.
constexpr float beltbreakerPlateMoveSpeedMult = 0.55F;
constexpr float beltbreakerPlateAttackScale = 0.5F;
// Full regen (0 -> max) takes as long as a fully-staffed shield generation does - the same
// number quoted for both, per design. A plate below max simply doesn't count toward
// countContributingPlates until it's back to full (see updateBeltbreakerCore).
constexpr float beltbreakerPlateRegenDuration = 1.0F / beltbreakerShieldGenRate;
constexpr float beltbreakerHarassExcursionDuration = 3.0F;
// PlateHurl: plates get launched at (roughly) the core's own full-strength ChargeDash speed, not
// their usual softened one - it's the core spending its own strength through them for a moment.
constexpr float beltbreakerPlateHurlDuration = 1.6F;
// Landing a shield-dash on the core at the exact moment it's actually vulnerable (no attached
// plates, shield not yet up) hits this much harder than an ordinary boss ram.
constexpr float beltbreakerVulnerableDashDamageMult = 2.5F;
// Shield-dashing a plate (dash + shield-block together) knocks it clean out of the attach/regen
// loop instead of just chipping it - permanently detaches it (no excursion timer, so it never
// auto-returns to heal), shoves it much further than a normal dash push, refunds the full charge
// spent, and grants a few seconds of shield uptime as a reward for landing it.
constexpr float beltbreakerPlateShieldDashPushDistance = 260.0F;
constexpr float beltbreakerPlateShieldDashShieldDuration = 2.5F;

// REBALANCE: was 250. Doubled to match the now-capped (not uncapped) player damage growth — keeps
// wave-100 time-to-kill healthy (~7-8s for an on-track build) while still producing a genuine,
// ever-escalating wall deep into infinite mode once postCapDamageLevelsCap is reached and player
// power stops growing but boss HP keeps climbing linearly forever.
constexpr int32_t bossHpPerTier = 500;

// The Wreckworm (Rustbloom signature boss). Each segment is a real Boss entry
// (isWreckwormSegment) chain-following the one ahead of it; see Boss::segmentOwnerId in boss.hpp.
constexpr float wreckwormSegmentSpacing = 42.0F;
// Comfortably faster than a chasing head ever moves (see the chase speed math in
// updateBossMovement) so the chain catches back up after a BurrowCharge instead of stretching
// forever.
constexpr float wreckwormChainFollowSpeed = 9.0F;
constexpr float wreckwormBurrowChargeDuration = 0.55F;
constexpr float wreckwormCoilClampDuration = 2.6F;
constexpr float wreckwormCoilClampStartRadius = 260.0F;
constexpr float wreckwormCoilClampEndRadius = 40.0F;
constexpr float wreckwormCoilClampGapDeg = 90.0F;
// ~1.5s full-chain stagger right after a Molt - the fight's designed crit window. Reuses
// Boss::recoveryTimer (holds movement still) plus a matching attackTimer reset (holds attacks
// off) rather than adding new state.
constexpr float wreckwormExposedLullDuration = 1.5F;
constexpr float wreckwormMoltSpeedBoost = 0.15F;
constexpr float wreckwormArmorDamageMult = 0.35F;
constexpr float wreckwormSegmentHealthPerTier = 90.0F;
constexpr float wreckwormArmorHealthMult = 2.0F;

constexpr float wormholeRadius = 26;
constexpr float wormholeLifetime = 20.0F;
constexpr float wormholeSpawnCooldownMin = 40.0F;
constexpr float wormholeSpawnCooldownMax = 70.0F;

constexpr float wormholeBeamDuration = 2.5F;
constexpr float wormholeBeamFlankMinDist = 150;
constexpr float wormholeBeamFlankMaxDist = 260;
constexpr float beamAttackDuration = 4.0F;

constexpr float eliteHazardSpawnIntervalMin = 45.0F;
constexpr float eliteHazardSpawnIntervalMax = 90.0F;
constexpr float eliteHazardOrbitDistFrac = 0.85F;
constexpr float eliteHazardOrbitSpin = 6.0F;
constexpr float eliteHazardFollowRate = 1.2F;
constexpr int32_t eliteHazardBaseHealth = 220;
constexpr int32_t eliteHazardScore = 150;
constexpr int32_t eliteHazardXpBonus = 300;
constexpr int32_t eliteHazardContactDamage = 2;
constexpr float warlordSpeedBuff = 1.35F;
constexpr float suppressorCooldownPenalty = 1.4F;

constexpr int mineDropCount = 4;
constexpr float mineDropRadius = 30;
constexpr int32_t mineDropDamage = 12;

constexpr float chargeDashSpeed = 4400;
constexpr float chargeDashDuration = 0.5F;

constexpr int summonAddsCount = 4;
constexpr float barrageFireInterval = 0.3F;
constexpr float barrageDuration = 2.5F;

constexpr float barrageProjSpeed = 8;
constexpr float homingBarrageFireInterval = 0.5F;

constexpr int spreadBulletsPerRound = 10;
constexpr int spreadRounds = 6;
constexpr float spreadRoundInterval = 0.25F;
// Was a flat 1.6 - well under any player's actual speed (4-6.5), so simply moving away outran it
// with room to spare and the attack never threatened anyone. Scaling with the player's own speed
// means only holding the exact opposite direction lets them crawl free; anything else nets pulled
// in, same as the Beltbreaker's own GravityWell rework.
constexpr float gravityWellPullFraction = 0.88F;
constexpr float gravityWellDuration = 2.0F;
// Beltbreaker-only: how much faster the ring spins on top of its normal rotation while GravityWell
// is dragging the player toward it - a real "fan" hazard, not just cosmetic.
constexpr float gravityWellFanSpinMult = 7.0F;

// Indices 6-13 are the M15 additions: Ricochet/FollowerDrone/LaserDrone are amplifiers with no
// fire-cooldown of their own (never read from this table), the rest are new primary weapons.
// REBALANCE: ChainLightning was 2.2 — cut to 1.0, it was tied for the weakest weapon in the roster.
constexpr std::array<float, static_cast<size_t>(WeaponType::Count)> weaponBaseCooldown{
    0.38F, 0.85F, 1.0F, 0.7F, 0.6F, 1.5F, 0.0F, 0.0F, 0.0F, 0.9F, 1.8F, 1.0F, 6.0F, 0.0F};

constexpr float droneMeleeRange = 60.0F;
constexpr float droneAttackIntervalBase = 0.6F;
constexpr float laserDroneRangeBase = 220.0F;
constexpr float droneOrbitRadius = 70.0F;
constexpr float turretLife = 8.0F;
// REBALANCE: was 0.8 — small buff, Turret Deploy was slightly under the target band.
constexpr float turretFireInterval = 0.7F;
constexpr float flakSplashRadius = 40.0F;
constexpr float flamethrowerRange = 160.0F;
constexpr float flamethrowerHalfAngle = 20.0F * DEG2RAD;
constexpr float flamethrowerOffAngle = 45.0F * DEG2RAD;
constexpr float chainLightningRange = 260.0F;
constexpr float chainLightningBoltLife = 0.2F;

// REBALANCE (tools/balance_sim, 2026-08-01): per-weapon damage-shape multipliers tuned so all 14
// weapons land in a ~77-122 dps band at Warhead Tuning 5 + Overclock 5, no evolution (was a
// 16-1520 dps range — Photon Cannon was 94x the weakest weapon). Re-run the simulator before
// changing any of these; they were converged on together, not picked in isolation.
constexpr float forwardDamageMult = 0.5F;     // was implicit 1.0 (full weaponDamage per shot)
constexpr float mineDamageMult = 0.7F;        // was implicit 1.0
constexpr float shockDamageMult = 1.5F;       // was implicit 1.0
constexpr float flakCannonDamageMult = 0.35F; // was implicit 1.0 — pellet count made it the 3rd
                                              // strongest weapon in the game
constexpr float railgunBaseDamageMult = 2.6F; // was a bare "* 2" literal at the fire site
constexpr float chainLightningDamageMult = 1.9F; // was implicit 1.0
constexpr float flamethrowerDamageMult = 2.5F;   // was a bare "* 2.0F" literal at the fire site

// M22: screen shake proportional to damage dealt, layered on top of (not replacing) the existing
// hand-tuned per-event shakes elsewhere.
constexpr float damageShakeDuration = 0.12F;
constexpr int bossMoveCount = 4;

