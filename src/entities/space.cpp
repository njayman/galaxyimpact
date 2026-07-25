#include "entities/space.hpp"

#include "raymath.h"
#include <cmath>

auto asteroidRadius(AsteroidTier tier) -> float
{
    switch (tier)
    {
    case AsteroidTier::Large:
        return SpaceConstants::LargeRadius;
    case AsteroidTier::Medium:
        return SpaceConstants::MediumRadius;
    case AsteroidTier::Small:
        return SpaceConstants::SmallRadius;
    }

    return SpaceConstants::SmallRadius;
}

auto asteroidScore(AsteroidTier tier) -> int
{
    switch (tier)
    {
    case AsteroidTier::Large:
        return SpaceConstants::LargeScore;
    case AsteroidTier::Medium:
        return SpaceConstants::MediumScore;
    case AsteroidTier::Small:
        return SpaceConstants::SmallScore;
    }

    return SpaceConstants::SmallScore;
}

auto wormholeFacingVector(WormholeFacing facing) -> Vector2
{
    switch (facing)
    {
    case WormholeFacing::East:
        return Vector2{.x = 1, .y = 0};
    case WormholeFacing::South:
        return Vector2{.x = 0, .y = 1};
    case WormholeFacing::West:
        return Vector2{.x = -1, .y = 0};
    case WormholeFacing::North:
        return Vector2{.x = 0, .y = -1};
    }

    return Vector2{.x = 1, .y = 0};
}

void breakAsteroid(std::vector<Asteroid>& asteroids, const Asteroid& asteroid)
{
    if (asteroid.tier == AsteroidTier::Small || static_cast<int>(asteroids.size()) >= maxAsteroid)
    {
        return;
    }

    const auto childTier = static_cast<AsteroidTier>(static_cast<int>(asteroid.tier) + 1);
    const float speed = static_cast<float>(GetRandomValue(30, 50)) / 10.0F;

    for (int i = 0; i < 3 && static_cast<int>(asteroids.size()) < maxAsteroid; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const Vector2 velocity{.x = std::cos(angle) * speed, .y = std::sin(angle) * speed};

        asteroids.push_back(Asteroid{.position = asteroid.position,
                                     .velocity = velocity,
                                     .radius = asteroidRadius(childTier),
                                     .tier = childTier,
                                     .active = true});
    }
}
