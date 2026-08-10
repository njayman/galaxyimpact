#pragma once

#include "raylib.h"
#include <cstdint>
#include <vector>

struct Game;

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

class Asteroid
{
  public:
    Vector2 position;
    Vector2 velocity;
    float radius;
    AsteroidTier tier;
    bool active;

    bool inCluster = false;
    Vector2 clusterCenter{};
    float clusterOrbitRadius = 0;
    float clusterAngleDeg = 0;
    float clusterRotationSpeed = 0;

    bool isFluid = false;
};

const int maxAsteroid = 40;

auto asteroidRadius(AsteroidTier tier) -> float;
auto asteroidScore(AsteroidTier tier) -> int;

auto isSolarForgeCaveOpen(Vector2 worldPos) -> bool;

auto rustbloomNearestPodCenter(Vector2 worldPos) -> Vector2;

void breakAsteroid(Game& game, const Asteroid& asteroid);
