#pragma once

#include "entities/boss.hpp"
#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "game.hpp"
#include "raylib.h"
#include <optional>
#include <utility>
#include <vector>

namespace UpdateConstants
{
constexpr float nerveMax = 100;
constexpr float chargeRegenTime = 3.0;
constexpr int32_t maxCharges = 2;
constexpr float slamDuration = 1.6F;
constexpr float bossDeathShockwaveDuration = 3.0F;
constexpr float shockFlashDuration = 0.4F;
constexpr float waveEnemyScalePerWave = 0.035F;
constexpr float pickupExpiryWarning = 2.5F;
constexpr float shockwaveStompRadius = 220;
constexpr float shockwaveStompDuration = 0.5F;
constexpr float shieldCooldownDuration = 2.0F;
constexpr int maxEnemies = 200;
constexpr float enemyChargeDashDuration = 0.4F;
constexpr float enemyChargeDashSpeedMult = 6.0F;
constexpr float frameScale = 60.0F;
constexpr float hitFlashDuration = 0.12F;
constexpr float nerveBurstWindup = 0.35F;
constexpr float beltbreakerPlateHealthPerTier = 220.0F;
// 4/6/8 plates (wave 10/20/25 tiers) -> 12/16/20s shielded, matching updateBeltbreakerCore.
constexpr auto beltbreakerShieldedDuration(int32_t plateCount) -> float
{
    return 12.0F + static_cast<float>(plateCount - 4) * 2.0F;
}
}

struct WaveHitResult
{
    bool resolved;
    bool hit;
};

void triggerShake(Game& game, float intensity, float duration);
void triggerHitPause(Game& game, float duration);
void duckBGM(Game& game);
auto updateTitle(Game& game) -> bool;
auto updateShipSelect(Game& game) -> bool;
auto updatePaused(Game& game) -> bool;
auto updateGameOver(Game& game) -> bool;
auto updateLevelUp(Game& game) -> bool;
void playMenuSounds(Game& game, int32_t newIndex, bool confirmed);
void playSFX(Game& game, Sound sound);
auto updateSettings(Game& game) -> bool;
void cycleResolution(Game& game, int32_t dir);
void cycleFPS(Game& game, int32_t dir);
void cycleHudScale(Game& game, int32_t dir);
void gainNerve(Game& game);
void updateNerve(Game& game, float deltaTime);
void updateNerveBurstInput(Game& game);
void fireNerveBurst(Game& game);
void fireNerveBeam(Game& game);
void fireNerveBladeTornado(Game& game);
void fireNerveBall(Game& game);
void nerveLineHit(Game& game, Vector2 start, Vector2 end, int32_t dmg, float extraRadius = 0);
void updateOrbitBladeContact(Game& game, float deltaTime);
void updateOrbitBladeLaunch(Game& game, float deltaTime);
void updatePiercingProjectiles(Game& game, float deltaTime,
                               std::vector<OrbitBladeProjectile>& projectiles, DamageSource source,
                               bool despawnOnCameraExit);
void updateOrbitBladeProjectiles(Game& game, float deltaTime);
void updateNerveBallProjectiles(Game& game, float deltaTime);
void updateNerveSpiralProjectiles(Game& game, float deltaTime);
void updateBeamContact(Game& game, float deltaTime);
void damagePlayer(Game& game, int32_t amount);
void spawnDeathExplosion(Game& game);
void updateDeathParticles(Game& game, float deltaTime);
void updateGameplay(Game& game, float deltaTime);
auto resolveExpandingWaveHit(Game& game, Vector2 from, float radius,
                             bool allowDash) -> WaveHitResult;
void updateBossDeathShockwave(Game& game, float deltaTime);
void updatePlayerMovement(Game& game, float deltaTime);
void updateShieldAndBarrier(Game& game, float deltaTime);
void updateAbilityCharges(Game& game, float deltaTime);
auto nearestEnemy(const Game& game, Vector2 from) -> std::optional<Vector2>;
auto nearestEnemyExcluding(const Game& game, Vector2 from,
                           Vector2 exclude) -> std::optional<Vector2>;
auto weaponCooldown(const Game& game, WeaponType kind, int32_t level) -> float;
auto weaponDamage(const Game& game, int32_t level) -> int32_t;
auto ricochetLevel(const Game& game) -> int32_t;
auto followerDroneDamageFraction(int32_t level) -> float;
auto laserDroneDamageFraction(int32_t level) -> float;
void updateFollowerDrones(Game& game, float deltaTime);
void updateLaserDrones(Game& game, float deltaTime);
void updateTurrets(Game& game, float deltaTime);
void updateFlamethrower(Game& game, float deltaTime);
void updateChainLightningBolts(Game& game, float deltaTime);
void fireChainLightning(Game& game, const Weapon& weapon);
auto mineCount(int32_t level, bool evolved) -> int;
auto mineRadius(bool evolved) -> float;
auto mineDamage(const Game& game, int32_t level, bool evolved) -> int32_t;
void spawnMines(Game& game, const Weapon& weapon);
void updateMines(Game& game, float deltaTime);
auto nearestEnemyWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>;
auto nearestAsteroidWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>;
void updateWeapons(Game& game, float deltaTime);
void recordDamage(Game& game, DamageSource source, int32_t amount);
auto currentBurnDps(const Game& game) -> float;
void applyElementDebuff(Enemy& enemy, ElementType element, float burnDps);
void applyActiveElementalDebuffs(Game& game, Enemy& enemy);
void applyElementDebuff(Boss& boss, ElementType element, float burnDps);
void applyActiveElementalDebuffs(Game& game, Boss& boss);
void collectElementalPickup(Game& game, ElementType element, ElementMechanism mechanism);
void updateElementalFields(Game& game, float deltaTime);
void updatePlayerBuffs(Game& game, float deltaTime);
void aoePulse(Game& game, Vector2 center, float radius, int32_t dmg, DamageSource source,
              float knockback = 0);
void updateWaveSpawner(Game& game, float deltaTime);
void updateEliteHazards(Game& game, float deltaTime);
void damageEliteHazard(Game& game, size_t index, int32_t amount, bool applyShake = true);
auto enemyDamage(const Game& game, int32_t base) -> int32_t;
void spawnEnemy(Game& game);
void spawnBossWave(Game& game, float healthMult, float sizeMult, bool isMega);
void spawnFinalBossWave(Game& game);
auto sampleBossMoveset(int count) -> std::vector<BossAttack>;
auto spawnRingPosition(const Game& game) -> Vector2;
void updateBossMovement(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter);
void updateBeltbreakerCore(Game& game, Boss& core, float deltaTime);
auto countAttachedAlivePlates(Game& game, const Boss& core) -> int32_t;
auto beltbreakerPlateSlotPosition(const Boss& core, int32_t slotIndex) -> Vector2;
auto findBeltbreakerById(Game& game, int32_t instanceId) -> Boss*;
void updateWreckwormChain(Game& game, Boss& head, float deltaTime);
auto findWreckwormSegment(Game& game, int32_t headId, int32_t segmentIndex) -> Boss*;
void triggerWreckwormMolt(Game& game, Boss& head);
void updatePickups(Game& game, float deltaTime);
void applyPickupEffect(Game& game, PickupType type, ElementType element, ElementMechanism mechanism,
                       const Pickup* except = nullptr);
void collectLifeOrb(Game& game);
auto equippedSlotCount(const Game& game) -> int;
auto sampleDistinct(std::vector<SkillType> ids, int count) -> std::vector<SkillType>;
auto rollLevelUpChoices(Game& game) -> std::vector<LevelUpChoice>;
auto rollRewardChoices(const Game& game) -> std::vector<LevelUpChoice>;
void applySkill(Game& game, SkillType id);
void grantOrLevelWeapon(Game& game, WeaponType kind);
void applyEvolution(Game& game, WeaponType kind);
void updateBullets(Game& game, float deltaTime);
void damageEnemy(Game& game, size_t index, int32_t amount);
void damageBoss(Game& game, Boss& boss, int32_t amount, bool applyShake = true);
void killEnemyForBossAttack(Game& game, size_t index, bool alwaysLoot);
void spawnRareBonusPickup(Game& game, Vector2 position);
void spawnRareDangerPickup(Game& game, Vector2 position);
void updateAsteroids(Game& game, float deltaTime);
void updateEnemies(Game& game, float deltaTime);
void updateProjectiles(Game& game, float deltaTime);
void filterDeadEntities(Game& game);
void updateBgParticles(Game& game);
void updateBlackHole(Game& game, float deltaTime);
void updateWormhole(Game& game, float deltaTime);
auto applyWormholeTransit(const Game& game, Vector2& position, Vector2& velocity,
                          float entityRadius) -> bool;
void updateBoss(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter);
void startBossAttack(Game& game, Boss& boss, Vector2 bossCenter);
void processBeamAttack(Game& game, Boss& boss, Vector2 beamStart, Vector2 beamEnd);
void updateBgmLayers(Game& game, float deltaTime);


auto nerveFrac(const Game& game) -> float;
auto isNerveChargeFeeding(const Game& game) -> bool;
auto chargeRegenDuration(const Game& game) -> float;
auto aimAtMouse(const Game& game) -> Vector2;
auto mouseWorldPos(const Game& game) -> Vector2;
auto orbitRadius(int32_t level) -> float;
auto orbitBladeCount(int32_t level) -> int32_t;
auto orbitBladePosition(const Game& game, Vector2 center, float radius, int32_t index,
                        int32_t count) -> Vector2;
auto shockwaveRadius(int32_t level, bool evolved) -> float;
auto beamAimDirection(const Game& game, bool evolved) -> Vector2;
auto beamLength(const Game& game, int32_t level, bool evolved) -> float;
auto flamethrowerConeDirs(const Game& game, int32_t level) -> std::vector<Vector2>;
auto flamethrowerRangeFor(int32_t level) -> float;
auto flamethrowerHalfAngleRad() -> float;
auto flamethrowerActive(int32_t level) -> bool;
auto confusePulseActive(float activeDuration) -> bool;
auto waveEnemyScale(const Game& game) -> float;
auto isFusedPassive(const Game& game, SkillType id) -> bool;
auto hasWeapon(const Game& game, WeaponType kind) -> bool;
auto weaponForGrantSkill(SkillType id) -> std::optional<WeaponType>;
auto weaponEvolved(const Game& game, WeaponType kind) -> bool;
auto bossWindupDuration(BossAttack attack) -> float;
auto wormholeBentBeamSegments(const Game& game, Vector2 start,
                              Vector2 end) -> std::vector<std::pair<Vector2, Vector2>>;

auto isConsumablePickup(PickupType type) -> bool;
auto consumablePickupBiasWeight(int32_t wave) -> float;
auto pickupDurationScale(int32_t wave) -> float;
auto pickWeightedPickupIndex(int32_t wave) -> size_t;
auto damageShakeIntensity(int32_t amount) -> float;
void spawnDamageNumber(Game& game, Vector2 position, int32_t amount);
void updateDamageNumbers(Game& game, float deltaTime);
void updateWeaponDowngrade(Game& game, float deltaTime);
auto updateAchievements(Game& game) -> bool;
void spawnKillExplosion(Game& game, Vector2 position, Color color, int32_t particleCount,
                        float speedScale);
void detonateMine(Game& game, size_t index);
auto isOutsideCameraView(const Game& game, Vector2 pos, float margin) -> bool;
void spawnRockCluster(Game& game, Vector2 center);
void spawnBossInstance(Game& game, float healthMult, float sizeMult, bool isMega, bool isSwarm,
                       bool isFinal = false);
auto beltbreakerCoreCenter(const Boss& core) -> Vector2;
auto countContributingPlates(Game& game, const Boss& core) -> int32_t;
void detachBeltbreakerPlates(Game& game, const Boss& core);
void triggerBeltbreakerReturn(Game& game, const Boss& core);
void sendPlateOnExcursion(Boss& plate, float durationSeconds);
auto demoSkillAllowed(const Game& game, SkillType id) -> bool;
auto droneCountForLevel(int32_t level) -> size_t;

auto UpdateGame(Game& game, float deltaTime) -> bool;

void applyBGMState(Game& game);
void startLevelUp(Game& game);
void spawnBoss(Game& game);
void spawnMiniboss(Game& game);
void spawnBeltbreaker(Game& game, int32_t wave);
void spawnWreckworm(Game& game, int32_t wave);
void spawnSwarmBoss(Game& game);
void spawnEnemyAt(Game& game, int kindIndex, Vector2 pos);
void forceBossAttack(Game& game, Boss& boss, BossAttack attack);
void spawnBlackHole(Game& game);
void spawnWormholePair(Game& game);
void spawnEliteHazard(Game& game, EliteHazardRole role);
void spawnPickup(Game& game, Vector2 position, int value, PickupType type,
                 ElementType element = ElementType::Static,
                 ElementMechanism mechanism = ElementMechanism::Infusion);

auto isWeaponTypeUnlocked(const Game& game, WeaponType weapon) -> bool;
void recordWeaponKill(Game& game, WeaponType weapon);
void recordSkillMaxed(Game& game, SkillType id);
void recordDashKill(Game& game, int32_t enemyKindIndex);
void recordDashOrNerveKill(Game& game);
void recordWaveReached(Game& game, int32_t wave);
