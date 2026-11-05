#pragma once

#include "raylib.h"
#include <cstdint>
#include <vector>

struct Game;

// Punctum background (docs/design/biomes.md): the other Biomes' background objects fall toward
// the map centre and come apart there, the side nearest the centre first. Each object is drawn
// once into a sprite with its own Biome's code, then cut into tiles that break off one by one.
// Positions are on a 0.3 parallax layer whose origin is the map centre.

enum class InfallKind : std::uint8_t
{
    BeltPlanet,   // Shattered Belt's broken planets
    Derelict,     // Rustbloom's derelict capital ships
    HullFragment, // Rustbloom's drifting hull plates
    BloomPods,    // Rustbloom's pods and tendrils
    GelBlobs      // Solar Forge's cave gel
};

struct InfallPiece
{
    Rectangle src{}; // the tile in its sprite's texture
    Vector2 local{}; // offset in the object's frame while attached
    float size = 0;
    bool loose = false;
    Vector2 pos{}; // layer position once loose
    Vector2 vel{};
    float rotation = 0;
    float life = 0;
};

struct InfallObject
{
    int32_t sprite = 0;
    Vector2 pos{};
    float angle = 0;
    float spin = 0;
    float reach = 0; // farthest tile from its centre
    std::vector<InfallPiece> pieces;
};

struct PunctumInfall
{
    std::vector<InfallObject> objects;
    float spawnTimer = 0;
};

void preparePunctumInfallSprites(const Game& game); // call outside any texture mode
void updatePunctumInfall(Game& game, float deltaTime);
void drawPunctumInfall(const Game& game);
void drawInfallSprite(InfallKind kind, float seed, Vector2 center, float size); // in draw.cpp
constexpr float infallPlanetSpriteRatio = 3.7F; // sprite side / planet radius: room for the debris ring
