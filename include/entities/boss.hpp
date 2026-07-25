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

enum class BossState : std::uint8_t
{
    IDLE,
    WINDING_UP,
    SHOOTING
};

// BossAttack is the shared move pool every boss (and miniboss) draws from -
// see BossType below for the "20 types, mix-and-match moves" split: a type
// is flavor only, the moveset is sampled from this pool at spawn.
enum class BossAttack : std::uint8_t
{
    Beam,
    Homing,
    Spread,
    Slam,
    WormholeBeam,
    MineDrop,
    ChargeDash,
    SummonAdds,
    ShockwaveStomp,
    Barrage,
    GravityWell
};

constexpr int bossAttackCount = 11;

constexpr std::array<std::string_view, bossAttackCount> bossAttackNames{
    "Beam",       "Homing",     "Spread",         "Slam",    "WormholeBeam", "MineDrop",
    "ChargeDash", "SummonAdds", "ShockwaveStomp", "Barrage", "GravityWell"};

class Boss
{
  public:
    Vector2 position;
    Vector2 size;
    Color color;
    Color baseColor; // the type's idle color, restored between attacks
    int health;
    int maxHealth;
    BossState state;
    BossAttack attack;
    std::vector<BossAttack> moveset; // sampled at spawn; attack is always drawn from this
    float attackTimer;
    float stateTimer;
    Vector2 targetPosition;
    float beamRotation;
    bool slamHit;
    bool beamShieldLatched;     // player was shielded while standing in the beam this attack
    Vector2 wormholeBeamOrigin; // flank point WormholeBeam fires its beam from
    Vector2 chargeVelocity;     // ChargeDash's fixed dash direction*speed for the attack
    float barrageTimer;         // Barrage's next-shot countdown
};

// BossType is flavor only (name/color/stat multipliers) - the 20 types share
// one global move pool (BossAttack) rather than each hand-authoring its own
// moveset, so new moves added to the pool enrich every type for free.
struct BossType
{
    std::string_view name;
    Color color;
    float healthMult;
    float sizeMult;
};

constexpr std::array<BossType, 20> bossTypes{
    BossType{
        .name = "Warbringer", .color = Palette::BossIdle, .healthMult = 1.0F, .sizeMult = 1.0F},
    BossType{.name = "Ashen Sentinel",
             .color = Palette::StructDark,
             .healthMult = 1.1F,
             .sizeMult = 1.05F},
    BossType{.name = "Void Herald", .color = Palette::Void, .healthMult = 0.9F, .sizeMult = 0.95F},
    BossType{
        .name = "Crimson Warden", .color = Palette::Accent, .healthMult = 1.05F, .sizeMult = 1.0F},
    BossType{
        .name = "Dim Reaver", .color = Palette::AccentDim, .healthMult = 1.15F, .sizeMult = 1.1F},
    BossType{
        .name = "Static Wraith", .color = Palette::Haze, .healthMult = 0.85F, .sizeMult = 0.9F},
    BossType{
        .name = "Barrier Colossus", .color = Palette::Shield, .healthMult = 1.3F, .sizeMult = 1.2F},
    BossType{
        .name = "Ember Titan", .color = Palette::Charge, .healthMult = 1.1F, .sizeMult = 1.05F},
    BossType{
        .name = "Judgment Spire", .color = Palette::Crit, .healthMult = 0.95F, .sizeMult = 0.95F},
    BossType{
        .name = "Hollow Idol", .color = Palette::BossHoming, .healthMult = 1.0F, .sizeMult = 1.0F},
    BossType{.name = "Shatter Prime",
             .color = Palette::BossSpread,
             .healthMult = 1.05F,
             .sizeMult = 1.0F},
    BossType{.name = "Beamforge", .color = Palette::BossBeam, .healthMult = 1.0F, .sizeMult = 1.0F},
    BossType{.name = "Structural Wyrm",
             .color = Palette::StructMid,
             .healthMult = 1.2F,
             .sizeMult = 1.15F},
    BossType{.name = "Lightbound Custodian",
             .color = Palette::StructLight,
             .healthMult = 0.9F,
             .sizeMult = 0.95F},
    BossType{
        .name = "Nullpoint Marauder", .color = Palette::Void, .healthMult = 1.1F, .sizeMult = 1.0F},
    BossType{
        .name = "Cinder Duke", .color = Palette::Accent, .healthMult = 0.95F, .sizeMult = 1.0F},
    BossType{.name = "Rimehollow", .color = Palette::Shield, .healthMult = 1.0F, .sizeMult = 1.05F},
    BossType{.name = "Fracture Queen",
             .color = Palette::AccentDim,
             .healthMult = 1.2F,
             .sizeMult = 1.1F},
    BossType{
        .name = "Stormcaller", .color = Palette::BossSpread, .healthMult = 0.9F, .sizeMult = 0.9F},
    BossType{
        .name = "The Last Signal", .color = Palette::Crit, .healthMult = 1.35F, .sizeMult = 1.25F},
};

class BossProjectile
{
  public:
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool homing;
    bool active;
    bool fromPlayer; // true for the player's own homing missiles: targets enemies, can't hurt the
                     // player who fired it
    int damage;
    int health; // hits (from player bullets) needed to shoot it down; immune to obstacles/enemies
};

// BossDeathShockwave is the expanding ring left behind by a killed boss - one
// instance per boss death, so multiple simultaneous boss kills each get their
// own independent shockwave.
class BossDeathShockwave
{
  public:
    float timer;
    Vector2 position;
    bool hit;
};
