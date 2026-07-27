#include "game.hpp"

#include "highscore.hpp"
#include "palette.hpp"
#include "platform.hpp"
#include "raymath.h"
#include "sound.hpp"
#include "update.hpp"
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

    std::vector<GasCloud> clouds;
    constexpr int clusterCount = 40;

    for (int i = 0; i < clusterCount; i++)
    {
        const float baseAngle = static_cast<float>(i) * 360.0F / clusterCount;
        const float angle = (baseAngle + static_cast<float>(GetRandomValue(-8, 8))) * DEG2RAD;
        const float dist = GameConstants::arenaHalf + static_cast<float>(GetRandomValue(-800, 800));
        const Vector2 center{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist};

        const int blobCount = GetRandomValue(3, 5);
        for (int b = 0; b < blobCount; b++)
        {
            const Vector2 offset{.x = static_cast<float>(GetRandomValue(-150, 150)),
                                 .y = static_cast<float>(GetRandomValue(-150, 150))};
            clouds.push_back(GasCloud{.position = Vector2Add(center, offset),
                                      .radius = static_cast<float>(GetRandomValue(80, 220)),
                                      .color = cloudColors.at(static_cast<size_t>(GetRandomValue(
                                          0, static_cast<int32_t>(cloudColors.size() - 1))))});
        }
    }

    return clouds;
}

auto loadReadableFont() -> Font
{
    const std::array<const char*, 5> candidates{
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
        "/Library/Fonts/Arial Bold.ttf",
        R"(C:\Windows\Fonts\arialbd.ttf)",
    };

    for (const auto* path : candidates)
    {
        if (std::filesystem::exists(path))
        {
            const Font font = LoadFontEx(path, 96, nullptr, 0);
            if (font.texture.id != 0)
            {
                SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
                return font;
            }
        }
    }

    return GetFontDefault();
}

}

auto InitGame() -> Game
{
    Game game{};

    game.screenWidth = GameConstants::defaultWindowWidth;
    game.screenHeight = GameConstants::defaultWindowHeight;
    game.windowWidth = GameConstants::defaultWindowWidth;
    game.windowHeight = GameConstants::defaultWindowHeight;
    game.state = GameState::TITLE;

    game.stars.resize(80);
    for (auto& star : game.stars)
    {
        star =
            Star{.position = Vector2{.x = static_cast<float>(GetRandomValue(0, game.screenWidth)),
                                     .y = static_cast<float>(GetRandomValue(0, game.screenHeight))},
                 .radius = static_cast<float>(GetRandomValue(10, 20)) / 10.0F};
    }

    const std::array<Color, 3> bgColors{Fade(Palette::StructMid, 0.15F), Fade(Palette::Haze, 0.1F),
                                        Fade(Palette::AccentDim, 0.08F)};
    game.bgParticles.resize(25);
    for (auto& particle : game.bgParticles)
    {
        particle = BgParticle{
            .position = Vector2{.x = static_cast<float>(GetRandomValue(0, game.screenWidth)),
                                .y = static_cast<float>(GetRandomValue(0, game.screenHeight))},
            .velocity = Vector2{.x = static_cast<float>(GetRandomValue(-4, 4)) / 10.0F,
                                .y = static_cast<float>(GetRandomValue(-4, 4)) / 10.0F},
            .radius = static_cast<float>(GetRandomValue(15, 40)) / 10.0F,
            .color = bgColors.at(
                static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(bgColors.size() - 1))))};
    }

    game.borderStars.resize(400);
    for (auto& star : game.borderStars)
    {
        star = Star{.position = Vector2{.x = static_cast<float>(GetRandomValue(0, 7680)),
                                        .y = static_cast<float>(GetRandomValue(0, 4320))},
                    .radius = static_cast<float>(GetRandomValue(10, 20)) / 10.0F};
    }

    game.gasClouds = generateBoundaryClouds();

    game.highScoreRepo = std::make_shared<highscore::FileRepository>(getSaveDataDir() +
                                                                     GameConstants::highScoreFile);
    game.highScores = game.highScoreRepo->load();

    game.sounds = LoadSounds();
    game.font = loadReadableFont();

    game.settings = loadSettings();

    game.bgm = loadBGM();
    PlayMusicStream(game.bgm.drone);
    PlayMusicStream(game.bgm.intensity);
    PlayMusicStream(game.bgm.upgrade);
    SetMusicVolume(game.bgm.drone, SoundConstants::droneVolume);
    SetMusicVolume(game.bgm.intensity, 0);
    SetMusicVolume(game.bgm.upgrade, 0);

    game.worldTarget = LoadRenderTexture(game.screenWidth / GameConstants::pixelScale,
                                         game.screenHeight / GameConstants::pixelScale);
    SetTextureFilter(game.worldTarget.texture, TEXTURE_FILTER_POINT);

    game.pixelTarget = LoadRenderTexture(game.screenWidth, game.screenHeight);

    SetTextureFilter(game.pixelTarget.texture, TEXTURE_FILTER_POINT);

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
    game.windowWidth = GetScreenWidth();
    game.windowHeight = GetScreenHeight();
}

void resetRun(Game& game)
{
    game.player = Player{.position = Vector2{},
                         .radius = 15,
                         .color = Palette::Accent,
                         .speed = 5,
                         .health = 5,
                         .maxHealth = 5,
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
                         .halfLifeOrb = false,
                         .nerve = 0};

    game.bosses.clear();

    game.bullets.clear();
    game.asteroids.clear();

    game.asteroids.reserve(static_cast<size_t>(maxAsteroid));
    game.bossProjectiles.clear();
    game.enemies.clear();

    game.enemies.reserve(static_cast<size_t>(UpdateConstants::maxEnemies));
    game.eliteHazards.clear();
    game.eliteHazardSpawnTimer = static_cast<float>(GetRandomValue(45, 90));
    game.pickups.clear();
    game.mines.clear();
    game.deathParticles.clear();
    game.weapons = {Weapon{.type = WeaponType::Forward, .level = 1}};
    game.skillLevels.fill(0);
    game.skillLevels.at(static_cast<size_t>(SkillType::ForwardShot)) = 1;
    game.postCapDamageLevels = 0;
    game.damageMeter = DamageMeter{};
    game.damageMeterDisplay = DamageMeter{};
    game.damageMeterHoldTimer = 0;
    game.bossDeathShockwaves.clear();

    game.blackhole = BlackHole{};
    game.blackhole.timer = static_cast<float>(GetRandomValue(30, 60)) / 10.0F;
    game.wormhole = Wormhole{};
    game.wormhole.timer = static_cast<float>(GetRandomValue(400, 700)) / 10.0F;
    game.asteroidSpawnTimer = 1.0F;
    game.enemySpawnTimer = 1.0F;
    game.spreadWindupShots = 0;

    game.xp = 0;
    game.level = 1;
    game.xpToNext = 100;
    game.waveNumber = 1;
    game.waveTimer = GameConstants::waveDuration;
    game.bossSpawnCount = 0;
    game.runTime = 0;

    game.shakeTimer = 0;
    game.shakeDuration = 0;
    game.shakeIntensity = 0;
    game.hitPauseTimer = 0;
    game.score = 0;
    game.scoreRecorded = false;
    game.state = GameState::GAMEPLAY;
}
