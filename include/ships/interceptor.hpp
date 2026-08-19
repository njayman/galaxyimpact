#pragma once

#include "raylib.h"

// Draws the Interceptor hull centered at p, sized by r, facing angle (radians, 0 = -Y/up),
// tinted by shipColor. Matches drawShipHull's coordinate convention in src/draw.cpp.
void drawInterceptorHull(Vector2 p, float r, float angle, Color shipColor);
