#include "ships/hull.hpp"

#include "palette.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <ranges>

namespace
{
auto cross(Vector2 o, Vector2 a, Vector2 b) -> float
{
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

// raylib culls clockwise triangles, so order each one before drawing.
void drawTriangleAnyWinding(Vector2 a, Vector2 b, Vector2 c, Color color)
{
    if (cross(a, b, c) > 0)
    {
        std::swap(b, c);
    }
    DrawTriangle(a, b, c, color);
}

// Ear clipping, since hull silhouettes are concave. ponytail: re-triangulates every frame (~20
// verts, 3 hulls max on screen); cache the triangle indices per hull if it ever shows in a profile.
void fillPolygon(const std::vector<Vector2>& v, Color color)
{
    float area = 0;
    for (size_t i = 0; i < v.size(); i++)
    {
        const Vector2 a = v[i];
        const Vector2 b = v[(i + 1) % v.size()];
        area += a.x * b.y - b.x * a.y;
    }
    std::vector<size_t> idx(v.size());
    for (size_t i = 0; i < idx.size(); i++)
    {
        idx[i] = i;
    }
    while (idx.size() > 3)
    {
        bool clipped = false;
        for (size_t i = 0; i < idx.size() && !clipped; i++)
        {
            const size_t ia = idx[(i + idx.size() - 1) % idx.size()];
            const size_t ib = idx[i];
            const size_t ic = idx[(i + 1) % idx.size()];
            const float turn = cross(v[ia], v[ib], v[ic]);
            if (area > 0 ? turn <= 0 : turn >= 0)
            {
                continue; // reflex corner, not an ear
            }
            const bool blocked = std::ranges::any_of(
                idx,
                [&](size_t k)
                {
                    if (k == ia || k == ib || k == ic)
                    {
                        return false;
                    }
                    const float d1 = cross(v[ia], v[ib], v[k]);
                    const float d2 = cross(v[ib], v[ic], v[k]);
                    const float d3 = cross(v[ic], v[ia], v[k]);
                    return (d1 > 0 && d2 > 0 && d3 > 0) || (d1 < 0 && d2 < 0 && d3 < 0);
                });
            if (blocked)
            {
                continue;
            }
            drawTriangleAnyWinding(v[ia], v[ib], v[ic], color);
            idx.erase(idx.begin() + static_cast<std::ptrdiff_t>(i));
            clipped = true;
        }
        if (!clipped)
        {
            return; // degenerate input; draw what we have rather than spin
        }
    }
    if (idx.size() == 3)
    {
        drawTriangleAnyWinding(v[idx[0]], v[idx[1]], v[idx[2]], color);
    }
}
} // namespace

auto mirrorHalf(const std::vector<Vector2>& rightHalf) -> std::vector<Vector2>
{
    std::vector<Vector2> out = rightHalf;
    for (const Vector2 v : std::views::reverse(rightHalf))
    {
        if (v.x != 0)
        {
            out.push_back(Vector2{.x = -v.x, .y = v.y});
        }
    }
    return out;
}

auto flipX(std::vector<Vector2> points) -> std::vector<Vector2>
{
    for (auto& point : points)
    {
        point.x = -point.x;
    }
    return points;
}

void drawPanelHull(const PanelHull& hull, Vector2 p, float r, float angle, Color tint)
{
    const float cosA = std::cos(angle);
    const float sinA = std::sin(angle);
    const auto at = [&](Vector2 v) -> Vector2
    {
        return Vector2Add(p, Vector2{.x = (v.x * cosA - v.y * sinA) * r,
                                     .y = (v.x * sinA + v.y * cosA) * r});
    };
    const auto place = [&](const std::vector<Vector2>& unit)
    {
        std::vector<Vector2> out;
        out.reserve(unit.size());
        for (const Vector2 v : unit)
        {
            out.push_back(at(v));
        }
        return out;
    };

    // Fades (twirl-in, phasing) arrive via tint alpha; carry them onto every layer.
    const float alpha = static_cast<float>(tint.a) / 255.0F;
    const float flicker = 0.65F + 0.35F * std::sin(static_cast<float>(GetTime()) * 28.0F);
    const Color flameOuter = ColorAlpha(tint, (0.7F + 0.3F * flicker) * alpha);
    const Color flameInner = Fade(Color{.r = 255, .g = 240, .b = 220, .a = 255}, 0.9F * alpha);
    for (const Vector3 jet : hull.jets)
    {
        drawTriangleAnyWinding(at({jet.x - jet.z, jet.y}),
                               at({jet.x, jet.y + 0.9F * (0.55F + 0.35F * flicker)}),
                               at({jet.x + jet.z, jet.y}), flameOuter);
        drawTriangleAnyWinding(at({jet.x - jet.z / 2, jet.y}),
                               at({jet.x, jet.y + 0.9F * (0.35F + 0.2F * flicker)}),
                               at({jet.x + jet.z / 2, jet.y}), flameInner);
    }

    const std::vector<Vector2> silhouette = place(hull.silhouette);
    fillPolygon(silhouette, tint);

    const Color panelColor = ColorLerp(tint, Fade(Palette::Void, alpha), 0.38F);
    for (const auto& panel : hull.panels)
    {
        fillPolygon(place(panel), panelColor);
    }

    const float seamWidth = std::max(1.0F, r * 0.04F);
    const Color seamColor = Fade(Palette::Void, 0.5F * alpha);
    for (const auto& [a, b] : hull.seams)
    {
        DrawLineEx(at(a), at(b), seamWidth, seamColor);
    }
    const Color outlineColor = Fade(Palette::Void, 0.6F * alpha);
    for (size_t i = 0; i < silhouette.size(); i++)
    {
        DrawLineEx(silhouette[i], silhouette[(i + 1) % silhouette.size()], seamWidth * 0.75F,
                   outlineColor);
    }

    fillPolygon(place(hull.notch), Fade(Palette::Void, alpha));
}
