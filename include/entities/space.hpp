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
} // namespace SpaceConstants

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

// WormholeFacing is one of the 4 axis-aligned directions a mouth can face,
// in clockwise order (each step is +90 degrees). Values are used directly as
// a rotation count, so the order must not change.
enum class WormholeFacing : std::uint8_t
{
    East,
    South,
    West,
    North
};

auto wormholeFacingVector(WormholeFacing facing) -> Vector2;

// Wormhole is a pair of linked mouths, each facing one of the 4 cardinal
// directions. Anything passing through one mouth exits the other, its
// velocity rotated by the facing difference between the two mouths (see
// wormholeFacingVector/the transit helper in update.cpp) - a real portal,
// not a teleport-in-place.
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
};

const int maxAsteroid = 40;

auto asteroidRadius(AsteroidTier tier) -> float;
auto asteroidScore(AsteroidTier tier) -> int;

// breakAsteroid shatters a non-small asteroid into 3 smaller ones flying
// outward, appended to asteroids; no-ops once maxAsteroid is reached.
void breakAsteroid(std::vector<Asteroid>& asteroids, const Asteroid& asteroid);
