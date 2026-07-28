#pragma once

#include "entities/boss.hpp"
#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "entities/player.hpp"
#include "entities/space.hpp"
#include "highscore.hpp"
#include "raylib.h"
#include "settings.hpp"
#include "sound.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace GameConstants
{
const float arenaHalf = 20000;
const float waveDuration = 25;
const std::string highScoreFile = "highscore.txt";

constexpr int pixelScale = 1;

constexpr int defaultWindowWidth = 1920;
constexpr int defaultWindowHeight = 1080;

}

enum class GameState : std::uint8_t
{
    TITLE,
    GAMEPLAY,
    PAUSED,
    GAME_OVER,
    LEVEL_UP,
    SETTINGS
};

struct Game
{
    int screenWidth{};
    int screenHeight{};
    int windowWidth{};
    int windowHeight{};
    GameState state{};
    Player player;
    std::vector<Boss> bosses;
    std::vector<Bullet> bullets;
    std::vector<Asteroid> asteroids;
    std::vector<BossProjectile> bossProjectiles;
    std::vector<Enemy> enemies;
    std::vector<EliteHazard> eliteHazards;
    float eliteHazardSpawnTimer;
    std::vector<Pickup> pickups;
    std::vector<Mine> mines;
    std::vector<Weapon> weapons;
    std::array<int, static_cast<size_t>(SkillType::Count)> skillLevels;
    std::vector<LevelUpChoice> pendingChoices;
    int postCapDamageLevels;
    DamageMeter damageMeter;
    DamageMeter damageMeterDisplay;
    float damageMeterHoldTimer;
    std::vector<BossDeathShockwave> bossDeathShockwaves;
    BlackHole blackhole;
    Wormhole wormhole;
    std::vector<Star> stars;
    std::vector<BgParticle> bgParticles;
    std::vector<Star> borderStars;
    std::vector<GasCloud> gasClouds;
    std::vector<Particle> deathParticles;
    float asteroidSpawnTimer;
    float enemySpawnTimer;
    int spreadWindupShots;
    int xp;
    int level;
    int xpToNext;
    int waveNumber;
    float waveTimer;
    int bossSpawnCount;
    float runTime;
    int score;
    std::vector<int32_t> highScores;
    std::shared_ptr<highscore::Repository> highScoreRepo;
    int menuIndex;
    bool scoreRecorded;
    Sounds sounds;
    float shakeTimer;
    float shakeDuration;
    float shakeIntensity;
    float hitPauseTimer;
    RenderTexture2D worldTarget;
    RenderTexture2D pixelTarget;
    Font font;
    Settings settings;
    GameState settingsReturnState;
    BgmLayers bgm;
    bool sandbox;
    int sandboxKindIndex;
    bool sandboxDeathEnabled;
    int sandboxBossAttackIndex;
};

auto InitGame() -> Game;

void resetRun(Game& game);

void toggleFullscreen(Game& game);

void syncScreenSize(Game& game);
