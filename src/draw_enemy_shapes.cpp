#include "draw_internal.hpp"

#include <span>

// Enemy silhouettes. Each one is a unit-space outline (+x is forward), filled flat and matte with
// no outline, plus a few parts that say what the Enemy does: a barrel on what shoots, pods on what
// spawns, spikes on what blows up. Only ring plates and spikes carry light and shade.

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

void triangle(Vector2 a, Vector2 b, Vector2 c, Color color)
{
    // raylib culls one winding
    if ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0)
    {
        std::swap(b, c);
    }
    DrawTriangle(a, b, c, color);
}

// The outline must be star-shaped about `hub` (every edge visible from it). `relief` 0 fills it
// flat; 1 lights each facet, which shows the fan's seams, so only spikes use it.
void facets(const Frame& f, std::span<const Vector2> unit, Color color, Vector2 hub = Vector2{}, float relief = 0)
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
        triangle(c, a, b, relief > 0 ? shade(color, relief * Vector2DotProduct(out, light)) : color);
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
    facets(f, std::span(pts.data(), static_cast<size_t>(points * 2)), color, Vector2{}, 1.0F);
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

// A flat dot: a fuse, a core.
void dot(const Frame& f, Vector2 unit, float size, Color color, float alpha)
{
    DrawCircleV(f.at(unit), f.radius * size, Fade(color, alpha));
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
    case EnemyShape::Pod: // Drifter: a disc with a crescent cut toward where it is going
    {
        DrawCircleV(center, radius, color);
        DrawCircleV(face.at(Vector2{.x = -0.3F, .y = 0}), radius * 0.62F, shade(color, -0.55F));
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
        dot(face, Vector2{.x = -0.7F, .y = 0}, 0.16F, Palette::Charge, alpha * (0.5F + 0.5F * pulse));
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
    case EnemyShape::LongBarrel: // Sniper, Solar Turret: a needle inside a turning reticle
    {
        constexpr std::array<Vector2, 4> needle{Vector2{1.7F, 0}, Vector2{-0.2F, 0.32F}, Vector2{-0.9F, 0},
                                                Vector2{-0.2F, -0.32F}};
        facets(face, needle, color);
        facetRing(center, radius * 0.72F, radius * 0.9F, 4, t * 35.0F + seed, shade(color, -0.2F), 2);
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
        dot(still, Vector2{}, 0.2F, Palette::Accent, alpha * blink);
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
        dot(still, Vector2{}, 0.28F, Palette::ElementBurn, alpha * (0.6F + 0.4F * pulse));
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
        const Color leg = shade(color, -0.45F);
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
        constexpr std::array<Vector2, 4> hull{Vector2{1.1F, 0}, Vector2{0, 0.55F}, Vector2{-1.0F, 0},
                                              Vector2{0, -0.55F}};
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
    case EnemyShape::Hacker: // a small core in a turning ring, sending
    {
        regular(still, 6, 0.42F, color);
        facetRing(center, radius * 0.66F, radius * 0.82F, 6, t * -45.0F, color, 3);
        for (int32_t i = 0; i < 2; i++)
        {
            // two arcs leave toward the target, one after the other
            const float phase = std::fmod(t * 0.9F + static_cast<float>(i) * 0.5F, 1.0F);
            const float r = radius * (1.05F + 0.7F * phase);
            DrawRing(center, r, r + radius * 0.1F, facingDeg - 32.0F, facingDeg + 32.0F, 8,
                     Fade(color, alpha * (1.0F - phase)));
        }
        break;
    }
    case EnemyShape::Pylon: // Laser Fence: a post with live emitters
    {
        regular(still, 4, 0.8F, color, tau / 8.0F);
        regular(still, 4, 0.45F, shade(color, -0.6F));
        for (int32_t i = 0; i < 4; i++)
        {
            const float a = seed + static_cast<float>(i) * tau / 4.0F;
            dot(still, Vector2{.x = std::cos(a - seed) * 0.95F, .y = std::sin(a - seed) * 0.95F}, 0.13F, Palette::Crit,
                 alpha * (0.4F + 0.6F * pulse));
        }
        break;
    }
    case EnemyShape::Organic:
    case EnemyShape::Cluster:
        break; // drawn by drawOrganicEnemyShape / drawClusterEnemyShape
    }
}

// Roaming Boss hulls (the Biome Bosses have their own drawing), in the same matte parts as the
// Enemies, at Boss size: turning rings and plates round a still core.
void drawBossHull(BossShape shape, Vector2 center, Vector2 size, Color color)
{
    const auto t = static_cast<float>(GetTime());
    const float radius = size.x / 2;
    const float alpha = static_cast<float>(color.a) / 255.0F;
    const Frame still = frameAt(center, radius, 0);
    // A hull as dark as the void would not read: lift it, and give its moving parts a lighter tone.
    const int32_t luma = (color.r * 3 + color.g * 6 + color.b) / 10;
    Color body = luma < 40 ? ColorLerp(color, Palette::StructMid, 0.55F) : color;
    body.a = color.a;
    Color part = luma < 70 ? ColorLerp(body, Palette::StructLight, 0.5F) : shade(color, 0.7F);
    part.a = color.a;
    const Color dark = shade(body, -0.55F);

    switch (shape)
    {
    case BossShape::Saucer: // a hollow disc, something turning in the hole
        regular(still, 12, 1.0F, body);
        DrawCircleV(center, radius * 0.58F, Fade(Palette::Void, alpha));
        facetRing(center, radius * 0.36F, radius * 0.5F, 6, t * -40.0F, part, 2);
        facetRing(center, radius * 1.08F, radius * 1.18F, 16, t * 12.0F, part, 4);
        DrawCircleV(center, radius * 0.14F, part);
        break;
    case BossShape::SpikedRing: // a crown of spikes turning round a plated core
        spikes(frameAt(center, radius * 1.05F, t * 0.5F), 8, 0.74F, part);
        regular(still, 8, 0.8F, body);
        facetRing(center, radius * 0.42F, radius * 0.58F, 8, t * -30.0F, dark, 2);
        regular(frameAt(center, radius, t * 0.8F), 4, 0.26F, part);
        break;
    case BossShape::TwinDome: // two cores on a spar, each in its own ring
    {
        const std::array<Vector2, 4> spar{Vector2{.x = -0.5F, .y = -0.14F}, Vector2{.x = 0.5F, .y = -0.14F},
                                          Vector2{.x = 0.5F, .y = 0.14F}, Vector2{.x = -0.5F, .y = 0.14F}};
        facets(still, spar, dark);
        for (const float side : {-1.0F, 1.0F})
        {
            const Vector2 lobe{.x = center.x + side * radius * 0.5F, .y = center.y};
            regular(frameAt(lobe, radius, 0), 8, 0.4F, body);
            facetRing(lobe, radius * 0.46F, radius * 0.56F, 6, side * t * 45.0F, part, 2);
            DrawCircleV(lobe, radius * 0.14F, part);
        }
        break;
    }
    case BossShape::Crystal: // a shard, with shards circling it
        regular(frameAt(center, radius, t * 0.14F), 6, 0.72F, body);
        regular(frameAt(center, radius, t * -0.3F), 4, 0.34F, part);
        facetRing(center, radius * 0.86F, radius * 1.04F, 6, t * -22.0F, part, 2);
        break;
    case BossShape::HexPlated: // armour: six plates bolted round a hex
        regular(still, 6, 0.74F, body);
        facetRing(center, radius * 0.84F, radius * 1.0F, 6, -30.0F, part);
        regular(still, 6, 0.4F, dark);
        DrawCircleV(center, radius * 0.14F, part);
        break;
    case BossShape::Segment:
        DrawCircleV(center, radius, body);
        DrawCircleV(center, radius * 0.55F, Fade(Palette::StructLight, 0.3F * alpha));
        break;
    }
}

// Elite Hazards, in the same matte parts as the Enemies. The Warlord drives the swarm on: a
// crown of spikes turning fast round a hard core. The Suppressor damps the ship's weapons: two
// broken rings turning against each other, closing on a still core. `body` is the biome-tinted
// hull colour, `signal` the role's colour.
void drawEliteHazardShape(EliteHazardRole role, Vector2 center, float radius, Color body, Color signal)
{
    const auto t = static_cast<float>(GetTime());
    const Frame still = frameAt(center, radius, 0);
    if (role == EliteHazardRole::Warlord)
    {
        spikes(frameAt(center, radius * 1.05F, t * 1.6F), 8, 0.78F, signal);
        regular(still, 6, 0.74F, body);
        regular(frameAt(center, radius, t * -0.9F), 4, 0.4F, signal);
        return;
    }
    facetRing(center, radius * 1.08F, radius * 1.28F, 8, t * 40.0F, signal, 2);
    facetRing(center, radius * 0.8F, radius * 0.98F, 6, t * -60.0F, shade(signal, -0.3F), 3);
    regular(still, 8, 0.7F, body);
    DrawCircleV(center, radius * 0.26F, signal);
}

// The reach of a Hazard's aura: a slow ring of dashes, not a hard line.
void drawEliteHazardAura(Vector2 center, float radius, Color signal, float turn)
{
    const auto t = static_cast<float>(GetTime());
    constexpr int32_t dashes = 36;
    for (int32_t i = 0; i < dashes; i += 2)
    {
        const float a0 = t * turn + static_cast<float>(i) * 360.0F / dashes;
        DrawRing(center, radius - 1.5F, radius + 1.5F, a0, a0 + 360.0F / dashes, 4, Fade(signal, 0.22F));
    }
}

} // namespace DrawDetail
