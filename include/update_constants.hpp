#pragma once

#include "update.hpp"
#include <array>
#include <cstdint>

constexpr float frameScale = UpdateConstants::frameScale;

constexpr float projectileSpeed = 10;
constexpr float projectileSize = 7;
constexpr int32_t bossRamDamage = 50;
constexpr float shieldDashBossShakeCooldown = 1.0F;

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

constexpr float holdChunkBaseDuration = 1.0F;
constexpr float holdChunkPerLevel = 0.03F;
constexpr float dashChunkCap = 1.5F;
constexpr float comboClickWindow = 0.15F;
constexpr int32_t comboBaseCost = 2;
constexpr float parryStunDuration = 2.0F;
constexpr float deflectedBulletSpeedMult = 1.2F;

constexpr float continuousDpsMultiplier = 2.1F;
constexpr float orbitBladeHitRadius = 10.0F;

constexpr float beamDamageMult = 1.0F;

constexpr float orbitProjectileSpeed = 11.0F;
constexpr float orbitProjectileRadius = 7.0F;
constexpr float orbitRegrowDuration = 0.35F;
constexpr float nerveSpiralSpinSpeed = 14.0F;
constexpr float cameraDespawnMargin = 80.0F;
constexpr float freezeSlowMult = 0.5F;

constexpr float baseBurnDps = 3.0F;

constexpr float pickupEffectDuration = 60.0F;
constexpr float enemyHealthMult = 1.2F;
constexpr float enemyDamageMult = 1.15F;

constexpr float dpsHealthScaleReference = 55.0F;
constexpr float dpsHealthScaleCap = 3.0F;

constexpr double sfxThrottleInterval = 0.045;
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
constexpr float shieldDashKillGraceDuration = 1.0F;

constexpr int32_t blackHoleDustCount = 250;
constexpr int32_t blackHoleDustArmCount = 7;
constexpr float blackHoleDustCoreFraction = 0.22F;
constexpr float blackHoleDustTwistStrength = 4.0F;
constexpr float blackHoleDustInwardSpeedMin = 0.05F;
constexpr float blackHoleDustInwardSpeedMax = 0.22F;
constexpr float bossBodyLingerLimit = 1.0F;
constexpr float damageMeterHoldDuration = 1.5F;

constexpr float chargeTelegraphDuration = 0.5F;
constexpr float enemyChargeDashDuration = UpdateConstants::enemyChargeDashDuration;

constexpr float xpPickupLifetime = 10.0F;
constexpr float bonusPickupLifetime = 18.0F;
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
constexpr float nerveConeHalfAngleDeg = 32.0F;
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

constexpr float wreckwormSegmentSpacing = 80.0F;
constexpr float wreckwormChaseSpeedMult = 0.75F;

constexpr float wreckwormBurrowChargeDuration = 0.55F;
constexpr float wreckwormCoilClampDuration = 2.6F;
constexpr float wreckwormCoilClampStartRadius = 260.0F;
constexpr float wreckwormCoilClampEndRadius = 40.0F;
constexpr float wreckwormCoilClampGapDeg = 90.0F;
constexpr float wreckwormCoilClampAssembleDuration = 1.5F;
constexpr float wreckwormCoilClampReleaseDuration = 1.5F;
constexpr float wreckwormCoilClampTotalDuration = wreckwormCoilClampAssembleDuration +
                                                  wreckwormCoilClampDuration +
                                                  wreckwormCoilClampReleaseDuration;

constexpr float wreckwormDetachDuration = 0.35F;
constexpr float wreckwormDetachSpeed = 900.0F;

constexpr float wreckwormTailWrapCoilInDuration = 1.2F;
constexpr float wreckwormTailWrapChaseDuration = 2.5F;
constexpr float wreckwormTailWrapReleaseDuration = 1.0F;
constexpr float wreckwormTailWrapDuration = wreckwormTailWrapCoilInDuration +
                                            wreckwormTailWrapChaseDuration +
                                            wreckwormTailWrapReleaseDuration;
constexpr float wreckwormTailWrapRadius = 150.0F;
constexpr float wreckwormTailWrapSpinSpeedDeg = 90.0F;
constexpr float wreckwormTailWrapMoveSpeed = 260.0F;
constexpr float wreckwormTailWrapCoilSpeedDeg = 160.0F;

constexpr float wreckwormExposedLullDuration = 1.5F;
constexpr float wreckwormMoltSpeedBoost = 0.15F;
constexpr float wreckwormArmorDamageMult = 0.35F;
constexpr float wreckwormSegmentHealthPerTier = 90.0F;
constexpr float wreckwormArmorHealthMult = 2.0F;

constexpr float wreckwormSegmentVolleyIntervalMin = 3.0F;
constexpr float wreckwormSegmentVolleyIntervalMax = 5.0F;
constexpr float wreckwormSegmentVolleyDamageMult = 0.4F;
constexpr float wreckwormSegmentVolleyProjSpeed = 7.0F;

constexpr float meteorHellSpawnDistMin = 500.0F;
constexpr float meteorHellSpawnDistMax = 700.0F;
constexpr float meteorHellSpawnInterval = 1.0F;
constexpr int32_t meteorHellSpawnPerBatchMin = 6;
constexpr int32_t meteorHellSpawnPerBatchMax = 8;
constexpr float meteorHellProjSpeed = 2.2F;
constexpr float meteorHellHealthMult = 1.6F;

constexpr auto meteorHellDuration(int32_t wave) -> float
{
    if (wave >= 75)
    {
        return 7.0F;
    }
    if (wave >= 70)
    {
        return 5.0F;
    }
    return 3.0F;
}

constexpr float slagmawMoveSpeedMult = 0.32F;
constexpr float ghostFadeDuration = 2.2F;
constexpr float ghostTeleportMargin = 120.0F;

constexpr int32_t meteorSwarmSpawnCount = 16;

constexpr float caveNoiseScaleCoarse = 380.0F;
constexpr float caveNoiseScaleFine = 60.0F;
constexpr float caveTunnelHalfWidth = 0.17F;

constexpr float rustbloomPodCellSize = 140.0F;
constexpr float rustbloomPodChance = 0.28F;

constexpr float solarForgeMeltThreshold = 10.0F;
constexpr float solarForgeHeatDecayRate = 2.5F;
constexpr float solarForgeMeltTickInterval = 0.5F;
constexpr int32_t solarForgeMeltDamage = 2;

constexpr float solarForgeFluidBurnDps = 6.0F;
constexpr float solarForgeFluidBurnRefreshDuration = 1.0F;

constexpr float orbitAsteroidRetargetSpeed = 240.0F;

constexpr int32_t solarForgeEnemyFireChancePercent = 10;
constexpr float solarForgeEnemyFireParticleLife = 0.4F;
constexpr float solarForgeEnemyFireParticleRadius = 5.0F;
constexpr float solarForgeEnemyFireParticleSpeed = 18.0F;

constexpr int32_t organicVertexCount = 10;
constexpr float organicWobbleAmplitude = 0.22F;
constexpr float organicWobbleSpeed = 2.0F;

constexpr float organicMergeCheckInterval = 2.5F;
constexpr int32_t organicMergeChancePercent = 8;
constexpr float organicMergeRange = 90.0F;
constexpr int32_t organicMergeMaxCount = 3;
constexpr float organicMergeRadiusGrowth = 0.22F;

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
    0.38F, 0.85F, 1.0F, 0.7F, 0.6F, 1.5F, 0.0F, 0.0F, 0.0F, 0.9F, 1.8F, 1.0F, 6.0F, 0.0F, 2.0F};

constexpr float droneMeleeRange = 60.0F;
constexpr float droneAttackIntervalBase = 0.6F;
constexpr float laserDroneRangeBase = 220.0F;
constexpr float droneOrbitRadius = 70.0F;
constexpr float droneContactRadius = 16.0F;
constexpr float droneContactDpsMult = 0.5F;
constexpr float droneShotInterval = 0.9F;
constexpr float droneShotSpeed = 13.0F;
constexpr float droneShotDamageMult = 1.4F;
constexpr float turretLife = 8.0F;

constexpr float turretFireInterval = 0.7F;
constexpr float flakSplashRadius = 40.0F;
constexpr float flamethrowerRange = 260.0F;
constexpr float flamethrowerHalfAngle = 20.0F * DEG2RAD;
constexpr float flamethrowerOffAngle = 45.0F * DEG2RAD;
constexpr float chainLightningRange = 260.0F;
constexpr float chainLightningBoltLife = 0.2F;

constexpr float forwardDamageMult = 0.5F;
constexpr float mineDamageMult = 0.7F;
constexpr float shockDamageMult = 1.5F;
constexpr float flakCannonDamageMult = 0.35F;

constexpr float railgunBaseDamageMult = 2.6F;

constexpr float sniperDamageMult = 2.3F;

constexpr float sniperLineLength = 2000.0F;
constexpr float sniperLineFlashDuration = 0.2F;
constexpr float chainLightningDamageMult = 1.9F;
constexpr float flamethrowerDamageMult = 2.5F;

constexpr float damageShakeDuration = 0.12F;
constexpr int bossMoveCount = 4;

constexpr float punctumSpawnIntervalMult = 3.0F;
constexpr int punctumSpawnCountDivisor = 2;
constexpr float punctumEnemyHealthMult = 1.8F;

constexpr float punctumPullSpeed = 2.6F;
constexpr float punctumPullAngleDeg = 205.0F;

constexpr float punctumThunderIntervalMin = 7.0F;
constexpr float punctumThunderIntervalMax = 11.0F;
constexpr float punctumThunderFlashDuration = 0.28F;
constexpr int32_t punctumThunderSpeedupWaveA = 87;
constexpr int32_t punctumThunderSpeedupWaveB = 97;

constexpr float punctumTrapHalfWidth = static_cast<float>(GameConstants::defaultWindowWidth);
constexpr float punctumTrapHalfHeight = static_cast<float>(GameConstants::defaultWindowHeight);
constexpr float punctumTrapIdlePullSpeed = 0.08F;
constexpr float punctumTrapSpiralInwardSpeed = 1.4F;
constexpr float punctumTrapSpiralSwirlSpeed = 1.1F;
constexpr float punctumTrapConsumeRadius = 40.0F;

constexpr float krakenSize = 520.0F;
constexpr float krakenTentacleCheckInterval = 5.0F;
constexpr float krakenTentacleTriggerChance = 0.5F;
constexpr float krakenTentacleEmergeDuration = 0.4F;
constexpr float krakenTentacleSwipeDuration = 0.8F;
constexpr float krakenTentacleRetreatDuration = 0.35F;
constexpr float krakenTentacleStaggerRetreatDuration = 0.5F;
constexpr float krakenTentacleCooldownDuration = 15.0F;
constexpr float krakenTentacleReach = 260.0F;
constexpr float krakenTentacleSwipeArcDeg = 140.0F;
constexpr float krakenTentaclePortalRadius = 34.0F;
constexpr float krakenTentacleHitRadius = 26.0F;
constexpr int32_t krakenTentaclePlayerDamage = 2;
constexpr int32_t krakenTentacleParryDamage = 180;

constexpr float krakenLimbOpenDuration = 0.7F;
constexpr float krakenLimbActiveDuration = 10.0F;
constexpr float krakenLimbRetreatDuration = 0.6F;
constexpr float krakenLimbCooldownDuration = 4.0F;
constexpr float krakenLimbGrabInterval = 2.2F;
constexpr float krakenLimbNearReach = 170.0F;
constexpr float krakenLimbHurlSpeed = 9.0F;
constexpr int32_t krakenLimbHurlDamage = 3;
constexpr float krakenLimbPortalOffset = 90.0F;
constexpr float krakenLimbLeanBackDuration = 0.3F;
constexpr float krakenLimbThrowDuration = 0.35F;
constexpr float krakenLimbSizeMult = 2.0F;

constexpr int hiveNodeDeathSpawnCount = 5;

constexpr int maxActiveRangedEnemies = 2;

constexpr float banishedSize = 640.0F;

constexpr float banishedNearReach = 220.0F;
constexpr float banishedFarPortalDist = 420.0F;
constexpr float banishedTentacleWindup = 0.6F;
constexpr float banishedTentacleStrike = 0.5F;
constexpr float banishedTentacleRetreat = 0.4F;
constexpr float banishedTentacleCooldownMin = 2.0F;
constexpr float banishedTentacleCooldownMax = 4.5F;
constexpr float banishedTentacleHitRadius = 24.0F;
constexpr int32_t banishedTentacleDamage = 4;
constexpr float banishedGrabChance = 0.35F;
constexpr float banishedGrabDuration = 4.0F;
constexpr float banishedGrabRadius = 130.0F;
constexpr float banishedGrabAngularSpeed = 3.2F;
constexpr float banishedGrabChipInterval = 0.6F;
constexpr int32_t banishedGrabChipDamage = 2;
constexpr float banishedGrabEscapeDot = -0.4F;

constexpr float banishedEyeChargeDuration = 5.0F;
constexpr float banishedEyeCooldownAfterFire = 3.0F;
constexpr int32_t banishedEyeBurstDamageDivisor = 6;
constexpr int32_t banishedEyeShieldDashDamageDivisor = 12;

constexpr float banishedFleeSpeed = 2.4F;
constexpr float banishedEyeFireDuration = 1.0F;
constexpr float banishedBeamDps = 6.0F;

constexpr float krakenBreakDuration = 1.2F;
constexpr float krakenFleeDuration = 1.8F;
constexpr float krakenDialogueDuration = 2.5F;
constexpr float krakenFleeSpeed = 700.0F;

constexpr float krakenLimbGrabAnimDuration = 1.5F;

constexpr float krakenSummonIntervalE1 = 9.0F;
constexpr int32_t krakenSummonCountE1 = 8;
constexpr float krakenSummonIntervalE2 = 12.0F;
constexpr int32_t krakenSummonCountE2 = 3;
constexpr float krakenSummonHealthMultE2 = 2.5F;
constexpr float krakenSnakeSpawnDelay = 4.0F;
constexpr float krakenSnakeHealthMult = 0.35F;
constexpr int32_t krakenSnakeSegmentCount = 6;

constexpr float endingDriftSpeed = 6.0F;

constexpr float bossDeathBlinkDuration = 0.5F;
constexpr float bossDeathCrackDuration = 0.7F;
constexpr float bossDeathGatherDuration = 0.6F;
constexpr float bossDeathAnimDuration =
    bossDeathBlinkDuration + bossDeathCrackDuration + bossDeathGatherDuration;

constexpr int32_t slagmawBreakDrifterCount = 14;
constexpr float slagmawBrokenDrifterHealthMult = 10.0F;

constexpr float wreckwormDashSpeedMult = 0.4F;
constexpr float wreckwormDashRushGapDuration = 0.5F;
constexpr float wreckwormDashRushOffscreenMargin = 150.0F;
constexpr float wreckwormDashRepositionSpeed = 900.0F;

constexpr auto wreckwormDashRushCount(int32_t wave) -> int32_t
{
    if (wave >= 50)
    {
        return 12;
    }
    if (wave >= 40)
    {
        return 8;
    }
    return 4;
}

constexpr float wreckwormTrailSampleDist = 4.0F;

constexpr int32_t wreckwormCloudBreathCount = 5;
constexpr float wreckwormCloudRadiusMin = 55.0F;
constexpr float wreckwormCloudRadiusMax = 90.0F;
constexpr float wreckwormCloudSpawnSpread = 140.0F;
constexpr float wreckwormCloudDriftSpeed = 45.0F;
constexpr float wreckwormCloudLifespan = 14.0F;
constexpr float wreckwormCloudBurnDps = 8.0F;
constexpr float wreckwormCloudBurnRefreshDuration = 1.0F;
