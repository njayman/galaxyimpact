#include "entities/space.hpp"

#include "raymath.h"
#include "update.hpp"
#include "update_constants.hpp"
#include <cmath>

namespace
{

auto hash2D(float x, float y) -> float
{
    const float v = std::sin(x * 127.1F + y * 311.7F) * 43758.5453F;
    return v - std::floor(v);
}

auto valueNoise2D(float x, float y) -> float
{
    const float xi = std::floor(x);
    const float yi = std::floor(y);
    const float xf = x - xi;
    const float yf = y - yi;
    const float u = xf * xf * (3.0F - 2.0F * xf);
    const float v = yf * yf * (3.0F - 2.0F * yf);

    const float h00 = hash2D(xi, yi);
    const float h10 = hash2D(xi + 1.0F, yi);
    const float h01 = hash2D(xi, yi + 1.0F);
    const float h11 = hash2D(xi + 1.0F, yi + 1.0F);

    return std::lerp(std::lerp(h00, h10, u), std::lerp(h01, h11, u), v);
}

auto hashNoise1D(float seed) -> float
{
    const float v = std::sin(seed) * 43758.5453F;
    return v - std::floor(v);
}

}

auto isSolarForgeCaveOpen(Vector2 worldPos) -> bool
{
    const float base = valueNoise2D(worldPos.x / caveNoiseScaleCoarse, worldPos.y / caveNoiseScaleCoarse);
    const float edgeJitter =
        (valueNoise2D(worldPos.x / caveNoiseScaleFine, worldPos.y / caveNoiseScaleFine) - 0.5F) *
        0.05F;
    return std::abs(base - 0.5F + edgeJitter) < caveTunnelHalfWidth;
}

auto rustbloomNearestPodCenter(Vector2 worldPos) -> Vector2
{
    const float baseX = std::floor(worldPos.x / rustbloomPodCellSize) * rustbloomPodCellSize;
    const float baseY = std::floor(worldPos.y / rustbloomPodCellSize) * rustbloomPodCellSize;

    Vector2 best = worldPos;
    float bestDist = -1.0F;
    for (int32_t dy = -1; dy <= 1; dy++)
    {
        for (int32_t dx = -1; dx <= 1; dx++)
        {
            const float x = baseX + static_cast<float>(dx) * rustbloomPodCellSize;
            const float y = baseY + static_cast<float>(dy) * rustbloomPodCellSize;
            if (hashNoise1D(x * 0.021F + y * 0.037F) > rustbloomPodChance)
            {
                continue;
            }
            const float jx = (hashNoise1D(x * 0.05F + y * 0.02F + 3.0F) - 0.5F) * rustbloomPodCellSize * 0.5F;
            const float jy = (hashNoise1D(x * 0.02F + y * 0.05F + 7.0F) - 0.5F) * rustbloomPodCellSize * 0.5F;
            const Vector2 podCenter{.x = x + rustbloomPodCellSize / 2 + jx,
                                    .y = y + rustbloomPodCellSize / 2 + jy};
            const float d = Vector2Distance(worldPos, podCenter);
            if (bestDist < 0 || d < bestDist)
            {
                bestDist = d;
                best = podCenter;
            }
        }
    }
    return best;
}

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

void breakAsteroid(Game& game, const Asteroid& asteroid)
{
    playSFX(game, game.resources.sounds.rockBreak);

    auto& asteroids = game.run.asteroids;
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
