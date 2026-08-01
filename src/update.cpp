#include "update.hpp"

#include "democonfig.hpp"
#include "draw.hpp"
#include "entities/item.hpp"
#include "entities/ship.hpp"
#include "highscore.hpp"
#include "menu.hpp"
#include "palette.hpp"
#include "raylib.h"
#include "raymath.h"
#include "sandbox.hpp"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

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
constexpr float continuousDpsMultiplier = 1.2F;
constexpr float orbitBladeHitRadius = 10.0F;
// Beam hits every target along its full length at once, so its per-target damage needs to be much
// lower than a single-target weapon's or it deletes whole crowds simultaneously.
constexpr float beamDamageMult = 0.35F;

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
// Duration for temporary player-side pickup effects (elemental Infusion/Field, Regen), by
// Difficulty (Easy/Normal/Hard) — longer on harder difficulties to help offset tougher enemies.
constexpr std::array<float, 3> difficultyPickupDuration{20.0F, 40.0F, 60.0F};
constexpr float elementalFieldRadius = 90.0F;
constexpr float regenRate = 0.4F;
constexpr float overchargeDuration = 8.0F;
constexpr float overchargeDamageMult = 1.5F;
constexpr float dashTrailRadius = 24.0F;
constexpr int32_t dashTrailDamage = 4;
constexpr float dashTrailParticleLife = 0.9F;
constexpr float magnetPulseSnapDistance = 40.0F;
constexpr float dangerDowngradeDuration = 10.0F;
constexpr int32_t dangerDowngradeLevels = 2;
// Out of 1000 rolls, independent of (and rarer than) the "good" rare bonus roll above.
constexpr int32_t dangerPickupChance = 2;

// M26: Dash Trail/Second Wind are the catalog's "permanent" (non-timed) effects; they get offered
// more often at higher wave numbers instead of adding new pickup types.
auto isPermanentPickup(PickupType type) -> bool
{
    return type == PickupType::DashTrail || type == PickupType::SecondWind;
}

// Capped so early waves aren't starved of permanent pickups and late waves don't guarantee them.
auto permanentPickupBiasWeight(int32_t wave) -> float
{
    constexpr float maxBias = 4.0F;
    constexpr float waveDivisor = 25.0F;
    return std::min(maxBias, 1.0F + static_cast<float>(wave) / waveDivisor);
}

// M26: temporary-buff durations (Regen, Overcharge, elemental Infusion/Field) scale up with wave
// number, capped at +50% so it can't snowball into an imbalance in a long/infinite-mode run.
auto pickupDurationScale(int32_t wave) -> float
{
    constexpr float maxBonus = 0.5F;
    constexpr float waveDivisor = 200.0F;
    return 1.0F + std::min(maxBonus, static_cast<float>(wave) / waveDivisor);
}

auto pickWeightedPickupIndex(int32_t wave) -> size_t
{
    std::array<float, pickupCatalog.size()> weights{};
    float total = 0.0F;
    for (size_t i = 0; i < pickupCatalog.size(); i++)
    {
        weights.at(i) =
            isPermanentPickup(pickupCatalog.at(i).type) ? permanentPickupBiasWeight(wave) : 1.0F;
        total += weights.at(i);
    }

    constexpr int32_t rollResolution = 100000;
    float roll = static_cast<float>(GetRandomValue(0, rollResolution)) /
                 static_cast<float>(rollResolution) * total;
    for (size_t i = 0; i < weights.size(); i++)
    {
        if (roll < weights.at(i))
        {
            return i;
        }
        roll -= weights.at(i);
    }
    return weights.size() - 1;
}
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
auto crossfireProjectileDamage(const Game& game) -> int32_t
{
    return game.resources.settings.difficulty == Difficulty::Hard ? 5 : 3;
}
constexpr int32_t shieldDropChance = 15;
constexpr int32_t lifeOrbDropChance = 40;
constexpr int32_t settingsRowCount = 8;
constexpr float nerveKillGain = 6;
constexpr float chargeFeedDelay = 0.6F;
constexpr float chargeFeedDrainRate = 12.0F;
constexpr float chargeFeedRegenMult = 3.0F;
constexpr float nerveBurstLength = 900.0F;
constexpr int32_t nerveBurstDamage = 260;
constexpr float postCapDamageBonusPerLevel = 0.05F;
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

constexpr int32_t baseProjectileHealth = 3;

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

constexpr std::array<int, static_cast<size_t>(Difficulty::Count)> summonAddsCountByDifficulty{2, 3,
                                                                                              4};
constexpr float barrageFireInterval = 0.3F;
constexpr float barrageDuration = 2.5F;

constexpr float barrageProjSpeed = 8;
constexpr float homingBarrageFireInterval = 0.5F;

constexpr int spreadBulletsPerRound = 10;
constexpr std::array<int, static_cast<size_t>(Difficulty::Count)> spreadRoundsByDifficulty{1, 3, 6};
constexpr float spreadRoundInterval = 0.25F;
constexpr float gravityWellPullStrength = 1.6F;
constexpr float gravityWellDuration = 2.0F;

// Indices 6-13 are the M15 additions: Ricochet/FollowerDrone/LaserDrone are amplifiers with no
// fire-cooldown of their own (never read from this table), the rest are new primary weapons.
constexpr std::array<float, static_cast<size_t>(WeaponType::Count)> weaponBaseCooldown{
    0.38F, 0.85F, 1.0F, 0.7F, 0.6F, 1.5F, 0.0F, 0.0F, 0.0F, 0.9F, 1.8F, 2.2F, 6.0F, 0.0F};

constexpr float droneMeleeRange = 60.0F;
constexpr float droneAttackIntervalBase = 0.6F;
constexpr float laserDroneRangeBase = 220.0F;
constexpr float droneOrbitRadius = 70.0F;
constexpr float turretLife = 8.0F;
constexpr float turretFireInterval = 0.8F;
constexpr float flakSplashRadius = 40.0F;
constexpr float flamethrowerRange = 160.0F;
constexpr float flamethrowerHalfAngle = 20.0F * DEG2RAD;
constexpr float flamethrowerOffAngle = 45.0F * DEG2RAD;
constexpr float chainLightningRange = 260.0F;
constexpr float chainLightningBoltLife = 0.2F;

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
void cycleDifficulty(Game& game, int32_t dir);
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
auto weaponCooldown(const Game& game, WeaponType kind, int32_t level, bool evolved) -> float;
auto weaponDamage(const Game& game, int32_t level) -> int32_t;
auto ricochetLevel(const Game& game) -> int32_t;
auto shipCurrentDamage(const Game& game) -> float;
auto droneDamageFraction(int32_t level) -> float;
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
void collectElementalPickup(Game& game, ElementType element, ElementMechanism mechanism);
void updateElementalFields(Game& game, float deltaTime);
void updatePlayerBuffs(Game& game, float deltaTime);
void aoePulse(Game& game, Vector2 center, float radius, int32_t dmg, DamageSource source,
              float knockback = 0);
void updateWaveSpawner(Game& game, float deltaTime);
auto asteroidIntervalMultiplier(const Game& game) -> float;
auto asteroidCap(const Game& game) -> int;
auto eliteHazardCap(const Game& game) -> int;
void updateEliteHazards(Game& game, float deltaTime);
void damageEliteHazard(Game& game, size_t index, int32_t amount);
auto enemyDamage(const Game& game, int32_t base) -> int32_t;
void spawnEnemy(Game& game);
void spawnBossWave(Game& game, float healthMult, float sizeMult, bool isMega);
void spawnFinalBossWave(Game& game);
auto bossMoveCountForDifficulty(Difficulty difficulty) -> int;
auto sampleBossMoveset(int count) -> std::vector<BossAttack>;
auto spawnRingPosition(const Game& game) -> Vector2;
void updateBossMovement(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter);
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
void damageBoss(Game& game, Boss& boss, int32_t amount);
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

// M22: screen shake proportional to damage dealt, layered on top of (not replacing) the existing
// hand-tuned per-event shakes elsewhere — triggerShake only ever increases toward the larger of
// the two, so a big scripted attack shake is never undercut by a small per-hit one.
auto damageShakeIntensity(int32_t amount) -> float
{
    constexpr float perDamageUnit = 0.4F;
    constexpr float maxIntensity = 9.0F;
    return std::min(maxIntensity, static_cast<float>(amount) * perDamageUnit);
}
constexpr float damageShakeDuration = 0.12F;

void triggerShake(Game& game, float intensity, float duration)
{
    if (intensity > game.run.shakeIntensity)
    {
        game.run.shakeIntensity = intensity;
    }
    if (duration > game.run.shakeTimer)
    {
        game.run.shakeTimer = duration;
        game.run.shakeDuration = duration;
    }
}

// M18: floating damage number, spawned alongside every hit that reduces an enemy/boss/hazard's
// health. Small random horizontal jitter keeps overlapping hits (e.g. a pierced line of enemies)
// from stacking illegibly on top of each other.
void spawnDamageNumber(Game& game, Vector2 position, int32_t amount)
{
    constexpr float damageNumberLife = 0.6F;
    constexpr float jitterRange = 10.0F;
    const Vector2 jittered{
        .x = position.x + (static_cast<float>(GetRandomValue(-100, 100)) / 100.0F) * jitterRange,
        .y = position.y};
    game.run.damageNumbers.push_back(DamageNumber{.position = jittered,
                                                  .amount = amount,
                                                  .timer = damageNumberLife,
                                                  .maxTimer = damageNumberLife});
}

void updateDamageNumbers(Game& game, float deltaTime)
{
    constexpr float riseSpeed = 45.0F;
    for (auto& number : game.run.damageNumbers)
    {
        number.position.y -= riseSpeed * deltaTime;
        number.timer -= deltaTime;
    }
    std::erase_if(game.run.damageNumbers, [](const DamageNumber& n) { return n.timer <= 0; });
}

// M23: restores the Danger-pickup weapon downgrade once its timer runs out. If the weapon is
// somehow gone by then (shouldn't normally happen — weapons are never removed mid-run) this is a
// no-op rather than a crash.
void updateWeaponDowngrade(Game& game, float deltaTime)
{
    if (!game.run.weaponDowngrade.has_value())
    {
        return;
    }

    auto& downgrade = *game.run.weaponDowngrade;
    downgrade.timer -= deltaTime;
    if (downgrade.timer > 0)
    {
        return;
    }

    for (auto& weapon : game.run.weapons)
    {
        if (weapon.type == downgrade.type)
        {
            weapon.level += downgrade.amount;
            game.run.achievementToast = std::string(weaponDisplayName(weapon.type)) + " restored!";
            game.run.achievementToastTimer = 3.0F;
            break;
        }
    }
    game.run.weaponDowngrade = std::nullopt;
}

void triggerHitPause(Game& game, float duration)
{
    if (duration > game.run.hitPauseTimer)
    {
        game.run.hitPauseTimer = duration;
    }
}

void duckBGM(Game& game) { game.resources.bgm.duckTimer = bgmDuckDuration; }

auto updateTitle(Game& game) -> bool
{
    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, 4, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
        MenuLayout::buttonGap, MenuLayout::titleMenuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        switch (index)
        {
        case 0:
            game.menuIndex = game.resources.settings.shipIndex;
            game.settingsReturnState = GameState::TITLE;
            game.state = GameState::SHIP_SELECT;
            break;
        case 1:
            game.settingsReturnState = GameState::TITLE;
            game.state = GameState::ACHIEVEMENTS;
            break;
        case 2:
            game.settingsReturnState = GameState::TITLE;
            game.menuIndex = 0;
            game.state = GameState::SETTINGS;
            break;
        case 3:
            return true;
        default:
            break;
        }
    }

    return false;
}

auto updateAchievements(Game& game) -> bool
{
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        game.state = game.settingsReturnState;
    }
    return false;
}

auto updateShipSelect(Game& game) -> bool
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        game.menuIndex = 0;
        game.state = game.settingsReturnState;
        return false;
    }

    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, static_cast<int32_t>(ShipClass::Count), MenuLayout::buttonWidth,
        MenuLayout::buttonHeight, MenuLayout::buttonGap, MenuLayout::titleMenuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        game.resources.settings.shipIndex = index;
        saveSettings(game.resources.settings);
        resetRun(game);
    }

    return false;
}

auto updatePaused(Game& game) -> bool
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        game.state = GameState::GAMEPLAY;
        return false;
    }

    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, 5, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
        MenuLayout::buttonGap, MenuLayout::pausedMenuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        switch (index)
        {
        case 0:
            game.state = GameState::GAMEPLAY;
            break;
        case 1:
            game.settingsReturnState = GameState::PAUSED;
            game.state = GameState::ACHIEVEMENTS;
            break;
        case 2:
            game.settingsReturnState = GameState::PAUSED;
            game.menuIndex = 0;
            game.state = GameState::SETTINGS;
            break;
        case 3:
            game.menuIndex = game.resources.settings.shipIndex;
            game.settingsReturnState = GameState::PAUSED;
            game.state = GameState::SHIP_SELECT;
            break;
        case 4:
            return true;
        default:
            break;
        }
    }

    return false;
}

auto updateGameOver(Game& game) -> bool
{

    if (!game.run.scoreRecorded && !game.sandbox)
    {
        game.resources.highScores = highscore::record(*game.resources.highScoreRepo,
                                                      game.resources.highScores, game.run.score);
        game.run.scoreRecorded = true;
    }

    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, 2, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
        MenuLayout::buttonGap, MenuLayout::gameOverMenuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        switch (index)
        {
        case 0:
            game.menuIndex = game.resources.settings.shipIndex;
            game.settingsReturnState = GameState::GAME_OVER;
            game.state = GameState::SHIP_SELECT;
            break;
        case 1:
            return true;
        default:
            break;
        }
    }

    return false;
}

auto updateLevelUp(Game& game) -> bool
{
    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, static_cast<int32_t>(game.run.pendingChoices.size()),
        MenuLayout::levelUpWidth, MenuLayout::levelUpHeight, MenuLayout::levelUpGap,
        MenuLayout::levelUpMenuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        const auto& choice = game.run.pendingChoices.at(static_cast<size_t>(index));
        switch (choice.type)
        {
        case ChoiceType::Evolve:
            applyEvolution(game, choice.weapon.value());
            break;
        case ChoiceType::Pickup:
            applyPickupEffect(game, choice.pickupType, choice.element, choice.mechanism);
            break;
        case ChoiceType::Skill:
        default:
            applySkill(game, choice.skill);
            break;
        }
        game.state = GameState::GAMEPLAY;
    }

    return false;
}

void playMenuSounds(Game& game, int32_t newIndex, bool confirmed)
{
    if (confirmed)
    {
        playSFX(game, game.resources.sounds.menuConfirm);
    }
    else if (newIndex != game.menuIndex)
    {
        playSFX(game, game.resources.sounds.menuMove);
    }
}

void playSFX(Game& game, Sound sound)
{
    if (game.resources.settings.soundOn)
    {
        PlaySound(sound);
    }
}

auto updateSettings(Game& game) -> bool
{
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
    {
        game.menuIndex--;
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
    {
        game.menuIndex++;
    }
    if (game.menuIndex < 0)
    {
        game.menuIndex = settingsRowCount - 1;
    }
    if (game.menuIndex >= settingsRowCount)
    {
        game.menuIndex = 0;
    }

    const bool left = IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
    const bool right = IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
    bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    const Vector2 mouseDelta = GetMouseDelta();
    const bool mouseMoved =
        std::abs(mouseDelta.x) > FLT_EPSILON || std::abs(mouseDelta.y) > FLT_EPSILON;
    if (mouseMoved)
    {
        if (const auto row = hoveredColumnRow(settingsRowCount, MenuLayout::settingsWidth,
                                              MenuLayout::settingsHeight, MenuLayout::settingsGap,
                                              MenuLayout::settingsMenuY, game);
            row.has_value())
        {
            game.menuIndex = *row;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (const auto row = hoveredColumnRow(settingsRowCount, MenuLayout::settingsWidth,
                                              MenuLayout::settingsHeight, MenuLayout::settingsGap,
                                              MenuLayout::settingsMenuY, game);
            row.has_value())
        {
            game.menuIndex = *row;
            confirm = true;
        }
    }

    if (left || right || confirm)
    {
        playSFX(game, game.resources.sounds.menuMove);
    }

    switch (game.menuIndex)
    {
    case 0:
        if (left)
        {
            cycleResolution(game, -1);
        }
        if (right || confirm)
        {
            cycleResolution(game, 1);
        }
        break;
    case 1:

        if (game.settingsReturnState != GameState::PAUSED)
        {
            if (left)
            {
                cycleDifficulty(game, -1);
            }
            if (right || confirm)
            {
                cycleDifficulty(game, 1);
            }
        }
        break;
    case 2:
        if (left || right || confirm)
        {
            game.resources.settings.bgmOn = !game.resources.settings.bgmOn;
            applyBGMState(game);
        }
        break;
    case 3:
        if (left || right || confirm)
        {
            game.resources.settings.soundOn = !game.resources.settings.soundOn;
        }
        break;
    case 4:
        if (left)
        {
            cycleFPS(game, -1);
        }
        if (right || confirm)
        {
            cycleFPS(game, 1);
        }
        break;
    case 5:
        if (left)
        {
            cycleHudScale(game, -1);
        }
        if (right || confirm)
        {
            cycleHudScale(game, 1);
        }
        break;
    case 6:
        if (left || right || confirm)
        {
            toggleFullscreen(game);
        }
        break;
    case 7:
        if (confirm)
        {
            game.state = game.settingsReturnState;
            game.menuIndex = 0;
        }
        break;
    default:
        break;
    }

    if ((left || right || confirm) && game.menuIndex <= 5)
    {
        saveSettings(game.resources.settings);
    }

    return false;
}

void cycleResolution(Game& game, int32_t dir)
{
    const auto n = static_cast<int32_t>(resolutionOptions.size());
    game.resources.settings.resolutionIndex =
        (game.resources.settings.resolutionIndex + dir + n) % n;
    const auto& opt =
        resolutionOptions.at(static_cast<size_t>(game.resources.settings.resolutionIndex));
    SetWindowSize(opt.width, opt.height);
    syncScreenSize(game);
}

void cycleFPS(Game& game, int32_t dir)
{
    const auto n = static_cast<int32_t>(fpsOptions.size());
    game.resources.settings.fpsIndex = (game.resources.settings.fpsIndex + dir + n) % n;
    SetTargetFPS(fpsOptions.at(static_cast<size_t>(game.resources.settings.fpsIndex)));
}

void cycleHudScale(Game& game, int32_t dir)
{
    const auto n = static_cast<int32_t>(hudScaleOptions.size());
    game.resources.settings.hudScaleIndex = (game.resources.settings.hudScaleIndex + dir + n) % n;
}

void cycleDifficulty(Game& game, int32_t dir)
{
    const auto n = static_cast<int32_t>(Difficulty::Count);
    game.resources.settings.difficulty = static_cast<Difficulty>(
        (static_cast<int32_t>(game.resources.settings.difficulty) + dir + n) % n);
}

void applyBGMState(Game& game)
{
    if (game.resources.settings.bgmOn)
    {
        ResumeMusicStream(game.resources.bgm.base);
        ResumeMusicStream(game.resources.bgm.intensity);
        ResumeMusicStream(game.resources.bgm.miniboss);
        ResumeMusicStream(game.resources.bgm.megaboss);
        ResumeMusicStream(game.resources.bgm.swarmBoss);
    }
    else
    {
        PauseMusicStream(game.resources.bgm.base);
        PauseMusicStream(game.resources.bgm.intensity);
        PauseMusicStream(game.resources.bgm.miniboss);
        PauseMusicStream(game.resources.bgm.megaboss);
        PauseMusicStream(game.resources.bgm.swarmBoss);
    }
}

void updateBgmLayers(Game& game, float deltaTime)
{
    constexpr float intensityEnemyThreshold = 25.0F;
    constexpr float volumeSmoothing = 1.5F;
    constexpr float calmCeiling = 0.35F;
    constexpr float swarmOverrideThreshold = 0.7F;

    constexpr float intensityFloor = 0.3F;

    float intensityTarget =
        std::clamp(static_cast<float>(game.run.enemies.size()) / intensityEnemyThreshold,
                   intensityFloor, 1.0F);

    if (game.resources.bgm.calmTimer > 0)
    {
        if (intensityTarget >= swarmOverrideThreshold)
        {
            game.resources.bgm.calmTimer = 0;
        }
        else
        {
            intensityTarget = std::min(intensityTarget, calmCeiling);
            game.resources.bgm.calmTimer -= deltaTime;
        }
    }

    constexpr float minibossFullVolume = 0.6F;
    constexpr float megabossFullVolume = 0.8F;
    constexpr float swarmBossFullVolume = 1.0F;

    bool minibossActive = false;
    bool megabossActive = false;
    bool swarmBossActive = false;
    for (const auto& boss : game.run.bosses)
    {
        if (boss.isSwarm)
        {
            swarmBossActive = true;
        }
        else if (boss.isMega)
        {
            megabossActive = true;
        }
        else
        {
            minibossActive = true;
        }
    }
    const float swarmBossTarget = swarmBossActive ? swarmBossFullVolume : 0.0F;
    const float megabossTarget = (megabossActive && !swarmBossActive) ? megabossFullVolume : 0.0F;
    const float minibossTarget =
        (minibossActive && !swarmBossActive && !megabossActive) ? minibossFullVolume : 0.0F;

    const float rate = 1 - std::exp(-volumeSmoothing * deltaTime);
    game.resources.bgm.intensityVolume +=
        (intensityTarget - game.resources.bgm.intensityVolume) * rate;
    game.resources.bgm.minibossVolume +=
        (minibossTarget - game.resources.bgm.minibossVolume) * rate;
    game.resources.bgm.megabossVolume +=
        (megabossTarget - game.resources.bgm.megabossVolume) * rate;
    game.resources.bgm.swarmBossVolume +=
        (swarmBossTarget - game.resources.bgm.swarmBossVolume) * rate;

    float duckFactor = 1.0F;
    if (game.resources.bgm.duckTimer > 0)
    {
        game.resources.bgm.duckTimer -= deltaTime;
        const float t = std::clamp(game.resources.bgm.duckTimer / bgmDuckDuration, 0.0F, 1.0F);
        duckFactor = 1.0F - bgmDuckStrength * t;
    }

    SetMusicVolume(game.resources.bgm.base, SoundConstants::baseVolume * duckFactor);
    SetMusicVolume(game.resources.bgm.intensity, game.resources.bgm.intensityVolume * duckFactor);
    SetMusicVolume(game.resources.bgm.miniboss, game.resources.bgm.minibossVolume * duckFactor);
    SetMusicVolume(game.resources.bgm.megaboss, game.resources.bgm.megabossVolume * duckFactor);
    SetMusicVolume(game.resources.bgm.swarmBoss, game.resources.bgm.swarmBossVolume * duckFactor);
}

auto nerveFrac(const Game& game) -> float
{
    return game.run.player.nerve / UpdateConstants::nerveMax;
}

void gainNerve(Game& game)
{
    game.run.player.nerve += nerveKillGain;
    if (game.run.player.nerve > UpdateConstants::nerveMax)
    {
        game.run.player.nerve = UpdateConstants::nerveMax;
    }
}

auto isNerveChargeFeeding(const Game& game) -> bool
{
    return game.run.player.charges < UpdateConstants::maxCharges && game.run.player.nerve > 0 &&
           game.run.player.nerveChargeFeedTimer >= chargeFeedDelay;
}

void updateNerve(Game& game, float deltaTime)
{
    auto& player = game.run.player;

    if (game.run.nerveBurstFlashTimer > 0)
    {
        game.run.nerveBurstFlashTimer -= deltaTime;
    }

    if (player.charges >= UpdateConstants::maxCharges)
    {
        player.nerveChargeFeedTimer = 0;
    }
    else
    {
        player.nerveChargeFeedTimer += deltaTime;
        if (isNerveChargeFeeding(game))
        {
            const float drain = std::min(player.nerve, chargeFeedDrainRate * deltaTime);
            player.nerve -= drain;
            player.chargeRegenTimer -= deltaTime * (chargeFeedRegenMult - 1.0F);
        }
    }

    if (!player.nerveCharging)
    {
        return;
    }

    if (!IsKeyDown(KEY_SPACE))
    {
        player.nerveCharging = false;
        return;
    }

    player.nerveChargeTimer -= deltaTime;
    if (player.nerveChargeTimer <= 0)
    {
        player.nerveCharging = false;
        player.nerve = 0;
        fireNerveBurst(game);
    }
}

void updateNerveBurstInput(Game& game)
{
    auto& player = game.run.player;
    if (!player.nerveCharging && IsKeyPressed(KEY_SPACE) &&
        player.nerve >= UpdateConstants::nerveMax)
    {
        player.nerveCharging = true;
        player.nerveChargeTimer = UpdateConstants::nerveBurstWindup;
        playSFX(game, game.resources.sounds.nerveCharge);
    }
}

void damagePlayer(Game& game, int32_t amount)
{
    if (game.sandbox && !game.sandboxDeathEnabled)
    {
        return;
    }

    if (game.run.player.nerveCharging)
    {
        game.run.player.nerveCharging = false;
        game.run.player.nerve = 0;
        playSFX(game, game.resources.sounds.nerveFizzle);
    }

    if (game.run.player.shieldStacks > 0)
    {
        game.run.player.shieldStacks--;
        game.run.player.immunityTimer = 1.0F;
        playSFX(game, game.resources.sounds.hit);
        triggerShake(game, 3, 0.15F);
        return;
    }

    const float reduced = std::max(1.0F, static_cast<float>(amount) - currentShip(game).armor);
    game.run.player.health -= reduced;
    game.run.player.immunityTimer = 1.0F;

    if (game.run.player.health <= 0 && game.run.player.secondWindReady)
    {
        game.run.player.secondWindReady = false;
        game.run.player.health = 1.0F;
        playSFX(game, game.resources.sounds.victory);
        triggerShake(game, 8, 0.3F);
        return;
    }

    if (game.run.player.health <= 0)
    {
        game.state = GameState::GAME_OVER;
        game.menuIndex = 0;
        playSFX(game, game.resources.sounds.defeat);
        triggerShake(game, 12, 0.5F);
        triggerHitPause(game, 0.15F);
        spawnDeathExplosion(game);
    }
    else
    {
        playSFX(game, game.resources.sounds.hit);
        triggerShake(game, 4, 0.2F);
    }
}

// M22: shared kill-explosion burst for enemies/elite hazards/bosses (reuses the same
// `deathParticles` vector/update/draw path as the player's own death explosion). Bosses pass a
// bigger particleCount/speedScale for a more dramatic pop.
void spawnKillExplosion(Game& game, Vector2 position, Color color, int32_t particleCount,
                        float speedScale)
{
    for (int i = 0; i < particleCount; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const float speed = static_cast<float>(GetRandomValue(15, 60)) / 10.0F * speedScale;
        const Vector2 velocity{.x = std::cos(angle) * speed, .y = std::sin(angle) * speed};
        const float life = static_cast<float>(GetRandomValue(30, 60)) / 100.0F;

        game.run.deathParticles.push_back(
            Particle{.position = position,
                     .velocity = velocity,
                     .radius = static_cast<float>(GetRandomValue(2, 4)) * speedScale,
                     .life = life,
                     .maxLife = life,
                     .color = color});
    }
}

void spawnDeathExplosion(Game& game)
{
    const std::array<Color, 3> debrisColors{Palette::Accent, Palette::Crit, Palette::Haze};

    for (int i = 0; i < 28; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const float speed = static_cast<float>(GetRandomValue(20, 80)) / 10.0F;
        const Vector2 velocity{.x = std::cos(angle) * speed, .y = std::sin(angle) * speed};
        const float life = static_cast<float>(GetRandomValue(60, 100)) / 100.0F;

        game.run.deathParticles.push_back(
            Particle{.position = game.run.player.position,
                     .velocity = velocity,
                     .radius = static_cast<float>(GetRandomValue(2, 5)),
                     .life = life,
                     .maxLife = life,
                     .color = debrisColors.at(static_cast<size_t>(
                         GetRandomValue(0, static_cast<int32_t>(debrisColors.size() - 1))))});
    }
}

void updateDeathParticles(Game& game, float deltaTime)
{
    for (auto& p : game.run.deathParticles)
    {
        p.position = Vector2Add(p.position, p.velocity);
        p.life -= deltaTime;
    }

    std::erase_if(game.run.deathParticles, [](const Particle& p) { return p.life <= 0; });
}

void updateGameplay(Game& game, float deltaTime)
{

    game.run.damageMeter = DamageMeter{};

    if (IsKeyPressed(KEY_ESCAPE))
    {
        game.state = GameState::PAUSED;
        game.menuIndex = 0;
        return;
    }

    game.run.runTime += deltaTime;

    if (game.sandbox)
    {
        updateSandboxInput(game);
    }

    updateNerve(game, deltaTime);
    updateAbilityCharges(game, deltaTime);
    updatePlayerMovement(game, deltaTime);
    updateShieldAndBarrier(game, deltaTime);
    updateWeapons(game, deltaTime);
    updateOrbitBladeContact(game, deltaTime);
    updateOrbitBladeLaunch(game, deltaTime);
    updateOrbitBladeProjectiles(game, deltaTime);
    updateNerveBallProjectiles(game, deltaTime);
    updateNerveSpiralProjectiles(game, deltaTime);
    updateBeamContact(game, deltaTime);
    updateFollowerDrones(game, deltaTime);
    updateLaserDrones(game, deltaTime);
    updateTurrets(game, deltaTime);
    updateFlamethrower(game, deltaTime);
    updateChainLightningBolts(game, deltaTime);
    updateElementalFields(game, deltaTime);
    updatePlayerBuffs(game, deltaTime);
    updateWaveSpawner(game, deltaTime);
    updateBlackHole(game, deltaTime);
    updateWormhole(game, deltaTime);
    updateEliteHazards(game, deltaTime);

    if (game.run.player.health > 0 && game.run.blackhole.active &&
        Vector2Distance(game.run.player.position, game.run.blackhole.position) <=
            game.run.blackhole.radius)
    {
        game.run.player.blackHoleCoreTimer += deltaTime;
        if (game.run.player.blackHoleCoreTimer >= 1.0F)
        {
            game.run.player.blackHoleCoreTimer = 0;
            damagePlayer(game, instakillDamage);
        }
    }
    else
    {
        game.run.player.blackHoleCoreTimer = 0;
    }

    for (auto& boss : game.run.bosses)
    {
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        updateBossMovement(game, deltaTime, boss, bossCenter);
        updateBoss(game, deltaTime, boss, bossCenter);
    }

    updateBullets(game, deltaTime);
    updateAsteroids(game, deltaTime);
    updateEnemies(game, deltaTime);
    updateProjectiles(game, deltaTime);
    updateMines(game, deltaTime);
    updatePickups(game, deltaTime);

    filterDeadEntities(game);

    bool anyBossKilledThisFrame = false;
    for (auto& boss : game.run.bosses)
    {
        if (boss.health > 0)
        {
            continue;
        }

        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};

        boss.health = 0;
        boss.color = Palette::StructDark;
        game.run.score += 1000;
        anyBossKilledThisFrame = true;

        game.run.xp += static_cast<int>(static_cast<float>(game.run.xpToNext) *
                                        (boss.isMega ? megaBossXpMult : miniBossXpMult));
        if (boss.isSwarm)
        {
            spawnPickup(game, bossCenter, 0, PickupType::Shield);
            spawnPickup(game, bossCenter, 0, PickupType::LifeOrb);
        }
        spawnRareBonusPickup(game, bossCenter);
        spawnRareDangerPickup(game, bossCenter);

        game.run.bossDeathShockwaves.push_back(
            BossDeathShockwave{.timer = UpdateConstants::bossDeathShockwaveDuration,
                               .position = bossCenter,
                               .hit = false});
        spawnKillExplosion(game, bossCenter, boss.baseColor, 40, 2.2F);

        if (game.run.player.health > 0)
        {
            playSFX(game, game.resources.sounds.victory);
            triggerShake(game, 14, 0.6F);
            triggerHitPause(game, 0.2F);
            gainNerve(game);
        }

        if (boss.isFinalBoss && !game.resources.achievements.infiniteModeUnlocked)
        {
            game.resources.achievements.infiniteModeUnlocked = true;
            saveAchievements(game.resources.achievements);
            game.run.achievementToast = "Infinite mode unlocked!";
            game.run.achievementToastTimer = 4.0F;
        }
    }
    if (anyBossKilledThisFrame)
    {
        game.resources.bgm.calmTimer = bossKillCalmDuration;
    }
    std::erase_if(game.run.bosses, [](const Boss& boss) { return boss.health <= 0; });

    updateBossDeathShockwave(game, deltaTime);

    if (game.run.xp >= game.run.xpToNext)
    {
        startLevelUp(game);
    }

    if (game.run.damageMeter.total > 0)
    {
        game.run.damageMeterDisplay = game.run.damageMeter;
        game.run.damageMeterHoldTimer = damageMeterHoldDuration;
    }
    else if (game.run.damageMeterHoldTimer > 0)
    {
        game.run.damageMeterHoldTimer -= deltaTime;
        if (game.run.damageMeterHoldTimer <= 0)
        {
            game.run.damageMeterDisplay = DamageMeter{};
        }
    }
}

auto resolveExpandingWaveHit(Game& game, Vector2 from, float radius,
                             bool allowDash) -> WaveHitResult
{
    const float distToPlayer = Vector2Distance(from, game.run.player.position);
    const bool inDanger = distToPlayer <= bossConstants::maxSlamRadius + game.run.player.radius;
    const bool dashProtected = allowDash && game.run.player.dashing;

    if (inDanger &&
        (game.run.player.shieldActive || dashProtected || game.run.player.shieldStacks > 0))
    {
        if (!game.run.player.shieldActive && !dashProtected && game.run.player.shieldStacks > 0)
        {
            game.run.player.shieldStacks--;
        }
        return WaveHitResult{.resolved = true, .hit = false};
    }

    if (distToPlayer <= radius + game.run.player.radius)
    {
        return WaveHitResult{.resolved = true, .hit = true};
    }

    return WaveHitResult{.resolved = false, .hit = false};
}

void updateBossDeathShockwave(Game& game, float deltaTime)
{
    for (auto& wave : game.run.bossDeathShockwaves)
    {
        wave.timer -= deltaTime;

        const float progress =
            std::clamp(1 - wave.timer / UpdateConstants::bossDeathShockwaveDuration, 0.0F, 1.0F);
        const float radius = bossConstants::maxSlamRadius * progress;

        for (size_t i = 0; i < game.run.enemies.size(); i++)
        {
            if (game.run.enemies.at(i).active &&
                Vector2Distance(wave.position, game.run.enemies.at(i).position) <= radius)
            {
                killEnemyForBossAttack(game, i, true);
            }
        }

        for (auto& asteroid : game.run.asteroids)
        {
            if (asteroid.active && Vector2Distance(wave.position, asteroid.position) <= radius)
            {
                asteroid.active = false;
            }
        }

        const float prevProgress = std::clamp(
            1 - (wave.timer + deltaTime) / UpdateConstants::bossDeathShockwaveDuration, 0.0F, 1.0F);
        const float prevRadius = bossConstants::maxSlamRadius * prevProgress;
        for (auto& other : game.run.bosses)
        {
            const Vector2 otherCenter{.x = other.position.x + other.size.x / 2,
                                      .y = other.position.y + other.size.y / 2};
            const float distToBoss = Vector2Distance(wave.position, otherCenter);
            if (distToBoss <= radius && distToBoss > prevRadius)
            {
                other.health -= std::max(1, other.maxHealth / 50);
            }
        }

        if (!wave.hit)
        {
            if (const auto result = resolveExpandingWaveHit(game, wave.position, radius, false);
                result.resolved)
            {
                wave.hit = true;
                if (result.hit)
                {
                    damagePlayer(game, enemyDamage(game, 1));
                }
            }
        }
    }

    std::erase_if(game.run.bossDeathShockwaves,
                  [](const BossDeathShockwave& wave) { return wave.timer <= 0; });
}

void updatePlayerMovement(Game& game, float deltaTime)
{
    const bool inBlackHole =
        game.run.blackhole.active &&
        Vector2Distance(game.run.player.position, game.run.blackhole.position) <=
            game.run.blackhole.influenceRadius;

    if (game.run.player.slowTimer > 0)
    {
        game.run.player.slowTimer -= deltaTime;
    }

    if (game.run.player.dashing)
    {
        game.run.player.dashTimer -= deltaTime;
        if (game.run.player.dashTimer <= 0)
        {
            game.run.player.dashing = false;
        }
        game.run.player.position =
            Vector2Add(game.run.player.position,
                       Vector2Scale(game.run.player.dashVelocity, deltaTime * frameScale));
    }
    else
    {
        float effectiveSpeed = game.run.player.speed;
        if (inBlackHole)
        {
            effectiveSpeed -= blackHoleSlow;
        }
        if (game.run.player.slowTimer > 0)
        {
            effectiveSpeed *= 0.5F;
        }
        if (effectiveSpeed < 1)
        {
            effectiveSpeed = 1;
        }

        Vector2 moveDelta{};

        if (IsKeyDown(KEY_D))
        {
            moveDelta.x += effectiveSpeed;
        }
        if (IsKeyDown(KEY_A))
        {
            moveDelta.x -= effectiveSpeed;
        }
        if (IsKeyDown(KEY_W))
        {
            moveDelta.y -= effectiveSpeed;
        }
        if (IsKeyDown(KEY_S))
        {
            moveDelta.y += effectiveSpeed;
        }

        game.run.player.position =
            Vector2Add(game.run.player.position, Vector2Scale(moveDelta, deltaTime * frameScale));
    }

    if (inBlackHole)
    {
        const Vector2 toHole =
            Vector2Subtract(game.run.blackhole.position, game.run.player.position);
        if (Vector2Length(toHole) > 0)
        {
            const Vector2 pull = Vector2Scale(Vector2Normalize(toHole), blackHolePull);
            game.run.player.position =
                Vector2Add(game.run.player.position, Vector2Scale(pull, deltaTime * frameScale));
        }
    }

    if (game.run.player.dashing)
    {
        applyWormholeTransit(game, game.run.player.position, game.run.player.dashVelocity,
                             game.run.player.radius);
    }
    else
    {
        Vector2 noVelocity{};
        applyWormholeTransit(game, game.run.player.position, noVelocity, game.run.player.radius);
    }

    if (const float dist = Vector2Length(game.run.player.position); dist > GameConstants::arenaHalf)
    {
        game.run.player.position =
            Vector2Scale(Vector2Normalize(game.run.player.position), GameConstants::arenaHalf);
    }

    bool touchingAnyBoss = false;
    for (auto& boss : game.run.bosses)
    {
        const Rectangle bossRect{.x = boss.position.x,
                                 .y = boss.position.y,
                                 .width = boss.size.x,
                                 .height = boss.size.y};
        if (boss.health <= 0 ||
            !CheckCollisionCircleRec(game.run.player.position, game.run.player.radius, bossRect))
        {
            continue;
        }

        touchingAnyBoss = true;

        if (game.run.player.dashing && !boss.hitByDash)
        {
            boss.hitByDash = true;
            const DashQuirk quirk = currentShip(game).dashQuirk;

            if (quirk != DashQuirk::Push)
            {
                const float quirkMult = quirk == DashQuirk::Hybrid ? 0.5F : 1.0F;
                const auto ramDamage = static_cast<int32_t>(
                    static_cast<float>(game.run.player.shieldActive ? bossRamDamage * 2
                                                                    : bossRamDamage) *
                    quirkMult * currentShip(game).damageMult);
                damageBoss(game, boss, ramDamage);
                if (game.run.player.shieldActive)
                {
                    game.run.player.chargeRegenTimer =
                        std::max(0.0F, game.run.player.chargeRegenTimer - shieldKillChargeRefund);
                }
            }

            if (quirk != DashQuirk::Damage)
            {
                const Vector2 pushDir = Vector2Normalize(game.run.player.dashVelocity);
                boss.position = Vector2Add(boss.position, Vector2Scale(pushDir, dashPushDistance));
            }

            triggerShake(game, 14, 0.4F);
            triggerHitPause(game, 0.12F);
        }
    }

    if (touchingAnyBoss && !game.run.player.dashing)
    {
        game.run.player.bossBodyTimer += deltaTime;
        if (game.run.player.bossBodyTimer >= bossBodyLingerLimit)
        {
            game.run.player.bossBodyTimer = 0;
            damagePlayer(game, instakillDamage);
        }
    }
    else
    {
        game.run.player.bossBodyTimer = 0;
    }

    if (game.run.player.immunityTimer > 0)
    {
        game.run.player.immunityTimer -= deltaTime;
    }
}

void updateShieldAndBarrier(Game& game, float deltaTime)
{
    if (game.run.player.shieldCooldownTimer > 0)
    {
        game.run.player.shieldCooldownTimer -= deltaTime;
    }

    if (game.run.player.shieldActive)
    {
        game.run.player.shieldTimer -= deltaTime;

        if (game.run.player.shieldTimer <= 0)
        {
            game.run.player.shieldActive = false;
            game.run.player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
        }
    }
}

auto chargeRegenDuration(const Game& game) -> float
{
    const float duration =
        UpdateConstants::chargeRegenTime -
        static_cast<float>(game.run.skillLevels.at(static_cast<size_t>(SkillType::Barrier))) * 0.3F;
    return duration < 1 ? 1 : duration;
}

void updateAbilityCharges(Game& game, float deltaTime)
{
    if (game.run.player.charges < UpdateConstants::maxCharges)
    {
        game.run.player.chargeRegenTimer -= deltaTime;
        if (game.run.player.chargeRegenTimer <= 0)
        {
            game.run.player.charges++;
            game.run.player.chargeRegenTimer = chargeRegenDuration(game);
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !game.run.player.dashing)
    {
        if (game.run.player.charges > 0)
        {
            game.run.player.charges--;

            game.run.player.dashing = true;
            game.run.player.dashTimer = dashDuration;
            game.run.player.dashVelocity =
                Vector2Scale(aimAtMouse(game), dashSpeed * currentShip(game).dashDistanceMult);

            for (auto& enemy : game.run.enemies)
            {
                enemy.hitByDash = false;
            }
            for (auto& boss : game.run.bosses)
            {
                boss.hitByDash = false;
            }
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !game.run.player.shieldActive)
    {
        if (game.run.player.charges > 0)
        {
            game.run.player.charges--;

            game.run.player.shieldActive = true;
            game.run.player.shieldTimer =
                shieldBaseDuration + static_cast<float>(game.run.skillLevels.at(
                                         static_cast<size_t>(SkillType::Barrier))) *
                                         0.4F;
        }
    }

    updateNerveBurstInput(game);
}

auto aimAtMouse(const Game& game) -> Vector2
{
    const Vector2 mouse = GetMousePosition();
    const Rectangle box = letterBoxRect(game);
    const Vector2 center{.x = box.x + box.width / 2, .y = box.y + box.height / 2};
    const Vector2 dir = Vector2Subtract(mouse, center);
    if (Vector2Length(dir) == 0)
    {
        return Vector2{.x = 0, .y = -1};
    }
    return Vector2Normalize(dir);
}

auto mouseWorldPos(const Game& game) -> Vector2
{
    const Vector2 mouse = GetMousePosition();
    const Rectangle box = letterBoxRect(game);
    const Vector2 center{.x = box.x + box.width / 2, .y = box.y + box.height / 2};
    const float scale = game.resources.renderScale;
    if (scale <= 0)
    {
        return game.run.player.position;
    }
    return Vector2Add(game.run.player.position,
                      Vector2Scale(Vector2Subtract(mouse, center), 1.0F / scale));
}

auto nearestEnemy(const Game& game, Vector2 from) -> std::optional<Vector2>
{
    float best = -1;
    Vector2 target{};
    bool found = false;

    for (const auto& enemy : game.run.enemies)
    {
        if (!enemy.active)
        {
            continue;
        }
        const float d = Vector2Distance(from, enemy.position);
        if (best < 0 || d < best)
        {
            best = d;
            target = enemy.position;
            found = true;
        }
    }

    for (const auto& hazard : game.run.eliteHazards)
    {
        if (!hazard.active)
        {
            continue;
        }
        const float d = Vector2Distance(from, hazard.position);
        if (best < 0 || d < best)
        {
            best = d;
            target = hazard.position;
            found = true;
        }
    }

    if (!found)
    {
        return std::nullopt;
    }
    return target;
}

// Seeker Swarm (evolved Homing Missiles): picks the nearest target that isn't the one another
// missile in the same volley already locked onto, so a 2-missile volley spreads across 2 enemies.
auto nearestEnemyExcluding(const Game& game, Vector2 from,
                           Vector2 exclude) -> std::optional<Vector2>
{
    constexpr float excludeEpsilon = 0.5F;
    float best = -1;
    Vector2 target{};
    bool found = false;

    for (const auto& enemy : game.run.enemies)
    {
        if (!enemy.active || Vector2Distance(enemy.position, exclude) <= excludeEpsilon)
        {
            continue;
        }
        const float d = Vector2Distance(from, enemy.position);
        if (best < 0 || d < best)
        {
            best = d;
            target = enemy.position;
            found = true;
        }
    }

    for (const auto& hazard : game.run.eliteHazards)
    {
        if (!hazard.active || Vector2Distance(hazard.position, exclude) <= excludeEpsilon)
        {
            continue;
        }
        const float d = Vector2Distance(from, hazard.position);
        if (best < 0 || d < best)
        {
            best = d;
            target = hazard.position;
            found = true;
        }
    }

    if (!found)
    {
        return std::nullopt;
    }
    return target;
}

auto orbitRadius(int32_t level) -> float { return 55 + static_cast<float>(level) * 4; }

auto orbitBladeCount(int32_t level) -> int32_t { return 2 + level / 2; }

auto orbitBladePosition(const Game& game, Vector2 center, float radius, int32_t index,
                        int32_t count) -> Vector2
{
    const float angle =
        static_cast<float>(GetTime()) * 2 * currentShip(game).orbitSpinMult +
        static_cast<float>(index) * 2 * std::numbers::pi_v<float> / static_cast<float>(count);
    return Vector2Add(center,
                      Vector2{.x = std::cos(angle) * radius, .y = std::sin(angle) * radius});
}

auto beamAimDirection(const Game& game, bool evolved) -> Vector2
{
    const Vector2 dir = aimAtMouse(game);
    if (!evolved)
    {
        return dir;
    }
    // Lance Sweep: oscillate the aim direction across a narrow arc instead of a fixed line.
    constexpr float sweepArcDegrees = 40.0F;
    constexpr float sweepSpeed = 2.0F;
    const float sweepAngle =
        std::sin(static_cast<float>(GetTime()) * sweepSpeed) * (sweepArcDegrees / 2) * DEG2RAD;
    const float cosA = std::cos(sweepAngle);
    const float sinA = std::sin(sweepAngle);
    return Vector2{.x = dir.x * cosA - dir.y * sinA, .y = dir.x * sinA + dir.y * cosA};
}

auto beamLength(const Game& game, int32_t level, bool evolved) -> float
{
    float length = (150 + static_cast<float>(level) * 20) * currentShip(game).beamLengthMult;
    if (evolved)
    {
        length *= 1.3F;
    }
    return length;
}

auto shockwaveRadius(int32_t level, bool evolved) -> float
{
    float radius = 90 + static_cast<float>(level) * 8;
    if (evolved)
    {
        radius *= 1.4F;
    }
    return radius;
}

auto weaponCooldown(const Game& game, WeaponType kind, int32_t level, bool evolved) -> float
{
    float base = weaponBaseCooldown.at(static_cast<size_t>(kind));
    base -= static_cast<float>(level) * 0.02F;
    if (base < 0.12F)
    {
        base = 0.12F;
    }
    base *= 1 - 0.1F * static_cast<float>(
                           game.run.skillLevels.at(static_cast<size_t>(SkillType::Cooldown)));
    if (evolved)
    {
        base *= 0.8F;
    }

    for (const auto& hazard : game.run.eliteHazards)
    {
        if (hazard.active && hazard.role == EliteHazardRole::Suppressor)
        {
            base *= suppressorCooldownPenalty;
            break;
        }
    }

    if (base < 0.08F)
    {
        base = 0.08F;
    }
    return base;
}

auto weaponDamage(const Game& game, int32_t level) -> int32_t
{
    auto dmg = static_cast<float>(baseBulletDamage + level * 2);
    dmg *= 1 + 0.15F * static_cast<float>(
                           game.run.skillLevels.at(static_cast<size_t>(SkillType::Damage)));
    dmg *= 1 + postCapDamageBonusPerLevel * static_cast<float>(game.run.postCapDamageLevels);
    dmg *= currentShip(game).damageMult;
    if (game.run.player.overchargeTimer > 0)
    {
        dmg *= overchargeDamageMult;
    }
    return static_cast<int32_t>(dmg);
}

// M15: how many extra enemies a "stops on contact" projectile bounces to (0 if Ricochet isn't
// equipped). Only meaningful for weapons that don't already pierce — callers gate on that.
auto ricochetLevel(const Game& game) -> int32_t
{
    for (const auto& w : game.run.weapons)
    {
        if (w.type == WeaponType::Ricochet)
        {
            return w.level;
        }
    }
    return 0;
}

// M15 drones: "50/70/70/90% of the ship's current damage" is read as this level-independent base
// damage figure (already scales with the Damage skill, post-cap bonus, ship multiplier,
// overcharge).
auto shipCurrentDamage(const Game& game) -> float
{
    return static_cast<float>(weaponDamage(game, 0));
}

// M15 drones: 50% base, +20% at level>=2, +20% more at level>=4 (Follower/Laser Drone share this).
auto droneDamageFraction(int32_t level) -> float
{
    float fraction = 0.5F;
    if (level >= 2)
    {
        fraction += 0.2F;
    }
    if (level >= 4)
    {
        fraction += 0.2F;
    }
    return fraction;
}

auto mineCount(int32_t level, bool evolved) -> int
{
    int count = 2 + static_cast<int>(level) / 3;
    if (evolved)
    {
        count++;
    }
    return count;
}

auto mineRadius(bool evolved) -> float { return evolved ? 42.0F : 32.0F; }

auto mineDamage(const Game& game, int32_t level, bool evolved) -> int32_t
{
    int32_t dmg = weaponDamage(game, level);
    if (evolved)
    {
        dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
    }
    return dmg;
}

void spawnMines(Game& game, const Weapon& weapon)
{
    const int count = mineCount(weapon.level, weapon.evolved);
    const float radius = mineRadius(weapon.evolved);
    const int32_t dmg = mineDamage(game, weapon.level, weapon.evolved);

    for (int i = 0; i < count; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto dist = static_cast<float>(GetRandomValue(20, 50));
        const Vector2 pos =
            Vector2Add(game.run.player.position,
                       Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

        game.run.mines.push_back(Mine{.position = pos,
                                      .velocity = Vector2{},
                                      .fuse = mineLifetime,
                                      .radius = radius,
                                      .damage = dmg,
                                      .active = true,
                                      .evolved = weapon.evolved});
    }
}

auto nearestEnemyWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>
{
    if (const auto target = nearestEnemy(game, from);
        target.has_value() && Vector2Distance(from, *target) <= maxDist)
    {
        return target;
    }
    return std::nullopt;
}

auto nearestAsteroidWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>
{
    float best = -1;
    Vector2 target{};
    bool found = false;

    for (const auto& asteroid : game.run.asteroids)
    {
        if (!asteroid.active)
        {
            continue;
        }
        const float d = Vector2Distance(from, asteroid.position);
        if (d <= maxDist && (best < 0 || d < best))
        {
            best = d;
            target = asteroid.position;
            found = true;
        }
    }

    if (!found)
    {
        return std::nullopt;
    }
    return target;
}

// Cluster Charges: an evolved mine detonating force-detonates any other active evolved mine
// within chainRadius, cascading in the same frame for a single connected blast.
void detonateMine(Game& game, size_t index)
{
    Mine& mine = game.run.mines.at(index);
    if (!mine.active)
    {
        return;
    }
    mine.active = false;
    aoePulse(game, mine.position, mine.radius, mine.damage, DamageSource::Mine);

    if (!mine.evolved)
    {
        return;
    }
    constexpr float chainRadius = 80.0F;
    const Vector2 origin = mine.position;
    for (size_t j = 0; j < game.run.mines.size(); j++)
    {
        if (j == index)
        {
            continue;
        }
        const Mine& other = game.run.mines.at(j);
        if (other.active && other.evolved && Vector2Distance(origin, other.position) <= chainRadius)
        {
            detonateMine(game, j);
        }
    }
}

void updateMines(Game& game, float deltaTime)
{
    for (size_t i = 0; i < game.run.mines.size(); i++)
    {
        Mine& mine = game.run.mines.at(i);
        if (!mine.active)
        {
            continue;
        }

        bool detonate = false;

        if (mine.evolved)
        {
            auto target = nearestEnemyWithin(game, mine.position, mineSeekRadius);
            if (!target.has_value())
            {
                target = nearestAsteroidWithin(game, mine.position, mineSeekRadius);
            }

            if (target.has_value())
            {
                const Vector2 dir = Vector2Subtract(*target, mine.position);
                if (Vector2Length(dir) <= mine.radius)
                {
                    detonate = true;
                }
                else
                {
                    mine.velocity = Vector2Scale(Vector2Normalize(dir), mineHomeSpeed);
                }
            }
            else
            {
                mine.velocity = Vector2{};
            }

            mine.position =
                Vector2Add(mine.position, Vector2Scale(mine.velocity, deltaTime * frameScale));
        }
        else if (nearestEnemyWithin(game, mine.position, mine.radius).has_value())
        {

            detonate = true;
        }

        mine.fuse -= deltaTime;
        if (mine.fuse <= 0)
        {
            detonate = true;
        }

        if (detonate)
        {
            detonateMine(game, i);
        }
    }
}

void updateWeapons(Game& game, float deltaTime)
{
    for (auto& w : game.run.weapons)
    {
        if (w.flashTimer > 0)
        {
            w.flashTimer -= deltaTime;
        }

        if (w.type == WeaponType::Orbit || w.type == WeaponType::Beam)
        {
            // Continuous contact weapons: handled every frame by updateOrbitBladeContact()/
            // updateBeamContact(), not on this fire-and-cooldown cadence.
            continue;
        }

        w.timer -= deltaTime;
        if (w.timer > 0)
        {
            continue;
        }
        w.timer = weaponCooldown(game, w.type, w.level, w.evolved);

        switch (w.type)
        {
        case WeaponType::Forward:
        {
            const Vector2 dir = aimAtMouse(game);
            int32_t shots = 1 + w.level / 3;
            if (w.evolved)
            {
                shots++;
            }
            constexpr float spread = 10;
            int32_t dmg = weaponDamage(game, w.level);
            Color color = Palette::Accent;
            if (w.evolved)
            {
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
                color = Palette::Crit;
            }
            for (int32_t s = 0; s < shots; s++)
            {
                const float angleOffset =
                    (static_cast<float>(s) - static_cast<float>(shots - 1) / 2) * spread * DEG2RAD;
                const float cosA = std::cos(angleOffset);
                const float sinA = std::sin(angleOffset);
                const Vector2 rotated{.x = dir.x * cosA - dir.y * sinA,
                                      .y = dir.x * sinA + dir.y * cosA};
                game.run.bullets.push_back(Bullet{
                    .position = game.run.player.position,
                    .velocity =
                        Vector2Scale(rotated, projectileSpeed * currentShip(game).bulletSpeedMult),
                    .radius = projectileSize,
                    .color = color,
                    .active = true,
                    .damage = dmg,
                    .pierceRemaining = w.evolved ? 2 : 0,
                    .ricochetRemaining = w.evolved ? 0 : ricochetLevel(game),
                    .source = DamageSource::Forward});
            }
            playSFX(game, game.resources.sounds.shoot);
            break;
        }
        case WeaponType::Homing:
        {
            const int missiles = w.evolved ? 2 : 1;
            int32_t dmg = weaponDamage(game, w.level);
            if (w.evolved)
            {
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
            }
            bool fired = false;
            std::optional<Vector2> firstTarget;
            for (int m = 0; m < missiles; m++)
            {
                const auto target =
                    (w.evolved && m > 0 && firstTarget.has_value())
                        ? nearestEnemyExcluding(game, game.run.player.position, *firstTarget)
                        : nearestEnemy(game, game.run.player.position);
                bool huntingNewTarget = false;
                Vector2 dir{};
                if (target.has_value())
                {
                    dir = Vector2Normalize(Vector2Subtract(*target, game.run.player.position));
                    if (m == 0)
                    {
                        firstTarget = target;
                    }
                }
                else if (w.evolved && firstTarget.has_value())
                {
                    // Seeker Swarm, no free target at spawn: fly straight until one appears.
                    dir = Vector2Normalize(Vector2Subtract(*firstTarget, game.run.player.position));
                    huntingNewTarget = true;
                }
                else
                {
                    continue;
                }
                game.run.bossProjectiles.push_back(
                    BossProjectile{.position = game.run.player.position,
                                   .velocity = Vector2Scale(dir, homingProjSpeed * 1.5F),
                                   .radius = 6,
                                   .homing = true,
                                   .active = true,
                                   .fromPlayer = true,
                                   .damage = dmg,
                                   .huntingNewTarget = huntingNewTarget,
                                   .ricochetRemaining = ricochetLevel(game)});
                fired = true;
            }
            if (fired)
            {
                playSFX(game, game.resources.sounds.homingLaunch);
            }
            break;
        }
        case WeaponType::Shock:
        {
            const float radius = shockwaveRadius(w.level, w.evolved);
            int32_t dmg = weaponDamage(game, w.level);
            if (w.evolved)
            {
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
                if (game.run.player.health < game.run.player.maxHealth)
                {
                    game.run.player.health++;
                }
            }
            aoePulse(game, game.run.player.position, radius, dmg, DamageSource::Shock,
                     w.evolved ? 40.0F : 0.0F);
            w.flashTimer = UpdateConstants::shockFlashDuration;
            break;
        }
        case WeaponType::Mine:
            spawnMines(game, w);
            break;
        case WeaponType::FlakCannon:
        {
            const Vector2 dir = aimAtMouse(game);
            const int32_t pellets = 3 + (w.level >= 2 ? 2 : 0);
            const int32_t dmg = weaponDamage(game, w.level);
            const bool pierces = w.level >= 3;
            for (int32_t s = 0; s < pellets; s++)
            {
                const float angleOffset =
                    (static_cast<float>(s) - static_cast<float>(pellets - 1) / 2) * 8.0F * DEG2RAD;
                const float cosA = std::cos(angleOffset);
                const float sinA = std::sin(angleOffset);
                const Vector2 rotated{.x = dir.x * cosA - dir.y * sinA,
                                      .y = dir.x * sinA + dir.y * cosA};
                game.run.bullets.push_back(Bullet{
                    .position = game.run.player.position,
                    .velocity =
                        Vector2Scale(rotated, projectileSpeed * currentShip(game).bulletSpeedMult),
                    .radius = projectileSize,
                    .color = Palette::Accent,
                    .active = true,
                    .damage = dmg,
                    .pierceRemaining = pierces ? 1 : 0,
                    .ricochetRemaining = pierces ? 0 : ricochetLevel(game),
                    .splashRadius = flakSplashRadius,
                    .source = DamageSource::FlakCannon});
            }
            playSFX(game, game.resources.sounds.shoot);
            break;
        }
        case WeaponType::Railgun:
        {
            const Vector2 dir = aimAtMouse(game);
            int32_t dmg = weaponDamage(game, w.level) * 2;
            if (w.level >= 3)
            {
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.3F);
            }
            game.run.bullets.push_back(Bullet{
                .position = game.run.player.position,
                .velocity =
                    Vector2Scale(dir, projectileSpeed * 1.6F * currentShip(game).bulletSpeedMult),
                .radius = projectileSize * 1.5F,
                .color = Palette::Crit,
                .active = true,
                .damage = dmg,
                .pierceRemaining = 999,
                .source = DamageSource::Railgun});
            playSFX(game, game.resources.sounds.shoot);
            break;
        }
        case WeaponType::ChainLightning:
            fireChainLightning(game, w);
            break;
        case WeaponType::TurretDeploy:
        {
            const int32_t count = w.level >= 3 ? 2 : 1;
            for (int32_t s = 0; s < count; s++)
            {
                game.run.turrets.push_back(Turret{.position = game.run.player.position,
                                                  .life = turretLife + (w.level >= 2 ? 4.0F : 0.0F),
                                                  .fireTimer = turretFireInterval});
            }
            break;
        }
        case WeaponType::Ricochet:
        case WeaponType::FollowerDrone:
        case WeaponType::LaserDrone:
        case WeaponType::Flamethrower:
        case WeaponType::Orbit:
        case WeaponType::Beam:
        case WeaponType::Count:
            break;
        }
    }
}

void recordDamage(Game& game, DamageSource source, int32_t amount)
{
    game.run.damageMeter.bySource.at(static_cast<size_t>(source)) += amount;
    game.run.damageMeter.total += amount;
}

void aoePulse(Game& game, Vector2 center, float radius, int32_t dmg, DamageSource source,
              float knockback)
{
    bool hitAny = false;

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        auto& enemy = game.run.enemies.at(j);
        if (enemy.active && !enemy.phased &&
            Vector2Distance(center, enemy.position) <=
                radius + enemyKinds.at(static_cast<size_t>(enemy.kind)).radius)
        {
            damageEnemy(game, j, dmg);
            recordDamage(game, source, dmg);
            if (!enemy.active && (source == DamageSource::Mine || source == DamageSource::Shock))
            {
                recordWeaponKill(game, source == DamageSource::Mine ? WeaponType::Mine
                                                                    : WeaponType::Shock);
            }
            if (knockback > 0 && Vector2Distance(center, enemy.position) > 0.01F)
            {
                const Vector2 pushDir = Vector2Normalize(Vector2Subtract(enemy.position, center));
                enemy.position = Vector2Add(enemy.position, Vector2Scale(pushDir, knockback));
            }
            hitAny = true;
        }
    }

    for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
    {
        const auto& hazard = game.run.eliteHazards.at(j);
        if (hazard.active &&
            Vector2Distance(center, hazard.position) <= radius + EliteHazardConstants::radius)
        {
            damageEliteHazard(game, j, dmg);
            recordDamage(game, source, dmg);
            hitAny = true;
        }
    }

    for (auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active &&
            Vector2Distance(center, asteroid.position) <= radius + asteroid.radius)
        {
            asteroid.active = false;
            game.run.score += asteroidScore(asteroid.tier);
            breakAsteroid(game.run.asteroids, asteroid);
            hitAny = true;
        }
    }

    if (hitAny)
    {
        playSFX(game, game.resources.sounds.explosion);
    }
}

void updateOrbitBladeContact(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::Orbit; });

    if (it == game.run.weapons.end())
    {
        for (auto& enemy : game.run.enemies)
        {
            enemy.orbitContact = false;
            enemy.orbitDamageAccum = 0;
        }
        for (auto& hazard : game.run.eliteHazards)
        {
            hazard.orbitContact = false;
            hazard.orbitDamageAccum = 0;
        }
        for (auto& boss : game.run.bosses)
        {
            boss.orbitContact = false;
            boss.orbitDamageAccum = 0;
        }
        return;
    }

    const Weapon& w = *it;
    float radius = orbitRadius(w.level);
    auto firstHit = static_cast<float>(weaponDamage(game, w.level));
    if (w.evolved)
    {
        radius *= 1.4F;
        firstHit *= 1.5F;
    }
    const float dps = firstHit * continuousDpsMultiplier;
    const int32_t count = orbitBladeCount(w.level);

    std::vector<Vector2> bladePositions;
    bladePositions.reserve(static_cast<size_t>(count));
    for (int32_t s = 0; s < count; s++)
    {
        bladePositions.push_back(
            orbitBladePosition(game, game.run.player.position, radius, s, count));
    }

    const auto touchesAnyBlade = [&](Vector2 pos, float targetRadius)
    {
        return std::ranges::any_of(
            bladePositions, [&](const Vector2& bp)
            { return CheckCollisionCircles(bp, orbitBladeHitRadius, pos, targetRadius); });
    };

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        auto& enemy = game.run.enemies.at(j);
        if (!enemy.active || enemy.phased ||
            !touchesAnyBlade(enemy.position, enemyKinds.at(static_cast<size_t>(enemy.kind)).radius))
        {
            enemy.orbitContact = false;
            enemy.orbitDamageAccum = 0;
            continue;
        }

        if (!enemy.orbitContact)
        {
            enemy.orbitContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageEnemy(game, j, hit);
            recordDamage(game, DamageSource::Orbit, hit);
            if (!enemy.active)
            {
                recordWeaponKill(game, WeaponType::Orbit);
            }
        }
        else
        {
            enemy.orbitDamageAccum += dps * deltaTime;
            if (enemy.orbitDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(enemy.orbitDamageAccum);
                damageEnemy(game, j, tick);
                recordDamage(game, DamageSource::Orbit, tick);
                if (!enemy.active)
                {
                    recordWeaponKill(game, WeaponType::Orbit);
                }
                enemy.orbitDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
    {
        auto& hazard = game.run.eliteHazards.at(j);
        if (!hazard.active || !touchesAnyBlade(hazard.position, EliteHazardConstants::radius))
        {
            hazard.orbitContact = false;
            hazard.orbitDamageAccum = 0;
            continue;
        }

        if (!hazard.orbitContact)
        {
            hazard.orbitContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageEliteHazard(game, j, hit);
            recordDamage(game, DamageSource::Orbit, hit);
        }
        else
        {
            hazard.orbitDamageAccum += dps * deltaTime;
            if (hazard.orbitDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(hazard.orbitDamageAccum);
                damageEliteHazard(game, j, tick);
                recordDamage(game, DamageSource::Orbit, tick);
                hazard.orbitDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Rectangle bossRect{.x = boss.position.x,
                                 .y = boss.position.y,
                                 .width = boss.size.x,
                                 .height = boss.size.y};
        const bool touching = std::ranges::any_of(
            bladePositions, [&](const Vector2& bp)
            { return CheckCollisionCircleRec(bp, orbitBladeHitRadius, bossRect); });

        if (!touching)
        {
            boss.orbitContact = false;
            boss.orbitDamageAccum = 0;
            continue;
        }

        if (!boss.orbitContact)
        {
            boss.orbitContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageBoss(game, boss, hit);
            recordDamage(game, DamageSource::Orbit, hit);
        }
        else
        {
            boss.orbitDamageAccum += dps * deltaTime;
            if (boss.orbitDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(boss.orbitDamageAccum);
                damageBoss(game, boss, tick);
                recordDamage(game, DamageSource::Orbit, tick);
                boss.orbitDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active && touchesAnyBlade(asteroid.position, asteroid.radius))
        {
            asteroid.active = false;
            game.run.score += asteroidScore(asteroid.tier);
            breakAsteroid(game.run.asteroids, asteroid);
        }
    }

    // Aegis Ring: evolved Orbit Blades destroy incoming boss projectiles outright on contact.
    if (w.evolved)
    {
        for (auto& projectile : game.run.bossProjectiles)
        {
            if (projectile.active && !projectile.fromPlayer &&
                touchesAnyBlade(projectile.position, projectile.radius))
            {
                projectile.active = false;
            }
        }
    }
}

void updateOrbitBladeLaunch(Game& game, float deltaTime)
{
    for (auto& w : game.run.weapons)
    {
        if (w.type != WeaponType::Orbit)
        {
            continue;
        }

        if (w.flashTimer > 0)
        {
            w.flashTimer -= deltaTime;
        }

        w.timer -= deltaTime;
        if (w.timer > 0)
        {
            continue;
        }
        w.timer = orbitLaunchInterval * (1 - 0.1F * static_cast<float>(game.run.skillLevels.at(
                                                        static_cast<size_t>(SkillType::Cooldown))));

        const int32_t bladeCount = orbitBladeCount(w.level);
        const int32_t shots = std::max(1, bladeCount / 2);
        auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) * 0.7F);
        if (w.evolved)
        {
            dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
        }

        const Vector2 dir = aimAtMouse(game);
        constexpr float spreadDeg = 8.0F;

        float ringRadius = orbitRadius(w.level);
        if (w.evolved)
        {
            ringRadius *= 1.4F;
        }

        for (int32_t s = 0; s < shots; s++)
        {
            const float angleOffset =
                (static_cast<float>(s) - static_cast<float>(shots - 1) / 2) * spreadDeg * DEG2RAD;
            const float cosA = std::cos(angleOffset);
            const float sinA = std::sin(angleOffset);
            const Vector2 shotDir{.x = dir.x * cosA - dir.y * sinA,
                                  .y = dir.x * sinA + dir.y * cosA};

            // Launches from wherever that blade currently sits on the ring, not the ship's
            // center, so it visually reads as one of the spinning blades being thrown.
            const Vector2 bladePos =
                orbitBladePosition(game, game.run.player.position, ringRadius, s, bladeCount);

            game.run.orbitBladeProjectiles.push_back(
                OrbitBladeProjectile{.position = bladePos,
                                     .velocity = Vector2Scale(shotDir, orbitProjectileSpeed),
                                     .radius = orbitProjectileRadius,
                                     .damage = dmg,
                                     .active = true});
        }

        // Reuses the flashTimer field (otherwise unused by Orbit) to drive the "new blade
        // regrowing" visual in drawOrbitBlades.
        w.flashTimer = orbitRegrowDuration;
        playSFX(game, game.resources.sounds.homingLaunch);
    }
}

// Shared by any piercing (no-deactivate-on-hit) travelling projectile: orbit blade shots and the
// Ranger nerve ball behave identically (move, hit everything along the way, despawn out of range).
// The world camera is always centered on the player and shows exactly screenWidth x screenHeight
// world units (renderScale only changes pixel density, not field of view), so this is an exact
// on-screen test, not an approximation.
auto isOutsideCameraView(const Game& game, Vector2 pos, float margin) -> bool
{
    return std::abs(pos.x - game.run.player.position.x) >
               static_cast<float>(game.resources.screenWidth) / 2 + margin ||
           std::abs(pos.y - game.run.player.position.y) >
               static_cast<float>(game.resources.screenHeight) / 2 + margin;
}

void updatePiercingProjectiles(Game& game, float deltaTime,
                               std::vector<OrbitBladeProjectile>& projectiles, DamageSource source,
                               bool despawnOnCameraExit)
{
    for (auto& proj : projectiles)
    {
        if (!proj.active)
        {
            continue;
        }

        proj.position =
            Vector2Add(proj.position, Vector2Scale(proj.velocity, deltaTime * frameScale));

        const bool offscreen =
            despawnOnCameraExit
                ? isOutsideCameraView(game, proj.position, cameraDespawnMargin)
                : Vector2Distance(proj.position, game.run.player.position) > entityDespawnRadius;
        if (offscreen)
        {
            proj.active = false;
            continue;
        }

        for (size_t j = 0; j < game.run.enemies.size(); j++)
        {
            auto& enemy = game.run.enemies.at(j);
            if (enemy.active && !enemy.phased &&
                CheckCollisionCircles(proj.position, proj.radius, enemy.position,
                                      enemyKinds.at(static_cast<size_t>(enemy.kind)).radius))
            {
                damageEnemy(game, j, proj.damage);
                recordDamage(game, source, proj.damage);
            }
        }

        for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
        {
            auto& hazard = game.run.eliteHazards.at(j);
            if (hazard.active && CheckCollisionCircles(proj.position, proj.radius, hazard.position,
                                                       EliteHazardConstants::radius))
            {
                damageEliteHazard(game, j, proj.damage);
                recordDamage(game, source, proj.damage);
            }
        }

        for (auto& boss : game.run.bosses)
        {
            if (boss.health <= 0)
            {
                continue;
            }
            const Rectangle bossRect{.x = boss.position.x,
                                     .y = boss.position.y,
                                     .width = boss.size.x,
                                     .height = boss.size.y};
            if (CheckCollisionCircleRec(proj.position, proj.radius, bossRect))
            {
                damageBoss(game, boss, proj.damage);
                recordDamage(game, source, proj.damage);
            }
        }

        for (auto& asteroid : game.run.asteroids)
        {
            if (asteroid.active && CheckCollisionCircles(proj.position, proj.radius,
                                                         asteroid.position, asteroid.radius))
            {
                asteroid.active = false;
                game.run.score += asteroidScore(asteroid.tier);
                breakAsteroid(game.run.asteroids, asteroid);
            }
        }
    }

    std::erase_if(projectiles, [](const OrbitBladeProjectile& p) { return !p.active; });
}

void updateOrbitBladeProjectiles(Game& game, float deltaTime)
{
    updatePiercingProjectiles(game, deltaTime, game.run.orbitBladeProjectiles, DamageSource::Orbit,
                              false);
}

void updateNerveBallProjectiles(Game& game, float deltaTime)
{
    updatePiercingProjectiles(game, deltaTime, game.run.nerveBallProjectiles, DamageSource::Nerve,
                              true);
}

void updateNerveSpiralProjectiles(Game& game, float deltaTime)
{
    for (auto& spiral : game.run.nerveSpiralProjectiles)
    {
        if (!spiral.active)
        {
            continue;
        }

        spiral.age += deltaTime;
        spiral.origin = Vector2Add(
            spiral.origin, Vector2Scale(spiral.direction, spiral.speed * deltaTime * frameScale));
        const Vector2& center = spiral.origin;

        // life is just a generous safety-net cap; the real despawn condition is leaving the
        // camera view, same as the nerve ball.
        if (spiral.age >= spiral.life || isOutsideCameraView(game, center, cameraDespawnMargin))
        {
            spiral.active = false;
            continue;
        }

        for (int32_t s = 0; s < spiral.bladeCount; s++)
        {
            const float angle = spiral.age * nerveSpiralSpinSpeed +
                                static_cast<float>(s) * 2 * std::numbers::pi_v<float> /
                                    static_cast<float>(spiral.bladeCount);
            const Vector2 bladePos =
                Vector2Add(center, Vector2{.x = std::cos(angle) * spiral.spinRadius,
                                           .y = std::sin(angle) * spiral.spinRadius});

            for (size_t j = 0; j < game.run.enemies.size(); j++)
            {
                auto& enemy = game.run.enemies.at(j);
                if (enemy.active && !enemy.phased &&
                    CheckCollisionCircles(bladePos, orbitBladeHitRadius, enemy.position,
                                          enemyKinds.at(static_cast<size_t>(enemy.kind)).radius))
                {
                    damageEnemy(game, j, spiral.damagePerBlade);
                    recordDamage(game, DamageSource::Nerve, spiral.damagePerBlade);
                    if (!enemy.active)
                    {
                        recordDashOrNerveKill(game);
                    }
                }
            }

            for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
            {
                auto& hazard = game.run.eliteHazards.at(j);
                if (hazard.active &&
                    CheckCollisionCircles(bladePos, orbitBladeHitRadius, hazard.position,
                                          EliteHazardConstants::radius))
                {
                    damageEliteHazard(game, j, spiral.damagePerBlade);
                    recordDamage(game, DamageSource::Nerve, spiral.damagePerBlade);
                }
            }

            for (auto& boss : game.run.bosses)
            {
                if (boss.health <= 0)
                {
                    continue;
                }
                const Rectangle bossRect{.x = boss.position.x,
                                         .y = boss.position.y,
                                         .width = boss.size.x,
                                         .height = boss.size.y};
                if (CheckCollisionCircleRec(bladePos, orbitBladeHitRadius, bossRect))
                {
                    damageBoss(game, boss, spiral.damagePerBlade);
                    recordDamage(game, DamageSource::Nerve, spiral.damagePerBlade);
                }
            }

            for (auto& asteroid : game.run.asteroids)
            {
                if (asteroid.active && CheckCollisionCircles(bladePos, orbitBladeHitRadius,
                                                             asteroid.position, asteroid.radius))
                {
                    asteroid.active = false;
                    game.run.score += asteroidScore(asteroid.tier);
                    breakAsteroid(game.run.asteroids, asteroid);
                }
            }
        }
    }

    std::erase_if(game.run.nerveSpiralProjectiles,
                  [](const NerveSpiralProjectile& s) { return !s.active; });
}

void updateBeamContact(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::Beam; });

    if (it == game.run.weapons.end())
    {
        for (auto& enemy : game.run.enemies)
        {
            enemy.beamContact = false;
            enemy.beamDamageAccum = 0;
        }
        for (auto& hazard : game.run.eliteHazards)
        {
            hazard.beamContact = false;
            hazard.beamDamageAccum = 0;
        }
        for (auto& boss : game.run.bosses)
        {
            boss.beamContact = false;
            boss.beamDamageAccum = 0;
        }
        return;
    }

    const Weapon& w = *it;
    const Vector2 dir = beamAimDirection(game, w.evolved);
    const float length = beamLength(game, w.level, w.evolved);
    auto firstHit = static_cast<float>(weaponDamage(game, w.level)) * beamDamageMult;
    if (w.evolved)
    {
        firstHit *= 1.5F;
    }
    const float dps = firstHit * continuousDpsMultiplier;
    const Vector2 start = game.run.player.position;
    const Vector2 end = Vector2Add(start, Vector2Scale(dir, length));

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        auto& enemy = game.run.enemies.at(j);
        if (!enemy.active || enemy.phased ||
            !CheckCollisionCircleLine(
                enemy.position, enemyKinds.at(static_cast<size_t>(enemy.kind)).radius, start, end))
        {
            enemy.beamContact = false;
            enemy.beamDamageAccum = 0;
            continue;
        }

        if (!enemy.beamContact)
        {
            enemy.beamContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageEnemy(game, j, hit);
            recordDamage(game, DamageSource::Beam, hit);
            if (!enemy.active)
            {
                recordWeaponKill(game, WeaponType::Beam);
            }
        }
        else
        {
            enemy.beamDamageAccum += dps * deltaTime;
            if (enemy.beamDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(enemy.beamDamageAccum);
                damageEnemy(game, j, tick);
                recordDamage(game, DamageSource::Beam, tick);
                if (!enemy.active)
                {
                    recordWeaponKill(game, WeaponType::Beam);
                }
                enemy.beamDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
    {
        auto& hazard = game.run.eliteHazards.at(j);
        if (!hazard.active ||
            !CheckCollisionCircleLine(hazard.position, EliteHazardConstants::radius, start, end))
        {
            hazard.beamContact = false;
            hazard.beamDamageAccum = 0;
            continue;
        }

        if (!hazard.beamContact)
        {
            hazard.beamContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageEliteHazard(game, j, hit);
            recordDamage(game, DamageSource::Beam, hit);
        }
        else
        {
            hazard.beamDamageAccum += dps * deltaTime;
            if (hazard.beamDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(hazard.beamDamageAccum);
                damageEliteHazard(game, j, tick);
                recordDamage(game, DamageSource::Beam, tick);
                hazard.beamDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        if (!CheckCollisionCircleLine(bossCenter, boss.size.x / 2, start, end))
        {
            boss.beamContact = false;
            boss.beamDamageAccum = 0;
            continue;
        }

        if (!boss.beamContact)
        {
            boss.beamContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageBoss(game, boss, hit);
            recordDamage(game, DamageSource::Beam, hit);
        }
        else
        {
            boss.beamDamageAccum += dps * deltaTime;
            if (boss.beamDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(boss.beamDamageAccum);
                damageBoss(game, boss, tick);
                recordDamage(game, DamageSource::Beam, tick);
                boss.beamDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active &&
            CheckCollisionCircleLine(asteroid.position, asteroid.radius, start, end))
        {
            asteroid.active = false;
            game.run.score += asteroidScore(asteroid.tier);
            breakAsteroid(game.run.asteroids, asteroid);
        }
    }
}

void nerveLineHit(Game& game, Vector2 start, Vector2 end, int32_t dmg, float extraRadius)
{
    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        const auto& enemy = game.run.enemies.at(j);
        if (enemy.active && !enemy.phased &&
            CheckCollisionCircleLine(
                enemy.position, enemyKinds.at(static_cast<size_t>(enemy.kind)).radius + extraRadius,
                start, end))
        {
            damageEnemy(game, j, dmg);
            recordDamage(game, DamageSource::Nerve, dmg);
            if (!enemy.active)
            {
                recordDashOrNerveKill(game);
            }
        }
    }

    for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
    {
        const auto& hazard = game.run.eliteHazards.at(j);
        if (hazard.active &&
            CheckCollisionCircleLine(hazard.position, EliteHazardConstants::radius + extraRadius,
                                     start, end))
        {
            damageEliteHazard(game, j, dmg);
            recordDamage(game, DamageSource::Nerve, dmg);
        }
    }

    for (auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active &&
            CheckCollisionCircleLine(asteroid.position, asteroid.radius + extraRadius, start, end))
        {
            asteroid.active = false;
            game.run.score += asteroidScore(asteroid.tier);
            breakAsteroid(game.run.asteroids, asteroid);
        }
    }

    for (auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        if (CheckCollisionCircleLine(bossCenter, boss.size.x / 2 + extraRadius, start, end))
        {
            damageBoss(game, boss, dmg);
            recordDamage(game, DamageSource::Nerve, dmg);
        }
    }
}

void fireNerveBeam(Game& game)
{
    const Vector2 dir = aimAtMouse(game);
    const Vector2 start = game.run.player.position;
    const Vector2 end = Vector2Add(start, Vector2Scale(dir, nerveBurstLength));

    nerveLineHit(game, start, end, nerveBurstDamage);

    game.run.nerveBurstFlashTimer = 0.15F;
    game.run.nerveBurstFlashEnd = end;

    playSFX(game, game.resources.sounds.nerveRelease);
    duckBGM(game);
    triggerShake(game, 10, 0.3F);
    triggerHitPause(game, 0.08F);
}

void fireNerveBladeTornado(Game& game)
{
    const Vector2 dir = aimAtMouse(game);

    int32_t count = 4;
    if (const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                             { return w.type == WeaponType::Orbit; });
        it != game.run.weapons.end())
    {
        count = orbitBladeCount(it->level);
    }

    // Divided down further than a one-shot hit would need: this travels and hits repeatedly
    // per frame of overlap as it spins through a target, not once.
    const int32_t dmgPerBlade = std::max(1, nerveBurstDamage / (count * 3));

    game.run.nerveSpiralProjectiles.push_back(
        NerveSpiralProjectile{.origin = game.run.player.position,
                              .direction = dir,
                              .age = 0,
                              .speed = 9.0F,
                              .life = 8.0F,
                              .spinRadius = 50.0F,
                              .bladeCount = count,
                              .damagePerBlade = dmgPerBlade,
                              .active = true});

    playSFX(game, game.resources.sounds.nerveRelease);
    duckBGM(game);
    triggerShake(game, 10, 0.3F);
    triggerHitPause(game, 0.08F);
}

void fireNerveBall(Game& game)
{
    const Vector2 dir = aimAtMouse(game);
    const int32_t dmg = std::max(1, nerveBurstDamage / 4);

    game.run.nerveBallProjectiles.push_back(
        OrbitBladeProjectile{.position = game.run.player.position,
                             .velocity = Vector2Scale(dir, 8.0F),
                             .radius = 55.0F,
                             .damage = dmg,
                             .active = true});

    playSFX(game, game.resources.sounds.nerveRelease);
    duckBGM(game);
    triggerShake(game, 10, 0.3F);
    triggerHitPause(game, 0.08F);
}

void fireNerveBurst(Game& game)
{
    switch (static_cast<ShipClass>(game.resources.settings.shipIndex))
    {
    case ShipClass::Bastion:
        fireNerveBladeTornado(game);
        break;
    case ShipClass::Interceptor:
        fireNerveBeam(game);
        break;
    case ShipClass::Ranger:
    case ShipClass::Count:
    default:
        fireNerveBall(game);
        break;
    }
}

void updateWaveSpawner(Game& game, float deltaTime)
{
    if (game.sandbox)
    {
        return;
    }

    if (game.run.bosses.empty())
    {
        game.run.waveTimer -= deltaTime;
        if (game.run.waveTimer <= 0)
        {
            game.run.waveNumber++;
            game.run.waveTimer = GameConstants::waveDuration;
            recordWaveReached(game, game.run.waveNumber);

            // M11: wave 100 (and every 100th wave after, since reaching wave 200+ is only
            // possible once the wave-100 boss has already been beaten) spawns the final boss
            // instead of a normal mega-boss.
            if (game.run.waveNumber % finalBossWaveInterval == 0)
            {
                spawnFinalBossWave(game);
            }
            else if (game.run.waveNumber % megaBossWaveInterval == 0)
            {
                spawnBossWave(game, megaBossHealthMult, megaBossSizeMult, true);
            }
            else if (game.run.waveNumber % miniBossWaveInterval == 0)
            {
                spawnBossWave(game, miniBossHealthMult, miniBossSizeMult, false);
            }
        }
    }

    game.run.enemySpawnTimer -= deltaTime;
    if (game.run.enemySpawnTimer <= 0 &&
        static_cast<int>(game.run.enemies.size()) < UpdateConstants::maxEnemies)
    {

        const int cycleIndex = (game.run.waveNumber - 1) / spawnRateCycleLength;
        const int cyclePosition = (game.run.waveNumber - 1) % spawnRateCycleLength + 1;

        const float baseInterval = std::max(
            1.2F - static_cast<float>(cycleIndex) * spawnRateCycleStep, spawnIntervalFloor);
        const float phaseMultiplier =
            cyclePosition <= spawnRateCycleLength / 2 ? 1.0F : spawnRateSecondHalfMultiplier;
        const float interval = std::max(baseInterval * phaseMultiplier, spawnIntervalFloor);

        game.run.enemySpawnTimer =
            interval * difficultyDefs.at(static_cast<size_t>(game.resources.settings.difficulty))
                           .spawnRateMult;

        const int spawnCount = 1 + game.run.waveNumber / 4;
        for (int i = 0; i < spawnCount &&
                        static_cast<int>(game.run.enemies.size()) < UpdateConstants::maxEnemies;
             i++)
        {
            spawnEnemy(game);
        }
    }

    const bool asteroidsUnlocked =
        game.resources.settings.difficulty != Difficulty::Easy || game.run.waveNumber >= 10;

    game.run.asteroidSpawnTimer -= deltaTime;
    if (asteroidsUnlocked && game.run.asteroidSpawnTimer <= 0 &&
        static_cast<int>(game.run.asteroids.size()) < asteroidCap(game))
    {
        game.run.asteroidSpawnTimer =
            static_cast<float>(GetRandomValue(8, 16)) / 10.0F * asteroidIntervalMultiplier(game);

        AsteroidTier tier = AsteroidTier::Large;
        if (GetRandomValue(0, 1) == 1)
        {
            tier = AsteroidTier::Medium;
        }

        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const Vector2 spawnPos =
            Vector2Add(game.run.player.position,
                       Vector2{.x = std::cos(angle) * 500, .y = std::sin(angle) * 500});
        const Vector2 aimPoint = Vector2Add(
            game.run.player.position, Vector2{.x = static_cast<float>(GetRandomValue(-180, 180)),
                                              .y = static_cast<float>(GetRandomValue(-180, 180))});
        const Vector2 direction = Vector2Normalize(Vector2Subtract(aimPoint, spawnPos));
        const float speed = static_cast<float>(GetRandomValue(25, 45)) / 10.0F;

        game.run.asteroids.push_back(Asteroid{.position = spawnPos,
                                              .velocity = Vector2Scale(direction, speed),
                                              .radius = asteroidRadius(tier),
                                              .tier = tier,
                                              .active = true});
    }
}

auto asteroidIntervalMultiplier(const Game& game) -> float
{
    return game.resources.settings.difficulty == Difficulty::Hard ? 1.0F : 1.6F;
}

auto asteroidCap(const Game& game) -> int
{
    switch (game.resources.settings.difficulty)
    {
    case Difficulty::Hard:
        return maxAsteroid;
    case Difficulty::Easy:
        return maxAsteroid / 3;
    default:
        return maxAsteroid * 2 / 3;
    }
}

auto eliteHazardCap(const Game& game) -> int
{
    switch (game.resources.settings.difficulty)
    {
    case Difficulty::Easy:
        return 1;
    case Difficulty::Hard:
        return 3;
    default:
        return 2;
    }
}

auto waveEnemyScale(const Game& game) -> float
{
    return 1 + static_cast<float>(game.run.waveNumber - 1) * UpdateConstants::waveEnemyScalePerWave;
}

auto enemyDamage(const Game& game, int32_t base) -> int32_t
{
    return static_cast<int32_t>(
        static_cast<float>(base) *
        difficultyDefs.at(static_cast<size_t>(game.resources.settings.difficulty)).enemyDamageMult *
        waveEnemyScale(game));
}

void spawnEnemy(Game& game)
{
    std::vector<int> eligible;
    eligible.reserve(enemyKinds.size());
    for (size_t i = 0; i < enemyKinds.size(); i++)
    {
        if (enemyKinds.at(i).minWave <= game.run.waveNumber)
        {
            eligible.push_back(static_cast<int>(i));
        }
    }
    if (eligible.empty())
    {
        return;
    }

    const int kindIndex =
        eligible.at(static_cast<size_t>(GetRandomValue(0, static_cast<int>(eligible.size()) - 1)));
    spawnEnemyAt(game, kindIndex, spawnRingPosition(game));
}

auto spawnRingPosition(const Game& game) -> Vector2
{
    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(420, 520));
    return Vector2Add(game.run.player.position,
                      Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
}

void spawnEnemyAt(Game& game, int kindIndex, Vector2 pos)
{
    if (static_cast<int>(game.run.enemies.size()) >= UpdateConstants::maxEnemies)
    {
        return;
    }

    const auto& kind = enemyKinds.at(static_cast<size_t>(kindIndex));
    const bool elite = GetRandomValue(0, 999) < static_cast<int32_t>(eliteChance * 1000);

    auto health = static_cast<int32_t>(
        static_cast<float>(kind.health) *
        difficultyDefs.at(static_cast<size_t>(game.resources.settings.difficulty)).enemyHealthMult *
        waveEnemyScale(game));
    if (elite)
    {
        health *= 2;
    }

    game.run.enemies.push_back(Enemy{.kind = kindIndex,
                                     .position = pos,
                                     .velocity = Vector2{},
                                     .health = health,
                                     .active = true,
                                     .stateTimer = kind.fireInterval + kind.spawnInterval + 1,
                                     .charging = false,
                                     .telegraphing = false,
                                     .phased = false,
                                     .orbitAngle = 0,
                                     .orbitDist = 200,
                                     .isElite = elite,
                                     .hitByDash = false,
                                     .hitFlashTimer = 0});
}

auto bossMoveCountForDifficulty(Difficulty difficulty) -> int
{
    switch (difficulty)
    {
    case Difficulty::Easy:
        return 2;
    case Difficulty::Hard:
        return 4;
    default:
        return 3;
    }
}

auto sampleBossMoveset(int count) -> std::vector<BossAttack>
{
    std::vector<BossAttack> pool;
    pool.reserve(bossAttackCount);
    for (int i = 0; i < bossAttackCount; i++)
    {
        if (const auto attack = static_cast<BossAttack>(i); DemoConfig::isBossAttackAllowed(attack))
        {
            pool.push_back(attack);
        }
    }

    count = std::min(count, static_cast<int>(pool.size()));
    for (int i = 0; i < count; i++)
    {
        const int32_t j = i + GetRandomValue(0, static_cast<int32_t>(pool.size()) - i - 1);
        std::swap(pool.at(static_cast<size_t>(i)), pool.at(static_cast<size_t>(j)));
    }
    pool.resize(static_cast<size_t>(count));
    return pool;
}

void spawnBossInstance(Game& game, float healthMult, float sizeMult, bool isMega, bool isSwarm,
                       bool isFinal = false)
{
    const int32_t tier = game.run.waveNumber / megaBossWaveInterval;

    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(1200, 1500));
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

    // M11: the wave-100 placeholder final boss is a deliberately-picked, stat-boosted BossType
    // rather than a random one — a bespoke final-boss shape/identity is a later pass.
    auto typeIndex =
        static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(bossTypes.size()) - 1));
    if constexpr (DemoConfig::isDemoBuild)
    {
        typeIndex = DemoConfig::allowedBossTypeIndices.at(static_cast<size_t>(GetRandomValue(
            0, static_cast<int32_t>(DemoConfig::allowedBossTypeIndices.size()) - 1)));
    }
    if (isFinal)
    {
        typeIndex = bossTypes.size() - 1;
    }
    const auto& type = bossTypes.at(typeIndex);

    const float finalBossMult = isFinal ? 3.0F : 1.0F;
    const auto health = static_cast<int32_t>(
        static_cast<float>(500 + tier * 250) * healthMult * type.healthMult * finalBossMult *
        difficultyDefs.at(static_cast<size_t>(game.resources.settings.difficulty)).enemyHealthMult);
    const float size = 100.0F * sizeMult * type.sizeMult * (isFinal ? 1.3F : 1.0F);

    auto moveset =
        sampleBossMoveset(bossMoveCountForDifficulty(game.resources.settings.difficulty));
    const BossAttack firstAttack = moveset.front();

    game.run.bosses.push_back(
        Boss{.position = spawnPos,
             .size = Vector2{.x = size, .y = size},
             .color = type.color,
             .baseColor = type.color,
             .health = health,
             .maxHealth = health,
             .state = BossState::IDLE,
             .attack = firstAttack,
             .moveset = std::move(moveset),
             .attackTimer = static_cast<float>(GetRandomValue(15, 35)) / 10.0F,
             .stateTimer = 0,
             .targetPosition = Vector2{},
             .slamHit = false,
             .beamShieldLatched = false,
             .wormholeBeamOrigin = Vector2{},
             .chargeVelocity = Vector2{},
             .barrageTimer = 0,
             .hitByDash = false,
             .isMega = isMega,
             .isSwarm = isSwarm,
             .strafePhase = static_cast<float>(GetRandomValue(0, 628)) / 100.0F,
             .hitFlashTimer = 0,
             .shape = type.shape,
             .isFinalBoss = isFinal});
}

void spawnBossWave(Game& game, float healthMult, float sizeMult, bool isMega)
{
    game.run.bossSpawnCount++;
    const bool isSwarm = game.run.bossSpawnCount % bossSwarmInterval == 0;
    const int count = isSwarm ? 3 : 1;
    for (int i = 0; i < count; i++)
    {
        spawnBossInstance(game, healthMult, sizeMult, isMega, isSwarm);
    }
    playSFX(game, game.resources.sounds.bossWindUp);
}

// M11: wave 100 (and every 100th wave after, once infinite mode is unlocked) spawns this single
// stat-boosted placeholder boss instead of a normal mega-boss.
void spawnFinalBossWave(Game& game)
{
    game.run.bossSpawnCount++;
    spawnBossInstance(game, megaBossHealthMult, megaBossSizeMult, true, false, true);
    playSFX(game, game.resources.sounds.bossWindUp);
}

void spawnBoss(Game& game) { spawnBossWave(game, megaBossHealthMult, megaBossSizeMult, true); }

void spawnMiniboss(Game& game) { spawnBossWave(game, miniBossHealthMult, miniBossSizeMult, false); }

void spawnSwarmBoss(Game& game)
{
    for (int i = 0; i < 3; i++)
    {
        spawnBossInstance(game, megaBossHealthMult, megaBossSizeMult, true, true);
    }
    playSFX(game, game.resources.sounds.bossWindUp);
}

void updateBossMovement(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
{
    if (boss.hitFlashTimer > 0)
    {
        boss.hitFlashTimer -= deltaTime;
    }

    if (boss.state == BossState::SHOOTING &&
        (boss.attack == BossAttack::ChargeDash || boss.attack == BossAttack::Slam ||
         boss.attack == BossAttack::WormholeBeam || boss.attack == BossAttack::Beam))
    {
        return;
    }

    const Vector2 toPlayer = Vector2Subtract(game.run.player.position, bossCenter);
    const float dist = Vector2Length(toPlayer);
    if (dist <= 1.0F)
    {
        return;
    }

    const Vector2 dirToPlayer = Vector2Scale(toPlayer, 1.0F / dist);
    const Vector2 perp{.x = -dirToPlayer.y, .y = dirToPlayer.x};

    const bool enraged =
        boss.health > 0 &&
        static_cast<float>(boss.health) <= static_cast<float>(boss.maxHealth) * enrageHealthFrac;
    const float speedMult = enraged ? 1.6F : 1.0F;

    const float radialAmount =
        std::clamp((dist - bossEngageDistance) / bossEngageDistance, -1.5F, 1.5F);
    const float strafe = std::sin(static_cast<float>(GetTime()) * 0.9F + boss.strafePhase);

    Vector2 move = Vector2Add(Vector2Scale(dirToPlayer, radialAmount), Vector2Scale(perp, strafe));
    if (Vector2Length(move) > 0)
    {
        const float chaseSpeed = (game.run.player.speed * 1.15F + 1) * speedMult;
        move = Vector2Scale(Vector2Normalize(move), chaseSpeed);
        boss.position = Vector2Add(boss.position, Vector2Scale(move, deltaTime * frameScale));
    }
}

void updatePickups(Game& game, float deltaTime)
{
    const float magnetRadius = 70 + 70 *
                                        static_cast<float>(game.run.skillLevels.at(
                                            static_cast<size_t>(SkillType::PickupRadius))) *
                                        0.2F;

    for (auto& pickup : game.run.pickups)
    {
        if (!pickup.active)
        {
            continue;
        }

        pickup.lifetime -= deltaTime;
        if (pickup.lifetime <= 0)
        {
            pickup.active = false;
            continue;
        }

        if (game.run.blackhole.active &&
            Vector2Distance(pickup.position, game.run.blackhole.position) <=
                game.run.blackhole.radius)
        {
            pickup.active = false;
            continue;
        }

        const Vector2 toPlayer = Vector2Subtract(game.run.player.position, pickup.position);
        const float dist = Vector2Length(toPlayer);

        if (dist <= magnetRadius && dist > 0)
        {
            const Vector2 pull = Vector2Scale(Vector2Normalize(toPlayer), pickupMagnetSpeed);
            pickup.position =
                Vector2Add(pickup.position, Vector2Scale(pull, deltaTime * frameScale));
        }

        if (dist <= game.run.player.radius + 6)
        {
            pickup.active = false;

            if (pickup.type == PickupType::XP)
            {
                game.run.xp += pickup.value;
            }
            else
            {
                applyPickupEffect(game, pickup.type, pickup.element, pickup.mechanism, &pickup);
            }
        }
    }
}

// Shared by world-pickup collection and the instant level-up "Pickup" choice. `except` excludes
// the originating world pickup from MagnetPulse's sweep (nullptr when there isn't one, i.e. when
// applied directly from a level-up choice).
void applyPickupEffect(Game& game, PickupType type, ElementType element, ElementMechanism mechanism,
                       const Pickup* except)
{
    switch (type)
    {
    case PickupType::XP:
        break;
    case PickupType::LifeOrb:
        collectLifeOrb(game);
        break;
    case PickupType::Shield:
        if (game.run.player.shieldStacks < currentShip(game).maxShieldStacks)
        {
            game.run.player.shieldStacks++;
        }
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::Elemental:
        collectElementalPickup(game, element, mechanism);
        break;
    case PickupType::Regen:
        game.run.player.regenTimer =
            difficultyPickupDuration.at(static_cast<size_t>(game.resources.settings.difficulty)) *
            pickupDurationScale(game.run.waveNumber);
        game.run.player.regenRate = regenRate;
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::DashTrail:
        game.run.player.dashTrailUnlocked = true;
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::MagnetPulse:
        for (auto& other : game.run.pickups)
        {
            if (other.active && &other != except)
            {
                const Vector2 toPlayer = Vector2Subtract(game.run.player.position, other.position);
                if (Vector2Length(toPlayer) > magnetPulseSnapDistance)
                {
                    other.position = Vector2Subtract(
                        game.run.player.position,
                        Vector2Scale(Vector2Normalize(toPlayer), magnetPulseSnapDistance));
                }
            }
        }
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::Overcharge:
        game.run.player.overchargeTimer =
            overchargeDuration * pickupDurationScale(game.run.waveNumber);
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::SecondWind:
        game.run.player.secondWindReady = true;
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::Danger:
        // One downgrade at a time, kept simple: a second Danger pickup while one is already
        // active is a no-op rather than stacking or restacking the timer.
        if (!game.run.weaponDowngrade.has_value() && !game.run.weapons.empty())
        {
            auto& weapon = game.run.weapons.at(static_cast<size_t>(
                GetRandomValue(0, static_cast<int32_t>(game.run.weapons.size()) - 1)));
            const int32_t amount = std::min(dangerDowngradeLevels, weapon.level - 1);
            if (amount > 0)
            {
                weapon.level -= amount;
                game.run.weaponDowngrade = WeaponDowngrade{
                    .type = weapon.type, .amount = amount, .timer = dangerDowngradeDuration};
                triggerShake(game, 6, 0.25F);
                playSFX(game, game.resources.sounds.explosion);
                game.run.achievementToast =
                    std::string(weaponDisplayName(weapon.type)) + " downgraded!";
                game.run.achievementToastTimer = 3.0F;
            }
        }
        break;
    case PickupType::Count:
        break;
    }
}

void collectElementalPickup(Game& game, ElementType element, ElementMechanism mechanism)
{
    const float burnDps = currentBurnDps(game);

    switch (mechanism)
    {
    case ElementMechanism::Infusion:
        game.run.player.elementalBuffTimer.at(static_cast<size_t>(element)) =
            difficultyPickupDuration.at(static_cast<size_t>(game.resources.settings.difficulty)) *
            pickupDurationScale(game.run.waveNumber);
        break;
    case ElementMechanism::Nova:
        for (auto& enemy : game.run.enemies)
        {
            if (enemy.active && !enemy.phased)
            {
                applyElementDebuff(enemy, element, burnDps);
            }
        }
        break;
    case ElementMechanism::Field:
        game.run.elementalFields.push_back(
            ElementalField{.position = game.run.player.position,
                           .radius = elementalFieldRadius,
                           .timer = difficultyPickupDuration.at(
                                        static_cast<size_t>(game.resources.settings.difficulty)) *
                                    pickupDurationScale(game.run.waveNumber),
                           .element = element,
                           .active = true});
        break;
    case ElementMechanism::Count:
        break;
    }

    playSFX(game, game.resources.sounds.menuConfirm);
}

void updateElementalFields(Game& game, float deltaTime)
{
    const float burnDps = currentBurnDps(game);

    for (auto& field : game.run.elementalFields)
    {
        if (!field.active)
        {
            continue;
        }

        field.timer -= deltaTime;
        if (field.timer <= 0)
        {
            field.active = false;
            continue;
        }

        for (auto& enemy : game.run.enemies)
        {
            if (enemy.active && !enemy.phased &&
                CheckCollisionCircles(field.position, field.radius, enemy.position,
                                      enemyKinds.at(static_cast<size_t>(enemy.kind)).radius))
            {
                applyElementDebuff(enemy, field.element, burnDps);
            }
        }
    }

    std::erase_if(game.run.elementalFields, [](const ElementalField& f) { return !f.active; });
}

void updatePlayerBuffs(Game& game, float deltaTime)
{
    auto& player = game.run.player;

    for (auto& timer : player.elementalBuffTimer)
    {
        if (timer > 0)
        {
            timer = std::max(0.0F, timer - deltaTime);
        }
    }

    if (player.regenTimer > 0)
    {
        player.regenTimer = std::max(0.0F, player.regenTimer - deltaTime);
        player.health = std::min(player.maxHealth, player.health + player.regenRate * deltaTime);
    }

    if (player.overchargeTimer > 0)
    {
        player.overchargeTimer = std::max(0.0F, player.overchargeTimer - deltaTime);
    }

    if (player.dashing && player.dashTrailUnlocked)
    {
        const float jitterAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const float jitterSpeed = static_cast<float>(GetRandomValue(2, 8)) / 10.0F;
        game.run.dashTrailParticles.push_back(
            Particle{.position = player.position,
                     .velocity = Vector2{.x = std::cos(jitterAngle) * jitterSpeed,
                                         .y = std::sin(jitterAngle) * jitterSpeed},
                     .radius = dashTrailRadius,
                     .life = dashTrailParticleLife,
                     .maxLife = dashTrailParticleLife,
                     .color = Palette::Accent});

        for (size_t i = 0; i < game.run.enemies.size(); i++)
        {
            auto& enemy = game.run.enemies.at(i);
            if (enemy.active && !enemy.phased &&
                CheckCollisionCircles(player.position, dashTrailRadius, enemy.position,
                                      enemyKinds.at(static_cast<size_t>(enemy.kind)).radius))
            {
                damageEnemy(game, i, dashTrailDamage);
                recordDamage(game, DamageSource::Dash, dashTrailDamage);
            }
        }
    }

    for (auto& particle : game.run.dashTrailParticles)
    {
        particle.position = Vector2Add(particle.position, particle.velocity);
        particle.life -= deltaTime;
    }
    std::erase_if(game.run.dashTrailParticles, [](const Particle& p) { return p.life <= 0; });
}

void collectLifeOrb(Game& game)
{
    game.run.player.health = std::min(game.run.player.health + 0.5F, game.run.player.maxHealth);
}

void startLevelUp(Game& game)
{
    game.run.xp -= game.run.xpToNext;
    game.run.level++;
    game.run.xpToNext = 100 + (game.run.level - 1) * 60;

    game.run.pendingChoices = rollLevelUpChoices(game);
    game.menuIndex = 0;
    game.state = GameState::LEVEL_UP;
    playSFX(game, game.resources.sounds.menuConfirm);
}

auto isFusedPassive(const Game& game, SkillType id) -> bool
{
    return std::ranges::any_of(
        game.run.weapons, [&](const Weapon& w)
        { return w.evolved && skillLinkedPassive.at(static_cast<size_t>(w.type)) == id; });
}

auto equippedSlotCount(const Game& game) -> int
{
    int count = 0;
    for (size_t i = 0; i < static_cast<size_t>(SkillType::Count); i++)
    {
        const auto id = static_cast<SkillType>(i);
        if (game.run.skillLevels.at(i) > 0 && !isFusedPassive(game, id))
        {
            count++;
        }
    }
    return count;
}

auto hasWeapon(const Game& game, WeaponType kind) -> bool
{
    return std::ranges::any_of(game.run.weapons,
                               [kind](const Weapon& w) { return w.type == kind; });
}

auto sampleDistinct(std::vector<SkillType> ids, int count) -> std::vector<SkillType>
{
    if (count > static_cast<int>(ids.size()))
    {
        count = static_cast<int>(ids.size());
    }
    for (int i = 0; i < count; i++)
    {
        const int32_t j = i + GetRandomValue(0, static_cast<int32_t>(ids.size()) - i - 1);
        std::swap(ids.at(static_cast<size_t>(i)), ids.at(static_cast<size_t>(j)));
    }
    ids.resize(static_cast<size_t>(count));
    return ids;
}

auto demoSkillAllowed(const Game& game, SkillType id) -> bool
{
    for (size_t i = 0; i < weaponGrantSkill.size(); i++)
    {
        const auto kind = static_cast<WeaponType>(i);
        if (weaponGrantSkill.at(i) == id)
        {
            return DemoConfig::isWeaponAllowed(kind) && isWeaponTypeUnlocked(game, kind);
        }
        if (skillLinkedPassive.at(i) == id)
        {
            return DemoConfig::isWeaponAllowed(kind);
        }
    }
    return true;
}

auto rollLevelUpChoices(Game& game) -> std::vector<LevelUpChoice>
{
    const bool slotsFull = equippedSlotCount(game) >= game.resources.achievements.slotCap;

    std::vector<SkillType> eligible;
    eligible.reserve(static_cast<size_t>(SkillType::Count));
    for (size_t i = 0; i < static_cast<size_t>(SkillType::Count); i++)
    {
        const auto id = static_cast<SkillType>(i);
        if (isFusedPassive(game, id) || game.run.skillLevels.at(i) >= Skills.at(i).maxLevel ||
            !demoSkillAllowed(game, id))
        {
            continue;
        }
        if (game.run.skillLevels.at(i) > 0 || !slotsFull)
        {
            eligible.push_back(id);
        }
    }

    std::vector<LevelUpChoice> choices;
    if (eligible.empty())
    {
        choices = rollRewardChoices(game);

        game.run.postCapDamageLevels++;
    }
    else
    {
        // Affinity: the first choice always favors a skill/weapon already owned, so early levels
        // (few unlocked weapon slots) aren't dominated by picks for things not yet equipped.
        std::vector<SkillType> owned;
        for (const auto id : eligible)
        {
            if (game.run.skillLevels.at(static_cast<size_t>(id)) > 0)
            {
                owned.push_back(id);
            }
        }

        std::vector<SkillType> picks;
        if (owned.empty())
        {
            picks = sampleDistinct(eligible, 3);
        }
        else
        {
            picks = sampleDistinct(owned, 1);
            std::vector<SkillType> rest;
            rest.reserve(eligible.size());
            for (const auto id : eligible)
            {
                if (id != picks.front())
                {
                    rest.push_back(id);
                }
            }
            const auto more = sampleDistinct(rest, 2);
            picks.insert(picks.end(), more.begin(), more.end());
        }

        for (const auto id : picks)
        {
            choices.push_back(LevelUpChoice{.type = ChoiceType::Skill, .skill = id});
        }

        // Always offer an escape hatch: a random pickup in case none of the skill picks appeal.
        const auto& entry = pickupCatalog.at(pickWeightedPickupIndex(game.run.waveNumber));
        choices.push_back(LevelUpChoice{.type = ChoiceType::Pickup,
                                        .pickupType = entry.type,
                                        .element = entry.element,
                                        .mechanism = entry.mechanism});
    }

    std::vector<WeaponType> evolvable;
    for (size_t i = 0; i < weaponGrantSkill.size(); i++)
    {
        const auto kind = static_cast<WeaponType>(i);
        const auto grantSkill = weaponGrantSkill.at(i);
        const auto passive = skillLinkedPassive.at(i);
        if (hasWeapon(game, kind) && !weaponEvolved(game, kind) &&
            game.run.skillLevels.at(static_cast<size_t>(grantSkill)) >= 3 &&
            game.run.skillLevels.at(static_cast<size_t>(passive)) >= 3 &&
            game.resources.achievements.evolutionUnlocked.at(i))
        {
            evolvable.push_back(kind);
        }
    }
    if (!evolvable.empty())
    {
        const auto pick = evolvable.at(
            static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(evolvable.size()) - 1)));
        choices.push_back(LevelUpChoice{.type = ChoiceType::Evolve, .weapon = pick});
    }

    return choices;
}

auto rollRewardChoices(const Game& game) -> std::vector<LevelUpChoice>
{
    std::vector<size_t> picked;
    while (picked.size() < 3 && picked.size() < pickupCatalog.size())
    {
        const size_t idx = pickWeightedPickupIndex(game.run.waveNumber);
        if (std::ranges::find(picked, idx) == picked.end())
        {
            picked.push_back(idx);
        }
    }

    std::vector<LevelUpChoice> choices;
    for (const auto idx : picked)
    {
        const auto& entry = pickupCatalog.at(idx);
        choices.push_back(LevelUpChoice{.type = ChoiceType::Pickup,
                                        .pickupType = entry.type,
                                        .element = entry.element,
                                        .mechanism = entry.mechanism});
    }
    return choices;
}

auto weaponEvolved(const Game& game, WeaponType kind) -> bool
{
    for (const auto& w : game.run.weapons)
    {
        if (w.type == kind)
        {
            return w.evolved;
        }
    }
    return false;
}

void applySkill(Game& game, SkillType id)
{
    game.run.skillLevels.at(static_cast<size_t>(id))++;

    if (const auto kind = weaponForGrantSkill(id); kind.has_value())
    {
        grantOrLevelWeapon(game, *kind);
        if (game.run.skillLevels.at(static_cast<size_t>(id)) >=
            Skills.at(static_cast<size_t>(id)).maxLevel)
        {
            recordSkillMaxed(game, id);
        }
        return;
    }

    switch (id)
    {
    case SkillType::MoveSpeed:
        game.run.player.speed *= 1.1F;
        break;
    case SkillType::MaxHp:
        game.run.player.maxHealth++;
        game.run.player.health = game.run.player.maxHealth;
        break;
    default:

        break;
    }
}

auto weaponForGrantSkill(SkillType id) -> std::optional<WeaponType>
{
    for (size_t i = 0; i < weaponGrantSkill.size(); i++)
    {
        if (weaponGrantSkill.at(i) == id)
        {
            return static_cast<WeaponType>(i);
        }
    }
    return std::nullopt;
}

void grantOrLevelWeapon(Game& game, WeaponType kind)
{
    for (auto& w : game.run.weapons)
    {
        if (w.type == kind)
        {
            w.level++;
            return;
        }
    }
    game.run.weapons.push_back(Weapon{.type = kind, .level = 1});
}

void applyEvolution(Game& game, WeaponType kind)
{
    for (auto& w : game.run.weapons)
    {
        if (w.type == kind)
        {
            w.evolved = true;
            triggerShake(game, 10, 0.3F);
            playSFX(game, game.resources.sounds.critical);
            return;
        }
    }
}

void updateBullets(Game& game, float deltaTime)
{
    for (auto& bullet : game.run.bullets)
    {
        if (!bullet.active)
        {
            continue;
        }

        bullet.position =
            Vector2Add(bullet.position, Vector2Scale(bullet.velocity, deltaTime * frameScale));
        applyWormholeTransit(game, bullet.position, bullet.velocity, bullet.radius);

        for (auto& boss : game.run.bosses)
        {
            const Rectangle bossRect{.x = boss.position.x,
                                     .y = boss.position.y,
                                     .width = boss.size.x,
                                     .height = boss.size.y};
            if (bullet.active && boss.health > 0 &&
                CheckCollisionCircleRec(bullet.position, bullet.radius, bossRect))
            {
                bullet.active = false;
                damageBoss(game, boss, bullet.damage);
                recordDamage(game, bullet.source, bullet.damage);
            }
        }

        for (auto& asteroid : game.run.asteroids)
        {
            if (bullet.active && asteroid.active &&
                CheckCollisionCircles(bullet.position, bullet.radius, asteroid.position,
                                      asteroid.radius))
            {
                bullet.active = false;
                asteroid.active = false;
                game.run.score += asteroidScore(asteroid.tier);
                breakAsteroid(game.run.asteroids, asteroid);
                playSFX(game, game.resources.sounds.explosion);
            }
        }

        for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
        {
            auto& hazard = game.run.eliteHazards.at(j);
            if (bullet.active && hazard.active &&
                CheckCollisionCircles(bullet.position, bullet.radius, hazard.position,
                                      EliteHazardConstants::radius))
            {
                bullet.active = false;
                damageEliteHazard(game, j, bullet.damage);
                recordDamage(game, bullet.source, bullet.damage);
            }
        }

        for (size_t j = 0; j < game.run.enemies.size(); j++)
        {
            auto& enemy = game.run.enemies.at(j);
            if (!bullet.active || !enemy.active || enemy.phased)
            {
                continue;
            }
            const auto& kind = enemyKinds.at(static_cast<size_t>(enemy.kind));
            if (CheckCollisionCircles(bullet.position, bullet.radius, enemy.position, kind.radius))
            {
                damageEnemy(game, j, bullet.damage);
                recordDamage(game, bullet.source, bullet.damage);
                if (!enemy.active)
                {
                    if (bullet.source == DamageSource::Forward)
                    {
                        recordWeaponKill(game, WeaponType::Forward);
                    }
                    else if (bullet.source == DamageSource::FlakCannon)
                    {
                        recordWeaponKill(game, WeaponType::FlakCannon);
                    }
                    else if (bullet.source == DamageSource::Railgun)
                    {
                        recordWeaponKill(game, WeaponType::Railgun);
                    }
                    else if (bullet.source == DamageSource::TurretDeploy)
                    {
                        recordWeaponKill(game, WeaponType::TurretDeploy);
                    }
                }

                // M15 Flak Cannon: splash nearby enemies once on the same hit that damaged the
                // primary target (a small, accepted overlap rather than excluding the primary).
                if (bullet.splashRadius > 0)
                {
                    aoePulse(game, bullet.position, bullet.splashRadius, bullet.damage / 2,
                             bullet.source);
                }

                if (bullet.pierceRemaining > 0)
                {
                    // Photon Cannon / Flak Cannon L3 / Railgun: punches through instead of
                    // stopping at the first enemy.
                    bullet.pierceRemaining--;
                }
                else if (bullet.ricochetRemaining > 0)
                {
                    // M15 Ricochet: bounce to another active enemy instead of deactivating.
                    if (const auto next =
                            nearestEnemyExcluding(game, bullet.position, enemy.position);
                        next.has_value())
                    {
                        bullet.velocity =
                            Vector2Scale(Vector2Normalize(Vector2Subtract(*next, bullet.position)),
                                         Vector2Length(bullet.velocity));
                        bullet.ricochetRemaining--;
                    }
                    else
                    {
                        bullet.active = false;
                    }
                }
                else
                {
                    bullet.active = false;
                }
            }
        }

        for (auto& projectile : game.run.bossProjectiles)
        {
            if (bullet.active && projectile.active && !projectile.fromPlayer &&
                CheckCollisionCircles(bullet.position, bullet.radius, projectile.position,
                                      projectile.radius))
            {
                bullet.active = false;
                projectile.health -= 1;
                if (projectile.health <= 0)
                {
                    projectile.active = false;
                    game.run.score += 5;
                }
            }
        }

        if (Vector2Distance(bullet.position, game.run.player.position) > entityDespawnRadius)
        {
            bullet.active = false;
        }
    }
}

// M15 Chain Lightning: instant multi-target hit at fire time (no travelling projectile). Arcs
// from the nearest enemy to the next-nearest untouched enemy, up to `weapon.level` total targets.
void fireChainLightning(Game& game, const Weapon& weapon)
{
    const int32_t dmg = weaponDamage(game, weapon.level);

    auto target = nearestEnemyWithin(game, game.run.player.position, chainLightningRange);
    if (!target.has_value())
    {
        return;
    }

    Vector2 from = game.run.player.position;
    for (int32_t hit = 0; hit < weapon.level; hit++)
    {
        if (!target.has_value())
        {
            break;
        }
        const Vector2 to = *target;

        for (size_t j = 0; j < game.run.enemies.size(); j++)
        {
            auto& enemy = game.run.enemies.at(j);
            if (enemy.active && !enemy.phased &&
                Vector2Distance(enemy.position, to) <=
                    enemyKinds.at(static_cast<size_t>(enemy.kind)).radius)
            {
                damageEnemy(game, j, dmg);
                recordDamage(game, DamageSource::ChainLightning, dmg);
                if (!enemy.active)
                {
                    recordWeaponKill(game, WeaponType::ChainLightning);
                }
                break;
            }
        }

        game.run.chainLightningBolts.push_back(
            ChainLightningBolt{.from = from, .to = to, .timer = chainLightningBoltLife});

        from = to;
        target = nearestEnemyExcluding(game, to, to);
    }
    playSFX(game, game.resources.sounds.shoot);
}

void updateChainLightningBolts(Game& game, float deltaTime)
{
    for (auto& bolt : game.run.chainLightningBolts)
    {
        bolt.timer -= deltaTime;
    }
    std::erase_if(game.run.chainLightningBolts,
                  [](const ChainLightningBolt& bolt) { return bolt.timer <= 0; });
}

// M15 Follower Drone / Laser Drone: keeps the live drone count in sync with the weapon's current
// level (1 drone at L1, 2 at L3+), independent of exactly when the level-up happened.
auto droneCountForLevel(int32_t level) -> size_t
{
    if (level >= 3)
    {
        return 2;
    }
    return level >= 1 ? 1 : 0;
}

void updateFollowerDrones(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::FollowerDrone; });
    if (it == game.run.weapons.end())
    {
        game.run.followerDrones.clear();
        return;
    }

    const Weapon& w = *it;
    const size_t desired = droneCountForLevel(w.level);
    while (game.run.followerDrones.size() < desired)
    {
        game.run.followerDrones.push_back(
            FollowerDrone{.position = game.run.player.position, .orbitAngle = 0, .attackTimer = 0});
    }
    game.run.followerDrones.resize(desired);

    const auto dmg = static_cast<int32_t>(shipCurrentDamage(game) * droneDamageFraction(w.level));
    const float interval = droneAttackIntervalBase / (w.level >= 2 ? 1.3F : 1.0F);

    for (size_t i = 0; i < game.run.followerDrones.size(); i++)
    {
        auto& drone = game.run.followerDrones.at(i);
        drone.orbitAngle += deltaTime * 2.0F;
        const float angle =
            drone.orbitAngle + static_cast<float>(i) * 2 * std::numbers::pi_v<float> /
                                   static_cast<float>(game.run.followerDrones.size());
        drone.position =
            Vector2Add(game.run.player.position, Vector2{.x = std::cos(angle) * droneOrbitRadius,
                                                         .y = std::sin(angle) * droneOrbitRadius});

        drone.attackTimer -= deltaTime;
        if (drone.attackTimer > 0)
        {
            continue;
        }
        if (const auto target = nearestEnemyWithin(game, drone.position, droneMeleeRange);
            target.has_value())
        {
            for (size_t j = 0; j < game.run.enemies.size(); j++)
            {
                auto& enemy = game.run.enemies.at(j);
                if (enemy.active && !enemy.phased &&
                    Vector2Distance(enemy.position, *target) <= 0.01F)
                {
                    damageEnemy(game, j, dmg);
                    recordDamage(game, DamageSource::FollowerDrone, dmg);
                    if (!enemy.active)
                    {
                        recordWeaponKill(game, WeaponType::FollowerDrone);
                    }
                    break;
                }
            }
            drone.attackTimer = interval;
        }
    }
}

void updateLaserDrones(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::LaserDrone; });
    if (it == game.run.weapons.end())
    {
        game.run.laserDrones.clear();
        return;
    }

    const Weapon& w = *it;
    const size_t desired = droneCountForLevel(w.level);
    while (game.run.laserDrones.size() < desired)
    {
        game.run.laserDrones.push_back(LaserDrone{.position = game.run.player.position,
                                                  .orbitAngle = 0,
                                                  .attackTimer = 0,
                                                  .beamFlashTimer = 0,
                                                  .beamTarget = Vector2{}});
    }
    game.run.laserDrones.resize(desired);

    const auto dmg = static_cast<int32_t>(shipCurrentDamage(game) * droneDamageFraction(w.level));
    const float interval = droneAttackIntervalBase;
    const float range =
        laserDroneRangeBase + (w.level >= 2 ? 60.0F : 0.0F) + (w.level >= 4 ? 60.0F : 0.0F);

    for (size_t i = 0; i < game.run.laserDrones.size(); i++)
    {
        auto& drone = game.run.laserDrones.at(i);
        drone.orbitAngle -= deltaTime * 1.6F;
        const float angle = drone.orbitAngle + static_cast<float>(i) * 2 *
                                                   std::numbers::pi_v<float> /
                                                   static_cast<float>(game.run.laserDrones.size());
        drone.position =
            Vector2Add(game.run.player.position, Vector2{.x = std::cos(angle) * droneOrbitRadius,
                                                         .y = std::sin(angle) * droneOrbitRadius});

        if (drone.beamFlashTimer > 0)
        {
            drone.beamFlashTimer -= deltaTime;
        }

        drone.attackTimer -= deltaTime;
        if (drone.attackTimer > 0)
        {
            continue;
        }
        if (const auto target = nearestEnemyWithin(game, drone.position, range); target.has_value())
        {
            for (size_t j = 0; j < game.run.enemies.size(); j++)
            {
                auto& enemy = game.run.enemies.at(j);
                if (enemy.active && !enemy.phased &&
                    Vector2Distance(enemy.position, *target) <= 0.01F)
                {
                    damageEnemy(game, j, dmg);
                    recordDamage(game, DamageSource::LaserDrone, dmg);
                    if (!enemy.active)
                    {
                        recordWeaponKill(game, WeaponType::LaserDrone);
                    }
                    break;
                }
            }
            drone.beamTarget = *target;
            drone.beamFlashTimer = 0.15F;
            drone.attackTimer = interval;
        }
    }
}

void updateTurrets(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::TurretDeploy; });
    const int32_t dmg = it != game.run.weapons.end() ? weaponDamage(game, it->level) : 0;

    for (auto& turret : game.run.turrets)
    {
        turret.life -= deltaTime;
        turret.fireTimer -= deltaTime;
        if (turret.fireTimer > 0)
        {
            continue;
        }
        turret.fireTimer = turretFireInterval;
        if (const auto target = nearestEnemy(game, turret.position); target.has_value())
        {
            const Vector2 dir = Vector2Normalize(Vector2Subtract(*target, turret.position));
            game.run.bullets.push_back(Bullet{.position = turret.position,
                                              .velocity = Vector2Scale(dir, projectileSpeed),
                                              .radius = projectileSize,
                                              .color = Palette::Accent,
                                              .active = true,
                                              .damage = dmg,
                                              .ricochetRemaining = ricochetLevel(game),
                                              .source = DamageSource::TurretDeploy});
        }
    }
    std::erase_if(game.run.turrets, [](const Turret& turret) { return turret.life <= 0; });
}

void updateFlamethrower(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::Flamethrower; });
    if (it == game.run.weapons.end())
    {
        return;
    }
    const Weapon& w = *it;

    // L3: the cone(s) periodically switch off for a short window, a power/uptime tradeoff.
    if (w.level >= 3)
    {
        const float cycle = std::fmod(static_cast<float>(GetTime()), 4.0F);
        if (cycle > 3.0F)
        {
            return;
        }
    }

    const Vector2 aim = aimAtMouse(game);
    const float range = flamethrowerRange + (w.level >= 3 ? 40.0F : 0.0F);
    const auto dps = static_cast<float>(weaponDamage(game, w.level)) * 2.0F;

    std::vector<Vector2> coneDirs;
    if (w.level >= 2)
    {
        const float aimAngle = std::atan2(aim.y, aim.x);
        coneDirs.push_back(Vector2{.x = std::cos(aimAngle + flamethrowerOffAngle),
                                   .y = std::sin(aimAngle + flamethrowerOffAngle)});
        coneDirs.push_back(Vector2{.x = std::cos(aimAngle - flamethrowerOffAngle),
                                   .y = std::sin(aimAngle - flamethrowerOffAngle)});
    }
    else
    {
        coneDirs.push_back(aim);
    }

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        auto& enemy = game.run.enemies.at(j);
        if (!enemy.active || enemy.phased)
        {
            continue;
        }
        const Vector2 toEnemy = Vector2Subtract(enemy.position, game.run.player.position);
        const float dist = Vector2Length(toEnemy);
        if (dist > range || dist <= 0.01F)
        {
            continue;
        }
        const Vector2 dirToEnemy = Vector2Scale(toEnemy, 1.0F / dist);
        const bool inCone = std::ranges::any_of(
            coneDirs, [&](const Vector2& dir)
            { return Vector2DotProduct(dir, dirToEnemy) >= std::cos(flamethrowerHalfAngle); });
        if (!inCone)
        {
            continue;
        }
        const auto tick = static_cast<int32_t>(dps * deltaTime);
        if (tick <= 0)
        {
            continue;
        }
        damageEnemy(game, j, tick);
        recordDamage(game, DamageSource::Flamethrower, tick);
        if (!enemy.active)
        {
            recordWeaponKill(game, WeaponType::Flamethrower);
        }
    }
}

void spawnPickup(Game& game, Vector2 position, int value, PickupType type, ElementType element,
                 ElementMechanism mechanism)
{
    const float lifetime = type == PickupType::XP ? xpPickupLifetime : bonusPickupLifetime;
    game.run.pickups.push_back(Pickup{.position = position,
                                      .value = value,
                                      .type = type,
                                      .element = element,
                                      .mechanism = mechanism,
                                      .active = true,
                                      .lifetime = lifetime,
                                      .maxLifetime = lifetime});
}

void spawnRareBonusPickup(Game& game, Vector2 position)
{
    if (GetRandomValue(0, 999) >= rareBonusDropChance)
    {
        return;
    }

    switch (GetRandomValue(0, rareBonusCategoryCount - 1))
    {
    case 0:
        spawnPickup(game, position, 0, PickupType::Regen);
        break;
    case 1:
        spawnPickup(game, position, 0, PickupType::DashTrail);
        break;
    case 2:
        spawnPickup(game, position, 0, PickupType::MagnetPulse);
        break;
    case 3:
        spawnPickup(game, position, 0, PickupType::Overcharge);
        break;
    case 4:
        spawnPickup(game, position, 0, PickupType::SecondWind);
        break;
    default:
    {
        const auto element = static_cast<ElementType>(
            GetRandomValue(0, static_cast<int32_t>(ElementType::Count) - 1));
        const auto mechanism = static_cast<ElementMechanism>(
            GetRandomValue(0, static_cast<int32_t>(ElementMechanism::Count) - 1));
        spawnPickup(game, position, 0, PickupType::Elemental, element, mechanism);
        break;
    }
    }
}

// M23: independent rare roll from spawnRareBonusPickup, so a Danger pickup can appear alongside
// (not instead of) a good bonus pickup from the same kill.
void spawnRareDangerPickup(Game& game, Vector2 position)
{
    if (GetRandomValue(0, 999) >= dangerPickupChance)
    {
        return;
    }
    spawnPickup(game, position, 0, PickupType::Danger);
}

auto currentBurnDps(const Game& game) -> float
{
    return baseBurnDps *
           difficultyDefs.at(static_cast<size_t>(game.resources.settings.difficulty))
               .enemyHealthMult *
           waveEnemyScale(game);
}

void applyElementDebuff(Enemy& enemy, ElementType element, float burnDps)
{
    switch (element)
    {
    case ElementType::Static:
        enemy.debuffStatic = true;
        break;
    case ElementType::Freeze:
        enemy.debuffFreeze = true;
        break;
    case ElementType::Confuse:
        enemy.debuffConfuse = true;
        break;
    case ElementType::Burn:
        if (enemy.burnDps <= 0)
        {
            enemy.burnDps = burnDps;
        }
        break;
    case ElementType::Count:
        break;
    }
}

void applyActiveElementalDebuffs(Game& game, Enemy& enemy)
{
    const float burnDps = currentBurnDps(game);
    for (size_t e = 0; e < static_cast<size_t>(ElementType::Count); e++)
    {
        if (game.run.player.elementalBuffTimer.at(e) > 0)
        {
            applyElementDebuff(enemy, static_cast<ElementType>(e), burnDps);
        }
    }
}

void damageEnemy(Game& game, size_t index, int32_t amount)
{
    const auto kind = enemyKinds.at(static_cast<size_t>(game.run.enemies.at(index).kind));
    const int32_t healthBefore = game.run.enemies.at(index).health;
    game.run.enemies.at(index).health -= amount;
    game.run.enemies.at(index).hitFlashTimer = UpdateConstants::hitFlashDuration;
    applyActiveElementalDebuffs(game, game.run.enemies.at(index));
    // Display the actual HP consumed rather than the raw amount, so an instakill (e.g. amount =
    // 999) shows the enemy's real remaining health instead of a misleadingly huge number.
    spawnDamageNumber(game, game.run.enemies.at(index).position, std::min(amount, healthBefore));

    if (game.run.enemies.at(index).health > 0)
    {
        return;
    }

    game.run.enemies.at(index).active = false;
    int32_t score = kind.score;
    if (game.run.enemies.at(index).isElite)
    {
        score *= 2;
        triggerHitPause(game, UpdateConstants::hitFlashDuration);
        spawnKillExplosion(game, game.run.enemies.at(index).position, kind.color, 14, 1.4F);
    }
    else
    {
        spawnKillExplosion(game, game.run.enemies.at(index).position, kind.color, 8, 1.0F);
    }
    game.run.score += score;
    playSFX(game, game.resources.sounds.explosion);
    gainNerve(game);

    spawnPickup(game, game.run.enemies.at(index).position, score, PickupType::XP);

    const Vector2 bonusPos =
        Vector2Add(game.run.enemies.at(index).position, Vector2{.x = 8, .y = 8});
    const int32_t roll = GetRandomValue(0, 999);
    if (roll < shieldDropChance)
    {
        spawnPickup(game, bonusPos, 0, PickupType::Shield);
    }
    else if (roll < shieldDropChance + lifeOrbDropChance)
    {
        spawnPickup(game, bonusPos, 0, PickupType::LifeOrb);
    }
    spawnRareBonusPickup(game, bonusPos);
    spawnRareDangerPickup(game, bonusPos);

    if (kind.splitsOnDeath)
    {
        for (int i = 0; i < kind.splitCount; i++)
        {
            const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 offset{.x = std::cos(angle) * 10, .y = std::sin(angle) * 10};
            spawnEnemyAt(game, kind.splitKind,
                         Vector2Add(game.run.enemies.at(index).position, offset));
        }
    }

    if (kind.explodesOnDeath &&
        Vector2Distance(game.run.player.position, game.run.enemies.at(index).position) <=
            kind.explodeRadius + game.run.player.radius)
    {
        damagePlayer(game, enemyDamage(game, kind.explodeDamage));
    }
}

// M18: centralizes every boss.health -= site so damage numbers and proportional hit-stop only
// need to live in one place instead of being duplicated at each of the ~10 call sites.
void damageBoss(Game& game, Boss& boss, int32_t amount)
{
    boss.health -= amount;
    boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
    spawnDamageNumber(game, boss.position, amount);
    triggerShake(game, damageShakeIntensity(amount), damageShakeDuration);

    // Hit-stop scales with how big a bite this hit took out of the boss's max health, so a
    // single heavy attack (e.g. a nerve burst or evolved weapon) reads as a "heavy" hit without
    // needing every source of boss damage to hand-tune its own hitstop duration.
    constexpr float heavyHitFraction = 0.05F;
    constexpr float heavyHitPause = 0.1F;
    if (boss.maxHealth > 0 &&
        static_cast<float>(amount) / static_cast<float>(boss.maxHealth) >= heavyHitFraction)
    {
        triggerHitPause(game, heavyHitPause);
    }
}

void killEnemyForBossAttack(Game& game, size_t index, bool alwaysLoot)
{
    if (alwaysLoot || game.resources.settings.difficulty == Difficulty::Easy)
    {
        damageEnemy(game, index, 999);
    }
    else
    {
        game.run.enemies.at(index).active = false;
    }
}

void updateAsteroids(Game& game, float deltaTime)
{
    for (auto& asteroid : game.run.asteroids)
    {
        if (!asteroid.active)
        {
            continue;
        }

        asteroid.position =
            Vector2Add(asteroid.position, Vector2Scale(asteroid.velocity, deltaTime * frameScale));

        if (Vector2Distance(asteroid.position, game.run.player.position) > asteroidDespawnRadius)
        {
            asteroid.active = false;
            continue;
        }

        if (game.run.blackhole.active)
        {
            const Vector2 toHole = Vector2Subtract(game.run.blackhole.position, asteroid.position);
            const float dist = Vector2Length(toHole);

            if (dist <= game.run.blackhole.influenceRadius && dist > 0)
            {
                const Vector2 pull = Vector2Scale(Vector2Normalize(toHole), blackHoleAsteroidPull);
                asteroid.position =
                    Vector2Add(asteroid.position, Vector2Scale(pull, deltaTime * frameScale));
            }

            if (dist <= game.run.blackhole.radius)
            {
                asteroid.active = false;
                breakAsteroid(game.run.asteroids, asteroid);
                continue;
            }
        }

        if (game.run.player.health > 0 && game.run.player.immunityTimer <= 0 &&
            CheckCollisionCircles(game.run.player.position, game.run.player.radius,
                                  asteroid.position, asteroid.radius))
        {
            if (game.run.player.shieldActive)
            {
                game.run.player.shieldActive = false;
                game.run.player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
            }
            else
            {
                damagePlayer(game, enemyDamage(game, 1));
            }

            asteroid.active = false;
            breakAsteroid(game.run.asteroids, asteroid);
        }
    }
}

void updateEnemies(Game& game, float deltaTime)
{

    game.run.enemies.reserve(static_cast<size_t>(UpdateConstants::maxEnemies));

    for (size_t i = 0; i < game.run.enemies.size(); i++)
    {
        auto& enemy = game.run.enemies.at(i);
        if (!enemy.active)
        {
            continue;
        }

        const auto& kind = enemyKinds.at(static_cast<size_t>(enemy.kind));
        float speedMod = enemy.isElite ? 1.2F : 1.0F;

        if (enemy.hitFlashTimer > 0)
        {
            enemy.hitFlashTimer -= deltaTime;
        }

        if (enemy.burnDps > 0)
        {
            enemy.burnDamageAccum += enemy.burnDps * deltaTime;
            if (enemy.burnDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(enemy.burnDamageAccum);
                damageEnemy(game, i, tick);
                recordDamage(game, DamageSource::Elemental, tick);
                enemy.burnDamageAccum -= static_cast<float>(tick);
            }
            if (!enemy.active)
            {
                continue;
            }
        }

        for (const auto& hazard : game.run.eliteHazards)
        {
            if (hazard.active && hazard.role == EliteHazardRole::Warlord)
            {
                speedMod *= warlordSpeedBuff;
                break;
            }
        }

        if (enemy.debuffFreeze)
        {
            speedMod *= freezeSlowMult;
        }

        if (enemy.debuffStatic)
        {
            // No movement, no attacks, no state-machine progress at all.
        }
        else if (enemy.debuffConfuse)
        {
            enemy.confuseWanderTimer -= deltaTime;
            if (enemy.confuseWanderTimer <= 0)
            {
                const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
                enemy.confuseWanderDir = Vector2{.x = std::cos(angle), .y = std::sin(angle)};
                enemy.confuseWanderTimer = confuseWanderInterval;
            }
            if (kind.pattern != EnemyPattern::Stationary)
            {
                enemy.position = Vector2Add(
                    enemy.position, Vector2Scale(enemy.confuseWanderDir,
                                                 kind.speed * speedMod * deltaTime * frameScale));
            }

            if (kind.pattern == EnemyPattern::Turret)
            {
                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0 &&
                    Vector2Distance(game.run.player.position, enemy.position) < turretFireRange)
                {
                    enemy.stateTimer = kind.fireInterval / speedMod;

                    std::vector<Vector2> otherTargets;
                    for (const auto& other : game.run.enemies)
                    {
                        if (other.active && !other.phased && &other != &enemy)
                        {
                            otherTargets.push_back(other.position);
                        }
                    }

                    // Confuse's friendly fire: retarget at another enemy instead of the player,
                    // if one exists; otherwise this fire cycle is simply skipped.
                    if (!otherTargets.empty())
                    {
                        const Vector2 targetPos = otherTargets.at(static_cast<size_t>(
                            GetRandomValue(0, static_cast<int32_t>(otherTargets.size() - 1))));
                        const Vector2 dir =
                            Vector2Normalize(Vector2Subtract(targetPos, enemy.position));
                        const auto turretProjectileHealth = static_cast<int32_t>(std::max(
                            1.0F,
                            static_cast<float>(baseProjectileHealth) *
                                difficultyDefs
                                    .at(static_cast<size_t>(game.resources.settings.difficulty))
                                    .enemyHealthMult *
                                waveEnemyScale(game)));
                        game.run.bossProjectiles.push_back(
                            BossProjectile{.position = enemy.position,
                                           .velocity = Vector2Scale(dir, kind.projectileSpeed),
                                           .radius = 6,
                                           .homing = false,
                                           .active = true,
                                           .fromPlayer = false,
                                           .damage = crossfireProjectileDamage(game),
                                           .health = turretProjectileHealth});
                    }
                }
            }
        }
        else
        {
            switch (kind.pattern)
            {
            case EnemyPattern::Chase:
            {
                const Vector2 dir = Vector2Subtract(game.run.player.position, enemy.position);
                if (Vector2Length(dir) > 0)
                {
                    enemy.position =
                        Vector2Add(enemy.position,
                                   Vector2Scale(Vector2Normalize(dir),
                                                kind.speed * speedMod * deltaTime * frameScale));
                }
                break;
            }
            case EnemyPattern::Zigzag:
            {
                Vector2 dir = Vector2Subtract(game.run.player.position, enemy.position);
                if (Vector2Length(dir) > 0)
                {
                    dir = Vector2Normalize(dir);
                    const Vector2 perp{.x = -dir.y, .y = dir.x};
                    const float wobble =
                        std::sin(static_cast<float>(GetTime()) * 5 + static_cast<float>(i)) * 1.5F;
                    const Vector2 move = Vector2Add(Vector2Scale(dir, kind.speed * speedMod),
                                                    Vector2Scale(perp, wobble));
                    enemy.position =
                        Vector2Add(enemy.position, Vector2Scale(move, deltaTime * frameScale));
                }
                break;
            }
            case EnemyPattern::Charge:
                enemy.stateTimer -= deltaTime;
                if (enemy.telegraphing)
                {

                    if (enemy.stateTimer <= 0)
                    {
                        enemy.telegraphing = false;
                        enemy.charging = true;
                        enemy.stateTimer = enemyChargeDashDuration;
                    }
                }
                else if (!enemy.charging)
                {
                    if (enemy.stateTimer <= 0)
                    {
                        enemy.telegraphing = true;
                        enemy.stateTimer = chargeTelegraphDuration;
                        const Vector2 dir = Vector2Normalize(
                            Vector2Subtract(game.run.player.position, enemy.position));
                        enemy.velocity = Vector2Scale(
                            dir, kind.speed * speedMod * UpdateConstants::enemyChargeDashSpeedMult);
                    }
                }
                else
                {
                    enemy.position = Vector2Add(
                        enemy.position, Vector2Scale(enemy.velocity, deltaTime * frameScale));
                    if (enemy.stateTimer <= 0)
                    {
                        enemy.charging = false;
                        enemy.stateTimer = 1.2F;
                        enemy.velocity = Vector2{};
                    }
                }
                break;
            case EnemyPattern::Orbit:
                enemy.orbitAngle += 1.5F * speedMod * deltaTime;
                if (enemy.orbitDist > kind.radius + 20)
                {

                    enemy.orbitDist -= 9 * speedMod * deltaTime;
                }
                enemy.position =
                    Vector2Add(game.run.player.position,
                               Vector2{.x = std::cos(enemy.orbitAngle) * enemy.orbitDist,
                                       .y = std::sin(enemy.orbitAngle) * enemy.orbitDist});
                break;
            case EnemyPattern::Turret:
            {
                const Vector2 toPlayer = Vector2Subtract(game.run.player.position, enemy.position);
                const float playerDist = Vector2Length(toPlayer);
                constexpr float kiteDistance = turretFireRange * 0.55F;
                if (playerDist > 1.0F)
                {
                    const Vector2 dirToPlayer = Vector2Scale(toPlayer, 1.0F / playerDist);
                    const Vector2 perp{.x = -dirToPlayer.y, .y = dirToPlayer.x};
                    const float strafe =
                        std::sin(static_cast<float>(GetTime()) * 0.8F + static_cast<float>(i)) *
                        0.6F;
                    const float radial = playerDist < kiteDistance
                                             ? -1.0F
                                             : (playerDist > turretFireRange ? 1.0F : 0.0F);
                    Vector2 move =
                        Vector2Add(Vector2Scale(dirToPlayer, radial), Vector2Scale(perp, strafe));
                    if (Vector2Length(move) > 0)
                    {
                        move = Vector2Scale(Vector2Normalize(move), kind.speed * speedMod);
                        enemy.position =
                            Vector2Add(enemy.position, Vector2Scale(move, deltaTime * frameScale));
                    }
                }

                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0 && playerDist < turretFireRange)
                {
                    enemy.stateTimer = kind.fireInterval / speedMod;
                    const Vector2 dir =
                        Vector2Normalize(Vector2Subtract(game.run.player.position, enemy.position));
                    const auto turretProjectileHealth = static_cast<int32_t>(std::max(
                        1.0F, static_cast<float>(baseProjectileHealth) *
                                  difficultyDefs
                                      .at(static_cast<size_t>(game.resources.settings.difficulty))
                                      .enemyHealthMult *
                                  waveEnemyScale(game)));
                    game.run.bossProjectiles.push_back(
                        BossProjectile{.position = enemy.position,
                                       .velocity = Vector2Scale(dir, kind.projectileSpeed),
                                       .radius = 6,
                                       .homing = false,
                                       .active = true,
                                       .fromPlayer = false,
                                       .damage = crossfireProjectileDamage(game),
                                       .health = turretProjectileHealth});
                }
                break;
            }
            case EnemyPattern::Spawner:
                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0)
                {
                    enemy.stateTimer = kind.spawnInterval / speedMod;
                    for (int s = 0; s < kind.spawnCount; s++)
                    {
                        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
                        const Vector2 offset{.x = std::cos(angle) * 30, .y = std::sin(angle) * 30};
                        spawnEnemyAt(game, kind.spawnKind, Vector2Add(enemy.position, offset));
                    }
                }
                break;
            case EnemyPattern::Stationary:

                break;
            }
        }

        if (kind.pattern != EnemyPattern::Stationary)
        {
            applyWormholeTransit(game, enemy.position, enemy.velocity, kind.radius);
        }

        if (kind.phaseCycle)
        {
            enemy.stateTimer -= deltaTime;
            if (enemy.stateTimer <= 0)
            {
                enemy.phased = !enemy.phased;
                enemy.stateTimer = 1.5F;
            }
        }

        if (Vector2Distance(enemy.position, game.run.player.position) > entityDespawnRadius)
        {
            enemy.active = false;
            continue;
        }

        if (enemy.phased)
        {
            continue;
        }

        const bool collides =
            game.run.player.health > 0 &&
            CheckCollisionCircles(game.run.player.position, game.run.player.radius, enemy.position,
                                  kind.radius);

        if (collides && game.run.player.dashing && !enemy.hitByDash)
        {
            enemy.hitByDash = true;
            const DashQuirk quirk = currentShip(game).dashQuirk;

            if (game.run.player.shieldActive)
            {
                damageEnemy(game, i, 999);
                recordDamage(game, DamageSource::Dash, 999);
            }
            else
            {
                if (quirk != DashQuirk::Push)
                {
                    const auto dmg = static_cast<int32_t>(
                        static_cast<float>(dashDamage) *
                        (quirk == DashQuirk::Hybrid ? 0.5F : 1.0F) * currentShip(game).damageMult);
                    damageEnemy(game, i, dmg);
                    recordDamage(game, DamageSource::Dash, dmg);
                }
                if (quirk != DashQuirk::Damage)
                {
                    const Vector2 pushDir = Vector2Normalize(game.run.player.dashVelocity);
                    enemy.position =
                        Vector2Add(enemy.position, Vector2Scale(pushDir, dashPushDistance));
                }
                damagePlayer(game, enemyDamage(game, kind.contactDamage));
            }

            if (!enemy.active)
            {
                game.run.player.chargeRegenTimer =
                    std::max(0.0F, game.run.player.chargeRegenTimer - dashKillChargeRefund);
                recordDashKill(game, enemy.kind);
                recordDashOrNerveKill(game);
            }
            continue;
        }

        if (collides && !game.run.player.dashing && game.run.player.immunityTimer <= 0 &&
            !enemy.debuffStatic)
        {
            if (kind.isLeech)
            {
                game.run.player.slowTimer = 2.0F;
                game.run.player.immunityTimer = 1.0F;
                damageEnemy(game, i, 999);
            }
            else if (game.run.player.shieldActive)
            {
                game.run.player.shieldActive = false;
                game.run.player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
                damageEnemy(game, i, 999);

                if (!enemy.active)
                {
                    game.run.player.chargeRegenTimer =
                        std::max(0.0F, game.run.player.chargeRegenTimer - shieldKillChargeRefund);
                }
            }
            else
            {
                damagePlayer(game, enemyDamage(game, kind.contactDamage));
            }
        }
    }
}

void updateProjectiles(Game& game, float deltaTime)
{
    for (auto& projectile : game.run.bossProjectiles)
    {
        if (!projectile.active)
        {
            continue;
        }

        if (projectile.homing)
        {
            if (projectile.fromPlayer)
            {
                if (projectile.huntingNewTarget)
                {
                    // Seeker Swarm: keep flying straight until an untargeted enemy is close
                    // enough to lock onto (mineSeekRadius reused as a reasonable acquire range).
                    if (const auto target =
                            nearestEnemyWithin(game, projectile.position, mineSeekRadius);
                        target.has_value())
                    {
                        const Vector2 direction =
                            Vector2Normalize(Vector2Subtract(*target, projectile.position));
                        projectile.velocity = Vector2Scale(direction, homingProjSpeed * 1.5F);
                        projectile.huntingNewTarget = false;
                    }
                }
                else if (const auto target = nearestEnemy(game, projectile.position);
                         target.has_value())
                {
                    const Vector2 direction =
                        Vector2Normalize(Vector2Subtract(*target, projectile.position));
                    projectile.velocity = Vector2Scale(direction, homingProjSpeed * 1.5F);
                }
            }
            else
            {
                const Vector2 direction = Vector2Normalize(
                    Vector2Subtract(game.run.player.position, projectile.position));
                projectile.velocity = Vector2Scale(direction, homingProjSpeed);
            }
        }

        projectile.position = Vector2Add(projectile.position,
                                         Vector2Scale(projectile.velocity, deltaTime * frameScale));
        applyWormholeTransit(game, projectile.position, projectile.velocity, projectile.radius);

        if (Vector2Distance(projectile.position, game.run.player.position) > entityDespawnRadius)
        {
            projectile.active = false;
            continue;
        }

        for (auto& asteroid : game.run.asteroids)
        {
            if (!asteroid.active || !CheckCollisionCircles(projectile.position, projectile.radius,
                                                           asteroid.position, asteroid.radius))
            {
                continue;
            }

            asteroid.active = false;
            breakAsteroid(game.run.asteroids, asteroid);

            if (projectile.fromPlayer)
            {
                projectile.active = false;
                break;
            }
        }

        for (size_t j = 0; j < game.run.enemies.size(); j++)
        {
            auto& enemy = game.run.enemies.at(j);
            if (!projectile.active || !enemy.active || enemy.phased ||
                !CheckCollisionCircles(projectile.position, projectile.radius, enemy.position,
                                       enemyKinds.at(static_cast<size_t>(enemy.kind)).radius))
            {
                continue;
            }

            if (projectile.fromPlayer)
            {
                damageEnemy(game, j, projectile.damage);
                recordDamage(game, DamageSource::Homing, projectile.damage);
                if (!enemy.active)
                {
                    recordWeaponKill(game, WeaponType::Homing);
                }
                if (projectile.ricochetRemaining > 0)
                {
                    // M15 Ricochet: retarget to another enemy instead of deactivating.
                    if (const auto next =
                            nearestEnemyExcluding(game, projectile.position, enemy.position);
                        next.has_value())
                    {
                        projectile.velocity = Vector2Scale(
                            Vector2Normalize(Vector2Subtract(*next, projectile.position)),
                            homingProjSpeed * 1.5F);
                        projectile.ricochetRemaining--;
                    }
                    else
                    {
                        projectile.active = false;
                    }
                }
                else
                {
                    projectile.active = false;
                }
                break;
            }

            enemy.active = false;
        }

        if (projectile.fromPlayer)
        {
            for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
            {
                auto& hazard = game.run.eliteHazards.at(j);
                if (!projectile.active || !hazard.active ||
                    !CheckCollisionCircles(projectile.position, projectile.radius, hazard.position,
                                           EliteHazardConstants::radius))
                {
                    continue;
                }

                projectile.active = false;
                damageEliteHazard(game, j, projectile.damage);
                recordDamage(game, DamageSource::Homing, projectile.damage);
                break;
            }
        }

        if (projectile.active && !projectile.fromPlayer && game.run.player.health > 0 &&
            !game.run.player.shieldActive && !game.run.player.dashing &&
            game.run.player.immunityTimer <= 0 &&
            CheckCollisionCircles(game.run.player.position, game.run.player.radius,
                                  projectile.position, projectile.radius))
        {
            projectile.active = false;
            damagePlayer(game, enemyDamage(game, projectile.damage));
        }
    }
}

void filterDeadEntities(Game& game)
{
    std::erase_if(game.run.asteroids, [](const Asteroid& a) { return !a.active; });
    std::erase_if(game.run.bullets, [](const Bullet& b) { return !b.active; });
    std::erase_if(game.run.bossProjectiles, [](const BossProjectile& p) { return !p.active; });
    std::erase_if(game.run.enemies, [](const Enemy& e) { return !e.active; });
    std::erase_if(game.run.eliteHazards, [](const EliteHazard& h) { return !h.active; });
    std::erase_if(game.run.pickups, [](const Pickup& p) { return !p.active; });
    std::erase_if(game.run.mines, [](const Mine& m) { return !m.active; });
}

void updateBgParticles(Game& game)
{
    const auto tileW = static_cast<float>(game.resources.screenWidth);
    const auto tileH = static_cast<float>(game.resources.screenHeight);

    for (auto& p : game.run.bgParticles)
    {
        p.position = Vector2Add(p.position, p.velocity);

        if (p.position.x < 0)
        {
            p.position.x += tileW;
        }
        if (p.position.x > tileW)
        {
            p.position.x -= tileW;
        }
        if (p.position.y < 0)
        {
            p.position.y += tileH;
        }
        if (p.position.y > tileH)
        {
            p.position.y -= tileH;
        }
    }
}

void damageEliteHazard(Game& game, size_t index, int32_t amount)
{
    auto& hazard = game.run.eliteHazards.at(index);
    hazard.health -= amount;
    spawnDamageNumber(game, hazard.position, amount);
    triggerShake(game, damageShakeIntensity(amount), damageShakeDuration);
    if (hazard.health > 0)
    {
        return;
    }

    hazard.active = false;
    game.run.score += eliteHazardScore;
    spawnKillExplosion(game, hazard.position, Palette::Crit, 18, 1.6F);
    playSFX(game, game.resources.sounds.explosion);
    gainNerve(game);

    spawnPickup(game, hazard.position, 0,
                GetRandomValue(0, 1) == 0 ? PickupType::Shield : PickupType::LifeOrb);
    spawnPickup(game, hazard.position, eliteHazardXpBonus, PickupType::XP);
    spawnRareBonusPickup(game, hazard.position);
    spawnRareDangerPickup(game, hazard.position);
}

void spawnEliteHazard(Game& game, EliteHazardRole role)
{

    const float halfExtentX = static_cast<float>(game.resources.screenWidth) / 2;
    const float halfExtentY = static_cast<float>(game.resources.screenHeight) / 2;
    const float orbitDist = std::min(halfExtentX, halfExtentY) * eliteHazardOrbitDistFrac;

    const auto health = static_cast<int32_t>(
        static_cast<float>(eliteHazardBaseHealth) *
        difficultyDefs.at(static_cast<size_t>(game.resources.settings.difficulty)).enemyHealthMult *
        waveEnemyScale(game));

    const auto spawnAngle = static_cast<float>(GetRandomValue(0, 359));
    const float spawnRad = spawnAngle * DEG2RAD;
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position, Vector2{.x = std::cos(spawnRad) * orbitDist,
                                                     .y = std::sin(spawnRad) * orbitDist});

    game.run.eliteHazards.push_back(EliteHazard{.position = spawnPos,
                                                .angle = spawnAngle,
                                                .role = role,
                                                .health = health,
                                                .maxHealth = health,
                                                .active = true});
}

void updateEliteHazards(Game& game, float deltaTime)
{

    const float halfExtentX = static_cast<float>(game.resources.screenWidth) / 2;
    const float halfExtentY = static_cast<float>(game.resources.screenHeight) / 2;
    const float orbitDist = std::min(halfExtentX, halfExtentY) * eliteHazardOrbitDistFrac;

    game.run.eliteHazardSpawnTimer -= deltaTime;

    if (!game.sandbox && game.run.eliteHazardSpawnTimer <= 0)
    {
        if (static_cast<int>(game.run.eliteHazards.size()) < eliteHazardCap(game))
        {
            spawnEliteHazard(game, GetRandomValue(0, 1) == 0 ? EliteHazardRole::Warlord
                                                             : EliteHazardRole::Suppressor);
        }
        game.run.eliteHazardSpawnTimer =
            static_cast<float>(GetRandomValue(static_cast<int32_t>(eliteHazardSpawnIntervalMin),
                                              static_cast<int32_t>(eliteHazardSpawnIntervalMax)));
    }

    for (auto& hazard : game.run.eliteHazards)
    {
        if (!hazard.active)
        {
            continue;
        }

        hazard.angle += eliteHazardOrbitSpin * deltaTime;
        const float rad = hazard.angle * DEG2RAD;
        const Vector2 targetPos =
            Vector2Add(game.run.player.position,
                       Vector2{.x = std::cos(rad) * orbitDist, .y = std::sin(rad) * orbitDist});

        const float lerpT = std::clamp(eliteHazardFollowRate * deltaTime, 0.0F, 1.0F);
        hazard.position = Vector2Lerp(hazard.position, targetPos, lerpT);

        if (game.run.player.health > 0 && game.run.player.immunityTimer <= 0 &&
            CheckCollisionCircles(game.run.player.position, game.run.player.radius, hazard.position,
                                  EliteHazardConstants::radius))
        {
            damagePlayer(game, enemyDamage(game, eliteHazardContactDamage));
        }
    }
}

void spawnBlackHole(Game& game)
{
    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(150, 350));
    game.run.blackhole.position =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
    game.run.blackhole.radius = 25;
    game.run.blackhole.influenceRadius = 140;
    game.run.blackhole.active = true;
    game.run.blackhole.timer = static_cast<float>(GetRandomValue(60, 100)) / 10.0F;
}

void updateBlackHole(Game& game, float deltaTime)
{
    game.run.blackhole.timer -= deltaTime;

    if (!game.sandbox && !game.run.blackhole.active && game.run.blackhole.timer <= 0)
    {
        spawnBlackHole(game);
    }
    else if (game.run.blackhole.active && game.run.blackhole.timer <= 0)
    {
        game.run.blackhole.active = false;
        game.run.blackhole.timer = static_cast<float>(GetRandomValue(50, 90)) / 10.0F;
    }

    if (game.run.blackhole.active)
    {
        const Vector2 toPlayer =
            Vector2Subtract(game.run.player.position, game.run.blackhole.position);
        const float dist = Vector2Length(toPlayer);

        if (dist > game.run.blackhole.influenceRadius)
        {
            const Vector2 drift = Vector2Scale(Vector2Normalize(toPlayer), blackHoleChaseSpeed);
            game.run.blackhole.position = Vector2Add(game.run.blackhole.position,
                                                     Vector2Scale(drift, deltaTime * frameScale));
        }
    }
}

void spawnWormholePair(Game& game)
{
    const float angleA = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto distA = static_cast<float>(GetRandomValue(300, 600));
    const float angleB = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto distB = static_cast<float>(GetRandomValue(300, 600));

    game.run.wormhole.positionA =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angleA) * distA, .y = std::sin(angleA) * distA});
    game.run.wormhole.positionB =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angleB) * distB, .y = std::sin(angleB) * distB});
    game.run.wormhole.facingA = static_cast<WormholeFacing>(GetRandomValue(0, 3));
    game.run.wormhole.facingB = static_cast<WormholeFacing>(GetRandomValue(0, 3));
    game.run.wormhole.radius = wormholeRadius;
    game.run.wormhole.active = true;
    game.run.wormhole.timer = wormholeLifetime;
}

void updateWormhole(Game& game, float deltaTime)
{
    game.run.wormhole.timer -= deltaTime;

    if (!game.sandbox && !game.run.wormhole.active && game.run.wormhole.timer <= 0)
    {
        spawnWormholePair(game);
    }
    else if (game.run.wormhole.active && game.run.wormhole.timer <= 0)
    {
        game.run.wormhole.active = false;
        game.run.wormhole.timer =
            static_cast<float>(GetRandomValue(static_cast<int32_t>(wormholeSpawnCooldownMin),
                                              static_cast<int32_t>(wormholeSpawnCooldownMax)));
    }
}

auto applyWormholeTransit(const Game& game, Vector2& position, Vector2& velocity,
                          float entityRadius) -> bool
{
    if (!game.run.wormhole.active)
    {
        return false;
    }

    const bool atA =
        Vector2Distance(position, game.run.wormhole.positionA) <= game.run.wormhole.radius;
    const bool atB =
        !atA && Vector2Distance(position, game.run.wormhole.positionB) <= game.run.wormhole.radius;
    if (!atA && !atB)
    {
        return false;
    }

    const WormholeFacing entryFacing = atA ? game.run.wormhole.facingA : game.run.wormhole.facingB;
    const WormholeFacing exitFacing = atA ? game.run.wormhole.facingB : game.run.wormhole.facingA;
    const Vector2 exitPoint = atA ? game.run.wormhole.positionB : game.run.wormhole.positionA;

    // Exit angle = entry angle + 180 (a portal reverses the direction of travel through it) +
    // the twist between the two mouths' facings. Deliberately unconditional: no "snap outward"
    // correction, since that would override the entry angle rather than just redirecting it.
    const float deltaDegrees =
        (static_cast<float>(exitFacing) - static_cast<float>(entryFacing)) * 90.0F + 180.0F;
    velocity = Vector2Rotate(velocity, deltaDegrees * DEG2RAD);

    const Vector2 exitDir = wormholeFacingVector(exitFacing);

    position =
        Vector2Add(exitPoint, Vector2Scale(exitDir, game.run.wormhole.radius + entityRadius + 4));

    return true;
}

// Splits a straight instant-hit beam (boss Beam/WormholeBeam attacks) into 1 or 2 segments if it
// passes through an active wormhole mouth, applying the same entry+180+twist redirect as
// applyWormholeTransit. Only ever bends once per cast (no multi-bounce) to keep this bounded.
auto wormholeBentBeamSegments(const Game& game, Vector2 start,
                              Vector2 end) -> std::vector<std::pair<Vector2, Vector2>>
{
    if (!game.run.wormhole.active)
    {
        return {{start, end}};
    }

    const auto closestOnSegment = [&](Vector2 point) -> Vector2
    {
        const Vector2 seg = Vector2Subtract(end, start);
        const float lenSq = Vector2LengthSqr(seg);
        if (lenSq <= 0.0F)
        {
            return start;
        }
        const float t =
            std::clamp(Vector2DotProduct(Vector2Subtract(point, start), seg) / lenSq, 0.0F, 1.0F);
        return Vector2Add(start, Vector2Scale(seg, t));
    };

    const Vector2 closestA = closestOnSegment(game.run.wormhole.positionA);
    const Vector2 closestB = closestOnSegment(game.run.wormhole.positionB);
    const bool hitsA =
        Vector2Distance(closestA, game.run.wormhole.positionA) <= game.run.wormhole.radius;
    const bool hitsB =
        Vector2Distance(closestB, game.run.wormhole.positionB) <= game.run.wormhole.radius;

    if (!hitsA && !hitsB)
    {
        return {{start, end}};
    }

    const bool useA =
        hitsA && (!hitsB || Vector2Distance(start, closestA) <= Vector2Distance(start, closestB));
    const Vector2 entryPoint = useA ? closestA : closestB;
    const WormholeFacing entryFacing = useA ? game.run.wormhole.facingA : game.run.wormhole.facingB;
    const WormholeFacing exitFacing = useA ? game.run.wormhole.facingB : game.run.wormhole.facingA;
    const Vector2 exitCenter = useA ? game.run.wormhole.positionB : game.run.wormhole.positionA;

    const float deltaDegrees =
        (static_cast<float>(exitFacing) - static_cast<float>(entryFacing)) * 90.0F + 180.0F;
    const Vector2 entryDir = Vector2Normalize(Vector2Subtract(end, start));
    const Vector2 exitVelDir = Vector2Rotate(entryDir, deltaDegrees * DEG2RAD);

    const Vector2 exitFacingDir = wormholeFacingVector(exitFacing);
    const Vector2 exitPoint =
        Vector2Add(exitCenter, Vector2Scale(exitFacingDir, game.run.wormhole.radius + 4));

    const float remainingLen =
        std::max(0.0F, Vector2Distance(start, end) - Vector2Distance(start, entryPoint));
    const Vector2 newEnd = Vector2Add(exitPoint, Vector2Scale(exitVelDir, remainingLen));

    return {{start, entryPoint}, {exitPoint, newEnd}};
}

void updateBoss(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
{

    const bool enraged =
        boss.health > 0 &&
        static_cast<float>(boss.health) <= static_cast<float>(boss.maxHealth) * enrageHealthFrac;
    const float rateMult = enraged ? enrageSpeedMult : 1.0F;

    switch (boss.state)
    {
    case BossState::IDLE:
        boss.attackTimer -= deltaTime;

        if (boss.attackTimer <= 0 && boss.health > 0)
        {
            boss.attack = boss.moveset.at(static_cast<size_t>(
                GetRandomValue(0, static_cast<int32_t>(boss.moveset.size()) - 1)));
            boss.state = BossState::WINDING_UP;
            boss.stateTimer = bossWindupDuration(boss.attack) * rateMult;
            boss.beamShieldLatched = false;

            switch (boss.attack)
            {
            case BossAttack::Spread:
                boss.color = Palette::BossSpread;
                boss.spreadWindupShots = 0;
                break;
            case BossAttack::Slam:
                boss.color = Palette::Accent;
                break;
            case BossAttack::Beam:
                boss.color = Palette::BossBeam;
                break;
            case BossAttack::WormholeBeam:
                boss.color = Palette::Shield;
                break;
            case BossAttack::MineDrop:
                boss.color = Palette::AccentDim;
                break;
            case BossAttack::ChargeDash:
                boss.color = Palette::Crit;
                break;
            case BossAttack::SummonAdds:
                boss.color = Palette::Charge;
                break;
            case BossAttack::ShockwaveStomp:
                boss.color = Palette::Haze;
                break;
            case BossAttack::Barrage:
                boss.color = Palette::BossSpread;
                break;
            case BossAttack::GravityWell:
                boss.color = Palette::StructMid;
                break;
            case BossAttack::HomingBarrage:
                boss.color = Palette::BossHoming;
                break;
            case BossAttack::Count:
                break;
            }

            playSFX(game, game.resources.sounds.bossWindUp);
        }
        break;
    case BossState::WINDING_UP:
        boss.stateTimer -= deltaTime;
        boss.targetPosition = game.run.player.position;

        if (boss.stateTimer <= 0)
        {
            startBossAttack(game, boss, bossCenter);
        }
        break;
    case BossState::SHOOTING:
        boss.stateTimer -= deltaTime;

        if (boss.attack == BossAttack::Slam)
        {
            const float progress =
                std::clamp(1 - boss.stateTimer / UpdateConstants::slamDuration, 0.0F, 1.0F);
            const float radius = bossConstants::maxSlamRadius * progress;

            for (auto& asteroid : game.run.asteroids)
            {
                if (asteroid.active &&
                    Vector2Distance(bossCenter, asteroid.position) <= radius + asteroid.radius)
                {
                    asteroid.active = false;
                    breakAsteroid(game.run.asteroids, asteroid);
                }
            }

            for (size_t i = 0; i < game.run.enemies.size(); i++)
            {
                const auto& e = game.run.enemies.at(i);
                if (e.active && !e.phased &&
                    Vector2Distance(bossCenter, e.position) <=
                        radius + enemyKinds.at(static_cast<size_t>(e.kind)).radius)
                {
                    killEnemyForBossAttack(game, i, false);
                }
            }

            if (!boss.slamHit)
            {
                if (const auto result = resolveExpandingWaveHit(game, bossCenter, radius, true);
                    result.resolved)
                {
                    boss.slamHit = true;
                    if (result.hit)
                    {
                        damagePlayer(game, enemyDamage(game, 1));
                    }
                }
            }
        }

        if (boss.attack == BossAttack::Spread)
        {
            boss.barrageTimer -= deltaTime;
            const int targetRounds = spreadRoundsByDifficulty.at(
                static_cast<size_t>(game.resources.settings.difficulty));
            if (boss.barrageTimer <= 0 && boss.spreadWindupShots < targetRounds)
            {
                boss.barrageTimer = spreadRoundInterval;
                boss.spreadWindupShots++;
                playSFX(game, game.resources.sounds.spreadBurst);
                triggerShake(game, 5, 0.2F);

                const Vector2 aimDir =
                    Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));
                const float baseAngle = std::atan2(aimDir.y, aimDir.x);
                const auto roundProjectileHealth = static_cast<int32_t>(std::max(
                    1.0F,
                    static_cast<float>(baseProjectileHealth) *
                        difficultyDefs.at(static_cast<size_t>(game.resources.settings.difficulty))
                            .enemyHealthMult *
                        waveEnemyScale(game)));
                constexpr int32_t half = spreadBulletsPerRound / 2;
                for (int32_t i = -half; i < spreadBulletsPerRound - half; i++)
                {
                    const float angle = baseAngle + static_cast<float>(i) * 15 * DEG2RAD;
                    const Vector2 vel{.x = std::cos(angle) * spreadProjSpeed,
                                      .y = std::sin(angle) * spreadProjSpeed};

                    game.run.bossProjectiles.push_back(
                        BossProjectile{.position = bossCenter,
                                       .velocity = vel,
                                       .radius = 7,
                                       .homing = false,
                                       .active = true,
                                       .fromPlayer = false,
                                       .damage = crossfireProjectileDamage(game),
                                       .health = roundProjectileHealth});
                }
            }
        }

        if (boss.attack == BossAttack::Beam)
        {
            const Vector2 direction =
                Vector2Normalize(Vector2Subtract(boss.targetPosition, bossCenter));
            const Vector2 beamEnd = Vector2Add(bossCenter, Vector2Scale(direction, 2000));
            for (const auto& [segStart, segEnd] :
                 wormholeBentBeamSegments(game, bossCenter, beamEnd))
            {
                processBeamAttack(game, boss, segStart, segEnd);
            }
        }

        if (boss.attack == BossAttack::WormholeBeam)
        {
            const Vector2 direction =
                Vector2Normalize(Vector2Subtract(boss.targetPosition, boss.wormholeBeamOrigin));
            const Vector2 beamEnd =
                Vector2Add(boss.wormholeBeamOrigin, Vector2Scale(direction, 2000));
            for (const auto& [segStart, segEnd] :
                 wormholeBentBeamSegments(game, boss.wormholeBeamOrigin, beamEnd))
            {
                processBeamAttack(game, boss, segStart, segEnd);
            }
        }

        if (boss.attack == BossAttack::ChargeDash)
        {
            boss.position = Vector2Add(boss.position, Vector2Scale(boss.chargeVelocity, deltaTime));
        }

        if (boss.attack == BossAttack::Barrage)
        {
            boss.barrageTimer -= deltaTime;
            if (boss.barrageTimer <= 0)
            {
                boss.barrageTimer = barrageFireInterval;
                const Vector2 direction =
                    Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));

                game.run.bossProjectiles.push_back(
                    BossProjectile{.position = bossCenter,
                                   .velocity = Vector2Scale(direction, barrageProjSpeed),
                                   .radius = 6,
                                   .homing = false,
                                   .active = true,
                                   .fromPlayer = false,
                                   .damage = crossfireProjectileDamage(game),
                                   .health = 1});
                playSFX(game, game.resources.sounds.spreadBurst);
            }
        }

        if (boss.attack == BossAttack::HomingBarrage)
        {
            boss.barrageTimer -= deltaTime;
            if (boss.barrageTimer <= 0)
            {
                boss.barrageTimer = homingBarrageFireInterval;
                const Vector2 direction =
                    Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));
                game.run.bossProjectiles.push_back(
                    BossProjectile{.position = bossCenter,
                                   .velocity = Vector2Scale(direction, homingProjSpeed),
                                   .radius = 6,
                                   .homing = true,
                                   .active = true,
                                   .fromPlayer = false,
                                   .damage = crossfireProjectileDamage(game),
                                   .health = 1});
                playSFX(game, game.resources.sounds.spreadBurst);
            }
        }

        if (boss.attack == BossAttack::GravityWell)
        {
            const Vector2 toBoss = Vector2Subtract(bossCenter, game.run.player.position);
            if (Vector2Length(toBoss) > 0)
            {
                const Vector2 pull =
                    Vector2Scale(Vector2Normalize(toBoss), gravityWellPullStrength);
                game.run.player.position = Vector2Add(game.run.player.position,
                                                      Vector2Scale(pull, deltaTime * frameScale));
            }
        }

        if (boss.stateTimer <= 0)
        {
            boss.state = BossState::IDLE;
            boss.attackTimer = static_cast<float>(GetRandomValue(15, 40)) / 10.0F * rateMult;
            boss.color = boss.baseColor;
        }
        break;
    }
}

auto bossWindupDuration(BossAttack attack) -> float
{
    switch (attack)
    {
    case BossAttack::Slam:
        return 1.3F;
    case BossAttack::WormholeBeam:
        return 1.4F;
    case BossAttack::ChargeDash:
        return 1.2F;
    default:
        return 1.0F;
    }
}

void processBeamAttack(Game& game, Boss& boss, Vector2 beamStart, Vector2 beamEnd)
{
    for (auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active &&
            CheckCollisionCircleLine(asteroid.position, asteroid.radius, beamStart, beamEnd))
        {
            asteroid.active = false;
            breakAsteroid(game.run.asteroids, asteroid);
        }
    }

    for (size_t i = 0; i < game.run.enemies.size(); i++)
    {
        const auto& e = game.run.enemies.at(i);
        if (e.active && !e.phased &&
            CheckCollisionCircleLine(e.position, enemyKinds.at(static_cast<size_t>(e.kind)).radius,
                                     beamStart, beamEnd))
        {
            killEnemyForBossAttack(game, i, false);
        }
    }

    if (game.run.player.health <= 0 || game.run.player.dashing)
    {
        return;
    }

    if (!CheckCollisionCircleLine(game.run.player.position, game.run.player.radius, beamStart,
                                  beamEnd))
    {
        return;
    }

    if (game.run.player.shieldActive)
    {
        boss.beamShieldLatched = true;
        return;
    }

    if (boss.beamShieldLatched)
    {
        boss.beamShieldLatched = false;
        damagePlayer(game, instakillDamage);
        return;
    }

    if (game.run.player.immunityTimer <= 0)
    {
        damagePlayer(game, enemyDamage(game, 1));
    }
}

void forceBossAttack(Game& game, Boss& boss, BossAttack attack)
{
    boss.attack = attack;
    boss.state = BossState::WINDING_UP;
    boss.stateTimer = bossWindupDuration(attack);
    boss.beamShieldLatched = false;

    switch (attack)
    {
    case BossAttack::Spread:
        boss.color = Palette::BossSpread;
        boss.spreadWindupShots = 0;
        break;
    case BossAttack::Slam:
        boss.color = Palette::Accent;
        break;
    case BossAttack::Beam:
        boss.color = Palette::BossBeam;
        break;
    case BossAttack::WormholeBeam:
        boss.color = Palette::Shield;
        break;
    case BossAttack::MineDrop:
        boss.color = Palette::AccentDim;
        break;
    case BossAttack::ChargeDash:
        boss.color = Palette::Crit;
        break;
    case BossAttack::SummonAdds:
        boss.color = Palette::Charge;
        break;
    case BossAttack::ShockwaveStomp:
        boss.color = Palette::Haze;
        break;
    case BossAttack::Barrage:
        boss.color = Palette::BossSpread;
        break;
    case BossAttack::GravityWell:
        boss.color = Palette::StructMid;
        break;
    case BossAttack::HomingBarrage:
        boss.color = Palette::BossHoming;
        break;
    case BossAttack::Count:
        break;
    }

    playSFX(game, game.resources.sounds.bossWindUp);
}

void startBossAttack(Game& game, Boss& boss, Vector2 bossCenter)
{
    const Vector2 direction = Vector2Subtract(boss.targetPosition, bossCenter);
    const Vector2 aimDirection = Vector2Normalize(direction);

    boss.state = BossState::SHOOTING;

    const auto projectileHealth = static_cast<int32_t>(std::max(
        1.0F, static_cast<float>(baseProjectileHealth) *
                  difficultyDefs.at(static_cast<size_t>(game.resources.settings.difficulty))
                      .enemyHealthMult *
                  waveEnemyScale(game)));

    switch (boss.attack)
    {
    case BossAttack::Beam:
        boss.stateTimer = beamAttackDuration;
        playSFX(game, game.resources.sounds.beamFire);
        triggerShake(game, 5, 0.2F);
        break;
    case BossAttack::Spread:
    {
        const int rounds =
            spreadRoundsByDifficulty.at(static_cast<size_t>(game.resources.settings.difficulty));
        boss.stateTimer = static_cast<float>(rounds) * spreadRoundInterval + 0.2F;
        boss.barrageTimer = 0;
        boss.spreadWindupShots = 0;
        break;
    }
    case BossAttack::Slam:
        boss.stateTimer = UpdateConstants::slamDuration;
        boss.slamHit = false;
        playSFX(game, game.resources.sounds.slamBoom);
        duckBGM(game);
        triggerShake(game, 10, 0.4F);
        break;
    case BossAttack::WormholeBeam:
    {
        boss.stateTimer = wormholeBeamDuration;
        boss.beamShieldLatched = false;
        const float flankAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto flankDist =
            static_cast<float>(GetRandomValue(static_cast<int32_t>(wormholeBeamFlankMinDist),
                                              static_cast<int32_t>(wormholeBeamFlankMaxDist)));
        boss.wormholeBeamOrigin =
            Vector2Add(game.run.player.position, Vector2{.x = std::cos(flankAngle) * flankDist,
                                                         .y = std::sin(flankAngle) * flankDist});
        playSFX(game, game.resources.sounds.beamFire);
        triggerShake(game, 6, 0.25F);
        break;
    }
    case BossAttack::MineDrop:
        boss.stateTimer = 0.5F;
        playSFX(game, game.resources.sounds.slamBoom);
        duckBGM(game);
        for (int i = 0; i < mineDropCount; i++)
        {
            const float angle = static_cast<float>(i) * (360.0F / mineDropCount) * DEG2RAD;
            const auto dist = static_cast<float>(GetRandomValue(60, 140));
            const Vector2 pos = Vector2Add(
                bossCenter, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

            game.run.bossProjectiles.push_back(BossProjectile{.position = pos,
                                                              .velocity = Vector2{},
                                                              .radius = mineDropRadius,
                                                              .homing = false,
                                                              .active = true,
                                                              .fromPlayer = false,
                                                              .damage = mineDropDamage,
                                                              .health = projectileHealth});
        }
        break;
    case BossAttack::ChargeDash:
        boss.stateTimer = chargeDashDuration;
        boss.chargeVelocity = Vector2Scale(aimDirection, chargeDashSpeed);
        playSFX(game, game.resources.sounds.bossWindUp);
        triggerShake(game, 8, 0.3F);
        break;
    case BossAttack::SummonAdds:
    {
        boss.stateTimer = 0.6F;
        playSFX(game, game.resources.sounds.spreadBurst);
        const int addsCount =
            summonAddsCountByDifficulty.at(static_cast<size_t>(game.resources.settings.difficulty));
        for (int i = 0; i < addsCount; i++)
        {
            spawnEnemy(game);
        }
        break;
    }
    case BossAttack::ShockwaveStomp:
        boss.stateTimer = UpdateConstants::shockwaveStompDuration;
        playSFX(game, game.resources.sounds.slamBoom);
        duckBGM(game);
        triggerShake(game, 10, 0.35F);
        for (auto& asteroid : game.run.asteroids)
        {
            if (asteroid.active && Vector2Distance(bossCenter, asteroid.position) <=
                                       UpdateConstants::shockwaveStompRadius + asteroid.radius)
            {
                asteroid.active = false;
                breakAsteroid(game.run.asteroids, asteroid);
            }
        }
        for (size_t i = 0; i < game.run.enemies.size(); i++)
        {
            const auto& e = game.run.enemies.at(i);
            if (e.active && !e.phased &&
                Vector2Distance(bossCenter, e.position) <=
                    UpdateConstants::shockwaveStompRadius +
                        enemyKinds.at(static_cast<size_t>(e.kind)).radius)
            {
                killEnemyForBossAttack(game, i, false);
            }
        }
        if (game.run.player.health > 0 && !game.run.player.shieldActive &&
            !game.run.player.dashing &&
            Vector2Distance(bossCenter, game.run.player.position) <=
                UpdateConstants::shockwaveStompRadius + game.run.player.radius)
        {
            damagePlayer(game, enemyDamage(game, 2));
        }
        break;
    case BossAttack::Barrage:
        boss.stateTimer = barrageDuration;
        boss.barrageTimer = 0;
        playSFX(game, game.resources.sounds.spreadBurst);
        break;
    case BossAttack::GravityWell:
        boss.stateTimer = gravityWellDuration;
        playSFX(game, game.resources.sounds.bossWindUp);
        break;
    case BossAttack::HomingBarrage:
        boss.stateTimer = barrageDuration;
        boss.barrageTimer = 0;
        playSFX(game, game.resources.sounds.homingLaunch);
        break;
    case BossAttack::Count:
        break;
    }
}
auto UpdateGame(Game& game, float deltaTime) -> bool
{
    if (IsKeyPressed(KEY_F11))
    {
        toggleFullscreen(game);
    }
    syncScreenSize(game);

    updateBgParticles(game);
    updateDeathParticles(game, deltaTime);
    updateDamageNumbers(game, deltaTime);
    updateWeaponDowngrade(game, deltaTime);
    updateBgmLayers(game, deltaTime);
    UpdateMusicStream(game.resources.bgm.base);
    UpdateMusicStream(game.resources.bgm.intensity);
    UpdateMusicStream(game.resources.bgm.miniboss);
    UpdateMusicStream(game.resources.bgm.megaboss);
    UpdateMusicStream(game.resources.bgm.swarmBoss);

    if (game.run.shakeTimer > 0)
    {
        game.run.shakeTimer -= deltaTime;
        if (game.run.shakeTimer <= 0)
        {
            game.run.shakeTimer = 0;
            game.run.shakeIntensity = 0;
        }
    }

    if (game.run.hitPauseTimer > 0)
    {
        game.run.hitPauseTimer -= deltaTime;
        return false;
    }

    if (game.run.achievementToastTimer > 0)
    {
        game.run.achievementToastTimer -= deltaTime;
    }

    switch (game.state)
    {
    case GameState::TITLE:
        return updateTitle(game);
    case GameState::SHIP_SELECT:
        return updateShipSelect(game);
    case GameState::GAMEPLAY:
        updateGameplay(game, deltaTime);
        break;
    case GameState::PAUSED:
        return updatePaused(game);
    case GameState::LEVEL_UP:
        return updateLevelUp(game);
    case GameState::GAME_OVER:
        return updateGameOver(game);
    case GameState::SETTINGS:
        return updateSettings(game);
    case GameState::ACHIEVEMENTS:
        return updateAchievements(game);
    }

    return false;
}
