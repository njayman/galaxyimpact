#include "ships/interceptor.hpp"

#include "ships/hull.hpp"

// Long needle with small swept tail fins; twin jets close to the spine.
void drawInterceptorHull(Vector2 p, float r, float angle, Color shipColor)
{
    static const std::vector<Vector2> fin{{0.36F, 0.28F}, {0.98F, 0.9F}, {0.98F, 1.0F}, {0.36F, 0.78F}};
    static const PanelHull hull{
        .silhouette = mirrorHalf({{0, -1.05F},
                                  {0.14F, -0.7F},
                                  {0.26F, 0.0F},
                                  {0.36F, 0.28F},
                                  {0.98F, 0.9F},
                                  {0.98F, 1.0F},
                                  {0.36F, 0.78F},
                                  {0.2F, 0.72F},
                                  {0, 0.82F}}),
        .panels = {fin, flipX(fin)},
        .seams = {{{0.26F, 0.0F}, {0.2F, 0.72F}}, {{-0.26F, 0.0F}, {-0.2F, 0.72F}}},
        .notch = {{0, -0.72F}, {0.09F, -0.42F}, {0, -0.16F}, {-0.09F, -0.42F}},
        .jets = {{-0.28F, 0.76F, 0.08F}, {0.28F, 0.76F, 0.08F}},
    };
    drawPanelHull(hull, p, r, angle, shipColor);
}
