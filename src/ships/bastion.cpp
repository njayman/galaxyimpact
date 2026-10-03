#include "ships/bastion.hpp"

#include "ships/hull.hpp"

// Heavy swept wings around a narrow spine; two jets at the wing roots.
void drawBastionHull(Vector2 p, float r, float angle, Color shipColor)
{
    static const std::vector<Vector2> wing{{0.5F, -0.15F}, {1.4F, 0.5F}, {1.4F, 0.95F}, {0.55F, 0.62F}};
    static const PanelHull hull{
        .silhouette = mirrorHalf({{0, -0.95F},
                                  {0.42F, -0.6F},
                                  {0.5F, -0.15F},
                                  {1.4F, 0.5F},
                                  {1.4F, 0.95F},
                                  {0.55F, 0.62F},
                                  {0.28F, 0.85F},
                                  {0, 0.85F}}),
        .panels = {wing, flipX(wing)},
        .seams = {{{0, 0.1F}, {0, 0.6F}}},
        .notch = {{0, -0.62F}, {0.16F, -0.36F}, {0, -0.1F}, {-0.16F, -0.36F}},
        .jets = {{-0.85F, 0.82F, 0.12F}, {0.85F, 0.82F, 0.12F}},
    };
    drawPanelHull(hull, p, r, angle, shipColor);
}
