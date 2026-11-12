#include "draw_internal.hpp"

namespace DrawDetail
{

// Laser bolt: a comet streak, faint thin tail to a bright head.
void drawLaserBolt(const Bullet& b)
{
    const LaserShape shape = laserShape(b);
    constexpr int steps = 5;
    for (int i = 0; i < steps; i++)
    {
        const float t0 = static_cast<float>(i) / steps;
        const float t1 = static_cast<float>(i + 1) / steps;
        DrawLineEx(Vector2Lerp(shape.points[0], shape.points[1], t0),
                   Vector2Lerp(shape.points[0], shape.points[1], t1),
                   b.halfWidth * 2.0F * (0.3F + 0.7F * t1), Fade(b.color, 0.08F + 0.92F * t1 * t1));
    }
    DrawCircleV(shape.points[1], b.halfWidth * 1.1F, Fade(WHITE, 0.9F));
    if (b.evolved) // evolved: a diamond head on the streak
    {
        const Vector2 d = Vector2Subtract(shape.points[1], shape.points[0]);
        const float rot = std::atan2(d.y, d.x) * RAD2DEG;
        DrawPoly(shape.points[1], 4, b.halfWidth * 3.0F, rot, Fade(b.color, 0.9F));
        DrawPoly(shape.points[1], 4, b.halfWidth * 1.4F, rot, WHITE);
    }
}

// A player bullet: laser bolts as streaks, round shots as dots; an evolved round shot is a
// gleaming lance along its flight instead.
void drawPlayerBullet(const Bullet& b)
{
    if (b.streak > 0)
    {
        drawLaserBolt(b);
        return;
    }
    if (b.phasing)
    {
        DrawCircleV(b.position, b.radius * 1.6F, Fade(WHITE, 0.35F));
    }
    if (!b.evolved)
    {
        // Ranger's base Forward shots are drawn at half size (the hitbox is unchanged).
        DrawCircleV(b.position, b.source == DamageSource::Forward ? b.radius * 0.5F : b.radius, b.color);
        return;
    }
    const float speed = Vector2Length(b.velocity);
    const Vector2 dir = speed > 0.01F ? Vector2Scale(b.velocity, 1.0F / speed) : Vector2{.x = 0, .y = -1};
    const Vector2 side{.x = -dir.y, .y = dir.x};
    const Vector2 tip = Vector2Add(b.position, Vector2Scale(dir, b.radius * 2.2F));
    const Vector2 tail = Vector2Subtract(b.position, Vector2Scale(dir, b.radius * 3.4F));
    const Vector2 left = Vector2Add(b.position, Vector2Scale(side, b.radius * 0.8F));
    const Vector2 right = Vector2Subtract(b.position, Vector2Scale(side, b.radius * 0.8F));
    for (int32_t k = 1; k <= 4; k++) // a short trail fading out behind
    {
        const auto kf = static_cast<float>(k);
        DrawLineEx(Vector2Subtract(tail, Vector2Scale(dir, b.radius * 1.2F * (kf - 1))),
                   Vector2Subtract(tail, Vector2Scale(dir, b.radius * 1.2F * kf)), b.radius * (0.9F - 0.18F * kf),
                   Fade(b.color, 0.6F - 0.13F * kf));
    }
    for (const auto& [p, q] : {std::pair{left, right}, std::pair{right, left}}) // both windings
    {
        DrawTriangle(tip, p, q, b.color);
        DrawTriangle(tail, p, q, Fade(b.color, 0.7F));
    }
    DrawLineEx(b.position, tip, 1.5F, WHITE);
}

// Bastion's orbit: a blade circling the ship, and one flung outward. Evolved, the circling blades
// are spinning diamonds and the flung ones eight-point stars, in the evolved colour.
void drawOrbitRingBlade(Vector2 pos, Color color, bool evolved)
{
    if (!evolved)
    {
        DrawCircleV(pos, 5, color);
        return;
    }
    const float spin = static_cast<float>(GetTime()) * 360.0F;
    DrawPoly(pos, 4, 8.0F, spin, color);
    DrawCircleV(pos, 2.5F, WHITE);
}

void drawOrbitShot(Vector2 pos, float radius, bool evolved)
{
    const float angle = static_cast<float>(GetTime()) * 720.0F;
    if (!evolved)
    {
        DrawPoly(pos, 4, radius, angle, weaponColor(WeaponType::Orbit));
        DrawCircleV(pos, radius * 0.4F, Fade(WHITE, 0.8F));
        return;
    }
    const Color gold = evolvedWeaponColor(WeaponType::Orbit);
    DrawCircleV(pos, radius * 1.8F, Fade(gold, 0.15F));
    DrawPoly(pos, 4, radius * 1.3F, angle, gold);
    DrawPoly(pos, 4, radius * 1.3F, angle + 45.0F, gold);
    DrawCircleV(pos, radius * 0.45F, WHITE);
}

// Nerve bursts' own shapes: Ranger's ball and Bastion's tornado blades; evolved, in the signature
// weapon's evolved colour (a turning-arc orb for the ball, star blades for the tornado).
void drawNerveBall(Vector2 pos, float radius, bool evolved)
{
    if (!evolved)
    {
        DrawCircleV(pos, radius, Fade(weaponColor(WeaponType::Forward), 0.55F));
        DrawCircleLines(static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y), radius, Fade(WHITE, 0.8F));
        return;
    }
    // Evolved: a gold orb with a hot core, three arcs turning round its rim, and sparks circling
    // inside the other way.
    const Color gold = evolvedWeaponColor(WeaponType::Forward);
    const float t = static_cast<float>(GetTime());
    DrawCircleV(pos, radius, Fade(gold, 0.35F));
    DrawCircleV(pos, radius * 0.5F, Fade(gold, 0.5F));
    DrawCircleV(pos, radius * 0.25F, Fade(WHITE, 0.75F));
    for (int32_t k = 0; k < 3; k++)
    {
        const float start = t * 160.0F + static_cast<float>(k) * 120.0F;
        DrawRing(pos, radius - 3.0F, radius + 1.0F, start, start + 80.0F, 16, gold);
    }
    for (int32_t k = 0; k < 4; k++)
    {
        const float a = -t * 4.0F + static_cast<float>(k) * PI / 2;
        DrawCircleV(Vector2Add(pos, Vector2{.x = std::cos(a) * radius * 0.7F, .y = std::sin(a) * radius * 0.7F}),
                    radius * 0.08F, Fade(WHITE, 0.85F));
    }
}

void drawSpiralBlade(Vector2 pos, bool evolved)
{
    if (!evolved)
    {
        DrawCircleV(pos, 14, weaponColor(WeaponType::Orbit));
        return;
    }
    const Color gold = evolvedWeaponColor(WeaponType::Orbit);
    const float spin = static_cast<float>(GetTime()) * 540.0F;
    DrawPoly(pos, 4, 20.0F, spin, gold);
    DrawPoly(pos, 4, 20.0F, spin + 45.0F, gold);
    DrawCircleV(pos, 6.0F, WHITE);
}

void drawGameplayWorld(const Game& game)
{
    drawBackgroundStars(game, game.run.player.position);
    drawPunctumLightning(game);
    drawPunctumInfall(game); // other Biomes' wreckage falling to the map centre, over the stars
    if (currentBiome(game.run.waveNumber) == Biome::ShatteredBelt)
    {
        drawBeltPlanet(game); // after the stars: the solid body hides them
    }
    if (currentBiome(game.run.waveNumber) == Biome::Rustbloom)
    {
        drawRustbloomDerelicts(game); // after the stars (the hulls hide them), before the fragments and spores
    }
    drawSolarForgeCaveBackground(game);
    drawRustbloomBackground(game);

    if (game.run.bossCutscenePhase == 11 || game.run.bossCutscenePhase == 12)
    {
        drawKrakenRemains(game);
    }

    constexpr float gasCullDistance = 3000.0F; // ponytail: fixed cull, widen if zoom goes further out
    for (const auto& c : game.run.gasClouds)
    {
        if (Vector2Distance(c.position, game.run.player.position) < gasCullDistance + c.radius)
        {
            DrawCircleV(c.position, c.radius, c.color);
        }
    }

    drawRustPatches(game);
    drawMagmaPools(game);

    if (game.run.blackhole.active)
    {
        drawBlackHoleTwirl(game.run.blackhole.position, game.run.blackhole.influenceRadius,
                           game.run.blackHoleDust);
        if (game.run.blackhole.isKrakenPortal)
        {
            // The way to the Banished realm: lit rim and swirling arcs so it reads on Punctum black.
            const Vector2 c = game.run.blackhole.position;
            const float r = game.run.blackhole.radius;
            const auto t = static_cast<float>(GetTime());
            const float pulse = 0.5F + 0.5F * std::sin(t * 3.0F);
            for (int32_t g = 6; g >= 1; g--)
            {
                DrawCircleV(c, r * (1.0F + 0.25F * static_cast<float>(g)),
                            Fade(Palette::PunctumHaze, 0.05F + 0.02F * pulse));
            }
            DrawCircleV(c, r, BLACK);
            for (int32_t a = 0; a < 6; a++)
            {
                const float base = t * 90.0F + static_cast<float>(a) * 60.0F;
                const float ar = r * (1.15F + 0.12F * static_cast<float>(a % 3));
                DrawRing(c, ar, ar + 3.0F, base, base + 70.0F, 24,
                         Fade(a % 2 == 0 ? Palette::PunctumHaze : Palette::Shield, 0.8F));
            }
            DrawRing(c, r - 2.0F, r + 2.0F + 2.0F * pulse, 0, 360, 64, Palette::PunctumAccent);
        }
    }

    if (game.run.wormhole.active)
    {
        Color rim = Palette::Shield;
        if (currentBiome(game.run.waveNumber) == Biome::ShatteredBelt)
        {
            rim = Palette::ShatteredBeltAccent;
        }
        else if (currentBiome(game.run.waveNumber) == Biome::Rustbloom)
        {
            rim = Palette::RustbloomAccent;
        }
        drawWormholeMouth(game.run.wormhole.positionA, game.run.wormhole.facingA,
                          game.run.wormhole.radius, rim);
        drawWormholeMouth(game.run.wormhole.positionB, game.run.wormhole.facingB,
                          game.run.wormhole.radius, rim);
    }

    for (const auto& cloud : game.run.gasHazards)
    {
        const float pulse =
            0.85F + 0.15F * std::sin(static_cast<float>(GetTime()) * 2.2F + cloud.seed);
        const float fadeAlpha = std::min(1.0F, cloud.life / 1.5F);
        Color cloudColor = ColorLerp(Palette::RustbloomAccent, Palette::RustbloomHaze, 0.3F);
        if (cloud.isFire)
        {
            cloudColor = ColorLerp(Palette::Accent, Palette::Crit, 0.3F);
        }
        else if (cloud.spores)
        {
            cloudColor = ColorLerp(Palette::RustbloomAccent, Palette::RustbloomHaze, 0.7F);
        }
        drawFluidBlob(cloud.position, cloud.radius * pulse, Fade(cloudColor, 0.55F * fadeAlpha),
                      cloud.seed, Vector2{});
    }

    for (const auto& a : game.run.asteroids)
    {
        if (a.isFluid)
        {
            const float pulse =
                0.85F + 0.15F * std::sin(static_cast<float>(GetTime()) * 3.0F + a.position.x);
            drawLavaChunk(a.position, a.radius * pulse, a.position.x * 0.037F + a.position.y * 0.019F,
                          a.velocity);
            continue;
        }
        DrawCircleV(a.position, a.radius, Palette::StructMid);
        DrawCircleV(Vector2{.x = a.position.x - a.radius / 3, .y = a.position.y - a.radius / 4},
                    a.radius / 4, Palette::StructDark);
        DrawCircleV(Vector2{.x = a.position.x + a.radius / 4, .y = a.position.y + a.radius / 3},
                    a.radius / 5, Palette::StructDark);
    }

    const bool warlordActive = std::any_of(
        game.run.eliteHazards.begin(), game.run.eliteHazards.end(), [](const EliteHazard& hazard)
        { return hazard.active && hazard.role == EliteHazardRole::Warlord; });
    for (const auto& e : game.run.enemies)
    {
        drawEnemy(game, e, warlordActive);
    }

    Color hazardBody = Palette::StructDark;
    if (currentBiome(game.run.waveNumber) == Biome::ShatteredBelt)
    {
        hazardBody = ColorLerp(Palette::StructDark, Palette::ShatteredBeltAccent, beltDustTint);
    }
    else if (currentBiome(game.run.waveNumber) == Biome::Rustbloom)
    {
        hazardBody = ColorLerp(Palette::StructDark, Palette::RustbloomAccent, rustbloomTint);
    }
    for (const auto& hazard : game.run.eliteHazards)
    {
        drawEliteHazard(hazard, hazardBody);
    }

    for (const auto& p : game.run.pickups)
    {
        const float alpha =
            p.lifetime <= UpdateConstants::pickupExpiryWarning
                ? 0.35F + 0.65F * (0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 12.0F))
                : 1.0F;
        if (p.type == PickupType::Danger)
        {
            continue; // drawn last, over everything (see the end of this function)
        }
        if (p.pull > 0)
        {
            // Magnet flight: a streak trailing away from the ship, longer as the pull ramps up.
            const Vector2 away = Vector2Normalize(Vector2Subtract(p.position, game.run.player.position));
            const float len = (p.homing ? 34.0F : 16.0F) * p.pull * p.pull;
            DrawLineEx(p.position, Vector2Add(p.position, Vector2Scale(away, len)), 2.0F,
                       Fade(p.type == PickupType::XP ? Palette::Charge : Palette::Crit, 0.45F * alpha * p.pull));
        }
        drawPickupIcon(p.type, p.element, p.position, alpha);
    }

    for (const auto& boss : game.run.bosses)
    {
        drawBoss(game, boss);
    }

    drawDebrisSquall(game);

    for (const auto& m : game.run.mines)
    {
        if (!m.active)
        {
            continue;
        }
        const Color mineColor{.r = 190, .g = 225, .b = 245, .a = 255};
        DrawCircleV(m.position, 6, mineColor);
        DrawCircleLines(static_cast<int32_t>(m.position.x), static_cast<int32_t>(m.position.y),
                        m.radius, Fade(mineColor, 0.2F));
    }

    for (const auto& turret : game.run.turrets)
    {
        DrawRectangle(static_cast<int32_t>(turret.position.x - 8),
                      static_cast<int32_t>(turret.position.y - 8), 16, 16, Palette::Accent);
        DrawRectangleLines(static_cast<int32_t>(turret.position.x - 8),
                           static_cast<int32_t>(turret.position.y - 8), 16, 16,
                           Fade(Palette::StructLight, 0.6F));
    }

    for (const auto& drone : game.run.followerDrones)
    {
        const Vector2 c = drone.position;
        const float spin = static_cast<float>(GetTime()) * 2.5F;
        constexpr int32_t spikeCount = 5;
        for (int32_t i = 0; i < spikeCount; i++)
        {
            const float angle = spin + static_cast<float>(i) * 2 * std::numbers::pi_v<float> /
                                           static_cast<float>(spikeCount);
            const Vector2 dir{.x = std::cos(angle), .y = std::sin(angle)};
            const Vector2 perp{.x = -dir.y, .y = dir.x};
            const Vector2 tip = Vector2Add(c, Vector2Scale(dir, 10.0F));
            const Vector2 baseA =
                Vector2Add(c, Vector2Add(Vector2Scale(dir, 3.0F), Vector2Scale(perp, 2.2F)));
            const Vector2 baseB =
                Vector2Add(c, Vector2Subtract(Vector2Scale(dir, 3.0F), Vector2Scale(perp, 2.2F)));
            DrawTriangle(tip, baseA, baseB, Palette::Accent);
        }
        DrawCircleV(c, 5.0F, Palette::StructMid);
        DrawCircleV(c, 3.0F, Palette::StructLight);
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), 5.0F,
                        Fade(Palette::Accent, 0.7F));
    }

    for (const auto& drone : game.run.laserDrones)
    {
        const Vector2 c = drone.position;
        const Vector2 toTarget = drone.beamFlashTimer > 0 ? Vector2Subtract(drone.beamTarget, c)
                                                          : Vector2{.x = 0, .y = -1};
        const Vector2 dir =
            Vector2Length(toTarget) > 0.01F ? Vector2Normalize(toTarget) : Vector2{.x = 0, .y = -1};
        const Vector2 perp{.x = -dir.y, .y = dir.x};
        const Vector2 nose = Vector2Add(c, Vector2Scale(dir, 9.0F));
        const Vector2 tailA =
            Vector2Add(c, Vector2Add(Vector2Scale(dir, -6.0F), Vector2Scale(perp, 5.0F)));
        const Vector2 tailB =
            Vector2Add(c, Vector2Subtract(Vector2Scale(dir, -6.0F), Vector2Scale(perp, 5.0F)));
        DrawTriangle(nose, tailA, tailB, Palette::Crit);
        DrawTriangle(nose, tailB, tailA, Fade(Palette::StructLight, 0.25F));
        DrawCircleV(nose, 2.2F, WHITE);
        DrawLineEx(tailA, tailB, 2.0F, Fade(Palette::Crit, 0.6F));
        if (drone.beamFlashTimer > 0)
        {
            DrawLineEx(drone.position, drone.beamTarget, 2,
                       Fade(Palette::Crit, drone.beamFlashTimer / 0.15F));
        }
    }

    // Chain Lightning: a jagged bolt that re-crackles every few frames, with two short forks,
    // a soft pale-blue halo under a near-white core, fading out over its life.
    for (const auto& bolt : game.run.chainLightningBolts)
    {
        const float fade = std::clamp(bolt.timer / chainLightningBoltLife, 0.0F, 1.0F);
        const float seed = bolt.from.x * 0.13F + bolt.to.y * 0.71F + std::floor(static_cast<float>(GetTime()) * 30.0F);
        const Vector2 span = Vector2Subtract(bolt.to, bolt.from);
        const float length = Vector2Length(span);
        if (length < 1.0F)
        {
            continue;
        }
        const Vector2 perp = Vector2Scale(Vector2{.x = -span.y, .y = span.x}, 1.0F / length);
        const float jag = std::min(26.0F, length * 0.08F);
        constexpr int32_t segments = 12;
        std::array<Vector2, segments + 1> points{};
        for (int32_t i = 0; i <= segments; i++)
        {
            const float f = static_cast<float>(i) / segments;
            const float offset = i == 0 || i == segments ? 0.0F : (hashNoise(seed + static_cast<float>(i) * 1.7F) - 0.5F) * 2.0F * jag;
            points.at(static_cast<size_t>(i)) = Vector2Add(Vector2Add(bolt.from, Vector2Scale(span, f)), Vector2Scale(perp, offset));
        }
        for (int32_t i = 0; i < segments; i++)
        {
            DrawLineEx(points.at(static_cast<size_t>(i)), points.at(static_cast<size_t>(i + 1)), 7.0F,
                       Fade(Palette::ElementFreeze, 0.18F * fade));
        }
        for (int32_t i = 0; i < segments; i++)
        {
            DrawLineEx(points.at(static_cast<size_t>(i)), points.at(static_cast<size_t>(i + 1)), 2.0F,
                       Fade(Palette::ElementStatic, fade));
        }
        for (const int32_t at : {4, 8})
        {
            const Vector2 base = points.at(static_cast<size_t>(at));
            const float side = hashNoise(seed + static_cast<float>(at) * 5.3F) > 0.5F ? 1.0F : -1.0F;
            const Vector2 fork = Vector2Add(Vector2Add(base, Vector2Scale(span, 0.06F)), Vector2Scale(perp, side * jag * 1.6F));
            DrawLineEx(base, fork, 1.2F, Fade(Palette::ElementStatic, 0.7F * fade));
        }
        DrawCircleV(bolt.to, 4.0F * fade + 2.0F, Fade(Palette::ElementStatic, 0.8F * fade)); // the strike
    }

    for (const auto& shot : game.run.sniperShots)
    {
        const float fade = std::clamp(shot.timer / sniperLineFlashDuration, 0.0F, 1.0F);
        const Color lineColor = Palette::Crit;
        const float width = game.run.sniperEventPhase == 2 ? sniperLineWidth : 4.0F;
        DrawLineEx(shot.from, shot.to, width + 4.0F, Fade(lineColor, 0.2F * fade));
        DrawLineEx(shot.from, shot.to, width, Fade(lineColor, 0.75F * fade));
        DrawLineEx(shot.from, shot.to, 2.0F, Fade(WHITE, fade));
    }

    for (const auto& wave : game.run.bossDeathShockwaves)
    {
        const float progress =
            std::clamp(1 - wave.timer / UpdateConstants::bossDeathShockwaveDuration, 0.0F, 1.0F);
        const float radius = bossConstants::maxSlamRadius * progress;
        DrawCircleLines(static_cast<int32_t>(wave.position.x),
                        static_cast<int32_t>(wave.position.y), radius,
                        Fade(Palette::Crit, 0.6F * (1 - progress)));
        DrawCircleLines(static_cast<int32_t>(wave.position.x),
                        static_cast<int32_t>(wave.position.y), radius * 0.85F,
                        Fade(Palette::Accent, 0.4F * (1 - progress)));
    }

    for (const auto& p : game.run.bossProjectiles)
    {
        if (p.isFluid)
        {
            drawFluidBlob(p.position, p.radius * 1.4F, p.fluidColor,
                          p.position.x * 0.05F + p.position.y * 0.03F, p.velocity);
        }
        else if (p.visualEnemyKind >= 0)
        {
            drawKrakenHurledEnemy(p);
        }
        else if (p.isMeteor)
        {
            drawMeteorProjectile(p);
        }
        else if (p.isSpore)
        {
            drawSporeProjectile(p);
        }
        else if (p.relayTargetId != 0)
        {
            // Ricochet relay: an amber slug with a streak behind it.
            const Vector2 back = Vector2Scale(Vector2Normalize(p.velocity), -1.0F);
            DrawLineEx(p.position, Vector2Add(p.position, Vector2Scale(back, p.radius * 6.0F)), p.radius * 1.2F,
                       Fade(Palette::ShatteredBeltAccent, 0.3F));
            DrawCircleV(p.position, p.radius * 1.5F, Fade(Palette::ShatteredBeltAccent, 0.35F));
            DrawCircleV(p.position, p.radius, Palette::ShatteredBeltAccent);
            DrawCircleV(p.position, p.radius * 0.45F, WHITE);
        }
        else if (p.isShard)
        {
            const Vector2 tail = Vector2Subtract(p.position, Vector2Scale(Vector2Normalize(p.velocity), p.radius * 3));
            DrawLineEx(tail, p.position, p.radius, Fade(Palette::ShatteredBeltAccent, 0.4F));
            DrawPoly(p.position, 5, p.radius * 1.3F, p.position.x + p.position.y, Palette::StructLight);
        }
        else if (p.isPetal)
        {
            // Petal Volley crystal: a long amber kite pointing where it flies, glowing as it stalls.
            constexpr Color gold{.r = 255, .g = 188, .b = 72, .a = 255};
            const Vector2 fwd = Vector2Length(p.velocity) > 0.01F ? Vector2Normalize(p.velocity) : Vector2{.x = 1, .y = 0};
            const Vector2 side{.x = -fwd.y, .y = fwd.x};
            const float r = p.radius;
            const bool stalling = p.petalTimer < petalStallTime;
            const float glow = stalling ? 0.5F + 0.5F * std::sin(p.petalTimer * 30.0F) : 0.4F;
            const Vector2 tip = Vector2Add(p.position, Vector2Scale(fwd, r * 2.2F));
            const Vector2 tail = Vector2Subtract(p.position, Vector2Scale(fwd, r * 1.4F));
            const Vector2 l = Vector2Add(p.position, Vector2Scale(side, r * 0.8F));
            const Vector2 rr = Vector2Subtract(p.position, Vector2Scale(side, r * 0.8F));
            DrawCircleV(p.position, r * 2.0F, Fade(Palette::ElementBurn, 0.12F + 0.15F * glow));
            DrawTriangle(tip, l, tail, Fade(gold, 0.9F));
            DrawTriangle(tip, tail, l, Fade(gold, 0.9F));
            DrawTriangle(tip, rr, tail, ColorLerp(gold, Palette::ElementBurn, 0.5F));
            DrawTriangle(tip, tail, rr, ColorLerp(gold, Palette::ElementBurn, 0.5F));
            DrawLineEx(tip, tail, 1.2F, Fade(WHITE, 0.55F + 0.4F * glow));
        }
        else if (p.isEmber)
        {
            // Seeker Ember: a spinning gold diamond in an ember glow, shedding sparks.
            const auto t = static_cast<float>(GetTime());
            const Vector2 back = Vector2Scale(Vector2Normalize(p.velocity), -1.0F);
            for (int32_t k = 1; k <= 3; k++)
            {
                const auto kf = static_cast<float>(k);
                const Vector2 spark = Vector2Add(
                    p.position, Vector2Add(Vector2Scale(back, p.radius * (1.2F + kf * 1.3F)),
                                           Vector2{.x = std::sin(t * 23.0F + kf) * 2.5F, .y = std::cos(t * 19.0F + kf) * 2.5F}));
                DrawRectangleV(Vector2Subtract(spark, Vector2{.x = 1.5F, .y = 1.5F}), Vector2{.x = 3, .y = 3},
                               Fade(Palette::ElementBurn, 0.8F - kf * 0.22F));
            }
            DrawCircleV(p.position, p.radius * 1.8F, Fade(Palette::ElementBurn, 0.22F));
            DrawPoly(p.position, 4, p.radius * 1.15F, t * 400.0F, Color{.r = 255, .g = 188, .b = 72, .a = 255});
            DrawCircleV(p.position, p.radius * 0.35F, Fade(WHITE, 0.85F));
        }
        else if (p.homing)
        {
            drawComet(p);
        }
        else if (p.isBlade)
        {
            // Grinder spike blade: two spinning squares make an 8-point star.
            const float rot = static_cast<float>(GetTime()) * 720.0F;
            DrawPoly(p.position, 4, p.radius * 1.3F, rot, Palette::Crit);
            DrawPoly(p.position, 4, p.radius * 1.3F, rot + 45, Palette::Crit);
            DrawCircleV(p.position, p.radius * 0.45F, Palette::StructDark);
        }
        else
        {
            // Plasma orb: a streak behind, a pulsing halo, a deep red body and a hot core. Drawn a
            // little larger than its hitbox.
            const float pr = p.radius * 1.25F;
            const float pulse = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 14.0F + p.position.x * 0.05F);
            const float speed = Vector2Length(p.velocity);
            if (speed > 0.01F)
            {
                const Vector2 back = Vector2Scale(p.velocity, -1.0F / speed);
                for (int32_t k = 3; k >= 1; k--)
                {
                    const auto kf = static_cast<float>(k);
                    DrawCircleV(Vector2Add(p.position, Vector2Scale(back, pr * 1.3F * kf)),
                                pr * (1.0F - 0.22F * kf), Fade(Palette::Accent, 0.32F - 0.08F * kf));
                }
            }
            DrawCircleV(p.position, pr * (2.0F + 0.35F * pulse), Fade(Palette::Accent, 0.14F + 0.08F * pulse));
            DrawCircleV(p.position, pr * 1.15F, Palette::AccentDim);
            DrawCircleV(p.position, pr * 0.85F, Palette::Accent);
            DrawCircleV(p.position, pr * 0.42F, ColorLerp(Palette::Crit, WHITE, 0.4F * pulse));
        }
    }

    // Radial bursts' shock rings: a bright edge racing out, thinning and fading.
    for (const auto& ring : game.run.blastRings)
    {
        const float t = 1.0F - ring.life / ring.maxLife;
        const float ease = 1.0F - (1.0F - t) * (1.0F - t);
        const float r = 12.0F + (ring.radius - 12.0F) * ease;
        const float width = 14.0F * (1.0F - t) + 2.0F;
        const float fade = 1.0F - t * t; // holds, then drops away
        DrawCircleV(ring.position, r, Fade(Palette::Accent, 0.12F * fade));
        DrawRing(ring.position, r - width, r, 0, 360, 64, Fade(Palette::Accent, 0.7F * fade));
        DrawRing(ring.position, r - 2.0F, r, 0, 360, 64, Fade(Palette::Crit, fade));
        if (t < 0.3F) // the flash where the burst left
        {
            DrawCircleV(ring.position, 26.0F * (1.0F - t / 0.3F), Fade(Palette::Crit, 0.6F * (1.0F - t / 0.3F)));
        }
    }

    if (game.run.player.blackHoleCoreTimer > 0)
    {
        DrawCircleLines(static_cast<int32_t>(game.run.player.position.x),
                        static_cast<int32_t>(game.run.player.position.y),
                        game.run.player.radius + 6,
                        Fade(Palette::Accent, game.run.player.blackHoleCoreTimer));
    }

    drawOrbitBlades(game);
    drawOrbitBladeProjectiles(game);
    drawNerveBallProjectiles(game);
    drawNerveSpiralProjectiles(game);

    const bool shipVisible =
        game.run.player.health > 0 &&
        (game.run.player.immunityTimer <= 0 || static_cast<int>(GetTime() * 10) % 2 == 0);
    if (shipVisible)
    {
        drawShip(game);
    }

    if (game.run.player.shieldActive && game.run.player.dashing &&
        Vector2Length(game.run.player.dashVelocity) > 0.01F)
    {
        // Shield dash: a spear tip of light wrapped round the ship, pointing along the dash. A
        // long faceted head, a pale ridge down its middle, and a faint split shaft trailing behind.
        const Vector2 p = game.run.player.position;
        const float r = game.run.player.radius;
        const Vector2 fwd = Vector2Normalize(game.run.player.dashVelocity);
        const Vector2 side{.x = -fwd.y, .y = fwd.x};
        const auto at = [&](float ahead, float across)
        { return Vector2Add(p, Vector2Add(Vector2Scale(fwd, ahead * r), Vector2Scale(side, across * r))); };
        const Vector2 tip = at(3.4F, 0);
        const Vector2 wingL = at(0.6F, 1.0F);
        const Vector2 wingR = at(0.6F, -1.0F);
        const Vector2 notch = at(-0.2F, 0);
        const float flicker = 0.85F + 0.15F * std::sin(static_cast<float>(GetTime()) * 40.0F);
        for (const auto& [a, b, c] : {std::array{tip, wingL, notch}, std::array{tip, notch, wingR}})
        {
            DrawTriangle(a, b, c, Fade(Palette::Shield, 0.55F * flicker));
            DrawTriangle(a, c, b, Fade(Palette::Shield, 0.55F * flicker));
        }
        DrawLineEx(tip, wingL, 1.5F, Fade(Palette::Crit, 0.9F));
        DrawLineEx(tip, wingR, 1.5F, Fade(Palette::Crit, 0.9F));
        DrawLineEx(wingL, notch, 1.0F, Fade(Palette::Crit, 0.5F));
        DrawLineEx(wingR, notch, 1.0F, Fade(Palette::Crit, 0.5F));
        DrawLineEx(at(2.9F, 0), notch, 2.0F, Fade(WHITE, 0.8F * flicker)); // the ridge
        for (const float s : {0.35F, -0.35F})
        {
            DrawLineEx(at(-0.4F, s), at(-2.6F, s * 0.4F), 1.0F, Fade(Palette::Shield, 0.4F));
        }
    }
    else if (game.run.player.shieldActive)
    {
        const float shieldRadius = game.run.player.radius + 5;
        DrawCircleV(game.run.player.position, shieldRadius, Fade(Palette::Shield, 0.5F));
        DrawCircleLines(static_cast<int32_t>(game.run.player.position.x),
                        static_cast<int32_t>(game.run.player.position.y), shieldRadius,
                        Palette::Shield);
    }
    else if (game.run.player.shieldCooldownTimer > 0)
    {
        drawShieldIndicator(game.run.player);
    }

    for (const auto& b : game.run.bullets)
    {
        drawPlayerBullet(b);
    }

    for (const auto& p : game.run.deathParticles)
    {
        DrawCircleV(p.position, p.radius, Fade(p.color, p.life / p.maxLife));
    }

    // Dash afterimages: copies of the ship's hull left along the dash, facing as it was, fading
    // out and cooling from gold to the ship's own colour.
    const auto shipClass = static_cast<ShipClass>(game.resources.settings.shipIndex);
    for (const auto& w : game.run.dashWedges)
    {
        const float fade = w.life / w.maxLife;
        const float angle = std::atan2(w.velocity.y, w.velocity.x) + std::numbers::pi_v<float> / 2;
        const Color ghost = ColorLerp(game.run.player.color, Palette::Crit, fade);
        drawShipHull(shipClass, w.position, game.run.player.radius * (0.85F + 0.15F * fade), angle,
                     Fade(ghost, 0.8F * fade));
    }

    // Bomber bombs: a red ring closing in on a hot dot until it blows.
    for (const auto& b : game.run.enemyBombs)
    {
        const float progress = 1.0F - b.life / b.maxLife;
        const float radius = bomberBombRadius * (1.0F - 0.7F * progress);
        DrawCircleV(b.position, bomberBombRadius, Fade(Palette::Accent, 0.06F + 0.1F * progress));
        DrawRing(b.position, radius - 2.0F, radius, 0, 360, 36, Fade(Palette::Accent, 0.5F + 0.5F * progress));
        DrawCircleV(b.position, 4.0F + 3.0F * progress, Palette::Accent);
    }

    // Delayed blast markers: a ring closing in on a hot dot until it detonates.
    for (const auto& b : game.run.dashBlasts)
    {
        const float progress = 1.0F - b.life / b.maxLife;
        const float radius = dashBlastRadius * (1.0F - 0.75F * progress);
        DrawRing(b.position, radius - 2.0F, radius, 0, 360, 36, Fade(Palette::Accent, 0.4F + 0.5F * progress));
        DrawCircleV(b.position, 3.0F + 3.0F * progress, Fade(Palette::Crit, 0.6F + 0.4F * progress));
    }


    for (const auto& p : game.run.flameParticles)
    {
        const float fade = p.life / p.maxLife;
        const float flicker = 0.75F + 0.25F * std::sin(static_cast<float>(GetTime()) * 24.0F +
                                                       p.position.x * 0.1F + p.position.y * 0.1F);
        if (p.maxLife == flamethrowerJetLife)
        {
            // Flamethrower jet: small and hot at the nozzle, billowing wider and redder as it goes.
            const float age = 1.0F - fade;
            const float radius = p.radius * (0.35F + 0.9F * age);
            DrawCircleV(p.position, radius, Fade(ColorLerp(p.color, Palette::Accent, age * 0.8F), 0.45F * fade * flicker));
            DrawCircleV(p.position, radius * 0.45F, Fade(ColorLerp(WHITE, Palette::Charge, age), 0.6F * fade));
            continue;
        }

        DrawCircleV(p.position, p.radius * fade, Fade(p.color, 0.22F * fade * flicker));
        DrawCircleV(p.position, p.radius * fade * 0.5F, Fade(Palette::Accent, 0.35F * fade));
    }

    constexpr int32_t bigDamageThreshold = 15;
    for (const auto& number : game.run.damageNumbers)
    {
        const float alpha = number.timer / number.maxTimer;
        const bool big = number.amount >= bigDamageThreshold;
        const std::string text = std::format("{}", number.amount);
        const int32_t fontSize = big ? 20 : 14;
        const Vector2 size =
            MeasureTextEx(game.resources.font, text.c_str(), static_cast<float>(fontSize), 1.0F);
        drawText(game, text.c_str(), static_cast<int32_t>(number.position.x - size.x / 2),
                 static_cast<int32_t>(number.position.y - size.y / 2), fontSize,
                 Fade(big ? Palette::Crit : Palette::StructLight, alpha));
    }

    // Danger pickups go over the swarm, each with a beacon of rings so it reads in a crowd.
    for (const auto& p : game.run.pickups)
    {
        if (!p.active || p.type != PickupType::Danger)
        {
            continue;
        }
        const auto now = static_cast<float>(GetTime());
        for (int32_t k = 0; k < 2; k++)
        {
            const float phase = std::fmod(now * 0.9F + static_cast<float>(k) * 0.5F, 1.0F);
            DrawRing(p.position, 14.0F + 30.0F * phase, 16.5F + 30.0F * phase, 0, 360, 32,
                     Fade(Palette::Accent, 0.8F * (1.0F - phase)));
        }
        DrawCircleV(p.position, 15.0F, Fade(Palette::Void, 0.85F));
        drawPickupIcon(p.type, p.element, p.position, 1.0F);
    }

    if (game.run.player.blindTimer > 0)
    {
        // Blind: the dark closes in to a circle round the ship, feathered over a few rings.
        const float t = game.run.player.blindTimer;
        const float a = 0.97F * std::min({1.0F, t / 0.6F, (blindDuration - t) / 0.4F + 0.05F});
        constexpr int32_t feather = 6;
        constexpr float band = 14.0F;
        for (int32_t k = 0; k < feather; k++)
        {
            const float inner = blindRadius + static_cast<float>(k) * band;
            DrawRing(game.run.player.position, inner, inner + band, 0, 360, 64,
                     Fade(Palette::Void, a * static_cast<float>(k + 1) / static_cast<float>(feather + 1)));
        }
        DrawRing(game.run.player.position, blindRadius + static_cast<float>(feather) * band, 6000.0F, 0, 360, 64,
                 Fade(Palette::Void, a));
    }

    if (game.run.punctumThunderFlashTimer > 0)
    {
        const float flashAlpha =
            0.5F * (game.run.punctumThunderFlashTimer / punctumThunderFlashDuration);
        const auto halfW = static_cast<float>(game.resources.screenWidth);
        const auto halfH = static_cast<float>(game.resources.screenHeight);
        DrawRectangle(static_cast<int32_t>(game.run.player.position.x - halfW),
                      static_cast<int32_t>(game.run.player.position.y - halfH),
                      static_cast<int32_t>(halfW * 2), static_cast<int32_t>(halfH * 2),
                      Fade(WHITE, flashAlpha));
    }
}

void drawShipHull(ShipClass shipClass, Vector2 p, float r, float angle, Color shipColor)
{
    const auto rotate = [angle](Vector2 v) -> Vector2
    {
        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);
        return Vector2{.x = v.x * cosA - v.y * sinA, .y = v.x * sinA + v.y * cosA};
    };

    switch (shipClass)
    {
    case ShipClass::Bastion:
    {
        drawBastionHull(p, r, angle, shipColor);
        break;
    }
    case ShipClass::Interceptor:
    {
        drawInterceptorHull(p, r, angle, shipColor);
        break;
    }
    case ShipClass::Ranger:
    {
        drawRangerHull(p, r, angle, shipColor);
        break;
    }
    case ShipClass::Count:
    default:
    {
        const Vector2 tip = Vector2Add(p, rotate(Vector2{.x = 0, .y = -r}));
        const Vector2 left = Vector2Add(p, rotate(Vector2{.x = -r * 0.8F, .y = r}));
        const Vector2 right = Vector2Add(p, rotate(Vector2{.x = r * 0.8F, .y = r}));
        DrawTriangle(tip, left, right, shipColor);
        DrawTriangleLines(tip, left, right, Fade(Palette::StructDark, 0.6F));

        const Vector2 plateTop = Vector2Add(p, rotate(Vector2{.x = 0, .y = -r * 0.5F}));
        const Vector2 plateL = Vector2Add(p, rotate(Vector2{.x = -r * 0.35F, .y = r * 0.3F}));
        const Vector2 plateB = Vector2Add(p, rotate(Vector2{.x = 0, .y = r * 0.7F}));
        const Vector2 plateR = Vector2Add(p, rotate(Vector2{.x = r * 0.35F, .y = r * 0.3F}));
        DrawTriangle(plateTop, plateL, plateB, Fade(Palette::StructLight, 0.5F));
        DrawTriangle(plateTop, plateB, plateR, Fade(Palette::StructLight, 0.5F));
        break;
    }
    }

    if (shipClass != ShipClass::Bastion && shipClass != ShipClass::Ranger &&
        shipClass != ShipClass::Interceptor)
    {
        DrawCircleV(p, r * 0.3F, Palette::StructLight);
    }
}

// Generic single engine flame, shown behind a ship that has no hull-specific thruster jets.
void drawEngineFlame(Vector2 p, float r, float angle)
{
    const auto rotate = [angle](Vector2 v) -> Vector2
    {
        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);
        return Vector2{.x = v.x * cosA - v.y * sinA, .y = v.x * sinA + v.y * cosA};
    };

    const float flicker = 0.65F + 0.35F * std::sin(static_cast<float>(GetTime()) * 28.0F);
    const Vector2 flameTip =
        Vector2Add(p, rotate(Vector2{.x = 0, .y = r * (1.5F + 0.6F * flicker)}));
    const Vector2 flameLeft = Vector2Add(p, rotate(Vector2{.x = -r * 0.35F, .y = r * 0.85F}));
    const Vector2 flameRight = Vector2Add(p, rotate(Vector2{.x = r * 0.35F, .y = r * 0.85F}));
    DrawTriangle(flameLeft, flameTip, flameRight, Fade(Palette::Charge, 0.75F * flicker));
    DrawTriangle(flameLeft,
                 Vector2Add(p, rotate(Vector2{.x = 0, .y = r * (1.0F + 0.3F * flicker)})),
                 flameRight, Fade(Palette::StructLight, 0.6F * flicker));
}

void drawShip(const Game& game)
{
    const Vector2 p = game.run.player.position;

    const bool twirlingIn = game.run.bossCutscenePhase == 10;
    const float twirlProgress =
        twirlingIn
            ? std::clamp(1.0F - game.run.bossCutsceneTimer / banishedEntryTwirlDuration, 0.0F, 1.0F)
            : 0.0F;
    const float r =
        game.run.player.radius * (twirlingIn ? std::max(0.05F, 1.0F - twirlProgress) : 1.0F);

    const Vector2 dir = aimAtMouse(game);
    const float angle = std::atan2(dir.y, dir.x) + std::numbers::pi_v<float> / 2 +
                        (twirlingIn ? twirlProgress * 900.0F * DEG2RAD : 0.0F);

    if (twirlingIn)
    {
        const auto shipClass = static_cast<ShipClass>(game.resources.settings.shipIndex);
        const Color shipColor = ColorLerp(game.run.player.color, BLACK, twirlProgress);
        drawShipHull(shipClass, p, r, angle, shipColor);
        return;
    }

    const Color shipColor = game.run.player.dashing ? Palette::Crit : game.run.player.color;

    const auto shipClass = static_cast<ShipClass>(game.resources.settings.shipIndex);
    if (shipClass != ShipClass::Bastion && shipClass != ShipClass::Interceptor &&
        shipClass != ShipClass::Ranger)
    {
        // Bastion, Interceptor, and Ranger draw their own jets as part of their hull;
        // every other ship falls back to this single generic engine flame.
        drawEngineFlame(p, r, angle);
    }
    drawShipHull(shipClass, p, r, angle, shipColor);

    const auto t = static_cast<float>(GetTime());
    // A ray of energy: a wide soft band, a gold body, a white core, and crackling filaments
    // writhing along both edges. `w` is the body's half width.
    const auto drawFinisherBeam = [&](Vector2 from, Vector2 to, float w, float alpha)
    {
        DrawLineEx(from, to, w * 5.0F, Fade(Palette::Charge, 0.18F * alpha));
        DrawLineEx(from, to, w * 2.0F, Fade(Palette::Charge, 0.85F * alpha));
        DrawLineEx(from, to, w * 1.1F, Fade(Palette::Crit, alpha));
        DrawLineEx(from, to, w * 0.45F, Fade(WHITE, alpha));
        const Vector2 span = Vector2Subtract(to, from);
        const float length = Vector2Length(span);
        if (length < 1.0F)
        {
            return;
        }
        const Vector2 dir = Vector2Scale(span, 1.0F / length);
        const Vector2 side{.x = -dir.y, .y = dir.x};
        for (const float sgn : {1.0F, -1.0F})
        {
            Vector2 prev = Vector2Add(from, Vector2Scale(side, sgn * w));
            constexpr int32_t steps = 18;
            for (int32_t i = 1; i <= steps; i++)
            {
                const float f = static_cast<float>(i) / steps;
                const float wave = std::sin(f * 40.0F + t * 50.0F * sgn) * w * 0.7F;
                const Vector2 next = Vector2Add(Vector2Add(from, Vector2Scale(span, f)), Vector2Scale(side, sgn * (w * 1.3F + wave)));
                DrawLineEx(prev, next, 1.5F, Fade(WHITE, 0.7F * alpha));
                prev = next;
            }
        }
    };

    // QTE finisher: energy streams into the nose and a swelling orb, then a huge beam fires.
    const Vector2 nose = Vector2Add(p, Vector2Scale(dir, r * 1.4F));
    if (game.run.qteFinisherPhase == 2)
    {
        const float c = 1.0F - std::clamp(game.run.qteFinisherTimer / UpdateConstants::qteFinisherCharge, 0.0F, 1.0F);
        for (int32_t i = 0; i < 14; i++)
        {
            // Streaks rushing in from all round, faster as the charge fills.
            const float a = static_cast<float>(i) * 0.449F + t * 2.0F;
            const float run = std::fmod(t * (2.0F + 4.0F * c) + static_cast<float>(i) * 0.37F, 1.0F);
            const float d0 = r * 9.0F * (1.0F - run);
            const Vector2 dirIn{.x = std::cos(a), .y = std::sin(a)};
            DrawLineEx(Vector2Add(nose, Vector2Scale(dirIn, d0 + r * 2.5F)), Vector2Add(nose, Vector2Scale(dirIn, d0)), 2.0F,
                       Fade(Palette::Charge, 0.3F + 0.6F * c));
        }
        const float throb = 1.0F + 0.12F * std::sin(t * 40.0F);
        DrawCircleV(nose, r * (0.6F + 1.6F * c) * throb, Fade(Palette::Charge, 0.25F + 0.35F * c));
        DrawCircleV(nose, r * (0.3F + 0.9F * c) * throb, Fade(Palette::Crit, 0.6F + 0.4F * c));
        DrawCircleV(nose, r * (0.15F + 0.45F * c), WHITE);
        for (int32_t k = 0; k < 2; k++)
        {
            const float ring = r * (2.6F - 1.6F * std::fmod(c * 3.0F + static_cast<float>(k) * 0.5F, 1.0F));
            DrawRing(nose, ring - 2.0F, ring, 0, 360, 32, Fade(Palette::Crit, 0.6F * c));
        }
    }
    else if (game.run.qteFinisherPhase == 3)
    {
        const float f = std::clamp(game.run.qteFinisherTimer / UpdateConstants::qteFinisherShot, 0.0F, 1.0F);
        const float grow = std::min(1.0F, (1.0F - f) * 4.0F); // snaps to full width in the first quarter
        drawFinisherBeam(nose, game.run.qteFinisherTarget, r * (0.6F + 1.6F * grow), 1.0F);
        DrawCircleV(nose, r * 2.2F * grow, Fade(WHITE, 0.85F));
        DrawCircleV(game.run.qteFinisherTarget, r * (1.5F + 2.5F * grow), Fade(WHITE, 0.8F));
        DrawCircleV(p, 4000.0F, Fade(WHITE, 0.08F * grow)); // the whole view flares
    }
    if (game.run.finisherFxTimer > 0)
    {
        // The shot has landed: the beam thins away, the hit point flashes, shockwaves spread.
        const float k = game.run.finisherFxTimer / UpdateConstants::qteFinisherFx;
        drawFinisherBeam(game.run.finisherFxFrom, game.run.finisherFxTo, r * 2.2F * k * k, k);
        const Vector2 hit = game.run.finisherFxTo;
        DrawCircleV(hit, r * 4.0F * k, Fade(WHITE, 0.9F * k));
        for (int32_t w = 0; w < 3; w++)
        {
            const float spread = std::clamp((1.0F - k) * 1.4F - static_cast<float>(w) * 0.15F, 0.0F, 1.0F);
            const float ring = r * 16.0F * spread;
            if (ring > 2.0F)
            {
                DrawRing(hit, ring - 3.0F * (1.0F - spread) - 1.0F, ring, 0, 360, 64,
                         Fade(w == 0 ? WHITE : Palette::Charge, 0.8F * (1.0F - spread)));
            }
        }
    }

    if (game.run.player.nerveCharging)
    {
        const float progress =
            1.0F - game.run.player.nerveChargeTimer / UpdateConstants::nerveBurstWindup;
        const float flare = 0.4F + 0.6F * progress;
        DrawCircleLines(static_cast<int32_t>(p.x), static_cast<int32_t>(p.y),
                        r * (1.2F + flare * 0.6F), Fade(Palette::Charge, flare));
        DrawCircleV(p, r * 0.5F * flare, Fade(WHITE, flare));
    }
}

void drawNerveBallProjectiles(const Game& game)
{
    for (const auto& proj : game.run.nerveBallProjectiles)
    {
        if (!proj.active)
        {
            continue;
        }
        drawNerveBall(proj.position, proj.radius, proj.evolved);
    }
}

void drawNerveSpiralProjectiles(const Game& game)
{
    for (const auto& spiral : game.run.nerveSpiralProjectiles)
    {
        if (!spiral.active)
        {
            continue;
        }
        const Vector2& center = spiral.origin;

        for (int32_t s = 0; s < spiral.bladeCount; s++)
        {
            const float angle = spiral.age * 14.0F + static_cast<float>(s) * 2 *
                                                         std::numbers::pi_v<float> /
                                                         static_cast<float>(spiral.bladeCount);
            const Vector2 bladePos =
                Vector2Add(center, Vector2{.x = std::cos(angle) * spiral.spinRadius,
                                           .y = std::sin(angle) * spiral.spinRadius});
            drawSpiralBlade(bladePos, spiral.evolved);
        }
    }
}

void drawClusterEnemyShape(Vector2 center, float baseRadius, float seed, Color color)
{
    const std::array<Color, 4> palette{color, Palette::PunctumHaze, Palette::Crit, Palette::Void};

    constexpr int32_t pieceCount = 6;
    for (int32_t i = 0; i < pieceCount; i++)
    {
        const float pieceSeed = seed * 7.0F + static_cast<float>(i) * 19.0F;
        const float angle = hashNoise(pieceSeed) * 2.0F * std::numbers::pi_v<float>;
        const float dist = hashNoise(pieceSeed + 1.0F) * baseRadius * 0.65F;
        const Vector2 pos =
            Vector2Add(center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        const float pieceRadius = baseRadius * (0.35F + hashNoise(pieceSeed + 2.0F) * 0.45F);
        const auto colorIdx =
            static_cast<size_t>(hashNoise(pieceSeed + 3.0F) * static_cast<float>(palette.size())) %
            palette.size();
        const Color pieceColor = palette.at(colorIdx);

        if (hashNoise(pieceSeed + 5.0F) < 0.35F)
        {
            DrawCircleV(pos, pieceRadius, pieceColor);
            DrawCircleLines(static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y), pieceRadius,
                            Fade(Palette::Void, 0.5F));
        }
        else
        {
            const int32_t sides = 3 + static_cast<int32_t>(hashNoise(pieceSeed + 4.0F) * 6.0F) % 6;
            const float rot = hashNoise(pieceSeed + 6.0F) * 360.0F;
            DrawPoly(pos, sides, pieceRadius, rot, pieceColor);
            DrawPolyLines(pos, sides, pieceRadius, rot, Fade(Palette::Void, 0.5F));
        }
    }

    DrawCircleV(center, baseRadius * 0.22F, ColorLerp(color, Palette::Void, 0.5F));

    const auto t = static_cast<float>(GetTime());
    constexpr int32_t eyeCount = 2;
    for (int32_t i = 0; i < eyeCount; i++)
    {
        const float eyeSeed = seed * 11.0F + static_cast<float>(i) * 31.0F;
        const float angle = hashNoise(eyeSeed) * 2.0F * std::numbers::pi_v<float>;
        const float dist = hashNoise(eyeSeed + 1.0F) * baseRadius * 0.5F;
        const Vector2 pos =
            Vector2Add(center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        const float pulse = 0.5F + 0.5F * std::sin(t * 4.0F + eyeSeed);
        DrawCircleV(pos, baseRadius * 0.1F, Fade(Palette::Crit, 0.5F + 0.4F * pulse));
    }
}

void drawOrganicEnemyShape(Vector2 center, float baseRadius, float seed, Color color)
{
    const int32_t sides = 4 + static_cast<int32_t>(hashNoise(seed * 3.7F) * 5.0F) % 5;
    const float hullRotation = hashNoise(seed * 5.3F) * 360.0F;
    drawPanelShape(center, baseRadius, sides, hullRotation,
                   ColorLerp(Palette::StructMid, Palette::Void, 0.1F), seed * 90.0F);

    const float blobScale = 0.55F + hashNoise(seed * 9.0F) * 0.7F;
    const float blobRadius = baseRadius * blobScale;
    const float offsetAngle = hashNoise(seed * 2.3F) * 2.0F * std::numbers::pi_v<float>;
    const float offsetDist = baseRadius * (1.0F - std::min(blobScale, 1.0F)) * 0.6F;
    const Vector2 blobCenter = Vector2Add(center, Vector2{.x = std::cos(offsetAngle) * offsetDist,
                                                          .y = std::sin(offsetAngle) * offsetDist});

    constexpr int32_t tendrilCount = 3;
    for (int32_t i = 0; i < tendrilCount; i++)
    {
        const float tendrilSeed = seed * 4.3F + static_cast<float>(i) * 29.0F;
        const float angle = hashNoise(tendrilSeed) * 2.0F * std::numbers::pi_v<float>;
        const Vector2 dir{.x = std::cos(angle), .y = std::sin(angle)};
        const Vector2 perp{.x = -dir.y, .y = dir.x};
        const Vector2 base = Vector2Add(blobCenter, Vector2Scale(dir, blobRadius * 0.75F));
        const Vector2 tip = Vector2Add(blobCenter, Vector2Scale(dir, blobRadius * 1.5F));
        DrawTriangle(Vector2Add(base, Vector2Scale(perp, 3)), tip,
                     Vector2Subtract(base, Vector2Scale(perp, 3)),
                     ColorLerp(color, Palette::Void, 0.35F));
    }

    std::array<Vector2, static_cast<size_t>(organicVertexCount)> verts{};
    for (int32_t i = 0; i < organicVertexCount; i++)
    {
        const float angle =
            static_cast<float>(i) * (360.0F / static_cast<float>(organicVertexCount)) * DEG2RAD;
        const float r = organicVertexRadius(blobRadius, i, seed);
        verts.at(static_cast<size_t>(i)) =
            Vector2Add(blobCenter, Vector2{.x = std::cos(angle) * r, .y = std::sin(angle) * r});
    }
    for (size_t i = 0; i < verts.size(); i++)
    {

        DrawTriangle(blobCenter, verts.at((i + 1) % verts.size()), verts.at(i), color);
    }

    const auto t = static_cast<float>(GetTime());
    constexpr int32_t pustuleCount = 2;
    for (int32_t i = 0; i < pustuleCount; i++)
    {
        const float pustuleSeed = seed * 6.7F + static_cast<float>(i) * 13.0F;
        const float angle = hashNoise(pustuleSeed) * 2.0F * std::numbers::pi_v<float>;
        const float dist = hashNoise(pustuleSeed + 1.0F) * blobRadius * 0.5F;
        const Vector2 pos = Vector2Add(
            blobCenter, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        const float pulse = 0.5F + 0.5F * std::sin(t * 3.0F + pustuleSeed);
        DrawCircleV(pos, blobRadius * 0.13F, Fade(Palette::Heal, 0.45F + 0.4F * pulse));
    }
}

// Husk Sentinel: a rusted wreck turret whose barrel tracks the player, crusted over with bloom.
void drawHuskSentinelShape(Vector2 center, float radius, float seed, Vector2 aim, Color color)
{
    const float alpha = static_cast<float>(color.a) / 255.0F;
    drawPanelShape(center, radius, 6, hashNoise(seed * 5.3F) * 360.0F,
                   ColorAlpha(ColorLerp(color, Palette::StructDark, 0.45F), alpha), seed * 90.0F);

    const Vector2 muzzle = Vector2Add(center, Vector2Scale(Vector2Normalize(aim), radius * 1.35F));
    DrawLineEx(center, muzzle, radius * 0.32F,
               ColorAlpha(ColorLerp(Palette::StructMid, color, 0.4F), alpha));
    DrawCircleV(muzzle, radius * 0.17F, ColorAlpha(Palette::StructDark, alpha));
    DrawCircleV(center, radius * 0.5F, color);
    DrawCircleLines(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y), radius * 0.5F,
                    Fade(Palette::StructDark, 0.7F * alpha));

    constexpr int32_t crustCount = 6;
    for (int32_t i = 0; i < crustCount; i++)
    {
        const float crustSeed = seed * 3.1F + static_cast<float>(i) * 17.0F;
        const float angle = hashNoise(crustSeed) * 2.0F * std::numbers::pi_v<float>;
        const float dist = radius * (0.45F + 0.45F * hashNoise(crustSeed + 1.0F));
        const Vector2 pos = Vector2Add(
            center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        const float lumpRadius = radius * (0.16F + 0.14F * hashNoise(crustSeed + 2.0F));
        const Color lump = ColorLerp(Palette::RustbloomAccent, Palette::RustbloomVoid,
                                     0.35F * hashNoise(crustSeed + 3.0F));
        DrawCircleV(pos, lumpRadius, ColorAlpha(lump, alpha));
        DrawCircleV(Vector2Add(pos, Vector2{.x = -lumpRadius * 0.3F, .y = -lumpRadius * 0.3F}),
                    lumpRadius * 0.4F, Fade(Palette::Heal, 0.5F * alpha));
    }
}

// Carrion Drone: green rot spots over its rust haze body.
void drawCarrionSpots(Vector2 center, float radius, float seed, Color color)
{
    const float alpha = static_cast<float>(color.a) / 255.0F;
    constexpr int32_t spotCount = 4;
    for (int32_t i = 0; i < spotCount; i++)
    {
        const float spotSeed = seed * 8.9F + static_cast<float>(i) * 23.0F;
        const float angle = hashNoise(spotSeed) * 2.0F * std::numbers::pi_v<float>;
        const float dist = hashNoise(spotSeed + 1.0F) * radius * 0.45F;
        const Vector2 pos = Vector2Add(
            center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        DrawCircleV(pos, radius * (0.12F + 0.08F * hashNoise(spotSeed + 2.0F)),
                    ColorAlpha(Palette::RustbloomAccent, alpha));
    }
}

void drawElementalDebuffEffects(Vector2 pos, float radius, bool burning, bool shocked, bool frozen)
{
    const auto t = static_cast<float>(GetTime());

    if (burning)
    {
        constexpr int32_t flameCount = 4;
        for (int32_t i = 0; i < flameCount; i++)
        {
            const float seed = pos.x * 0.13F + pos.y * 0.29F + static_cast<float>(i) * 17.0F;
            const float angle = std::fmod(seed + t * 1.5F, 2.0F * std::numbers::pi_v<float>);
            const float bob = 0.5F + 0.5F * std::sin(t * 9.0F + seed);
            const Vector2 base{.x = pos.x + std::cos(angle) * radius * 0.6F,
                               .y = pos.y + std::sin(angle) * radius * 0.6F - radius * 0.2F};
            const Vector2 tip{.x = base.x + std::sin(t * 14.0F + seed) * 3.0F,
                              .y = base.y - (6.0F + bob * 7.0F)};
            const Color flameColor =
                bob > 0.6F ? Palette::ElementBurn : ColorLerp(Palette::ElementBurn, YELLOW, 0.5F);
            DrawTriangle(Vector2{.x = base.x - 3, .y = base.y}, tip,
                         Vector2{.x = base.x + 3, .y = base.y}, Fade(flameColor, 0.65F));
        }
    }

    if (shocked)
    {
        constexpr int32_t boltCount = 2;
        constexpr int32_t segments = 3;
        const auto crackleStep = static_cast<float>(static_cast<int32_t>(t * 10.0F));
        for (int32_t i = 0; i < boltCount; i++)
        {
            float boltSeed =
                pos.x * 0.7F + pos.y * 0.31F + crackleStep * 3.1F + static_cast<float>(i) * 91.0F;
            const float startAngle = hashNoise(boltSeed) * 2.0F * std::numbers::pi_v<float>;
            Vector2 point{.x = pos.x + std::cos(startAngle) * radius,
                          .y = pos.y + std::sin(startAngle) * radius};
            for (int32_t s = 0; s < segments; s++)
            {
                boltSeed += 7.7F;
                const Vector2 next{.x = pos.x + (hashNoise(boltSeed) - 0.5F) * 2.0F * radius,
                                   .y =
                                       pos.y + (hashNoise(boltSeed + 3.3F) - 0.5F) * 2.0F * radius};
                DrawLineEx(point, next, 1.5F, Fade(Palette::ElementStatic, 0.8F));
                point = next;
            }
        }
    }

    if (frozen)
    {
        DrawCircleLines(static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y), radius + 3,
                        Fade(Palette::ElementFreeze, 0.55F));
        DrawCircleLines(static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y), radius + 5,
                        Fade(Palette::ElementFreeze, 0.3F));
    }
}

void drawEnemy(const Game& game, const Enemy& enemy, bool buffed)
{
    const auto& kind = enemyKinds.at(static_cast<size_t>(enemy.kind));
    Color color = kind.color;
    // Global Enemies pick up a light dust tint from the biome they fight in.
    if (!kind.biomeExclusive && currentBiome(game.run.waveNumber) == Biome::ShatteredBelt)
    {
        color = ColorLerp(color, Palette::ShatteredBeltAccent, beltDustTint);
    }
    else if (!kind.biomeExclusive && currentBiome(game.run.waveNumber) == Biome::Rustbloom)
    {
        const Color bloom = ColorLerp(Palette::RustbloomHaze, Palette::RustbloomAccent, 0.5F);
        color = ColorLerp(color, bloom, rustbloomTint);
    }
    if (enemy.fadeAlpha < 1.0F)
    {
        color = Fade(color, enemy.fadeAlpha);
    }
    if (enemy.phased)
    {
        color = Fade(color, 0.35F);
    }
    if (enemy.hitFlashTimer > 0)
    {
        color = ColorLerp(color, WHITE, enemy.hitFlashTimer / UpdateConstants::hitFlashDuration);
    }
    if (enemy.burnDps > 0)
    {

        const float flicker = 0.6F + 0.4F * std::sin(static_cast<float>(GetTime()) * 10.0F);
        color = ColorLerp(color, Palette::ElementBurn, 0.5F * flicker);
    }
    if (enemy.isElite)
    {
        DrawCircleLines(static_cast<int32_t>(enemy.position.x),
                        static_cast<int32_t>(enemy.position.y), kind.radius + 4, Palette::Crit);
    }

    if (buffed)
    {
        const Vector2 tip{.x = enemy.position.x, .y = enemy.position.y - kind.radius - 14};
        const Vector2 left{.x = tip.x - 5, .y = tip.y + 6};
        const Vector2 right{.x = tip.x + 5, .y = tip.y + 6};
        DrawTriangle(left, tip, right, Palette::Accent);
    }

    const auto t = static_cast<float>(GetTime());
    if (enemy.kind == enemyKindHacker && enemy.hackRayOn)
    {
        // The ray: a soft wide glow with a bright core, both swelling as the hack fills; a ring
        // round the ship fills with the exposure.
        const float fill = std::clamp(enemy.hackExposure / hackExposureTime, 0.0F, 1.0F);
        const Vector2 end = hackRayEnd(enemy);
        const float flicker = 0.75F + 0.25F * std::sin(t * 30.0F);
        DrawLineEx(enemy.position, end, hackRayHalfWidth * (0.8F + 1.2F * fill), Fade(Palette::Heal, 0.16F + 0.2F * fill));
        DrawLineEx(enemy.position, end, 2.0F + 5.0F * fill, Fade(ColorLerp(Palette::Heal, WHITE, 0.7F), (0.7F + 0.3F * fill) * flicker));
        if (fill > 0)
        {
            const Vector2 ship = game.run.player.position;
            const float r = game.run.player.radius + 14.0F;
            DrawRing(ship, r, r + 3.0F, 0, 360, 32, Fade(Palette::StructDark, 0.7F));
            DrawRing(ship, r, r + 3.0F, -90, -90 + 360 * fill, 32, ColorLerp(Palette::Heal, Palette::Accent, fill));
        }
    }
    if (kind.pattern == EnemyPattern::Charge && enemy.telegraphing)
    {
        // Sniper-style aim: a line through the player that thickens and blinks faster to the lock.
        const float lock = 1.0F - std::clamp(enemy.stateTimer / chargeTelegraphDuration, 0.0F, 1.0F);
        const Vector2 toPlayer = Vector2Subtract(game.run.player.position, enemy.position);
        const Vector2 lineEnd = Vector2Add(
            enemy.position, Vector2Scale(Vector2Normalize(toPlayer),
                                         Vector2Length(toPlayer) + UpdateConstants::enemyChargeOvershoot));
        const float blink = 0.5F + 0.5F * std::sin(t * (10.0F + 30.0F * lock));
        DrawLineEx(enemy.position, lineEnd, 1.0F + 4.0F * lock, Fade(Palette::Accent, 0.25F + 0.5F * blink));
        DrawCircleLines(static_cast<int32_t>(lineEnd.x), static_cast<int32_t>(lineEnd.y),
                        6.0F + 10.0F * (1.0F - lock), Fade(Palette::Accent, 0.6F));
    }
    if (kind.pattern == EnemyPattern::Charge && enemy.charging)
    {
        // Afterimages and a tapering streak behind the charge.
        const Vector2 back = Vector2Scale(Vector2Normalize(enemy.velocity), -1.0F);
        DrawLineEx(enemy.position, Vector2Add(enemy.position, Vector2Scale(back, kind.radius * 7.0F)),
                   kind.radius * 0.8F, Fade(color, 0.18F));
        for (int32_t k = 1; k <= 4; k++)
        {
            const float f = static_cast<float>(k);
            drawEnemyShape(kind.shape, Vector2Add(enemy.position, Vector2Scale(back, f * kind.radius * 1.3F)),
                           kind.radius * (1.0F - f * 0.12F), std::atan2(-back.y, -back.x) * RAD2DEG,
                           Fade(color, 0.35F - f * 0.07F));
        }
    }
    if (enemy.kind == enemyKindBomber && (enemy.telegraphing || enemy.charging))
    {
        // Dive: a tell line through the player, then a streak behind the dive.
        if (enemy.telegraphing)
        {
            const float lock = 1.0F - std::clamp(enemy.stateTimer / bomberDiveTell, 0.0F, 1.0F);
            const Vector2 dir = Vector2Normalize(Vector2Subtract(game.run.player.position, enemy.position));
            DrawLineEx(enemy.position,
                       Vector2Add(game.run.player.position, Vector2Scale(dir, bomberDiveOvershoot)), 1.0F + 3.0F * lock,
                       Fade(Palette::Accent, 0.3F + 0.5F * lock));
        }
        else
        {
            const Vector2 back = Vector2Scale(Vector2Normalize(enemy.velocity), -kind.radius * 6.0F);
            DrawLineEx(enemy.position, Vector2Add(enemy.position, back), kind.radius * 0.8F, Fade(color, 0.2F));
        }
    }
    else if (kind.pattern == EnemyPattern::Turret && enemy.kind != enemyKindSniper && (enemy.telegraphing || enemy.charging))
    {
        // Turret: a ring closes in on it through the tell; a soft flash while it bursts.
        if (enemy.telegraphing)
        {
            const float p = 1.0F - std::clamp(enemy.stateTimer / turretPatternTell, 0.0F, 1.0F);
            const float r = kind.radius * (3.0F - 2.0F * p);
            DrawRing(enemy.position, r - 2.0F, r, 0, 360, 32, Fade(Palette::Accent, 0.3F + 0.6F * p));
        }
        else
        {
            DrawCircleV(enemy.position, kind.radius * 1.6F, Fade(Palette::Accent, 0.15F + 0.1F * std::sin(t * 40.0F)));
        }
    }
    else if (kind.fuseRadius > 0 && enemy.telegraphing)
    {
        // Lit fuse: the blast radius fills in while a countdown arc runs out round the body.
        const float p = 1.0F - std::clamp(enemy.stateTimer / kind.fuseTime, 0.0F, 1.0F);
        DrawCircleV(enemy.position, kind.explodeRadius * p, Fade(Palette::Accent, 0.12F));
        DrawCircleLines(static_cast<int32_t>(enemy.position.x), static_cast<int32_t>(enemy.position.y),
                        kind.explodeRadius, Fade(Palette::Accent, 0.3F + 0.5F * p));
        DrawRing(enemy.position, kind.radius + 4.0F, kind.radius + 7.0F, -90.0F,
                 -90.0F + 360.0F * (1.0F - p), 32, Palette::Crit);
        const float blink = std::sin(t * (8.0F + 40.0F * p)) > 0 ? 1.0F : 0.0F;
        color = ColorLerp(color, WHITE, 0.7F * blink);
    }

    const bool rollTell = kind.rollCharge && enemy.telegraphing;
    const float rollGlow = rollTell ? 1.0F - enemy.stateTimer / rockhideRollTell : 0.0F;
    if (rollTell)
    {
        // The roll line shows where to sidestep; the cracks glow on top of the body below.
        const Vector2 toPlayer = Vector2Subtract(game.run.player.position, enemy.position);
        const Vector2 lineEnd = Vector2Add(
            enemy.position, Vector2Scale(Vector2Normalize(toPlayer),
                                         Vector2Length(toPlayer) + UpdateConstants::enemyChargeOvershoot));
        DrawLineEx(enemy.position, lineEnd, 3, Fade(Palette::ShatteredBeltAccent, 0.6F * rollGlow));
    }

    if (kind.pulseInsteadOfSpawn)
    {
        DrawCircleLines(static_cast<int32_t>(enemy.position.x), static_cast<int32_t>(enemy.position.y),
                        kind.pulseRadius, Fade(Palette::PunctumAccent, 0.12F)); // its reach, always
    }
    if (kind.pulseInsteadOfSpawn && enemy.stateTimer < 0.6F)
    {
        const float warn = 1.0F - enemy.stateTimer / 0.6F;
        DrawCircleLines(static_cast<int32_t>(enemy.position.x),
                        static_cast<int32_t>(enemy.position.y), kind.pulseRadius,
                        Fade(Palette::PunctumAccent, warn * 0.6F));
    }

    if (kind.pattern == EnemyPattern::Turret && enemy.kind != enemyKindHacker && // its ray is its tell
        Vector2Distance(enemy.position, game.run.player.position) < 700.0F)
    {
        float blink = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 12.0F);
        DrawLineEx(enemy.position, game.run.player.position, 2, Fade(Palette::Crit, blink * 0.5F));
    }

    // What the body points along: its aim for what shoots, its travel for everything else.
    Vector2 heading = Vector2Subtract(game.run.player.position, enemy.position);
    if (enemy.kind == enemyKindHacker && enemy.hackRayOn)
    {
        heading = Vector2Subtract(hackRayEnd(enemy), enemy.position);
    }
    else if (kind.pattern != EnemyPattern::Turret && Vector2LengthSqr(enemy.velocity) > 0.01F)
    {
        heading = enemy.velocity;
    }
    const float facing = std::atan2(heading.y, heading.x) * RAD2DEG;
    if (enemy.kind == enemyKindHuskSentinel)
    {
        drawHuskSentinelShape(enemy.position, kind.radius * enemy.organicRadiusMult,
                              enemy.organicSeed,
                              Vector2Subtract(game.run.player.position, enemy.position), color);
    }
    else if (kind.shape == EnemyShape::Organic)
    {
        drawOrganicEnemyShape(enemy.position, kind.radius * enemy.organicRadiusMult,
                              enemy.organicSeed, color);
        if (enemy.kind == enemyKindCarrionDrone)
        {
            drawCarrionSpots(enemy.position, kind.radius * enemy.organicRadiusMult,
                             enemy.organicSeed, color);
        }
    }
    else if (kind.shape == EnemyShape::Cluster)
    {
        drawClusterEnemyShape(enemy.position, kind.radius, enemy.organicSeed, color);
    }
    else
    {
        drawEnemyShape(kind.shape, enemy.position, kind.radius, facing, color,
                       static_cast<float>(enemy.id % 97));
    }
    drawElementalDebuffEffects(enemy.position, kind.radius, enemy.burnDps > 0, enemy.debuffStatic,
                               enemy.debuffFreeze);
    if (rollTell)
    {
        const Color amber = Fade(Palette::ShatteredBeltAccent, 0.4F + 0.6F * rollGlow);
        for (int32_t k = 0; k < 3; k++)
        {
            const float angle = enemy.organicSeed + static_cast<float>(k) * 2.1F;
            const Vector2 tip{.x = std::cos(angle) * kind.radius * 0.9F,
                              .y = std::sin(angle) * kind.radius * 0.9F};
            DrawLineEx(enemy.position, Vector2Add(enemy.position, tip), 3, amber);
        }
    }

    constexpr float enemyHealthMult = 1.2F;
    auto maxHealth = static_cast<int32_t>(static_cast<float>(kind.health) * enemyHealthMult *
                                          enemyHealthScale(game));
    if (enemy.isElite)
    {
        maxHealth *= 2;
    }
    const float healthFrac = static_cast<float>(enemy.health) / static_cast<float>(maxHealth);
    if (healthFrac < 1 && healthFrac > 0)
    {
        const float barWidth = kind.radius * 2;
        DrawRectangle(static_cast<int32_t>(enemy.position.x - kind.radius),
                      static_cast<int32_t>(enemy.position.y - kind.radius - 8),
                      static_cast<int32_t>(barWidth), 3, Fade(Palette::StructDark, 0.6F));
        DrawRectangle(static_cast<int32_t>(enemy.position.x - kind.radius),
                      static_cast<int32_t>(enemy.position.y - kind.radius - 8),
                      static_cast<int32_t>(barWidth * healthFrac), 3, Palette::Accent);
    }
}

void drawEliteHazard(const EliteHazard& hazard, Color body)
{
    const Color auraColor =
        hazard.role == EliteHazardRole::Warlord ? Palette::Accent : Palette::Shield;

    const bool warlord = hazard.role == EliteHazardRole::Warlord;
    drawEliteHazardAura(hazard.position, EliteHazardConstants::auraRadius, auraColor, warlord ? 8.0F : -8.0F);
    // A dark hull would vanish into the void: lift it a little toward its signal colour.
    drawEliteHazardShape(hazard.role, hazard.position, EliteHazardConstants::radius,
                         ColorLerp(body, auraColor, 0.18F), auraColor);

    const float healthFrac =
        static_cast<float>(hazard.health) / static_cast<float>(hazard.maxHealth);
    const float barWidth = EliteHazardConstants::radius * 2;
    DrawRectangle(static_cast<int32_t>(hazard.position.x - EliteHazardConstants::radius),
                  static_cast<int32_t>(hazard.position.y - EliteHazardConstants::radius - 10),
                  static_cast<int32_t>(barWidth), 4, Fade(Palette::StructDark, 0.6F));
    DrawRectangle(static_cast<int32_t>(hazard.position.x - EliteHazardConstants::radius),
                  static_cast<int32_t>(hazard.position.y - EliteHazardConstants::radius - 10),
                  static_cast<int32_t>(barWidth * healthFrac), 4, auraColor);
}

void drawOrbitBlades(const Game& game)
{
    for (const auto& w : game.run.weapons)
    {
        const Color color = w.evolved ? evolvedWeaponColor(w.type) : weaponColor(w.type);

        switch (w.type)
        {
        case WeaponType::Orbit:
        {
            float radius = orbitRadius(w.level);
            if (w.evolved)
            {
                radius *= 1.4F;
            }
            const int32_t count = orbitBladeCount(w.level);

            for (int32_t s = 0; s < count; s++)
            {
                const Vector2 pos =
                    orbitBladePosition(game, game.run.player.position, radius, s, count);
                drawOrbitRingBlade(pos, color, w.evolved);
            }

            if (w.flashTimer > 0)
            {

                const int32_t shots = std::max(1, count / 2);
                constexpr float regrowDuration = 0.35F;
                const float progress = 1 - w.flashTimer / regrowDuration;

                for (int32_t s = 0; s < shots; s++)
                {
                    const Vector2 regrowTarget =
                        orbitBladePosition(game, game.run.player.position, radius, s, count);
                    const Vector2 regrowPos =
                        Vector2Lerp(game.run.player.position, regrowTarget, progress);
                    DrawCircleV(regrowPos, 5 * progress, Fade(WHITE, 1 - progress));
                }
            }
            break;
        }
        case WeaponType::Beam:
        {
            const Vector2 dir = beamAimDirection(game, w.evolved);
            const float length = beamLength(game, w.level, w.evolved);
            const Vector2 start = game.run.player.position;
            const Vector2 end = Vector2Add(start, Vector2Scale(dir, length));
            DrawLineEx(start, end, 4, Fade(color, 0.85F));
            DrawLineEx(start, end, 9, Fade(color, 0.2F));
            break;
        }
        case WeaponType::Flamethrower:
        {
            // The jets themselves are flame particles (drawn with the rest); here only the nozzle:
            // a white-hot flare while firing, and while it reheats a glow that swells as embers are
            // drawn in toward it.
            const float t = static_cast<float>(GetTime());
            const Vector2 nozzle = game.run.player.position;
            const float hw = flamethrowerRayHalfWidth(w.level);
            if (flamethrowerFiring(game))
            {
                DrawCircleV(nozzle, hw * (2.2F + 0.3F * std::sin(t * 40.0F)), Fade(Palette::Crit, 0.5F));
                for (const Vector2& dir : flamethrowerRayDirs(game))
                {
                    DrawLineEx(nozzle, Vector2Add(nozzle, Vector2Scale(dir, flamethrowerRayLength)), 1.0F,
                               Fade(Palette::Charge, 0.18F));
                }
            }
            else
            {
                const float charge = (game.run.flamethrowerCycle - flamethrowerBurstDuration) / flamethrowerChargeDuration;
                DrawCircleV(nozzle, hw * (0.6F + 1.8F * charge), Fade(Palette::Accent, 0.25F + 0.35F * charge));
                for (int32_t k = 0; k < 8; k++)
                {
                    const float a = static_cast<float>(k) * 0.785F + t * 1.5F;
                    const float d = hw * 7.0F * (1.0F - std::fmod(charge * 2.0F + static_cast<float>(k) * 0.125F, 1.0F));
                    DrawCircleV(Vector2Add(nozzle, Vector2{.x = std::cos(a) * d, .y = std::sin(a) * d}), 2.0F,
                                Fade(Palette::Charge, 0.4F + 0.5F * charge));
                }
            }
            break;
        }
        case WeaponType::Shock:
        {
            const float maxRadius = shockwaveRadius(w.level, w.evolved);
            if (w.flashTimer > 0)
            {

                const float progress = 1 - w.flashTimer / UpdateConstants::shockFlashDuration;
                const float pulseRadius = maxRadius * progress;
                DrawCircleLines(static_cast<int32_t>(game.run.player.position.x),
                                static_cast<int32_t>(game.run.player.position.y), pulseRadius,
                                Fade(color, 0.7F * (1 - progress)));
            }
            DrawCircleLines(static_cast<int32_t>(game.run.player.position.x),
                            static_cast<int32_t>(game.run.player.position.y), maxRadius,
                            Fade(color, 0.15F));
            break;
        }
        default:
            break;
        }
    }
}

void drawOrbitBladeProjectiles(const Game& game)
{
    for (const auto& proj : game.run.orbitBladeProjectiles)
    {
        if (!proj.active)
        {
            continue;
        }
        drawOrbitShot(proj.position, proj.radius, proj.evolved);
    }
}

void drawShieldIndicator(const Player& player)
{
    const float ringRadius = player.radius + 10;

    if (player.shieldCooldownTimer > 0)
    {
        const float progress =
            1 - player.shieldCooldownTimer / UpdateConstants::shieldCooldownDuration;
        DrawCircleLines(static_cast<int32_t>(player.position.x),
                        static_cast<int32_t>(player.position.y), ringRadius,
                        Fade(Palette::StructMid, 0.5F));
        DrawRing(player.position, ringRadius - 2, ringRadius, -90, -90 + 360 * progress, 32,
                 Palette::Shield);
    }
    else
    {
        const float pulse = 0.4F + 0.2F * std::sin(static_cast<float>(GetTime()) * 3);
        DrawCircleLines(static_cast<int32_t>(player.position.x),
                        static_cast<int32_t>(player.position.y), ringRadius,
                        Fade(Palette::Shield, pulse));
    }
}

void drawChargeParticles(Vector2 center, float fraction, Color color, float maxRadius,
                         float minRadius)
{
    constexpr int count = 6;

    const auto t = static_cast<float>(GetTime());
    const float radius = maxRadius - (maxRadius - minRadius) * fraction;

    for (int i = 0; i < count; i++)
    {
        const float angle = t * 4 + static_cast<float>(i) * (2 * std::numbers::pi_v<float> / count);
        const Vector2 pos{.x = center.x + std::cos(angle) * radius,
                          .y = center.y + std::sin(angle) * radius};
        DrawCircleV(pos, 3, color);
    }
}

} // namespace DrawDetail
