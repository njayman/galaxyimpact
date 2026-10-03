#pragma once

#include "raylib.h"
#include <vector>

// Flat "panel" hull: one silhouette in the ship's tint, darker wing panels, Void seams and outline
// (same treatment as drawPanelShape on the Kraken hull / arena fragments), and a Void cockpit
// notch. All points are in unit space: nose at -Y, scaled by the ship radius.
struct PanelHull
{
    std::vector<Vector2> silhouette;
    std::vector<std::vector<Vector2>> panels;
    std::vector<std::pair<Vector2, Vector2>> seams;
    std::vector<Vector2> notch;
    std::vector<Vector3> jets; // x, y, half-width of each exhaust flame's base
};

void drawPanelHull(const PanelHull& hull, Vector2 p, float r, float angle, Color tint);

// Unit-space helpers for building symmetric hull data.
auto mirrorHalf(const std::vector<Vector2>& rightHalf) -> std::vector<Vector2>;
auto flipX(std::vector<Vector2> points) -> std::vector<Vector2>;
