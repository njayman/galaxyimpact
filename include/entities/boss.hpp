#pragma once

#include "palette.hpp"
#include "raylib.h"
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace bossConstants
{
constexpr float maxSlamRadius = 1000;
}

enum class BossShape : std::uint8_t
{
    Saucer,
    SpikedRing,
    TwinDome,
    Crystal,
    HexPlated,

    Segment
};

enum class BossState : std::uint8_t
{
    IDLE,
    WINDING_UP,
    SHOOTING
};

enum class BossAttack : std::uint8_t
{
    Beam,
    Spread,
    Slam,
    WormholeBeam,
    ChargeDash,
    SummonAdds,
    ShockwaveStomp,
    Barrage,
    GravityWell,
    HomingBarrage,

    PlateHurl,

    BurrowCharge,

    CoilClamp,

    MeteorHell,

    MeteorSwarm,

    DashRush,
    CloudBreath,
    TailWrap,

    Count
};

constexpr int bossAttackCount = static_cast<int>(BossAttack::Count);

constexpr std::array<std::string_view, bossAttackCount> bossAttackNames{
    "Beam",          "Spread",         "Slam",       "WormholeBeam", "ChargeDash",
    "SummonAdds",    "ShockwaveStomp", "Barrage",    "GravityWell",  "HomingBarrage",
    "PlateHurl",     "BurrowCharge",   "CoilClamp",  "MeteorHell",   "MeteorSwarm",
    "DashRush",      "CloudBreath",    "TailWrap"};

// M12 adaptive director (generic bosses only): the boss mixes these at runtime, weighted toward
// whatever counters the player's current habit. See src/entities/boss_director.cpp.
enum class BossPattern : std::uint8_t
{
    HoverShoot,
    DashChain,
    Ambush,
    Pressure,
    Zone,
    Feint,

    StrafeRun, // Saucer "Hunter" signature
    BladeToss, // SpikedRing "Grinder" signature
    BladeCage, // SpikedRing "Grinder", phase 2+

    Count
};

constexpr int bossPatternCount = static_cast<int>(BossPattern::Count);

constexpr std::array<std::string_view, bossPatternCount> bossPatternNames{
    "HoverShoot", "DashChain", "Ambush", "Pressure", "Zone", "Feint", "StrafeRun",
    "BladeToss",  "BladeCage"};

enum class Tactic : std::uint8_t
{
    Kite,
    Circle,
    Camp,
    DashSpam,
    Turtle,

    Count
};

constexpr int tacticCount = static_cast<int>(Tactic::Count);

constexpr std::array<std::string_view, tacticCount> tacticNames{"Kite", "Circle", "Camp",
                                                                "DashSpam", "Turtle"};

// Rolling score (0..1, ~8s window) of what the player keeps doing against the nearest director
// boss. Shared by all bosses, since they all face the same player.
struct TacticTracker
{
    std::array<float, tacticCount> score{};
    float angularEma = 0;
    float prevAngle = 0;
    Vector2 prevPosition{};
    Vector2 playerVelocity{};
    bool prevDashing = false;
    bool primed = false;
};

class Boss
{
  public:
    Vector2 position;
    Vector2 size;
    Color color;
    Color baseColor;
    int health;
    int maxHealth;
    BossState state;
    BossAttack attack;
    std::vector<BossAttack> moveset;
    float attackTimer;
    float stateTimer;
    Vector2 targetPosition;
    bool slamHit;
    bool beamShieldLatched;
    Vector2 wormholeBeamOrigin;
    Vector2 chargeVelocity;
    float barrageTimer;
    int spreadWindupShots;
    bool hitByDash;
    bool dashHitPlayer = false;
    bool isMega;
    bool isSwarm;
    float strafePhase;
    float hitFlashTimer;
    BossShape shape;
    int32_t typeIndex = -1;
    bool orbitContact;
    float orbitDamageAccum;
    bool beamContact;
    float beamDamageAccum;

    float recoveryTimer{};

    float meleeStompTimer{};

    bool isFinalBoss = false;

    bool isBeltbreaker = false;
    int32_t plateCount = 0;
    float plateAngleDeg = 0;
    float plateRadiusMult = 1.0F; // Ring Crush contracts and flings the plate ring
    float plateRotationSpeed = 0;
    bool beltbreakerShielded = false;
    float beltbreakerShieldTimer = 0;
    float shieldGenProgress = 0;
    bool beltbreakerReturnTriggered = false;

    int32_t instanceId = -1;

    float harassTimer = 0;

    bool isBeltbreakerPlate = false;
    int32_t plateOwnerId = -1;
    int32_t plateSlotIndex = 0;
    bool plateAttached = true;
    bool plateReturning = false;
    // Plate rush: 0 normal, 1 bursting out, 2 parked off-screen, 3 incoming (prompted),
    // 4 missed - live dash, 5 answered - flying back off-screen.
    int32_t plateRushPhase = 0;
    float plateRushDelay = 0;
    Vector2 plateRushPromptDirection{}; // plate's travel direction while incoming

    float plateExcursionTimer = 0;

    bool isWreckwormHead = false;
    int32_t segmentCount = 0;

    float wreckwormSpeedMult = 1.0F;

    std::vector<Vector2> wreckwormTrail;
    bool wreckwormRepositioning = false;
    bool wreckwormDetached = false;
    Vector2 wreckwormPrevPosition{};
    Vector2 wreckwormVelocity{};

    std::vector<float> moltThresholds;

    float coilClampGapDeg = 0;

    bool isWreckwormSegment = false;
    int32_t segmentOwnerId = -1;
    int32_t segmentIndex = 0;

    bool isArmorSegment = false;
    float segmentVolleyTimer = 0;

    bool isSlagmaw = false;

    bool isKraken = false;
    int32_t krakenEncounter = 0;
    bool retreating = false;
    float tentacleCheckTimer = 5.0F;

    static constexpr int32_t krakenLimbSlots = 4;
    std::array<int32_t, krakenLimbSlots> tentaclePhase{0, 0, 0, 0};
    std::array<float, krakenLimbSlots> tentacleStateTimer{0, 0, 0, 0};
    std::array<float, krakenLimbSlots> tentacleCooldownTimer{0, 0, 0, 0};
    std::array<Vector2, krakenLimbSlots> tentaclePortalPos{};
    std::array<float, krakenLimbSlots> tentacleSwipeStartAngle{0, 0, 0, 0};
    std::array<float, krakenLimbSlots> tentacleSwipeEndAngle{0, 0, 0, 0};
    std::array<bool, krakenLimbSlots> tentacleHitPlayerThisSwipe{};

    // ponytail: parry no longer deals instant damage; it staggers the tentacle out at its
    // body gate for a hittable window (see krakenStaggerDuration). This is Kraken's only
    // damage source now.
    std::array<bool, krakenLimbSlots> tentacleStaggered{};
    std::array<float, krakenLimbSlots> tentacleStaggerTimer{0, 0, 0, 0};
    std::array<Vector2, krakenLimbSlots> tentacleStaggerPos{};
    float krakenCoreExposedTimer = 0;
    bool krakenCoreExposureUsed = false;

    std::array<int32_t, krakenLimbSlots> limbPod{-1, -1, -1, -1};
    std::array<int32_t, krakenLimbSlots> limbPhase{0, 0, 0, 0};
    std::array<float, krakenLimbSlots> limbTimer{0, 0, 0, 0};
    std::array<float, krakenLimbSlots> limbCooldown{0, 0, 0, 0};
    std::array<float, krakenLimbSlots> limbGrabTimer{0, 0, 0, 0};
    std::array<Vector2, krakenLimbSlots> limbGrabTarget{};
    std::array<Vector2, krakenLimbSlots> limbPortalPos{};
    std::array<float, krakenLimbSlots> limbGrabAnimTimer{};
    std::array<int32_t, krakenLimbSlots> limbGrabbedKind{-1, -1, -1, -1};
    std::array<int32_t, krakenLimbSlots> limbThrowPhase{0, 0, 0, 0};
    std::array<float, krakenLimbSlots> limbThrowTimer{0, 0, 0, 0};

    float krakenSummonTimer = 0;
    bool krakenSnakeSummoned = false;

    bool krakenSnakeVariant = false;

    float ghostFadeAlpha = 1.0F;

    bool debuffStatic = false;
    bool debuffFreeze = false;
    float burnDps = 0;
    float burnDamageAccum = 0;

    float shockwaveSlowTimer = 0;
    float shockwaveSlowMult = 1.0F;

    float deathAnimTimer = -1.0F;

    bool slagmawBreaksIntoDrifters = false;
    bool slagmawBreaksIntoHalves = false;
    int32_t slagmawBreakGeneration = 0;

    float parryStunTimer = 0;

    bool isBanished = false;
    static constexpr int32_t banishedTentacleCount = 12;
    std::array<bool, banishedTentacleCount> banishedStaggered{};
    std::array<int32_t, banishedTentacleCount> banishedPhase{};
    std::array<float, banishedTentacleCount> banishedTimer{};
    std::array<float, banishedTentacleCount> banishedCooldown{};
    std::array<bool, banishedTentacleCount> banishedIsFar{};
    std::array<bool, banishedTentacleCount> banishedGrabbing{};
    std::array<Vector2, banishedTentacleCount> banishedAnchor{};
    std::array<Vector2, banishedTentacleCount> banishedTip{};
    std::array<float, banishedTentacleCount> banishedTentacleHitFlash{};
    std::array<int32_t, banishedTentacleCount> banishedAttackKind{};
    std::array<float, banishedTentacleCount> banishedThrashStartAngle{};
    std::array<float, banishedTentacleCount> banishedThrashEndAngle{};

    int32_t banishedStage = 0;
    bool banishedEyeCharging = false;
    float banishedEyeTimer = 0;
    bool banishedEyeChargeBurstUsed = false;
    Vector2 banishedEyePos{};

    bool banishedDefeated = false;

    bool usesDirector = false;
    BossPattern pattern = BossPattern::HoverShoot;
    float patternTimer = 0;
    int32_t patternStep = 0;
    bool patternLead = false;
    bool awaitingAttack = false;
    bool chainConnected = false;
    float dazedTimer = 0;
    int32_t phase = 0;
    float phaseInvulnTimer = 0;
    float readTellTimer = 0;

    int32_t plateRushAttacksLeft = 0;
    float beltbreakerPunishTimer = 0; // Punish Window after a clean Plate Rush
    float plateRushLastAngle = 0;

    // StrafeRun: the run line (start, direction, which side of the player) and the P2 decoy.
    Vector2 runStart{};
    Vector2 runDir{};
    Vector2 runSide{};
    Vector2 runEnd{};
    Vector2 runMirrorShift{}; // decoy line = run line + this
    float runTravel = 0;
    int32_t runsLeft = 0;
    bool decoyActive = false;
    Vector2 decoyCenter{};
};

struct BossType
{
    std::string_view name;
    Color color;
    float healthMult;
    float sizeMult;
    BossShape shape;
};

constexpr std::array<BossType, 20> bossTypes{
    BossType{.name = "Warbringer",
             .color = Palette::BossIdle,
             .healthMult = 1.0F,
             .sizeMult = 1.0F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "Ashen Sentinel",
             .color = Palette::StructDark,
             .healthMult = 1.1F,
             .sizeMult = 1.05F,
             .shape = BossShape::HexPlated},
    BossType{.name = "Void Herald",
             .color = Palette::Void,
             .healthMult = 0.9F,
             .sizeMult = 0.95F,
             .shape = BossShape::Crystal},
    BossType{.name = "Crimson Warden",
             .color = Palette::Accent,
             .healthMult = 1.05F,
             .sizeMult = 1.0F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "Dim Reaver",
             .color = Palette::AccentDim,
             .healthMult = 1.15F,
             .sizeMult = 1.1F,
             .shape = BossShape::Crystal},
    BossType{.name = "Static Wraith",
             .color = Palette::Haze,
             .healthMult = 0.85F,
             .sizeMult = 0.9F,
             .shape = BossShape::Crystal},
    BossType{.name = "Barrier Colossus",
             .color = Palette::Shield,
             .healthMult = 1.3F,
             .sizeMult = 1.2F,
             .shape = BossShape::HexPlated},
    BossType{.name = "Ember Titan",
             .color = Palette::Charge,
             .healthMult = 1.1F,
             .sizeMult = 1.05F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "Judgment Spire",
             .color = Palette::Crit,
             .healthMult = 0.95F,
             .sizeMult = 0.95F,
             .shape = BossShape::TwinDome},
    BossType{.name = "Hollow Idol",
             .color = Palette::BossHoming,
             .healthMult = 1.0F,
             .sizeMult = 1.0F,
             .shape = BossShape::Saucer},
    BossType{.name = "Shatter Prime",
             .color = Palette::BossSpread,
             .healthMult = 1.05F,
             .sizeMult = 1.0F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "Beamforge",
             .color = Palette::BossBeam,
             .healthMult = 1.0F,
             .sizeMult = 1.0F,
             .shape = BossShape::HexPlated},
    BossType{.name = "Structural Wyrm",
             .color = Palette::StructMid,
             .healthMult = 1.2F,
             .sizeMult = 1.15F,
             .shape = BossShape::HexPlated},
    BossType{.name = "Lightbound Custodian",
             .color = Palette::StructLight,
             .healthMult = 0.9F,
             .sizeMult = 0.95F,
             .shape = BossShape::TwinDome},
    BossType{.name = "Nullpoint Marauder",
             .color = Palette::Void,
             .healthMult = 1.1F,
             .sizeMult = 1.0F,
             .shape = BossShape::Crystal},
    BossType{.name = "Cinder Duke",
             .color = Palette::Accent,
             .healthMult = 0.95F,
             .sizeMult = 1.0F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "Rimehollow",
             .color = Palette::Shield,
             .healthMult = 1.0F,
             .sizeMult = 1.05F,
             .shape = BossShape::Crystal},
    BossType{.name = "Fracture Queen",
             .color = Palette::AccentDim,
             .healthMult = 1.2F,
             .sizeMult = 1.1F,
             .shape = BossShape::TwinDome},
    BossType{.name = "Stormcaller",
             .color = Palette::BossSpread,
             .healthMult = 0.9F,
             .sizeMult = 0.9F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "The Last Signal",
             .color = Palette::Crit,
             .healthMult = 1.35F,
             .sizeMult = 1.25F,
             .shape = BossShape::TwinDome},
};

class BossProjectile
{
  public:
    float life = 0; // > 0: expires after this many seconds; 0: lives until out of range
    Vector2 accel{}; // added to velocity each second (boomerang blades)
    bool isBlade = false;
    // Cage blades orbit a fixed center, tightening, then fling outward when orbitTime runs out.
    Vector2 orbitCenter{};
    float orbitRadius = 0;
    float orbitAngle = 0;
    float orbitSpin = 0;
    float orbitShrink = 0;
    float orbitTime = 0;
    float orbitFling = 0;
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool homing;
    bool active;
    bool fromPlayer;

    int damage;
    int health;

    bool huntingNewTarget = false;

    bool isMeteor = false;
    bool isShard = false; // Beltbreaker Rubble Barrage stone

    bool isFluid = false;
    Color fluidColor = Palette::RustbloomAccent;

    int32_t ricochetRemaining = 0;
    uint32_t ricochetLastHit = 0; // Enemy::id bounced off last; never re-hit in a row

    int32_t visualEnemyKind = -1;
};

class BossDeathShockwave
{
  public:
    float timer;
    Vector2 position;
    bool hit;
};

class GasHazard
{
  public:
    Vector2 position{};
    float radius = 0;
    float seed = 0;
    float life = 0;
    bool active = true;
    bool isFire = false;
    bool anchored = false; // director Zone walls hold position instead of drifting at the player
};
