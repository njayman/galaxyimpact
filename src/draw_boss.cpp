#include "draw_internal.hpp"

namespace DrawDetail
{

void drawMeteorBossShape(Vector2 center, float radius, int32_t seed, Color baseColor)
{
    const auto t = static_cast<float>(GetTime());
    const auto seedF = static_cast<float>(seed);

    const float glowPulse = 0.85F + 0.15F * std::sin(t * 2.4F + seedF);
    DrawCircleV(center, radius * 1.5F * glowPulse, Fade(Palette::ElementBurn, 0.16F));
    DrawCircleV(center, radius * 1.25F * glowPulse, Fade(Palette::ElementBurn, 0.26F));
    DrawCircleLines(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y),
                    radius * 1.08F * glowPulse, Fade(YELLOW, 0.55F));

    constexpr int32_t vertexCount = 16;
    std::array<Vector2, vertexCount> verts{};
    for (int32_t i = 0; i < vertexCount; i++)
    {
        const float angle = static_cast<float>(i) * (360.0F / vertexCount) * DEG2RAD;
        const float noise = hashNoise(seedF * 7.3F + static_cast<float>(i) * 2.9F);
        const float r = radius * (0.76F + noise * 0.34F);
        verts.at(static_cast<size_t>(i)) =
            Vector2Add(center, Vector2{.x = std::cos(angle) * r, .y = std::sin(angle) * r});
    }
    for (int32_t i = 0; i < vertexCount; i++)
    {
        DrawTriangle(center, verts.at(static_cast<size_t>((i + 1) % vertexCount)),
                     verts.at(static_cast<size_t>(i)), ColorLerp(baseColor, Palette::Void, 0.45F));
    }

    for (int32_t i = 0; i < vertexCount; i++)
    {
        const Vector2 innerA = Vector2Lerp(center, verts.at(static_cast<size_t>(i)), 0.8F);
        const Vector2 innerB =
            Vector2Lerp(center, verts.at(static_cast<size_t>((i + 1) % vertexCount)), 0.8F);
        DrawTriangle(center, innerB, innerA, ColorLerp(baseColor, Palette::Void, 0.22F));
    }

    constexpr int32_t craterCount = 8;
    for (int32_t i = 0; i < craterCount; i++)
    {
        const float cs = seedF * 13.0F + static_cast<float>(i) * 53.0F;
        const float craterAngle = hashNoise(cs) * 2.0F * std::numbers::pi_v<float>;
        const float craterDist = hashNoise(cs + 3.1F) * radius * 0.62F;
        const float craterRadius = radius * (0.07F + hashNoise(cs + 5.0F) * 0.13F);
        const Vector2 craterPos =
            Vector2Add(center, Vector2{.x = std::cos(craterAngle) * craterDist,
                                       .y = std::sin(craterAngle) * craterDist});

        DrawCircleV(craterPos, craterRadius, ColorLerp(baseColor, Palette::Void, 0.55F));
        DrawCircleLines(static_cast<int32_t>(craterPos.x), static_cast<int32_t>(craterPos.y),
                        craterRadius, Fade(ColorLerp(baseColor, WHITE, 0.35F), 0.6F));
        DrawCircleV(craterPos, craterRadius * 0.42F,
                    Fade(ColorLerp(baseColor, Palette::Void, 0.75F), 0.85F));
    }

    constexpr int32_t flameCount = 30;
    for (int32_t i = 0; i < flameCount; i++)
    {
        const float flameSeed = seedF * 5.1F + static_cast<float>(i) * 17.0F;
        const float angle = hashNoise(flameSeed) * 2.0F * std::numbers::pi_v<float>;
        const float dist = hashNoise(flameSeed + 1.7F) * radius * 0.92F;
        const Vector2 base =
            Vector2Add(center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        const float bob = 0.5F + 0.5F * std::sin(t * 8.0F + flameSeed);
        const Vector2 tip{.x = base.x + std::sin(t * 11.0F + flameSeed) * 3.0F,
                          .y = base.y - (5.0F + bob * 9.0F)};
        const Color flameColor =
            bob > 0.6F ? Palette::ElementBurn : ColorLerp(Palette::ElementBurn, YELLOW, 0.5F);
        DrawTriangle(Vector2{.x = base.x - 3, .y = base.y}, tip,
                     Vector2{.x = base.x + 3, .y = base.y}, Fade(flameColor, 0.8F));
    }
}

void drawPanelShape(Vector2 center, float radius, int32_t sides, float rotationDeg, Color baseColor,
                    float seed)
{
    if (sides <= 2)
    {
        DrawCircleV(center, radius, baseColor);
        DrawCircleLines(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y), radius,
                        Fade(Palette::Void, 0.6F));
    }
    else
    {
        DrawPoly(center, sides, radius, rotationDeg, baseColor);
        DrawPolyLines(center, sides, radius, rotationDeg, Fade(Palette::Void, 0.6F));
    }

    constexpr int32_t seamCount = 5;
    for (int32_t i = 0; i < seamCount; i++)
    {
        const float seamSeed = seed * 11.0F + static_cast<float>(i) * 17.0F;
        const float angle = hashNoise(seamSeed) * 2.0F * std::numbers::pi_v<float>;
        const float len = radius * (0.5F + hashNoise(seamSeed + 3.0F) * 0.5F);
        const float offsetDist = hashNoise(seamSeed + 6.0F) * radius * 0.6F;
        const Vector2 base =
            Vector2Add(center, Vector2{.x = std::cos(angle + 1.5708F) * offsetDist,
                                       .y = std::sin(angle + 1.5708F) * offsetDist});
        const Vector2 end =
            Vector2Add(base, Vector2{.x = std::cos(angle) * len, .y = std::sin(angle) * len});
        DrawLineEx(base, end, 2.0F, Fade(Palette::Void, 0.5F));
    }

    constexpr int32_t rivetCount = 3;
    for (int32_t i = 0; i < rivetCount; i++)
    {
        const float rivetSeed = seed * 19.0F + static_cast<float>(i) * 29.0F;
        const float angle = hashNoise(rivetSeed) * 2.0F * std::numbers::pi_v<float>;
        const float dist = hashNoise(rivetSeed + 1.0F) * radius * 0.6F;
        const Vector2 pos =
            Vector2Add(center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        DrawRectangle(static_cast<int32_t>(pos.x - 2), static_cast<int32_t>(pos.y - 3), 4, 6,
                      Fade(Palette::Void, 0.7F));
    }
}

void drawPanelHex(Vector2 center, float radius, float rotationDeg, Color baseColor, float seed)
{
    drawPanelShape(center, radius, 6, rotationDeg, baseColor, seed);
}


// A goo tube between two Wreckworm parts: overlapping beads that pinch to a neck mid-span.
void drawWreckwormGooBridge(Vector2 from, Vector2 to, float fromRadius, float toRadius, float seed)
{
    const float dist = Vector2Distance(from, to);
    if (dist < 1.0F || dist > (fromRadius + toRadius) * 1.8F)
    {
        return; // shed or torn apart: no strand
    }
    const Vector2 dir = Vector2Scale(Vector2Subtract(to, from), 1.0F / dist);
    const Vector2 perp{.x = -dir.y, .y = dir.x};
    const auto t = static_cast<float>(GetTime());
    const Vector2 start = Vector2Add(from, Vector2Scale(dir, fromRadius * 0.5F));
    const Vector2 end = Vector2Subtract(to, Vector2Scale(dir, toRadius * 0.5F));
    constexpr int32_t beads = 9;
    for (int32_t pass = 0; pass < 2; pass++)
    {
        for (int32_t i = 0; i <= beads; i++)
        {
            const float s = static_cast<float>(i) / beads;
            const float pinch = 1.0F - 0.3F * std::sin(s * std::numbers::pi_v<float>);
            const float width = std::lerp(fromRadius, toRadius, s) * 0.5F * pinch;
            const float sag = std::sin(s * std::numbers::pi_v<float>) * std::sin(t * 2.6F + seed) *
                              width * 0.25F;
            const Vector2 pos = Vector2Add(Vector2Lerp(start, end, s), Vector2Scale(perp, sag));
            if (pass == 0)
            {
                DrawCircleV(pos, width, wreckwormSlime);
            }
            else
            {
                DrawCircleV(Vector2Add(pos, Vector2{.x = -width * 0.35F, .y = -width * 0.35F}),
                            width * 0.28F, Fade(WHITE, 0.12F));
            }
        }
    }
}

// A slime drop oozing off a rim point: it stretches out on a thread, then falls away.
void drawSlimeDrip(Vector2 rim, Vector2 outward, float size, float phase, Color color)
{
    const float p = phase - std::floor(phase);
    const Vector2 drop = Vector2Add(rim, Vector2Scale(outward, size * (0.6F + p * 3.2F)));
    const float alpha = p < 0.8F ? 1.0F : (1.0F - p) / 0.2F;
    if (p < 0.65F)
    {
        DrawLineEx(rim, drop, size * 0.35F, Fade(color, alpha));
    }
    DrawCircleV(drop, size * (1.0F - p * 0.35F), Fade(color, alpha));
    DrawCircleV(Vector2Add(drop, Vector2{.x = -size * 0.3F, .y = -size * 0.3F}), size * 0.3F,
                Fade(WHITE, 0.3F * alpha));
}

// A gun that pushes out through a body segment's goo, aimed at the player. ext 0..1.
void drawWreckwormSegmentGun(Vector2 center, float radius, Vector2 aim, float ext, bool flash)
{
    const float e = 1.0F - (1.0F - ext) * (1.0F - ext) * (1.0F - ext);
    const Vector2 tip = Vector2Add(center, Vector2Scale(aim, radius * wreckwormGunLength * e));
    const float aimDeg = std::atan2(aim.y, aim.x) * RAD2DEG;
    DrawCircleV(center, radius * 0.5F * e, ColorLerp(wreckwormSlime, Palette::RustbloomVoid, 0.5F));
    DrawLineEx(center, tip, radius * 0.42F, Palette::StructDark);
    DrawLineEx(center, tip, radius * 0.3F, Palette::Haze);
    DrawLineEx(center, tip, radius * 0.1F, Palette::StructDark);
    const Vector2 band = Vector2Subtract(tip, Vector2Scale(aim, radius * 0.22F * e));
    DrawLineEx(band, tip, radius * 0.48F, Palette::StructDark);
    DrawLineEx(band, tip, radius * 0.38F, Palette::StructLight);
    drawPanelShape(center, radius * 0.42F * e, 6, aimDeg, Palette::StructMid, radius);
    DrawRing(center, radius * 0.4F * e, radius * 0.55F * e, 0, 360, 24,
             Fade(wreckwormSlime, 0.9F));
    // Charging: the muzzle glows as the gun comes out.
    DrawCircleV(tip, radius * 0.14F * e, Fade(Palette::RustbloomAccent, 0.4F + 0.5F * e));
    if (flash)
    {
        DrawCircleV(tip, radius * 0.26F, Fade(Palette::Crit, 0.85F));
        DrawCircleV(tip, radius * 0.13F, WHITE);
    }
}

// A curved chitin claw from base to tip, bowing toward `bow`, tapering to a point.
void drawChitinClaw(Vector2 base, Vector2 tip, Vector2 bow, float width, Color color)
{
    constexpr int32_t steps = 6;
    const Vector2 mid = Vector2Add(Vector2Lerp(base, tip, 0.5F), bow);
    Vector2 prev = base;
    for (int32_t i = 1; i <= steps; i++)
    {
        const float s = static_cast<float>(i) / steps;
        const float u = 1.0F - s;
        const Vector2 p = Vector2Add(Vector2Add(Vector2Scale(base, u * u), Vector2Scale(mid, 2.0F * u * s)),
                                     Vector2Scale(tip, s * s));
        const float w = width * (1.0F - s * 0.85F);
        DrawLineEx(prev, p, w, color);
        DrawCircleV(p, w / 2, color);
        prev = p;
    }
    DrawLineEx(Vector2Lerp(base, mid, 0.3F), Vector2Lerp(mid, tip, 0.6F), width * 0.18F,
               Fade(Palette::Haze, 0.35F));
}

void drawWreckwormSegmentShape(Vector2 center, float radius, float seed, Color baseColor,
                               bool armor, float spinDeg, Vector2 spineDir)
{
    const auto t = static_cast<float>(GetTime());
    if (armor && Vector2Length(spineDir) > 0.01F)
    {
        // Lateral spines swept back from the travel direction, spinning with the body.
        const Vector2 dir = Vector2Rotate(spineDir, spinDeg * DEG2RAD);
        const Vector2 perp{.x = -dir.y, .y = dir.x};
        for (const float side : {1.0F, -1.0F})
        {
            const Vector2 base = Vector2Add(center, Vector2Scale(perp, side * radius * 0.7F));
            const Vector2 tip = Vector2Add(
                base, Vector2Add(Vector2Scale(perp, side * radius * 0.55F), Vector2Scale(dir, -radius * 0.35F)));
            drawChitinClaw(base, tip, Vector2Scale(dir, radius * 0.12F), radius * 0.3F,
                           Palette::StructDark);
        }
    }
    drawPanelHex(center, radius, seed * 40.0F + spinDeg, baseColor, seed);
    if (armor)
    {
        // A glowing seam where the plate has split.
        const float glow = 0.4F + 0.3F * std::sin(t * 2.5F + seed);
        const float a = hashNoise(seed * 4.1F) * 2.0F * std::numbers::pi_v<float> + spinDeg * DEG2RAD;
        const Vector2 c0 = Vector2Add(center, Vector2{.x = std::cos(a) * radius * 0.7F, .y = std::sin(a) * radius * 0.7F});
        const Vector2 c1 = Vector2Add(center, Vector2{.x = std::cos(a + 2.4F) * radius * 0.15F,
                                                      .y = std::sin(a + 2.4F) * radius * 0.15F});
        const Vector2 c2 = Vector2Add(center, Vector2{.x = std::cos(a + 3.3F) * radius * 0.6F,
                                                      .y = std::sin(a + 3.3F) * radius * 0.6F});
        DrawLineEx(c0, c1, radius * 0.06F, Fade(Palette::RustbloomAccent, glow));
        DrawLineEx(c1, c2, radius * 0.05F, Fade(Palette::RustbloomAccent, glow));
    }
    if (spinDeg != 0)
    {
        // Spinning fast: two faint motion arcs ride the rim.
        for (int32_t i = 0; i < 2; i++)
        {
            const float a = spinDeg + static_cast<float>(i) * 180.0F;
            DrawRing(center, radius * 1.04F, radius * 1.14F, a, a + 80.0F, 16,
                     Fade(Palette::Haze, 0.35F));
        }
    }

    // Goo slicked over one flank, with a drip.
    const float gooAngle = hashNoise(seed * 2.7F) * 2.0F * std::numbers::pi_v<float> +
                           spinDeg * DEG2RAD;
    const Vector2 gooDir{.x = std::cos(gooAngle), .y = std::sin(gooAngle)};
    const float gooScale = armor ? 0.6F : 1.0F;
    for (int32_t i = 0; i < 3; i++)
    {
        const float k = static_cast<float>(i);
        const Vector2 pos = Vector2Add(
            center, Vector2Scale(Vector2Rotate(gooDir, (k - 1.0F) * 0.6F), radius * (0.55F - k * 0.08F)));
        DrawCircleV(pos, radius * (0.32F - k * 0.05F) * gooScale, wreckwormSlime);
    }
    DrawCircleV(Vector2Add(center, Vector2Scale(gooDir, radius * 0.5F)), radius * 0.08F * gooScale,
                Fade(WHITE, 0.3F));
    drawSlimeDrip(Vector2Add(center, Vector2Scale(gooDir, radius * 0.82F)), gooDir,
                  radius * 0.1F * gooScale, t * 0.3F + seed * 0.37F, wreckwormSlime);
    if (armor)
    {
        return;
    }
    constexpr int32_t pustuleCount = 2;
    for (int32_t i = 0; i < pustuleCount; i++)
    {
        const float pustuleSeed = seed * 6.3F + static_cast<float>(i) * 15.0F;
        const float angle = hashNoise(pustuleSeed) * 2.0F * std::numbers::pi_v<float>;
        const float dist = hashNoise(pustuleSeed + 1.0F) * radius * 0.5F;
        const Vector2 pos =
            Vector2Add(center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        const float pulse = 0.5F + 0.5F * std::sin(t * 3.0F + pustuleSeed);
        DrawCircleV(pos, radius * 0.18F, Fade(Palette::Heal, 0.35F + 0.35F * pulse));
    }
}

// Dazed: stars circling overhead. Read: a pulsing ring when the boss picks a counter-pattern.
void drawBossDirectorTells(const Boss& boss, Vector2 center)
{
    const float r = std::max(boss.size.x, boss.size.y) / 2;
    const auto t = static_cast<float>(GetTime());
    if (boss.dazedTimer > 0)
    {
        for (int i = 0; i < 3; i++)
        {
            const float a = t * 4.0F + static_cast<float>(i) * 2.094F;
            DrawCircleV(Vector2{.x = center.x + std::cos(a) * r * 0.6F,
                                .y = center.y - r * 0.9F + std::sin(a) * r * 0.2F},
                        r * 0.08F, Palette::Accent);
        }
    }
    if (boss.readTellTimer > 0)
    {
        const float pulse = 1.0F + 0.15F * std::sin(t * 20.0F);
        DrawRing(center, r * 1.15F * pulse, r * 1.25F * pulse, 0, 360, 48,
                 Fade(Palette::Crit, std::min(1.0F, boss.readTellTimer * 2.0F)));
    }
}

void drawBoss(const Game& game, const Boss& boss)
{
    const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                             .y = boss.position.y + boss.size.y / 2};
    Color ufoColor = boss.health <= 0 ? Palette::StructDark : boss.color;
    if (boss.hitFlashTimer > 0)
    {
        ufoColor =
            ColorLerp(ufoColor, WHITE, boss.hitFlashTimer / UpdateConstants::hitFlashDuration);
    }
    if (boss.burnDps > 0)
    {
        const float flicker = 0.6F + 0.4F * std::sin(static_cast<float>(GetTime()) * 10.0F);
        ufoColor = ColorLerp(ufoColor, Palette::ElementBurn, 0.5F * flicker);
    }
    if (boss.debuffFreeze)
    {
        ufoColor = ColorLerp(ufoColor, Palette::ElementFreeze, 0.35F);
    }
    if (boss.debuffStatic)
    {
        const float flicker = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 18.0F);
        ufoColor = ColorLerp(ufoColor, Palette::ElementStatic, 0.4F * flicker);
    }
    if (boss.phaseInvulnTimer > 0)
    {
        const float flicker = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 25.0F);
        ufoColor = ColorLerp(ufoColor, WHITE, 0.7F * flicker);
    }

    if (boss.isWreckwormHead && !boss.krakenSnakeVariant && boss.state == BossState::WINDING_UP &&
        boss.attack == BossAttack::Barrage)
    {
        // Spore Spit tell: the head flares pale green.
        const float flare = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 24.0F);
        ufoColor = ColorLerp(ufoColor, ColorLerp(Palette::RustbloomAccent, WHITE, 0.5F), flare);
    }

    const float deathElapsed = bossDeathAnimDuration - boss.deathAnimTimer;
    if (boss.deathAnimTimer >= 0 && deathElapsed < bossDeathBlinkDuration)
    {
        const float blink = std::sin(static_cast<float>(GetTime()) * 30.0F) > 0 ? 1.0F : 0.0F;
        ufoColor = ColorLerp(ufoColor, WHITE, blink);
    }

    if (boss.isSlagmaw)
    {
        const float flash = boss.hitFlashTimer > 0 ? boss.hitFlashTimer / UpdateConstants::hitFlashDuration : 0.0F;
        drawLavaMole(game, boss, bossCenter, flash);
    }
    else if (boss.isWreckwormHead && boss.krakenSnakeVariant)
    {
        drawMeteorBossShape(bossCenter, boss.size.x / 2, boss.instanceId, ufoColor);
    }
    else if (boss.isWreckwormHead)
    {
        // The mouth gapes and turns on the player for Spore Spit, half-opens while coiling.
        const bool spitting = boss.attack == BossAttack::Barrage && boss.state != BossState::IDLE;
        const bool coiling = boss.attack == BossAttack::CoilClamp && boss.state != BossState::IDLE;
        const Vector2 toPlayer = Vector2Subtract(game.run.player.position, bossCenter);
        const Vector2 facing = spitting || Vector2Length(boss.wreckwormVelocity) < 1.0F
                                   ? toPlayer
                                   : boss.wreckwormVelocity;
        const float idleMouth = 0.2F + 0.08F * std::sin(static_cast<float>(GetTime()) * 2.0F);
        const Color slime =
            ColorLerp(ColorLerp(Palette::RustbloomAccent, Palette::RustbloomVoid, 0.15F), ufoColor,
                      boss.color.r == boss.baseColor.r && boss.color.g == boss.baseColor.g ? 0.0F
                                                                                          : 0.25F);
        const float flash = std::max(boss.hitFlashTimer / UpdateConstants::hitFlashDuration, 0.0F);
        drawWreckwormHeadShape(bossCenter, boss.size.x / 2, static_cast<float>(boss.instanceId),
                               ColorLerp(slime, WHITE, 0.3F * std::min(flash, 1.0F)),
                               boss.wreckwormVelocity, facing,
                               spitting ? 1.0F : coiling ? 0.55F : idleMouth);
    }
    else if (boss.isWreckwormSegment && boss.krakenSnakeVariant)
    {
        drawBossHull(BossShape::HexPlated, bossCenter, boss.size, ufoColor);
    }
    else if (boss.isWreckwormSegment)
    {
        // Goo strand to the part ahead (the previous segment, or the head for the first).
        const Boss* ahead = nullptr;
        const Boss* head = nullptr;
        for (const auto& other : game.run.bosses)
        {
            if (other.health <= 0)
            {
                continue;
            }
            if (other.isWreckwormHead && other.instanceId == boss.segmentOwnerId)
            {
                head = &other;
            }
            else if (other.isWreckwormSegment && other.segmentOwnerId == boss.segmentOwnerId &&
                     other.segmentIndex == boss.segmentIndex - 1)
            {
                ahead = &other;
            }
        }
        if (boss.segmentIndex == 0)
        {
            ahead = head;
        }
        Vector2 spine{};
        if (ahead != nullptr && boss.health > 0)
        {
            const Vector2 aheadCenter{.x = ahead->position.x + ahead->size.x / 2,
                                      .y = ahead->position.y + ahead->size.y / 2};
            spine = Vector2Normalize(Vector2Subtract(aheadCenter, bossCenter));
            drawWreckwormGooBridge(bossCenter, aheadCenter, boss.size.x / 2,
                                   std::min(ahead->size.x / 2, boss.size.x * 0.8F),
                                   static_cast<float>(boss.segmentIndex));
        }
        // Bind: the whole body spins up while the coil closes.
        const bool binding = head != nullptr && head->attack == BossAttack::CoilClamp &&
                             head->state != BossState::IDLE;
        const float spinDir = boss.segmentIndex % 2 == 0 ? 1.0F : -1.0F;
        drawWreckwormSegmentShape(bossCenter, boss.size.x / 2,
                                  static_cast<float>(boss.segmentIndex + 1), ufoColor,
                                  boss.isArmorSegment,
                                  binding ? static_cast<float>(GetTime()) * 900.0F * spinDir : 0.0F,
                                  spine);
        if (!boss.isArmorSegment && boss.health > 0)
        {
            const float lead = std::clamp(1.0F - boss.segmentVolleyTimer / wreckwormGunPopLead, 0.0F, 1.0F);
            const float ext = std::max(lead, boss.gunPopTimer / wreckwormGunRetractDuration);
            if (ext > 0.01F)
            {
                const Vector2 aim =
                    Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));
                drawWreckwormSegmentGun(bossCenter, boss.size.x / 2, aim, ext,
                                        boss.gunPopTimer > wreckwormGunRetractDuration - 0.08F);
            }
        }
    }
    else if (boss.isKraken && boss.krakenEncounter >= 3 && game.run.bossCutscenePhase == 4)
    {
        const float progress =
            std::clamp(1.0F - game.run.bossCutsceneTimer / krakenOutroTwirlDuration, 0.0F, 1.0F);
        const float shrink = std::max(0.04F, 1.0F - progress);
        const Color shadowColor = ColorLerp(ufoColor, BLACK, progress);
        const float twirlSeed = static_cast<float>(boss.instanceId) + progress * 40.0F +
                                static_cast<float>(GetTime()) * 4.0F;
        drawKrakenShape(bossCenter, boss.size.x / 2 * shrink, twirlSeed, shadowColor,
                        Vector2Rotate(Vector2{.x = 0, .y = 1}, twirlSeed));
    }
    else if (boss.isKraken)
    {
        drawKrakenShape(bossCenter, boss.size.x / 2, static_cast<float>(boss.instanceId), ufoColor,
                        Vector2Subtract(game.run.player.position, bossCenter));
    }
    else if (boss.isBanished)
    {
        drawBanishedShape(bossCenter, boss);
    }
    else
    {
        drawBossHull(boss.shape, bossCenter, boss.size, ufoColor);
    }
    drawElementalDebuffEffects(bossCenter, std::max(boss.size.x, boss.size.y) / 2, boss.burnDps > 0,
                               boss.debuffStatic, boss.debuffFreeze);
    drawBossDirectorTells(boss, bossCenter);
    if (boss.pattern == BossPattern::StrafeRun && boss.patternStep == 2)
    {
        // Telegraph the run lines (the start can sit near the screen edge).
        DrawLineEx(boss.runStart, boss.runEnd, 6, Fade(Palette::Crit, 0.35F));
        if (boss.phase >= 1)
        {
            DrawLineEx(Vector2Add(boss.runStart, boss.runMirrorShift),
                       Vector2Add(boss.runEnd, boss.runMirrorShift), 6, Fade(Palette::Crit, 0.35F));
        }
    }
    if (boss.decoyActive)
    {
        // P2 Hunter decoy: a flickering ghost of the hull on the mirror run.
        const float flicker = 0.5F + 0.15F * std::sin(static_cast<float>(GetTime()) * 22.0F);
        drawBossHull(boss.shape, boss.decoyCenter, boss.size, Fade(boss.color, flicker));
    }

    if (boss.isKraken)
    {
        for (int32_t slot = 0; slot < Boss::krakenLimbSlots; slot++)
        {
            if (boss.tentaclePhase.at(static_cast<size_t>(slot)) != 0)
            {
                drawKrakenTentacle(boss, slot);
            }
            if (boss.tentacleStaggered.at(static_cast<size_t>(slot)))
            {
                drawKrakenStaggeredTentacle(boss, slot);
            }
            drawKrakenLimb(boss, slot);
        }
        drawKrakenSnatch(boss);
    }
    if (boss.isBanished)
    {
        for (int32_t slot = 0; slot < Boss::banishedTentacleCount; slot++)
        {
            drawBanishedTentacle(bossCenter, boss, slot);
        }
        // The eye also snaps open for a clean Juggled's finisher.
        if (boss.banishedStage == 1 || boss.banishedStage == 2 ||
            (game.run.qteBossId == boss.instanceId && game.run.qteFinisherPhase > 0))
        {
            drawBanishedEye(boss);
        }
    }

    if (boss.deathAnimTimer >= 0)
    {
        const float radius = std::max(boss.size.x, boss.size.y) / 2;
        if (deathElapsed >= bossDeathBlinkDuration &&
            deathElapsed < bossDeathBlinkDuration + bossDeathCrackDuration)
        {
            const float crackProgress =
                (deathElapsed - bossDeathBlinkDuration) / bossDeathCrackDuration;
            constexpr int32_t maxCracks = 8;
            const auto crackCount = static_cast<int32_t>(crackProgress * maxCracks);
            for (int32_t i = 0; i < crackCount; i++)
            {
                const float seed =
                    static_cast<float>(boss.instanceId) * 13.0F + static_cast<float>(i) * 7.0F;
                const float angle = hashNoise(seed) * 2.0F * std::numbers::pi_v<float>;
                const Vector2 start =
                    Vector2Add(bossCenter, Vector2{.x = std::cos(angle) * radius * 0.15F,
                                                   .y = std::sin(angle) * radius * 0.15F});
                const Vector2 end =
                    Vector2Add(bossCenter, Vector2{.x = std::cos(angle) * radius * 0.95F,
                                                   .y = std::sin(angle) * radius * 0.95F});
                DrawLineEx(start, end, 2.5F, Fade(Palette::Void, 0.75F));
            }
        }
        else if (deathElapsed >= bossDeathBlinkDuration + bossDeathCrackDuration)
        {
            const float gatherElapsed =
                deathElapsed - bossDeathBlinkDuration - bossDeathCrackDuration;
            const float gatherProgress =
                std::clamp(gatherElapsed / bossDeathGatherDuration, 0.0F, 1.0F);
            constexpr int32_t particleCount = 14;
            for (int32_t i = 0; i < particleCount; i++)
            {
                const float seed =
                    static_cast<float>(boss.instanceId) * 17.0F + static_cast<float>(i) * 11.0F;
                const float angle = hashNoise(seed) * 2.0F * std::numbers::pi_v<float>;
                const float startDist = radius * (1.3F + hashNoise(seed + 1.0F) * 0.6F);
                const float dist = startDist * (1.0F - gatherProgress);
                const Vector2 pos = Vector2Add(
                    bossCenter, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
                DrawCircleV(pos, 4.0F, Fade(WHITE, 0.7F));
            }
            DrawCircleV(bossCenter, radius * (0.3F + gatherProgress * 0.5F),
                        Fade(WHITE, gatherProgress * 0.6F));
        }
    }

    if (boss.isBeltbreakerPlate && boss.plateAttached)
    {
        DrawCircleLinesV(bossCenter, boss.size.x * 0.75F, ColorAlpha(Palette::Shield, 0.6F));
    }

    if (boss.isBeltbreaker && !boss.beltbreakerShielded)
    {

        const float ringRadius = boss.size.x * 0.65F;
        DrawRing(bossCenter, ringRadius - 3, ringRadius + 3, -90.0F,
                 -90.0F + 360.0F * std::clamp(boss.shieldGenProgress, 0.0F, 1.0F), 48,
                 Palette::Shield);
    }

    if ((boss.isBeltbreaker && boss.beltbreakerPunishTimer > 0) ||
        (boss.isWreckwormHead && boss.wreckwormPunishTimer > 0) ||
        (boss.isSlagmaw && boss.slagmawPunishTimer > 0) || boss.qteStunTimer > 0)
    {
        // Punish Window: pulsing fragments ring the open core (Beltbreaker) or stunned head.
        const float pulse = 0.6F + 0.4F * std::sin(static_cast<float>(GetTime()) * 14.0F);
        const float ringRadius = boss.size.x * 0.62F;
        for (int32_t i = 0; i < 10; i++)
        {
            const float a = static_cast<float>(i) * 36.0F + static_cast<float>(GetTime()) * 40.0F;
            DrawRing(bossCenter, ringRadius - 3, ringRadius + 3, a, a + 22.0F, 6, Fade(Palette::Crit, pulse));
        }
    }

    if (boss.isBeltbreaker && boss.beltbreakerShielded)
    {

        const float total = UpdateConstants::beltbreakerShieldedDuration(boss.plateCount);
        const float frac = std::clamp(boss.beltbreakerShieldTimer / total, 0.0F, 1.0F);
        const float alpha =
            0.35F + 0.25F * (std::sin(static_cast<float>(GetTime()) * 3.0F) + 1.0F) * 0.5F;
        const float ringRadius = boss.size.x * 0.65F;
        DrawCircleLinesV(bossCenter, ringRadius, ColorAlpha(Palette::Shield, alpha * 0.4F));
        DrawRing(bossCenter, ringRadius - 3, ringRadius + 3, -90.0F, -90.0F + 360.0F * frac, 48,
                 ColorAlpha(Palette::Shield, alpha + 0.3F));
    }

    // Afterimages behind a charging boss, so its speed and line read at a glance.
    if (boss.state == BossState::SHOOTING &&
        (boss.attack == BossAttack::ChargeDash || boss.attack == BossAttack::DashRush ||
         boss.attack == BossAttack::BurrowCharge) &&
        Vector2LengthSqr(boss.chargeVelocity) > 0.01F && !boss.isWreckwormHead && !boss.isWreckwormSegment &&
        !boss.isKraken && !boss.isBanished)
    {
        const Vector2 back = Vector2Normalize(Vector2Negate(boss.chargeVelocity));
        for (int32_t k = 1; k <= 4; k++)
        {
            const float f = static_cast<float>(k);
            const Vector2 ghost = Vector2Add(bossCenter, Vector2Scale(back, f * boss.size.x * 0.45F));
            if (boss.isSlagmaw)
            {
                DrawCircleV(ghost, boss.size.x / 2 * (1.0F - f * 0.12F), Fade(boss.color, 0.22F - f * 0.04F));
            }
            else
            {
                drawBossHull(boss.shape, ghost, Vector2Scale(boss.size, 1.0F - f * 0.1F), Fade(boss.color, 0.22F - f * 0.04F));
            }
        }
    }

    if (boss.state == BossState::WINDING_UP)
    {
        const float total = boss.windupTotal > 0 ? boss.windupTotal : bossWindupDuration(boss.attack);
        const float progress = std::clamp(1 - boss.stateTimer / total, 0.0F, 1.0F);
        const bool locked = isAimLockedAttack(boss.attack) && boss.stateTimer <= aimLockTime;
        drawChargeParticles(bossCenter, progress, boss.color, 100, 65);

        if (boss.isWreckwormHead && !boss.krakenSnakeVariant && boss.attack == BossAttack::Barrage)
        {
            // Spore Spit tell: a pulsing green ring round the flaring head.
            const float pulse = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 24.0F);
            const float r = boss.size.x / 2;
            DrawRing(bossCenter, r * 1.15F, r * (1.25F + 0.1F * pulse), 0, 360, 48,
                     ColorLerp(Palette::RustbloomAccent, WHITE, 0.2F + 0.3F * pulse));
        }

        // Beam and dash tells: a lane along the coming shot. It follows the aim, then freezes for the
        // last aimLockTime, where it turns solid: step out of the lane to dodge.
        const bool beam = boss.attack == BossAttack::Beam || boss.attack == BossAttack::WormholeBeam;
        const bool dash = boss.attack == BossAttack::ChargeDash || boss.attack == BossAttack::DashRush;
        if (beam || dash)
        {
            const Vector2 origin = boss.attack == BossAttack::WormholeBeam && Vector2LengthSqr(boss.wormholeBeamOrigin) > 0
                                       ? boss.wormholeBeamOrigin
                                       : bossCenter;
            const Vector2 dir = Vector2Normalize(Vector2Subtract(boss.targetPosition, origin));
            const Vector2 side{.x = -dir.y, .y = dir.x};
            const float length = beam ? 2000.0F
                                      : chargeDashSpeed * chargeDashDuration *
                                            (boss.isBeltbreakerPlate ? beltbreakerPlateAttackScale : 1.0F);
            const float half = beam ? 10.0F + game.run.player.radius : boss.size.x / 2;
            const Vector2 end = Vector2Add(origin, Vector2Scale(dir, length));
            const float blink = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 18.0F);
            const Color edge = locked ? Fade(Palette::Crit, 0.55F + 0.35F * blink) : Fade(Palette::Crit, 0.12F + 0.3F * progress);
            const Vector2 a1 = Vector2Add(origin, Vector2Scale(side, half));
            const Vector2 a2 = Vector2Subtract(origin, Vector2Scale(side, half));
            const Vector2 b1 = Vector2Add(end, Vector2Scale(side, half));
            const Vector2 b2 = Vector2Subtract(end, Vector2Scale(side, half));
            const Color fill = Fade(Palette::Accent, locked ? 0.16F : 0.05F + 0.06F * progress);
            DrawTriangle(a1, b1, b2, fill); // both windings: raylib culls one
            DrawTriangle(a1, b2, b1, fill);
            DrawTriangle(a1, b2, a2, fill);
            DrawTriangle(a1, a2, b2, fill);
            DrawLineEx(a1, b1, locked ? 2.0F : 1.0F, edge);
            DrawLineEx(a2, b2, locked ? 2.0F : 1.0F, edge);
            if (!locked) // centre line while still aiming
            {
                DrawLineEx(origin, end, 1.0F, Fade(Palette::Crit, 0.1F + 0.25F * progress));
            }
            if (boss.attack == BossAttack::WormholeBeam) // the portal opening
            {
                DrawCircleLinesV(origin, 10.0F + 8.0F * progress, Fade(Palette::Shield, 0.4F + 0.5F * progress));
            }
        }
    }

    if ((boss.isBeltbreakerPlate || boss.isWreckwormSegment) && boss.health > 0)
    {
        const float healthPercentage =
            static_cast<float>(boss.health) / static_cast<float>(boss.maxHealth);
        const float healthBarWidth = boss.size.x * healthPercentage;
        DrawRectangle(static_cast<int32_t>(boss.position.x),
                      static_cast<int32_t>(boss.position.y) - 10, static_cast<int32_t>(boss.size.x),
                      6, Fade(Palette::Haze, 0.25F));
        DrawRectangle(static_cast<int32_t>(boss.position.x),
                      static_cast<int32_t>(boss.position.y) - 10,
                      static_cast<int32_t>(healthBarWidth), 6, Palette::Haze);
        DrawRectangleLines(static_cast<int32_t>(boss.position.x),
                           static_cast<int32_t>(boss.position.y) - 10,
                           static_cast<int32_t>(boss.size.x), 6, Palette::StructDark);
    }

    if (boss.state == BossState::SHOOTING && boss.attack == BossAttack::Beam)
    {
        const Vector2 direction =
            Vector2Normalize(Vector2Subtract(boss.targetPosition, bossCenter));
        const Vector2 beamEnd = Vector2Add(bossCenter, Vector2Scale(direction, 2000));

        for (const auto& [segStart, segEnd] : wormholeBentBeamSegments(game, bossCenter, beamEnd))
        {
            float beamLength = Vector2Distance(segStart, segEnd);
            for (const auto& a : game.run.asteroids)
            {
                if (CheckCollisionCircleLine(a.position, a.radius, segStart, segEnd))
                {
                    if (const float dist = Vector2Distance(segStart, a.position) - a.radius;
                        dist < beamLength)
                    {
                        beamLength = dist;
                    }
                }
            }

            const Vector2 segDir = Vector2Normalize(Vector2Subtract(segEnd, segStart));
            const float rotation = std::atan2(segDir.y, segDir.x) * RAD2DEG;
            const Rectangle beamRec{
                .x = segStart.x, .y = segStart.y, .width = beamLength, .height = 20};
            const Vector2 beamOrigin{.x = 0, .y = beamRec.height / 2};

            DrawRectanglePro(beamRec, beamOrigin, rotation, Fade(Palette::Accent, 0.7F));
        }
    }

    if (boss.state == BossState::SHOOTING && boss.attack == BossAttack::Slam)
    {
        const float progress =
            std::clamp(1 - boss.stateTimer / UpdateConstants::slamDuration, 0.0F, 1.0F);
        DrawCircleLines(static_cast<int32_t>(bossCenter.x), static_cast<int32_t>(bossCenter.y),
                        bossConstants::maxSlamRadius * progress,
                        Fade(Palette::Accent, 1 - progress * 0.5F));
    }

    if (boss.state == BossState::SHOOTING && boss.attack == BossAttack::WormholeBeam)
    {
        const Vector2 direction =
            Vector2Normalize(Vector2Subtract(boss.targetPosition, boss.wormholeBeamOrigin));
        const float rotation = std::atan2(direction.y, direction.x) * RAD2DEG;

        const Rectangle beamRec{.x = boss.wormholeBeamOrigin.x,
                                .y = boss.wormholeBeamOrigin.y,
                                .width = 2000,
                                .height = 20};
        const Vector2 beamOrigin{.x = 0, .y = beamRec.height / 2};
        DrawRectanglePro(beamRec, beamOrigin, rotation, Fade(Palette::Shield, 0.7F));

        DrawCircleV(bossCenter, 14, Fade(Palette::Shield, 0.6F));
        DrawCircleV(boss.wormholeBeamOrigin, 18, Fade(Palette::Shield, 0.5F));
        DrawCircleLinesV(boss.wormholeBeamOrigin, 18, Palette::Haze);
    }

    if (boss.state == BossState::SHOOTING && boss.attack == BossAttack::ShockwaveStomp)
    {
        const float progress =
            std::clamp(1 - boss.stateTimer / UpdateConstants::shockwaveStompDuration, 0.0F, 1.0F);
        const float radius = UpdateConstants::shockwaveStompRadius * progress;
        DrawCircleLines(static_cast<int32_t>(bossCenter.x), static_cast<int32_t>(bossCenter.y),
                        radius, Fade(Palette::Haze, 1 - progress));
        DrawCircleLines(static_cast<int32_t>(bossCenter.x), static_cast<int32_t>(bossCenter.y),
                        radius * 0.8F, Fade(Palette::Accent, (1 - progress) * 0.7F));
    }

    if (boss.isBeltbreaker && boss.state == BossState::SHOOTING && boss.attack == BossAttack::GravityWell)
    {
        // Ring Crush: an amber arc behind each attached plate; missing plates leave gaps.
        const float radius = boss.size.x * 0.75F * boss.plateRadiusMult;
        const float slotWidth = 360.0F / static_cast<float>(boss.plateCount);
        for (const auto& plate : game.run.bosses)
        {
            if (plate.isBeltbreakerPlate && plate.plateOwnerId == boss.instanceId && plate.plateAttached &&
                plate.health > 0)
            {
                const float mid = boss.plateAngleDeg + static_cast<float>(plate.plateSlotIndex) * slotWidth;
                const float half = slotWidth * ringCrushArcFrac / 2;
                DrawRing(bossCenter, radius - 6, radius + 6, mid - half, mid + half, 24,
                         Fade(Palette::ShatteredBeltAccent, 0.7F));
            }
        }
    }

    if (!boss.isBeltbreaker && boss.state == BossState::SHOOTING && boss.attack == BossAttack::GravityWell)
    {
        DrawCircleLinesV(bossCenter, 60, Fade(Palette::StructMid, 0.5F));
        DrawLineEx(game.run.player.position, bossCenter, 2, Fade(Palette::StructLight, 0.4F));

        for (int i = 0; i < 6; i++)
        {
            const float angle = static_cast<float>(GetTime()) * 3 +
                                static_cast<float>(i) * (2 * std::numbers::pi_v<float> / 6);
            const float radius =
                80.0F -
                std::fmod(static_cast<float>(GetTime()) * 40 + static_cast<float>(i) * 13, 80.0F);
            const Vector2 pos = Vector2Add(
                bossCenter, Vector2{.x = std::cos(angle) * radius, .y = std::sin(angle) * radius});
            DrawCircleV(pos, 3, Palette::StructLight);
        }
    }

    if (boss.state == BossState::SHOOTING && boss.attack == BossAttack::ChargeDash)
    {
        const Vector2 trailDir = Vector2Normalize(boss.chargeVelocity);
        for (int i = 1; i <= 4; i++)
        {
            const Vector2 trailPos =
                Vector2Subtract(bossCenter, Vector2Scale(trailDir, static_cast<float>(i) * 18));
            DrawCircleV(trailPos, boss.size.x / 3.5F * (1 - static_cast<float>(i) * 0.15F),
                        Fade(Palette::Crit, 0.3F / static_cast<float>(i)));
        }
    }
}

void drawComet(const BossProjectile& projectile)
{
    const float speed = Vector2Length(projectile.velocity);
    Vector2 tailDir{.x = 0, .y = -1};
    if (speed > 0)
    {
        tailDir = Vector2Scale(projectile.velocity, -1 / speed);
    }

    constexpr int segments = 5;
    for (int i = segments; i >= 1; i--)
    {
        const float frac = static_cast<float>(i) / segments;
        const Vector2 segPos =
            Vector2Add(projectile.position,
                       Vector2Scale(tailDir, projectile.radius * 2 * static_cast<float>(i)));
        DrawCircleV(segPos, projectile.radius * (1 - frac * 0.6F),
                    Fade(Palette::BossHoming, 0.5F * (1 - frac)));
    }

    DrawCircleV(projectile.position, projectile.radius, Fade(Palette::Haze, 0.9F));
    DrawCircleV(projectile.position, projectile.radius * 0.6F, Palette::BossHoming);
}

void drawMeteorProjectile(const BossProjectile& projectile)
{
    drawSlagGob(projectile.position, projectile.radius, projectile.velocity);
}

// Slag gob: a jagged basalt crust round a molten gold core, a short ember streak behind.
void drawSlagGob(Vector2 position, float radius, Vector2 velocity)
{
    constexpr Color gold{.r = 255, .g = 188, .b = 72, .a = 255};
    const float speed = Vector2Length(velocity);
    const Vector2 back = speed > 0.01F ? Vector2Scale(velocity, -1.0F / speed) : Vector2{.x = 0, .y = 1};
    DrawLineEx(position, Vector2Add(position, Vector2Scale(back, radius * 3.2F)), radius * 1.1F,
               Fade(Palette::ElementBurn, 0.25F));
    DrawLineEx(position, Vector2Add(position, Vector2Scale(back, radius * 1.8F)), radius * 0.6F, Fade(gold, 0.35F));
    constexpr int32_t sides = 6;
    const float spin = static_cast<float>(GetTime()) * 3.0F + position.x * 0.05F;
    std::array<Vector2, sides> verts{};
    for (int32_t i = 0; i < sides; i++)
    {
        const float a = spin + static_cast<float>(i) * 2.0F * PI / sides;
        const float r = radius * (0.85F + 0.25F * hashNoise(static_cast<float>(i) * 3.7F + 1.3F));
        verts.at(static_cast<size_t>(i)) = Vector2Add(position, Vector2{.x = std::cos(a) * r, .y = std::sin(a) * r});
    }
    const Color crust = ColorLerp(Palette::SolarForgeAccent, Palette::Void, 0.75F);
    for (int32_t i = 0; i < sides; i++)
    {
        DrawTriangle(position, verts.at(static_cast<size_t>((i + 1) % sides)), verts.at(static_cast<size_t>(i)), crust);
    }
    DrawCircleV(position, radius * 0.55F, gold);
    DrawCircleV(position, radius * 0.25F, ColorLerp(gold, WHITE, 0.6F));
}

// Devour's inhale: a faint gold wedge with streaks flowing down it into the maw.
// Magma Ram pools: a dark crust ring round molten slag, cooling as it fades, bubbling.
void drawMagmaPools(const Game& game)
{
    constexpr Color gold{.r = 255, .g = 188, .b = 72, .a = 255};
    const auto t = static_cast<float>(GetTime());
    for (const auto& pool : game.run.magmaPools)
    {
        const float f = std::clamp(pool.life / magmaPoolLife, 0.0F, 1.0F);
        const float r = magmaPoolRadius;
        DrawCircleV(pool.position, r, Fade(ColorLerp(Palette::SolarForgeAccent, Palette::Void, 0.8F), 0.75F * f));
        DrawCircleV(pool.position, r * 0.8F, Fade(Palette::ElementBurn, 0.55F * f));
        DrawCircleV(pool.position, r * 0.45F, Fade(gold, 0.5F * f));
        for (int32_t k = 0; k < 3; k++)
        {
            const auto kf = static_cast<float>(k);
            const float phase = std::fmod(t * 1.2F + hashNoise(pool.position.x * 0.01F + kf), 1.0F);
            const float a = hashNoise(pool.position.y * 0.01F + kf * 3.0F) * 2.0F * PI;
            const Vector2 bubble = Vector2Add(pool.position, Vector2{.x = std::cos(a) * r * 0.5F, .y = std::sin(a) * r * 0.5F});
            DrawCircleLines(static_cast<int32_t>(bubble.x), static_cast<int32_t>(bubble.y), r * 0.08F * (0.5F + phase),
                            Fade(gold, (1.0F - phase) * 0.8F * f));
        }
    }
}

void drawSporeProjectile(const BossProjectile& projectile)
{
    // Wreckworm Spore Spit: a ribbed hexagonal spore with a ring of spines and a pale nucleus.
    constexpr int32_t spineCount = 8;
    constexpr int32_t trailCount = 3;
    constexpr float spinDegPerSec = 90.0F;
    const Color accent = Palette::RustbloomAccent;
    const Color dark = ColorLerp(accent, Palette::Void, 0.55F);
    const Color pale = ColorLerp(accent, WHITE, 0.5F);
    const float r = projectile.radius * 1.4F;
    const float rot = static_cast<float>(GetTime()) * spinDegPerSec;

    const float speed = Vector2Length(projectile.velocity);
    const Vector2 tailDir =
        speed > 0 ? Vector2Scale(projectile.velocity, -1 / speed) : Vector2{.x = 0, .y = -1};
    for (int32_t i = trailCount; i >= 1; i--)
    {
        const float frac = static_cast<float>(i) / static_cast<float>(trailCount);
        const Vector2 puff =
            Vector2Add(projectile.position, Vector2Scale(tailDir, r * 1.8F * static_cast<float>(i)));
        DrawPoly(puff, 6, r * (0.8F - frac * 0.4F), rot, Fade(accent, 0.3F * (1 - frac * 0.7F)));
    }

    for (int32_t i = 0; i < spineCount; i++)
    {
        const float angle = (rot + static_cast<float>(i) * (360.0F / spineCount)) * DEG2RAD;
        const Vector2 dir{.x = std::cos(angle), .y = std::sin(angle)};
        const float len = i % 2 == 0 ? 1.9F : 1.55F;
        const Vector2 tip = Vector2Add(projectile.position, Vector2Scale(dir, r * len));
        DrawLineEx(Vector2Add(projectile.position, Vector2Scale(dir, r * 0.8F)), tip, 1.5F, dark);
        DrawCircleV(tip, r * 0.18F, pale);
    }

    DrawPoly(projectile.position, 6, r * 1.2F, rot, dark);
    DrawPoly(projectile.position, 6, r * 0.95F, rot + 30, accent);
    DrawPolyLines(projectile.position, 6, r * 0.95F, rot + 30, Fade(pale, 0.7F));
    DrawPoly(projectile.position, 3, r * 0.45F, -rot, pale);
}

void drawKrakenHurledEnemy(const BossProjectile& projectile)
{
    const float speed = Vector2Length(projectile.velocity);
    Vector2 tailDir{.x = 0, .y = -1};
    if (speed > 0)
    {
        tailDir = Vector2Scale(projectile.velocity, -1 / speed);
    }
    const Vector2 perp{.x = -tailDir.y, .y = tailDir.x};

    constexpr int32_t lineCount = 5;
    for (int32_t i = 0; i < lineCount; i++)
    {
        const float side = (i % 2 == 0) ? 1.0F : -1.0F;
        const float offset =
            std::floor(static_cast<float>(i + 1) / 2.0F) * projectile.radius * 0.55F;
        const Vector2 start = Vector2Add(projectile.position, Vector2Scale(perp, offset * side));
        const Vector2 end = Vector2Add(
            start, Vector2Scale(tailDir, projectile.radius * (2.5F + static_cast<float>(i))));
        DrawLineEx(start, end, 2.5F,
                   Fade(Palette::PunctumHaze, 0.5F - static_cast<float>(i) * 0.07F));
    }

    const float angle = std::atan2(-tailDir.y, -tailDir.x);
    drawKrakenGrabbedEnemy(projectile.position, projectile.visualEnemyKind, 1.0F, angle);
}

// Slime head: a glossy goo dome over a maw that faces `facing`, sitting in a lobed puddle that
// oozes drips. Light comes from the upper left. mouthOpen 0..1.
void drawWreckwormHeadShape(Vector2 center, float radius, float seed, Color color, Vector2 velocity,
                            Vector2 facing, float mouthOpen)
{
    const float speed = Vector2Length(velocity);
    const float stretchAmount = std::min(speed / 900.0F, 0.08F); // stays round: no comet look
    const Vector2 dir = speed > 1.0F ? Vector2Normalize(velocity) : Vector2{.x = 0, .y = -1};
    const Vector2 perp{.x = -dir.y, .y = dir.x};
    const Vector2 face =
        Vector2Length(facing) > 0.01F ? Vector2Normalize(facing) : dir;
    const Vector2 facePerp{.x = -face.y, .y = face.x};
    const auto t = static_cast<float>(GetTime());
    constexpr float tau = 2.0F * std::numbers::pi_v<float>;

    // A wobbling goo outline stretched along the travel direction.
    const auto blob = [&](Vector2 c, float r, float stretch, Color col)
    {
        constexpr int32_t vertexCount = 24;
        std::array<Vector2, vertexCount> verts{};
        for (int32_t i = 0; i < vertexCount; i++)
        {
            const float angle = static_cast<float>(i) * tau / static_cast<float>(vertexCount);
            const Vector2 local{.x = std::cos(angle), .y = std::sin(angle)};
            const float jiggle = 0.95F + hashNoise(seed * 3.1F + static_cast<float>(i)) * 0.07F +
                                 0.035F * std::sin(t * 4.0F + static_cast<float>(i) * 1.3F + seed);
            const float along = Vector2DotProduct(local, dir) * (1.0F + stretch);
            const float across = Vector2DotProduct(local, perp) * (1.0F - stretch * 0.5F);
            verts.at(static_cast<size_t>(i)) =
                Vector2Add(c, Vector2Scale(Vector2Add(Vector2Scale(dir, along),
                                                      Vector2Scale(perp, across)),
                                           r * jiggle));
        }
        for (size_t i = 0; i < verts.size(); i++)
        {
            DrawTriangle(c, verts.at((i + 1) % verts.size()), verts.at(i), col);
        }
    };

    const Color dark = ColorLerp(color, Palette::RustbloomVoid, 0.55F);
    const Color shade = ColorLerp(color, Palette::RustbloomVoid, 0.25F);
    const Color lit = ColorLerp(color, WHITE, 0.14F);

    // Chitin crest: three spikes fanned back from the crown, under the goo.
    for (int32_t i = 0; i < 3; i++)
    {
        const float k = static_cast<float>(i) - 1.0F;
        const Vector2 out = Vector2Rotate(Vector2Negate(face), k * 0.55F);
        const Vector2 base = Vector2Add(center, Vector2Scale(out, radius * 0.75F));
        const Vector2 tip = Vector2Add(center, Vector2Scale(out, radius * (1.45F - std::fabs(k) * 0.2F)));
        drawChitinClaw(base, tip, Vector2Scale(Vector2Rotate(out, 1.57F), radius * 0.1F * k),
                       radius * 0.26F, Palette::StructDark);
    }

    // Puddle the head sits in: a dark base with lobes oozing out behind.
    blob(Vector2Subtract(center, Vector2Scale(dir, radius * 0.12F)), radius * 1.03F,
         stretchAmount + 0.1F, dark);
    for (int32_t i = 0; i < 4; i++)
    {
        const float k = static_cast<float>(i);
        const float angle = std::atan2(-dir.y, -dir.x) + (k - 1.5F) * 0.7F;
        const float ooze = 0.5F + 0.5F * std::sin(t * 1.4F + k * 1.9F + seed);
        const Vector2 lobe = Vector2Add(
            center, Vector2Scale(Vector2{.x = std::cos(angle), .y = std::sin(angle)},
                                 radius * (0.98F + 0.1F * ooze)));
        DrawCircleV(lobe, radius * (0.11F + 0.04F * hashNoise(seed + k)), dark);
    }

    // Dome, shaded in layers toward the light.
    blob(center, radius, stretchAmount, shade);
    blob(Vector2Add(center, Vector2{.x = -radius * 0.07F, .y = -radius * 0.08F}), radius * 0.86F,
         stretchAmount, color);
    blob(Vector2Add(center, Vector2{.x = -radius * 0.2F, .y = -radius * 0.22F}), radius * 0.5F,
         stretchAmount * 0.5F, lit);

    // Bubbles suspended in the goo.
    for (int32_t i = 0; i < 4; i++)
    {
        const float k = static_cast<float>(i);
        const float angle = hashNoise(seed * 5.0F + k) * tau + t * 0.15F;
        const float dist = radius * (0.25F + 0.35F * hashNoise(seed * 7.0F + k));
        const Vector2 pos =
            Vector2Add(center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        const float r = radius * (0.04F + 0.04F * hashNoise(seed * 9.0F + k));
        DrawRing(pos, r * 0.7F, r, 0, 360, 16, Fade(lit, 0.5F));
    }

    // The maw on the front face: dark lips, a glowing throat, drool strands across the gap.
    const Vector2 mouth = Vector2Add(center, Vector2Scale(face, radius * 0.45F));
    const float halfWidth = radius * (0.42F + 0.1F * mouthOpen);
    const float halfDepth = radius * (0.1F + 0.22F * mouthOpen);
    const auto mouthPoint = [&](float angle, float scale)
    {
        return Vector2Add(mouth, Vector2Add(Vector2Scale(facePerp, std::cos(angle) * halfWidth * scale),
                                            Vector2Scale(face, std::sin(angle) * halfDepth * scale)));
    };
    constexpr int32_t mouthVerts = 20;
    for (int32_t pass = 0; pass < 3; pass++)
    {
        const float scale = std::array{1.18F, 1.0F, 0.55F}.at(static_cast<size_t>(pass));
        const float pulse = 0.5F + 0.5F * std::sin(t * 6.0F + seed);
        const Color col = pass == 0   ? dark
                          : pass == 1 ? Palette::RustbloomVoid
                                      : Fade(ColorLerp(Palette::RustbloomAccent, Palette::Crit, 0.3F),
                                             (0.25F + 0.45F * pulse) * mouthOpen);
        for (int32_t i = 0; i < mouthVerts; i++)
        {
            const float a0 = static_cast<float>(i) * tau / mouthVerts;
            const float a1 = static_cast<float>(i + 1) * tau / mouthVerts;
            DrawTriangle(mouth, mouthPoint(a1, scale), mouthPoint(a0, scale), col);
            DrawTriangle(mouth, mouthPoint(a0, scale), mouthPoint(a1, scale), col);
        }
    }
    // Mandibles: two pairs of curved claws flanking the maw, spreading as it opens.
    for (const float side : {1.0F, -1.0F})
    {
        for (int32_t i = 0; i < 2; i++)
        {
            const float k = static_cast<float>(i);
            const Vector2 base = Vector2Add(mouthPoint(side > 0 ? 0.0F : std::numbers::pi_v<float>, 1.05F),
                                            Vector2Scale(face, -radius * 0.12F * k));
            const float spread = (0.25F + 0.45F * mouthOpen) * side;
            const Vector2 out = Vector2Add(Vector2Scale(face, 1.0F - k * 0.25F),
                                           Vector2Scale(facePerp, spread + side * k * 0.35F));
            const Vector2 tip = Vector2Add(base, Vector2Scale(Vector2Normalize(out), radius * (0.62F - k * 0.15F)));
            drawChitinClaw(base, tip, Vector2Scale(facePerp, side * radius * 0.16F),
                           radius * (0.16F - k * 0.03F), Palette::StructDark);
        }
    }

    // Upper lip highlight on the dome side.
    for (int32_t i = 0; i < mouthVerts / 2; i++)
    {
        const float a0 = std::numbers::pi_v<float> + static_cast<float>(i) * tau / mouthVerts;
        const float a1 = a0 + tau / mouthVerts;
        DrawLineEx(mouthPoint(a0, 1.22F), mouthPoint(a1, 1.22F), radius * 0.035F, Fade(lit, 0.8F));
    }
    for (int32_t i = 0; i < 3; i++)
    {
        const float x = std::array{-0.55F, 0.05F, 0.5F}.at(static_cast<size_t>(i));
        const float sway = 0.15F * std::sin(t * 3.0F + static_cast<float>(i) * 2.0F + seed);
        const Vector2 top = mouthPoint(std::acos(std::clamp(x, -1.0F, 1.0F)) + std::numbers::pi_v<float>, 1.0F);
        const Vector2 bottom = mouthPoint(std::acos(std::clamp(x + sway, -1.0F, 1.0F)), 1.0F);
        DrawLineEx(top, bottom, radius * (0.025F + 0.02F * (1.0F - mouthOpen)), Fade(lit, 0.55F));
    }

    // Gloss: hot spots where the light catches the dome.
    DrawCircleV(Vector2Add(center, Vector2{.x = -radius * 0.34F, .y = -radius * 0.4F}),
                radius * 0.08F, Fade(WHITE, 0.55F));
    DrawCircleV(Vector2Add(center, Vector2{.x = -radius * 0.2F, .y = -radius * 0.5F}),
                radius * 0.035F, Fade(WHITE, 0.45F));

    // Drips oozing off the rim, plus one from the lower lip.
    for (int32_t i = 0; i < 3; i++)
    {
        const float k = static_cast<float>(i);
        const float angle = std::atan2(dir.y, dir.x) + 1.7F + k * 1.45F;
        const Vector2 out{.x = std::cos(angle), .y = std::sin(angle)};
        drawSlimeDrip(Vector2Add(center, Vector2Scale(out, radius * 0.95F)), out, radius * 0.06F,
                      t * 0.25F + k * 0.37F + seed, shade);
    }
    drawSlimeDrip(mouthPoint(std::numbers::pi_v<float> * 0.5F, 1.15F), face, radius * 0.05F,
                  t * 0.4F + seed, Fade(lit, 0.8F));
}

// The Kraken: small panelled body in a squid's silhouette, facing the player, with eight thin
// tentacles many body-lengths long spread all round it. The mantle is the old panel octagon
// stretched along the facing and trailing away, with two panel fins at its tip; the head is a
// panel hex turned to the ship with two pod eyes and the pulsing gold core.
void drawKrakenShape(Vector2 center, float radius, float seed, Color color, Vector2 facing)
{
    const auto t = static_cast<float>(GetTime());
    for (int32_t i = 0; i < 8; i++)
    {
        const float sway = 0.3F * std::sin(t * 0.6F + static_cast<float>(i) * 1.3F + seed);
        const float angle = static_cast<float>(i) * 45.0F * DEG2RAD + seed * 0.7F + sway;
        const Vector2 dir{.x = std::cos(angle), .y = std::sin(angle)};
        const float length = radius * (6.0F + 1.2F * std::sin(t * 0.9F + static_cast<float>(i) * 2.1F));
        drawBendyTentacle(Vector2Add(center, Vector2Scale(dir, radius * 0.5F)), Vector2Add(center, Vector2Scale(dir, length)),
                          center, radius, seed + static_cast<float>(i) * 3.0F, 0.22F, radius * 0.45F, false,
                          ColorLerp(color, Palette::Void, 0.35F), Palette::ElementFreeze);
    }

    const Vector2 f = Vector2Normalize(facing);
    const Vector2 side{.x = -f.y, .y = f.x};
    const float facingDeg = std::atan2(f.y, f.x) * RAD2DEG;
    const auto at = [&](float along, float across)
    { return Vector2Add(center, Vector2Add(Vector2Scale(f, along * radius), Vector2Scale(side, across * radius))); };
    // A panel shape drawn squashed and turned to the facing: stretch along it, squeeze across.
    const auto stretched = [&](Vector2 c, float r, int32_t sides, float along, float across, Color col, float s)
    {
        rlDrawRenderBatchActive();
        rlPushMatrix();
        rlTranslatef(c.x, c.y, 0);
        rlRotatef(facingDeg, 0, 0, 1);
        rlScalef(along, across, 1);
        drawPanelShape(Vector2{}, r, sides, seed * 12.0F, col, s);
        rlDrawRenderBatchActive();
        rlPopMatrix();
    };
    const Color podColor = ColorLerp(color, Palette::StructDark, 0.25F);

    for (const float sgn : {1.0F, -1.0F})
    {
        const float flap = 0.06F * std::sin(t * 3.0F + seed + sgn);
        drawPanelShape(at(-1.75F, sgn * (0.42F + flap)), radius * 0.34F, 3, facingDeg + 180.0F + sgn * 25.0F, podColor,
                       seed + sgn);
    }
    stretched(at(-0.6F, 0), radius, 8, 1.55F, 0.62F, ColorLerp(color, Palette::Void, 0.15F), seed);
    drawPanelShape(at(0.5F, 0), radius * 0.55F, 6, facingDeg, color, seed + 2.0F);
    for (const float sgn : {1.0F, -1.0F})
    {
        drawPanelShape(at(0.62F, sgn * 0.42F), radius * 0.2F, 6, seed * 30.0F + sgn * 40.0F, podColor, seed + sgn * 5.0F);
    }

    const float pulse = 0.6F + 0.4F * std::sin(t * 2.0F + seed);
    const Vector2 core = at(0.5F, 0);
    DrawCircleV(core, radius * 0.16F, Fade(Palette::PunctumAccent, 0.5F + 0.4F * pulse));
    DrawCircleLines(static_cast<int32_t>(core.x), static_cast<int32_t>(core.y), radius * 0.16F, Fade(WHITE, 0.6F));
}

void drawBendyTentacle(Vector2 anchor, Vector2 tip, Vector2 bodyCenter, float bodyRadius,
                       float seed, float thicknessScale, float waveScale, bool calm, Color color,
                       Color tipColor)
{
    const float length = Vector2Distance(anchor, tip);
    if (length < 4.0F)
    {
        return;
    }

    Vector2 mid = Vector2Lerp(anchor, tip, 0.5F);
    const float midDist = Vector2Distance(mid, bodyCenter);
    const float minMidDist = bodyRadius * 1.25F;
    if (midDist < minMidDist)
    {
        const Vector2 outDir = midDist > 0.01F
                                   ? Vector2Scale(Vector2Subtract(mid, bodyCenter), 1.0F / midDist)
                                   : Vector2{.x = 0, .y = -1};
        mid = Vector2Add(bodyCenter, Vector2Scale(outDir, minMidDist));
    }

    const auto bezier = [&](float tt) -> Vector2
    {
        const float u = 1.0F - tt;
        return Vector2Add(Vector2Add(Vector2Scale(anchor, u * u), Vector2Scale(mid, 2.0F * u * tt)),
                          Vector2Scale(tip, tt * tt));
    };

    const float t = static_cast<float>(GetTime());
    const Vector2 overallDir = Vector2Scale(Vector2Subtract(tip, anchor), 1.0F / length);
    const Vector2 perp{.x = -overallDir.y, .y = overallDir.x};
    constexpr int32_t segments = 9;
    Vector2 prev = anchor;
    for (int32_t i = 1; i <= segments; i++)
    {
        const float frac = static_cast<float>(i) / static_cast<float>(segments);
        const float wave =
            std::sin(t * 5.0F + frac * 7.0F + seed) * waveScale * frac * (calm ? 0.35F : 1.0F);
        const Vector2 point = Vector2Add(bezier(frac), Vector2Scale(perp, wave));
        const float thickness = 26.0F * thicknessScale * (1.0F - frac * 0.5F);
        DrawLineEx(prev, point, thickness, Fade(color, 0.85F));
        DrawCircleV(point, thickness * 0.5F, Fade(color, 0.85F));
        prev = point;
    }
    DrawCircleV(prev, 14.0F * thicknessScale, Fade(tipColor, 0.8F));
}

void drawKrakenStaggeredTentacle(const Boss& boss, int32_t slot)
{
    const Vector2 bodyCenter{.x = boss.position.x + boss.size.x / 2,
                             .y = boss.position.y + boss.size.y / 2};
    const Vector2 gate = krakenPodPosition(boss, slot % 4);
    const Vector2 tip = boss.tentacleStaggerPos[static_cast<size_t>(slot)];
    const auto t = static_cast<float>(GetTime());
    const float flash = 0.5F + 0.5F * std::sin(t * 10.0F);
    const Color hangColor = ColorLerp(Palette::Crit, WHITE, flash * 0.4F);
    drawBendyTentacle(gate, tip, bodyCenter, boss.size.x / 2,
                      static_cast<float>(boss.instanceId) + 7.0F, krakenTentacleThickness, 34.0F, true,
                      hangColor, hangColor);
    DrawCircleLines(static_cast<int32_t>(tip.x), static_cast<int32_t>(tip.y),
                    krakenTentacleHitRadius, Fade(WHITE, 0.5F + flash * 0.3F));
}

void drawKrakenPortal(Vector2 portal, float t, float open)
{
    const float r = krakenTentaclePortalRadius * (0.7F + 0.3F * std::sin(t * 6.0F)) * open;
    if (r < 1.0F)
    {
        return;
    }
    DrawCircleV(portal, r, Fade(Palette::Void, 0.7F));
    DrawCircleLines(static_cast<int32_t>(portal.x), static_cast<int32_t>(portal.y), r, Fade(Palette::PunctumAccent, 0.8F));
    DrawCircleLines(static_cast<int32_t>(portal.x), static_cast<int32_t>(portal.y), r * 0.6F, Fade(Palette::PunctumHaze, 0.6F));
}

// A rope arm: thick at the root tapering to the tip, a darker rim under a lavender body.
void drawKrakenRope(const std::array<Vector2, Boss::krakenRopePoints>& pts, float rootWidth, float tipWidth, float alpha)
{
    const Color body = Fade(Palette::PunctumHaze, alpha);
    const Color rim = Fade(ColorLerp(Palette::PunctumHaze, Palette::Void, 0.55F), alpha);
    const auto width = [&](size_t i)
    { return rootWidth + (tipWidth - rootWidth) * static_cast<float>(i) / static_cast<float>(pts.size() - 1); };
    for (size_t i = 0; i + 1 < pts.size(); i++)
    {
        DrawLineEx(pts.at(i), pts.at(i + 1), width(i) + 2.5F, rim);
    }
    for (size_t i = 0; i + 1 < pts.size(); i++)
    {
        DrawLineEx(pts.at(i), pts.at(i + 1), width(i), body);
        DrawCircleV(pts.at(i + 1), width(i + 1) / 2, body);
    }
}

// Feeding Strike (the tell, the shot and the reel-in) and the grip of Dragged Under.
void drawKrakenSnatch(const Boss& boss)
{
    const auto t = static_cast<float>(GetTime());
    if (boss.gripping)
    {
        if (!boss.gripFromMouth)
        {
            drawKrakenPortal(boss.gripRoot, t);
        }
        drawKrakenRope(boss.ropePos.at(4), 7.0F, 4.0F, 1.0F);
        const Vector2 c = boss.snatchPos;
        for (int32_t k = 0; k < 2; k++)
        {
            // Two coils wound round the ship, turning against each other.
            const float spin = t * (k == 0 ? 220.0F : -160.0F);
            const float radius = 20.0F + static_cast<float>(k) * 6.0F;
            DrawRing(c, radius - 4.0F, radius, spin, spin + 230.0F, 24, Fade(Palette::PunctumHaze, 0.85F));
        }
        const float seed = std::floor(t * 14.0F);
        Vector2 prev = Vector2Add(c, Vector2{.x = 26.0F, .y = 0});
        for (int32_t k = 1; k <= 6; k++)
        {
            const float a = static_cast<float>(k) * 60.0F * DEG2RAD;
            const float jitter = 22.0F + 8.0F * hashNoise(seed + static_cast<float>(k));
            const Vector2 next = Vector2Add(c, Vector2{.x = std::cos(a) * jitter, .y = std::sin(a) * jitter});
            DrawLineEx(prev, next, 1.5F, Fade(Palette::ElementFreeze, 0.8F));
            prev = next;
        }
        return;
    }
    if (boss.snatchPhase == 0)
    {
        return;
    }
    if (boss.snatchPhase == 1)
    {
        // The tell: a thin line to where the clubs will land and the catch ring, both tightening.
        const float progress = 1.0F - std::clamp(boss.snatchStateTimer / krakenFeedTell, 0.0F, 1.0F);
        DrawLineEx(boss.krakenMouth, boss.feedAim, 1.0F, Fade(Palette::ElementFreeze, 0.25F + 0.4F * progress));
        const float ring = krakenFeedCatchRadius * (2.0F - progress);
        DrawRing(boss.feedAim, ring - 1.5F, ring, 0, 360, 32, Fade(Palette::ElementFreeze, 0.3F + 0.5F * progress));
    }
    for (int32_t k = 0; k < 2; k++)
    {
        const auto& rope = boss.ropePos.at(static_cast<size_t>(4 + k));
        drawKrakenRope(rope, 6.0F, 3.0F, 1.0F);
        const float glow = boss.snatchPhase == 1 ? 0.5F + 0.5F * std::sin(t * 24.0F) : 0.6F;
        DrawCircleV(rope.back(), 10.0F, ColorLerp(Palette::PunctumHaze, Palette::Void, 0.55F)); // the club
        DrawCircleV(rope.back(), 5.0F, Fade(Palette::ElementFreeze, glow));
    }
}

void drawKrakenTentacle(const Boss& boss, int32_t slot)
{
    const auto s = static_cast<size_t>(slot);
    const auto t = static_cast<float>(GetTime());
    const int32_t phase = boss.tentaclePhase.at(s);
    const int32_t kind = boss.tentacleKind.at(s);
    const float timer = boss.tentacleStateTimer.at(s);

    if (kind == 2)
    {
        // Spear Rake: dashed tell lines fan out from the face, then the spears snap out in turn.
        const int32_t count = boss.rakeCount.at(s);
        const Vector2 mouth = boss.krakenMouth;
        if (phase == 1)
        {
            const float progress = 1.0F - std::clamp(timer / krakenRakeTell, 0.0F, 1.0F);
            for (int32_t spear = 0; spear < count; spear++)
            {
                const Vector2 dir = krakenRakeDir(boss, slot, spear);
                for (float d = 30.0F; d < krakenTentacleReach; d += 36.0F)
                {
                    DrawLineEx(Vector2Add(mouth, Vector2Scale(dir, d)), Vector2Add(mouth, Vector2Scale(dir, d + 18.0F)), 1.5F,
                               Fade(Palette::ElementFreeze, 0.15F + 0.5F * progress));
                }
            }
            return;
        }
        const float elapsed = phase == 2 ? krakenRakeStrikeDuration(boss, slot) - timer : 0.0F;
        const float shrink = phase == 4 ? std::clamp(timer / krakenTentacleStaggerRetreatDuration, 0.0F, 1.0F) : 1.0F;
        for (int32_t spear = 0; spear < count; spear++)
        {
            const Vector2 dir = krakenRakeDir(boss, slot, spear);
            const float extent = (phase == 2 ? krakenRakeExtent(boss, slot, spear, elapsed) : krakenTentacleReach) * shrink;
            if (extent < 2.0F)
            {
                continue;
            }
            const Vector2 side{.x = -dir.y, .y = dir.x};
            const float jitter = phase == 4 ? std::sin(t * 60.0F + static_cast<float>(spear)) * 4.0F : 0.0F;
            std::array<Vector2, Boss::krakenRopePoints> pts{};
            for (size_t i = 0; i < pts.size(); i++)
            {
                const float f = static_cast<float>(i) / static_cast<float>(pts.size() - 1);
                pts.at(i) = Vector2Add(Vector2Add(mouth, Vector2Scale(dir, extent * f)), Vector2Scale(side, jitter * f));
            }
            drawKrakenRope(pts, 8.0F, 2.0F, 1.0F);
            DrawLineEx(Vector2Add(mouth, Vector2Scale(dir, extent * 0.2F)), pts.back(), 1.0F, Fade(Palette::ElementFreeze, 0.6F));
        }
        return;
    }

    // Arm Lash and Portal Lash.
    const Vector2 root = kind == 1 ? boss.tentaclePortalPos.at(s) : boss.krakenMouth;
    if (kind == 1)
    {
        float open = 1.0F;
        if (phase == 1)
        {
            open = std::clamp((krakenArmTell + krakenPortalOpen - timer) / krakenPortalOpen, 0.0F, 1.0F);
        }
        else if (phase == 3 || phase == 4)
        {
            open = std::clamp(timer / (phase == 3 ? krakenArmRecover : krakenTentacleStaggerRetreatDuration) * 1.5F, 0.0F, 1.0F);
        }
        drawKrakenPortal(root, t, open);
        if (phase == 1 && timer > krakenArmTell)
        {
            return; // the portal is still opening
        }
    }
    const auto& rope = boss.ropePos.at(s);
    if (phase == 1)
    {
        // The tell: a faint arc where the whip will sweep, and a cyan pulse running down the arm.
        const float progress = 1.0F - std::clamp(timer / krakenArmTell, 0.0F, 1.0F);
        constexpr int32_t arcSteps = 16;
        for (int32_t i = 0; i < arcSteps; i += 2)
        {
            const float a0 = krakenLashAngle(boss, slot, static_cast<float>(i) / arcSteps);
            const float a1 = krakenLashAngle(boss, slot, static_cast<float>(i + 1) / arcSteps);
            DrawLineEx(Vector2Add(root, Vector2Scale(Vector2{.x = std::cos(a0), .y = std::sin(a0)}, krakenTentacleReach)),
                       Vector2Add(root, Vector2Scale(Vector2{.x = std::cos(a1), .y = std::sin(a1)}, krakenTentacleReach)),
                       1.5F, Fade(Palette::ElementFreeze, 0.12F + 0.35F * progress));
        }
        drawKrakenRope(rope, 8.0F, 3.0F, 1.0F);
        const auto at = static_cast<size_t>(progress * static_cast<float>(rope.size() - 1));
        DrawCircleV(rope.at(at), 5.0F, Fade(Palette::ElementFreeze, 0.9F));
        return;
    }
    drawKrakenRope(rope, 8.0F, 3.0F, 1.0F);
    if (phase == 2)
    {
        // The crack: a pale streak where the tip just passed.
        const auto& old = boss.ropeOld.at(s);
        for (size_t i = rope.size() / 2; i < rope.size(); i++)
        {
            DrawLineEx(old.at(i), rope.at(i), 2.0F, Fade(WHITE, 0.5F * static_cast<float>(i) / static_cast<float>(rope.size())));
        }
    }
}

void drawKrakenGrabbedEnemy(Vector2 pos, int32_t kindIndex, float scale, float facingAngle)
{
    if (kindIndex < 0 || scale <= 0.01F)
    {
        return;
    }
    const auto& kind = enemyKinds.at(static_cast<size_t>(kindIndex));
    const float radius = std::clamp(kind.radius, 8.0F, 24.0F) * scale;
    DrawCircleV(pos, radius * 1.15F, Fade(Palette::Void, 0.5F));
    DrawPoly(pos, 3, radius, facingAngle * RAD2DEG - 90.0F, Fade(kind.color, 0.95F));
    DrawCircleV(pos, radius * 0.3F, Fade(WHITE, 0.6F));
}

void drawKrakenLimb(const Boss& boss, int32_t slot)
{
    const int32_t pod = boss.limbPod[slot];
    if (pod < 0)
    {
        return;
    }

    const Vector2 center{.x = boss.position.x + boss.size.x / 2,
                         .y = boss.position.y + boss.size.y / 2};
    const float bodyRadius = boss.size.x / 2;
    const float seed = static_cast<float>(boss.instanceId);
    const float podAngle = static_cast<float>(pod) * 90.0F * DEG2RAD + seed;
    const Vector2 podPos =
        Vector2Add(center, Vector2{.x = std::cos(podAngle) * boss.size.x / 2 * 0.78F,
                                   .y = std::sin(podAngle) * boss.size.y / 2 * 0.78F});

    Vector2 tip = podPos;
    float openProgress = 1.0F;
    bool grabbing = false;
    float grabbedEnemyScale = 0.0F;
    if (boss.limbPhase[slot] == 1)
    {
        openProgress = std::clamp(1.0F - boss.limbTimer[slot] / krakenLimbOpenDuration, 0.0F, 1.0F);
        tip = Vector2Lerp(podPos, boss.limbGrabTarget[slot], openProgress);
    }
    else if (boss.limbPhase[slot] == 2)
    {
        if (boss.limbThrowPhase[slot] == 1)
        {
            grabbing = true;
            grabbedEnemyScale = 1.0F;
            const float progress =
                1.0F -
                std::clamp(boss.limbThrowTimer[slot] / krakenLimbLeanBackDuration, 0.0F, 1.0F);
            const Vector2 leanBackTarget = Vector2Add(
                podPos, Vector2Scale(Vector2Subtract(podPos, boss.limbGrabTarget[slot]),
                                     krakenLimbNearReach /
                                         std::max(1.0F, Vector2Distance(
                                                            podPos, boss.limbGrabTarget[slot]))));
            tip = Vector2Lerp(boss.limbPortalPos[slot], leanBackTarget, progress);
        }
        else if (boss.limbThrowPhase[slot] == 2)
        {
            grabbing = true;
            grabbedEnemyScale = 1.0F;
            const float progress =
                1.0F - std::clamp(boss.limbThrowTimer[slot] / krakenLimbThrowDuration, 0.0F, 1.0F);
            const Vector2 leanBackTarget = Vector2Add(
                podPos, Vector2Scale(Vector2Subtract(podPos, boss.limbGrabTarget[slot]),
                                     krakenLimbNearReach /
                                         std::max(1.0F, Vector2Distance(
                                                            podPos, boss.limbGrabTarget[slot]))));
            tip = Vector2Lerp(leanBackTarget, boss.limbGrabTarget[slot], progress);
        }
        else if (boss.limbGrabAnimTimer[slot] > 0)
        {
            grabbing = true;
            tip = boss.limbPortalPos[slot];
            const float elapsed = krakenLimbGrabAnimDuration - boss.limbGrabAnimTimer[slot];
            if (elapsed < krakenLimbGrabAnimDuration / 3.0F)
            {
                tip = Vector2Lerp(
                    podPos, boss.limbPortalPos[slot],
                    std::clamp(elapsed / (krakenLimbGrabAnimDuration / 3.0F), 0.0F, 1.0F));
            }
            else if (elapsed < krakenLimbGrabAnimDuration * 2.0F / 3.0F)
            {
                grabbedEnemyScale = 0.0F;
            }
            else
            {
                grabbedEnemyScale =
                    std::clamp((elapsed - krakenLimbGrabAnimDuration * 2.0F / 3.0F) /
                                   (krakenLimbGrabAnimDuration / 3.0F),
                               0.0F, 1.0F);
            }
        }
        else
        {
            tip = boss.limbGrabTarget[slot];
        }
    }
    else if (boss.limbPhase[slot] == 3)
    {
        openProgress = std::clamp(boss.limbTimer[slot] / krakenLimbRetreatDuration, 0.0F, 1.0F);
        tip = Vector2Lerp(podPos, boss.limbGrabTarget[slot], openProgress);
    }
    else
    {
        return;
    }

    const float pulse = 0.6F + 0.4F * std::sin(static_cast<float>(GetTime()) * 3.0F + seed);
    DrawCircleV(podPos, boss.size.x / 2 * 0.32F * krakenLimbSizeMult * (0.7F + 0.3F * openProgress),
                Fade(Palette::Void, 0.5F + 0.3F * pulse));

    const float length = Vector2Distance(podPos, tip);
    if (length < 4.0F)
    {
        return;
    }

    if (grabbing && boss.limbThrowPhase[slot] == 0)
    {
        const float grabPulse = 0.7F + 0.3F * std::sin(static_cast<float>(GetTime()) * 10.0F);
        DrawCircleV(tip, 30.0F * krakenLimbSizeMult * grabPulse, Fade(Palette::Void, 0.7F));
        DrawCircleLines(static_cast<int32_t>(tip.x), static_cast<int32_t>(tip.y),
                        30.0F * krakenLimbSizeMult * grabPulse, Fade(Palette::PunctumAccent, 0.8F));
    }

    drawBendyTentacle(podPos, tip, center, bodyRadius, seed, krakenTentacleThickness, 24.0F, grabbing,
                      Palette::PunctumHaze, Palette::Crit);

    if (grabbedEnemyScale > 0.0F)
    {
        const Vector2 overallDir = Vector2Scale(Vector2Subtract(tip, podPos), 1.0F / length);
        drawKrakenGrabbedEnemy(tip, boss.limbGrabbedKind[slot], grabbedEnemyScale,
                               std::atan2(overallDir.y, overallDir.x));
    }
}

void drawBanishedShape(Vector2 center, const Boss& boss)
{
    const auto t = static_cast<float>(GetTime());
    const float coreRadius = boss.size.x * 0.22F;

    for (int32_t i = 0; i < 24; i++)
    {
        const float seed =
            static_cast<float>(boss.instanceId) * 5.0F + static_cast<float>(i) * 11.0F;
        const float angle = hashNoise(seed) * 2.0F * std::numbers::pi_v<float>;
        const float wobble = std::sin(t * 1.5F + seed) * 5.0F;
        const Vector2 knot =
            Vector2Add(center, Vector2{.x = std::cos(angle) * (coreRadius * 0.5F + wobble),
                                       .y = std::sin(angle) * (coreRadius * 0.5F + wobble)});
        DrawCircleV(knot, coreRadius * (0.25F + hashNoise(seed * 3.0F) * 0.2F),
                    Fade(Palette::Void, 0.85F));
    }
    DrawCircleV(center, coreRadius * 0.6F, Fade(Palette::PunctumHaze, 0.5F));
}

void drawBanishedPortal(Vector2 position, float progress)
{
    const float radius = 34.0F * std::clamp(progress, 0.0F, 1.0F);
    if (radius < 1.0F)
    {
        return;
    }
    const auto t = static_cast<float>(GetTime());
    DrawCircleV(position, radius, Fade(Palette::Void, 0.35F));
    DrawRing(position, radius - 4, radius, t * -90.0F, t * -90.0F + 300.0F, 24,
             Fade(Palette::Crit, 0.8F));
    DrawCircleLines(static_cast<int32_t>(position.x), static_cast<int32_t>(position.y),
                    static_cast<int32_t>(radius), Fade(Palette::PunctumHaze, 0.6F));
}

void drawBanishedTentacle(Vector2 center, const Boss& boss, int32_t slot)
{
    const int32_t phase = boss.banishedPhase[static_cast<size_t>(slot)];
    const bool staggered = boss.banishedStaggered[static_cast<size_t>(slot)];
    const auto t = static_cast<float>(GetTime());
    const float seed =
        static_cast<float>(boss.instanceId) * 7.0F + static_cast<float>(slot) * 13.0F;

    if (staggered)
    {
        const float angle = static_cast<float>(slot) /
                                static_cast<float>(Boss::banishedTentacleCount) * 2.0F *
                                std::numbers::pi_v<float> +
                            seed;
        const float r = boss.size.x * 0.3F;
        const Vector2 wrapPos =
            Vector2Add(center, Vector2{.x = std::cos(angle) * r, .y = std::sin(angle) * r});
        const float hitFlash = boss.banishedTentacleHitFlash[static_cast<size_t>(slot)];
        const Color color = hitFlash > 0 ? ColorLerp(Palette::Void, WHITE,
                                                     hitFlash / UpdateConstants::hitFlashDuration)
                                         : Palette::Void;
        DrawLineEx(center, wrapPos, 10.0F, Fade(color, 0.7F));
        if (hitFlash > 0)
        {
            DrawCircleLines(static_cast<int32_t>(wrapPos.x), static_cast<int32_t>(wrapPos.y),
                            14.0F * (1.0F - hitFlash / UpdateConstants::hitFlashDuration) + 4.0F,
                            Fade(WHITE, hitFlash / UpdateConstants::hitFlashDuration));
        }
        return;
    }

    if (phase == 0)
    {
        const float angle = static_cast<float>(slot) /
                                static_cast<float>(Boss::banishedTentacleCount) * 2.0F *
                                std::numbers::pi_v<float> +
                            seed + std::sin(t * 0.6F + seed) * 0.15F;
        const float r = boss.size.x * (0.32F + std::sin(t * 1.2F + seed) * 0.04F);
        const Vector2 stub =
            Vector2Add(center, Vector2{.x = std::cos(angle) * r, .y = std::sin(angle) * r});
        DrawLineEx(center, stub, 8.0F, Fade(Palette::PunctumHaze, 0.6F));
        return;
    }

    if (phase == 6)
    {
        const Vector2 tip = boss.banishedTip[static_cast<size_t>(slot)];
        for (int32_t k = 0; k < 3; k++)
        {
            const float a = seed + t * 4.0F + static_cast<float>(k) * 2.1F;
            const Vector2 wrapPos =
                Vector2Add(tip, Vector2{.x = std::cos(a) * 22.0F, .y = std::sin(a) * 22.0F});
            DrawLineEx(center, wrapPos, 9.0F, Fade(Palette::Crit, 0.55F));
        }
        DrawCircleLines(static_cast<int32_t>(tip.x), static_cast<int32_t>(tip.y), 26.0F,
                        Fade(WHITE, 0.5F));
        return;
    }

    const Vector2 anchor = boss.banishedAnchor[static_cast<size_t>(slot)];
    const Vector2 tip = boss.banishedTip[static_cast<size_t>(slot)];

    if (phase == 1)
    {
        const float progress =
            1.0F - boss.banishedTimer[static_cast<size_t>(slot)] / banishedTentacleWindup;
        drawBanishedPortal(anchor, progress);
    }
    else if (phase == 5)
    {
        drawBanishedPortal(anchor, 1.0F);
    }

    if (Vector2Distance(anchor, tip) < 2.0F)
    {
        return;
    }

    drawBendyTentacle(anchor, tip, center, boss.size.x / 2, seed, 0.7F, 12.0F, false,
                      Palette::PunctumHaze, Palette::Crit);
}

void drawBanishedEye(const Boss& boss)
{
    const Vector2 pos = boss.banishedStage == 1 ? boss.banishedEyePos
                                                : Vector2{.x = boss.position.x + boss.size.x / 2,
                                                          .y = boss.position.y + boss.size.y / 2};
    const float chargeFrac =
        boss.banishedStage == 1
            ? std::clamp(boss.banishedEyeTimer / banishedEyeChargeDuration, 0.0F, 1.0F)
            : 1.0F;
    const float radius = boss.size.x * 0.16F * (0.6F + 0.4F * chargeFrac);

    DrawCircleV(pos, radius * 1.4F, Fade(Palette::Crit, 0.2F + 0.3F * chargeFrac));
    DrawCircleV(pos, radius, WHITE);
    const Vector2 mouse = GetMousePosition();
    const Vector2 pupilDir = Vector2Length(Vector2Subtract(mouse, pos)) > 1.0F
                                 ? Vector2Normalize(Vector2Subtract(mouse, pos))
                                 : Vector2{.x = 0, .y = 1};
    DrawCircleV(Vector2Add(pos, Vector2Scale(pupilDir, radius * 0.35F)), radius * 0.45F,
                Palette::Void);

    if (boss.banishedStage == 2)
    {
        DrawLineEx(pos, boss.targetPosition, 14.0F, Fade(Palette::Crit, 0.75F));
        DrawLineEx(pos, boss.targetPosition, 5.0F, Fade(WHITE, 0.9F));
    }
}

void drawFluidBlob(Vector2 center, float radius, Color color, float seed, Vector2 trailDir)
{
    const float dirLen = Vector2Length(trailDir);
    const Vector2 dir =
        dirLen > 0.01F ? Vector2Scale(trailDir, 1.0F / dirLen) : Vector2{.x = 0, .y = -1};
    const Vector2 perp{.x = -dir.y, .y = dir.x};

    DrawCircleV(center, radius, color);

    constexpr int32_t notchCount = 5;
    for (int32_t i = 0; i < notchCount; i++)
    {
        const float frac = static_cast<float>(i) / static_cast<float>(notchCount - 1) - 0.5F;
        const float notchSeed = seed * 9.0F + static_cast<float>(i) * 7.0F;
        const float notchRadius = radius * (0.22F + hashNoise(notchSeed) * 0.3F);
        const Vector2 edge =
            Vector2Add(center, Vector2Add(Vector2Scale(dir, -radius * 0.75F),
                                          Vector2Scale(perp, frac * radius * 1.7F)));
        DrawCircleV(edge, notchRadius, ColorLerp(color, Palette::Void, 0.6F));
    }

    DrawCircleV(Vector2Add(center, Vector2Scale(perp, -radius * 0.3F)), radius * 0.22F,
                Fade(WHITE, 0.55F));
    DrawCircleV(Vector2Add(center, Vector2{.x = radius * 0.15F, .y = -radius * 0.15F}),
                radius * 0.12F, Fade(WHITE, 0.4F));

    constexpr int32_t dripCount = 3;
    for (int32_t i = 0; i < dripCount; i++)
    {
        const float dripSeed = seed * 5.0F + static_cast<float>(i) * 11.0F;
        const float dist =
            radius * (1.3F + static_cast<float>(i) * 0.6F + hashNoise(dripSeed) * 0.3F);
        const float side = (hashNoise(dripSeed + 1.0F) - 0.5F) * radius * 0.8F;
        const Vector2 dripPos =
            Vector2Add(center, Vector2Add(Vector2Scale(dir, -dist), Vector2Scale(perp, side)));
        DrawCircleV(dripPos, radius * (0.14F - static_cast<float>(i) * 0.02F),
                    Fade(color, 0.6F - static_cast<float>(i) * 0.15F));
    }
}

// Solar Forge lava chunk: an angular basalt crust, lit per facet, split by glowing lava seams
// that meet at a molten core, over a faint heat halo, shedding a few embers behind it.
void drawLavaChunk(Vector2 center, float radius, float seed, Vector2 velocity)
{
    const auto t = static_cast<float>(GetTime());
    constexpr int32_t sides = 7;
    const float spin = seed * 3.0F + t * 0.4F;
    std::array<Vector2, sides> verts{};
    for (int32_t i = 0; i < sides; i++)
    {
        const float a = spin + static_cast<float>(i) * 2.0F * PI / sides;
        const float r = radius * (0.8F + 0.25F * hashNoise(seed * 7.0F + static_cast<float>(i)));
        verts.at(static_cast<size_t>(i)) = Vector2Add(center, Vector2{.x = std::cos(a) * r, .y = std::sin(a) * r});
    }

    const float pulse = 0.5F + 0.5F * std::sin(t * 2.2F + seed * 5.0F);
    DrawCircleV(center, radius * 1.2F, Fade(Palette::SolarForgeHaze, 0.12F + 0.06F * pulse));

    // Embers: a few small sparks scattered behind the travel direction, flickering.
    const float speed = Vector2Length(velocity);
    const Vector2 back = speed > 0.01F ? Vector2Scale(velocity, -1.0F / speed) : Vector2{.x = 0, .y = 1};
    const Vector2 side{.x = -back.y, .y = back.x};
    for (int32_t i = 0; i < 4; i++)
    {
        const float k = static_cast<float>(i);
        const float life = std::fmod(t * 1.3F + hashNoise(seed + k) * 4.0F, 1.0F);
        const Vector2 pos = Vector2Add(
            center, Vector2Add(Vector2Scale(back, radius * (1.0F + life * 1.6F)),
                               Vector2Scale(side, (hashNoise(seed * 3.0F + k) - 0.5F) * radius * 1.6F)));
        DrawRectangleV(Vector2Subtract(pos, Vector2{.x = 1.5F, .y = 1.5F}), Vector2{.x = 3, .y = 3},
                       Fade(ColorLerp(Palette::ElementBurn, YELLOW, 0.4F), 1.0F - life));
    }

    // Crust facets, lit from the upper left.
    const Color crust = ColorLerp(Palette::SolarForgeAccent, Palette::Void, 0.78F);
    const Vector2 light = Vector2Normalize(Vector2{.x = -1, .y = -1});
    for (int32_t i = 0; i < sides; i++)
    {
        const Vector2 v0 = verts.at(static_cast<size_t>(i));
        const Vector2 v1 = verts.at(static_cast<size_t>((i + 1) % sides));
        const Vector2 mid = Vector2Normalize(Vector2Subtract(Vector2Lerp(v0, v1, 0.5F), center));
        const float lit = 0.5F + 0.5F * Vector2DotProduct(mid, light);
        const Color facet = ColorLerp(crust, Palette::StructLight, 0.25F * lit);
        DrawTriangle(center, v1, v0, facet);
        DrawTriangle(center, v0, v1, facet);
    }

    // Lava seams from the core out to three vertices, pulsing, with a molten core.
    const Color lava = ColorLerp(Palette::ElementBurn, YELLOW, 0.25F * pulse);
    const Vector2 core = Vector2Add(center, Vector2Scale(Vector2Subtract(verts.at(0), center), 0.15F));
    for (const int32_t i : {0, 2, 5})
    {
        const Vector2 end = Vector2Lerp(core, verts.at(static_cast<size_t>(i)), 0.85F);
        const Vector2 bend = Vector2Add(Vector2Lerp(core, end, 0.5F),
                                        Vector2Scale(side, (hashNoise(seed + static_cast<float>(i)) - 0.5F) * radius * 0.3F));
        DrawLineEx(core, bend, radius * 0.11F, Fade(lava, 0.85F));
        DrawLineEx(bend, end, radius * 0.07F, Fade(lava, 0.7F));
    }
    DrawCircleV(core, radius * (0.16F + 0.04F * pulse), ColorLerp(lava, YELLOW, 0.4F));
}

void drawWormholeMouth(Vector2 position, WormholeFacing facing, float radius, Color rim)
{
    DrawCircleV(position, radius, Fade(rim, 0.25F));
    DrawRing(position, radius - 4, radius, static_cast<float>(GetTime()) * 60,
             static_cast<float>(GetTime()) * 60 + 300, 24, rim);
    DrawCircleLines(static_cast<int32_t>(position.x), static_cast<int32_t>(position.y), radius,
                    Fade(Palette::Haze, 0.6F));

    const Vector2 dir = wormholeFacingVector(facing);
    const Vector2 tip = Vector2Add(position, Vector2Scale(dir, radius + 12));
    const Vector2 perp{.x = -dir.y, .y = dir.x};
    const Vector2 base = Vector2Add(position, Vector2Scale(dir, radius - 2));
    DrawTriangle(tip, Vector2Add(base, Vector2Scale(perp, 6)),
                 Vector2Subtract(base, Vector2Scale(perp, 6)), Palette::Crit);
}

// Lava mole Slagmaw: basalt body sections trailing the head, a collar of molten crystal petals
// behind the jaws (folded back while it moves or dives, flared for attacks, locked open once it
// blooms), and a cracked swell in the crust while it is underground. Erupt and Tunnel Run tells.
void drawLavaMole(const Game& game, const Boss& boss, Vector2 center, float flash)
{
    constexpr Color gold{.r = 255, .g = 188, .b = 72, .a = 255};
    // Iron hide: dark brown-grey metal, steel highlights, molten lines between the plates.
    constexpr Color basalt{.r = 36, .g = 29, .b = 29, .a = 255};
    constexpr Color basaltLit{.r = 104, .g = 92, .b = 88, .a = 255};
    constexpr Color molten{.r = 255, .g = 128, .b = 40, .a = 255};
    const Color ember = Palette::ElementBurn;
    const auto t = static_cast<float>(GetTime());
    const float radius = boss.size.x / 2;
    const float seed = static_cast<float>(boss.instanceId);
    const bool winding = boss.state == BossState::WINDING_UP;
    const bool shooting = boss.state == BossState::SHOOTING;
    const float windup = winding && boss.windupTotal > 0 ? std::clamp(1.0F - boss.stateTimer / boss.windupTotal, 0.0F, 1.0F) : 0.0F;
    const float seen = (1.0F - boss.moleUnder) * boss.ghostFadeAlpha;
    const bool swallowing = game.run.qteBossId == boss.instanceId;
    const Vector2 face = Vector2Length(boss.slagmawFacing) > 0.01F ? Vector2Normalize(boss.slagmawFacing) : Vector2{.x = 0, .y = 1};

    // Tells.
    if (winding && boss.attack == BossAttack::Devour)
    {
        const bool locked = boss.stateTimer <= eruptLockTime;
        const float pulse = 0.5F + 0.5F * std::sin(t * (locked ? 26.0F : 9.0F));
        DrawCircleV(boss.eruptAt, eruptRadius, Fade(locked ? Palette::Accent : gold, 0.07F + 0.1F * windup));
        DrawRing(boss.eruptAt, eruptRadius - 3.0F, eruptRadius, 0, 360, 48,
                 Fade(locked ? Palette::Accent : gold, 0.35F + 0.4F * pulse));
        DrawRing(boss.eruptAt, eruptRadius * (1.0F - windup) - 2.0F, eruptRadius * (1.0F - windup), 0, 360, 48,
                 Fade(gold, 0.5F));
    }
    if ((winding || shooting) && boss.attack == BossAttack::BurrowCharge)
    {
        const Vector2 dir = Vector2Normalize(Vector2Subtract(boss.targetPosition, center));
        const Vector2 from = center;
        const Vector2 to = winding ? Vector2Add(boss.targetPosition, Vector2Scale(dir, tunnelOvershoot))
                                   : Vector2Add(center, Vector2Scale(boss.chargeVelocity, std::max(0.0F, boss.stateTimer)));
        const float a = winding ? 0.15F + 0.45F * windup : 0.4F;
        for (float d = 0; d < Vector2Distance(from, to); d += 34.0F) // a dashed crack line
        {
            const Vector2 p = Vector2Add(from, Vector2Scale(dir, d));
            DrawLineEx(p, Vector2Add(p, Vector2Scale(dir, 18.0F)), 3.0F, Fade(gold, a));
        }
        DrawRing(to, tunnelBreachRadius - 3.0F, tunnelBreachRadius, 0, 360, 40, Fade(gold, a));
    }

    // The swell in the crust while underground.
    if (boss.moleUnder > 0.01F)
    {
        const float u = boss.moleUnder;
        DrawCircleV(center, radius * 0.95F, Fade(ColorLerp(basalt, Palette::Void, 0.3F), 0.8F * u));
        DrawCircleV(center, radius * 0.7F, Fade(basaltLit, 0.35F * u));
        for (int32_t k = 0; k < 7; k++)
        {
            const auto kf = static_cast<float>(k);
            float a = hashNoise(seed + kf * 2.9F) * 2.0F * PI + t * 0.3F;
            Vector2 p = Vector2Add(center, Vector2{.x = std::cos(a) * radius * 0.2F, .y = std::sin(a) * radius * 0.2F});
            for (int32_t seg = 0; seg < 3; seg++)
            {
                a += (hashNoise(seed * 1.7F + kf * 4.0F + static_cast<float>(seg)) - 0.5F) * 0.9F;
                const Vector2 q = Vector2Add(p, Vector2{.x = std::cos(a) * radius * 0.28F, .y = std::sin(a) * radius * 0.28F});
                DrawLineEx(p, q, 3.0F, Fade(gold, (0.5F + 0.3F * std::sin(t * 6.0F + kf)) * u));
                p = q;
            }
        }
        for (int32_t k = 0; k < 10; k++) // dust kicked up
        {
            const auto kf = static_cast<float>(k);
            const float phase = std::fmod(t * 1.3F + hashNoise(kf * 5.1F), 1.0F);
            const float a = hashNoise(seed + kf * 7.3F) * 2.0F * PI;
            const Vector2 p = Vector2Add(center, Vector2Scale(Vector2{.x = std::cos(a), .y = std::sin(a)}, radius * (0.6F + 0.6F * phase)));
            DrawCircleV(p, radius * 0.06F * (1.0F - phase), Fade(basaltLit, 0.6F * (1.0F - phase) * u));
        }
    }
    if (seen <= 0.01F)
    {
        return;
    }

    // How far the petals stand open: flared for attacks, locked open once bloomed.
    const auto paint = [&](Color c, float a) { return Fade(ColorLerp(c, WHITE, 0.2F * flash), a * seen); };
    float open = 0.0F;
    if (winding && (boss.attack == BossAttack::Spread || boss.attack == BossAttack::Barrage))
    {
        open = windup;
    }
    else if (shooting && (boss.attack == BossAttack::Spread || boss.attack == BossAttack::Barrage ||
                          boss.attack == BossAttack::Devour))
    {
        open = 1.0F;
    }
    if (boss.slagmawBloomed)
    {
        open = 0.9F + 0.1F * std::sin(t * 5.0F);
        DrawCircleV(center, radius * 1.5F, paint(Palette::ElementBurn, 0.1F + 0.05F * std::sin(t * 5.0F)));
    }
    open *= 1.0F - boss.moleUnder;

    // The body: one rigid tube tapering back from the head to a drill point, a molten seam down
    // its spine and screw ridges that turn as the drill spins (fast while underground).
    const Vector2 light = Vector2Normalize(Vector2{.x = -1, .y = -1});
    const float glow = std::clamp(0.35F + 0.2F * std::sin(t * 2.0F + seed), 0.0F, 1.0F);
    {
        const Vector2 back = Vector2Negate(face);
        const Vector2 across{.x = -back.y, .y = back.x};
        const float length = radius * 2.4F;
        const auto width = [&](float u) { return radius * 0.92F * std::pow(1.0F - u, 0.7F) + radius * 0.02F; };
        const auto at = [&](float u, float v) // u along 0 head .. 1 tip, v across -1 .. 1
        { return Vector2Add(Vector2Add(center, Vector2Scale(back, length * u)), Vector2Scale(across, width(u) * v)); };
        const bool litSide = Vector2DotProduct(across, light) > 0; // which half catches the light
        // Peeking out of a magma blob only the front shows; a pounce brings the whole tube out.
        const bool flying = shooting && boss.attack == BossAttack::ChargeDash;
        const float uMax = flying ? 1.0F : 0.45F;
        const Vector2 holeAt = at(uMax, 0);
        const float holeR = width(uMax) * 1.35F;
        if (!flying) // the hole it came up through
        {
            DrawCircleV(holeAt, holeR * 1.25F, paint(Palette::ElementBurn, 0.18F));
            DrawCircleV(holeAt, holeR, paint(ColorLerp(basalt, Palette::Void, 0.6F), 1.0F));
            DrawRing(holeAt, holeR - 3.0F, holeR + 2.0F, 0, 360, 40, paint(molten, 0.85F));
        }
        constexpr int32_t slices = 18;
        const Color dark = ColorLerp(basalt, Palette::Void, 0.5F);
        const Color lit = ColorLerp(basalt, basaltLit, 0.5F);
        for (int32_t i = 0; i < slices; i++)
        {
            const float u0 = static_cast<float>(i) / slices;
            const float u1 = std::min(static_cast<float>(i + 1) / slices, uMax);
            if (u0 >= uMax)
            {
                break;
            }
            const auto quad = [&](float v0, float v1, Color c)
            {
                const Vector2 p0 = at(u0, v0), p1 = at(u0, v1), p2 = at(u1, v1), p3 = at(u1, v0);
                DrawTriangle(p0, p1, p2, c);
                DrawTriangle(p0, p2, p1, c);
                DrawTriangle(p0, p2, p3, c);
                DrawTriangle(p0, p3, p2, c);
            };
            quad(-1.12F, 1.12F, paint(dark, 1.0F)); // outline
            quad(-1.0F, 0.0F, paint(litSide ? basalt : lit, 1.0F));
            quad(0.0F, 1.0F, paint(litSide ? lit : basalt, 1.0F));
        }
        // Screw ridges: slanted bands sliding toward the tip as the drill turns.
        constexpr int32_t ridges = 7;
        for (int32_t k = 0; k < ridges; k++)
        {
            const float u = std::fmod((static_cast<float>(k) + boss.moleDrill) / ridges, 1.0F) * 0.92F + 0.04F;
            const float slant = 0.07F;
            if (u + slant > uMax)
            {
                continue;
            }
            const Vector2 l = at(std::min(u + slant, 0.99F), -0.95F);
            const Vector2 r = at(u, 0.95F);
            DrawLineEx(l, r, radius * 0.07F * (1.0F - u * 0.6F), paint(dark, 0.9F));
            DrawLineEx(Vector2Add(l, Vector2Scale(back, -radius * 0.03F)), Vector2Add(r, Vector2Scale(back, -radius * 0.03F)),
                       radius * 0.02F, paint(basaltLit, 0.8F));
        }
        // Molten spine seam.
        const float spineEnd = std::min(0.9F, uMax);
        DrawLineEx(at(0.05F, 0), at(spineEnd, 0), radius * 0.07F, paint(ember, 0.4F * glow));
        DrawLineEx(at(0.05F, 0), at(spineEnd, 0), radius * 0.025F, paint(gold, glow));
        if (flying)
        {
            DrawCircleV(at(0.97F, 0), radius * 0.05F, paint(gold, 0.6F + 0.4F * glow)); // hot drill point
        }
        // Opened, the tube glows through the gaps.
        DrawLineEx(at(0.02F, 0), at(spineEnd, 0), radius * 0.5F * open, paint(ember, 0.25F * open));

        // Peel: four long petals, each covering a strip of the tube from behind the head to the
        // drill point. Closed they wrap it; opening, they stay fixed behind the head and peel
        // away outward like a banana's skin, curling more toward their free ends.
        constexpr int32_t strips = 4;
        const Vector2 root = at(0.04F, 0);
        for (int32_t k = 0; k < strips; k++)
        {
            const float vc = -0.75F + 0.5F * static_cast<float>(k); // strip centre across the tube
            const float sign = vc > 0 ? 1.0F : -1.0F;
            const float spread = (std::abs(vc) > 0.5F ? 1.45F : 0.75F) * open;
            // A point on the strip: on the closed tube, then turned about the root (more near the end).
            const auto peel = [&](float u, float v)
            {
                const Vector2 closed = Vector2Subtract(at(u, v), root);
                return Vector2Add(root, Vector2Rotate(closed, sign * spread * (0.55F + 0.45F * u)));
            };
            constexpr int32_t steps = 14;
            const float lo = vc - 0.3F;
            const float hi = vc + 0.3F;
            const bool litStrip = (sign * (litSide ? 1.0F : -1.0F)) > 0;
            const Color plate = ColorLerp(basalt, basaltLit, litStrip ? 0.28F : 0.06F);
            const Color fill = paint(ColorLerp(plate, ember, 0.3F * open), 1.0F);
            const Color inner = paint(ColorLerp(ember, gold, 0.4F), 0.9F);
            for (int32_t i = 0; i < steps; i++)
            {
                const float u0 = 0.04F + 0.93F * static_cast<float>(i) / steps;
                const float u1 = std::min(0.04F + 0.93F * static_cast<float>(i + 1) / steps, uMax);
                if (u0 >= uMax)
                {
                    break;
                }
                // The tip narrows to a point; peeled, the strip shows its glowing inner face.
                const float n0 = 1.0F - std::pow(u0, 4.0F);
                const float n1 = 1.0F - std::pow(u1, 4.0F);
                const Vector2 a0 = peel(u0, vc + (lo - vc) * n0);
                const Vector2 b0 = peel(u0, vc + (hi - vc) * n0);
                const Vector2 a1 = peel(u1, vc + (lo - vc) * n1);
                const Vector2 b1 = peel(u1, vc + (hi - vc) * n1);
                const Color c = ColorLerp(fill, inner, open * 0.6F * u0);
                DrawTriangle(a0, b0, b1, c);
                DrawTriangle(a0, b1, b0, c);
                DrawTriangle(a0, b1, a1, c);
                DrawTriangle(a0, a1, b1, c);
                DrawLineEx(a0, a1, 1.5F, paint(basalt, 0.9F));
                DrawLineEx(b0, b1, 1.5F, paint(basalt, 0.9F));
                // Steel glint along the lit edge.
                DrawLineEx(litStrip ? b0 : a0, litStrip ? b1 : a1, 1.5F, paint(basaltLit, 0.7F));
            }
            // Molten zigzag down the plate, Heatran-style hot lines in iron.
            const float hot = std::clamp(0.7F + 0.3F * glow + 0.3F * open, 0.0F, 1.0F);
            Vector2 prev = peel(0.08F, vc);
            for (int32_t i = 1; i <= 7; i++)
            {
                const float u = 0.08F + 0.8F * static_cast<float>(i) / 7.0F;
                if (u > uMax)
                {
                    break;
                }
                const float zig = (i % 2 == 0 ? 0.13F : -0.13F) * (1.0F - std::pow(u, 4.0F));
                const Vector2 q = peel(u, vc + zig);
                DrawLineEx(prev, q, radius * 0.07F, paint(molten, 0.3F * hot));
                DrawLineEx(prev, q, radius * 0.03F, paint(molten, hot));
                DrawLineEx(prev, q, radius * 0.012F, paint(gold, hot));
                prev = q;
            }
            // Rivets along both edges.
            for (const float u : {0.25F, 0.6F})
            {
                if (u > uMax)
                {
                    continue;
                }
                const float n = 1.0F - std::pow(u, 4.0F);
                for (const float edge : {lo + 0.07F, hi - 0.07F})
                {
                    const Vector2 r = peel(u, vc + (edge - vc) * n);
                    DrawCircleV(r, radius * 0.03F, paint(Palette::Void, 0.9F));
                    DrawCircleV(Vector2Add(r, Vector2Scale(light, radius * 0.01F)), radius * 0.015F, paint(basaltLit, 0.9F));
                }
            }
        }
    }

    // The head: a curved, pointed basalt nose (an ogive) leading the tube, molten seams sweeping
    // to a tip that heats up for attacks.
    float heat = boss.slagmawBloomed ? 0.7F : 0.2F;
    if (swallowing || (shooting && boss.attack == BossAttack::Devour))
    {
        heat = 1.0F;
    }
    else if (winding || shooting)
    {
        heat = std::max(heat, winding ? windup : 0.8F);
    }
    if (boss.slagmawPunishTimer > 0)
    {
        heat = 0.4F + 0.4F * std::sin(t * 9.0F);
    }
    Vector2 head = center;
    if (swallowing)
    {
        head = Vector2Add(head, Vector2{.x = std::sin(t * 37.0F) * 6.0F, .y = std::cos(t * 29.0F) * 6.0F});
    }
    {
        const Vector2 across{.x = -face.y, .y = face.x};
        const float base = radius * 0.94F;
        const float noseLength = radius * 1.35F;
        const auto nose = [&](float s, float v) // s 0 base .. 1 tip, v across -1 .. 1
        {
            const float w = base * std::sqrt(std::max(0.0F, 1.0F - s * s)) * std::pow(1.0F - s, 0.35F);
            return Vector2Add(Vector2Add(head, Vector2Scale(face, noseLength * s)), Vector2Scale(across, w * v));
        };
        const bool litSide = Vector2DotProduct(across, light) > 0;
        const Color dark = ColorLerp(basalt, Palette::Void, 0.5F);
        const Color lit = ColorLerp(basalt, basaltLit, 0.6F);
        constexpr int32_t slices = 16;
        for (int32_t i = 0; i < slices; i++)
        {
            const float s0 = static_cast<float>(i) / slices;
            const float s1 = static_cast<float>(i + 1) / slices;
            const auto quad = [&](float v0, float v1, Color c, float grow)
            {
                const auto g = [&](float s, float v) { return Vector2Add(nose(s, v), Vector2Scale(face, grow * (s > 0.99F ? 1.0F : 0.0F))); };
                const Vector2 p0 = g(s0, v0), p1 = g(s0, v1), p2 = g(s1, v1), p3 = g(s1, v0);
                DrawTriangle(p0, p1, p2, c);
                DrawTriangle(p0, p2, p1, c);
                DrawTriangle(p0, p2, p3, c);
                DrawTriangle(p0, p3, p2, c);
            };
            quad(-1.1F, 1.1F, paint(dark, 1.0F), radius * 0.04F); // outline
            quad(-1.0F, 0.0F, paint(litSide ? basalt : lit, 1.0F), 0);
            quad(0.0F, 1.0F, paint(litSide ? lit : basalt, 1.0F), 0);
        }
        // Base rim where the peel attaches.
        DrawLineEx(nose(0, -1.05F), nose(0, 1.05F), radius * 0.08F, paint(dark, 1.0F));
        // Steel glint down the lit flank.
        for (int32_t i = 0; i < 8; i++)
        {
            const float s0 = 0.08F + 0.7F * static_cast<float>(i) / 8.0F;
            const float s1 = 0.08F + 0.7F * static_cast<float>(i + 1) / 8.0F;
            const float v = litSide ? 0.6F : -0.6F;
            DrawLineEx(nose(s0, v), nose(s1, v), radius * 0.03F, paint(basaltLit, 0.8F));
        }
        // Molten chevrons pointing to the tip, and a hot line up the crest.
        const float seamGlow = std::clamp(0.7F + 0.3F * glow + 0.3F * heat, 0.0F, 1.0F);
        const auto hotLine = [&](Vector2 a, Vector2 b, float w)
        {
            DrawLineEx(a, b, radius * 0.08F * w, paint(molten, 0.3F * seamGlow));
            DrawLineEx(a, b, radius * 0.035F * w, paint(molten, seamGlow));
            DrawLineEx(a, b, radius * 0.014F * w, paint(gold, seamGlow));
        };
        for (const float c : {0.12F, 0.36F, 0.58F})
        {
            const float w = 1.0F - c * 0.6F;
            hotLine(nose(c, -0.8F), nose(c + 0.14F, 0), w);
            hotLine(nose(c + 0.14F, 0), nose(c, 0.8F), w);
        }
        hotLine(nose(0.74F, 0), nose(0.95F, 0), 0.8F);
        for (const float v : {-0.7F, -0.35F, 0.0F, 0.35F, 0.7F}) // rivets on the base rim
        {
            DrawCircleV(nose(0.03F, v), radius * 0.035F, paint(Palette::Void, 0.9F));
            DrawCircleV(nose(0.03F, v), radius * 0.017F, paint(basaltLit, 0.9F));
        }
        const Vector2 tip = nose(1.0F, 0);
        DrawCircleV(tip, radius * (0.1F + 0.12F * heat), paint(ember, 0.25F + 0.4F * heat));
        DrawCircleV(tip, radius * (0.04F + 0.05F * heat), paint(gold, 0.7F + 0.3F * heat));
    }
    if (swallowing && game.run.qtePrompt != QtePrompt::None && game.run.qtePrompt != QtePrompt::Mash &&
        game.run.qteTimerMax > 0)
    {
        const float left = std::clamp(game.run.qteTimer / game.run.qteTimerMax, 0.0F, 1.0F);
        const Vector2 gob = Vector2Subtract(game.run.player.position, Vector2Scale(game.run.qteThreatDir, 240.0F * left));
        drawSlagGob(gob, 13.0F, game.run.qteThreatDir);
    }
}

} // namespace DrawDetail
