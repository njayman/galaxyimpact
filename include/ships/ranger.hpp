#pragma once

#include "raylib.h"

// Draws the Ranger hull centered at p, sized by r, facing angle (radians, 0 = -Y/up),
// tinted by shipColor. Matches drawShipHull's coordinate convention in src/draw.cpp.
void drawRangerHull(Vector2 p, float r, float angle, Color shipColor);
