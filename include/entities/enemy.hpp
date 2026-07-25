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
    bool telegraphing; // Charge pattern only: winding up, direction locked but not yet moving
    bool phased;
    float orbitAngle;
    float orbitDist;
    bool isElite;
    bool hitByDash;
};

// EliteHazard is a rare, tough field hazard distinct from the regular
// enemyKinds roster: it doesn't chase, it holds a position near the edge of
// the player's screen and either buffs nearby enemies (Warlord) or debuffs
// the player (Suppressor) for as long as it's alive.
enum class EliteHazardRole : std::uint8_t
{
    Warlord,
    Suppressor
};

namespace EliteHazardConstants
{
constexpr float radius = 26;
constexpr float auraRadius = 220;
} // namespace EliteHazardConstants

class EliteHazard
{
  public:
    Vector2 position;
    float angle; // orbit angle around the player, for the screen-edge-follow drift
    EliteHazardRole role;
    int health;
    int maxHealth;
    bool active;
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
    int minWave;
    bool splitsOnDeath = false;
    int splitKind = 0;
    int splitCount = 0;
    bool explodesOnDeath = false;
    int explodeDamage = 0;
    float explodeRadius = 0;
    bool isLeech = false;
    bool phaseCycle = false;
    float fireInterval = 0;    // PatternTurret only
    float projectileSpeed = 0; // PatternTurret only
    int spawnKind = 0;         // PatternSpawner only
    int spawnCount = 0;        // PatternSpawner only
    float spawnInterval = 0;   // PatternSpawner only
};

// Index into enemyKinds, referenced by splitKind/spawnKind below.
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
              .minWave = 1},
    EnemyKind{.name = "Swarmling",
              .radius = 8,
              .health = 4,
              .speed = 2.2,
              .contactDamage = 1,
              .score = 3,
              .pattern = EnemyPattern::Chase,
              .color = Palette::Haze,
              .minWave = 1},
    EnemyKind{.name = "Brute",
              .radius = 22,
              .health = 40,
              .speed = 0.6,
              .contactDamage = 3,
              .score = 15,
              .pattern = EnemyPattern::Chase,
              .color = Palette::StructDark,
              .minWave = 2},
    EnemyKind{.name = "Zigzagger",
              .radius = 12,
              .health = 12,
              .speed = 1.4,
              .contactDamage = 1,
              .score = 8,
              .pattern = EnemyPattern::Zigzag,
              .color = Palette::BossSpread,
              .minWave = 2},
    EnemyKind{.name = "Charger",
              .radius = 14,
              .health = 14,
              .speed = 0.8,
              .contactDamage = 2,
              .score = 10,
              .pattern = EnemyPattern::Charge,
              .color = Palette::AccentDim,
              .minWave = 3},
    EnemyKind{.name = "Orbiter",
              .radius = 12,
              .health = 10,
              .speed = 1.2,
              .contactDamage = 1,
              .score = 8,
              .pattern = EnemyPattern::Orbit,
              .color = Palette::Shield,
              .minWave = 3},
    EnemyKind{.name = "Splitter",
              .radius = 16,
              .health = 16,
              .speed = 1.0,
              .contactDamage = 1,
              .score = 10,
              .pattern = EnemyPattern::Chase,
              .color = Palette::StructMid,
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
              .minWave = 4},
    EnemyKind{.name = "Bomber",
              .radius = 14,
              .health = 8,
              .speed = 1.1,
              .contactDamage = 0,
              .score = 12,
              .pattern = EnemyPattern::Chase,
              .color = Palette::Accent,
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
              .minWave = 3},
    EnemyKind{.name = "Laser Fence",
              .radius = 26,
              .health = 30,
              .speed = 0,
              .contactDamage = 3,
              .score = 15,
              .pattern = EnemyPattern::Stationary,
              .color = Palette::Accent,
              .minWave = 6},
    EnemyKind{.name = "Void Rift",
              .radius = 22,
              .health = 25,
              .speed = 0,
              .contactDamage = 1,
              .score = 20,
              .pattern = EnemyPattern::Spawner,
              .color = Palette::BossIdle,
              .minWave = 7,
              .spawnKind = enemyKindSwarmling,
              .spawnCount = 3,
              .spawnInterval = 4.0},
};
