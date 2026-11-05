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
constexpr float breakRadius = 700.0F;  // tiles inside this distance of the centre come loose
constexpr float swallowRadius = 30.0F; // loose tiles vanish here
constexpr float looseLife = 2.2F;
constexpr int32_t tilesAcross = 24;
constexpr Color dim{.r = 125, .g = 120, .b = 140, .a = 255}; // background strength, a little lavender
constexpr float tau = 2.0F * std::numbers::pi_v<float>;

auto rnd(float lo, float hi) -> float
{
    return lo + (hi - lo) * static_cast<float>(GetRandomValue(0, 10000)) / 10000.0F;
}

struct InfallSprite
{
    RenderTexture2D target;
    float size;
    std::vector<Rectangle> tiles; // drawn-orientation tiles that hold some of the object
};

// ponytail: built once on first entering Punctum and kept for the process; a dozen small textures.
std::vector<InfallSprite> sprites;

struct SpriteSpec
{
    InfallKind kind;
    int32_t count;
    float size;
};
constexpr std::array<SpriteSpec, 5> spriteSpecs{{{InfallKind::BeltPlanet, 3, 320},
                                                 {InfallKind::Derelict, 3, 400},
                                                 {InfallKind::HullFragment, 3, 150},
                                                 {InfallKind::BloomPods, 2, 220},
                                                 {InfallKind::GelBlobs, 3, 220}}};

auto layerView(const Game& game) -> Vector2 { return Vector2Scale(game.run.player.position, parallax); }
} // namespace

void preparePunctumInfallSprites(const Game& game)
{
    if (!sprites.empty() || currentBiome(game.run.waveNumber) != Biome::Punctum)
    {
        return;
    }
    for (const auto& spec : spriteSpecs)
    {
        for (int32_t v = 0; v < spec.count; v++)
        {
            const auto px = static_cast<int32_t>(spec.size);
            InfallSprite sprite{.target = LoadRenderTexture(px, px), .size = spec.size, .tiles = {}};
            BeginTextureMode(sprite.target);
            ClearBackground(BLANK);
            drawInfallSprite(spec.kind, rnd(1, 1000), Vector2{.x = spec.size / 2, .y = spec.size / 2}, spec.size);
            EndTextureMode();

            // Keep only the tiles the object actually covers.
            Image image = LoadImageFromTexture(sprite.target.texture);
            ImageFlipVertical(&image); // render textures are stored upside down
            const auto* pixels = static_cast<const Color*>(image.data);
            const int32_t tile = px / tilesAcross;
            for (int32_t ty = 0; ty + tile <= px; ty += tile)
            {
                for (int32_t tx = 0; tx + tile <= px; tx += tile)
                {
                    int32_t covered = 0;
                    for (int32_t y = ty; y < ty + tile; y++)
                    {
                        for (int32_t x = tx; x < tx + tile; x++)
                        {
                            covered += static_cast<int32_t>(pixels[y * px + x].a > 40);
                        }
                    }
                    if (covered * 4 > tile * tile)
                    {
                        sprite.tiles.push_back(Rectangle{.x = static_cast<float>(tx),
                                                         .y = static_cast<float>(ty),
                                                         .width = static_cast<float>(tile),
                                                         .height = static_cast<float>(tile)});
                    }
                }
            }
            UnloadImage(image);
            sprites.push_back(std::move(sprite));
        }
    }
}

void updatePunctumInfall(Game& game, float deltaTime)
{
    PunctumInfall& infall = game.run.punctumInfall;
    if (currentBiome(game.run.waveNumber) != Biome::Punctum || sprites.empty())
    {
        infall.objects.clear();
        return;
    }

    const Vector2 view = layerView(game);
    const float viewR = std::hypot(static_cast<float>(game.resources.screenWidth),
                                   static_cast<float>(game.resources.screenHeight)) / 2;

    // Spawn upstream (on the far side of the view from the centre) so objects stream through the
    // screen on their way in; near the centre they arrive from all round.
    infall.spawnTimer -= deltaTime;
    if (infall.spawnTimer <= 0 && static_cast<int32_t>(infall.objects.size()) < maxObjects)
    {
        infall.spawnTimer = spawnInterval * rnd(0.6F, 1.4F);
        const auto index = GetRandomValue(0, static_cast<int32_t>(sprites.size()) - 1);
        const InfallSprite& sprite = sprites.at(static_cast<size_t>(index));
        const float scale = rnd(0.8F, 1.25F);
        const float reach = sprite.size * scale / 2;
        const float base = Vector2Length(view) > 1500 ? std::atan2(view.y, view.x) : rnd(0, tau);
        const float a = base + rnd(-1.0F, 1.0F);
        Vector2 at = Vector2Add(view, Vector2Scale(Vector2{.x = std::cos(a), .y = std::sin(a)}, viewR + reach + rnd(0, 400)));
        if (Vector2Length(at) < breakRadius + reach * 2) // never born inside the break zone
        {
            at = Vector2Scale(Vector2Normalize(at), breakRadius + reach * 2);
        }
        InfallObject obj{.sprite = index, .pos = at, .angle = rnd(0, tau), .spin = rnd(-0.08F, 0.08F), .pieces = {}};
        for (const auto& tile : sprite.tiles)
        {
            obj.pieces.push_back(InfallPiece{
                .src = tile,
                .local = Vector2Scale(Vector2{.x = tile.x + tile.width / 2 - sprite.size / 2,
                                              .y = tile.y + tile.height / 2 - sprite.size / 2},
                                      scale),
                .size = tile.width * scale});
        }
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
                    // Comes away, keeping the object's fall plus a little spin-off.
                    piece.loose = true;
                    piece.pos = at;
                    piece.rotation = obj.angle * RAD2DEG;
                    piece.vel = Vector2Add(Vector2Scale(obj.pos, -speed / std::max(dist, 1.0F)),
                                           Vector2Scale(Vector2Normalize(at), rnd(-30, 30)));
                    piece.life = looseLife;
                }
                continue;
            }
            if (piece.life <= 0)
            {
                continue;
            }
            // Loose tiles spiral in: pulled to the centre, swirled sideways, shrinking.
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

    // Gone once every tile has burnt out, or once it drifts far from view.
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
    if (currentBiome(game.run.waveNumber) != Biome::Punctum || sprites.empty())
    {
        return;
    }
    // Layer position -> world position for this parallax.
    const Vector2 shift = Vector2Scale(game.run.player.position, 1.0F - parallax);
    for (const auto& obj : game.run.punctumInfall.objects)
    {
        const InfallSprite& sprite = sprites.at(static_cast<size_t>(obj.sprite));
        const Texture2D& texture = sprite.target.texture;
        for (const auto& piece : obj.pieces)
        {
            // Render textures are upside down: flip the source rectangle.
            const Rectangle src{.x = piece.src.x,
                                .y = sprite.size - piece.src.y - piece.src.height,
                                .width = piece.src.width,
                                .height = -piece.src.height};
            if (!piece.loose)
            {
                const Vector2 at = Vector2Add(shift, Vector2Add(obj.pos, Vector2Rotate(piece.local, obj.angle)));
                const float s = piece.size + 1.0F; // a hair of overlap hides the seams
                DrawTexturePro(texture, src, Rectangle{.x = at.x, .y = at.y, .width = s, .height = s},
                               Vector2{.x = s / 2, .y = s / 2}, obj.angle * RAD2DEG, dim);
                continue;
            }
            if (piece.life <= 0)
            {
                continue;
            }
            // Burning away: heats to lavender, shrinks and fades.
            const float k = piece.life / looseLife;
            const float s = piece.size * (0.3F + 0.7F * k);
            const Color heat = ColorLerp(dim, Palette::PunctumHaze, std::min(1.0F, (1.0F - k) * 1.6F));
            const Vector2 at = Vector2Add(shift, piece.pos);
            DrawTexturePro(texture, src, Rectangle{.x = at.x, .y = at.y, .width = s, .height = s},
                           Vector2{.x = s / 2, .y = s / 2}, piece.rotation, Fade(heat, 0.25F + 0.75F * k));
            // A lavender glow over the tile, so even dark rock visibly burns.
            const float glow = std::sin((1.0F - k) * std::numbers::pi_v<float>); // flares, then dies
            DrawRectanglePro(Rectangle{.x = at.x, .y = at.y, .width = s * 0.8F, .height = s * 0.8F},
                             Vector2{.x = s * 0.4F, .y = s * 0.4F}, piece.rotation, Fade(Palette::PunctumHaze, 0.6F * glow));
        }
    }
}
