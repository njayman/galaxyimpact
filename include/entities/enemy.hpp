#pragma once

#include "palette.hpp"
#include "raylib.h"
#include <array>
#include <cstdint>
#include <string_view>

enum class EnemyPattern : std::uint8_t
{
    Chase,
    Zigzag,
    Charge,
    Orbit,
    Turret,
    Spawner,
    Stationary
};

enum class EnemyShape : std::uint8_t
{
    Circle,
    Triangle,
    Square,
    Diamond,
    Pentagon,
    Hexagon,
    Octagon,
    Ring,
    Star
};

const float eliteChance = 0.06;

class Enemy
{
  public:
    int kind;
    Vector2 position;
    Vector2 velocity;
    int health;
    bool active;
    float stateTimer;
    bool charging;
    bool telegraphing;
    bool phased;
    float orbitAngle;
    float orbitDist;
    bool isElite;
    bool hitByDash;
    float hitFlashTimer;
    bool orbitContact;
    float orbitDamageAccum;
    bool beamContact;
    float beamDamageAccum;
    bool debuffStatic;
    bool debuffFreeze;
    bool debuffConfuse;
    float burnDps;
    float burnDamageAccum;
    float confuseWanderTimer;
    Vector2 confuseWanderDir;
};

enum class EliteHazardRole : std::uint8_t
{
    Warlord,
    Suppressor
};

namespace EliteHazardConstants
{
constexpr float radius = 26;
constexpr float auraRadius = 220;
}

class EliteHazard
{
  public:
    Vector2 position;
    float angle;
    EliteHazardRole role;
    int health;
    int maxHealth;
    bool active;
    bool orbitContact;
    float orbitDamageAccum;
    bool beamContact;
    float beamDamageAccum;
};

struct EnemyKind
{
    std::string_view name;
    float radius;
    int health;
    float speed;
    int contactDamage;
    int score;
    EnemyPattern pattern;
    Color color;
    EnemyShape shape;
    int minWave;
    bool splitsOnDeath = false;
    int splitKind = 0;
    int splitCount = 0;
    bool explodesOnDeath = false;
    int explodeDamage = 0;
    float explodeRadius = 0;
    bool isLeech = false;
    bool phaseCycle = false;
    float fireInterval = 0;
    float projectileSpeed = 0;
    int spawnKind = 0;
    int spawnCount = 0;
    float spawnInterval = 0;
};

constexpr int enemyKindSwarmling = 1;

constexpr std::array<EnemyKind, 17> enemyKinds{
    EnemyKind{.name = "Drifter",
              .radius = 14,
              .health = 10,
              .speed = 1.0,
              .contactDamage = 1,
              .score = 5,
              .pattern = EnemyPattern::Chase,
              .color = Palette::BossIdle,
              .shape = EnemyShape::Circle,
              .minWave = 1},
    EnemyKind{.name = "Swarmling",
              .radius = 8,
              .health = 4,
              .speed = 2.2,
              .contactDamage = 1,
              .score = 3,
              .pattern = EnemyPattern::Chase,
              .color = Palette::Haze,
              .shape = EnemyShape::Triangle,
              .minWave = 1},
    EnemyKind{.name = "Brute",
              .radius = 22,
              .health = 40,
              .speed = 0.6,
              .contactDamage = 3,
              .score = 15,
              .pattern = EnemyPattern::Chase,
              .color = Palette::StructDark,
              .shape = EnemyShape::Hexagon,
              .minWave = 2},
    EnemyKind{.name = "Zigzagger",
              .radius = 12,
              .health = 12,
              .speed = 1.4,
              .contactDamage = 1,
              .score = 8,
              .pattern = EnemyPattern::Zigzag,
              .color = Palette::BossSpread,
              .shape = EnemyShape::Diamond,
              .minWave = 2},
    EnemyKind{.name = "Charger",
              .radius = 14,
              .health = 14,
              .speed = 0.8,
              .contactDamage = 2,
              .score = 10,
              .pattern = EnemyPattern::Charge,
              .color = Palette::AccentDim,
              .shape = EnemyShape::Triangle,
              .minWave = 3},
    EnemyKind{.name = "Orbiter",
              .radius = 12,
              .health = 10,
              .speed = 1.2,
              .contactDamage = 1,
              .score = 8,
              .pattern = EnemyPattern::Orbit,
              .color = Palette::Shield,
              .shape = EnemyShape::Ring,
              .minWave = 3},
    EnemyKind{.name = "Splitter",
              .radius = 16,
              .health = 16,
              .speed = 1.0,
              .contactDamage = 1,
              .score = 10,
              .pattern = EnemyPattern::Chase,
              .color = Palette::StructMid,
              .shape = EnemyShape::Square,
              .minWave = 2,
              .splitsOnDeath = true,
              .splitKind = enemyKindSwarmling,
              .splitCount = 2},
    EnemyKind{.name = "Turret",
              .radius = 16,
              .health = 20,
              .speed = 0,
              .contactDamage = 1,
              .score = 12,
              .pattern = EnemyPattern::Turret,
              .color = Palette::BossHoming,
              .shape = EnemyShape::Octagon,
              .minWave = 3,
              .fireInterval = 2.5,
              .projectileSpeed = 6},
    EnemyKind{.name = "Sniper",
              .radius = 14,
              .health = 10,
              .speed = 0,
              .contactDamage = 1,
              .score = 14,
              .pattern = EnemyPattern::Turret,
              .color = Palette::Crit,
              .shape = EnemyShape::Pentagon,
              .minWave = 5,
              .fireInterval = 3.5,
              .projectileSpeed = 6},
    EnemyKind{.name = "Shielded Drone",
              .radius = 16,
              .health = 35,
              .speed = 0.9,
              .contactDamage = 2,
              .score = 16,
              .pattern = EnemyPattern::Chase,
              .color = Palette::Haze,
              .shape = EnemyShape::Ring,
              .minWave = 4},
    EnemyKind{.name = "Bomber",
              .radius = 14,
              .health = 8,
              .speed = 1.1,
              .contactDamage = 0,
              .score = 12,
              .pattern = EnemyPattern::Chase,
              .color = Palette::Accent,
              .shape = EnemyShape::Star,
              .minWave = 4,
              .explodesOnDeath = true,
              .explodeDamage = 2,
              .explodeRadius = 50},
    EnemyKind{.name = "Leech",
              .radius = 12,
              .health = 10,
              .speed = 1.3,
              .contactDamage = 0,
              .score = 8,
              .pattern = EnemyPattern::Chase,
              .color = Palette::Charge,
              .shape = EnemyShape::Diamond,
              .minWave = 3,
              .isLeech = true},
    EnemyKind{.name = "Swarm Mother",
              .radius = 20,
              .health = 30,
              .speed = 0.4,
              .contactDamage = 1,
              .score = 20,
              .pattern = EnemyPattern::Spawner,
              .color = Palette::BossSpread,
              .shape = EnemyShape::Octagon,
              .minWave = 5,
              .spawnKind = enemyKindSwarmling,
              .spawnCount = 2,
              .spawnInterval = 3.0},
    EnemyKind{.name = "Phase Wraith",
              .radius = 14,
              .health = 18,
              .speed = 1.3,
              .contactDamage = 2,
              .score = 18,
              .pattern = EnemyPattern::Chase,
              .color = Palette::StructLight,
              .shape = EnemyShape::Ring,
              .minWave = 6,
              .phaseCycle = true},
    EnemyKind{.name = "Mine",
              .radius = 18,
              .health = 5,
              .speed = 0,
              .contactDamage = 4,
              .score = 10,
              .pattern = EnemyPattern::Stationary,
              .color = Palette::AccentDim,
              .shape = EnemyShape::Star,
              .minWave = 3},
    EnemyKind{.name = "Laser Fence",
              .radius = 26,
              .health = 30,
              .speed = 0,
              .contactDamage = 3,
              .score = 15,
              .pattern = EnemyPattern::Stationary,
              .color = Palette::Accent,
              .shape = EnemyShape::Square,
              .minWave = 6},
    EnemyKind{.name = "Void Rift",
              .radius = 22,
              .health = 25,
              .speed = 0,
              .contactDamage = 1,
              .score = 20,
              .pattern = EnemyPattern::Spawner,
              .color = Palette::BossIdle,
              .shape = EnemyShape::Ring,
              .minWave = 7,
              .spawnKind = enemyKindSwarmling,
              .spawnCount = 3,
              .spawnInterval = 4.0},
};
