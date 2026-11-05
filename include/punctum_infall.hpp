#pragma once

#include "raylib.h"
#include <cstdint>
#include <vector>

struct Game;

// Punctum background (docs/design/biomes.md): wreckage from the other Biomes falls toward the
// map centre and comes apart there, the side nearest the centre first. Each object is a mosaic of
// small facets so it can break piece by piece. Positions are on a 0.3 parallax layer whose origin
// is the map centre.

struct InfallPiece
{
    Vector2 local; // offset in the object's frame while attached
    float radius;
    float rotation;
    int32_t sides;
    Color color;
    bool loose = false;
    Vector2 pos{}; // layer position once loose
    Vector2 vel{};
    float life = 0;
};

struct InfallObject
{
    Vector2 pos;
    float angle;
    float spin;
    std::vector<InfallPiece> pieces;
};

struct PunctumInfall
{
    std::vector<InfallObject> objects;
    float spawnTimer = 0;
};

void updatePunctumInfall(Game& game, float deltaTime);
void drawPunctumInfall(const Game& game);
