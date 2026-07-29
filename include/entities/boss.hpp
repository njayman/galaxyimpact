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
    HexPlated
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
    MineDrop,
    ChargeDash,
    SummonAdds,
    ShockwaveStomp,
    Barrage,
    GravityWell,
    HomingBarrage
};

constexpr int bossAttackCount = 11;

constexpr std::array<std::string_view, bossAttackCount> bossAttackNames{
    "Beam",       "Spread",         "Slam",    "WormholeBeam", "MineDrop",     "ChargeDash",
    "SummonAdds", "ShockwaveStomp", "Barrage", "GravityWell",  "HomingBarrage"};

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
    float beamRotation;
    bool slamHit;
    bool beamShieldLatched;
    Vector2 wormholeBeamOrigin;
    Vector2 chargeVelocity;
    float barrageTimer;
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
};

class BossDeathShockwave
{
  public:
    float timer;
    Vector2 position;
    bool hit;
};
