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
#include "update_constants.hpp"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <string>
#include <vector>
auto isConsumablePickup(PickupType type) -> bool { return type == PickupType::SecondWind; }

auto consumablePickupBiasWeight(int32_t wave) -> float
{
    constexpr float maxBias = 4.0F;
    constexpr float waveDivisor = 25.0F;
    return std::min(maxBias, 1.0F + static_cast<float>(wave) / waveDivisor);
}

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
            isConsumablePickup(pickupCatalog.at(i).type) ? consumablePickupBiasWeight(wave) : 1.0F;
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

auto damageShakeIntensity(int32_t amount) -> float
{
    constexpr float perDamageUnit = 0.4F;
    constexpr float maxIntensity = 9.0F;
    return std::min(maxIntensity, static_cast<float>(amount) * perDamageUnit);
}
void triggerShake(Game& /*game*/, float /*intensity*/, float /*duration*/)
{
    // ponytail: screen shake is disabled everywhere except triggerBossShieldDashShake below,
    // per explicit request that shake should only fire on a shield-dash into a boss.
}

void triggerBossShieldDashShake(Game& game)
{
    if (game.run.shieldDashShakeCooldownTimer > 0)
    {
        return;
    }
    game.run.shieldDashShakeCooldownTimer = shieldDashBossShakeCooldown;

    game.run.shakeIntensity = 14.0F;
    game.run.shakeTimer = 0.4F;
    game.run.shakeDuration = 0.4F;
    game.run.hitPauseTimer = 0.12F;
    playSFX(game, game.resources.sounds.critical);
}

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
            if (downgrade.unevolved)
            {
                weapon.evolved = true;
            }
            else if (downgrade.disabled)
            {
                weapon.disabled = false;
            }
            else
            {
                weapon.level += 1;
            }
            game.run.achievementToast =
                std::string(effectiveWeaponName(weapon)) + " restored!";
            game.run.achievementToastTimer = 3.0F;
            break;
        }
    }
    game.run.weaponDowngrade = std::nullopt;
}

void triggerHitPause(Game& /*game*/, float /*duration*/)
{
    // ponytail: see triggerShake above — hit-pause is likewise reserved for
    // triggerBossShieldDashShake only.
}

void duckBGM(Game& game) { game.resources.bgm.duckTimer = bgmDuckDuration; }

auto updateTitle(Game& game) -> bool
{
    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, 5, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
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
            game.state = GameState::BESTIARY;
            break;
        case 3:
            game.settingsReturnState = GameState::TITLE;
            game.menuIndex = 0;
            game.state = GameState::SETTINGS;
            break;
        case 4:
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

auto updateBestiary(Game& game) -> bool
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
        game, game.menuIndex, 6, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
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
            game.state = GameState::BESTIARY;
            break;
        case 3:
            game.settingsReturnState = GameState::PAUSED;
            game.menuIndex = 0;
            game.state = GameState::SETTINGS;
            break;
        case 4:
            game.menuIndex = game.resources.settings.shipIndex;
            game.settingsReturnState = GameState::PAUSED;
            game.state = GameState::SHIP_SELECT;
            break;
        case 5:
            return true;
        default:
            break;
        }
    }

    return false;
}

auto updateEnding(Game& game, float deltaTime) -> bool
{
    game.run.player.position.x -= endingDriftSpeed * deltaTime;
    game.run.player.dashing = false;
    game.run.player.shieldActive = false;
    game.run.player.nerveCharging = false;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        game.state = GameState::TITLE;
        game.menuIndex = 0;
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
    if (!game.resources.settings.soundOn)
    {
        return;
    }

    // ponytail: dedupe by buffer pointer identity, small fixed table, linear scan is fine at this size
    struct SfxThrottleEntry
    {
        void* handle = nullptr;
        double lastPlayTime = -1000.0;
    };
    static std::array<SfxThrottleEntry, 48> throttleTable{};

    const double now = GetTime();
    for (auto& entry : throttleTable)
    {
        if (entry.handle == sound.stream.buffer)
        {
            if (now - entry.lastPlayTime < sfxThrottleInterval)
            {
                return;
            }
            entry.lastPlayTime = now;
            PlaySound(sound);
            return;
        }
        if (entry.handle == nullptr)
        {
            entry.handle = sound.stream.buffer;
            entry.lastPlayTime = now;
            PlaySound(sound);
            return;
        }
    }
    PlaySound(sound);
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
        if (left || right || confirm)
        {
            game.resources.settings.bgmOn = !game.resources.settings.bgmOn;
            applyBGMState(game);
        }
        break;
    case 2:
        if (left || right || confirm)
        {
            game.resources.settings.soundOn = !game.resources.settings.soundOn;
        }
        break;
    case 3:
        if (left)
        {
            cycleFPS(game, -1);
        }
        if (right || confirm)
        {
            cycleFPS(game, 1);
        }
        break;
    case 4:
        if (left)
        {
            cycleHudScale(game, -1);
        }
        if (right || confirm)
        {
            cycleHudScale(game, 1);
        }
        break;
    case 5:
        if (left || right || confirm)
        {
            toggleFullscreen(game);
        }
        break;
    case 6:
        if (confirm)
        {
            game.state = game.settingsReturnState;
            game.menuIndex = 0;
        }
        break;
    default:
        break;
    }

    if ((left || right || confirm) && game.menuIndex <= 4)
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

auto updateBossCutscene(Game& game, float deltaTime) -> bool
{
    if (game.run.bossCutscenePhase == 0)
    {
        return false;
    }

    game.run.bossCutsceneTimer -= deltaTime;

    if (game.run.bossCutscenePhase == 1)
    {
        if (game.run.bossCutsceneTimer <= 0)
        {
            game.run.bossCutscenePhase = 2;
            game.run.bossCutsceneTimer = krakenFleeDuration;
            game.run.punctumTrapActive = true;
        }
        return true;
    }

    if (game.run.bossCutscenePhase == 2)
    {
        const Vector2 toCenter = Vector2Subtract(Vector2{}, game.run.krakenFleePos);
        const float dist = Vector2Length(toCenter);
        if (dist > 4.0F)
        {
            game.run.krakenFleePos = Vector2Add(
                game.run.krakenFleePos,
                Vector2Scale(Vector2Normalize(toCenter), krakenFleeSpeed * deltaTime));
        }
        if (game.run.bossCutsceneTimer <= 0 || dist <= 4.0F)
        {
            game.run.bossCutscenePhase = 3;
            game.run.bossCutsceneTimer = krakenDialogueDuration;
            game.run.blackhole.position = Vector2{};
            game.run.blackhole.radius = 60;
            game.run.blackhole.influenceRadius = 260;
            game.run.blackhole.active = true;
            game.run.achievementToast = "Follow, if you dare.";
            game.run.achievementToastTimer = krakenDialogueDuration;
            game.run.enemies.clear();
            game.run.eliteHazards.clear();
            game.run.asteroids.clear();
            game.run.bossProjectiles.clear();
        }
        return true;
    }

    if (game.run.bossCutscenePhase == 3 && game.run.bossCutsceneTimer <= 0)
    {
        game.run.bossCutscenePhase = 0;
        game.run.player.position = Vector2{.x = 400, .y = 0};
    }
    return true;
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

    if (game.sandboxKillBoxArmed)
    {
        game.sandboxKillBoxTimer -= deltaTime;
        if (game.sandboxKillBoxTimer <= 0)
        {
            game.sandboxKillBoxArmed = false;
            for (auto& boss : game.run.bosses)
            {
                if (!boss.isBeltbreakerPlate && !boss.isWreckwormSegment)
                {
                    boss.health = 0;
                }
            }
        }
    }

    const bool cutsceneLocked = updateBossCutscene(game, deltaTime);

    updateNerve(game, deltaTime);
    if (!cutsceneLocked)
    {
        updateAbilityCharges(game, deltaTime);
        updatePlayerMovement(game, deltaTime);
    }
    updateShieldAndBarrier(game, deltaTime);
    if (!cutsceneLocked)
    {
        updateWeapons(game, deltaTime);
    }
    updateOrbitBladeContact(game, deltaTime);
    updateOrbitBladeLaunch(game, deltaTime);
    updateOrbitBladeProjectiles(game, deltaTime);
    updateNerveBallProjectiles(game, deltaTime);
    updateNerveSpiralProjectiles(game, deltaTime);
    updateBeamContact(game, deltaTime);
    updateFollowerDrones(game, deltaTime);
    updateLaserDrones(game, deltaTime);
    updateDroneContact(game, deltaTime);
    updateTurrets(game, deltaTime);
    updateFlamethrower(game, deltaTime);
    updateSolarForgeEnemyFireParticles(game, deltaTime);
    updateChainLightningBolts(game, deltaTime);
    updateSniperShots(game, deltaTime);
    updatePlayerBuffs(game, deltaTime);
    updateWaveSpawner(game, deltaTime);
    updateBlackHole(game, deltaTime);
    updateBlackHoleDust(game, deltaTime);
    updateWormhole(game, deltaTime);
    updateEliteHazards(game, deltaTime);

    if (game.run.player.health > 0 && game.run.blackhole.active &&
        Vector2Distance(game.run.player.position, game.run.blackhole.position) <=
            game.run.blackhole.radius)
    {
        if (game.run.punctumTrapActive && !game.run.inBanishedRealm)
        {
            game.run.inBanishedRealm = true;
            game.run.punctumTrapActive = false;
            game.run.blackhole.active = false;
            game.run.enemies.clear();
            game.run.eliteHazards.clear();
            game.run.asteroids.clear();
            game.run.bossProjectiles.clear();
            game.run.gasHazards.clear();
            game.run.mines.clear();
            game.run.bgParticles.clear();
            game.run.stars.clear();
            game.run.player.position = Vector2{};
            game.run.achievementToast = "It was never a ship.";
            game.run.achievementToastTimer = 4.0F;
            spawnBanished(game);
        }
        else
        {
            game.run.player.blackHoleCoreTimer += deltaTime;
            if (game.run.player.blackHoleCoreTimer >= 1.0F)
            {
                game.run.player.blackHoleCoreTimer = 0;
                damagePlayer(game, instakillDamage);
            }
        }
    }
    else
    {
        game.run.player.blackHoleCoreTimer = 0;
    }

    for (auto& boss : game.run.bosses)
    {
        if (boss.deathAnimTimer >= 0)
        {
            continue;
        }

        if (boss.isBanished)
        {
            updateBanished(game, boss, deltaTime);
            continue;
        }

        if (boss.health > 0 && boss.burnDps > 0)
        {
            boss.burnDamageAccum += boss.burnDps * deltaTime;
            if (boss.burnDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(boss.burnDamageAccum);
                damageBoss(game, boss, tick, false, true);
                recordDamage(game, DamageSource::Elemental, tick);
                boss.burnDamageAccum -= static_cast<float>(tick);
            }
        }

        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};

        if (boss.parryStunTimer > 0)
        {
            boss.parryStunTimer -= deltaTime;
        }
        else
        {
            updateBossMovement(game, deltaTime, boss, bossCenter);
            updateBoss(game, deltaTime, boss, bossCenter);
        }
        updateBeltbreakerCore(game, boss, deltaTime);
        updateWreckwormChain(game, boss, deltaTime);
        updateWreckwormSegmentVolley(game, boss, deltaTime);
        updateKrakenTentacle(game, boss, deltaTime);
        updateKrakenLimbs(game, boss, deltaTime);
        updateKrakenSummon(game, boss, deltaTime);

        if (boss.isWreckwormHead && deltaTime > 0)
        {
            boss.wreckwormVelocity =
                Vector2Scale(Vector2Subtract(boss.position, boss.wreckwormPrevPosition), 1.0F / deltaTime);
            boss.wreckwormPrevPosition = boss.position;
        }
    }

    updateBullets(game, deltaTime);
    updateAsteroids(game, deltaTime);
    updateSolarForgeHeat(game, deltaTime);
    updatePunctumThunder(game, deltaTime);
    updateFluidHazardContact(game);
    updateGasHazards(game, deltaTime);
    updateOrganicMerges(game, deltaTime);
    updateEnemies(game, deltaTime);
    updateProjectiles(game, deltaTime);
    updateMines(game, deltaTime);
    updatePickups(game, deltaTime);

    filterDeadEntities(game);

    bool anyBossKilledThisFrame = false;
    std::vector<Boss> pendingSlagmawHalves;
    for (auto& boss : game.run.bosses)
    {
        if (boss.health > 0)
        {
            continue;
        }

        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};

        if (boss.isBeltbreakerPlate || boss.isWreckwormSegment)
        {
            boss.health = 0;
            game.run.score += 100;
            spawnKillExplosion(game, bossCenter, boss.baseColor, 14, 1.2F);
            playSFX(game, game.resources.sounds.explosion);
            continue;
        }

        boss.health = 0;

        if (boss.deathAnimTimer < 0)
        {
            boss.deathAnimTimer = bossDeathAnimDuration;
            boss.color = Palette::StructDark;

            if (boss.isWreckwormHead)
            {
                for (auto& other : game.run.bosses)
                {
                    if (other.isWreckwormSegment && other.segmentOwnerId == boss.instanceId &&
                        other.health > 0)
                    {
                        other.health = 0;
                    }
                }
            }
        }

        boss.deathAnimTimer -= deltaTime;
        if (boss.deathAnimTimer > 0)
        {
            continue;
        }

        game.run.score += 1000;
        anyBossKilledThisFrame = true;
        recordBossKilled(game, boss);

        if (!boss.slagmawBreaksIntoDrifters && !boss.slagmawBreaksIntoHalves)
        {
            game.run.xp += static_cast<int>(static_cast<float>(game.run.xpToNext) *
                                            (boss.isMega ? megaBossXpMult : miniBossXpMult));
        }
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

        if (boss.isFinalBoss && !boss.isKraken && !game.resources.achievements.infiniteModeUnlocked)
        {
            game.resources.achievements.infiniteModeUnlocked = true;
            saveAchievements(game.resources.achievements);
            game.run.achievementToast = "Infinite mode unlocked!";
            game.run.achievementToastTimer = 4.0F;
        }

        if (boss.isFinalBoss && !game.run.punctumTrapActive && game.run.bossCutscenePhase == 0)
        {
            game.run.bossCutscenePhase = 1;
            game.run.bossCutsceneTimer = krakenBreakDuration;
            game.run.krakenFleePos = bossCenter;
        }

        if (boss.slagmawBreaksIntoDrifters)
        {
            spawnSlagmawDrifterBreak(game, bossCenter);
        }
        else if (boss.slagmawBreaksIntoHalves)
        {
            spawnSlagmawHalfBreak(game, boss, bossCenter, pendingSlagmawHalves);
        }
    }
    for (auto& half : pendingSlagmawHalves)
    {
        game.run.bosses.push_back(std::move(half));
    }
    if (anyBossKilledThisFrame)
    {
        game.resources.bgm.calmTimer = bossKillCalmDuration;
    }
    std::erase_if(game.run.bosses,
                  [](const Boss& boss) { return boss.health <= 0 && boss.deathAnimTimer <= 0; });

    if (anyBossKilledThisFrame && game.run.bosses.empty())
    {
        game.run.waveTimer = 0;
    }

    updateBossDeathShockwave(game, deltaTime);

    if (game.run.player.health > 0 && game.run.xp >= game.run.xpToNext)
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

    for (const auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        const float d = Vector2Distance(from, bossCenter);
        if (best < 0 || d < best)
        {
            best = d;
            target = bossCenter;
            found = true;
        }
    }

    if (!found)
    {
        return std::nullopt;
    }
    return target;
}

auto farthestEnemy(const Game& game, Vector2 from) -> std::optional<Vector2>
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
        if (d > best)
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
        if (d > best)
        {
            best = d;
            target = hazard.position;
            found = true;
        }
    }

    for (const auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        const float d = Vector2Distance(from, bossCenter);
        if (d > best)
        {
            best = d;
            target = bossCenter;
            found = true;
        }
    }

    if (!found)
    {
        return std::nullopt;
    }
    return target;
}

auto nearestEnemyExcluding(const Game& game, Vector2 from,
                           Vector2 exclude) -> std::optional<Vector2>
{
    constexpr float excludeEpsilon = 0.5F;
    float best = -1;
    Vector2 target{};
    bool found = false;

    for (const auto& enemy : game.run.enemies)
    {
        if (!enemy.active || enemy.phased ||
            Vector2Distance(enemy.position, exclude) <= excludeEpsilon)
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

    for (const auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        if (Vector2Distance(bossCenter, exclude) <= excludeEpsilon)
        {
            continue;
        }
        const float d = Vector2Distance(from, bossCenter);
        if (best < 0 || d < best)
        {
            best = d;
            target = bossCenter;
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

auto weaponCooldown(const Game& game, WeaponType kind, int32_t level) -> float
{
    float base = weaponBaseCooldown.at(static_cast<size_t>(kind));
    base -= static_cast<float>(level) * 0.02F;
    if (base < 0.12F)
    {
        base = 0.12F;
    }
    if (kind == WeaponType::Forward)
    {
        base /= currentShip(game).forwardFireRateMult;
    }
    base *= 1 - 0.1F * static_cast<float>(
                           game.run.skillLevels.at(static_cast<size_t>(SkillType::Cooldown)));

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
    dmg *= playerConstant(game);
    dmg *= currentShip(game).damageMult;
    if (game.run.player.overchargeTimer > 0)
    {
        dmg *= overchargeDamageMult;
    }
    return static_cast<int32_t>(dmg);
}

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

auto estimatePlayerDps(const Game& game) -> float
{
    float dps = 0.0F;
    for (const auto& w : game.run.weapons)
    {
        if (w.disabled)
        {
            continue;
        }
        const float dmg = static_cast<float>(weaponDamage(game, w.level));
        const float cooldown = std::max(weaponCooldown(game, w.type, w.level), 0.05F);

        float hitsPerCooldown = 1.0F;
        switch (w.type)
        {
        case WeaponType::Forward:
            hitsPerCooldown = 1.0F + static_cast<float>(ricochetLevel(game)) + (w.evolved ? 2.0F : 0.0F);
            break;
        case WeaponType::Orbit:
            hitsPerCooldown = static_cast<float>(orbitBladeCount(w.level)) * continuousDpsMultiplier;
            break;
        case WeaponType::FlakCannon:
        case WeaponType::ChainLightning:
            hitsPerCooldown = 3.0F;
            break;
        default:
            break;
        }
        if (w.evolved)
        {
            hitsPerCooldown *= 1.3F;
        }
        dps += dmg * hitsPerCooldown / cooldown;
    }
    return dps;
}

auto dpsHealthScale(const Game& game) -> float
{
    const float dps = estimatePlayerDps(game);
    const float ratio = dps / dpsHealthScaleReference;
    return std::clamp(std::sqrt(std::max(ratio, 1.0F)), 1.0F, dpsHealthScaleCap);
}

auto enemyHealthScale(const Game& game) -> float
{
    return waveEnemyScale(game) * dpsHealthScale(game);
}

auto followerDroneDamageFraction(int32_t level) -> float
{
    constexpr std::array<float, 4> fractionByLevel{0.4F, 0.6F, 0.6F, 0.75F};
    return fractionByLevel.at(static_cast<size_t>(std::clamp(level, 1, 4) - 1));
}

auto laserDroneDamageFraction(int32_t level) -> float
{
    constexpr std::array<float, 4> fractionByLevel{0.55F, 0.76F, 0.76F, 0.98F};
    return fractionByLevel.at(static_cast<size_t>(std::clamp(level, 1, 4) - 1));
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
    auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, level)) * mineDamageMult);
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

        if (w.disabled)
        {
            continue;
        }

        if (w.type == WeaponType::Orbit || w.type == WeaponType::Beam)
        {

            continue;
        }

        w.timer -= deltaTime;
        if (w.timer > 0)
        {
            continue;
        }
        w.timer = weaponCooldown(game, w.type, w.level);

        switch (w.type)
        {
        case WeaponType::Forward:
        {
            const Vector2 dir = aimAtMouse(game);

            const int32_t shots = std::min(2, 1 + w.level / 4);
            constexpr float spread = 10;
            auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                            forwardDamageMult);
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
                    .pierceRemaining = 0,
                    .ricochetRemaining = ricochetLevel(game),
                    .source = DamageSource::Forward,
                    .ricochetThenPhase = w.evolved});
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
            auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                            shockDamageMult);
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
            const auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                                  flakCannonDamageMult);
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
            auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                            railgunBaseDamageMult);
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
        case WeaponType::Sniper:
        {

            if (const auto target = farthestEnemy(game, game.run.player.position);
                target.has_value())
            {
                const Vector2 dir =
                    Vector2Normalize(Vector2Subtract(*target, game.run.player.position));
                auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                                sniperDamageMult);
                if (w.level >= 3)
                {
                    dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.3F);
                }
                const Vector2 start = game.run.player.position;
                const Vector2 end = Vector2Add(start, Vector2Scale(dir, sniperLineLength));
                sniperLineHit(game, start, end, dmg);
                game.run.sniperShots.push_back(
                    ChainLightningBolt{.from = start, .to = end, .timer = sniperLineFlashDuration});
                playSFX(game, game.resources.sounds.shoot);
            }
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
                radius + enemyCollisionRadius(enemy))
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

    for (auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        const float bossRadius = std::max(boss.size.x, boss.size.y) / 2;
        if (Vector2Distance(center, bossCenter) <= radius + bossRadius)
        {
            damageBoss(game, boss, dmg);
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
            breakAsteroid(game, asteroid);
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

    if (it == game.run.weapons.end() || it->disabled)
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
            !touchesAnyBlade(enemy.position, enemyCollisionRadius(enemy)))
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
                damageEliteHazard(game, j, tick, false);
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
            damageBoss(game, boss, hit, true, true);
            recordDamage(game, DamageSource::Orbit, hit);
        }
        else
        {
            boss.orbitDamageAccum += dps * deltaTime;
            if (boss.orbitDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(boss.orbitDamageAccum);
                damageBoss(game, boss, tick, false, true);
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
            breakAsteroid(game, asteroid);
        }
    }

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
        if (w.type != WeaponType::Orbit || w.disabled)
        {
            continue;
        }

        if (w.flashTimer > 0)
        {
            w.flashTimer -= deltaTime;
        }

        const int32_t count = orbitBladeCount(w.level);
        auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) * 0.7F);
        if (w.evolved)
        {
            dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
        }

        const Vector2 dir = aimAtMouse(game);
        const float noseAngle = std::atan2(dir.y, dir.x);
        constexpr float twoPi = 2 * std::numbers::pi_v<float>;
        const float angularStep = 2 * currentShip(game).orbitSpinMult * deltaTime;

        float ringRadius = orbitRadius(w.level);
        if (w.evolved)
        {
            ringRadius *= 1.4F;
        }

        for (int32_t s = 0; s < count; s++)
        {
            const float bladeAngle = static_cast<float>(GetTime()) * 2 * currentShip(game).orbitSpinMult +
                                     static_cast<float>(s) * twoPi / static_cast<float>(count);
            float rel = std::fmod(bladeAngle - noseAngle, twoPi);
            if (rel < 0)
            {
                rel += twoPi;
            }
            if (rel >= angularStep)
            {
                continue;
            }

            const Vector2 shotDir{.x = std::cos(bladeAngle), .y = std::sin(bladeAngle)};
            const Vector2 bladePos =
                Vector2Add(game.run.player.position, Vector2Scale(shotDir, ringRadius));

            game.run.orbitBladeProjectiles.push_back(
                OrbitBladeProjectile{.position = bladePos,
                                     .velocity = Vector2Scale(shotDir, orbitProjectileSpeed),
                                     .radius = orbitProjectileRadius,
                                     .damage = dmg,
                                     .active = true});

            w.flashTimer = orbitRegrowDuration;
            playSFX(game, game.resources.sounds.homingLaunch);
        }
    }
}

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
                                      enemyCollisionRadius(enemy)))
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
                damageBoss(game, boss, proj.damage, true, source == DamageSource::Orbit);
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
                breakAsteroid(game, asteroid);
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
                                          enemyCollisionRadius(enemy)))
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
                    breakAsteroid(game, asteroid);
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

    if (it == game.run.weapons.end() || it->disabled)
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
                enemy.position, enemyCollisionRadius(enemy), start, end))
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
                damageEliteHazard(game, j, tick, false);
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
            damageBoss(game, boss, hit, true, true);
            recordDamage(game, DamageSource::Beam, hit);
        }
        else
        {
            boss.beamDamageAccum += dps * deltaTime;
            if (boss.beamDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(boss.beamDamageAccum);
                damageBoss(game, boss, tick, false, true);
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
            breakAsteroid(game, asteroid);
        }
    }
}

void nerveConeHit(Game& game, Vector2 origin, Vector2 direction, float range, float halfAngleRad,
                  int32_t dmg)
{
    const auto inCone = [&](Vector2 pos, float entityRadius)
    {
        const Vector2 toEntity = Vector2Subtract(pos, origin);
        const float dist = Vector2Length(toEntity);
        if (dist > range + entityRadius)
        {
            return false;
        }
        if (dist < 0.01F)
        {
            return true;
        }
        const Vector2 dirToEntity = Vector2Scale(toEntity, 1.0F / dist);
        const float dot =
            std::clamp(direction.x * dirToEntity.x + direction.y * dirToEntity.y, -1.0F, 1.0F);
        const float angle = std::acos(dot);
        const float padding = std::atan2(entityRadius, dist);
        return angle <= halfAngleRad + padding;
    };

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        const auto& enemy = game.run.enemies.at(j);
        if (enemy.active && !enemy.phased && inCone(enemy.position, enemyCollisionRadius(enemy)))
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
        if (hazard.active && inCone(hazard.position, EliteHazardConstants::radius))
        {
            damageEliteHazard(game, j, dmg);
            recordDamage(game, DamageSource::Nerve, dmg);
        }
    }

    for (auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active && inCone(asteroid.position, asteroid.radius))
        {
            asteroid.active = false;
            game.run.score += asteroidScore(asteroid.tier);
            breakAsteroid(game, asteroid);
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
        if (inCone(bossCenter, boss.size.x / 2))
        {
            if (boss.isBanished && boss.banishedStage == 1 && !boss.banishedEyeChargeBurstUsed)
            {
                damageBanishedEyeBurst(game, boss, banishedEyeBurstDamageDivisor);
            }
            else
            {
                damageBoss(game, boss, dmg);
                recordDamage(game, DamageSource::Nerve, dmg);
            }
        }
    }
}

void sniperLineHit(Game& game, Vector2 start, Vector2 end, int32_t dmg)
{
    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        const auto& enemy = game.run.enemies.at(j);
        if (enemy.active && !enemy.phased &&
            CheckCollisionCircleLine(enemy.position, enemyCollisionRadius(enemy), start, end))
        {
            damageEnemy(game, j, dmg);
            recordDamage(game, DamageSource::Sniper, dmg);
            if (!enemy.active)
            {
                recordWeaponKill(game, WeaponType::Sniper);
            }
        }
    }

    for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
    {
        const auto& hazard = game.run.eliteHazards.at(j);
        if (hazard.active &&
            CheckCollisionCircleLine(hazard.position, EliteHazardConstants::radius, start, end))
        {
            damageEliteHazard(game, j, dmg);
            recordDamage(game, DamageSource::Sniper, dmg);
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
            damageBoss(game, boss, dmg, true, true);
            recordDamage(game, DamageSource::Sniper, dmg);
        }
    }
}

void updateSniperShots(Game& game, float deltaTime)
{
    for (auto& bolt : game.run.sniperShots)
    {
        bolt.timer -= deltaTime;
    }
    std::erase_if(game.run.sniperShots, [](const ChainLightningBolt& bolt)
                  { return bolt.timer <= 0; });
}

void fireNerveCone(Game& game)
{
    const Vector2 dir = aimAtMouse(game);
    const Vector2 start = game.run.player.position;
    const Vector2 end = Vector2Add(start, Vector2Scale(dir, nerveBurstLength));

    nerveConeHit(game, start, dir, nerveBurstLength, nerveConeHalfAngleDeg * DEG2RAD,
                nerveBurstDamage);

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
        fireNerveCone(game);
        break;
    case ShipClass::Ranger:
    case ShipClass::Count:
    default:
        fireNerveBall(game);
        break;
    }
}

void spawnRockCluster(Game& game, Vector2 center)
{
    constexpr int32_t chunkCount = 5;
    constexpr float orbitRadius = 70.0F;
    const float rotationSpeed =
        static_cast<float>(GetRandomValue(0, 1) == 0 ? 1 : -1) * rockClusterRotationSpeed;

    for (int32_t i = 0; i < chunkCount; i++)
    {
        const auto tier = GetRandomValue(0, 1) == 0 ? AsteroidTier::Large : AsteroidTier::Medium;
        const float startAngle = static_cast<float>(i) * (360.0F / chunkCount);

        game.run.asteroids.push_back(Asteroid{
            .position =
                Vector2Add(center, Vector2{.x = std::cos(startAngle * DEG2RAD) * orbitRadius,
                                           .y = std::sin(startAngle * DEG2RAD) * orbitRadius}),
            .velocity = Vector2{},
            .radius = asteroidRadius(tier),
            .tier = tier,
            .active = true,
            .inCluster = true,
            .clusterCenter = center,
            .clusterOrbitRadius = orbitRadius,
            .clusterAngleDeg = startAngle,
            .clusterRotationSpeed = rotationSpeed});
    }
}

void updateSolarForgeHeat(Game& game, float deltaTime)
{
    if (game.run.player.health <= 0)
    {
        return;
    }

    const bool inSolarForge = currentBiome(game.run.waveNumber) == Biome::SolarForge;
    const bool onSafePath = !inSolarForge || isSolarForgeCaveOpen(game.run.player.position);

    if (onSafePath)
    {
        game.run.solarForgeHeatMeter =
            std::max(0.0F, game.run.solarForgeHeatMeter - solarForgeHeatDecayRate * deltaTime);
    }
    else
    {
        game.run.solarForgeHeatMeter += deltaTime;
    }

    if (!inSolarForge || game.run.solarForgeHeatMeter <= solarForgeMeltThreshold)
    {
        game.run.solarForgeHeatTickTimer = 0;
        return;
    }

    game.run.solarForgeHeatTickTimer -= deltaTime;
    if (game.run.solarForgeHeatTickTimer <= 0)
    {
        game.run.solarForgeHeatTickTimer = solarForgeMeltTickInterval;
        damagePlayer(game, enemyDamage(game, solarForgeMeltDamage));
    }
}

void updatePunctumThunder(Game& game, float deltaTime)
{
    if (game.run.punctumThunderFlashTimer > 0)
    {
        game.run.punctumThunderFlashTimer -= deltaTime;
    }

    if (currentBiome(game.run.waveNumber) != Biome::Punctum)
    {
        return;
    }

    game.run.punctumThunderTimer -= deltaTime;
    if (game.run.punctumThunderTimer > 0)
    {
        return;
    }

    float intervalReduction = 0;
    if (game.run.waveNumber >= punctumThunderSpeedupWaveA)
    {
        intervalReduction += 1.0F;
    }
    if (game.run.waveNumber >= punctumThunderSpeedupWaveB)
    {
        intervalReduction += 1.0F;
    }
    const float intervalMin = std::max(1.0F, punctumThunderIntervalMin - intervalReduction);
    const float intervalMax = std::max(intervalMin + 1.0F, punctumThunderIntervalMax - intervalReduction);
    game.run.punctumThunderTimer =
        static_cast<float>(GetRandomValue(static_cast<int32_t>(intervalMin * 10),
                                          static_cast<int32_t>(intervalMax * 10))) /
        10.0F;
    game.run.punctumThunderFlashTimer = punctumThunderFlashDuration;
    const float boltAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto boltDist = static_cast<float>(GetRandomValue(300, 900));
    game.run.punctumThunderBoltOrigin =
        Vector2Add(game.run.player.position,
                  Vector2{.x = std::cos(boltAngle) * boltDist, .y = std::sin(boltAngle) * boltDist});
    game.run.punctumThunderBoltSeed = static_cast<float>(GetRandomValue(0, 10000));
    triggerShake(game, 6, 0.3F);
    playSFX(game, game.resources.sounds.thunder);
}

void updateFluidHazardContact(Game& game)
{
    if (game.run.player.health <= 0)
    {
        return;
    }

    bool touching = false;
    for (const auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active && asteroid.isFluid &&
            Vector2Distance(asteroid.position, game.run.player.position) <=
                asteroid.radius + game.run.player.radius)
        {
            touching = true;
            break;
        }
    }

    if (!touching)
    {
        return;
    }

    game.run.player.burnDps = solarForgeFluidBurnDps;
    game.run.player.burnRefreshTimer = solarForgeFluidBurnRefreshDuration;
}

void updateGasHazards(Game& game, float deltaTime)
{
    const bool shieldDashing = game.run.player.shieldDashing;

    for (auto& cloud : game.run.gasHazards)
    {
        if (!cloud.active)
        {
            continue;
        }

        cloud.life -= deltaTime;
        if (cloud.life <= 0)
        {
            cloud.active = false;
            continue;
        }

        const Vector2 toPlayer = Vector2Subtract(game.run.player.position, cloud.position);
        if (Vector2Length(toPlayer) > 1)
        {
            cloud.position = Vector2Add(
                cloud.position,
                Vector2Scale(Vector2Normalize(toPlayer), wreckwormCloudDriftSpeed * deltaTime));
        }

        const float contactDist = cloud.radius + game.run.player.radius;
        if (Vector2Distance(cloud.position, game.run.player.position) > contactDist)
        {
            continue;
        }

        if (shieldDashing)
        {
            cloud.active = false;
            triggerShake(game, 4, 0.15F);
            continue;
        }

        if (game.run.player.health > 0)
        {
            game.run.player.burnDps = wreckwormCloudBurnDps;
            game.run.player.burnRefreshTimer = wreckwormCloudBurnRefreshDuration;
        }
    }

    std::erase_if(game.run.gasHazards, [](const GasHazard& cloud) { return !cloud.active; });
}

void updateWaveSpawner(Game& game, float deltaTime)
{
    if (game.sandbox && !game.sandboxNaturalSpawnEnabled)
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

            if (game.run.waveNumber % finalBossWaveInterval == 0 &&
                currentBiome(game.run.waveNumber) != Biome::Punctum)
            {
                spawnFinalBossWave(game);
            }
            else if (currentBiome(game.run.waveNumber) == Biome::ShatteredBelt &&
                     (game.run.waveNumber == 10 || game.run.waveNumber == 20 ||
                      game.run.waveNumber == 25))
            {
                spawnBeltbreaker(game, game.run.waveNumber);
            }
            else if (currentBiome(game.run.waveNumber) == Biome::Rustbloom &&
                     (game.run.waveNumber == 30 || game.run.waveNumber == 40 ||
                      game.run.waveNumber == 50))
            {
                spawnWreckworm(game, game.run.waveNumber);
            }
            else if (currentBiome(game.run.waveNumber) == Biome::SolarForge &&
                     (game.run.waveNumber == 60 || game.run.waveNumber == 70 ||
                      game.run.waveNumber == 75))
            {
                spawnSlagmaw(game, game.run.waveNumber);
            }
            else if (currentBiome(game.run.waveNumber) == Biome::Punctum &&
                     (game.run.waveNumber == 80 || game.run.waveNumber == 90 ||
                      game.run.waveNumber == 100))
            {
                spawnKraken(game, game.run.waveNumber);
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

    if (game.run.bosses.empty())
    {
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

            constexpr float spawnRateMult = 0.85F;
            const bool inPunctum = currentBiome(game.run.waveNumber) == Biome::Punctum;
            const float punctumIntervalMult = inPunctum ? punctumSpawnIntervalMult : 1.0F;
            game.run.enemySpawnTimer = interval * spawnRateMult * punctumIntervalMult;

            const int rawSpawnCount = 1 + game.run.waveNumber / 4;
            const int spawnCount =
                inPunctum ? std::max(1, rawSpawnCount / punctumSpawnCountDivisor) : rawSpawnCount;
            for (int i = 0; i < spawnCount &&
                            static_cast<int>(game.run.enemies.size()) < UpdateConstants::maxEnemies;
                 i++)
            {
                spawnEnemy(game);
            }
        }
    }

    game.run.asteroidSpawnTimer -= deltaTime;
    if (game.run.asteroidSpawnTimer <= 0 &&
        static_cast<int>(game.run.asteroids.size()) < maxAsteroid)
    {
        constexpr float asteroidIntervalMultiplier = 1.0F;
        game.run.asteroidSpawnTimer =
            static_cast<float>(GetRandomValue(8, 16)) / 10.0F * asteroidIntervalMultiplier;

        if (currentBiome(game.run.waveNumber) == Biome::ShatteredBelt && GetRandomValue(0, 2) == 0)
        {
            const float clusterAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 clusterCenter =
                Vector2Add(game.run.player.position, Vector2{.x = std::cos(clusterAngle) * 500,
                                                             .y = std::sin(clusterAngle) * 500});
            spawnRockCluster(game, clusterCenter);
            return;
        }

        AsteroidTier tier = AsteroidTier::Large;
        if (GetRandomValue(0, 1) == 1)
        {
            tier = AsteroidTier::Medium;
        }

        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const Vector2 spawnPos =
            Vector2Add(game.run.player.position,
                       Vector2{.x = std::cos(angle) * 500, .y = std::sin(angle) * 500});

        if (currentBiome(game.run.waveNumber) == Biome::SolarForge)
        {

            game.run.asteroids.push_back(Asteroid{.position = spawnPos,
                                                  .velocity = Vector2{},
                                                  .radius = asteroidRadius(tier),
                                                  .tier = tier,
                                                  .active = true,
                                                  .isFluid = true});
            return;
        }

        const float driftAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const Vector2 direction{.x = std::cos(driftAngle), .y = std::sin(driftAngle)};
        const float speed = static_cast<float>(GetRandomValue(25, 45)) / 10.0F;

        game.run.asteroids.push_back(Asteroid{.position = spawnPos,
                                              .velocity = Vector2Scale(direction, speed),
                                              .radius = asteroidRadius(tier),
                                              .tier = tier,
                                              .active = true});
    }
}

constexpr int eliteHazardCap = 3;

void updatePickups(Game& game, float deltaTime)
{
    const float magnetRadius = 110 +
                               110 *
                                   static_cast<float>(game.run.skillLevels.at(
                                       static_cast<size_t>(SkillType::PickupRadius))) *
                                   0.2F +
                               pickupRadiusPerLevel * static_cast<float>(game.run.level - 1);

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
            pickupEffectDuration * pickupDurationScale(game.run.waveNumber);
        game.run.player.regenRate = regenRate;
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::DashTrail:
        game.run.player.dashTrailTimer =
            pickupEffectDuration * pickupDurationScale(game.run.waveNumber);
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
    case PickupType::Overdrive:
        game.run.player.overdriveTimer =
            overdriveDuration * pickupDurationScale(game.run.waveNumber);
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::Danger:

        if (!game.run.weaponDowngrade.has_value() && !game.run.weapons.empty())
        {
            auto& weapon = game.run.weapons.at(static_cast<size_t>(
                GetRandomValue(0, static_cast<int32_t>(game.run.weapons.size()) - 1)));

            WeaponDowngrade downgrade{.type = weapon.type, .timer = dangerDowngradeDuration};
            if (weapon.evolved)
            {
                weapon.evolved = false;
                downgrade.unevolved = true;
            }
            else if (weapon.level > 1)
            {
                weapon.level -= 1;
            }
            else
            {
                weapon.disabled = true;
                downgrade.disabled = true;
            }

            game.run.achievementToast =
                std::string(effectiveWeaponName(weapon)) + " stolen!";
            game.run.achievementToastTimer = 3.0F;
            game.run.weaponDowngrade = downgrade;
            triggerShake(game, 6, 0.25F);
            playSFX(game, game.resources.sounds.explosion);
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
            pickupEffectDuration * pickupDurationScale(game.run.waveNumber);
        break;
    case ElementMechanism::Nova:
        for (auto& enemy : game.run.enemies)
        {
            if (enemy.active && !enemy.phased)
            {
                applyElementDebuff(enemy, element, burnDps);
            }
        }
        for (auto& boss : game.run.bosses)
        {
            if (boss.health > 0)
            {
                applyElementDebuff(boss, element, burnDps);
            }
        }
        break;
    case ElementMechanism::Count:
        break;
    }

    playSFX(game, game.resources.sounds.menuConfirm);
}

void startLevelUp(Game& game)
{
    game.run.xp -= game.run.xpToNext;
    game.run.level++;
    game.run.xpToNext = 100 + (game.run.level - 1) * 60;

    game.run.player.maxHealth += maxHealthPerLevel;
    game.run.player.health += maxHealthPerLevel;

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

        if (game.run.postCapDamageLevels < postCapDamageLevelsCap)
        {
            game.run.postCapDamageLevels++;
        }
    }
    else
    {
        std::vector<SkillType> owned;
        for (const auto id : eligible)
        {
            if (game.run.skillLevels.at(static_cast<size_t>(id)) > 0)
            {
                owned.push_back(id);
            }
        }

        std::vector<SkillType> picks;
        const bool affinityTriggers = !owned.empty() && GetRandomValue(0, 99) < 50;
        if (affinityTriggers)
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
        else
        {
            picks = sampleDistinct(eligible, 3);
        }

        for (const auto id : picks)
        {
            choices.push_back(LevelUpChoice{.type = ChoiceType::Skill, .skill = id});
        }

        while (choices.size() < 3 && !eligible.empty())
        {
            const auto extra = sampleDistinct(eligible, 1);
            choices.push_back(LevelUpChoice{.type = ChoiceType::Skill, .skill = extra.front()});
        }
    }

    constexpr size_t evolvableWeaponCount = 6;
    std::vector<WeaponType> evolvable;
    for (size_t i = 0; i < evolvableWeaponCount; i++)
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
    if (!evolvable.empty() && !choices.empty())
    {
        const auto pick = evolvable.at(
            static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(evolvable.size()) - 1)));
        const auto replaceIndex =
            static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(choices.size()) - 1));
        choices.at(replaceIndex) = LevelUpChoice{.type = ChoiceType::Evolve, .weapon = pick};
    }

    for (size_t i = choices.size(); i > 1; i--)
    {
        const auto j = static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(i) - 1));
        std::swap(choices.at(i - 1), choices.at(j));
    }

    const auto& dropEntry = pickupCatalog.at(pickWeightedPickupIndex(game.run.waveNumber));
    choices.push_back(LevelUpChoice{.type = ChoiceType::Pickup,
                                    .pickupType = dropEntry.type,
                                    .element = dropEntry.element,
                                    .mechanism = dropEntry.mechanism});

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
                damageBoss(game, boss, bullet.damage, true, bullet.source == DamageSource::Railgun);
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
                breakAsteroid(game, asteroid);
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
            if (CheckCollisionCircles(bullet.position, bullet.radius, enemy.position,
                                      enemyCollisionRadius(enemy)))
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

                if (bullet.splashRadius > 0)
                {
                    aoePulse(game, bullet.position, bullet.splashRadius, bullet.damage / 2,
                             bullet.source);
                }

                if (bullet.pierceRemaining > 0)
                {

                    bullet.pierceRemaining--;
                }
                else if (bullet.ricochetRemaining > 0)
                {

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
                    break;
                }
                else if (bullet.phasing)
                {
                    // already phasing: keeps flying and keeps damaging everything it touches
                }
                else if (bullet.ricochetThenPhase)
                {
                    bullet.phasing = true;
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

void fireChainLightning(Game& game, const Weapon& weapon)
{
    const auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, weapon.level)) *
                                          chainLightningDamageMult);

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
                    enemyCollisionRadius(enemy))
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

auto droneCountForLevel(int32_t level) -> size_t
{
    if (level >= 3)
    {
        return 2;
    }
    return level >= 1 ? 1 : 0;
}

auto dronePosition(Game& game, size_t index) -> Vector2
{
    const Vector2 dir = aimAtMouse(game);
    const Vector2 perp{.x = -dir.y, .y = dir.x};
    const auto side = index % 2 == 0 ? 1.0F : -1.0F;
    const size_t rankInt = index / 2;
    const auto rank = static_cast<float>(rankInt);
    const float lateral = side * (droneOrbitRadius + rank * 30.0F);
    return Vector2Add(game.run.player.position, Vector2Scale(perp, lateral));
}

void updateDroneContactDamage(Game& game, float deltaTime, int32_t followerDmg, int32_t laserDmg)
{
    const auto touchesDrones = [&](Vector2 enemyPos, float enemyRadius, DamageSource& sourceOut,
                                   int32_t& dmgOut)
    {
        for (const auto& drone : game.run.followerDrones)
        {
            if (Vector2Distance(enemyPos, drone.position) <= droneContactRadius + enemyRadius)
            {
                sourceOut = DamageSource::FollowerDrone;
                dmgOut = followerDmg;
                return true;
            }
        }
        for (const auto& drone : game.run.laserDrones)
        {
            if (Vector2Distance(enemyPos, drone.position) <= droneContactRadius + enemyRadius)
            {
                sourceOut = DamageSource::LaserDrone;
                dmgOut = laserDmg;
                return true;
            }
        }
        return false;
    };

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        auto& enemy = game.run.enemies.at(j);
        DamageSource source = DamageSource::FollowerDrone;
        int32_t baseDmg = 0;
        if (!enemy.active || enemy.phased ||
            !touchesDrones(enemy.position, enemyCollisionRadius(enemy), source, baseDmg))
        {
            enemy.droneContact = false;
            enemy.droneDamageAccum = 0;
            continue;
        }

        const float dps = static_cast<float>(baseDmg) * droneContactDpsMult;
        if (!enemy.droneContact)
        {
            enemy.droneContact = true;
            const auto hit = std::max(1, static_cast<int32_t>(dps));
            damageEnemy(game, j, hit);
            recordDamage(game, source, hit);
        }
        else
        {
            enemy.droneDamageAccum += dps * deltaTime;
            if (enemy.droneDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(enemy.droneDamageAccum);
                damageEnemy(game, j, tick);
                recordDamage(game, source, tick);
                enemy.droneDamageAccum -= static_cast<float>(tick);
            }
        }
        if (!enemy.active)
        {
            recordWeaponKill(game, source == DamageSource::FollowerDrone ? WeaponType::FollowerDrone
                                                                         : WeaponType::LaserDrone);
        }
    }
}

void updateFollowerDrones(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::FollowerDrone; });
    if (it == game.run.weapons.end() || it->disabled)
    {
        game.run.followerDrones.clear();
        return;
    }

    const Weapon& w = *it;
    const size_t desired = droneCountForLevel(w.level);
    while (game.run.followerDrones.size() < desired)
    {
        game.run.followerDrones.push_back(
            FollowerDrone{.position = game.run.player.position, .attackTimer = 0, .shotTimer = 0});
    }
    game.run.followerDrones.resize(desired);

    const auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                          followerDroneDamageFraction(w.level));
    const float interval = droneAttackIntervalBase / (w.level >= 2 ? 1.3F : 1.0F);
    const auto shotDmg = static_cast<int32_t>(static_cast<float>(dmg) * droneShotDamageMult);
    const Vector2 aimDir = aimAtMouse(game);

    for (size_t i = 0; i < game.run.followerDrones.size(); i++)
    {
        auto& drone = game.run.followerDrones.at(i);
        drone.position = dronePosition(game, i);

        drone.attackTimer -= deltaTime;
        if (drone.attackTimer <= 0)
        {
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

        drone.shotTimer -= deltaTime;
        if (drone.shotTimer <= 0 && nearestEnemyWithin(game, drone.position, laserDroneRangeBase))
        {
            game.run.bullets.push_back(Bullet{.position = drone.position,
                                              .velocity = Vector2Scale(aimDir, droneShotSpeed),
                                              .radius = projectileSize,
                                              .color = Palette::Shield,
                                              .active = true,
                                              .damage = shotDmg,
                                              .source = DamageSource::FollowerDrone});
            drone.shotTimer = droneShotInterval;
        }
    }
}

void updateLaserDrones(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::LaserDrone; });
    if (it == game.run.weapons.end() || it->disabled)
    {
        game.run.laserDrones.clear();
        return;
    }

    const Weapon& w = *it;
    const size_t desired = droneCountForLevel(w.level);
    while (game.run.laserDrones.size() < desired)
    {
        game.run.laserDrones.push_back(LaserDrone{.position = game.run.player.position,
                                                  .attackTimer = 0,
                                                  .beamFlashTimer = 0,
                                                  .beamTarget = Vector2{}});
    }
    game.run.laserDrones.resize(desired);

    const auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                          laserDroneDamageFraction(w.level));
    const float interval = droneAttackIntervalBase;
    const float range =
        laserDroneRangeBase + (w.level >= 2 ? 60.0F : 0.0F) + (w.level >= 4 ? 60.0F : 0.0F);

    for (size_t i = 0; i < game.run.laserDrones.size(); i++)
    {
        auto& drone = game.run.laserDrones.at(i);
        drone.position = dronePosition(game, i);

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

void updateDroneContact(Game& game, float deltaTime)
{
    if (game.run.followerDrones.empty() && game.run.laserDrones.empty())
    {
        return;
    }

    int32_t followerDmg = 0;
    if (const auto it = std::ranges::find_if(
            game.run.weapons,
            [](const Weapon& w) { return w.type == WeaponType::FollowerDrone; });
        it != game.run.weapons.end())
    {
        followerDmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, it->level)) *
                                           followerDroneDamageFraction(it->level));
    }

    int32_t laserDmg = 0;
    if (const auto it = std::ranges::find_if(
            game.run.weapons, [](const Weapon& w) { return w.type == WeaponType::LaserDrone; });
        it != game.run.weapons.end())
    {
        laserDmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, it->level)) *
                                        laserDroneDamageFraction(it->level));
    }

    updateDroneContactDamage(game, deltaTime, followerDmg, laserDmg);
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

auto flamethrowerRangeFor(int32_t level) -> float
{
    return flamethrowerRange + (level >= 3 ? 40.0F : 0.0F);
}

auto flamethrowerHalfAngleRad() -> float { return flamethrowerHalfAngle; }

auto flamethrowerConeDirs(const Game& game, int32_t level) -> std::vector<Vector2>
{
    const Vector2 aim = aimAtMouse(game);
    std::vector<Vector2> coneDirs;
    if (level >= 2)
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
    return coneDirs;
}

auto flamethrowerActive(int32_t level) -> bool
{
    if (level < 3)
    {
        return true;
    }
    return std::fmod(static_cast<float>(GetTime()), 4.0F) <= 3.0F;
}

void updateFlamethrower(Game& game, float deltaTime)
{
    for (auto& particle : game.run.flameParticles)
    {
        particle.position = Vector2Add(particle.position, particle.velocity);
        particle.life -= deltaTime;
    }
    std::erase_if(game.run.flameParticles, [](const Particle& p) { return p.life <= 0; });

    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::Flamethrower; });
    if (it == game.run.weapons.end() || it->disabled)
    {
        return;
    }
    const Weapon& w = *it;

    if (!flamethrowerActive(w.level))
    {
        return;
    }

    const float range = flamethrowerRangeFor(w.level);
    const auto dps = static_cast<float>(weaponDamage(game, w.level)) * flamethrowerDamageMult;
    const auto coneDirs = flamethrowerConeDirs(game, w.level);

    const auto particlesPerConeThisFrame = std::max(
        1, static_cast<int32_t>(static_cast<float>(flameParticlesPerConePerSecond) * deltaTime));

    const float particleSpeed = (range / flameParticleLife) * deltaTime;
    for (const auto& dir : coneDirs)
    {
        const float baseAngle = std::atan2(dir.y, dir.x);
        for (int32_t i = 0; i < particlesPerConeThisFrame; i++)
        {
            const float spreadAngle =
                baseAngle + (static_cast<float>(GetRandomValue(-100, 100)) / 100.0F) *
                                flamethrowerHalfAngle;
            const float speedJitter = static_cast<float>(GetRandomValue(70, 100)) / 100.0F;

            const Vector2 outward{.x = std::cos(spreadAngle) * particleSpeed * speedJitter,
                                  .y = std::sin(spreadAngle) * particleSpeed * speedJitter};
            game.run.flameParticles.push_back(
                Particle{.position = game.run.player.position,
                         .velocity = Vector2Add(outward, game.run.player.velocity),
                         .radius = flameParticleRadius,
                         .life = flameParticleLife,
                         .maxLife = flameParticleLife,
                         .color = GetRandomValue(0, 1) == 0 ? Palette::Accent : Palette::Crit});
        }
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

    for (auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        const Vector2 toBoss = Vector2Subtract(bossCenter, game.run.player.position);
        const float dist = Vector2Length(toBoss);
        if (dist > range || dist <= 0.01F)
        {
            continue;
        }
        const Vector2 dirToBoss = Vector2Scale(toBoss, 1.0F / dist);
        const bool inCone = std::ranges::any_of(
            coneDirs, [&](const Vector2& dir)
            { return Vector2DotProduct(dir, dirToBoss) >= std::cos(flamethrowerHalfAngle); });
        if (!inCone)
        {
            continue;
        }
        const auto tick = static_cast<int32_t>(dps * deltaTime);
        if (tick <= 0)
        {
            continue;
        }

        damageBoss(game, boss, tick, false);
        recordDamage(game, DamageSource::Flamethrower, tick);
    }
}

void updateSolarForgeEnemyFireParticles(Game& game, float deltaTime)
{
    for (const auto& enemy : game.run.enemies)
    {
        if (!enemy.active || enemy.phased)
        {
            continue;
        }
        const auto& kind = enemyKinds.at(static_cast<size_t>(enemy.kind));
        if (!kind.biomeExclusive || kind.biome != Biome::SolarForge)
        {
            continue;
        }
        if (GetRandomValue(0, 99) >= solarForgeEnemyFireChancePercent)
        {
            continue;
        }

        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto offset = static_cast<float>(GetRandomValue(0, static_cast<int32_t>(kind.radius)));
        game.run.flameParticles.push_back(Particle{
            .position = Vector2Add(
                enemy.position, Vector2{.x = std::cos(angle) * offset, .y = std::sin(angle) * offset}),
            .velocity = Vector2{.x = 0, .y = -solarForgeEnemyFireParticleSpeed * deltaTime},
            .radius = solarForgeEnemyFireParticleRadius,
            .life = solarForgeEnemyFireParticleLife,
            .maxLife = solarForgeEnemyFireParticleLife,
            .color = GetRandomValue(0, 1) == 0 ? Palette::ElementBurn : Palette::SolarForgeAccent});
    }

    for (const auto& boss : game.run.bosses)
    {
        if (!boss.isSlagmaw || boss.health <= 0)
        {
            continue;
        }
        if (GetRandomValue(0, 99) >= solarForgeEnemyFireChancePercent * 3)
        {
            continue;
        }
        const Vector2 center{.x = boss.position.x + boss.size.x / 2,
                             .y = boss.position.y + boss.size.y / 2};
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto offset =
            static_cast<float>(GetRandomValue(0, static_cast<int32_t>(boss.size.x / 2)));
        game.run.flameParticles.push_back(Particle{
            .position = Vector2Add(center, Vector2{.x = std::cos(angle) * offset,
                                                    .y = std::sin(angle) * offset}),
            .velocity = Vector2{.x = 0, .y = -solarForgeEnemyFireParticleSpeed * deltaTime},
            .radius = solarForgeEnemyFireParticleRadius * 1.5F,
            .life = solarForgeEnemyFireParticleLife,
            .maxLife = solarForgeEnemyFireParticleLife,
            .color = GetRandomValue(0, 1) == 0 ? Palette::ElementBurn : Palette::SolarForgeAccent});
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
    return baseBurnDps * enemyHealthMult * waveEnemyScale(game);
}

void updateAsteroids(Game& game, float deltaTime)
{
    for (auto& asteroid : game.run.asteroids)
    {
        if (!asteroid.active)
        {
            continue;
        }

        if (asteroid.inCluster)
        {

            asteroid.clusterAngleDeg = std::fmod(
                asteroid.clusterAngleDeg + asteroid.clusterRotationSpeed * deltaTime, 360.0F);
            asteroid.position = Vector2Add(
                asteroid.clusterCenter, Vector2{.x = std::cos(asteroid.clusterAngleDeg * DEG2RAD) *
                                                     asteroid.clusterOrbitRadius,
                                                .y = std::sin(asteroid.clusterAngleDeg * DEG2RAD) *
                                                     asteroid.clusterOrbitRadius});
        }
        else
        {
            asteroid.position = Vector2Add(asteroid.position,
                                           Vector2Scale(asteroid.velocity, deltaTime * frameScale));
        }

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
                breakAsteroid(game, asteroid);
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
            breakAsteroid(game, asteroid);
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

            if (!projectile.fromPlayer && projectile.ricochetRemaining > 0)
            {
                const Vector2 normal =
                    Vector2Normalize(Vector2Subtract(projectile.position, asteroid.position));
                const float speed = Vector2Length(projectile.velocity);
                const Vector2 incomingDir = Vector2Normalize(projectile.velocity);
                const float dot = Vector2DotProduct(incomingDir, normal);
                const Vector2 reflected =
                    Vector2Subtract(incomingDir, Vector2Scale(normal, 2.0F * dot));
                projectile.velocity = Vector2Scale(reflected, speed);
                projectile.ricochetRemaining--;
                break;
            }

            asteroid.active = false;
            breakAsteroid(game, asteroid);

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
                                       enemyCollisionRadius(enemy)))
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

            for (auto& boss : game.run.bosses)
            {
                const Rectangle bossRect{.x = boss.position.x,
                                         .y = boss.position.y,
                                         .width = boss.size.x,
                                         .height = boss.size.y};
                if (!projectile.active || boss.health <= 0 ||
                    !CheckCollisionCircleRec(projectile.position, projectile.radius, bossRect))
                {
                    continue;
                }

                projectile.active = false;
                damageBoss(game, boss, projectile.damage);
                recordDamage(game, DamageSource::Homing, projectile.damage);
                break;
            }
        }

        if (projectile.active && !projectile.fromPlayer && game.run.player.health > 0 &&
            game.run.player.immunityTimer <= 0 &&
            CheckCollisionCircles(game.run.player.position, game.run.player.radius,
                                  projectile.position, projectile.radius))
        {
            if (game.run.player.shieldActive || game.run.player.dashing)
            {
                Vector2 targetPos{};
                bool foundTarget = false;
                float bestDist = 1.0e9F;
                for (auto& boss : game.run.bosses)
                {
                    if (boss.health <= 0)
                    {
                        continue;
                    }
                    const Vector2 c{.x = boss.position.x + boss.size.x / 2,
                                    .y = boss.position.y + boss.size.y / 2};
                    const float d = Vector2Distance(projectile.position, c);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        targetPos = c;
                        foundTarget = true;
                    }
                }
                for (auto& enemy : game.run.enemies)
                {
                    if (!enemy.active || enemy.phased)
                    {
                        continue;
                    }
                    const float d = Vector2Distance(projectile.position, enemy.position);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        targetPos = enemy.position;
                        foundTarget = true;
                    }
                }

                const float speed = Vector2Length(projectile.velocity) * deflectedBulletSpeedMult;
                projectile.velocity =
                    foundTarget
                        ? Vector2Scale(Vector2Normalize(Vector2Subtract(targetPos, projectile.position)),
                                      speed)
                        : Vector2Scale(projectile.velocity, -deflectedBulletSpeedMult);
                projectile.fromPlayer = true;
                projectile.huntingNewTarget = false;
                tryComboRefund(game);
                playSFX(game, game.resources.sounds.critical);
            }
            else
            {
                projectile.active = false;
                damagePlayer(game, enemyDamage(game, projectile.damage));
            }
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

    const bool inPunctum = currentBiome(game.run.waveNumber) == Biome::Punctum;
    const bool spiraling = inPunctum && game.run.punctumTrapActive;
    const Vector2 fixedPull =
        inPunctum ? Vector2{.x = std::cos(punctumPullAngleDeg * DEG2RAD) * punctumPullSpeed,
                            .y = std::sin(punctumPullAngleDeg * DEG2RAD) * punctumPullSpeed}
                  : Vector2{};
    const Vector2 tileCenter{.x = tileW / 2, .y = tileH / 2};

    const auto driftFor = [&](Vector2 pos) -> Vector2
    {
        if (!spiraling)
        {
            return fixedPull;
        }
        const Vector2 toCenter = Vector2Subtract(tileCenter, pos);
        if (Vector2Length(toCenter) < 1.0F)
        {
            return Vector2{};
        }
        const Vector2 radial = Vector2Normalize(toCenter);
        const Vector2 tangent{.x = -radial.y, .y = radial.x};
        return Vector2Add(Vector2Scale(radial, punctumTrapSpiralInwardSpeed),
                          Vector2Scale(tangent, punctumTrapSpiralSwirlSpeed));
    };

    const auto respawnAtEdge = [&]() -> Vector2
    {
        return Vector2{.x = static_cast<float>(GetRandomValue(0, static_cast<int32_t>(tileW))),
                       .y = static_cast<float>(GetRandomValue(0, static_cast<int32_t>(tileH)))};
    };

    for (auto& p : game.run.bgParticles)
    {
        if (spiraling && Vector2Distance(p.position, tileCenter) <= punctumTrapConsumeRadius)
        {
            p.position = respawnAtEdge();
            continue;
        }

        p.position = Vector2Add(p.position, Vector2Add(p.velocity, driftFor(p.position)));

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

    if (inPunctum)
    {
        for (auto& s : game.run.stars)
        {
            if (spiraling && Vector2Distance(s.position, tileCenter) <= punctumTrapConsumeRadius)
            {
                s.position = respawnAtEdge();
                continue;
            }

            s.position = Vector2Add(s.position, driftFor(s.position));

            if (s.position.x < 0)
            {
                s.position.x += tileW;
            }
            if (s.position.x > tileW)
            {
                s.position.x -= tileW;
            }
            if (s.position.y < 0)
            {
                s.position.y += tileH;
            }
            if (s.position.y > tileH)
            {
                s.position.y -= tileH;
            }
        }
    }
}

void damageEliteHazard(Game& game, size_t index, int32_t amount, bool applyShake)
{
    auto& hazard = game.run.eliteHazards.at(index);
    hazard.health -= amount;
    spawnDamageNumber(game, hazard.position, amount);
    if (applyShake)
    {
        triggerShake(game, damageShakeIntensity(amount), damageShakeDuration);
    }
    if (hazard.health > 0)
    {
        return;
    }

    hazard.active = false;
    recordHazardKilled(game, hazard.role);
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

    const auto health = static_cast<int32_t>(static_cast<float>(eliteHazardBaseHealth) *
                                             enemyHealthMult * enemyHealthScale(game));

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
        if (static_cast<int>(game.run.eliteHazards.size()) < eliteHazardCap)
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
    if (game.run.punctumTrapActive)
    {
        return;
    }

    game.run.blackhole.timer -= deltaTime;

    if ((!game.sandbox || game.sandboxNaturalSpawnEnabled) && !game.run.blackhole.active &&
        game.run.blackhole.timer <= 0)
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

void updateBlackHoleDust(Game& game, float deltaTime)
{
    if (!game.run.blackhole.active)
    {
        return;
    }

    for (auto& dust : game.run.blackHoleDust)
    {
        const float speed = blackHoleDustInwardSpeedMin +
                            (blackHoleDustInwardSpeedMax - blackHoleDustInwardSpeedMin) *
                                (1.0F - dust.radiusFrac);
        dust.radiusFrac -= speed * deltaTime;

        if (dust.radiusFrac <= blackHoleDustCoreFraction)
        {
            dust.radiusFrac = 1.0F;
            dust.armPeakAngle = static_cast<float>(GetRandomValue(0, blackHoleDustArmCount - 1)) /
                                    static_cast<float>(blackHoleDustArmCount) * 2.0F *
                                    std::numbers::pi_v<float>;
            dust.jitter = static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0F * 0.25F;
            dust.brightness = static_cast<float>(GetRandomValue(60, 100)) / 100.0F;
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
        (static_cast<float>(exitFacing) - static_cast<float>(entryFacing)) * 90.0F + 180.0F;
    velocity = Vector2Rotate(velocity, deltaDegrees * DEG2RAD);

    const Vector2 exitDir = wormholeFacingVector(exitFacing);

    position =
        Vector2Add(exitPoint, Vector2Scale(exitDir, game.run.wormhole.radius + entityRadius + 4));

    return true;
}

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

    if (game.run.shieldDashShakeCooldownTimer > 0)
    {
        game.run.shieldDashShakeCooldownTimer -= deltaTime;
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
    case GameState::BESTIARY:
        return updateBestiary(game);
    case GameState::SANDBOX_MENU:
        updateSandboxMenuInput(game);
        break;
    case GameState::ENDING:
        return updateEnding(game, deltaTime);
    }

    return false;
}
