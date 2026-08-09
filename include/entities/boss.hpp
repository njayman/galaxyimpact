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

// Heat-meter phases for the Slagmaw (Solar Forge) - drives which reflavored generic BossAttack
// is in the moveset, in fixed order, instead of the usual random-from-moveset pick every other
// boss uses. See updateSlagmawHeat.
enum class SlagmawPhase : std::uint8_t
{
    Ember,
    Flare,
    Vent
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

    Count
};

constexpr int bossAttackCount = static_cast<int>(BossAttack::Count);

constexpr std::array<std::string_view, bossAttackCount> bossAttackNames{
    "Beam",          "Spread",         "Slam",     "WormholeBeam", "ChargeDash",
    "SummonAdds",    "ShockwaveStomp", "Barrage",  "GravityWell",  "HomingBarrage",
    "PlateHurl",     "BurrowCharge",   "CoilClamp"};

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
    bool isMega;
    bool isSwarm;
    float strafePhase;
    float hitFlashTimer;
    BossShape shape;
    bool orbitContact;
    float orbitDamageAccum;
    bool beamContact;
    float beamDamageAccum;

    float recoveryTimer{};

    // Built-in anti-camping punish, every boss has this regardless of moveset (see
    // updateBossMovement / ShockwaveStomp): accumulates while the player sits within stomp
    // range and the boss is IDLE, resets otherwise, forces a ShockwaveStomp once it crosses
    // bossMeleeStompTriggerDuration.
    float meleeStompTimer{};

    bool isFinalBoss = false;

    bool isBeltbreaker = false;
    int32_t plateCount = 0;
    float plateAngleDeg = 0;
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

    float plateExcursionTimer = 0;

    bool isWreckwormHead = false;
    int32_t segmentCount = 0;

    float wreckwormSpeedMult = 1.0F;

    std::vector<float> moltThresholds;

    float coilClampGapDeg = 0;

    bool isWreckwormSegment = false;
    int32_t segmentOwnerId = -1;
    int32_t segmentIndex = 0;

    bool isArmorSegment = false;
    float segmentVolleyTimer = 0;

    bool isSlagmaw = false;
    float heatMeter = 0;
    float heatFillRate = 0;
    SlagmawPhase slagmawPhase = SlagmawPhase::Ember;
    bool slagmawHotRoundUnlocked = false;
    bool slagmawVentPending = false;

    bool debuffStatic = false;
    bool debuffFreeze = false;
    bool debuffConfuse = false;
    float burnDps = 0;
    float burnDamageAccum = 0;
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
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool homing;
    bool active;
    bool fromPlayer;

    int damage;
    int health;

    bool huntingNewTarget = false;

    int32_t ricochetRemaining = 0;
};

class BossDeathShockwave
{
  public:
    float timer;
    Vector2 position;
    bool hit;
};
