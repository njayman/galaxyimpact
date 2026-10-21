#include "game.hpp"

#include "entities/ship.hpp"
#include "highscore.hpp"
#include "palette.hpp"
#include "platform.hpp"
#include "raymath.h"
#include "sound.hpp"
#include "update.hpp"
#include "update_constants.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <memory>

namespace
{

auto generateBoundaryClouds() -> std::vector<GasCloud>
{
    const std::array<Color, 3> cloudColors{Fade(Palette::Haze, 0.12F),
                                           Fade(Palette::StructMid, 0.15F),
                                           Fade(Palette::AccentDim, 0.08F)};

    // Overlapping rings of gas balls pack into a solid wall from wallInner outward; the inner
    // ring's lumpy edge is the only boundary the player sees.
    std::vector<GasCloud> clouds;
    constexpr int32_t layerCount = 6;
    constexpr float layerGap = 200.0F;
    constexpr float ballSpacing = 170.0F;
    for (int32_t layer = 0; layer < layerCount; layer++)
    {
        const float ringRadius = GameConstants::wallInner + static_cast<float>(layer) * layerGap;
        const auto count = static_cast<int32_t>(2.0F * PI * ringRadius / ballSpacing);
        for (int32_t i = 0; i < count; i++)
        {
            const float angle = (static_cast<float>(i) + static_cast<float>(GetRandomValue(-30, 30)) / 100.0F) *
                                2.0F * PI / static_cast<float>(count);
            const float dist = ringRadius + static_cast<float>(GetRandomValue(-60, 60));
            clouds.push_back(GasCloud{
                .position = Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist},
                .radius = static_cast<float>(GetRandomValue(110, 230)),
                .color = cloudColors.at(static_cast<size_t>(
                    GetRandomValue(0, static_cast<int32_t>(cloudColors.size() - 1))))});
        }
    }

    return clouds;
}

auto loadGameFont() -> Font
{
#if defined(PLATFORM_ANDROID)
    // Android assets live inside the APK and are only reachable through
    // raylib's wrapped fopen()/AAssetManager, which takes a plain path
    // relative to the assets root - no real filesystem entry to stat first.
    const std::string path = "assets/fonts/PeaberryDoublespace.ttf";
    const Font font = LoadFontEx(path.c_str(), 96, nullptr, 0);
    if (font.texture.id != 0)
    {
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
        return font;
    }
#else
    const std::string path =
        std::string(GetApplicationDirectory()) + "assets/fonts/PeaberryDoublespace.ttf";

    if (std::filesystem::exists(path))
    {
        const Font font = LoadFontEx(path.c_str(), 96, nullptr, 0);
        if (font.texture.id != 0)
        {
            SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
            return font;
        }
    }
#endif

    return GetFontDefault();
}

auto computeRenderScale(const Game& game) -> float
{
    const float widthScale = static_cast<float>(game.resources.windowWidth) /
                             static_cast<float>(game.resources.screenWidth);
    const float heightScale = static_cast<float>(game.resources.windowHeight) /
                              static_cast<float>(game.resources.screenHeight);
    return std::min(widthScale, heightScale);
}

void createRenderTargets(Game& game)
{
    const auto width = std::max(1, static_cast<int>(static_cast<float>(game.resources.screenWidth) *
                                                    game.resources.renderScale));
    const auto height =
        std::max(1, static_cast<int>(static_cast<float>(game.resources.screenHeight) *
                                     game.resources.renderScale));

    game.resources.worldTarget.load(width, height);
    SetTextureFilter(game.resources.worldTarget.get().texture, TEXTURE_FILTER_BILINEAR);

    game.resources.pixelTarget.load(width, height);
    SetTextureFilter(game.resources.pixelTarget.get().texture, TEXTURE_FILTER_BILINEAR);
}

}

auto InitGame() -> Game
{
    Game game{};

    game.resources.screenWidth = GameConstants::defaultWindowWidth;
    game.resources.screenHeight = GameConstants::defaultWindowHeight;
    game.resources.windowWidth = GameConstants::defaultWindowWidth;
    game.resources.windowHeight = GameConstants::defaultWindowHeight;
    game.state = GameState::TITLE;

    game.run.stars.resize(80);
    for (auto& star : game.run.stars)
    {
        star = Star{
            .position =
                Vector2{.x = static_cast<float>(GetRandomValue(0, game.resources.screenWidth)),
                        .y = static_cast<float>(GetRandomValue(0, game.resources.screenHeight))},
            .radius = static_cast<float>(GetRandomValue(10, 20)) / 10.0F};
    }

    const std::array<float, 3> bgColorAlpha{0.15F, 0.1F, 0.08F};
    game.run.bgParticles.resize(25);
    for (auto& particle : game.run.bgParticles)
    {
        const auto colorIndex = static_cast<size_t>(
            GetRandomValue(0, static_cast<int32_t>(Palette::AmbientDustColors.size() - 1)));
        particle = BgParticle{
            .position =
                Vector2{.x = static_cast<float>(GetRandomValue(0, game.resources.screenWidth)),
                        .y = static_cast<float>(GetRandomValue(0, game.resources.screenHeight))},
            .velocity = Vector2{.x = static_cast<float>(GetRandomValue(-4, 4)) / 10.0F,
                                .y = static_cast<float>(GetRandomValue(-4, 4)) / 10.0F},
            .radius = static_cast<float>(GetRandomValue(15, 40)) / 10.0F,
            .color = Fade(Palette::AmbientDustColors.at(colorIndex), bgColorAlpha.at(colorIndex))};
    }

    game.run.borderStars.resize(400);
    for (auto& star : game.run.borderStars)
    {
        star = Star{.position = Vector2{.x = static_cast<float>(GetRandomValue(0, 7680)),
                                        .y = static_cast<float>(GetRandomValue(0, 4320))},
                    .radius = static_cast<float>(GetRandomValue(10, 20)) / 10.0F};
    }

    game.run.gasClouds = generateBoundaryClouds();

    game.resources.highScoreRepo = std::make_shared<highscore::FileRepository>(
        getSaveDataDir() + GameConstants::highScoreFile);
    game.resources.highScores = game.resources.highScoreRepo->load();

    game.resources.sounds = LoadSounds();
    game.resources.font = loadGameFont();

    game.resources.settings = loadSettings();
    game.resources.achievements = loadAchievements();
    game.resources.bestiary = loadBestiary();

    game.resources.bgm = loadBGM();
    PlayMusicStream(game.resources.bgm.base);
    PlayMusicStream(game.resources.bgm.intensity);
    PlayMusicStream(game.resources.bgm.miniboss);
    PlayMusicStream(game.resources.bgm.megaboss);
    PlayMusicStream(game.resources.bgm.swarmBoss);
    SetMusicVolume(game.resources.bgm.base, SoundConstants::baseVolume);
    SetMusicVolume(game.resources.bgm.intensity, 0);
    SetMusicVolume(game.resources.bgm.miniboss, 0);
    SetMusicVolume(game.resources.bgm.megaboss, 0);
    SetMusicVolume(game.resources.bgm.swarmBoss, 0);

    game.resources.renderScale = computeRenderScale(game);
    createRenderTargets(game);

    resetRun(game);
    game.state = GameState::TITLE;

    return game;
}

void toggleFullscreen(Game& game)
{
    if (IsWindowFullscreen())
    {
        ToggleFullscreen();
        SetWindowSize(GameConstants::defaultWindowWidth, GameConstants::defaultWindowHeight);
    }
    else
    {
        const int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
    }

    syncScreenSize(game);
}

void syncScreenSize(Game& game)
{
    game.resources.windowWidth = GetScreenWidth();
    game.resources.windowHeight = GetScreenHeight();

    if (const float newScale = computeRenderScale(game); newScale != game.resources.renderScale)
    {
        game.resources.renderScale = newScale;
        createRenderTargets(game);
    }
}

void resetRun(Game& game)
{
    const ShipDef& ship = currentShip(game);

    // Random start anywhere in the inner arena, so a run doesn't always begin in the same spot.
    const float spawnAngle = static_cast<float>(GetRandomValue(0, 3599)) / 10.0F * DEG2RAD;
    const float spawnDist = GameConstants::arenaHalf * 0.7F *
                            std::sqrt(static_cast<float>(GetRandomValue(0, 1000)) / 1000.0F);
    game.run.player = Player{.position = Vector2{.x = std::cos(spawnAngle) * spawnDist,
                                                 .y = std::sin(spawnAngle) * spawnDist},
                             .velocity = Vector2{},
                             .radius = ship.radius,
                             .color = ship.color,
                             .speed = ship.speed,
                             .health = ship.maxHealth,
                             .maxHealth = ship.maxHealth,
                             .shieldActive = false,
                             .shieldTimer = 0,
                             .shieldCooldownTimer = 0,
                             .immunityTimer = 0,
                             .blackHoleCoreTimer = 0,
                             .bossBodyTimer = 0,
                             .slowTimer = 0,
                             .charges = UpdateConstants::maxCharges,
                             .chargeRegenTimer = 0,
                             .dashing = false,
                             .dashTimer = 0,
                             .dashVelocity = Vector2{},
                             .shieldStacks = 0,
                             .nerve = 0,
                             .nerveCharging = false,
                             .nerveChargeTimer = 0,
                             .nerveChargeFeedTimer = 0,
                             .elementalBuffTimer = {},
                             .regenTimer = 0,
                             .regenRate = 0,
                             .overchargeTimer = 0,
                             .dashTrailTimer = 0,
                             .secondWindReady = false,
                             .overdriveTimer = 0};

    game.run.bosses.clear();

    game.run.bullets.clear();
    game.run.asteroids.clear();

    game.run.asteroids.reserve(static_cast<size_t>(maxAsteroid));
    game.run.bossProjectiles.clear();
    game.run.gasHazards.clear();
    game.run.tactics = TacticTracker{};
    game.run.enemies.clear();

    game.run.enemies.reserve(static_cast<size_t>(UpdateConstants::maxEnemies));
    game.run.eliteHazards.clear();
    game.run.eliteHazardSpawnTimer = static_cast<float>(GetRandomValue(45, 90));
    game.run.pickups.clear();
    game.run.mines.clear();
    game.run.orbitBladeProjectiles.clear();
    game.run.nerveBallProjectiles.clear();
    game.run.nerveSpiralProjectiles.clear();
    game.run.deathParticles.clear();
    game.run.dashTrailParticles.clear();
    game.run.flameParticles.clear();
    game.run.damageNumbers.clear();
    game.run.weaponDowngrade = std::nullopt;
    game.run.followerDrones.clear();
    game.run.laserDrones.clear();
    game.run.turrets.clear();
    game.run.chainLightningBolts.clear();
    game.run.sniperShots.clear();
    game.run.sniperEventPhase = 0;
    game.run.sniperEventTimer = 0;
    game.run.sniperEventCooldown = 0;
    game.run.sniperEventEnemyId = -1;
    game.run.sniperEventStart = Vector2{};
    game.run.sniperEventEnd = Vector2{};
    game.run.sniperEventPlayerStart = Vector2{};
    game.run.weapons = {Weapon{.type = ship.defaultWeapon, .level = 1}};
    game.run.skillLevels.fill(0);
    game.run.skillLevels.at(
        static_cast<size_t>(weaponGrantSkill.at(static_cast<size_t>(ship.defaultWeapon)))) = 1;
    game.run.postCapDamageLevels = 0;
    game.run.damageMeter = DamageMeter{};
    game.run.damageMeterDisplay = DamageMeter{};
    game.run.damageMeterHoldTimer = 0;
    game.run.bossDeathShockwaves.clear();

    game.run.blackhole = BlackHole{};
    game.run.blackhole.timer = static_cast<float>(GetRandomValue(30, 60)) / 10.0F;

    game.run.blackHoleDust.resize(blackHoleDustCount);
    for (auto& dust : game.run.blackHoleDust)
    {
        dust = spawnBlackHoleDustParticle(game);
        dust.radiusFrac = static_cast<float>(GetRandomValue(220, 1000)) / 1000.0F;
    }
    game.run.wormhole = Wormhole{};
    game.run.wormhole.timer = static_cast<float>(GetRandomValue(400, 700)) / 10.0F;
    game.run.asteroidSpawnTimer = 1.0F;
    game.run.enemySpawnTimer = 1.0F;
    game.run.solarForgeHeatMeter = 0;
    game.run.solarForgeHeatTickTimer = 0;
    game.run.solarForgeGelWobble = Vector2{};
    game.run.solarForgeGelWobbleVel = Vector2{};
    game.run.solarForgeGelPocket = 0;
    game.run.solarForgeInGel = false;
    game.run.player.burnDps = 0;
    game.run.player.burnDamageAccum = 0;
    game.run.player.burnRefreshTimer = 0;
    game.run.organicMergeTimer = organicMergeCheckInterval;

    game.run.xp = 0;
    game.run.level = 1;
    game.run.xpToNext = 100;
    game.run.waveNumber = 1;
    game.run.waveTimer = GameConstants::waveDuration;
    game.run.bossSpawnCount = 0;
    game.run.runTime = 0;

    game.run.shakeTimer = 0;
    game.run.shakeDuration = 0;
    game.run.shakeIntensity = 0;
    game.run.hitPauseTimer = 0;
    game.run.score = 0;
    game.run.scoreRecorded = false;
    game.run.achievementToast.clear();
    game.run.achievementToastTimer = 0;
    game.run.punctumThunderTimer = static_cast<float>(GetRandomValue(
                                       static_cast<int32_t>(punctumThunderIntervalMin * 10),
                                       static_cast<int32_t>(punctumThunderIntervalMax * 10))) /
                                   10.0F;
    game.run.punctumThunderFlashTimer = 0;
    game.run.qteBossId = -1; // a run that ended mid-cutscene must not carry it over
    game.run.qteEnemyId = -1;
    game.run.qteFinisherPhase = 0;
    game.run.qtePrompt = QtePrompt::None;
    game.run.qteCooldown = 0;
    game.run.backdropSeed = static_cast<float>(GetRandomValue(0, 9999)) * 10.0F;
    game.run.debrisSquallTimer = debrisSquallIntervalStart;
    game.run.debrisSquallPhase = 0;
    game.run.rustPatches.clear();
    game.run.rustPatchTickTimer = rustPatchEntryGrace;
    game.run.punctumTrapActive = false;
    game.run.inBanishedRealm = false;
    game.run.bossCutscenePhase = 0;
    game.run.bossCutsceneTimer = 0;
    game.run.krakenFleePos = Vector2{};
    game.run.endingTimer = 0;
    game.state = GameState::GAMEPLAY;
}
