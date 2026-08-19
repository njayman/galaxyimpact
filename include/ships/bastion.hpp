#pragma once

#include "raylib.h"

// Draws the Bastion hull centered at p, sized by r, facing angle (radians, 0 = -Y/up),
// tinted by shipColor. Matches drawShipHull's coordinate convention in src/draw.cpp.
void drawBastionHull(Vector2 p, float r, float angle, Color shipColor);
