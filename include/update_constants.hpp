#pragma once

#include "update.hpp"
#include <array>
#include <cstdint>

constexpr float frameScale = UpdateConstants::frameScale;

constexpr float projectileSpeed = 10;
constexpr float projectileSize = 7;
constexpr int32_t bossRamDamage = 50;

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

constexpr float continuousDpsMultiplier = 2.1F;
constexpr float orbitBladeHitRadius = 10.0F;

constexpr float beamDamageMult = 1.0F;

constexpr float orbitProjectileSpeed = 11.0F;
constexpr float orbitProjectileRadius = 7.0F;
constexpr float orbitLaunchInterval = 2.2F;
constexpr float orbitRegrowDuration = 0.35F;
constexpr float nerveSpiralSpinSpeed = 14.0F;
constexpr float cameraDespawnMargin = 80.0F;
constexpr float freezeSlowMult = 0.5F;
constexpr float confuseWanderInterval = 1.0F;

constexpr float baseBurnDps = 3.0F;

constexpr float pickupEffectDuration = 60.0F;
constexpr float enemyHealthMult = 1.2F;
constexpr float enemyDamageMult = 1.15F;
constexpr float regenRate = 0.4F;
constexpr float overchargeDuration = 8.0F;
constexpr float overdriveDuration = 8.0F;

constexpr float rockClusterRotationSpeed = 12.0F;
constexpr float overchargeDamageMult = 1.5F;
constexpr float dashTrailRadius = 24.0F;
constexpr int32_t dashTrailDamage = 4;
constexpr float dashTrailParticleLife = 0.9F;
constexpr float flameParticleLife = 0.35F;
constexpr float flameParticleRadius = 16.0F;
constexpr int32_t flameParticlesPerConePerSecond = 40;
constexpr float magnetPulseSnapDistance = 40.0F;
constexpr float dangerDowngradeDuration = 10.0F;

constexpr int32_t dangerPickupChance = 2;

constexpr int32_t rareBonusDropChance = 3;
constexpr int32_t rareBonusCategoryCount =
    6;
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

constexpr float bossRecoveryDuration = 0.5F;

// How long a player has to sit within a boss's ShockwaveStomp blast radius (reused as the
// trigger range - see updateBossMovement) before it auto-punishes the camping.
constexpr float bossMeleeStompTriggerDuration = 2.5F;

constexpr int32_t baseProjectileHealth = 3;

constexpr float beltbreakerRotationSpeed = 18.0F;

constexpr float beltbreakerShieldGenRate = 1.0F / 8.0F;
constexpr float beltbreakerReturnLeadTime = 2.0F;
constexpr float beltbreakerPlateReturnSpeed = 7.5F;
constexpr float beltbreakerReturnArriveDist = 50.0F;

constexpr float beltbreakerPlateMoveSpeedMult = 0.55F;
constexpr float beltbreakerPlateAttackScale = 0.5F;

constexpr float beltbreakerPlateRegenDuration = 1.0F / beltbreakerShieldGenRate;
constexpr float beltbreakerHarassExcursionDuration = 3.0F;

constexpr float beltbreakerPlateHurlDuration = 1.6F;

constexpr float beltbreakerVulnerableDashDamageMult = 2.5F;

constexpr float beltbreakerPlateShieldDashPushDistance = 260.0F;
constexpr float beltbreakerPlateShieldDashShieldDuration = 2.5F;

constexpr int32_t bossHpPerTier = 500;

constexpr float wreckwormSegmentSpacing = 42.0F;

constexpr float wreckwormChainFollowSpeed = 9.0F;
constexpr float wreckwormBurrowChargeDuration = 0.55F;
constexpr float wreckwormCoilClampDuration = 2.6F;
constexpr float wreckwormCoilClampStartRadius = 260.0F;
constexpr float wreckwormCoilClampEndRadius = 40.0F;
constexpr float wreckwormCoilClampGapDeg = 90.0F;

constexpr float wreckwormExposedLullDuration = 1.5F;
constexpr float wreckwormMoltSpeedBoost = 0.15F;
constexpr float wreckwormArmorDamageMult = 0.35F;
constexpr float wreckwormSegmentHealthPerTier = 90.0F;
constexpr float wreckwormArmorHealthMult = 2.0F;

// Infected segments each fire their own weak spore volley on a stagger - the fight was reading
// as "shoot the drifting worm" with only the head as a threat source. Weaker than the head's own
// Barrage (see wreckwormSegmentVolleyDamageMult) since there can be up to 5 of these firing.
constexpr float wreckwormSegmentVolleyIntervalMin = 3.0F;
constexpr float wreckwormSegmentVolleyIntervalMax = 5.0F;
constexpr float wreckwormSegmentVolleyDamageMult = 0.4F;
constexpr float wreckwormSegmentVolleyProjSpeed = 7.0F;

// The Slagmaw (Solar Forge boss) - heat meter (0-1) fills passively, phase boundaries pick which
// reflavored generic attack is live: Ember (< emberEnd) = ChargeDash, Flare (< flareEnd) =
// Barrage/Beam(+HomingBarrage once slagmawHotRoundUnlocked), Vent (>= flareEnd) = one forced Slam
// eruption, then a held crit window before heat resets. Fill rate itself is picked per-wave in
// spawnSlagmaw (60/70/75), same literal-per-tier pattern as spawnWreckworm's segmentCount.
constexpr float slagmawHeatEmberEnd = 0.4F;
constexpr float slagmawHeatFlareEnd = 0.85F;
constexpr float slagmawCritWindowDuration = 2.0F;

// Solar Forge's ambient heat DoT - "open areas" are the default, shadow pockets are the
// exception, matching the locked biome design ("rising ambient heat DoT... with safe shadow
// pockets"). Ticks damagePlayer whenever the player is outside every live pocket's radius.
constexpr float shadowPocketRadius = 130.0F;
constexpr float shadowPocketSpawnIntervalMin = 6.0F;
constexpr float shadowPocketSpawnIntervalMax = 10.0F;
constexpr float solarForgeHeatTickInterval = 1.0F;
constexpr int32_t solarForgeHeatDamage = 1;

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

constexpr float gravityWellPullFraction = 0.88F;
constexpr float gravityWellDuration = 2.0F;

constexpr float gravityWellFanSpinMult = 7.0F;

constexpr std::array<float, static_cast<size_t>(WeaponType::Count)> weaponBaseCooldown{
    0.38F, 0.85F, 1.0F, 0.7F, 0.6F, 1.5F, 0.0F, 0.0F, 0.0F, 0.9F, 1.8F, 1.0F, 6.0F, 0.0F};

constexpr float droneMeleeRange = 60.0F;
constexpr float droneAttackIntervalBase = 0.6F;
constexpr float laserDroneRangeBase = 220.0F;
constexpr float droneOrbitRadius = 70.0F;
constexpr float turretLife = 8.0F;

constexpr float turretFireInterval = 0.7F;
constexpr float flakSplashRadius = 40.0F;
constexpr float flamethrowerRange = 160.0F;
constexpr float flamethrowerHalfAngle = 20.0F * DEG2RAD;
constexpr float flamethrowerOffAngle = 45.0F * DEG2RAD;
constexpr float chainLightningRange = 260.0F;
constexpr float chainLightningBoltLife = 0.2F;

constexpr float forwardDamageMult = 0.5F;
constexpr float mineDamageMult = 0.7F;
constexpr float shockDamageMult = 1.5F;
constexpr float flakCannonDamageMult = 0.35F;

constexpr float railgunBaseDamageMult = 2.6F;
constexpr float chainLightningDamageMult = 1.9F;
constexpr float flamethrowerDamageMult = 2.5F;

constexpr float damageShakeDuration = 0.12F;
constexpr int bossMoveCount = 4;
