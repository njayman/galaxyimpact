#include "punctum_infall.hpp"

#include "entities/enemy.hpp"
#include "game.hpp"
#include "palette.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
constexpr float parallax = 0.3F;
constexpr int32_t maxObjects = 6;
constexpr float spawnInterval = 2.5F;
constexpr float breakRadius = 700.0F;  // facets inside this distance of the centre come loose
constexpr float swallowRadius = 30.0F; // loose facets vanish here
constexpr float looseLife = 2.2F;
constexpr float visibility = 0.35F; // how far toward full colour the wreckage is drawn
constexpr float tau = 2.0F * std::numbers::pi_v<float>;

auto rnd(float lo, float hi) -> float
{
    return lo + (hi - lo) * static_cast<float>(GetRandomValue(0, 10000)) / 10000.0F;
}

enum class Wreck : std::uint8_t
{
    PlanetChunk, // Shattered Belt: faceted stone with amber, a wedge broken out
    Derelict,    // Rustbloom: a rusted hull snapped in two, bloom along its edges
    BasaltSlab   // Solar Forge: dark basalt split by a gold seam
};

// A mosaic of facets filling the wreck's silhouette, lit from one side.
auto buildWreck(Wreck kind, float size) -> InfallObject
{
    InfallObject obj{.pos = {}, .angle = rnd(0, tau), .spin = rnd(-0.08F, 0.08F), .pieces = {}};
    const float spacing = size / 7.0F;
    const Vector2 light = Vector2Rotate(Vector2{.x = 1, .y = 0}, rnd(0, tau));
    const float cutStart = rnd(0, tau);
    const float breakX = rnd(-0.25F, 0.25F) * size;
    const float seamAngle = rnd(0, tau);
    const Color stone = GetRandomValue(0, 1) == 0 ? Palette::StructLight : Palette::RustbloomHaze;

    for (float y = -size; y <= size; y += spacing)
    {
        for (float x = -size; x <= size; x += spacing)
        {
            const Vector2 p{.x = x + rnd(-0.25F, 0.25F) * spacing, .y = y + rnd(-0.25F, 0.25F) * spacing};
            const float r = Vector2Length(p);
            const float a = std::atan2(p.y, p.x);
            bool inside = false;
            Color color = stone;
            switch (kind)
            {
            case Wreck::PlanetChunk:
            {
                const float rel = std::fmod(a - cutStart + 2 * tau, tau);
                inside = r < size * (0.85F + 0.12F * std::sin(a * 5.0F)) && rel > 1.1F;
                color = GetRandomValue(0, 9) == 0 ? Palette::ShatteredBeltAccent : stone;
                break;
            }
            case Wreck::Derelict:
            {
                const float half = size * 0.24F * (1.0F - 0.45F * (p.x / size) * (p.x / size));
                inside = std::abs(p.y) < half && std::abs(p.x - breakX) > spacing * 0.7F;
                color = std::abs(p.y) > half - spacing && GetRandomValue(0, 2) == 0 ? Palette::RustbloomAccent
                                                                                 : Palette::RustbloomHaze;
                break;
            }
            case Wreck::BasaltSlab:
            {
                inside = (p.x * p.x) / (size * size) + (p.y * p.y) / (size * size * 0.36F) <
                         0.85F + 0.15F * std::sin(a * 3.0F);
                const float seam = std::abs(p.x * std::sin(seamAngle) - p.y * std::cos(seamAngle));
                color = seam < spacing * 0.7F ? Palette::Charge : Color{.r = 52, .g = 40, .b = 36, .a = 255};
                break;
            }
            }
            if (!inside)
            {
                continue;
            }
            const float lit = 0.55F + 0.45F * Vector2DotProduct(r > 0 ? Vector2Scale(p, 1.0F / r) : p, light);
            obj.pieces.push_back(InfallPiece{.local = p,
                                             .radius = spacing * rnd(0.55F, 0.72F),
                                             .rotation = rnd(0, 360),
                                             .sides = GetRandomValue(4, 6),
                                             .color = ColorBrightness(color, -0.5F + 0.5F * lit)});
        }
    }
    return obj;
}

auto toned(Color c) -> Color { return ColorLerp(Palette::PunctumVoid, c, visibility); }

auto layerView(const Game& game) -> Vector2 { return Vector2Scale(game.run.player.position, parallax); }
} // namespace

void updatePunctumInfall(Game& game, float deltaTime)
{
    PunctumInfall& infall = game.run.punctumInfall;
    if (currentBiome(game.run.waveNumber) != Biome::Punctum)
    {
        infall.objects.clear();
        return;
    }

    const Vector2 view = layerView(game);
    const float viewR = std::hypot(static_cast<float>(game.resources.screenWidth),
                                   static_cast<float>(game.resources.screenHeight)) / 2;

    // Spawn upstream (on the far side of the view from the centre) so wreckage streams through
    // the screen on its way in; near the centre it arrives from all round.
    infall.spawnTimer -= deltaTime;
    if (infall.spawnTimer <= 0 && static_cast<int32_t>(infall.objects.size()) < maxObjects)
    {
        infall.spawnTimer = spawnInterval * rnd(0.6F, 1.4F);
        const float size = rnd(90, 190);
        const float base = Vector2Length(view) > 1500 ? std::atan2(view.y, view.x) : rnd(0, tau);
        const float a = base + rnd(-1.0F, 1.0F);
        Vector2 at = Vector2Add(view, Vector2Scale(Vector2{.x = std::cos(a), .y = std::sin(a)}, viewR + size + rnd(0, 400)));
        if (Vector2Length(at) < breakRadius + size * 2) // never born inside the break zone
        {
            at = Vector2Scale(Vector2Normalize(at), breakRadius + size * 2);
        }
        InfallObject obj = buildWreck(static_cast<Wreck>(GetRandomValue(0, 2)), size);
        obj.pos = at;
        infall.objects.push_back(std::move(obj));
    }

    for (auto& obj : infall.objects)
    {
        const float dist = Vector2Length(obj.pos);
        const float speed = 70.0F + 160.0F * std::clamp(1.0F - dist / 4000.0F, 0.0F, 1.0F); // quickens near the centre
        if (dist > 1.0F)
        {
            obj.pos = Vector2Subtract(obj.pos, Vector2Scale(obj.pos, speed * deltaTime / dist));
        }
        obj.angle += obj.spin * deltaTime;

        for (auto& piece : obj.pieces)
        {
            if (!piece.loose)
            {
                const Vector2 at = Vector2Add(obj.pos, Vector2Rotate(piece.local, obj.angle));
                if (Vector2Length(at) < breakRadius)
                {
                    // Comes away, keeping the wreck's fall plus a little spin-off.
                    piece.loose = true;
                    piece.pos = at;
                    piece.vel = Vector2Add(Vector2Scale(obj.pos, -speed / std::max(dist, 1.0F)),
                                           Vector2Rotate(Vector2Normalize(at), tau / 4 * (obj.spin > 0 ? 1.0F : -1.0F)));
                    piece.vel = Vector2Add(piece.vel, Vector2Scale(Vector2Normalize(at), rnd(-30, 30)));
                    piece.life = looseLife;
                }
                continue;
            }
            if (piece.life <= 0)
            {
                continue;
            }
            // Loose facets spiral in: pulled to the centre, swirled sideways, shrinking.
            const float d = std::max(Vector2Length(piece.pos), 1.0F);
            const Vector2 inward = Vector2Scale(piece.pos, -1.0F / d);
            const Vector2 swirl{.x = -inward.y, .y = inward.x};
            piece.vel = Vector2Add(piece.vel, Vector2Scale(Vector2Add(Vector2Scale(inward, 260.0F), Vector2Scale(swirl, 90.0F)),
                                                           deltaTime));
            piece.pos = Vector2Add(piece.pos, Vector2Scale(piece.vel, deltaTime));
            piece.rotation += 240.0F * deltaTime;
            piece.life -= deltaTime;
            if (d < swallowRadius)
            {
                piece.life = 0;
            }
        }
    }

    // Gone once every facet has burnt out, or once it drifts far from view.
    std::erase_if(infall.objects,
                  [&](const InfallObject& obj)
                  {
                      const bool spent = std::ranges::all_of(obj.pieces, [](const InfallPiece& p)
                                                             { return p.loose && p.life <= 0; });
                      return spent || Vector2Distance(obj.pos, view) > viewR * 4 + 2000;
                  });
}

void drawPunctumInfall(const Game& game)
{
    if (currentBiome(game.run.waveNumber) != Biome::Punctum)
    {
        return;
    }
    // Layer position -> world position for this parallax.
    const Vector2 shift = Vector2Scale(game.run.player.position, 1.0F - parallax);
    for (const auto& obj : game.run.punctumInfall.objects)
    {
        for (const auto& piece : obj.pieces)
        {
            if (!piece.loose)
            {
                const Vector2 at = Vector2Add(shift, Vector2Add(obj.pos, Vector2Rotate(piece.local, obj.angle)));
                DrawPoly(at, piece.sides, piece.radius, piece.rotation + obj.angle * RAD2DEG, toned(piece.color));
                continue;
            }
            if (piece.life <= 0)
            {
                continue;
            }
            // Burning away: heats to lavender, shrinks and fades, with a short streak inward.
            const float k = piece.life / looseLife;
            const Color heat = ColorLerp(toned(piece.color), Palette::PunctumHaze, std::min(1.0F, (1.0F - k) * 1.6F));
            const Vector2 at = Vector2Add(shift, piece.pos);
            const Vector2 back = Vector2Scale(Vector2Normalize(piece.vel), -piece.radius * 2.5F);
            DrawLineEx(at, Vector2Add(at, back), piece.radius * 0.5F * k, Fade(heat, 0.35F * k));
            DrawPoly(at, piece.sides, piece.radius * (0.3F + 0.7F * k), piece.rotation, Fade(heat, 0.25F + 0.65F * k));
        }
    }
}
