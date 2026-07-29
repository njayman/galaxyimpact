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
#include <cmath>
#include <cstddef>
#include <optional>
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

constexpr std::array<float, static_cast<size_t>(WeaponType::Count)> weaponBaseCooldown{
    0.38F, 0.85F, 1.0F, 0.7F, 0.6F, 1.5F};

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
auto weaponCooldown(const Game& game, WeaponType kind, int32_t level, bool evolved) -> float;
auto weaponDamage(const Game& game, int32_t level) -> int32_t;
auto mineCount(int32_t level, bool evolved) -> int;
auto mineRadius(bool evolved) -> float;
auto mineDamage(const Game& game, int32_t level, bool evolved) -> int32_t;
void spawnMines(Game& game, const Weapon& weapon);
void updateMines(Game& game, float deltaTime);
auto nearestEnemyWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>;
auto nearestAsteroidWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>;
void updateWeapons(Game& game, float deltaTime);
void recordDamage(Game& game, DamageSource source, int32_t amount);
void aoePulse(Game& game, Vector2 center, float radius, int32_t dmg, DamageSource source);
void beamPulse(Game& game, Vector2 dir, float length, int32_t dmg, DamageSource source);
void updateWaveSpawner(Game& game, float deltaTime);
auto asteroidIntervalMultiplier(const Game& game) -> float;
auto asteroidCap(const Game& game) -> int;
auto eliteHazardCap(const Game& game) -> int;
void updateEliteHazards(Game& game, float deltaTime);
void damageEliteHazard(Game& game, size_t index, int32_t amount);
auto enemyDamage(const Game& game, int32_t base) -> int32_t;
void spawnEnemy(Game& game);
void spawnBossWave(Game& game, float healthMult, float sizeMult, bool isMega);
auto bossMoveCountForDifficulty(Difficulty difficulty) -> int;
auto sampleBossMoveset(int count) -> std::vector<BossAttack>;
auto spawnRingPosition(const Game& game) -> Vector2;
void updateBossMovement(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter);
void updatePickups(Game& game, float deltaTime);
void collectLifeOrb(Game& game);
auto equippedSlotCount(const Game& game) -> int;
auto sampleDistinct(std::vector<SkillType> ids, int count) -> std::vector<SkillType>;
auto rollLevelUpChoices(Game& game) -> std::vector<LevelUpChoice>;
auto rollRewardChoices() -> std::vector<LevelUpChoice>;
auto weaponEvolved(const Game& game, WeaponType kind) -> bool;
void applySkill(Game& game, SkillType id);
void grantOrLevelWeapon(Game& game, WeaponType kind);
void applyEvolution(Game& game, WeaponType kind);
void updateBullets(Game& game, float deltaTime);
void damageEnemy(Game& game, size_t index, int32_t amount);
void killEnemyForBossAttack(Game& game, size_t index, bool alwaysLoot);
void spawnPickup(Game& game, Vector2 position, int value, PickupType type);
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
        game, game.menuIndex, 3, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
        MenuLayout::buttonGap, MenuLayout::titleMenuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        switch (index)
        {
        case 0:
            game.menuIndex = game.resources.settings.shipIndex;
            game.state = GameState::SHIP_SELECT;
            break;
        case 1:
            game.settingsReturnState = GameState::TITLE;
            game.menuIndex = 0;
            game.state = GameState::SETTINGS;
            break;
        case 2:
            return true;
        default:
            break;
        }
    }

    return false;
}

auto updateShipSelect(Game& game) -> bool
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        game.menuIndex = 0;
        game.state = GameState::TITLE;
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
        game, game.menuIndex, 4, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
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
            game.menuIndex = 0;
            game.state = GameState::SETTINGS;
            break;
        case 2:
            resetRun(game);
            break;
        case 3:
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
            resetRun(game);
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
        case ChoiceType::LifeOrb:
            for (int32_t i = 0; i < choice.count; i++)
            {
                collectLifeOrb(game);
            }
            break;
        case ChoiceType::Shield:
            for (int32_t i = 0; i < choice.count; i++)
            {
                if (game.run.player.shieldStacks < currentShip(game).maxShieldStacks)
                {
                    game.run.player.shieldStacks++;
                }
            }
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
    const bool mouseMoved = mouseDelta.x != 0 || mouseDelta.y != 0;
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

        game.run.bossDeathShockwaves.push_back(
            BossDeathShockwave{.timer = UpdateConstants::bossDeathShockwaveDuration,
                               .position = bossCenter,
                               .hit = false});

        if (game.run.player.health > 0)
        {
            playSFX(game, game.resources.sounds.victory);
            triggerShake(game, 14, 0.6F);
            triggerHitPause(game, 0.2F);
            gainNerve(game);
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
                boss.health -= ramDamage;
                boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
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

auto orbitRadius(int32_t level) -> float { return 55 + static_cast<float>(level) * 4; }

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
    return static_cast<int32_t>(dmg);
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

void updateMines(Game& game, float deltaTime)
{
    for (auto& mine : game.run.mines)
    {
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
            mine.active = false;
            aoePulse(game, mine.position, mine.radius, mine.damage, DamageSource::Mine);
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
                game.run.bullets.push_back(
                    Bullet{.position = game.run.player.position,
                           .velocity = Vector2Scale(rotated, projectileSpeed),
                           .radius = projectileSize,
                           .color = color,
                           .active = true,
                           .damage = dmg});
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
            for (int m = 0; m < missiles; m++)
            {
                if (const auto target = nearestEnemy(game, game.run.player.position);
                    target.has_value())
                {
                    const Vector2 dir =
                        Vector2Normalize(Vector2Subtract(*target, game.run.player.position));
                    game.run.bossProjectiles.push_back(
                        BossProjectile{.position = game.run.player.position,
                                       .velocity = Vector2Scale(dir, homingProjSpeed * 1.5F),
                                       .radius = 6,
                                       .homing = true,
                                       .active = true,
                                       .fromPlayer = true,
                                       .damage = dmg});
                    fired = true;
                }
            }
            if (fired)
            {
                playSFX(game, game.resources.sounds.homingLaunch);
            }
            break;
        }
        case WeaponType::Orbit:
        {
            float radius = orbitRadius(w.level);
            int32_t dmg = weaponDamage(game, w.level) / 2;
            if (w.evolved)
            {
                radius *= 1.4F;
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
            }
            aoePulse(game, game.run.player.position, radius, dmg, DamageSource::Orbit);
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
            aoePulse(game, game.run.player.position, radius, dmg, DamageSource::Shock);
            w.flashTimer = UpdateConstants::shockFlashDuration;
            break;
        }
        case WeaponType::Mine:
            spawnMines(game, w);
            break;
        case WeaponType::Beam:
        {
            const Vector2 dir = aimAtMouse(game);
            float length = 300 + static_cast<float>(w.level) * 15;
            int32_t dmg = weaponDamage(game, w.level);
            if (w.evolved)
            {
                length *= 1.3F;
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
            }
            beamPulse(game, dir, length, dmg, DamageSource::Beam);
            break;
        }
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

void aoePulse(Game& game, Vector2 center, float radius, int32_t dmg, DamageSource source)
{
    bool hitAny = false;

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        const auto& enemy = game.run.enemies.at(j);
        if (enemy.active && !enemy.phased &&
            Vector2Distance(center, enemy.position) <=
                radius + enemyKinds.at(static_cast<size_t>(enemy.kind)).radius)
        {
            damageEnemy(game, j, dmg);
            recordDamage(game, source, dmg);
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

void beamPulse(Game& game, Vector2 dir, float length, int32_t dmg, DamageSource source)
{
    const Vector2 start = game.run.player.position;
    const Vector2 end = Vector2Add(start, Vector2Scale(dir, length));
    bool hitAny = false;

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        const auto& enemy = game.run.enemies.at(j);
        if (enemy.active && !enemy.phased &&
            CheckCollisionCircleLine(
                enemy.position, enemyKinds.at(static_cast<size_t>(enemy.kind)).radius, start, end))
        {
            damageEnemy(game, j, dmg);
            recordDamage(game, source, dmg);
            hitAny = true;
        }
    }

    for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
    {
        const auto& hazard = game.run.eliteHazards.at(j);
        if (hazard.active &&
            CheckCollisionCircleLine(hazard.position, EliteHazardConstants::radius, start, end))
        {
            damageEliteHazard(game, j, dmg);
            recordDamage(game, source, dmg);
            hitAny = true;
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
            hitAny = true;
        }
    }

    if (hitAny)
    {
        playSFX(game, game.resources.sounds.explosion);
    }
}

void fireNerveBurst(Game& game)
{
    const Vector2 dir = aimAtMouse(game);
    const Vector2 start = game.run.player.position;
    const Vector2 end = Vector2Add(start, Vector2Scale(dir, nerveBurstLength));

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        const auto& enemy = game.run.enemies.at(j);
        if (enemy.active && !enemy.phased &&
            CheckCollisionCircleLine(
                enemy.position, enemyKinds.at(static_cast<size_t>(enemy.kind)).radius, start, end))
        {
            damageEnemy(game, j, nerveBurstDamage);
            recordDamage(game, DamageSource::Nerve, nerveBurstDamage);
        }
    }

    for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
    {
        const auto& hazard = game.run.eliteHazards.at(j);
        if (hazard.active &&
            CheckCollisionCircleLine(hazard.position, EliteHazardConstants::radius, start, end))
        {
            damageEliteHazard(game, j, nerveBurstDamage);
            recordDamage(game, DamageSource::Nerve, nerveBurstDamage);
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

    for (auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        if (CheckCollisionCircleLine(bossCenter, boss.size.x / 2, start, end))
        {
            boss.health -= nerveBurstDamage;
            boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
            recordDamage(game, DamageSource::Nerve, nerveBurstDamage);
        }
    }

    game.run.nerveBurstFlashTimer = 0.15F;
    game.run.nerveBurstFlashEnd = end;

    playSFX(game, game.resources.sounds.nerveRelease);
    duckBGM(game);
    triggerShake(game, 10, 0.3F);
    triggerHitPause(game, 0.08F);
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

            if (game.run.waveNumber % megaBossWaveInterval == 0)
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

void spawnBossInstance(Game& game, float healthMult, float sizeMult, bool isMega, bool isSwarm)
{
    const int32_t tier = game.run.waveNumber / megaBossWaveInterval;

    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(1200, 1500));
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

    auto typeIndex =
        static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(bossTypes.size()) - 1));
    if constexpr (DemoConfig::isDemoBuild)
    {
        typeIndex = DemoConfig::allowedBossTypeIndices.at(static_cast<size_t>(GetRandomValue(
            0, static_cast<int32_t>(DemoConfig::allowedBossTypeIndices.size()) - 1)));
    }
    const auto& type = bossTypes.at(typeIndex);

    const auto health = static_cast<int32_t>(
        static_cast<float>(500 + tier * 250) * healthMult * type.healthMult *
        difficultyDefs.at(static_cast<size_t>(game.resources.settings.difficulty)).enemyHealthMult);
    const float size = 100.0F * sizeMult * type.sizeMult;

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
             .beamRotation = 0,
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
             .shape = type.shape});
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

            switch (pickup.type)
            {
            case PickupType::XP:
                game.run.xp += pickup.value;
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
            default:
                break;
            }
        }
    }
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

auto demoSkillAllowed(SkillType id) -> bool
{
    for (size_t i = 0; i < weaponGrantSkill.size(); i++)
    {
        if (weaponGrantSkill.at(i) == id || skillLinkedPassive.at(i) == id)
        {
            return DemoConfig::isWeaponAllowed(static_cast<WeaponType>(i));
        }
    }
    return true;
}

auto rollLevelUpChoices(Game& game) -> std::vector<LevelUpChoice>
{
    const bool slotsFull = equippedSlotCount(game) >= ItemConstants::maxAbilitySlots;

    std::vector<SkillType> eligible;
    eligible.reserve(static_cast<size_t>(SkillType::Count));
    for (size_t i = 0; i < static_cast<size_t>(SkillType::Count); i++)
    {
        const auto id = static_cast<SkillType>(i);
        if (isFusedPassive(game, id) || game.run.skillLevels.at(i) >= Skills.at(i).maxLevel ||
            !demoSkillAllowed(id))
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
        choices = rollRewardChoices();

        game.run.postCapDamageLevels++;
    }
    else
    {
        for (const auto id : sampleDistinct(eligible, 3))
        {
            choices.push_back(LevelUpChoice{.type = ChoiceType::Skill, .skill = id});
        }
    }

    std::vector<WeaponType> evolvable;
    for (size_t i = 0; i < weaponGrantSkill.size(); i++)
    {
        const auto kind = static_cast<WeaponType>(i);
        const auto grantSkill = weaponGrantSkill.at(i);
        const auto passive = skillLinkedPassive.at(i);
        if (hasWeapon(game, kind) && !weaponEvolved(game, kind) &&
            game.run.skillLevels.at(static_cast<size_t>(grantSkill)) >= 3 &&
            game.run.skillLevels.at(static_cast<size_t>(passive)) >= 3)
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

auto rollRewardChoices() -> std::vector<LevelUpChoice>
{
    std::vector<LevelUpChoice> pool{
        LevelUpChoice{.type = ChoiceType::LifeOrb, .count = 1},
        LevelUpChoice{.type = ChoiceType::LifeOrb, .count = 2},
        LevelUpChoice{.type = ChoiceType::LifeOrb, .count = 3},
        LevelUpChoice{.type = ChoiceType::Shield, .count = 1},
        LevelUpChoice{.type = ChoiceType::Shield, .count = 2},
        LevelUpChoice{.type = ChoiceType::Shield, .count = 3},
    };

    const auto n = static_cast<int32_t>(pool.size());
    for (int32_t i = 0; i < 3; i++)
    {
        const int32_t j = i + GetRandomValue(0, n - i - 1);
        std::swap(pool.at(static_cast<size_t>(i)), pool.at(static_cast<size_t>(j)));
    }
    pool.resize(3);
    return pool;
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
                boss.health -= bullet.damage;
                boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
                recordDamage(game, DamageSource::Forward, bullet.damage);
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
                recordDamage(game, DamageSource::Forward, bullet.damage);
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
                bullet.active = false;
                damageEnemy(game, j, bullet.damage);
                recordDamage(game, DamageSource::Forward, bullet.damage);
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

void spawnPickup(Game& game, Vector2 position, int value, PickupType type)
{
    const float lifetime = type == PickupType::XP ? xpPickupLifetime : bonusPickupLifetime;
    game.run.pickups.push_back(Pickup{.position = position,
                                      .value = value,
                                      .type = type,
                                      .active = true,
                                      .lifetime = lifetime,
                                      .maxLifetime = lifetime});
}

void damageEnemy(Game& game, size_t index, int32_t amount)
{
    const auto kind = enemyKinds.at(static_cast<size_t>(game.run.enemies.at(index).kind));
    game.run.enemies.at(index).health -= amount;
    game.run.enemies.at(index).hitFlashTimer = UpdateConstants::hitFlashDuration;

    if (game.run.enemies.at(index).health > 0)
    {
        return;
    }

    game.run.enemies.at(index).active = false;
    int32_t score = kind.score;
    if (game.run.enemies.at(index).isElite)
    {
        score *= 2;
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

        for (const auto& hazard : game.run.eliteHazards)
        {
            if (hazard.active && hazard.role == EliteHazardRole::Warlord)
            {
                speedMod *= warlordSpeedBuff;
                break;
            }
        }

        switch (kind.pattern)
        {
        case EnemyPattern::Chase:
        {
            const Vector2 dir = Vector2Subtract(game.run.player.position, enemy.position);
            if (Vector2Length(dir) > 0)
            {
                enemy.position = Vector2Add(
                    enemy.position, Vector2Scale(Vector2Normalize(dir),
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
                    const Vector2 dir =
                        Vector2Normalize(Vector2Subtract(game.run.player.position, enemy.position));
                    enemy.velocity = Vector2Scale(
                        dir, kind.speed * speedMod * UpdateConstants::enemyChargeDashSpeedMult);
                }
            }
            else
            {
                enemy.position = Vector2Add(enemy.position,
                                            Vector2Scale(enemy.velocity, deltaTime * frameScale));
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
            enemy.position = Vector2Add(game.run.player.position,
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
                    std::sin(static_cast<float>(GetTime()) * 0.8F + static_cast<float>(i)) * 0.6F;
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
                    1.0F,
                    static_cast<float>(baseProjectileHealth) *
                        difficultyDefs.at(static_cast<size_t>(game.resources.settings.difficulty))
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
            }
            continue;
        }

        if (collides && !game.run.player.dashing && game.run.player.immunityTimer <= 0)
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

                if (const auto target = nearestEnemy(game, projectile.position); target.has_value())
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
                projectile.active = false;
                damageEnemy(game, j, projectile.damage);
                recordDamage(game, DamageSource::Homing, projectile.damage);
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
    if (hazard.health > 0)
    {
        return;
    }

    hazard.active = false;
    game.run.score += eliteHazardScore;
    playSFX(game, game.resources.sounds.explosion);
    gainNerve(game);

    spawnPickup(game, hazard.position, 0,
                GetRandomValue(0, 1) == 0 ? PickupType::Shield : PickupType::LifeOrb);
    spawnPickup(game, hazard.position, eliteHazardXpBonus, PickupType::XP);
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

    const float deltaDegrees =
        (static_cast<float>(exitFacing) - static_cast<float>(entryFacing)) * 90.0F;
    velocity = Vector2Rotate(velocity, deltaDegrees * DEG2RAD);

    const Vector2 exitDir = wormholeFacingVector(exitFacing);

    if (const float outward = Vector2DotProduct(velocity, exitDir); outward < 0)
    {
        velocity = Vector2Subtract(velocity, Vector2Scale(exitDir, 2 * outward));
    }

    position =
        Vector2Add(exitPoint, Vector2Scale(exitDir, game.run.wormhole.radius + entityRadius + 4));

    return true;
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
                game.run.spreadWindupShots = 0;
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
            if (boss.barrageTimer <= 0 && game.run.spreadWindupShots < targetRounds)
            {
                boss.barrageTimer = spreadRoundInterval;
                game.run.spreadWindupShots++;
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
            processBeamAttack(game, boss, bossCenter, beamEnd);
        }

        if (boss.attack == BossAttack::WormholeBeam)
        {
            const Vector2 direction =
                Vector2Normalize(Vector2Subtract(boss.targetPosition, boss.wormholeBeamOrigin));
            const Vector2 beamEnd =
                Vector2Add(boss.wormholeBeamOrigin, Vector2Scale(direction, 2000));
            processBeamAttack(game, boss, boss.wormholeBeamOrigin, beamEnd);
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
        game.run.spreadWindupShots = 0;
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
    }

    playSFX(game, game.resources.sounds.bossWindUp);
}

void startBossAttack(Game& game, Boss& boss, Vector2 bossCenter)
{
    const Vector2 direction = Vector2Subtract(boss.targetPosition, bossCenter);
    boss.beamRotation = std::atan2(direction.y, direction.x) * RAD2DEG;
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
        game.run.spreadWindupShots = 0;
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
    }

    return false;
}
