#include "draw_internal.hpp"

namespace DrawDetail
{

void drawSolarForgeCaveBackground(const Game& game)
{
    if (currentBiome(game.run.waveNumber) != Biome::SolarForge)
    {
        return;
    }

    constexpr float cellSize = 48.0F;
    const float halfW = static_cast<float>(game.resources.screenWidth) / 2 + cellSize;
    const float halfH = static_cast<float>(game.resources.screenHeight) / 2 + cellSize;
    const Vector2 center = game.run.player.position;
    const auto t = static_cast<float>(GetTime());
    const float startX = std::floor((center.x - halfW) / cellSize) * cellSize;
    const float startY = std::floor((center.y - halfH) / cellSize) * cellSize;

    for (float y = startY; y < center.y + halfH; y += cellSize)
    {
        for (float x = startX; x < center.x + halfW; x += cellSize)
        {
            const Vector2 cellCenter{.x = x + cellSize / 2, .y = y + cellSize / 2};
            if (isSolarForgeCaveOpen(cellCenter))
            {
                continue;
            }
            const float shade = hashNoise(x * 0.013F + y * 0.029F);
            const Color rock =
                ColorLerp(Palette::SolarForgeAccent, Palette::Void, 0.5F + shade * 0.25F);

            const float jx = (hashNoise(x * 0.071F + y * 0.019F + 4.1F) - 0.5F) * cellSize * 0.6F;
            const float jy = (hashNoise(x * 0.019F + y * 0.071F + 8.3F) - 0.5F) * cellSize * 0.6F;
            const Vector2 restCenter{.x = cellCenter.x + jx, .y = cellCenter.y + jy};
            Vector2 blobCenter = restCenter;
            float blobRadius =
                cellSize * (0.74F + (hashNoise(x * 0.037F + y * 0.053F + 1.7F) - 0.5F) * 0.3F);

            // Gel near the ship lags behind its movement, parts round it, and ripples.
            const float dist = Vector2Distance(blobCenter, center);
            if (const float fall = 1.0F - dist / solarForgeGelReach; fall > 0)
            {
                const float f2 = fall * fall;
                const Vector2 away = dist > 1.0F ? Vector2Scale(Vector2Subtract(blobCenter, center), 1.0F / dist)
                                                 : Vector2{};
                blobCenter = Vector2Add(blobCenter, Vector2Scale(game.run.solarForgeGelWobble, f2));
                blobCenter = Vector2Add(blobCenter, Vector2Scale(away, 34.0F * f2 * game.run.solarForgeGelPocket));
                const float jiggle = std::min(1.0F, Vector2Length(game.run.solarForgeGelWobbleVel) / 300.0F);
                blobRadius *= 1.0F + 0.16F * std::sin(dist * 0.04F - t * 12.0F) * f2 * jiggle;
            }

            DrawCircleV(blobCenter, blobRadius, Fade(rock, 0.62F));
            DrawCircleV(Vector2Add(blobCenter, Vector2{.x = -blobRadius * 0.28F, .y = -blobRadius * 0.28F}),
                        blobRadius * 0.42F, Fade(ColorLerp(rock, WHITE, 0.3F), 0.14F));
            DrawCircleLines(static_cast<int32_t>(blobCenter.x), static_cast<int32_t>(blobCenter.y),
                            blobRadius, Fade(ColorLerp(rock, WHITE, 0.2F), 0.2F));
            if (shade > 0.78F)
            {
                const float ex =
                    (hashNoise(x * 0.091F + y * 0.061F + 2.3F) - 0.5F) * cellSize * 0.5F;
                const float ey =
                    (hashNoise(x * 0.061F + y * 0.091F + 6.7F) - 0.5F) * cellSize * 0.5F;

                const float glowPulse =
                    0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 1.3F + shade * 30.0F);
                const Color emberColor = ColorLerp(Palette::ElementBurn, YELLOW, glowPulse * 0.5F);
                DrawCircleV(Vector2Add(Vector2{.x = cellCenter.x + ex, .y = cellCenter.y + ey},
                                       Vector2Subtract(blobCenter, restCenter)),
                            cellSize * 0.16F, Fade(emberColor, 0.5F + glowPulse * 0.3F));
            }
        }
    }
}

// Three parallax layers: an element at layer coordinate L with parallax p is drawn at
// L + player * (1 - p), so it trails the camera by p.
// Shattered Belt broken planets. The arena is cut into an equal grid of zones, one planet each.
// A planet is a jittered low-poly sphere lit per facet; a fracture zone around a break point
// pushes facets outward and apart (glowing seams on ember planets), the facets nearest the
// break fly off as shards, and a tilted debris ring along the dust-lane angle wraps it.
// Everything is drawn opaque and blended toward the void colour, so it reads as distant and
// faint while still hiding the stars behind it.
namespace
{
struct BeltPlanetFace
{
    std::array<Vector3, 4> v;
    Vector3 normal;
    float fracture; // 0 intact .. 1 at the break
    bool shard;
};

struct BeltPlanet
{
    std::vector<BeltPlanetFace> faces;
    Vector3 breakDir;
    Vector3 light;
    Color rock;
    bool live;     // molten core: glows through the seams and slowly pulses
    Color coreHot; // live cores only
    float pulsePhase;
};

auto buildBeltPlanet(float seed) -> BeltPlanet
{
    constexpr int32_t latBands = 9;
    constexpr int32_t lonSegments = 16;
    const auto h = [seed](float k) { return hashNoise(seed * 31.7F + k); };

    // Random orientation so each planet's facet grid sits differently.
    const Matrix spin = MatrixMultiply(
        MatrixMultiply(MatrixRotateX((h(1) - 0.5F) * 70.0F * DEG2RAD), MatrixRotateY(h(2) * 360.0F * DEG2RAD)),
        MatrixRotateZ((h(3) - 0.5F) * 50.0F * DEG2RAD));

    std::array<std::array<Vector3, lonSegments>, latBands + 1> grid{};
    for (int32_t i = 0; i <= latBands; i++)
    {
        const bool pole = i == 0 || i == latBands;
        for (int32_t j = 0; j < lonSegments; j++)
        {
            const float k = static_cast<float>(i * lonSegments + j);
            const float lat = (-90.0F + 180.0F * (static_cast<float>(i) + (pole ? 0.0F : (h(10 + k) - 0.5F) * 0.5F)) /
                                            latBands) *
                              DEG2RAD;
            const float lon = 360.0F *
                              (static_cast<float>(j) + 0.5F * static_cast<float>(i % 2) +
                               (h(500 + k) - 0.5F) * 0.45F) /
                              lonSegments * DEG2RAD;
            const float r = 1.0F + (h(900 + k) - 0.5F) * 0.07F;
            const Vector3 p{.x = std::cos(lat) * std::cos(lon) * r, .y = std::sin(lat) * r,
                            .z = std::cos(lat) * std::sin(lon) * r};
            grid.at(i).at(j) = Vector3Transform(p, spin);
        }
    }

    // Break point on the visible hemisphere (+z faces the viewer).
    const float breakAngle = h(4) * 2.0F * std::numbers::pi_v<float>;
    const float breakZ = 0.35F + h(5) * 0.4F;
    const float breakXY = std::sqrt(1.0F - breakZ * breakZ);
    const Vector3 breakDir{.x = std::cos(breakAngle) * breakXY, .y = std::sin(breakAngle) * breakXY, .z = breakZ};

    // Rock and live-core colours: varied, but all muted enough to sit in the Belt's palette.
    constexpr std::array<Color, 6> rocks{Color{.r = 150, .g = 165, .b = 185, .a = 255}, Color{.r = 150, .g = 110, .b = 80, .a = 255},
                                         Color{.r = 200, .g = 190, .b = 170, .a = 255}, Color{.r = 90, .g = 110, .b = 140, .a = 255},
                                         Color{.r = 120, .g = 100, .b = 130, .a = 255}, Color{.r = 170, .g = 140, .b = 90, .a = 255}};
    constexpr std::array<Color, 4> cores{Color{.r = 255, .g = 150, .b = 60, .a = 255}, Color{.r = 235, .g = 70, .b = 45, .a = 255},
                                         Color{.r = 90, .g = 200, .b = 230, .a = 255}, Color{.r = 190, .g = 110, .b = 255, .a = 255}};
    const float lightAngle = h(7) * 2.0F * std::numbers::pi_v<float>;
    BeltPlanet planet{.breakDir = breakDir,
                      .light = Vector3Normalize(Vector3{.x = std::cos(lightAngle), .y = std::sin(lightAngle), .z = 0.7F}),
                      .rock = rocks.at(static_cast<size_t>(h(8) * rocks.size()) % rocks.size()),
                      .live = h(6) > 0.5F,
                      .coreHot = cores.at(static_cast<size_t>(h(9) * cores.size()) % cores.size()),
                      .pulsePhase = h(11) * 10.0F};
    for (int32_t i = 0; i < latBands; i++)
    {
        for (int32_t j = 0; j < lonSegments; j++)
        {
            const int32_t jn = (j + 1) % lonSegments;
            BeltPlanetFace face{.v = {grid.at(i).at(j), grid.at(i).at(jn), grid.at(i + 1).at(jn), grid.at(i + 1).at(j)}};
            Vector3 c{};
            for (const auto& v : face.v)
            {
                c = Vector3Add(c, v);
            }
            c = Vector3Scale(c, 0.25F);
            face.normal = Vector3Normalize(c);
            const float facing = Vector3DotProduct(face.normal, breakDir);
            const float k = static_cast<float>(i * lonSegments + j);
            face.fracture = std::clamp((facing - 0.45F) / (0.95F - 0.45F), 0.0F, 1.0F);
            face.shard = facing > 0.8F + h(1300 + k) * 0.14F;

            if (face.shard)
            {
                // Flung clear of the body, outward and along the break, shrunk a little.
                const float fling = 0.12F + h(1700 + k) * 0.55F;
                const Vector3 pos = Vector3Add(Vector3Scale(face.normal, 1.08F + fling * 0.3F),
                                               Vector3Scale(breakDir, fling));
                // Lay the facet's true shape flat toward the viewer, at a random spin, so a shard
                // never collapses into an edge-on sliver.
                const Vector3 u = Vector3Normalize(Vector3CrossProduct(face.normal, Vector3{.x = 0, .y = 0, .z = 1}));
                const Vector3 w = Vector3CrossProduct(face.normal, u);
                const float scale = 0.55F + h(2100 + k) * 0.35F;
                const float turn = h(2300 + k) * 2.0F * std::numbers::pi_v<float>;
                for (auto& v : face.v)
                {
                    const Vector3 d = Vector3Subtract(v, c);
                    const float a = Vector3DotProduct(d, u) * scale;
                    const float b = Vector3DotProduct(d, w) * scale;
                    v = Vector3Add(pos, Vector3{.x = a * std::cos(turn) - b * std::sin(turn),
                                                .y = a * std::sin(turn) + b * std::cos(turn), .z = 0});
                }
            }
            else if (face.fracture > 0)
            {
                // Lifted and shrunk toward its own centre, opening seams onto the core.
                const float f = face.fracture * face.fracture;
                const Vector3 lifted = Vector3Add(Vector3Scale(c, 1.0F + 0.12F * f), Vector3Scale(breakDir, 0.08F * f));
                for (auto& v : face.v)
                {
                    v = Vector3Add(lifted, Vector3Scale(Vector3Subtract(v, c), 1.0F - 0.22F * f));
                }
            }
            planet.faces.push_back(face);
        }
    }

    std::ranges::sort(planet.faces, {},
                      [](const BeltPlanetFace& f) { return f.v[0].z + f.v[1].z + f.v[2].z + f.v[3].z; });
    return planet;
}

void fillConvexQuad(const std::array<Vector2, 4>& p, Color color)
{
    // raylib only fills counter-clockwise triangles; pick the order from the winding.
    const float cross = (p[1].x - p[0].x) * (p[2].y - p[0].y) - (p[1].y - p[0].y) * (p[2].x - p[0].x);
    if (cross < 0)
    {
        DrawTriangle(p[0], p[1], p[2], color);
        DrawTriangle(p[0], p[2], p[3], color);
    }
    else
    {
        DrawTriangle(p[0], p[2], p[1], color);
        DrawTriangle(p[0], p[3], p[2], color);
    }
}
// A Belt planet with its debris ring, at any centre and size; tone maps a colour and strength to
// the final colour. Punctum's falling wreckage uses it too.
void drawBeltPlanetBody(const BeltPlanet& planet, Vector2 center, float radius,
                        const std::function<Color(Color, float)>& tone)
{
    const auto proj = [&](Vector3 v) { return Vector2{.x = center.x + v.x * radius, .y = center.y - v.y * radius}; };
    const Vector3 light = planet.light;
    const Color rock = planet.rock;
    const auto t = static_cast<float>(GetTime());
    const float pulse = 0.75F + 0.25F * std::sin(t * 1.3F + planet.pulsePhase);
    const Color emberHot = planet.coreHot;
    const Color emberDeep = ColorBrightness(planet.coreHot, -0.55F);

    // Debris ring: a tilted ellipse along the dust-lane angle, slowly orbiting. The far half
    // goes behind the body and the near half in front.
    const float laneRad = beltPlanetRingAngleDeg * DEG2RAD;
    const Vector2 ringMajor{.x = std::cos(laneRad), .y = std::sin(laneRad)};
    const Vector2 ringMinor{.x = -ringMajor.y, .y = ringMajor.x};
    const auto drawRing = [&](bool front)
    {
        constexpr int32_t debrisCount = 150;
        for (int32_t i = 0; i < debrisCount; i++)
        {
            const float k = static_cast<float>(i);
            const float dist = radius * (1.35F + 0.4F * hashNoise(k * 1.3F + 3.0F));
            const float theta = hashNoise(k * 2.7F + 9.0F) * 2.0F * std::numbers::pi_v<float> +
                                t * 0.012F * (radius * 1.5F / dist);
            if ((std::sin(theta) > 0) != front)
            {
                continue;
            }
            const Vector2 pos = Vector2Add(center, Vector2Add(Vector2Scale(ringMajor, std::cos(theta) * dist),
                                                              Vector2Scale(ringMinor, std::sin(theta) * dist * 0.2F)));
            const float size = radius * (0.006F + 0.03F * hashNoise(k * 4.1F + 1.0F) * hashNoise(k * 0.7F));
            DrawPoly(pos, 3 + i % 3, size, hashNoise(k * 5.3F) * 360.0F + t * 4.0F,
                     tone(rock, 0.55F + 0.45F * hashNoise(k * 6.1F)));
        }
    };

    drawRing(false);

    // Core, seen through the seams: molten on ember planets, dark rock otherwise.
    DrawCircleV(center, radius * 0.97F, tone(planet.live ? emberDeep : Palette::StructDark, 1.0F));
    if (planet.live)
    {
        const Vector2 glow = proj(Vector3Scale(planet.breakDir, 0.55F));
        const Color hot = tone(emberHot, 0.55F * pulse);
        DrawCircleGradient(glow, radius * 0.55F, hot,
                           Fade(hot, 0.0F));
    }

    for (const auto& face : planet.faces)
    {
        if (!face.shard && face.normal.z < -0.05F)
        {
            continue;
        }
        const std::array<Vector2, 4> pts{proj(face.v[0]), proj(face.v[1]), proj(face.v[2]), proj(face.v[3])};
        const float lit = 0.15F + 0.85F * std::max(0.0F, Vector3DotProduct(face.normal, light));
        // Darken toward the silhouette so the sphere reads round.
        const float limb = 0.55F + 0.45F * std::clamp(face.normal.z, 0.0F, 1.0F);
        fillConvexQuad(pts, tone(ColorBrightness(rock, -0.75F + 0.75F * lit * limb), 1.0F));

        const bool seam = planet.live && face.fracture > 0;
        const Color edge = seam ? tone(emberHot, (0.6F + 0.9F * face.fracture) * pulse) : tone(rock, 0.35F + 0.2F * lit);
        const float width = seam ? 1.0F + 1.5F * face.fracture : 1.0F;
        for (size_t i = 0; i < pts.size(); i++)
        {
            DrawLineEx(pts.at(i), pts.at((i + 1) % pts.size()), width, edge);
        }
    }

    drawRing(true);
}
} // namespace

void drawBeltPlanet(const Game& game)
{
    constexpr int32_t zonesPerSide = 3;
    constexpr float visibility = 0.25F; // how far toward full colour the planet is drawn: a solid, dim body

    const Vector2 player = game.run.player.position;
    const float zoneSize = 2.0F * GameConstants::arenaHalf / zonesPerSide;
    const auto zoneOf = [&](float v)
    {
        return std::clamp(static_cast<int32_t>(std::floor((v + GameConstants::arenaHalf) / zoneSize)), 0,
                          zonesPerSide - 1);
    };
    const int32_t zx = zoneOf(player.x);
    const int32_t zy = zoneOf(player.y);
    const Vector2 zoneCenter{.x = -GameConstants::arenaHalf + (static_cast<float>(zx) + 0.5F) * zoneSize,
                             .y = -GameConstants::arenaHalf + (static_cast<float>(zy) + 0.5F) * zoneSize};
    const Vector2 rel = Vector2Subtract(zoneCenter, player);
    // The drift is scaled so the planet slides just off screen as you reach the zone edge; it
    // only fades over the last few percent, to hide the debris ring that trails past the body.
    const float radius = static_cast<float>(game.resources.screenHeight) * 0.45F;
    const float screenHalf = static_cast<float>(std::max(game.resources.screenWidth, game.resources.screenHeight)) / 2;
    const float drift = (screenHalf + radius) / (zoneSize / 2);
    const float edge = std::max(std::abs(rel.x), std::abs(rel.y)) / (zoneSize / 2);
    const float vis = visibility * std::clamp((1.0F - edge) / 0.06F, 0.0F, 1.0F);
    if (vis <= 0.005F)
    {
        return;
    }

    // Rebuilt on entering a zone or starting a run, so every run gets a new set of planets.
    // Only some zones have one; the rest stay open space.
    constexpr float planetChance = 0.45F;
    const float seed = static_cast<float>(zy * zonesPerSide + zx) + 1.0F + game.run.backdropSeed;
    if (hashNoise(seed * 17.3F + 5.0F) > planetChance)
    {
        return;
    }
    static float cachedSeed = -1;
    static BeltPlanet planet;
    if (seed != cachedSeed)
    {
        cachedSeed = seed;
        planet = buildBeltPlanet(seed);
    }

    const Color voidColor = biomeVoidColor(game);
    const auto tone = [&](Color c, float k) { return ColorLerp(voidColor, c, vis * k); };
    drawBeltPlanetBody(planet, Vector2Add(player, Vector2Scale(rel, drift)), radius, tone);
}


namespace
{
struct DerelictStation
{
    float u;    // along the hull, -0.5 stern to 0.5 bow, in hull lengths
    float hw;   // half width, in hull lengths
    float skew; // the cut leans: the top edge sits this far ahead of the bottom
    bool aft;   // on the stern half, past the break
};
} // namespace

// One derelict capital ship: a faceted hull snapped in two, the stern half knocked askew, with a
// few superstructure blocks and dead engines. Bloom furs its edges in patches, thickest along the
// break, with a few pods rooted in the plating. `tone` maps a colour and strength to the faint
// final colour.
void drawRustbloomDerelict(Vector2 center, float angle, float length, float seed,
                           const std::function<Color(Color, float)>& tone)
{
    constexpr int32_t baseStations = 6;
    constexpr std::array<float, baseStations> baseU{0.5F, 0.36F, 0.14F, -0.12F, -0.34F, -0.5F};
    constexpr std::array<float, baseStations> baseHw{0.008F, 0.05F, 0.07F, 0.075F, 0.08F, 0.065F};
    constexpr float breakGap = 0.035F;
    constexpr float breakSkew = 0.025F;
    constexpr float furStep = 7.0F; // px between fur strands along an edge

    const Vector2 along{.x = std::cos(angle), .y = std::sin(angle)};
    const Vector2 across{.x = -along.y, .y = along.x};
    const float breakU = -0.05F + 0.15F * hashNoise(seed + 1.0F);
    const float breakRot = (hashNoise(seed + 2.0F) - 0.5F) * 0.22F;
    const float widthScale = 0.85F + 0.35F * hashNoise(seed + 3.0F);
    const auto t = static_cast<float>(GetTime());

    // Local (u, v) to world; the stern half turns about the break and drifts back from it.
    const auto toWorld = [&](float u, float v, bool aft)
    {
        if (aft)
        {
            const float du = u - breakU;
            u = breakU - breakGap + du * std::cos(breakRot) - v * std::sin(breakRot);
            v = du * std::sin(breakRot) + v * std::cos(breakRot);
        }
        return Vector2Add(
            center, Vector2Add(Vector2Scale(along, u * length), Vector2Scale(across, v * length)));
    };

    std::array<DerelictStation, baseStations + 2> st{};
    size_t count = 0;
    float breakHw = 0;
    for (size_t i = 0; i < baseStations; i++)
    {
        const float hw = baseHw.at(i) * widthScale *
                         (0.9F + 0.2F * hashNoise(seed + 10.0F + static_cast<float>(i)));
        if (i > 0 && baseU.at(i - 1) > breakU && baseU.at(i) <= breakU)
        {
            const float k = (breakU - baseU.at(i - 1)) / (baseU.at(i) - baseU.at(i - 1));
            breakHw = std::lerp(st.at(count - 1).hw, hw, k);
            st.at(count++) =
                DerelictStation{.u = breakU, .hw = breakHw, .skew = breakSkew, .aft = false};
            st.at(count++) =
                DerelictStation{.u = breakU, .hw = breakHw, .skew = breakSkew, .aft = true};
        }
        st.at(count++) =
            DerelictStation{.u = baseU.at(i), .hw = hw, .skew = 0, .aft = baseU.at(i) <= breakU};
    }

    const Vector2 light = Vector2Normalize(Vector2{.x = -0.6F, .y = -0.8F});
    const float topLit =
        Vector2DotProduct(across, light) < 0 ? 1.0F : 0.0F; // which flank faces the light
    const Color hull = ColorLerp(Palette::StructMid, Palette::RustbloomHaze, 0.55F);

    const auto furEdge = [&](Vector2 a, Vector2 b, Vector2 inside, float density, float key)
    {
        const Vector2 d = Vector2Subtract(b, a);
        const float len = Vector2Length(d);
        Vector2 n = Vector2Normalize(Vector2{.x = -d.y, .y = d.x});
        if (Vector2DotProduct(n, Vector2Subtract(Vector2Lerp(a, b, 0.5F), inside)) < 0)
        {
            n = Vector2Negate(n);
        }
        const auto strands = static_cast<int32_t>(len / furStep);
        for (int32_t j = 0; j < strands; j++)
        {
            const float s = static_cast<float>(j);
            // Patchy: a slow wave along the edge decides where the bloom has taken hold.
            const float patch = 0.5F + 0.5F * std::sin(s * 0.23F + key * 3.1F);
            if (hashNoise(key * 13.7F + s * 1.37F) > density * patch)
            {
                continue;
            }
            const Vector2 p =
                Vector2Lerp(a, b, (s + hashNoise(key + s * 2.9F)) / static_cast<float>(strands));
            const float sway = std::sin(t * 0.7F + s * 0.9F + key) * 0.12F;
            const Vector2 dir = Vector2Rotate(n, (hashNoise(key * 3.3F + s) - 0.5F) * 0.9F + sway);
            const float strand =
                length * (0.006F + 0.022F * patch * hashNoise(key * 7.1F + s * 0.61F));
            DrawLineEx(p, Vector2Add(p, Vector2Scale(dir, strand)), 1.0F,
                       tone(Palette::RustbloomAccent, 1.2F + 0.8F * patch));
        }
    };

    // Engines first, so the hull overlaps their roots.
    const DerelictStation& stern = st.at(count - 1);
    const int32_t engines = 2 + static_cast<int32_t>(hashNoise(seed + 4.0F) * 2.0F);
    for (int32_t e = 0; e < engines; e++)
    {
        const float v =
            stern.hw * (2.0F * (static_cast<float>(e) + 0.5F) / static_cast<float>(engines) - 1.0F);
        const float r = stern.hw * 0.7F / static_cast<float>(engines);
        fillConvexQuad({toWorld(-0.48F, v - r, true), toWorld(-0.48F, v + r, true),
                        toWorld(-0.53F, v + r * 1.3F, true), toWorld(-0.53F, v - r * 1.3F, true)},
                       tone(ColorBrightness(hull, -0.6F), 1.0F));
    }

    // Hull: each section split along the keel into a lit and a shadowed facet, plated in
    // slightly different tones, with faint seams on every facet edge.
    for (size_t i = 0; i + 1 < count; i++)
    {
        const DerelictStation& a = st.at(i);
        const DerelictStation& b = st.at(i + 1);
        if (a.aft != b.aft)
        {
            continue;
        }
        const float plate = 0.85F + 0.3F * hashNoise(seed + 20.0F + static_cast<float>(i));
        for (const float side : {1.0F, -1.0F})
        {
            const std::array<Vector2, 4> q{toWorld(a.u + a.skew * side, a.hw * side, a.aft),
                                           toWorld(b.u + b.skew * side, b.hw * side, b.aft),
                                           toWorld(b.u, 0, b.aft), toWorld(a.u, 0, a.aft)};
            const bool lit = (side > 0) == (topLit > 0.5F);
            fillConvexQuad(q, tone(ColorBrightness(hull, lit ? 0.0F : -0.45F), plate));
            for (size_t k = 0; k < q.size(); k++)
            {
                DrawLineEx(q.at(k), q.at((k + 1) % q.size()), 1.0F,
                           tone(Palette::StructLight, lit ? 0.9F : 0.5F));
            }
            const Vector2 inside = toWorld((a.u + b.u) / 2, 0, a.aft);
            furEdge(q[0], q[1], inside, 0.5F, seed + static_cast<float>(i) * 2.0F + side);
        }
    }

    // Superstructure: raised blocks along the keel, each casting a short shadow away from the
    // light, and a stepped bridge tower toward the stern.
    const Vector2 castShadow = Vector2Scale(light, -length * 0.008F);
    const auto drawBlock = [&](float u0, float u1, float w, bool aft, float lift)
    {
        const std::array<Vector2, 4> q{toWorld(u0, -w, aft), toWorld(u0, w, aft),
                                       toWorld(u1, w, aft), toWorld(u1, -w, aft)};
        std::array<Vector2, 4> shadow = q;
        for (auto& v : shadow)
        {
            v = Vector2Add(v, Vector2Scale(castShadow, lift));
        }
        fillConvexQuad(shadow, tone(ColorBrightness(hull, -0.7F), 1.0F));
        fillConvexQuad(q, tone(ColorBrightness(hull, 0.05F + 0.05F * lift), 1.0F));
        for (size_t k = 0; k < q.size(); k++)
        {
            DrawLineEx(q.at(k), q.at((k + 1) % q.size()), 1.0F, tone(Palette::StructLight, 0.7F));
        }
    };
    constexpr int32_t blocks = 3;
    for (int32_t k = 0; k < blocks; k++)
    {
        const float bs = seed + 30.0F + static_cast<float>(k) * 3.0F;
        const float u0 = 0.25F - 0.6F * (static_cast<float>(k) + hashNoise(bs) * 0.6F) / blocks;
        const float u1 = u0 - 0.04F - 0.08F * hashNoise(bs + 1.0F);
        const bool aft = u0 <= breakU;
        if (aft != (u1 <= breakU))
        {
            continue; // would straddle the break
        }
        drawBlock(u0, u1, 0.012F + 0.02F * hashNoise(bs + 2.0F), aft, 1.0F);
    }
    drawBlock(-0.3F, -0.42F, 0.032F, true, 1.0F);
    drawBlock(-0.33F, -0.39F, 0.018F, true, 2.0F);

    // Plating rotted away just ahead of the break, baring the dark interior and a few ribs.
    constexpr float baredLength = 0.07F;
    const float baredHw = breakHw * 0.85F;
    fillConvexQuad({toWorld(breakU + breakSkew * 0.85F, baredHw, false),
                    toWorld(breakU + baredLength, baredHw, false),
                    toWorld(breakU + baredLength, -baredHw, false),
                    toWorld(breakU - breakSkew * 0.85F, -baredHw, false)},
                   tone(ColorBrightness(hull, -0.8F), 1.0F));
    constexpr int32_t ribs = 4;
    for (int32_t k = 0; k < ribs; k++)
    {
        const float u = breakU + baredLength *
                                     (static_cast<float>(k) + 0.4F +
                                      0.5F * hashNoise(seed + 40.0F + static_cast<float>(k))) /
                                     (ribs + 1);
        DrawLineEx(toWorld(u + breakSkew * 0.85F, baredHw, false),
                   toWorld(u - breakSkew * 0.85F, -baredHw, false), 1.5F, tone(hull, 0.8F));
    }

    // The break: both cut faces thick with bloom, and a few torn-off chunks adrift in the gap.
    for (size_t i = 0; i + 1 < count; i++)
    {
        const DerelictStation& a = st.at(i);
        if (a.u != breakU)
        {
            continue;
        }
        const Vector2 top = toWorld(a.u + a.skew, a.hw, a.aft);
        const Vector2 bottom = toWorld(a.u - a.skew, -a.hw, a.aft);
        const Vector2 inside = toWorld(a.aft ? a.u - 0.05F : a.u + 0.05F, 0, a.aft);
        furEdge(top, bottom, inside, 1.0F, seed + (a.aft ? 50.0F : 60.0F));
    }
    constexpr int32_t chunks = 3;
    for (int32_t k = 0; k < chunks; k++)
    {
        const float cs = seed + 70.0F + static_cast<float>(k) * 5.0F;
        const Vector2 p = toWorld(breakU - breakGap * (0.2F + 0.6F * hashNoise(cs)),
                                  (hashNoise(cs + 1.0F) - 0.5F) * 0.12F, false);
        DrawPoly(p, 3 + k % 2, length * (0.006F + 0.008F * hashNoise(cs + 2.0F)),
                 hashNoise(cs + 3.0F) * 360.0F, tone(hull, 0.9F));
    }

    // Pods rooted in the plating, clustered toward the break where the rot began.
    constexpr int32_t pods = 6;
    for (int32_t k = 0; k < pods; k++)
    {
        const float ps = seed + 90.0F + static_cast<float>(k) * 4.0F;
        const float u = breakU + (hashNoise(ps) - 0.5F) * 0.5F * hashNoise(ps + 1.0F);
        const bool aft = u <= breakU;
        const Vector2 p = toWorld(u, (hashNoise(ps + 2.0F) - 0.5F) * 0.1F * widthScale, aft);
        const float r = length * (0.006F + 0.012F * hashNoise(ps + 3.0F));
        DrawCircleV(p, r, tone(Palette::RustbloomAccent, 1.1F));
        DrawCircleV(Vector2Add(p, Vector2Scale(light, -r * 0.3F)), r * 0.55F,
                    tone(Palette::RustbloomAccent, 0.7F));
    }
}

// Rustbloom far layer (parallax 0.05): derelict capital ships in the dark, drawn before the stars
// like the Belt planets. At this parallax a ship takes much of the arena to cross the screen, so
// they only ever slide in and out at the edges.
void drawRustbloomDerelicts(const Game& game)
{
    constexpr float parallax = 0.05F;
    constexpr float cellSize = 1100.0F;
    constexpr float shipChance = 0.6F;
    constexpr float visibility = 0.1F; // how far toward full colour the ships are drawn

    const Vector2 player = game.run.player.position;
    const Vector2 layerCenter = Vector2Scale(player, parallax);
    const Vector2 shift = Vector2Scale(player, 1.0F - parallax);
    const float halfW = static_cast<float>(game.resources.screenWidth) / 2 + cellSize;
    const float halfH = static_cast<float>(game.resources.screenHeight) / 2 + cellSize;
    const Color voidColor = biomeVoidColor(game);
    const std::function<Color(Color, float)> tone = [&](Color c, float k)
    { return ColorLerp(voidColor, c, std::min(1.0F, visibility * k)); };

    for (float cy = std::floor((layerCenter.y - halfH) / cellSize);
         cy * cellSize < layerCenter.y + halfH; cy++)
    {
        for (float cx = std::floor((layerCenter.x - halfW) / cellSize);
             cx * cellSize < layerCenter.x + halfW; cx++)
        {
            const float seed = cx * 7.13F + cy * 3.71F + game.run.backdropSeed;
            if (hashNoise(seed * 1.7F + 2.0F) > shipChance)
            {
                continue;
            }
            const Vector2 jitter{.x = (hashNoise(seed + 5.0F) - 0.5F) * cellSize * 0.3F,
                                 .y = (hashNoise(seed + 6.0F) - 0.5F) * cellSize * 0.3F};
            const Vector2 center = Vector2Add(
                shift, Vector2Add(Vector2{.x = cx * cellSize, .y = cy * cellSize}, jitter));
            // Mostly lying across the screen, headed either way.
            const float heading =
                (hashNoise(seed + 7.0F) - 0.5F) * 0.9F +
                (hashNoise(seed + 8.0F) < 0.5F ? 0.0F : std::numbers::pi_v<float>);
            const float length = cellSize * (0.6F + 0.2F * hashNoise(seed + 9.0F));
            drawRustbloomDerelict(center, heading, length, seed, tone);
        }
    }
}

// Rustbloom mid (0.3) and near (0.8) layers, drawn over the far derelicts (which hide the stars).
void drawRustbloomBackground(const Game& game)
{
    if (currentBiome(game.run.waveNumber) != Biome::Rustbloom)
    {
        return;
    }

    const auto t = static_cast<float>(GetTime());
    const Vector2 player = game.run.player.position;
    const Color voidColor = biomeVoidColor(game);

    // Mid (0.3): drifting hull fragments, and spore motes circling slowly in place.
    {
        constexpr float parallax = 0.3F;
        constexpr float fragmentCellSize = 260.0F;
        constexpr float fragmentChance = 0.12F;
        constexpr float fragmentStrength = 0.25F; // toward full colour from the void
        const Vector2 fragArea = Vector2Scale(player, parallax);
        const Vector2 shift = Vector2Scale(player, 1.0F - parallax);
        const float halfW = static_cast<float>(game.resources.screenWidth) / 2 + fragmentCellSize;
        const float halfH = static_cast<float>(game.resources.screenHeight) / 2 + fragmentCellSize;
        const float fragStartX =
            std::floor((fragArea.x - halfW) / fragmentCellSize) * fragmentCellSize;
        const float fragStartY =
            std::floor((fragArea.y - halfH) / fragmentCellSize) * fragmentCellSize;
        const Color fragColor =
            ColorLerp(voidColor, ColorLerp(Palette::StructMid, Palette::RustbloomHaze, 0.3F),
                      fragmentStrength);
        for (float y = fragStartY; y < fragArea.y + halfH; y += fragmentCellSize)
        {
            for (float x = fragStartX; x < fragArea.x + halfW; x += fragmentCellSize)
            {
                const float presence = hashNoise(x * 0.011F + y * 0.023F + 50.0F);
                if (presence > fragmentChance)
                {
                    continue;
                }
                const float jx =
                    (hashNoise(x * 0.031F + y * 0.013F + 61.0F) - 0.5F) * fragmentCellSize * 0.5F;
                const float jy =
                    (hashNoise(x * 0.013F + y * 0.031F + 67.0F) - 0.5F) * fragmentCellSize * 0.5F;
                const Vector2 fragCenter{.x = x + fragmentCellSize / 2 + jx + shift.x,
                                         .y = y + fragmentCellSize / 2 + jy + shift.y};
                const float fragRadius = fragmentCellSize * (0.22F + presence * 0.4F);
                const float rot = hashNoise(presence * 71.0F) * 360.0F;

                const float shapeRoll = hashNoise(presence * 53.0F + 4.0F);
                const int32_t sides =
                    shapeRoll < 0.2F ? 0 : 3 + static_cast<int32_t>(shapeRoll * 7.5F) % 6;
                drawPanelShape(fragCenter, fragRadius, sides, rot, fragColor, presence * 90.0F);
            }
        }

        constexpr float moteCellSize = 200.0F;
        constexpr float moteChance = 0.22F;
        constexpr float moteOrbit = 26.0F;
        const float moteStartX = std::floor((fragArea.x - halfW) / moteCellSize) * moteCellSize;
        const float moteStartY = std::floor((fragArea.y - halfH) / moteCellSize) * moteCellSize;
        for (float y = moteStartY; y < fragArea.y + halfH; y += moteCellSize)
        {
            for (float x = moteStartX; x < fragArea.x + halfW; x += moteCellSize)
            {
                const float m = hashNoise(x * 0.019F + y * 0.041F + 131.0F);
                if (m > moteChance)
                {
                    continue;
                }
                const float phase = m * 97.0F;
                const Vector2 pos{.x = x + moteCellSize * hashNoise(phase + 1.0F) +
                                       std::sin(t * 0.21F + phase) * moteOrbit + shift.x,
                                  .y = y + moteCellSize * hashNoise(phase + 2.0F) +
                                       std::cos(t * 0.17F + phase) * moteOrbit + shift.y};
                const float r = 1.2F + 1.4F * hashNoise(phase + 3.0F);
                const float glow = 0.8F + 0.2F * std::sin(t * 0.9F + phase);
                DrawCircleV(pos, r * 3.5F, Fade(Palette::RustbloomAccent, 0.03F * glow));
                DrawCircleV(pos, r, Fade(Palette::RustbloomAccent, 0.3F * glow));
            }
        }
    }

    // Near (0.8): bloom pods strung together by tendrils, both breathing slowly.
    constexpr float parallax = 0.8F;
    constexpr float cellSize = rustbloomPodCellSize;
    const float halfW = static_cast<float>(game.resources.screenWidth) / 2 + cellSize;
    const float halfH = static_cast<float>(game.resources.screenHeight) / 2 + cellSize;
    const Vector2 center = Vector2Scale(player, parallax);
    const Vector2 shift = Vector2Scale(player, 1.0F - parallax);
    const float startX = std::floor((center.x - halfW) / cellSize) * cellSize;
    const float startY = std::floor((center.y - halfH) / cellSize) * cellSize;

    constexpr float podChance = rustbloomPodChance;
    auto podPresence = [](float px, float py) -> float
    { return hashNoise(px * 0.021F + py * 0.037F); };
    auto podPulse = [&](float px, float py) -> float
    { return 0.5F + 0.5F * std::sin(t * 1.5F + px * 0.01F + py * 0.01F); };
    auto podEnergy = [&](float px, float py) -> float
    {
        const auto it = game.run.podJiggle.find(rustbloomPodKey(px, py, cellSize));
        return it == game.run.podJiggle.end() ? 0.0F : it->second;
    };

    for (float y = startY; y < center.y + halfH; y += cellSize)
    {
        for (float x = startX; x < center.x + halfW; x += cellSize)
        {
            if (podPresence(x, y) > podChance)
            {
                continue;
            }
            const float jx = (hashNoise(x * 0.05F + y * 0.02F + 3.0F) - 0.5F) * cellSize * 0.5F;
            const float jy = (hashNoise(x * 0.02F + y * 0.05F + 7.0F) - 0.5F) * cellSize * 0.5F;
            const Vector2 podCenter{.x = x + cellSize / 2 + jx + shift.x,
                                    .y = y + cellSize / 2 + jy + shift.y};
            const float pulse = podPulse(x, y);

            const float energy = podEnergy(x, y);

            // A tendril carries the blend of its two pods' pulses, and waves when either is stirred.
            const auto tendril = [&](float nx, float ny, Vector2 neighbor)
            {
                const float flow = (pulse + podPulse(nx, ny)) / 2;
                const Color color = Fade(Palette::RustbloomHaze, 0.12F + 0.1F * flow);
                const float wave = std::max(energy, podEnergy(nx, ny)) * 12.0F;
                const auto pulled = game.run.tendrilPull.find(rustbloomPodKey(x, y, cellSize) * 2 + (ny == y ? 0 : 1));
                const bool isPulled = pulled != game.run.tendrilPull.end();
                if (wave < 0.1F && !isPulled)
                {
                    DrawLineEx(podCenter, neighbor, 2.5F, color);
                    return;
                }
                const Vector2 dir = Vector2Subtract(neighbor, podCenter);
                const Vector2 side = Vector2Normalize(Vector2{.x = -dir.y, .y = dir.x});
                Vector2 prev = podCenter;
                constexpr int32_t steps = 14;
                for (int32_t i = 1; i <= steps; i++)
                {
                    const float s = static_cast<float>(i) / steps;
                    const float bend = wave * std::sin(s * PI) * std::sin(t * 14.0F + s * 7.0F + x * 0.01F);
                    Vector2 p = Vector2Add(Vector2Add(podCenter, Vector2Scale(dir, s)), Vector2Scale(side, bend));
                    if (isPulled) // hooked or springing back: bent toward its offset, most at the hook
                    {
                        const float at = pulled->second.at;
                        const float k = s < at ? s / at : (1.0F - s) / (1.0F - at);
                        p = Vector2Add(p, Vector2Scale(pulled->second.offset, std::sin(k * PI / 2)));
                    }
                    DrawLineEx(prev, p, 2.5F, color);
                    prev = p;
                }
            };
            if (podPresence(x + cellSize, y) <= podChance)
            {
                tendril(x + cellSize, y, Vector2{.x = x + cellSize * 1.5F + shift.x, .y = y + cellSize / 2 + shift.y});
            }
            if (podPresence(x, y + cellSize) <= podChance)
            {
                tendril(x, y + cellSize, Vector2{.x = x + cellSize / 2 + shift.x, .y = y + cellSize * 1.5F + shift.y});
            }

            const float sizeSeed = hashNoise(x * 0.043F + y * 0.017F + 9.0F);
            const float podRadius = cellSize * (0.14F + sizeSeed * 0.06F) * (0.94F + 0.06F * pulse);
            // Stirred pods wobble like jelly: the outline ripples and the core sloshes about.
            const float phase = x * 0.013F + y * 0.029F;
            const Vector2 slosh{.x = std::sin(t * 17.0F + phase) * energy * 9.0F,
                                .y = std::cos(t * 13.0F + phase) * energy * 9.0F};
            if (energy > 0.01F)
            {
                constexpr int32_t rim = 20;
                const auto rimPoint = [&](int32_t i)
                {
                    const float a = static_cast<float>(i) / rim * 2.0F * PI;
                    const float r = podRadius * (1.0F + energy * (0.28F * std::sin(3.0F * a + t * 15.0F) +
                                                                  0.12F * std::sin(5.0F * a - t * 11.0F)));
                    return Vector2Add(podCenter, Vector2{.x = std::cos(a) * r, .y = std::sin(a) * r});
                };
                const Color body = Fade(Palette::RustbloomAccent, 0.22F);
                for (int32_t i = 0; i < rim; i++)
                {
                    DrawTriangle(podCenter, rimPoint(i), rimPoint(i + 1), body); // both windings: raylib culls one
                    DrawTriangle(podCenter, rimPoint(i + 1), rimPoint(i), body);
                }
            }
            else
            {
                DrawCircleV(podCenter, podRadius, Fade(Palette::RustbloomAccent, 0.22F));
            }
            DrawCircleV(
                Vector2Add(Vector2Add(podCenter, Vector2{.x = podRadius * 0.5F, .y = podRadius * 0.3F}), Vector2Scale(slosh, 0.6F)),
                podRadius * 0.6F, Fade(Palette::RustbloomHaze, 0.2F));
            DrawCircleV(Vector2Add(podCenter, slosh), podRadius * 0.3F, Fade(Palette::Heal, 0.2F + 0.3F * pulse));
        }
    }
}

// Fire trail behind a Debris Squall rock, drawn like the Flamethrower's particles: soft red and
// pale-gold puffs with a red core, flickering, shrinking and fading as they fall behind.
void drawSquallRockFlame(Vector2 pos, Vector2 along, Vector2 across, float radius, float seed)
{
    constexpr int32_t puffCount = 6;
    const auto t = static_cast<float>(GetTime());
    for (int32_t k = puffCount - 1; k >= 0; k--)
    {
        const float back = static_cast<float>(k) / puffCount; // 0 at the rock, 1 at the tail
        const float fade = 1.0F - back;
        const float wobble = std::sin(t * 22.0F + seed + static_cast<float>(k) * 1.7F) * radius * 0.35F * back;
        const Vector2 p = Vector2Add(Vector2Subtract(pos, Vector2Scale(along, radius * (0.6F + 6.0F * back))),
                                     Vector2Scale(across, wobble));
        const float flicker = 0.75F + 0.25F * std::sin(t * 24.0F + p.x * 0.1F + p.y * 0.1F);
        const float r = radius * (1.1F + 0.6F * back) * fade + radius * 0.3F;
        const Color color = (k + static_cast<int32_t>(seed)) % 2 == 0 ? Palette::Accent : Palette::Crit;
        DrawCircleV(p, r, Fade(color, 0.22F * fade * flicker));
        DrawCircleV(p, r * 0.5F, Fade(Palette::Accent, 0.35F * fade));
    }
}

// A Debris Squall rock: an irregular six-sided body cut into facets, each shaded by which way it
// faces (lit, mid or shadow), the trailing face warmed by the fire, plus one crater.
void drawSquallRock(Vector2 pos, Vector2 along, float radius, float rotationDeg, float seed)
{
    constexpr int32_t sides = 6;
    const Vector2 light = Vector2Normalize(Vector2{.x = -0.6F, .y = -0.8F});
    std::array<Vector2, sides> pts{};
    for (int32_t i = 0; i < sides; i++)
    {
        const float deg = rotationDeg + static_cast<float>(i) * (360.0F / sides) +
                          (hashNoise(seed + 10.0F + static_cast<float>(i)) - 0.5F) * 30.0F;
        const float r = radius * (0.75F + 0.4F * hashNoise(seed + 20.0F + static_cast<float>(i)));
        pts.at(i) = Vector2Add(pos, Vector2{.x = std::cos(deg * DEG2RAD) * r, .y = std::sin(deg * DEG2RAD) * r});
    }
    for (int32_t i = 0; i < sides; i++)
    {
        const Vector2 a = pts.at(i);
        const Vector2 b = pts.at((i + 1) % sides);
        const Vector2 normal = Vector2Normalize(Vector2Subtract(Vector2Lerp(a, b, 0.5F), pos));
        const float lit = Vector2DotProduct(normal, light);
        Color c = lit > 0 ? ColorLerp(Palette::StructMid, Palette::StructLight, lit * 0.8F)
                          : ColorLerp(Palette::StructMid, Palette::Void, -lit * 0.75F);
        c = ColorLerp(c, Palette::ElementBurn, std::max(0.0F, -Vector2DotProduct(normal, along)) * 0.35F);
        const float cross = (a.x - pos.x) * (b.y - pos.y) - (a.y - pos.y) * (b.x - pos.x);
        if (cross < 0)
        {
            DrawTriangle(pos, a, b, c);
        }
        else
        {
            DrawTriangle(pos, b, a, c);
        }
    }
    const float craterDeg = hashNoise(seed + 30.0F) * 360.0F * DEG2RAD;
    DrawCircleV(Vector2Add(pos, Vector2{.x = std::cos(craterDeg) * radius * 0.3F, .y = std::sin(craterDeg) * radius * 0.3F}),
                radius * 0.2F, Fade(Palette::Void, 0.45F));
}

void drawDebrisSquall(const Game& game)
{
    if (game.run.debrisSquallPhase == 0)
    {
        return;
    }

    const float angle = game.run.debrisSquallAngle;
    const Vector2 along{.x = std::cos(angle), .y = std::sin(angle)};
    const Vector2 across{.x = -along.y, .y = along.x};
    const Vector2 center = game.run.debrisSquallCenter;
    const auto at = [&](float s, float off)
    { return Vector2Add(center, Vector2Add(Vector2Scale(along, s), Vector2Scale(across, off))); };
    const Color amber = Palette::ShatteredBeltAccent;
    // The band runs past both screen edges wherever the player goes: it is drawn around the
    // player's own position along it, so moving along the band never reveals an end.
    const float playerAlong = Vector2DotProduct(Vector2Subtract(game.run.player.position, center), along);
    const float reach = debrisSquallReach;

    if (game.run.debrisSquallPhase == 1)
    {
        const auto t = static_cast<float>(GetTime());
        const float pulse = 0.5F + 0.5F * std::sin(t * 18.0F);
        const float s0 = playerAlong - reach;
        const float s1 = playerAlong + reach;
        DrawTriangle(at(s0, -debrisSquallHalfWidth), at(s1, debrisSquallHalfWidth),
                     at(s1, -debrisSquallHalfWidth), Fade(amber, 0.06F + 0.05F * pulse));
        DrawTriangle(at(s0, -debrisSquallHalfWidth), at(s0, debrisSquallHalfWidth),
                     at(s1, debrisSquallHalfWidth), Fade(amber, 0.06F + 0.05F * pulse));
        DrawLineEx(at(s0, -debrisSquallHalfWidth), at(s1, -debrisSquallHalfWidth), 2.0F,
                   Fade(Palette::Accent, 0.4F + 0.4F * pulse));
        DrawLineEx(at(s0, debrisSquallHalfWidth), at(s1, debrisSquallHalfWidth), 2.0F,
                   Fade(Palette::Accent, 0.4F + 0.4F * pulse));

        // Edge warning: chevrons where the band enters the screen, pointing along the sweep.
        const auto halfW = static_cast<float>(game.resources.screenWidth) / 2 - 40;
        const auto halfH = static_cast<float>(game.resources.screenHeight) / 2 - 40;
        const Vector2 rel = Vector2Subtract(center, game.run.player.position);
        float exitT = std::numeric_limits<float>::max();
        if (along.x != 0)
        {
            exitT = std::min(exitT, (along.x > 0 ? rel.x + halfW : rel.x - halfW) / along.x);
        }
        if (along.y != 0)
        {
            exitT = std::min(exitT, (along.y > 0 ? rel.y + halfH : rel.y - halfH) / along.y);
        }
        for (int32_t i = 0; i < 3; i++)
        {
            const float s = -exitT + static_cast<float>(i) * 28.0F;
            DrawTriangle(at(s + 22, 0), at(s, -22), at(s, 22),
                         Fade(Palette::Accent, (0.5F + 0.5F * pulse) * (1.0F - 0.25F * static_cast<float>(i))));
        }
        return;
    }

    for (int32_t i = 0; i < debrisSquallRockCount; i++)
    {
        const SquallRock rock = debrisSquallRock(game, i);
        if (!rock.live)
        {
            continue;
        }
        drawSquallRockFlame(rock.position, along, across, rock.radius, rock.seed);
        drawSquallRock(rock.position, along, rock.radius,
                       hashNoise(rock.seed + 3.0F) * 360.0F + game.run.debrisSquallFront * 0.3F, rock.seed);
    }
}

void drawRustPatches(const Game& game)
{
    constexpr int32_t sides = 14;
    const auto t = static_cast<float>(GetTime());
    for (const auto& patch : game.run.rustPatches)
    {
        const float ripe = std::clamp(patch.age / rustPatchRipenTime, 0.0F, 1.0F);
        const bool playerIn = ripe >= 1.0F && CheckCollisionCircles(game.run.player.position,
                                                                    game.run.player.radius * 0.5F,
                                                                    patch.position, patch.radius);
        // Ragged rim: each vertex keeps its own hashed reach and breathes slowly.
        std::array<Vector2, sides> rim{};
        for (int32_t i = 0; i < sides; i++)
        {
            const float a = static_cast<float>(i) / sides * 2.0F * PI + patch.seed;
            const float reach = 0.78F + 0.22F * hashNoise(patch.seed + static_cast<float>(i)) +
                                0.04F * std::sin(t * 0.8F + static_cast<float>(i) * 1.7F);
            rim.at(static_cast<size_t>(i)) =
                Vector2Add(patch.position, Vector2{.x = std::cos(a) * patch.radius * reach,
                                                   .y = std::sin(a) * patch.radius * reach});
        }
        for (int32_t i = 0; i < sides; i++)
        {
            const Vector2 a = rim.at(static_cast<size_t>(i));
            const Vector2 b = rim.at(static_cast<size_t>((i + 1) % sides));
            DrawTriangle(patch.position, b, a, Fade(Palette::RustbloomHaze, 0.3F * ripe));
            DrawTriangle(patch.position, Vector2Lerp(patch.position, b, 0.55F),
                         Vector2Lerp(patch.position, a, 0.55F),
                         Fade(Palette::RustbloomHaze, 0.22F * ripe));
            // Ripening: a dashed, pulsing rim is the tell; once ripe the rim is solid.
            if (ripe < 1.0F && i % 2 == 1)
            {
                continue;
            }
            const Color rimColor = playerIn ? Palette::Accent : Palette::RustbloomAccent;
            const float rimAlpha =
                ripe < 1.0F ? 0.35F + 0.35F * (0.5F + 0.5F * std::sin(t * 14.0F)) : 0.7F;
            DrawLineEx(a, b, 1.5F, Fade(rimColor, rimAlpha));
        }
        // Spore specks scattered inside, flickering out of phase.
        for (int32_t i = 0; i < 5; i++)
        {
            const float s = patch.seed + 40.0F + static_cast<float>(i) * 3.1F;
            const float a = hashNoise(s) * 2.0F * PI;
            const float d = hashNoise(s + 1.0F) * patch.radius * 0.6F;
            const Vector2 p =
                Vector2Add(patch.position, Vector2{.x = std::cos(a) * d, .y = std::sin(a) * d});
            const float flicker = 0.5F + 0.5F * std::sin(t * 2.5F + s);
            DrawRectangleV(Vector2Subtract(p, Vector2{.x = 1.5F, .y = 1.5F}),
                           Vector2{.x = 3, .y = 3},
                           Fade(Palette::RustbloomAccent, (0.3F + 0.4F * flicker) * ripe));
        }
    }
}

void drawPunctumLightning(const Game& game)
{
    if (game.run.punctumThunderFlashTimer <= 0)
    {
        return;
    }

    const float frac = game.run.punctumThunderFlashTimer / punctumThunderFlashDuration;
    const Vector2 start = game.run.punctumThunderBoltOrigin;
    const Vector2 toPlayer = Vector2Subtract(game.run.player.position, start);
    const float length = Vector2Length(toPlayer);
    if (length < 1.0F)
    {
        return;
    }
    const Vector2 dir = Vector2Scale(toPlayer, 1.0F / length);
    const Vector2 perp{.x = -dir.y, .y = dir.x};

    constexpr int32_t segments = 9;
    Vector2 prev = start;
    for (int32_t i = 1; i <= segments; i++)
    {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float jitterSeed = game.run.punctumThunderBoltSeed + static_cast<float>(i) * 3.7F;
        const float jitter = (hashNoise(jitterSeed) - 0.5F) * length * 0.12F;
        const Vector2 point = Vector2Add(Vector2Add(start, Vector2Scale(dir, length * t)),
                                         Vector2Scale(perp, jitter));
        DrawLineEx(prev, point, 3.0F * frac, Fade(WHITE, 0.7F * frac));
        DrawLineEx(prev, point, 7.0F * frac, Fade(Palette::PunctumAccent, 0.35F * frac));

        if (i > 1 && i < segments && hashNoise(jitterSeed + 50.0F) > 0.6F)
        {
            const float branchAngle = (hashNoise(jitterSeed + 90.0F) - 0.5F) * 90.0F * DEG2RAD;
            const Vector2 branchDir{
                .x = dir.x * std::cos(branchAngle) - dir.y * std::sin(branchAngle),
                .y = dir.x * std::sin(branchAngle) + dir.y * std::cos(branchAngle)};
            const Vector2 branchEnd = Vector2Add(point, Vector2Scale(branchDir, length * 0.12F));
            DrawLineEx(point, branchEnd, 2.0F * frac, Fade(WHITE, 0.5F * frac));
        }

        prev = point;
    }
}

void drawBlackHoleGradient(Vector2 center, float radius)
{
    constexpr float coreFraction = 0.22F;
    constexpr int32_t edgeSteps = 4;
    const float coreRadius = radius * coreFraction;

    DrawCircleV(center, coreRadius, BLACK);
    for (int32_t g = edgeSteps; g >= 1; g--)
    {
        const float t = static_cast<float>(g) / static_cast<float>(edgeSteps);
        DrawCircleV(center, coreRadius + coreRadius * 0.15F * t, Fade(BLACK, t * 0.4F));
    }
}

// ponytail: kept as a backup look (spiral arm lines radiating past the disc) in case the
// twirl-texture version below isn't what's wanted; not currently called.
[[maybe_unused]] void drawBlackHoleSpiralArmsBackup(Vector2 center, float radius, Color color)
{
    constexpr int32_t armCount = 9;
    constexpr int32_t segments = 14;
    const float rotation = static_cast<float>(GetTime()) * 6.0F;

    drawBlackHoleGradient(center, radius);

    for (int32_t i = 0; i < armCount; i++)
    {
        const float seed = static_cast<float>(i) * 2.7F;
        const float baseAngle =
            static_cast<float>(i) / static_cast<float>(armCount) * 360.0F + rotation;
        const float lengthMult = 0.65F + 0.35F * std::sin(seed * 1.7F);
        const float curl = 70.0F + 20.0F * std::sin(seed);

        Vector2 prev = center;
        for (int32_t s = 1; s <= segments; s++)
        {
            const float t = static_cast<float>(s) / static_cast<float>(segments);
            const float r = radius * (0.12F + 0.95F * lengthMult * t);
            const float a = (baseAngle + curl * t) * DEG2RAD;
            const Vector2 p =
                Vector2Add(center, Vector2{.x = std::cos(a) * r, .y = std::sin(a) * r});
            DrawLineEx(prev, p, 2.2F, Fade(color, 0.55F * (1.0F - t * 0.3F)));
            prev = p;
        }
    }
}

void drawBlackHoleTwirl(Vector2 center, float radius,
                        const std::vector<BlackHoleDustParticle>& dust)
{
    drawBlackHoleGradient(center, radius);

    constexpr int32_t ringDotCount = 140;
    constexpr float ringWidth = 0.012F;
    for (int32_t i = 0; i < ringDotCount; i++)
    {
        const float u = static_cast<float>(GetRandomValue(0, 1000)) / 1000.0F;
        const float r = blackHoleDustCoreFraction + ringWidth * u;
        const float angle = static_cast<float>(GetRandomValue(0, 36000)) / 100.0F * DEG2RAD;
        const Vector2 p = Vector2Add(
            center, Vector2{.x = std::cos(angle) * r * radius, .y = std::sin(angle) * r * radius});
        const auto brightness = static_cast<float>(GetRandomValue(70, 100)) / 100.0F;
        DrawCircleV(p, 1.2F, Fade(WHITE, brightness));
    }

    for (const auto& d : dust)
    {
        const float twist = blackHoleDustTwistStrength * (1.0F - d.radiusFrac);
        const float angle = d.armPeakAngle + d.jitter - twist;
        const Vector2 p = Vector2Add(center, Vector2{.x = std::cos(angle) * d.radiusFrac * radius,
                                                     .y = std::sin(angle) * d.radiusFrac * radius});
        const float dotSize = 0.7F + 0.9F * (1.0F - d.radiusFrac);
        DrawCircleV(p, dotSize, Fade(d.color, d.brightness));
    }
}

void drawKrakenRemains(const Game& game)
{
    const Vector2 center{};
    constexpr float radius = 220.0F;

    if (game.run.bossCutscenePhase == 11)
    {
        const float blink = std::sin(static_cast<float>(GetTime()) * 30.0F) > 0 ? 1.0F : 0.0F;
        const Color color = ColorLerp(Palette::StructDark, WHITE, blink * 0.5F);
        drawKrakenShape(center, radius, 7.0F, color);

        constexpr int32_t crackCount = 6;
        for (int32_t i = 0; i < crackCount; i++)
        {
            const float seed = static_cast<float>(i) * 13.0F;
            const float angle = hashNoise(seed) * 2.0F * std::numbers::pi_v<float>;
            const Vector2 start =
                Vector2Add(center, Vector2{.x = std::cos(angle) * radius * 0.15F,
                                           .y = std::sin(angle) * radius * 0.15F});
            const Vector2 end = Vector2Add(center, Vector2{.x = std::cos(angle) * radius * 0.95F,
                                                           .y = std::sin(angle) * radius * 0.95F});
            DrawLineEx(start, end, 2.5F, Fade(Palette::Void, 0.75F));
        }
        return;
    }

    if (game.run.bossCutscenePhase == 12)
    {
        const float progress =
            std::clamp(1.0F - game.run.bossCutsceneTimer / banishedBreakApartDuration, 0.0F, 1.0F);

        drawKrakenShape(center, radius * (1.0F - progress * 0.5F), 7.0F,
                        Fade(Palette::StructDark, 1.0F - progress));

        const auto crackCount = static_cast<int32_t>(4 + progress * 10.0F);
        for (int32_t i = 0; i < crackCount; i++)
        {
            const float seed = static_cast<float>(i) * 13.0F;
            const float angle = hashNoise(seed) * 2.0F * std::numbers::pi_v<float>;
            const Vector2 start =
                Vector2Add(center, Vector2{.x = std::cos(angle) * radius * 0.15F,
                                           .y = std::sin(angle) * radius * 0.15F});
            const Vector2 end = Vector2Add(center, Vector2{.x = std::cos(angle) * radius * 0.95F,
                                                           .y = std::sin(angle) * radius * 0.95F});
            DrawLineEx(start, end, 2.5F, Fade(Palette::Void, 0.75F * (1.0F - progress)));
        }

        constexpr int32_t fragmentCount = 8;
        for (int32_t i = 0; i < fragmentCount; i++)
        {
            const float seed = static_cast<float>(i) * 19.0F;
            const float angle = hashNoise(seed) * 2.0F * std::numbers::pi_v<float>;
            const float dist = radius * 0.5F * progress;
            const Vector2 pos = Vector2Add(
                center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
            const float rot = hashNoise(seed + 1.0F) * 360.0F;
            DrawPoly(pos, 5, radius * 0.16F * (1.0F - progress * 0.5F), rot,
                     Fade(Palette::StructDark, 1.0F - progress));
        }
    }
}

} // namespace DrawDetail

using namespace DrawDetail;

// One background object from another Biome, drawn into a Punctum wreckage sprite (see
// punctum_infall.cpp) with its own Biome's drawing code, at fuller strength than in its Biome.
void drawInfallSprite(InfallKind kind, float seed, Vector2 center, float size)
{
    const std::function<Color(Color, float)> tone = [](Color c, float k)
    { return ColorLerp(Palette::Void, c, std::min(1.0F, 0.85F * k)); };
    const auto h = [seed](float k) { return hashNoise(seed * 13.1F + k); };
    switch (kind)
    {
    case InfallKind::BeltPlanet:
        drawBeltPlanetBody(buildBeltPlanet(seed), center, size / infallPlanetSpriteRatio, tone);
        break;
    case InfallKind::Derelict:
        drawRustbloomDerelict(center, (h(1) - 0.5F) * 0.6F, size * 0.88F, seed, tone);
        break;
    case InfallKind::HullFragment:
        drawPanelShape(center, size * 0.4F, 3 + GetRandomValue(0, 5), h(2) * 360.0F,
                       tone(ColorLerp(Palette::StructMid, Palette::RustbloomHaze, 0.3F), 1.0F), seed);
        break;
    case InfallKind::BloomPods:
    {
        // Three pods strung on tendrils, as on Rustbloom's near layer.
        std::array<Vector2, 3> pods{};
        for (size_t i = 0; i < pods.size(); i++)
        {
            const float a = h(3.0F + static_cast<float>(i)) * 2.0F * PI;
            pods.at(i) = Vector2Add(center, Vector2{.x = std::cos(a) * size * 0.26F, .y = std::sin(a) * size * 0.26F}); // ~140 apart, as in Rustbloom
        }
        DrawLineEx(pods.at(0), pods.at(1), 3.0F, tone(Palette::RustbloomHaze, 0.5F));
        DrawLineEx(pods.at(1), pods.at(2), 3.0F, tone(Palette::RustbloomHaze, 0.5F));
        for (size_t i = 0; i < pods.size(); i++)
        {
            const float r = rustbloomPodCellSize * (0.14F + 0.06F * h(7.0F + static_cast<float>(i))); // Rustbloom pod radius
            DrawCircleV(pods.at(i), r, tone(Palette::RustbloomAccent, 0.75F));
            DrawCircleV(Vector2Add(pods.at(i), Vector2{.x = r * 0.5F, .y = r * 0.3F}), r * 0.6F,
                        tone(Palette::RustbloomHaze, 0.6F));
            DrawCircleV(pods.at(i), r * 0.3F, tone(Palette::Heal, 0.9F));
        }
        break;
    }
    case InfallKind::GelBlobs:
    {
        // A clump of Solar Forge cave gel: dark red globules with a sheen, one with an ember.
        const int32_t count = 6 + GetRandomValue(0, 3);
        for (int32_t i = 0; i < count; i++)
        {
            const auto k = static_cast<float>(i);
            const float a = h(11.0F + k) * 2.0F * PI;
            const float d = size * 0.22F * std::sqrt(h(21.0F + k));
            const Vector2 c = Vector2Add(center, Vector2{.x = std::cos(a) * d, .y = std::sin(a) * d});
            const float r = size * (0.12F + 0.07F * h(31.0F + k));
            const Color rock = ColorLerp(Palette::SolarForgeAccent, Palette::Void, 0.45F + 0.25F * h(41.0F + k));
            DrawCircleV(c, r, rock);
            DrawCircleV(Vector2Add(c, Vector2{.x = -r * 0.28F, .y = -r * 0.28F}), r * 0.42F, ColorLerp(rock, WHITE, 0.12F));
            DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), r, ColorLerp(rock, WHITE, 0.25F));
            if (i == 0)
            {
                DrawCircleV(c, r * 0.3F, ColorLerp(Palette::ElementBurn, YELLOW, 0.3F));
            }
        }
        break;
    }
    }
}
