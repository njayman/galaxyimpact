#pragma once

#include "raylib.h"
#include <cstdint>
#include <vector>

namespace SpaceConstants
{
constexpr float LargeRadius = 32.0F;
constexpr float MediumRadius = 20.0F;
constexpr float SmallRadius = 12.0F;
constexpr int LargeScore = 10;
constexpr int MediumScore = 20;
constexpr int SmallScore = 30;
}

class Star
{
  public:
    Vector2 position;
    float radius;
};

class Particle
{
  public:
    Vector2 position;
    Vector2 velocity;
    float radius;
    float life;
    float maxLife;
    Color color;
};

// M18: floating combat text — one per damage instance dealt to an enemy/boss/hazard.
class DamageNumber
{
  public:
    Vector2 position;
    int32_t amount;
    float timer;
    float maxTimer;
};

class GasCloud
{
  public:
    Vector2 position;
    float radius;
    Color color;
};

class BgParticle
{
  public:
    Vector2 position;
    Vector2 velocity;
    float radius;
    Color color;
};

class BlackHole
{
  public:
    Vector2 position;
    float radius;
    float influenceRadius;
    bool active;
    float timer;
};

enum class WormholeFacing : std::uint8_t
{
    East,
    South,
    West,
    North
};

auto wormholeFacingVector(WormholeFacing facing) -> Vector2;

class Wormhole
{
  public:
    Vector2 positionA;
    Vector2 positionB;
    WormholeFacing facingA;
    WormholeFacing facingB;
    float radius;
    bool active;
    float timer;
};

enum class AsteroidTier : std::uint8_t
{
    Large,
    Medium,
    Small
};

// Every field below `active` has a default so every existing designated-init call site (which
// only ever lists position/velocity/radius/tier/active) keeps compiling unchanged.
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
class Asteroid
{
  public:
    Vector2 position;
    Vector2 velocity;
    float radius;
    AsteroidTier tier;
    bool active;

    // Shattered Belt's signature hazard: a "rotating rock cluster" is just several Asteroid
    // entries orbiting a shared, stationary clusterCenter instead of free-drifting on velocity -
    // reuses every existing weapon/asteroid collision site (they already break an asteroid on any
    // hit) with zero changes there. See spawnRockCluster/updateAsteroids in update.cpp.
    bool inCluster = false;
    Vector2 clusterCenter{};
    float clusterOrbitRadius = 0;
    float clusterAngleDeg = 0;
    float clusterRotationSpeed = 0;
};

const int maxAsteroid = 40;

auto asteroidRadius(AsteroidTier tier) -> float;
auto asteroidScore(AsteroidTier tier) -> int;

void breakAsteroid(std::vector<Asteroid>& asteroids, const Asteroid& asteroid);
