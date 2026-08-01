#pragma once

#include "achievements.hpp"
#include "entities/boss.hpp"
#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "entities/player.hpp"
#include "entities/space.hpp"
#include "highscore.hpp"
#include "raylib.h"
#include "rendertarget.hpp"
#include "settings.hpp"
#include "sound.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
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
    SHIP_SELECT,
    GAMEPLAY,
    PAUSED,
    GAME_OVER,
    LEVEL_UP,
    SETTINGS,
    ACHIEVEMENTS
};

// Engine/asset-level resources: window & render setup, loaded audio/fonts, persisted settings
// and high scores. Not touched by resetRun() — these outlive any single run.
struct GameResources
{
    int screenWidth{};
    int screenHeight{};
    int windowWidth{};
    int windowHeight{};
    float renderScale{1.0F};
    RenderTarget worldTarget;
    RenderTarget pixelTarget;
    Font font;
    Sounds sounds;
    BgmLayers bgm;
    Settings settings;
    std::vector<int32_t> highScores;
    std::shared_ptr<highscore::Repository> highScoreRepo;
    Achievements achievements;
};

// Per-run gameplay state: entities, timers, and score. Fully reinitialized by resetRun() at the
// start of every run.
struct GameplayState
{
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
    std::vector<OrbitBladeProjectile> orbitBladeProjectiles;
    std::vector<OrbitBladeProjectile> nerveBallProjectiles;
    std::vector<NerveSpiralProjectile> nerveSpiralProjectiles;
    std::vector<ElementalField> elementalFields;
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
    std::vector<Particle> dashTrailParticles;
    std::vector<DamageNumber> damageNumbers;
    std::optional<WeaponDowngrade> weaponDowngrade;
    std::vector<FollowerDrone> followerDrones;
    std::vector<LaserDrone> laserDrones;
    std::vector<Turret> turrets;
    std::vector<ChainLightningBolt> chainLightningBolts;
    float asteroidSpawnTimer;
    float enemySpawnTimer;
    int xp;
    int level;
    int xpToNext;
    int waveNumber;
    float waveTimer;
    int bossSpawnCount;
    float runTime;
    int score;
    bool scoreRecorded;
    float shakeTimer;
    float shakeDuration;
    float shakeIntensity;
    float hitPauseTimer;
    float nerveBurstFlashTimer;
    Vector2 nerveBurstFlashEnd;
    std::string achievementToast;
    float achievementToastTimer;
};

// Move-only: copy would double-free the raylib GPU/audio handles held in `resources`
// (RenderTexture2D, Font, Music, Sound). Move stays defaulted (bitwise) rather than deleted because
// InitGame() relies on NRVO to return Game by value; the move is never actually invoked at runtime.
// Every field is still value-initialized via `Game game{};` (this ctor is defaulted, not
// user-provided, so zero-init still runs first) — the missing-member-init check below is a false
// positive. NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct Game
{
    Game() = default;
    Game(const Game&) = delete;
    auto operator=(const Game&) -> Game& = delete;
    Game(Game&&) = default;
    auto operator=(Game&&) -> Game& = default;
    ~Game() = default;

    GameState state{};
    int menuIndex;
    GameState settingsReturnState;
    bool sandbox;
    int sandboxKindIndex;
    bool sandboxDeathEnabled;
    int sandboxBossAttackIndex;
    int sandboxPickupIndex;

    GameResources resources;
    GameplayState run;
};

auto InitGame() -> Game;

void resetRun(Game& game);

void toggleFullscreen(Game& game);

void syncScreenSize(Game& game);
