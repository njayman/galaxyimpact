#include "ships/ranger.hpp"

#include "ships/hull.hpp"

// Short delta with two forward prongs; single center jet.
void drawRangerHull(Vector2 p, float r, float angle, Color shipColor)
{
    static const std::vector<Vector2> prong{{0.62F, -0.08F}, {0.62F, -0.95F}, {0.86F, -0.62F}, {0.9F, -0.15F}};
    static const PanelHull hull{
        .silhouette = mirrorHalf({{0, -0.62F},
                                  {0.3F, -0.12F},
                                  {0.34F, 0.18F},
                                  {0.62F, -0.08F},
                                  {0.62F, -0.95F},
                                  {0.86F, -0.62F},
                                  {0.96F, 0.3F},
                                  {0.34F, 0.96F},
                                  {0, 0.8F}}),
        .panels = {prong, flipX(prong)},
        .seams = {{{0.34F, 0.18F}, {0.34F, 0.96F}}, {{-0.34F, 0.18F}, {-0.34F, 0.96F}}},
        .notch = {{0, -0.4F}, {0.12F, -0.16F}, {0, 0.06F}, {-0.12F, -0.16F}},
        .jets = {{0, 0.8F, 0.14F}},
    };
    drawPanelHull(hull, p, r, angle, shipColor);
}
