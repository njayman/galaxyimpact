#pragma once

#include "achievements.hpp"
#include "bestiary.hpp"
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
    ACHIEVEMENTS,
    BESTIARY,
    SANDBOX_MENU,
    ENDING
};

// A normal virtual stick: springs back to center on release.
struct TouchStick
{
    int32_t touchId = -1;
    Vector2 offset{};   // live offset from the fixed anchor, |offset| <= radius
    Vector2 axis{};     // offset scaled to [-1,1], deadzone applied and rescaled
};

// A simple circular press/hold button.
struct TouchButton
{
    int32_t touchId = -1;
    bool down = false;
    bool pressed = false;
};

struct TouchControls
{
    bool hasEverTouched = false;
    bool visible = false;

    TouchStick moveStick;
    TouchStick aimStick;
    Vector2 aimDir{.x = 0, .y = -1};

    TouchButton dash;
    TouchButton shield;
    TouchButton combo; // dash + shield together, dedicated button
    TouchButton nerve;
    TouchButton pause;
};

// raylib aliases touch to MOUSE_BUTTON_LEFT/GetMousePosition() outside
// gameplay (see TouchControls), which is what makes tapping menu buttons
// already work on touch for free. Menu hover previews (tooltips) don't have
// a touch equivalent though, since touch has no "point without pressing"
// gesture - this tracks press duration/drift so hover-driven UI can treat a
// held press (tap-and-hold) as hover, while a quick tap still acts as a
// click. Works for desktop mouse too: a plain mouse hover (button up) always
// counts, unchanged from before.
struct PointerHold
{
    Vector2 position{};
    Vector2 pressStartPos{};
    float pressStartTime = 0;
    bool isHold = false;
    bool releasedAsTap = false;
};

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
    Bestiary bestiary;
};

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
    std::vector<Weapon> weapons;
    std::array<int, static_cast<size_t>(SkillType::Count)> skillLevels;
    std::vector<LevelUpChoice> pendingChoices;
    int postCapDamageLevels;
    DamageMeter damageMeter;
    DamageMeter damageMeterDisplay;
    float damageMeterHoldTimer;
    std::vector<BossDeathShockwave> bossDeathShockwaves;
    std::vector<GasHazard> gasHazards;
    BlackHole blackhole;
    std::vector<BlackHoleDustParticle> blackHoleDust;
    Wormhole wormhole;
    std::vector<Star> stars;
    std::vector<BgParticle> bgParticles;
    std::vector<Star> borderStars;
    std::vector<GasCloud> gasClouds;
    std::vector<Particle> deathParticles;
    std::vector<Particle> dashTrailParticles;
    std::vector<Particle> flameParticles;
    std::vector<DamageNumber> damageNumbers;
    std::optional<WeaponDowngrade> weaponDowngrade;
    std::vector<FollowerDrone> followerDrones;
    std::vector<LaserDrone> laserDrones;
    std::vector<Turret> turrets;
    std::vector<ChainLightningBolt> chainLightningBolts;
    std::vector<ChainLightningBolt> sniperShots;
    float asteroidSpawnTimer;
    float enemySpawnTimer;
    float solarForgeHeatMeter;
    float solarForgeHeatTickTimer;
    float organicMergeTimer;
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
    float punctumThunderTimer;
    float punctumThunderFlashTimer;
    Vector2 punctumThunderBoltOrigin;
    float punctumThunderBoltSeed;
    float shieldDashShakeCooldownTimer;
    bool punctumTrapActive;
    bool inBanishedRealm;

    int32_t bossCutscenePhase;
    float bossCutsceneTimer;
    Vector2 krakenFleePos;

    float endingTimer;

};

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
    bool sandboxNaturalSpawnEnabled;
    bool sandboxKillBoxArmed;
    float sandboxKillBoxTimer;

    bool sandboxBrowserMode;
    int browserCategoryIndex;
    int browserBossTypeIndex;
    int browserWeaponIndex;
    int browserShipIndex;
    int browserParticleIndex;

    GameResources resources;
    GameplayState run;
    TouchControls touch;
    PointerHold pointerHold;
};

auto InitGame() -> Game;

void resetRun(Game& game);

void toggleFullscreen(Game& game);

void syncScreenSize(Game& game);
