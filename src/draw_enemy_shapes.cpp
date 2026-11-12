#include "draw_internal.hpp"

#include <span>

// Enemy silhouettes. Each one is a unit-space outline (+x is forward) filled as a fan of facets
// lit from the upper left, the way the asteroids and planets are, plus a few parts that say what
// the Enemy does: a barrel on what shoots, pods on what spawns, spikes on what blows up.

namespace DrawDetail
{

namespace
{

constexpr float tau = 2.0F * std::numbers::pi_v<float>;

struct Frame
{
    Vector2 center;
    float radius;
    float cosA;
    float sinA;

    [[nodiscard]] auto at(Vector2 unit) const -> Vector2
    {
        return Vector2{.x = center.x + (unit.x * cosA - unit.y * sinA) * radius,
                       .y = center.y + (unit.x * sinA + unit.y * cosA) * radius};
    }
};

auto frameAt(Vector2 center, float radius, float angleRad) -> Frame
{
    return Frame{.center = center, .radius = radius, .cosA = std::cos(angleRad), .sinA = std::sin(angleRad)};
}

// `amount` in -1..1: toward white when lit, toward the void when not. Alpha is kept.
auto shade(Color color, float amount) -> Color
{
    Color out = amount >= 0 ? ColorLerp(color, WHITE, 0.30F * amount) : ColorLerp(color, Palette::Void, -0.50F * amount);
    out.a = color.a;
    return out;
}

// Dark bodies get a light rim and light bodies a dark one, so both read against the void.
auto rimColor(Color color) -> Color
{
    const int32_t luma = (color.r * 3 + color.g * 6 + color.b) / 10;
    Color out = luma < 70 ? ColorLerp(color, Palette::Haze, 0.55F) : ColorLerp(color, Palette::Void, 0.60F);
    out.a = color.a;
    return out;
}

void triangle(Vector2 a, Vector2 b, Vector2 c, Color color)
{
    // raylib culls one winding
    if ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0)
    {
        std::swap(b, c);
    }
    DrawTriangle(a, b, c, color);
}

// The outline must be star-shaped about `hub` (every edge visible from it).
void facets(const Frame& f, std::span<const Vector2> unit, Color color, Vector2 hub = Vector2{})
{
    constexpr Vector2 light{.x = -0.6F, .y = -0.8F};
    const Vector2 c = f.at(hub);
    const size_t n = unit.size();
    for (size_t i = 0; i < n; i++)
    {
        const Vector2 a = f.at(unit[i]);
        const Vector2 b = f.at(unit[(i + 1) % n]);
        const Vector2 out = Vector2Normalize(
            Vector2{.x = (a.x + b.x) / 2 - c.x, .y = (a.y + b.y) / 2 - c.y});
        triangle(c, a, b, shade(color, Vector2DotProduct(out, light)));
    }
    const Color rim = rimColor(color);
    for (size_t i = 0; i < n; i++)
    {
        DrawLineEx(f.at(unit[i]), f.at(unit[(i + 1) % n]), 1.2F, rim);
    }
}

// A ring of lit plates; `gapEvery` > 0 leaves every gapEvery-th plate out.
void facetRing(Vector2 center, float inner, float outer, int32_t plates, float rotDeg, Color color, int32_t gapEvery = 0)
{
    const float step = 360.0F / static_cast<float>(plates);
    for (int32_t i = 0; i < plates; i++)
    {
        if (gapEvery > 0 && i % gapEvery == 0)
        {
            continue;
        }
        const float a0 = rotDeg + static_cast<float>(i) * step;
        const float mid = (a0 + step / 2) * DEG2RAD;
        const float lit = std::cos(mid) * -0.6F + std::sin(mid) * -0.8F;
        DrawRing(center, inner, outer, a0 + 1.5F, a0 + step - 1.5F, 3, shade(color, lit));
    }
}

void regular(const Frame& f, int32_t sides, float scale, Color color, float turn = 0)
{
    std::array<Vector2, 12> pts{};
    for (int32_t i = 0; i < sides; i++)
    {
        const float a = turn + static_cast<float>(i) * tau / static_cast<float>(sides);
        pts.at(static_cast<size_t>(i)) = Vector2{.x = std::cos(a) * scale, .y = std::sin(a) * scale};
    }
    facets(f, std::span(pts.data(), static_cast<size_t>(sides)), color);
}

void spikes(const Frame& f, int32_t points, float inner, Color color)
{
    std::array<Vector2, 16> pts{};
    for (int32_t i = 0; i < points * 2; i++)
    {
        const float a = static_cast<float>(i) * tau / static_cast<float>(points * 2);
        const float r = i % 2 == 0 ? 1.15F : inner;
        pts.at(static_cast<size_t>(i)) = Vector2{.x = std::cos(a) * r, .y = std::sin(a) * r};
    }
    facets(f, std::span(pts.data(), static_cast<size_t>(points * 2)), color);
}

// A lump of rock: eight corners at seeded distances.
void rock(const Frame& f, float seed, Color color)
{
    std::array<Vector2, 8> pts{};
    for (size_t i = 0; i < pts.size(); i++)
    {
        const float a = static_cast<float>(i) * tau / 8.0F;
        const float r = 0.78F + 0.27F * (0.5F + 0.5F * std::sin(seed * 12.9898F + static_cast<float>(i) * 4.37F));
        pts.at(i) = Vector2{.x = std::cos(a) * r, .y = std::sin(a) * r};
    }
    facets(f, pts, color);
}

void barrel(const Frame& f, float length, float halfWidth, Color color)
{
    const std::array<Vector2, 4> pts{Vector2{.x = 0.2F, .y = -halfWidth}, Vector2{.x = length, .y = -halfWidth},
                                     Vector2{.x = length, .y = halfWidth}, Vector2{.x = 0.2F, .y = halfWidth}};
    facets(f, pts, shade(color, -0.5F), Vector2{.x = (0.2F + length) / 2, .y = 0});
}

// A lit point: an eye, a fuse, a core.
void glow(const Frame& f, Vector2 unit, float size, Color color, float alpha)
{
    const Vector2 p = f.at(unit);
    DrawCircleV(p, f.radius * size * 1.8F, Fade(color, 0.25F * alpha));
    DrawCircleV(p, f.radius * size, Fade(color, alpha));
}

} // namespace

void drawEnemyShape(EnemyShape shape, Vector2 center, float radius, float facingDeg, Color color, float seed)
{
    const auto t = static_cast<float>(GetTime());
    const float alpha = static_cast<float>(color.a) / 255.0F;
    const Frame face = frameAt(center, radius, facingDeg * DEG2RAD);
    const Frame still = frameAt(center, radius, seed);
    const float pulse = 0.5F + 0.5F * std::sin(t * 6.0F + seed);

    switch (shape)
    {
    case EnemyShape::Pod: // Drifter: a blunt pod with one eye
    {
        constexpr std::array<Vector2, 8> hull{Vector2{1.0F, 0},        Vector2{0.6F, 0.7F},   Vector2{-0.2F, 0.85F},
                                              Vector2{-0.9F, 0.5F},    Vector2{-1.0F, 0},     Vector2{-0.9F, -0.5F},
                                              Vector2{-0.2F, -0.85F},  Vector2{0.6F, -0.7F}};
        facets(face, hull, color);
        glow(face, Vector2{.x = 0.4F, .y = 0}, 0.2F, Palette::Crit, alpha);
        break;
    }
    case EnemyShape::Dart: // Swarmling: small and sharp
    {
        constexpr std::array<Vector2, 4> hull{Vector2{1.25F, 0}, Vector2{-0.75F, 0.8F}, Vector2{-0.3F, 0},
                                              Vector2{-0.75F, -0.8F}};
        facets(face, hull, color);
        break;
    }
    case EnemyShape::Slab: // Brute: armour plate over armour plate
    {
        constexpr std::array<Vector2, 6> hull{Vector2{1.0F, 0.35F},  Vector2{0.45F, 0.95F},  Vector2{-0.9F, 0.8F},
                                              Vector2{-0.9F, -0.8F}, Vector2{0.45F, -0.95F}, Vector2{1.0F, -0.35F}};
        facets(face, hull, color);
        regular(face, 6, 0.5F, shade(color, 0.6F));
        DrawLineEx(face.at(Vector2{.x = 0.8F, .y = -0.28F}), face.at(Vector2{.x = 0.8F, .y = 0.28F}), 2.0F,
                   Fade(Palette::Accent, alpha));
        break;
    }
    case EnemyShape::Chevron: // Zigzagger: a swept wing
    {
        constexpr std::array<Vector2, 6> hull{Vector2{0.9F, 0},      Vector2{0.1F, 0.5F},  Vector2{-0.8F, 1.15F},
                                              Vector2{-0.25F, 0},    Vector2{-0.8F, -1.15F}, Vector2{0.1F, -0.5F}};
        facets(face, hull, color);
        break;
    }
    case EnemyShape::Lance: // Charger: a long head, fins, an engine
    {
        constexpr std::array<Vector2, 8> hull{Vector2{1.45F, 0},     Vector2{0.2F, 0.45F},  Vector2{-0.65F, 0.95F},
                                              Vector2{-0.9F, 0.4F},  Vector2{-0.6F, 0},     Vector2{-0.9F, -0.4F},
                                              Vector2{-0.65F, -0.95F}, Vector2{0.2F, -0.45F}};
        facets(face, hull, color);
        glow(face, Vector2{.x = -0.7F, .y = 0}, 0.16F, Palette::Charge, alpha * (0.5F + 0.5F * pulse));
        break;
    }
    case EnemyShape::Orbiter: // a ring with its own moon
    {
        facetRing(center, radius * 0.55F, radius, 10, t * 40.0F, color);
        DrawCircleV(center, radius * 0.22F, shade(color, 0.6F));
        const float a = t * 3.0F + seed;
        DrawCircleV(Vector2{.x = center.x + std::cos(a) * radius * 1.35F, .y = center.y + std::sin(a) * radius * 1.35F},
                    radius * 0.18F, shade(color, 0.8F));
        break;
    }
    case EnemyShape::Splitter: // two halves, already coming apart
    {
        const float gap = 0.08F + 0.05F * pulse;
        for (const float side : {-1.0F, 1.0F})
        {
            const std::array<Vector2, 4> half{
                Vector2{.x = -0.9F, .y = side * gap}, Vector2{.x = 0.9F, .y = side * gap},
                Vector2{.x = 0.75F, .y = side * 0.95F}, Vector2{.x = -0.75F, .y = side * 0.95F}};
            facets(face, half, color, Vector2{.x = 0, .y = side * 0.5F});
        }
        break;
    }
    case EnemyShape::Turret: // a plinth and a gun
    {
        regular(still, 8, 1.0F, color, tau / 16.0F);
        barrel(face, 1.5F, 0.2F, color);
        regular(face, 6, 0.42F, shade(color, 0.7F));
        break;
    }
    case EnemyShape::LongBarrel: // Sniper, Solar Turret: all barrel
    {
        constexpr std::array<Vector2, 5> hull{Vector2{0.9F, 0}, Vector2{0.1F, 0.75F}, Vector2{-0.9F, 0.5F},
                                              Vector2{-0.9F, -0.5F}, Vector2{0.1F, -0.75F}};
        barrel(face, 2.0F, 0.1F, color);
        facets(face, hull, color);
        glow(face, Vector2{.x = 0.15F, .y = 0}, 0.16F, Palette::Crit, alpha);
        break;
    }
    case EnemyShape::ShieldDrone: // a core behind plates
    {
        regular(face, 6, 0.6F, color);
        facetRing(center, radius * 0.82F, radius * 1.05F, 6, t * -30.0F, shade(color, 0.5F));
        break;
    }
    case EnemyShape::Bomb: // Bomber, Cinder Wisp: spikes round a fuse
    {
        spikes(frameAt(center, radius, t * 1.2F + seed), 6, 0.55F, color);
        const float fuse = 0.5F + 0.5F * std::sin(t * 14.0F + seed);
        DrawCircleV(center, radius * 0.3F, Fade(ColorLerp(Palette::Crit, WHITE, fuse), alpha));
        break;
    }
    case EnemyShape::Pincer: // Leech, mandibles first
    {
        constexpr std::array<Vector2, 10> hull{Vector2{1.2F, 0.55F},   Vector2{0.45F, 0.2F},   Vector2{0.45F, -0.2F},
                                               Vector2{1.2F, -0.55F},  Vector2{0.5F, -0.95F},  Vector2{-0.5F, -0.85F},
                                               Vector2{-1.0F, -0.3F},  Vector2{-1.0F, 0.3F},   Vector2{-0.5F, 0.85F},
                                               Vector2{0.5F, 0.95F}};
        facets(face, hull, color);
        break;
    }
    case EnemyShape::Hive: // Swarm Mother: a body carrying its brood
    {
        regular(still, 8, 0.78F, color);
        for (int32_t i = 0; i < 4; i++)
        {
            const float a = t * 0.6F + seed + static_cast<float>(i) * tau / 4.0F;
            const Frame pod = frameAt(
                Vector2{.x = center.x + std::cos(a) * radius * 0.95F, .y = center.y + std::sin(a) * radius * 0.95F},
                radius * 0.3F, a);
            regular(pod, 5, 1.0F, shade(color, 0.7F));
        }
        break;
    }
    case EnemyShape::Wraith: // Phase Wraith, Null Crawler: never quite all there
    {
        facetRing(center, radius * 0.7F, radius, 8, t * 50.0F, color, 2);
        facetRing(center, radius * 0.3F, radius * 0.55F, 6, t * -70.0F, shade(color, 0.6F), 3);
        break;
    }
    case EnemyShape::Mine: // four spikes and a blinking light
    {
        spikes(frameAt(center, radius, t * 0.5F + seed), 4, 0.6F, color);
        const float blink = std::fmod(t * 1.5F + seed, 1.0F) < 0.25F ? 1.0F : 0.25F;
        glow(still, Vector2{}, 0.2F, Palette::Accent, alpha * blink);
        break;
    }
    case EnemyShape::Rift: // Void Rift: a hole with rings turning round it
    {
        DrawCircleV(center, radius * 0.5F, Fade(Palette::Void, alpha));
        facetRing(center, radius * 0.85F, radius, 9, t * 30.0F, color, 3);
        facetRing(center, radius * 0.6F, radius * 0.75F, 7, t * -55.0F, shade(color, 0.5F), 2);
        facetRing(center, radius * 0.4F, radius * 0.5F, 5, t * 90.0F, shade(color, 0.9F), 2);
        break;
    }
    case EnemyShape::Rock:
        rock(still, seed, color);
        break;
    case EnemyShape::CrackedRock: // Rubble Splitter: the break is already showing
    {
        rock(still, seed, color);
        const Color crack = Fade(Palette::Void, alpha);
        DrawLineEx(still.at(Vector2{.x = -0.7F, .y = -0.25F}), still.at(Vector2{.x = -0.1F, .y = 0.1F}), 2.0F, crack);
        DrawLineEx(still.at(Vector2{.x = -0.1F, .y = 0.1F}), still.at(Vector2{.x = 0.2F, .y = -0.2F}), 2.0F, crack);
        DrawLineEx(still.at(Vector2{.x = 0.2F, .y = -0.2F}), still.at(Vector2{.x = 0.75F, .y = 0.2F}), 2.0F, crack);
        break;
    }
    case EnemyShape::MoltenRock: // Forgeborn, Meteor Chunk: rock round a live core
    {
        rock(still, seed, color);
        glow(still, Vector2{}, 0.28F, Palette::ElementBurn, alpha * (0.6F + 0.4F * pulse));
        break;
    }
    case EnemyShape::Strafer: // Ricochet Strafer: a ring with fins
    {
        constexpr std::array<Vector2, 3> finA{Vector2{0.2F, 0.85F}, Vector2{-1.1F, 1.3F}, Vector2{-0.5F, 0.7F}};
        constexpr std::array<Vector2, 3> finB{Vector2{0.2F, -0.85F}, Vector2{-0.5F, -0.7F}, Vector2{-1.1F, -1.3F}};
        facets(face, finA, color, Vector2{.x = -0.45F, .y = 0.95F});
        facets(face, finB, color, Vector2{.x = -0.45F, .y = -0.95F});
        facetRing(center, radius * 0.5F, radius * 0.95F, 8, facingDeg, color);
        break;
    }
    case EnemyShape::Crawler: // Vent Crawler: a body on legs
    {
        const Color leg = rimColor(color);
        for (int32_t i = 0; i < 3; i++)
        {
            const float x = 0.5F - static_cast<float>(i) * 0.5F;
            const float kick = 0.2F * std::sin(t * 12.0F + static_cast<float>(i) * 2.0F + seed);
            for (const float side : {-1.0F, 1.0F})
            {
                DrawLineEx(face.at(Vector2{.x = x, .y = side * 0.3F}),
                           face.at(Vector2{.x = x + kick * side, .y = side * 1.2F}), 1.6F, leg);
            }
        }
        constexpr std::array<Vector2, 6> hull{Vector2{1.1F, 0}, Vector2{0.3F, 0.55F}, Vector2{-0.7F, 0.45F},
                                              Vector2{-1.0F, 0}, Vector2{-0.7F, -0.45F}, Vector2{0.3F, -0.55F}};
        facets(face, hull, color);
        break;
    }
    case EnemyShape::Warden: // Anomaly Warden: a keep with its wards turning
    {
        regular(still, 8, 0.85F, color);
        regular(frameAt(center, radius, t * -0.8F), 4, 0.5F, shade(color, 0.7F));
        facetRing(center, radius * 1.05F, radius * 1.18F, 12, t * 25.0F, color, 3);
        break;
    }
    case EnemyShape::Hacker: // a dish on a small body
    {
        constexpr std::array<Vector2, 4> hull{Vector2{0.5F, 0}, Vector2{-0.3F, 0.7F}, Vector2{-1.1F, 0},
                                              Vector2{-0.3F, -0.7F}};
        facets(face, hull, color);
        DrawRing(center, radius * 0.95F, radius * 1.15F, facingDeg - 55.0F, facingDeg + 55.0F, 8, shade(color, 0.5F));
        DrawLineEx(center, face.at(Vector2{.x = 1.05F, .y = 0}), 1.6F, rimColor(color));
        glow(face, Vector2{.x = 1.05F, .y = 0}, 0.14F, WHITE, alpha * (0.5F + 0.5F * pulse));
        break;
    }
    case EnemyShape::Pylon: // Laser Fence: a post with live emitters
    {
        regular(still, 4, 0.8F, color, tau / 8.0F);
        regular(still, 4, 0.45F, shade(color, -0.6F));
        for (int32_t i = 0; i < 4; i++)
        {
            const float a = seed + static_cast<float>(i) * tau / 4.0F;
            glow(still, Vector2{.x = std::cos(a - seed) * 0.95F, .y = std::sin(a - seed) * 0.95F}, 0.13F, Palette::Crit,
                 alpha * (0.4F + 0.6F * pulse));
        }
        break;
    }
    case EnemyShape::Organic:
    case EnemyShape::Cluster:
        break; // drawn by drawOrganicEnemyShape / drawClusterEnemyShape
    }
}

} // namespace DrawDetail
