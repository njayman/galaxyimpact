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
// The arena edge is a wall of gas: past wallInner the player loses wallTickDamage every wallTick.
const float wallInner = arenaHalf - 450;
const float wallTick = 0.3F;
const int32_t wallTickDamage = 1;
const float waveDuration = 25;
const std::string highScoreFile = "highscore.txt";

constexpr int pixelScale = 1;

constexpr int defaultWindowWidth = 1920;
constexpr int defaultWindowHeight = 1080;

}

enum class QtePrompt : std::uint8_t
{
    None,
    Up,
    Left,
    Right,
    Down,
    Shield,
    Dash,
    Parry, // shield + dash
    Nerve,
    Mash // nerve, repeatedly, to fill a bar
};

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

    // Dash/shield/combo are each a 3-band kite split top=shield,
    // middle=combo, bottom=dash, duplicated on both sides of the screen so
    // either thumb can reach any of the three. Combo is its own dedicated
    // touch (simultaneous two-finger presses on one capsule turned out
    // unreliable to land on separate halves), and just ORs into both
    // dash/shield being pressed, same as any dash+shield overlap.
    TouchButton leftShield;
    TouchButton leftCombo;
    TouchButton leftDash;
    TouchButton rightShield;
    TouchButton rightCombo;
    TouchButton rightDash;
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

// Vertical scroll for a screen whose content height isn't known until it's
// actually drawn (Bestiary's section count depends on window width). Draw
// writes maxOffset from what it just measured; update reads/clamps it next
// frame. A 1-frame lag on the clamp bound is imperceptible.
struct ContentScroll
{
    float offset = 0;
    float maxOffset = 0;
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

// Rustbloom's Biome Hazard: a ground patch that blooms where something died and slowly spreads.
struct RustPatch
{
    Vector2 position;
    float radius;
    float age;      // harmless outline until rustPatchRipenTime
    float budTimer; // counts down while full grown, then buds a neighbour
    float seed;
    bool active;
};

struct GameplayState
{
    Player player;
    std::vector<Boss> bosses;
    std::vector<Bullet> bullets;
    std::vector<Asteroid> asteroids;
    std::vector<BossProjectile> bossProjectiles;
    std::vector<Enemy> enemies;
    uint32_t nextEnemyId = 0;
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
    TacticTracker tactics;
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
    int32_t sniperEventPhase = 0; // 0 idle, 1 dodge tell, 2 live line, 3 counter window
    float sniperEventTimer = 0;
    float sniperEventCooldown = 0;
    int32_t sniperEventEnemyId = -1;
    Vector2 sniperEventStart{};
    Vector2 sniperEventEnd{};
    Vector2 sniperEventPlayerStart{};
    // Boss QTE cutscene (see qte.cpp): while qteBossId >= 0 the player only answers prompts.
    int32_t qteBossId = -1;
    int32_t qteEnemyId = -1; // a Sniper's shot QTE (no 20 s gap, see sniperQteGap)
    QtePrompt qtePrompt = QtePrompt::None;
    float qteTimer = 0;    // time left on the current prompt / beat
    float qteTimerMax = 0; // full length of the current prompt, for the HUD timer
    float qteActionTimer = 0;
    Vector2 qteActionVelocity{};
    Vector2 qteThreatDir{}; // incoming threat's travel direction, for the HUD edge marker
    int32_t qteStickDir = 0; // last touch-stick cardinal, for flick edge detection
    int32_t qteMashCount = 0;  // presses landed on the current Mash prompt
    int32_t qteMashTarget = 0; // presses that fill the Mash bar
    float qteCooldown = 0;     // minimum gap before the next Boss QTE may start
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
    std::string achievementToast;
    float achievementToastTimer;
    float punctumThunderTimer;
    float punctumThunderFlashTimer;
    Vector2 punctumThunderBoltOrigin;
    float punctumThunderBoltSeed;
    float backdropSeed; // per-run variety for biome backdrops
    float debrisSquallTimer;
    int32_t debrisSquallPhase; // 0 idle, 1 edge warning, 2 rocks sweeping
    float debrisSquallPhaseTimer;
    Vector2 debrisSquallCenter;
    float debrisSquallDir; // +1 or -1 along the dust-lane direction
    float debrisSquallFront;
    float debrisSquallEnemyTickTimer;
    uint64_t debrisSquallBrokenMask; // bit per rock shot this sweep
    std::vector<RustPatch> rustPatches;
    float rustPatchTickTimer;
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
    bool autoplay = false; // --autoplay: a bot plays the sandbox (see updateAutoplayBot)
    float autoplayTime = 0;
    int sandboxKindIndex;
    bool sandboxDeathEnabled;
    int sandboxBossAttackIndex;
    int sandboxBossPatternIndex;
    bool sandboxBossNoDeath;
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
    ContentScroll achievementsScroll;
    ContentScroll bestiaryScroll;
};

auto InitGame() -> Game;

void resetRun(Game& game);

void toggleFullscreen(Game& game);

void syncScreenSize(Game& game);
