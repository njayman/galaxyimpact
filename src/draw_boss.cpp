#include "draw_internal.hpp"

#include "rlgl.h"
#include <span>

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


// Wreckworm: flat rust hull plates over a steel spine, with the slime that infected the wreck
// and drives it showing wherever the hull is open.
constexpr Color wormRust{.r = 122, .g = 88, .b = 62, .a = 255};
constexpr Color wormRustDark{.r = 74, .g = 56, .b = 44, .a = 255};
constexpr Color wormRustLite{.r = 158, .g = 124, .b = 88, .a = 255};
constexpr Color wormSteelDark{.r = 44, .g = 46, .b = 50, .a = 255};
constexpr Color wormSlime{.r = 104, .g = 128, .b = 58, .a = 255};
constexpr Color wormSlimeDark{.r = 62, .g = 78, .b = 40, .a = 255};

// The thing inside, drawn round the origin: a lobed, pulsing mass with a roving dark nucleus and
// feelers. size is its radius; out 0..1 is how far the feelers reach.
static void drawWormParasite(float size, float out, float seed)
{
    const auto t = static_cast<float>(GetTime());
    for (int32_t i = 0; i < 6; i++)
    {
        const auto fi = static_cast<float>(i);
        const float a =
            seed + fi * 2.0F * std::numbers::pi_v<float> / 6.0F + 0.3F * std::sin(t * 1.3F + fi);
        Vector2 p{.x = std::cos(a) * size * 0.6F, .y = std::sin(a) * size * 0.6F};
        const float len = size * (0.5F + 1.5F * out) * (0.8F + 0.2F * std::sin(t * 2.0F + fi * 1.7F));
        for (int32_t k = 0; k < 8; k++)
        {
            const float u = static_cast<float>(k) / 8.0F;
            const float bend = a + 0.5F * std::sin(t * 2.4F + fi * 2.1F + u * 4.0F);
            const Vector2 q = Vector2Add(p, Vector2{.x = std::cos(bend) * len / 8.0F, .y = std::sin(bend) * len / 8.0F});
            const float w = size * 0.22F * (1.0F - u) + 1.5F;
            DrawLineEx(p, q, w, wormSlimeDark);
            DrawCircleV(q, w / 2, wormSlimeDark);
            p = q;
        }
    }
    for (int32_t i = 0; i < 7; i++)
    {
        const auto fi = static_cast<float>(i);
        const float a = seed * 1.7F + fi * 2.0F * std::numbers::pi_v<float> / 7.0F;
        const float r = size * (0.42F + 0.1F * std::sin(t * 2.2F + fi * 1.9F));
        DrawCircleV(Vector2{.x = std::cos(a) * size * 0.62F, .y = std::sin(a) * size * 0.62F}, r,
                    (i % 2) != 0 ? wormSlime : wormSlimeDark);
    }
    DrawCircleV(Vector2{}, size * (0.72F + 0.05F * std::sin(t * 3.0F)), Palette::RustbloomAccent);
    const Vector2 look{.x = size * 0.12F * std::sin(t * 0.9F + seed), .y = size * 0.1F * std::cos(t * 1.3F + seed)};
    DrawCircleV(look, size * 0.34F, wormSlimeDark);
    DrawCircleV(look, size * 0.15F, Palette::RustbloomVoid);
}

// The spine between two Wreckworm parts, the infection running down it.
void drawWreckwormGooBridge(Vector2 from, Vector2 to, float fromRadius, float toRadius, float seed)
{
    (void)seed;
    const float dist = Vector2Distance(from, to);
    if (dist < fromRadius * 1.4F || dist > (fromRadius + toRadius) * 2.6F)
    {
        return; // closed up (the plates cover it), or shed and torn apart
    }
    DrawLineEx(from, to, fromRadius * 0.42F, wormSteelDark);
    DrawLineEx(from, to, fromRadius * 0.13F, wormSlimeDark);
}

// A gun pushed out of a body segment, aimed at the player. ext 0..1.
void drawWreckwormSegmentGun(Vector2 center, float radius, Vector2 aim, float ext, bool flash)
{
    const float e = 1.0F - (1.0F - ext) * (1.0F - ext) * (1.0F - ext);
    const Vector2 tip = Vector2Add(center, Vector2Scale(aim, radius * wreckwormGunLength * e));
    DrawLineEx(center, tip, radius * 0.34F, wormSteelDark);
    DrawLineEx(Vector2Subtract(tip, Vector2Scale(aim, radius * 0.22F * e)), tip, radius * 0.46F, wormRustLite);
    DrawCircleV(tip, radius * 0.14F * e, Fade(Palette::RustbloomAccent, 0.4F + 0.5F * e));
    if (flash)
    {
        DrawCircleV(tip, radius * 0.26F, Fade(Palette::Crit, 0.85F));
        DrawCircleV(tip, radius * 0.13F, WHITE);
    }
}

// Armoured: a hull chevron pointing up the spine. Bare: the steel core with the slime showing.
void drawWreckwormSegmentShape(Vector2 center, float radius, float seed, Color baseColor,
                               bool armor, float spinDeg, Vector2 spineDir)
{
    const Vector2 dir = Vector2Length(spineDir) > 0.01F ? spineDir : Vector2{.x = 0, .y = -1};
    const auto tint = [&](Color c) { return ColorLerp(c, baseColor, 0.3F); };
    rlPushMatrix();
    rlTranslatef(center.x, center.y, 0);
    rlRotatef(std::atan2(dir.y, dir.x) * RAD2DEG + spinDeg, 0, 0, 1);
    DrawCircleV(Vector2{}, radius * 0.5F, wormSteelDark);
    if (armor)
    {
        const float r = radius * 1.15F;
        const Color plate = tint(static_cast<int32_t>(seed) % 2 != 0 ? wormRust : wormRustLite);
        const Vector2 tip{.x = r * 0.9F, .y = 0};
        const Vector2 notch{.x = -r * 0.3F, .y = 0};
        for (const float side : {1.0F, -1.0F})
        {
            const Vector2 shoulder{.x = r * 0.1F, .y = side * r};
            flatTriangle(tip, shoulder, notch, plate);
            flatTriangle(shoulder, Vector2{.x = -r * 0.7F, .y = side * r * 0.85F}, notch, plate);
        }
        flatTriangle(Vector2{.x = r * 0.5F, .y = 0}, Vector2{.x = -r * 0.05F, .y = r * 0.35F},
                     Vector2{.x = -r * 0.05F, .y = -r * 0.35F}, tint(wormRustDark));
    }
    else
    {
        drawWormParasite(radius * 0.34F, 0.25F, seed);
    }
    rlPopMatrix();
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
        // Shut while it travels, breathing a crack; it opens to spit, half-opens to coil, and
        // hangs open while stunned.
        const float idleMouth = 0.04F + 0.04F * std::sin(static_cast<float>(GetTime()) * 2.0F);
        const float flash = std::clamp(boss.hitFlashTimer / UpdateConstants::hitFlashDuration, 0.0F, 1.0F);
        drawWreckwormHeadShape(bossCenter, boss.size.x / 2, static_cast<float>(boss.instanceId),
                               ColorLerp(ufoColor, WHITE, flash), facing,
                               [&]() -> float { if (spitting || boss.wreckwormPunishTimer > 0) { return 1.0F; } if (coiling) { return 0.55F; } return idleMouth; }());
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
        // Hauled down the tear: it spirals in head first, shrinking and going dark.
        const float progress =
            std::clamp(1.0F - game.run.bossCutsceneTimer / krakenOutroTwirlDuration, 0.0F, 1.0F);
        const Vector2 hole = game.run.blackhole.position;
        const Vector2 arm = Vector2Rotate(Vector2Subtract(bossCenter, hole), progress * progress * 9.0F);
        drawKrakenShape(Vector2Add(hole, arm), boss.size.x / 2 * std::max(0.04F, 1.0F - progress),
                        static_cast<float>(boss.instanceId),
                        ColorLerp(ColorLerp(boss.baseColor, Palette::StructMid, 0.75F), BLACK, progress * 0.8F),
                        Vector2Rotate(Vector2Negate(arm), 0.9F), 0.35F + 1.5F * progress, Palette::StructDark);
    }
    else if (boss.isKraken)
    {
        // The body keeps its lavender; the attack colour shows in the eye and arm tips.
        const bool drained = boss.health <= 0 || game.run.bossCutscenePhase != 0;
        const bool idle = boss.color.r == Palette::StructMid.r && boss.color.g == Palette::StructMid.g;
        const float flash = std::clamp(boss.hitFlashTimer / UpdateConstants::hitFlashDuration, 0.0F, 1.0F);
        drawKrakenShape(bossCenter, boss.size.x / 2, static_cast<float>(boss.instanceId),
                        ColorLerp(drained ? ColorLerp(boss.baseColor, Palette::StructMid, 0.75F) : boss.baseColor, WHITE, flash),
                        Vector2Subtract(game.run.player.position, bossCenter), drained ? 0.35F : 1.0F,
                        idle || drained ? Palette::ElementFreeze : boss.color);
    }
    else if (boss.isBanished)
    {
        drawBanishedShape(bossCenter, boss, game.run.player.position);
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
            drawBanishedTentacle(boss, slot);
        }
        drawBanishedToss(boss, game.run.player.position);
        drawBanishedEye(boss, game.run.player.position);
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
        Vector2LengthSqr(boss.chargeVelocity) > 0.01F && !boss.isWreckwormHead &&
        !boss.isWreckwormSegment && !boss.isKraken && !boss.isBanished &&
        (!boss.isSlagmaw || boss.attack != BossAttack::ChargeDash)) // a leaping mole trails nothing
    {
        const Vector2 back = Vector2Normalize(Vector2Negate(boss.chargeVelocity));
        for (int32_t k = 1; k <= 4; k++)
        {
            const auto f = static_cast<float>(k);
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
            if (beam) // energy gathering where the beam will leave
            {
                drawBeamCharge(origin, 22.0F, progress, locked ? 1.0F : 0.0F,
                               boss.attack == BossAttack::WormholeBeam ? Palette::Shield : Palette::Accent);
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

            (void)beamRec;
            (void)beamOrigin;
            (void)rotation;
            drawBeam(segStart, Vector2Add(segStart, Vector2Scale(segDir, beamLength)), 20.0F, -1.0F, Palette::Accent);
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
        (void)beamRec;
        (void)beamOrigin;
        (void)rotation;
        drawBeam(boss.wormholeBeamOrigin, Vector2Add(boss.wormholeBeamOrigin, Vector2Scale(direction, 2000.0F)), 20.0F, -1.0F,
                 Palette::Shield);

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
        const float a = spin + static_cast<float>(i) * 2.0F * std::numbers::pi_v<float> / sides;
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
            const float a =
                hashNoise(pool.position.y * 0.01F + kf * 3.0F) * 2.0F * std::numbers::pi_v<float>;
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

// What the Banished hurls: a real enemy, a meteor or a lava chunk (kind indexes the enemy kinds,
// then meteor, then lava chunk), wrapped in the energy it was charged with: turning gold plate
// rings and sparks orbiting it.
static void drawBanishedThrowable(Vector2 pos, int32_t kind, float scale, float facingDeg, Vector2 velocity)
{
    if (kind < 0 || scale <= 0.01F)
    {
        return;
    }
    const auto t = static_cast<float>(GetTime());
    const auto enemyCount = static_cast<int32_t>(enemyKinds.size());
    const float seed = static_cast<float>(kind) * 3.7F;
    float radius = 22.0F * scale;
    DrawCircleV(pos, radius * 1.5F, Fade(Palette::Crit, 0.14F));
    if (kind < enemyCount)
    {
        const auto& enemy = enemyKinds.at(static_cast<size_t>(kind));
        radius = std::clamp(enemy.radius, 14.0F, 26.0F) * scale;
        drawEnemyShape(enemy.shape, pos, radius, facingDeg, ColorLerp(enemy.color, Palette::Crit, 0.25F), seed);
    }
    else if (kind == enemyCount)
    {
        drawMeteorBossShape(pos, radius, 7, ColorLerp(Palette::StructMid, Palette::Crit, 0.25F));
    }
    else
    {
        drawLavaChunk(pos, radius, seed, velocity);
    }
    plateRing(pos, radius * 1.25F, radius * 1.42F, 3, t * 260.0F, 40.0F, Palette::Crit);
    plateRing(pos, radius * 1.6F, radius * 1.7F, 2, t * -190.0F, 90.0F, Fade(Palette::Crit, 0.6F));
    for (int32_t k = 0; k < 4; k++)
    {
        const float a = t * 5.0F + static_cast<float>(k) * 1.571F;
        DrawPoly(Vector2Add(pos, Vector2{.x = std::cos(a) * radius * 1.9F, .y = std::sin(a) * radius * 1.9F}), 4, radius * 0.16F,
                 a * RAD2DEG, ColorLerp(Palette::Crit, WHITE, 0.4F));
    }
}

// A hurled, energized throwable in flight: spinning, a wake of fading plates behind it.
void drawBanishedHurled(const BossProjectile& projectile)
{
    const auto t = static_cast<float>(GetTime());
    const float speed = Vector2Length(projectile.velocity);
    const Vector2 back = speed > 0.01F ? Vector2Scale(projectile.velocity, -1.0F / speed) : Vector2{.x = 0, .y = 1};
    for (int32_t k = 1; k <= 6; k++)
    {
        const auto fk = static_cast<float>(k);
        const Vector2 at = Vector2Add(projectile.position, Vector2Scale(back, projectile.radius * (0.9F + fk * 0.8F)));
        DrawPoly(at, 4, projectile.radius * (0.55F - fk * 0.07F), t * 300.0F + fk * 40.0F, Fade(Palette::Crit, 0.5F - fk * 0.07F));
    }
    drawBanishedThrowable(projectile.position, projectile.visualEnemyKind, 1.0F, t * 400.0F, projectile.velocity);
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

// Head: a salvaged prow in three plates. It opens three ways (nose forward, wings out and back)
// on the slime inside, a strand stretched to each plate. open 0..1.
void drawWreckwormHeadShape(Vector2 center, float radius, float seed, Color color, Vector2 facing, float open)
{
    const Vector2 face = Vector2Length(facing) > 0.01F ? facing : Vector2{.x = 0, .y = -1};
    const float r = radius * 0.85F;
    const auto tint = [&](Color c) { return ColorLerp(c, color, 0.3F); };
    rlPushMatrix();
    rlTranslatef(center.x, center.y, 0);
    rlRotatef(std::atan2(face.y, face.x) * RAD2DEG, 0, 0, 1);
    rlTranslatef(-r * 0.3F, 0, 0);
    const Vector2 c{.x = r * 0.2F, .y = 0};
    rlPushMatrix();
    rlTranslatef(c.x, c.y, 0);
    drawWormParasite(r * (0.34F + 0.3F * open), open, seed);
    rlPopMatrix();
    const Vector2 tip{.x = r * 1.5F, .y = 0};
    const Vector2 rl{.x = -r * 0.8F, .y = -r * 0.9F};
    const Vector2 rr{.x = -r * 0.8F, .y = r * 0.9F};
    const Vector2 notch{.x = -r * 0.45F, .y = 0};
    const Vector2 ml = Vector2Lerp(rl, tip, 0.5F);
    const Vector2 mr = Vector2Lerp(rr, tip, 0.5F);
    const auto piece = [&](Vector2 dir, Vector2 a, Vector2 b, Vector2 d, Color col)
    {
        const Vector2 o = Vector2Scale(dir, r * 0.62F * open);
        const Vector2 pc = Vector2Add(c, o);
        const Vector2 pa = Vector2Add(a, o);
        const Vector2 pb = Vector2Add(b, o);
        const Vector2 pd = Vector2Add(d, o);
        DrawLineEx(c, pc, r * 0.07F * (1.0F - 0.6F * open), wormSlimeDark);
        flatTriangle(pc, pa, pb, col);
        flatTriangle(pc, pb, pd, col);
        const Vector2 g = Vector2Scale(Vector2Add(Vector2Add(pa, pb), Vector2Add(pc, pd)), 0.25F);
        flatTriangle(Vector2Lerp(g, pa, 0.5F), Vector2Lerp(g, pb, 0.55F), Vector2Lerp(g, pd, 0.5F), tint(wormRustDark));
    };
    piece(Vector2{.x = 1, .y = 0}, ml, tip, mr, tint(wormRustLite));
    piece(Vector2Normalize(Vector2{.x = -0.5F, .y = -0.87F}), notch, rl, ml, tint(wormRust));
    piece(Vector2Normalize(Vector2{.x = -0.5F, .y = 0.87F}), mr, rr, notch, tint(wormRust));
    rlPopMatrix();
}

// raylib culls one winding, so order the corners first.
void flatTriangle(Vector2 a, Vector2 b, Vector2 c, Color color)
{
    if ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0)
    {
        std::swap(b, c);
    }
    DrawTriangle(a, b, c, color);
}

// A tentacle as a chain of flat plates: diamonds strung along the polyline, shrinking from
// rootWidth to tipWidth, alternating between two tones.
void drawChain(std::span<const Vector2> pts, float rootWidth, float tipWidth, Color a, Color b)
{
    float total = 0;
    for (size_t i = 1; i < pts.size(); i++)
    {
        total += Vector2Distance(pts[i - 1], pts[i]);
    }
    if (total < 1.0F)
    {
        return;
    }
    float walked = 0;   // arc length of the next plate
    float segStart = 0; // arc length at the start of this segment
    int32_t count = 0;
    for (size_t i = 1; i < pts.size(); i++)
    {
        const float segLen = Vector2Distance(pts[i - 1], pts[i]);
        if (segLen < 0.001F)
        {
            continue;
        }
        const float rot = std::atan2(pts[i].y - pts[i - 1].y, pts[i].x - pts[i - 1].x) * RAD2DEG;
        while (walked <= segStart + segLen && count < 200)
        {
            const float w = std::max(1.5F, std::lerp(rootWidth, tipWidth, walked / total) * 0.62F);
            DrawPoly(Vector2Lerp(pts[i - 1], pts[i], (walked - segStart) / segLen), 4, w, rot, (count % 2) == 0 ? a : b);
            walked += w * 1.25F;
            count++;
        }
        segStart += segLen;
    }
}

// Flat plates in a ring with `gapDeg` between them: the turning-ring motif.
void plateRing(Vector2 center, float inner, float outer, int32_t plates, float rotDeg, float gapDeg, Color color)
{
    const float step = 360.0F / static_cast<float>(plates);
    const int32_t segments = std::max(4, static_cast<int32_t>(step / 6.0F));
    for (int32_t i = 0; i < plates; i++)
    {
        const float a0 = rotDeg + static_cast<float>(i) * step;
        DrawRing(center, inner, outer, a0 + gapDeg / 2, a0 + step - gapDeg / 2, segments, color);
    }
}

// A tear in space: a black disc inside broken rings turning against each other.
void drawTearPortal(Vector2 center, float radius, float open)
{
    const float r = radius * std::clamp(open, 0.0F, 1.0F);
    if (r < 1.0F)
    {
        return;
    }
    const auto t = static_cast<float>(GetTime());
    DrawCircleV(center, r * 1.5F, Fade(Palette::PunctumHaze, 0.08F));
    plateRing(center, r * 1.24F, r * 1.36F, 5, t * -35.0F, 22.0F, ColorLerp(Palette::PunctumHaze, Palette::Void, 0.5F));
    plateRing(center, r * 1.05F, r * 1.17F, 3, t * 60.0F, 40.0F, Palette::PunctumHaze);
    DrawCircleV(center, r, BLACK);
    plateRing(center, r * 0.52F, r * 0.6F, 4, t * -90.0F, 50.0F, Fade(Palette::ElementFreeze, 0.45F));
}

// The Kraken, facing the player: a mantle of four stacked hull chevrons trailing away, a ring
// collar round a hole for a head, and eight arms (chains of flat plates) fanned out ahead. `life` under 1
// slows and shortens the arms and puts the eye out (the drained Kraken of the outro). `signal`
// colours the eye and arm tips: the attack it is winding up.
void drawKrakenShape(Vector2 center, float radius, float seed, Color color, Vector2 facing, float life, Color signal)
{
    const auto t = static_cast<float>(GetTime());
    const int32_t luma = (color.r * 3 + color.g * 6 + color.b) / 10;
    const Color base = luma < 40 ? ColorLerp(color, Palette::StructMid, 0.55F) : color;
    const auto tone = [&](Color c)
    {
        c.a = color.a;
        return c;
    };
    const Color body = tone(ColorLerp(base, Palette::Void, 0.22F));
    const Color deep = tone(ColorLerp(base, Palette::Void, 0.58F));
    const Color part = tone(ColorLerp(base, WHITE, 0.25F));
    const Color arm = tone(ColorLerp(base, Palette::Void, 0.42F));

    const Vector2 f = Vector2Length(facing) > 0.001F ? Vector2Normalize(facing) : Vector2{.x = 0, .y = 1};
    const Vector2 side{.x = -f.y, .y = f.x};
    const float facingAngle = std::atan2(f.y, f.x);
    const auto at = [&](float along, float across)
    { return Vector2Add(center, Vector2Add(Vector2Scale(f, along * radius), Vector2Scale(side, across * radius))); };
    const Vector2 head = at(0.5F, 0);

    constexpr int32_t armPoints = 14;
    for (int32_t i = 0; i < 8; i++)
    {
        const auto fi = static_cast<float>(i);
        const float angle = facingAngle + (fi - 3.5F) * 0.5F + 0.2F * std::sin(t * 0.5F * life + fi * 1.3F + seed);
        const Vector2 dir{.x = std::cos(angle), .y = std::sin(angle)};
        const Vector2 perp{.x = -dir.y, .y = dir.x};
        const float length = radius * (5.2F + 1.0F * std::sin(t * 0.7F * life + fi * 2.1F)) * (0.55F + 0.45F * life);
        std::array<Vector2, armPoints> pts{};
        for (int32_t k = 0; k < armPoints; k++)
        {
            const float frac = static_cast<float>(k) / static_cast<float>(armPoints - 1);
            const float wave = (0.07F * std::sin(t * 1.5F * life - frac * 7.0F + fi * 1.7F + seed) +
                                0.16F * std::sin(t * 0.8F * life - frac * 2.6F + fi)) * length * frac;
            pts.at(static_cast<size_t>(k)) = Vector2Add(
                head, Vector2Add(Vector2Scale(dir, radius * 0.5F + length * frac), Vector2Scale(perp, wave)));
        }
        drawChain(pts, radius * 0.5F, radius * 0.12F, body, arm);
        if (life > 0.5F)
        {
            DrawPoly(pts.back(), 4, radius * 0.07F, t * 90.0F, tone(signal));
        }
    }

    // Mantle: four hull chevrons stacked back from the head, each smaller, pointing away.
    for (int32_t i = 3; i >= 0; i--)
    {
        const auto fi = static_cast<float>(i);
        const float along = 0.2F - fi * 0.55F;
        const float w = 1.0F - 0.2F * fi;
        const Color plate = (i % 2) != 0 ? deep : body;
        flatTriangle(at(along - w * 1.1F, 0), at(along, w), at(along, -w), plate);
        flatTriangle(at(along, w), at(along + w * 0.35F, 0), at(along, -w), plate);
    }
    // Head: a ring collar round a hole, the attack signal turning inside it.
    DrawCircleV(head, radius * 0.62F, deep);
    plateRing(head, radius * 0.5F, radius * 0.7F, 6, t * 16.0F * life, 12.0F, part);
    const float lit = std::clamp(life * 2.0F - 1.0F, 0.0F, 1.0F);
    plateRing(head, radius * 0.28F, radius * 0.4F, 3, t * -40.0F * life, 30.0F, tone(ColorLerp(deep, signal, lit)));
    DrawCircleV(head, radius * 0.24F, tone(Palette::Void));
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

    const auto t = static_cast<float>(GetTime());
    const Vector2 overallDir = Vector2Scale(Vector2Subtract(tip, anchor), 1.0F / length);
    const Vector2 perp{.x = -overallDir.y, .y = overallDir.x};
    constexpr int32_t segments = 12;
    std::array<Vector2, segments + 1> pts{};
    pts.front() = anchor;
    for (int32_t i = 1; i <= segments; i++)
    {
        const float frac = static_cast<float>(i) / static_cast<float>(segments);
        const float wave =
            std::sin(t * 5.0F + frac * 7.0F + seed) * waveScale * frac * (calm ? 0.35F : 1.0F);
        pts.at(static_cast<size_t>(i)) = Vector2Add(bezier(frac), Vector2Scale(perp, wave));
    }
    drawChain(pts, 30.0F * thicknessScale, 9.0F * thicknessScale, color, ColorLerp(color, Palette::Void, 0.45F));
    DrawPoly(pts.back(), 4, 9.0F * thicknessScale, t * 90.0F, tipColor);
}

void drawKrakenStaggeredTentacle(const Boss& boss, int32_t slot)
{
    const Vector2 bodyCenter{.x = boss.position.x + boss.size.x / 2,
                             .y = boss.position.y + boss.size.y / 2};
    const Vector2 gate = krakenPodPosition(boss, slot % 4);
    const Vector2 tip = boss.tentacleStaggerPos.at(static_cast<size_t>(slot));
    const auto t = static_cast<float>(GetTime());
    const float flash = 0.5F + 0.5F * std::sin(t * 10.0F);
    const Color hangColor = ColorLerp(Palette::Crit, WHITE, flash * 0.4F);
    drawBendyTentacle(gate, tip, bodyCenter, boss.size.x / 2,
                      static_cast<float>(boss.instanceId) + 7.0F, krakenTentacleThickness, 34.0F, true,
                      hangColor, hangColor);
    plateRing(tip, krakenTentacleHitRadius - 4.0F, krakenTentacleHitRadius, 3, t * 120.0F, 40.0F,
              Fade(Palette::Crit, 0.5F + flash * 0.3F));
}

void drawKrakenPortal(Vector2 portal, float t, float open)
{
    drawTearPortal(portal, krakenTentaclePortalRadius * (0.85F + 0.15F * std::sin(t * 6.0F)), open);
}

// A rope arm: a chain of plates, thick at the root, small at the tip.
void drawKrakenRope(const std::array<Vector2, Boss::krakenRopePoints>& pts, float rootWidth, float tipWidth, float alpha)
{
    drawChain(pts, (rootWidth + 2.0F) * 1.6F, tipWidth * 1.6F, Fade(ColorLerp(Palette::PunctumHaze, Palette::Void, 0.3F), alpha),
              Fade(ColorLerp(Palette::PunctumHaze, Palette::Void, 0.62F), alpha));
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
        const auto& rope = boss.ropePos.at(static_cast<size_t>(k) + 4);
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
                for (int32_t di = 0; 30.0F + static_cast<float>(di) * 36.0F < krakenTentacleReach; di++)
                {
                    const float d = 30.0F + static_cast<float>(di) * 36.0F;
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
    const int32_t pod = boss.limbPod.at(slot);
    if (pod < 0)
    {
        return;
    }

    const Vector2 center{.x = boss.position.x + boss.size.x / 2,
                         .y = boss.position.y + boss.size.y / 2};
    const float bodyRadius = boss.size.x / 2;
    const auto seed = static_cast<float>(boss.instanceId);
    const float podAngle = static_cast<float>(pod) * 90.0F * DEG2RAD + seed;
    const Vector2 podPos =
        Vector2Add(center, Vector2{.x = std::cos(podAngle) * boss.size.x / 2 * 0.78F,
                                   .y = std::sin(podAngle) * boss.size.y / 2 * 0.78F});

    Vector2 tip = podPos;
    float openProgress = 1.0F;
    bool grabbing = false;
    float grabbedEnemyScale = 0.0F;
    if (boss.limbPhase.at(slot) == 1)
    {
        openProgress = std::clamp(1.0F - boss.limbTimer.at(slot) / krakenLimbOpenDuration, 0.0F, 1.0F);
        tip = Vector2Lerp(podPos, boss.limbGrabTarget.at(slot), openProgress);
    }
    else if (boss.limbPhase.at(slot) == 2)
    {
        if (boss.limbThrowPhase.at(slot) == 1)
        {
            grabbing = true;
            grabbedEnemyScale = 1.0F;
            const float progress =
                1.0F -
                std::clamp(boss.limbThrowTimer.at(slot) / krakenLimbLeanBackDuration, 0.0F, 1.0F);
            const Vector2 leanBackTarget = Vector2Add(
                podPos, Vector2Scale(Vector2Subtract(podPos, boss.limbGrabTarget.at(slot)),
                                     krakenLimbNearReach /
                                         std::max(1.0F, Vector2Distance(
                                                            podPos, boss.limbGrabTarget.at(slot)))));
            tip = Vector2Lerp(boss.limbPortalPos.at(slot), leanBackTarget, progress);
        }
        else if (boss.limbThrowPhase.at(slot) == 2)
        {
            grabbing = true;
            grabbedEnemyScale = 1.0F;
            const float progress =
                1.0F - std::clamp(boss.limbThrowTimer.at(slot) / krakenLimbThrowDuration, 0.0F, 1.0F);
            const Vector2 leanBackTarget = Vector2Add(
                podPos, Vector2Scale(Vector2Subtract(podPos, boss.limbGrabTarget.at(slot)),
                                     krakenLimbNearReach /
                                         std::max(1.0F, Vector2Distance(
                                                            podPos, boss.limbGrabTarget.at(slot)))));
            tip = Vector2Lerp(leanBackTarget, boss.limbGrabTarget.at(slot), progress);
        }
        else if (boss.limbGrabAnimTimer.at(slot) > 0)
        {
            grabbing = true;
            tip = boss.limbPortalPos.at(slot);
            const float elapsed = krakenLimbGrabAnimDuration - boss.limbGrabAnimTimer.at(slot);
            if (elapsed < krakenLimbGrabAnimDuration / 3.0F)
            {
                tip = Vector2Lerp(
                    podPos, boss.limbPortalPos.at(slot),
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
            tip = boss.limbGrabTarget.at(slot);
        }
    }
    else if (boss.limbPhase.at(slot) == 3)
    {
        openProgress = std::clamp(boss.limbTimer.at(slot) / krakenLimbRetreatDuration, 0.0F, 1.0F);
        tip = Vector2Lerp(podPos, boss.limbGrabTarget.at(slot), openProgress);
    }
    else
    {
        return;
    }

    const float pulse = 0.6F + 0.4F * std::sin(static_cast<float>(GetTime()) * 3.0F + seed);
    const float podRadius = boss.size.x / 2 * 0.15F * krakenLimbSizeMult * (0.7F + 0.3F * openProgress);
    DrawCircleV(podPos, podRadius, ColorLerp(Palette::PunctumHaze, Palette::Void, 0.7F));
    plateRing(podPos, podRadius * 0.55F, podRadius * 0.8F, 3, static_cast<float>(GetTime()) * 90.0F, 30.0F,
              Fade(Palette::ElementFreeze, 0.4F + 0.3F * pulse));

    const float length = Vector2Distance(podPos, tip);
    if (length < 4.0F)
    {
        return;
    }

    if (grabbing && boss.limbThrowPhase.at(slot) == 0)
    {
        const float grabPulse = 0.7F + 0.3F * std::sin(static_cast<float>(GetTime()) * 10.0F);
        drawTearPortal(tip, 16.0F * krakenLimbSizeMult * grabPulse, 1.0F);
    }

    drawBendyTentacle(podPos, tip, center, bodyRadius, seed, krakenTentacleThickness, 24.0F, grabbing,
                      ColorLerp(Palette::PunctumHaze, Palette::Void, 0.3F), Palette::Crit);

    if (grabbedEnemyScale > 0.0F)
    {
        const Vector2 overallDir = Vector2Scale(Vector2Subtract(tip, podPos), 1.0F / length);
        drawKrakenGrabbedEnemy(tip, boss.limbGrabbedKind.at(slot), grabbedEnemyScale,
                               std::atan2(overallDir.y, overallDir.x));
    }
}

// Banished: a matte dark disc. Twelve ash-bone ticks stand round it like a clock face, no two the
// same size or shade; the tentacles come out from behind the disc between them. A tick goes short
// and dark when the tentacle beside it is staggered, so the face counts down to the eye. The eye is eclipsed: a sliver of gold shows past the disc that covers it, and when it
// opens it leaves an empty socket (the eye itself is drawn by drawBanishedEye). The body does not
// turn: only the ticks circle it, and the eye faces `facing` (the ship). `reveal` 0..1
// assembles it, the ticks closing in from outside (entry cutscene, and arriving from a ring hop).
void drawBanishedBody(Vector2 center, float size, float seed, float reveal, uint32_t brokenSeals, float eyeGlow,
                      float flash, float facing)
{
    (void)seed; // ponytail: kept in the signature for the cutscene caller; nothing random in the body now
    const auto t = static_cast<float>(GetTime());
    const float grow = std::clamp(reveal, 0.0F, 1.0F);
    const float spread = 1.0F + (1.0F - grow) * 1.4F;
    const float r = size * banishedFaceScale; // tick tips
    const Color flesh = ColorLerp(banishedFlesh, WHITE, 0.4F * flash);
    const Color bone = ColorLerp(banishedBone, Palette::Crit, 0.6F * eyeGlow);
    const Color dim = ColorLerp(banishedBone, banishedVoid, 0.6F);

    for (int32_t i = 0; i < Boss::banishedTentacleCount; i++)
    {
        // No two ticks alike: each has its own length, width and shade of bone.
        const auto fi = static_cast<float>(i);
        const bool broken = (brokenSeals >> static_cast<uint32_t>(i) & 1U) != 0;
        const float a = (t * banishedTickSpin + (1.0F - grow) * 260.0F + fi * 30.0F) * DEG2RAD;
        const Vector2 d{.x = std::cos(a), .y = std::sin(a)};
        const float length = r * (0.1F + 0.2F * hashNoise(fi * 3.1F + 1.0F)) * (broken ? 0.4F : 1.0F);
        const float width = r * (0.05F + 0.09F * hashNoise(fi * 5.7F + 2.0F));
        const Color shade = ColorLerp(bone, dim, 0.75F * hashNoise(fi * 7.3F + 3.0F));
        DrawLineEx(Vector2Add(center, Vector2Scale(d, r * 0.84F * spread)),
                   Vector2Add(center, Vector2Scale(d, (r * 0.84F + length) * spread)), width,
                   Fade(broken ? ColorLerp(dim, banishedVoid, 0.4F) : shade, grow));
    }
    DrawCircleV(center, r * 0.78F * grow, flesh);
    DrawRing(center, r * 0.6F * grow, r * 0.63F * grow, 0, 360, 48, Fade(dim, grow));
    if (eyeGlow > 0)
    {
        DrawCircleV(center, r * 0.36F * grow, BLACK); // the eye is out
        return;
    }
    // The eclipsed eye faces the ship: the sliver shows on the ship's side.
    const Vector2 toward{.x = std::cos(facing), .y = std::sin(facing)};
    DrawCircleV(center, r * 0.36F * grow, Palette::Crit);
    DrawCircleV(Vector2Subtract(center, Vector2Scale(toward, r * 0.07F)), r * 0.37F * grow, flesh);
}

// A tear the size of a tentacle: black, inside a turning broken ring of ash bone.
void drawBanishedPortal(Vector2 position, float progress)
{
    const float radius = 34.0F * std::clamp(progress, 0.0F, 1.0F);
    if (radius < 1.0F)
    {
        return;
    }
    const auto t = static_cast<float>(GetTime());
    DrawCircleV(position, radius, BLACK);
    plateRing(position, radius, radius + 6.0F, 3, t * -90.0F, 34.0F, banishedBone);
    plateRing(position, radius * 0.5F, radius * 0.5F + 3.0F, 2, t * 140.0F, 70.0F, Fade(Palette::Crit, 0.6F));
}

// A Banished tentacle is always the same seventeen points, from the same root. Resting, they
// wave free; attacking, they take the attack's shape; and between the two they are blended
// point by point, so a tentacle never vanishes or reappears: it wakes, strikes and settles.
using BanishedArm = std::array<Vector2, 17>;

static void drawBanishedArm(const BanishedArm& pts, float rootWidth, float tipWidth, Color color)
{
    drawChain(pts, rootWidth, tipWidth, color, ColorLerp(color, banishedVoid, 0.45F));
}

// `shape` eased toward `rest` by 1 - k (k 1: all shape).
static auto blendBanishedArm(const BanishedArm* rest, BanishedArm shape, float k) -> BanishedArm
{
    if (rest != nullptr && k < 1.0F)
    {
        const float e = k * k * (3.0F - 2.0F * k);
        for (size_t i = 0; i < shape.size(); i++)
        {
            shape.at(i) = Vector2Lerp(rest->at(i), shape.at(i), e);
        }
    }
    return shape;
}

// A free-waving arm: each link turns a little further from `angle`, the sway growing toward the
// tip. No two keep the same time: each has its own tempo and reach of sway, and each drifts
// between moving and hanging almost still, on its own slow clock.
static auto banishedFreeArm(Vector2 root, float angle, float length, float seed) -> BanishedArm
{
    const auto t = static_cast<float>(GetTime());
    const float tempo = 0.5F + 2.0F * hashNoise(seed * 1.3F + 5.0F);
    const float lively = std::clamp(std::sin(t * (0.15F + 0.3F * hashNoise(seed * 2.7F)) + seed) * 2.0F + 0.6F, 0.0F, 1.0F);
    const float sway = 0.05F + (0.25F + 0.5F * hashNoise(seed * 4.1F + 9.0F)) * lively;
    BanishedArm pts{};
    pts.front() = root;
    const auto links = static_cast<float>(pts.size() - 1);
    for (size_t k = 1; k < pts.size(); k++)
    {
        const float u = static_cast<float>(k - 1) / links;
        const float a = angle + sway * std::sin(t * tempo + seed * 1.9F + u * 4.2F) * (0.3F + u);
        pts.at(k) = Vector2Add(pts.at(k - 1), Vector2{.x = std::cos(a) * length / links, .y = std::sin(a) * length / links});
    }
    return pts;
}

// Tentacle `slot` at rest: a long waving chain from under the disc. `reach` scales its length.
static auto banishedRestArm(Vector2 center, float size, float instanceSeed, int32_t slot, float reach) -> BanishedArm
{
    const float angle = banishedArmAngle(slot);
    const float root = size * banishedFaceScale * 0.5F; // under the disc: draw this before the body
    return banishedFreeArm(Vector2Add(center, Vector2{.x = std::cos(angle) * root, .y = std::sin(angle) * root}), angle,
                           size * 0.88F * reach, instanceSeed * 7.0F + static_cast<float>(slot) * 13.0F);
}

// A resting tentacle (the entry cutscene overshoots `reach`). Shared so the cutscene ends on the
// fight's first frame.
void drawBanishedStub(Vector2 center, float size, float instanceSeed, int32_t slot, float reach, Color color)
{
    if (reach * size * 0.88F < size * 0.04F)
    {
        return;
    }
    drawBanishedArm(banishedRestArm(center, size, instanceSeed, slot, reach), size * 0.04F, size * 0.012F, color);
}

// The tip of an attacking tentacle: a spear point, or a claw of three fingers (open 0..1).
static void banishedTip(Vector2 at, float angle, float open, Color color, bool claw, float scale)
{
    scale *= 1.5F; // big enough to read as the business end
    const Vector2 d{.x = std::cos(angle), .y = std::sin(angle)};
    const Vector2 n{.x = -d.y, .y = d.x};
    if (!claw)
    {
        flatTriangle(Vector2Add(at, Vector2Scale(d, 30.0F * scale)), Vector2Add(at, Vector2Scale(n, 9.0F * scale)),
                     Vector2Subtract(at, Vector2Scale(n, 9.0F * scale)), color);
        flatTriangle(Vector2Subtract(at, Vector2Scale(d, 12.0F * scale)), Vector2Add(at, Vector2Scale(n, 9.0F * scale)),
                     Vector2Subtract(at, Vector2Scale(n, 9.0F * scale)), color);
        return;
    }
    for (int32_t k = -1; k <= 1; k++)
    {
        const float fa = angle + static_cast<float>(k) * (0.2F + 0.75F * open);
        const Vector2 fd{.x = std::cos(fa), .y = std::sin(fa)};
        const Vector2 fn{.x = -fd.y, .y = fd.x};
        flatTriangle(Vector2Add(at, Vector2Scale(fn, 6.0F * scale)), Vector2Subtract(at, Vector2Scale(fn, 6.0F * scale)),
                     Vector2Add(at, Vector2Scale(fd, 28.0F * scale)), color);
    }
    DrawPoly(at, 4, 10.0F * scale, angle * RAD2DEG, color);
}

// The points of a tentacle from a to b. `bend` bows it sideways and `wave` ripples it; both
// vanish at the ends, so the tip is exactly where it hits.
static auto banishedReachArm(Vector2 a, Vector2 b, float bend, float wave, float seed) -> BanishedArm
{
    const auto t = static_cast<float>(GetTime());
    const float length = std::max(Vector2Distance(a, b), 0.001F);
    const Vector2 dir = Vector2Scale(Vector2Subtract(b, a), 1.0F / length);
    const Vector2 perp{.x = -dir.y, .y = dir.x};
    BanishedArm pts{};
    for (size_t i = 0; i < pts.size(); i++)
    {
        const float f = static_cast<float>(i) / static_cast<float>(pts.size() - 1);
        const float off = bend * std::sin(std::numbers::pi_v<float> * f) +
                          wave * std::sin(t * 7.0F - f * 9.0F + seed) * 4.0F * f * (1.0F - f);
        pts.at(i) = Vector2Add(Vector2Lerp(a, b, f), Vector2Scale(perp, off));
    }
    return pts;
}

static auto banishedArmHeading(const BanishedArm& pts) -> float
{
    const Vector2 last = Vector2Subtract(pts.back(), pts.at(pts.size() - 2));
    return std::atan2(last.y, last.x);
}

// Draws a tentacle from a to b and returns the heading at its tip (the toss and the bat use this).
static auto banishedArm(Vector2 a, Vector2 b, float bend, float wave, float seed, float scale) -> float
{
    if (Vector2Distance(a, b) < 2.0F)
    {
        return 0.0F;
    }
    const BanishedArm pts = banishedReachArm(a, b, bend, wave, seed);
    drawBanishedArm(pts, 24.0F * scale, 9.0F * scale, banishedTendril);
    return banishedArmHeading(pts);
}

// An attacking tentacle takes a stance and aims, then goes. The pierce draws back in a tight S
// with a spear tip, shows its line once the aim freezes, and thrusts dead straight. The grab sways
// wide with a claw that opens as it readies, and lunges in a curve after the ship. The hurl leans
// back holding an enemy it pulled through, and whips forward to throw. The big grab is a grab at
// 1.7 times the size that aims for as long as it likes and flashes before it lunges.
//
// `rest` is the tentacle's own resting pose. From the body (no portal) the attack is that same
// tentacle: it eases out of `rest` into the stance and back into `rest` afterwards. Through a
// portal, this draws the far end coming out by the ship, growing out of the portal and hauled
// back into it; the near end, plunged into the entry portal, is drawBanishedRestingArms' part.
static void drawBanishedAttack(const Boss& boss, int32_t slot, const BanishedArm* rest)
{
    const auto idx = static_cast<size_t>(slot);
    const int32_t phase = boss.banishedPhase.at(idx);
    const auto t = static_cast<float>(GetTime());
    const float seed = static_cast<float>(boss.instanceId) * 7.0F + static_cast<float>(slot) * 13.0F;
    const bool big = boss.banishedAttackKind.at(idx) == 4;
    const bool hurl = boss.banishedAttackKind.at(idx) == 3;
    const bool grab = boss.banishedAttackKind.at(idx) == 2 || hurl || big; // the hurl holds its enemy in the same claw
    const bool portalled = boss.banishedIsFar.at(idx);
    const float scale = big ? 1.7F : 1.0F;
    const Vector2 anchor = boss.banishedAnchor.at(idx);
    const Vector2 tip = boss.banishedTip.at(idx);
    const float timer = boss.banishedTimer.at(idx);
    const float aim = boss.banishedAimAngle.at(idx);
    const Vector2 aimDir{.x = std::cos(aim), .y = std::sin(aim)};
    // Draws the tentacle and its tip; `k` is how far it has left its resting pose (body attacks).
    const auto show = [&](const BanishedArm& shape, float k, float open, Color tipColor)
    {
        const BanishedArm pts = blendBanishedArm(rest, shape, k);
        drawBanishedArm(pts, 24.0F * scale, 9.0F * scale, banishedTendril);
        banishedTip(pts.back(), banishedArmHeading(pts), open, Fade(tipColor, rest != nullptr ? k : 1.0F), grab, scale);
        return pts.back();
    };

    if (phase == 6)
    {
        // the coil: the tentacle to the ship, rings of ash bone turning tight round it
        drawBanishedPortal(anchor, portalled ? 1.0F : 0.0F);
        drawBanishedArm(banishedReachArm(anchor, tip, 0, 10.0F, seed), 24.0F * scale, 9.0F * scale, banishedTendril);
        plateRing(tip, 22.0F, 28.0F, 3, t * 240.0F, 30.0F, Palette::Crit);
        plateRing(tip, 31.0F, 34.0F, 2, t * -160.0F, 80.0F, banishedBone);
        return;
    }
    if (phase == 5)
    {
        drawBanishedPortal(anchor, 1.0F);
        return;
    }
    if (phase == 1)
    {
        // out: how far it has come out of its portal. wake: how far it has left its resting pose.
        // lock: 0 while aiming, rising to 1 as it is about to go.
        const float progress = std::clamp(1.0F - timer / banishedTentacleWindup, 0.0F, 1.0F);
        const float grown = big ? std::clamp((banishedBigAimMax - timer) / 0.5F, 0.0F, 1.0F)
                                : std::clamp((progress - 0.15F) / 0.3F, 0.0F, 1.0F);
        const float out = portalled ? grown : 1.0F;
        const float wake = std::clamp(progress / 0.3F, 0.0F, 1.0F);
        const float lock = std::clamp(1.0F - timer / (big ? banishedBigTell : banishedAimLock), 0.0F, 1.0F);
        drawBanishedPortal(anchor, portalled ? std::min(1.0F, big ? grown * 2.0F : progress * 5.0F) : 0.0F);
        if (out <= 0)
        {
            return;
        }
        const Vector2 side{.x = -aimDir.y, .y = aimDir.x};
        if (hurl)
        {
            // leaning back, the enemy held out behind; it leans further as the aim freezes
            const float lean = (slot % 2) == 0 ? 1.0F : -1.0F;
            const Vector2 held = Vector2Add(anchor, Vector2Add(Vector2Scale(aimDir, (-50.0F - 50.0F * lock) * out),
                                                                Vector2Scale(side, lean * 100.0F * out)));
            const Vector2 hand = show(banishedReachArm(anchor, held, lean * 45.0F * out, 4.0F, seed), wake, 0.5F, Palette::Crit);
            drawBanishedThrowable(hand, boss.banishedHeldKind.at(idx), out * wake, aim * RAD2DEG, Vector2{});
            if (lock > 0)
            {
                DrawLineEx(anchor, Vector2Add(anchor, Vector2Scale(aimDir, 420.0F)), 2.0F, Fade(Palette::Crit, 0.15F + 0.4F * lock));
            }
            return;
        }
        const float reach = 150.0F * scale * out * (grab ? 1.0F : 1.0F - 0.3F * lock); // the pierce draws back
        BanishedArm pts{};
        for (size_t i = 0; i < pts.size(); i++)
        {
            const float f = static_cast<float>(i) / static_cast<float>(pts.size() - 1);
            // an S along the aim; the grab also sways from side to side, the pierce holds still and trembles
            const float off =
                grab ? 26.0F * std::sin(2.0F * std::numbers::pi_v<float> * f) +
                           60.0F * (1.0F - lock) * std::sin(t * 3.0F + seed) *
                               std::sin(std::numbers::pi_v<float> * f)
                     : (48.0F + 14.0F * lock) * std::sin(2.0F * std::numbers::pi_v<float> * f) +
                           3.0F * lock * std::sin(t * 55.0F);
            pts.at(i) = Vector2Add(anchor, Vector2Add(Vector2Scale(aimDir, reach * f), Vector2Scale(side, off * out * scale)));
        }
        const Color hot = ColorLerp(Palette::Crit, WHITE, lock * (0.5F + 0.5F * std::sin(t * 40.0F)));
        const Vector2 point = show(pts, wake, grab ? 0.25F + 0.75F * lock : 0.0F, hot);
        if (!grab && lock > 0)
        {
            // the line it will take
            DrawLineEx(point, Vector2Add(anchor, Vector2Scale(aimDir, boss.banishedAimReach.at(idx))), 2.0F,
                       Fade(Palette::Crit, 0.2F + 0.5F * lock));
        }
        if (big)
        {
            // the big one wears a turning ring; it closes and brightens as it is about to lunge
            const float r = 60.0F - 22.0F * lock;
            plateRing(point, r, r + 6.0F, 3, t * (120.0F + 400.0F * lock), 40.0F, Fade(Palette::Crit, 0.45F + 0.55F * lock));
        }
        return;
    }

    Vector2 end = tip;
    float bend = 0;
    float wave = 0;
    float portal = 1.0F;
    float open = 1.0F;
    float settle = 1.0F; // 1 striking .. 0 back in its resting pose
    const Vector2 thrown = Vector2Add(anchor, Vector2Scale(aimDir, banishedHurlRelease)); // a hurl's follow-through
    if (hurl)
    {
        end = thrown;
        wave = 5.0F;
    }
    if (phase == 2 && grab && !hurl)
    {
        const float progress = std::clamp(1.0F - timer / (big ? banishedBigLunge : banishedTentacleStrike), 0.0F, 1.0F);
        bend = Vector2Distance(anchor, tip) * 0.3F * (1.0F - progress) * ((slot % 2) == 0 ? 1.0F : -1.0F);
        wave = 8.0F;
    }
    else if (phase == 3)
    {
        const float pulled = std::clamp(1.0F - timer / banishedTentacleRetreat, 0.0F, 1.0F);
        wave = 16.0F;
        open = 0.0F;
        if (portalled)
        {
            end = Vector2Lerp(end, anchor, pulled * pulled); // hauled back into its portal
            portal = 1.0F - pulled * pulled;
        }
        else
        {
            settle = 1.0F - pulled; // from the body it does not shrink away: it settles back to rest
        }
    }
    drawBanishedPortal(anchor, portalled ? portal : 0.0F);
    if (Vector2Distance(anchor, end) >= 2.0F)
    {
        show(banishedReachArm(anchor, end, bend, wave, seed), settle, open,
             phase == 2 ? ColorLerp(Palette::Crit, WHITE, 0.5F) : Palette::Crit);
    }
}

// An attacking tentacle, coming out of its portal by the ship.
void drawBanishedTentacle(const Boss& boss, int32_t slot)
{
    const auto idx = static_cast<size_t>(slot);
    if (!boss.banishedStaggered.at(idx) && boss.banishedPhase.at(idx) != 0)
    {
        drawBanishedAttack(boss, slot, nullptr);
    }
}

// The body's tentacles (it only has its body at the start and in the finale), drawn before the
// body so they come out from behind it; paralysed ones hang limp, drained to dull bone.
void drawBanishedRestingArms(Vector2 center, const Boss& boss)
{
    const auto instanceSeed = static_cast<float>(boss.instanceId);
    for (int32_t slot = 0; slot < Boss::banishedTentacleCount; slot++)
    {
        const auto idx = static_cast<size_t>(slot);
        const float hit = std::clamp(boss.banishedTentacleHitFlash.at(idx) / UpdateConstants::hitFlashDuration, 0.0F, 1.0F);
        const bool limp = boss.banishedStaggered.at(idx);
        drawBanishedStub(center, boss.size.x, instanceSeed, slot, (limp ? 0.5F : 1.0F) * boss.banishedReveal,
                         limp ? ColorLerp(ColorLerp(banishedBone, banishedVoid, 0.6F), WHITE, hit) : banishedTendril);
    }
}

// The toss and the bat (Juggled): the tentacle holding the ship leans back with it and throws;
// the thrower follows through while a catcher reaches out at the far end; after the last throw
// three tentacles braid into one fat bat that curls back and swings.
void drawBanishedToss(const Boss& boss, Vector2 ship)
{
    if (boss.banishedTossPhase == 0)
    {
        return;
    }
    const auto t = static_cast<float>(GetTime());
    const Vector2 dir{.x = std::cos(boss.banishedTossAngle), .y = std::sin(boss.banishedTossAngle)};
    const Vector2 anchor = boss.banishedTossAnchor;
    if (boss.banishedTossPhase == 1)
    {
        drawBanishedPortal(anchor, 1.0F);
        const float heading = banishedArm(anchor, ship, 40.0F, 5.0F, 1.0F, 1.7F);
        banishedTip(ship, heading, 0.0F, Palette::Crit, true, 1.7F);
        plateRing(ship, 24.0F, 30.0F, 3, t * 240.0F, 30.0F, Palette::Crit);
        return;
    }
    if (boss.banishedTossPhase == 2)
    {
        const float p = std::clamp(1.0F - boss.banishedTossTimer / banishedTossFlight, 0.0F, 1.0F);
        // the thrower follows through and is drawn back in
        drawBanishedPortal(anchor, 1.0F - p);
        const Vector2 swung = Vector2Add(anchor, Vector2Scale(dir, 280.0F * (1.0F - p)));
        banishedTip(swung, banishedArm(anchor, swung, 0, 8.0F, 1.0F, 1.7F), 1.0F, Palette::Crit, true, 1.7F);
        // the ship's streak
        for (const float off : {-9.0F, 0.0F, 9.0F})
        {
            const Vector2 n{.x = -dir.y * off, .y = dir.x * off};
            DrawLineEx(Vector2Add(ship, n), Vector2Add(Vector2Subtract(ship, Vector2Scale(dir, 110.0F * (1.0F - p))), n), 2.0F,
                       Fade(banishedBone, 0.35F));
        }
        if (boss.banishedTossLeft > 1)
        {
            // the catcher reaches back up the ship's path
            const Vector2 next = boss.banishedTossNextAnchor;
            drawBanishedPortal(next, std::min(1.0F, p * 3.0F));
            const Vector2 hand = Vector2Lerp(next, boss.banishedTossTo, p);
            banishedTip(hand, banishedArm(next, hand, 30.0F * (1.0F - p), 6.0F, 2.0F, 1.7F), 1.0F - 0.6F * p, Palette::Crit, true, 1.7F);
        }
        return;
    }

    // The bat. swing: the heading it bends toward, curled back then whipped through. fold: extra curl.
    float swing = -2.1F;
    float fold = -1.3F;
    if (boss.banishedTossPhase == 3)
    {
        const float w = std::clamp(1.0F - boss.banishedTossTimer / banishedBatWindup, 0.0F, 1.0F);
        const float eased = w * (2.0F - w);
        swing *= eased;
        fold *= eased;
    }
    else if (boss.banishedTossPhase == 4)
    {
        const float through = std::clamp(1.0F - boss.banishedTossTimer / banishedBatSwing, 0.0F, 1.0F);
        swing = -2.1F + 2.6F * through;
        fold = -1.3F * (1.0F - through);
    }
    const bool limp = boss.banishedTossPhase == 5; // paralysed mid-curl, the ship shield-dashing through it
    const float burst = limp ? std::clamp(1.0F - boss.banishedTossTimer / banishedBatBreak, 0.0F, 1.0F) : 0.0F;
    const float aim = std::atan2(ship.y - anchor.y, ship.x - anchor.x);
    const float length = banishedBatLength;
    drawBanishedPortal(anchor, 1.0F);
    Vector2 club = anchor;
    for (int32_t k = 0; k < 3; k++)
    {
        std::array<Vector2, 17> pts{};
        Vector2 at = anchor;
        for (size_t i = 0; i < pts.size(); i++)
        {
            const float f = static_cast<float>(i) / static_cast<float>(pts.size() - 1);
            const float heading = aim + swing * (0.35F + 0.65F * f) + fold * f * f;
            const Vector2 n{.x = -std::sin(heading), .y = std::cos(heading)};
            // three strands wound round one line
            // (struck, the braid bursts apart: each strand is thrown off its own way)
            const float wind = 11.0F * std::sin(f * 9.0F + static_cast<float>(k) * 2.094F + t * (limp ? 30.0F : 4.0F)) * (1.0F - f * 0.5F) +
                               (static_cast<float>(k) - 1.0F) * 130.0F * burst * f;
            pts.at(i) = Vector2Add(at, Vector2Scale(n, wind));
            at = Vector2Add(at, Vector2{.x = std::cos(heading) * length / 16.0F, .y = std::sin(heading) * length / 16.0F});
        }
        const Color strand = limp ? ColorLerp(banishedBone, banishedVoid, 0.5F) : ColorLerp(banishedTendril, banishedBone, 0.12F * static_cast<float>(k));
        drawChain(pts, 30.0F, 16.0F, strand, ColorLerp(strand, banishedVoid, 0.45F));
        club = at;
    }
    if (!limp)
    {
        DrawPoly(club, 4, 24.0F, t * 90.0F, Palette::Crit);
    }
}

// A Doom Gaze in progress, from the finale or from the tentacle stage: charging (track 0..1 while
// the eye follows the ship, then lock 0..1 as the line freezes) or firing (life 0..1).
struct BanishedGaze
{
    bool charging;
    bool firing;
    float track;
    float lock;
    float life;
};
static auto banishedGaze(const Boss& boss) -> BanishedGaze
{
    const bool finale = boss.banishedStage == 1;
    const float timer = finale ? boss.banishedEyeTimer : boss.banishedGazeTimer;
    const bool charging = finale ? boss.banishedFinaleMove == 3 : boss.banishedGazePhase == 1;
    const bool firing = finale ? boss.banishedFinaleMove == 4 : boss.banishedGazePhase == 2;
    return BanishedGaze{.charging = charging,
                        .firing = firing,
                        .track = std::clamp(1.0F - (timer - banishedGazeLock) / banishedGazeCharge, 0.0F, 1.0F),
                        .lock = std::clamp(1.0F - timer / banishedGazeLock, 0.0F, 1.0F),
                        .life = std::clamp(1.0F - timer / banishedGazeFire, 0.0F, 1.0F)};
}

// The eye takes the size of what it is in: its body, or the background ring it looks out of.
static auto banishedEyeRadius(const Boss& boss) -> float
{
    return boss.banishedInBody ? banishedFaceRadius * 0.36F
                               : banishedSeals.at(static_cast<size_t>(std::max(boss.banishedSeal, 0))).radius * 0.3F;
}

// The eye is open for a Doom Gaze, and on the one dash that can be rammed.
static auto banishedEyeOpen(const Boss& boss) -> bool
{
    const BanishedGaze gaze = banishedGaze(boss);
    return gaze.charging || gaze.firing ||
           (boss.banishedStage == 1 && boss.banishedFinaleEyeDash && boss.banishedFinaleMove != 0);
}

// Tentacle stage: no body. The background ring it is in, lit, with twelve marks round it (one per
// tentacle, dark when paralysed), and its open eye looking out through it at the ship.
static void drawBanishedRingEye(Vector2 center, const Boss& boss, Vector2 ship)
{
    const BanishedSeal& seal = banishedSeals.at(static_cast<size_t>(std::max(boss.banishedSeal, 0)));
    const auto t = static_cast<float>(GetTime());
    const float there = std::clamp(boss.banishedReveal, 0.0F, 1.0F);
    const float r = seal.radius;
    plateRing(center, r * 0.88F, r, seal.plates, t * seal.spin, seal.gap, Fade(banishedBone, 0.5F * there));
    plateRing(center, r * 0.66F, r * 0.72F, seal.plates + 1, t * -seal.spin * 1.6F, seal.gap, Fade(banishedBone, 0.32F * there));
    for (int32_t i = 0; i < Boss::banishedTentacleCount; i++)
    {
        const bool live = !boss.banishedStaggered.at(static_cast<size_t>(i));
        const float a = (t * banishedTickSpin + static_cast<float>(i) * 30.0F) * DEG2RAD;
        const Vector2 d{.x = std::cos(a), .y = std::sin(a)};
        DrawLineEx(Vector2Add(center, Vector2Scale(d, r * 1.07F)), Vector2Add(center, Vector2Scale(d, r * (live ? 1.2F : 1.12F))),
                   std::max(2.0F, r * 0.035F), Fade(live ? banishedBone : ColorLerp(banishedBone, banishedVoid, 0.7F), there));
    }
    // Arriving: the inside of the ring goes dark, an eye opens in the darkness, and then the dark
    // takes on its body colour. Leaving runs the same thing backwards.
    const float dark = std::clamp(there / 0.3F, 0.0F, 1.0F);
    const float lid = std::clamp((there - 0.15F) / 0.45F, 0.0F, 1.0F);
    const float tint = std::clamp((there - 0.6F) / 0.4F, 0.0F, 1.0F);
    DrawCircleV(center, r * 0.62F, Fade(ColorLerp(BLACK, banishedFlesh, tint), dark));
    const float eye = banishedEyeRadius(boss);
    if (lid <= 0)
    {
        return;
    }
    const Vector2 toward = Vector2Distance(ship, center) > 1.0F ? Vector2Normalize(Vector2Subtract(ship, center)) : Vector2{.x = 0, .y = 1};
    DrawEllipse(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y), eye, eye * lid,
                ColorLerp(Palette::Crit, Palette::Void, 0.12F));
    if (lid >= 1.0F)
    {
        plateRing(center, eye * 0.64F, eye * 0.84F, 5, t * 60.0F, 16.0F, ColorLerp(Palette::Crit, Palette::Void, 0.45F));
    }
    DrawCircleV(Vector2Add(center, Vector2Scale(toward, eye * 0.3F * lid)), std::min(eye * 0.4F, eye * lid * 0.85F), BLACK);
}

void drawBanishedShape(Vector2 center, const Boss& boss, Vector2 ship)
{
    if (!boss.banishedInBody)
    {
        drawBanishedRingEye(center, boss, ship);
        return;
    }
    drawBanishedRestingArms(center, boss);
    uint32_t broken = 0;
    for (int32_t i = 0; i < Boss::banishedTentacleCount; i++)
    {
        broken |= boss.banishedStaggered.at(static_cast<size_t>(i)) ? 1U << static_cast<uint32_t>(i) : 0U;
    }
    drawBanishedBody(center, boss.size.x, static_cast<float>(boss.instanceId), boss.banishedReveal, broken,
                     banishedEyeOpen(boss) ? 1.0F : 0.0F,
                     std::clamp(boss.hitFlashTimer / UpdateConstants::hitFlashDuration, 0.0F, 1.0F),
                     std::atan2(ship.y - center.y, ship.x - center.x));
}

// Energy gathering before a beam: broken rings close in on `at`, motes are drawn in along
// spirals, and a core swells. As the aim freezes (`lock`) the rings spin up and the core whitens.
void drawBeamCharge(Vector2 at, float radius, float progress, float lock, Color color)
{
    const auto t = static_cast<float>(GetTime());
    for (int32_t i = 0; i < 3; i++)
    {
        const auto fi = static_cast<float>(i);
        const float r = radius * (3.4F - 2.2F * progress) * (1.0F - 0.24F * fi);
        plateRing(at, r, r + radius * 0.14F, 3 + i, t * (70.0F + 40.0F * fi) * ((i % 2) != 0 ? -1.0F : 1.0F) * (1.0F + 3.0F * lock),
                  40.0F, Fade(color, 0.15F + 0.55F * progress));
    }
    for (int32_t k = 0; k < 14; k++)
    {
        const auto fk = static_cast<float>(k);
        const float u = std::fmod(t * 0.9F + fk / 14.0F + hashNoise(fk * 3.3F), 1.0F); // 0 far .. 1 arrived
        const float a = fk * 0.449F + u * 2.6F;
        const float dist = radius * (4.4F * (1.0F - u) + 0.5F);
        DrawPoly(Vector2Add(at, Vector2{.x = std::cos(a) * dist, .y = std::sin(a) * dist}), 4, radius * 0.12F * (0.4F + u), a * RAD2DEG,
                 Fade(ColorLerp(color, WHITE, 0.3F), u * progress));
    }
    DrawCircleV(at, radius * (0.25F + 0.5F * progress) * (1.0F + 0.08F * lock * std::sin(t * 40.0F)), ColorLerp(color, WHITE, 0.6F * lock));
}

// A beam from `from` to `to`: three flat bands, plates racing down it, ragged teeth flicking off
// its edges, and rings thrown off where it leaves. It snaps open, holds, and pinches shut.
void drawBeam(Vector2 from, Vector2 to, float width, float life, Color color)
{
    const auto t = static_cast<float>(GetTime());
    const float length = Vector2Distance(from, to);
    if (length < 1.0F)
    {
        return;
    }
    const float shape = life < 0 ? 1.0F : std::min(std::clamp(life / 0.12F, 0.0F, 1.0F), std::clamp((1.0F - life) / 0.25F, 0.0F, 1.0F));
    const float w = width * shape * (1.0F + 0.08F * std::sin(t * 45.0F));
    if (w < 0.5F)
    {
        return;
    }
    const Vector2 dir = Vector2Scale(Vector2Subtract(to, from), 1.0F / length);
    const Vector2 side{.x = -dir.y, .y = dir.x};
    const float heading = std::atan2(dir.y, dir.x) * RAD2DEG;
    const Color hot = ColorLerp(color, WHITE, 0.7F);
    DrawLineEx(from, to, w * 2.4F, Fade(color, 0.12F));
    DrawLineEx(from, to, w * 1.3F, Fade(color, 0.5F));
    DrawLineEx(from, to, w * 0.55F, hot);
    // plates racing down the core
    constexpr float spacing = 96.0F;
    const float plateStart = std::fmod(t * 1700.0F, spacing);
    for (int32_t di = 0; plateStart + static_cast<float>(di) * spacing < length; di++)
    {
        const float d = plateStart + static_cast<float>(di) * spacing;
        DrawPoly(Vector2Add(from, Vector2Scale(dir, d)), 4, w * 0.5F, heading, Fade(WHITE, 0.6F));
    }
    // teeth flicking off both edges
    const float frame = std::floor(t * 24.0F);
    int32_t n = 0;
    for (; 30.0F + static_cast<float>(n) * 58.0F < length; n++)
    {
        const float d = 30.0F + static_cast<float>(n) * 58.0F;
        const float sign = (n % 2) == 0 ? 1.0F : -1.0F;
        const float tall = w * (0.4F + 1.1F * hashNoise(frame * 0.37F + static_cast<float>(n) * 1.9F));
        const Vector2 base = Vector2Add(Vector2Add(from, Vector2Scale(dir, d)), Vector2Scale(side, sign * w * 0.6F));
        flatTriangle(Vector2Subtract(base, Vector2Scale(dir, 14.0F)), Vector2Add(base, Vector2Scale(dir, 14.0F)),
                     Vector2Add(Vector2Add(base, Vector2Scale(dir, 26.0F)), Vector2Scale(side, sign * tall)), Fade(color, 0.55F));
    }
    // rings thrown off the muzzle
    for (int32_t k = 0; k < 2; k++)
    {
        const float u = std::fmod(t * 2.6F + static_cast<float>(k) * 0.5F, 1.0F);
        const float r = w * (1.0F + 2.6F * u);
        plateRing(from, r, r + w * 0.25F, 4, t * 220.0F + static_cast<float>(k) * 45.0F, 30.0F, Fade(color, 1.0F - u));
    }
    DrawCircleV(from, w * 1.05F, hot);
}

// The lane a finale dash will take, the Doom Gaze (charging, aiming and firing), and the open
// eye (matte pale gold, a broken iris ring turning in it, the pupil on the ship).
void drawBanishedEye(const Boss& boss, Vector2 lookAt)
{
    const Vector2 pos{.x = boss.position.x + boss.size.x / 2, .y = boss.position.y + boss.size.y / 2};
    const auto t = static_cast<float>(GetTime());
    const int32_t move = boss.banishedStage == 1 ? boss.banishedFinaleMove : 0;
    const BanishedGaze gaze = banishedGaze(boss);
    const float radius = banishedEyeRadius(boss) * std::clamp(boss.banishedReveal, 0.0F, 1.0F);
    if (move == 1 || move == 2)
    {
        const Vector2 dir = Vector2Normalize(boss.chargeVelocity);
        const float left = move == 1 ? banishedDashTravel : std::max(0.0F, boss.banishedEyeTimer) * banishedDashSpeed;
        const Vector2 to = Vector2Add(pos, Vector2Scale(dir, left));
        const float tell = move == 1 ? 0.25F + 0.5F * (1.0F - boss.banishedEyeTimer / banishedDashTell) : 0.35F;
        const Color lane = boss.banishedFinaleEyeDash ? Palette::Crit : banishedBone;
        DrawLineEx(pos, to, banishedDiscRadius * 2.0F, Fade(lane, 0.05F));
        DrawLineEx(pos, to, 2.0F, Fade(lane, tell));
    }
    if (!banishedEyeOpen(boss))
    {
        return;
    }
    // the eye opens as it starts to charge
    const float opened = gaze.charging ? std::clamp(gaze.track / 0.25F, 0.0F, 1.0F) : 1.0F;
    DrawCircleV(pos, radius * opened, ColorLerp(Palette::Crit, Palette::Void, 0.12F));
    plateRing(pos, radius * 0.64F * opened, radius * 0.84F * opened, 5, t * (90.0F + 300.0F * gaze.lock), 16.0F,
              ColorLerp(Palette::Crit, Palette::Void, 0.45F));
    const Vector2 focus = gaze.charging || gaze.firing ? boss.targetPosition : lookAt;
    const Vector2 pupilDir = Vector2Distance(focus, pos) > 1.0F ? Vector2Normalize(Vector2Subtract(focus, pos)) : Vector2{.x = 0, .y = 1};
    const Vector2 pupil = Vector2Add(pos, Vector2Scale(pupilDir, radius * 0.3F * opened));
    DrawCircleV(pupil, radius * 0.4F * opened, BLACK);
    if (gaze.charging)
    {
        // The line it will take: dashes running out while it tracks, solid once it freezes.
        const Vector2 far = Vector2Add(pos, Vector2Scale(pupilDir, 2400.0F));
        if (gaze.lock > 0)
        {
            DrawLineEx(pos, far, 2.0F + 3.0F * gaze.lock, Fade(Palette::Crit, 0.5F + 0.4F * gaze.lock));
        }
        else
        {
            const float gazeStart = std::fmod(t * 600.0F, 60.0F);
            for (int32_t di = 0; gazeStart + static_cast<float>(di) * 60.0F < 2400.0F; di++)
            {
                const float d = gazeStart + static_cast<float>(di) * 60.0F;
                DrawLineEx(Vector2Add(pos, Vector2Scale(pupilDir, d)), Vector2Add(pos, Vector2Scale(pupilDir, d + 26.0F)), 2.0F,
                           Fade(Palette::Crit, 0.15F + 0.35F * gaze.track));
            }
        }
        drawBeamCharge(pupil, radius * 1.1F, gaze.track, gaze.lock, Palette::Crit); // the rings close in from outside the eye
    }
    else if (gaze.firing)
    {
        drawBeam(pupil, boss.targetPosition, 18.0F, gaze.life, Palette::Crit);
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
        const float a = spin + static_cast<float>(i) * 2.0F * std::numbers::pi_v<float> / sides;
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
        const auto k = static_cast<float>(i);
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
    const auto t = static_cast<float>(GetTime());
    const float radius = boss.size.x / 2;
    const auto seed = static_cast<float>(boss.instanceId);
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
        for (int32_t di = 0; static_cast<float>(di) * 34.0F < Vector2Distance(from, to); di++) // a dashed crack line
        {
            const float d = static_cast<float>(di) * 34.0F;
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
            float a = hashNoise(seed + kf * 2.9F) * 2.0F * std::numbers::pi_v<float> + t * 0.3F;
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
            const float a = hashNoise(seed + kf * 7.3F) * 2.0F * std::numbers::pi_v<float>;
            const Vector2 p = Vector2Add(center, Vector2Scale(Vector2{.x = std::cos(a), .y = std::sin(a)}, radius * (0.6F + 0.6F * phase)));
            DrawCircleV(p, radius * 0.06F * (1.0F - phase), Fade(basaltLit, 0.6F * (1.0F - phase) * u));
        }
    }
    // Blob balls thrown out of the pool where it launches and where it dives back in.
    const auto splash = [&](Vector2 where, float age)
    {
        const float u = age / 0.45F;
        if (u < 0 || u > 1)
        {
            return;
        }
        const Color rock = ColorLerp(Palette::SolarForgeAccent, Palette::Void, 0.45F);
        for (int32_t k = 0; k < 10; k++)
        {
            const float a = static_cast<float>(k) / 10.0F * 2.0F * std::numbers::pi_v<float> +
                            hashNoise(seed + static_cast<float>(k));
            const float far = radius * (0.6F + 1.3F * u * (2.0F - u));
            DrawCircleV(Vector2Add(where, Vector2{.x = std::cos(a) * far, .y = std::sin(a) * far}), 22.0F * (1.0F - u),
                        (k % 3) == 0 ? molten : rock);
        }
    };
    if (boss.moleHidden && boss.moleRumble <= 0)
    {
        splash(center, moleHideTime - boss.moleHideTimer);
    }
    if (seen <= 0.01F)
    {
        return;
    }

    // How far the flower head stands open: bloomed for attacks, locked open once it blooms.
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
    if (swallowing || boss.slagmawPunishTimer > 0)
    {
        open = 1.0F; // jaws wide; choking, the core is bared
    }
    float heat = std::max(boss.slagmawBloomed ? 0.7F : 0.2F, open);
    if (boss.slagmawPunishTimer > 0)
    {
        heat = 0.4F + 0.4F * std::sin(t * 9.0F);
    }

    // Flat matte mole: round body, two digging claws, a shovel rump with a hot edge, and a bud of
    // six iron petals for a head. It grows up out of the magma, the background blobs parting
    // round it, and shrinks back in to dive; a pounce is a leap over its shadow.
    constexpr Color hide{.r = 58, .g = 46, .b = 48, .a = 255};
    constexpr Color hideLite{.r = 92, .g = 76, .b = 76, .a = 255};
    constexpr Color clawBone{.r = 150, .g = 132, .b = 120, .a = 255};
    constexpr Color petalHot{.r = 150, .g = 70, .b = 34, .a = 255};
    const bool flying = shooting && boss.attack == BossAttack::ChargeDash;
    const float R = radius * 0.85F;
    float scale = 1.0F - boss.moleUnder;
    float reach = 0.9F; // claws hooked over the blob's rim
    Vector2 at = center;
    if (swallowing)
    {
        at = Vector2Add(at, Vector2{.x = std::sin(t * 37.0F) * 6.0F, .y = std::cos(t * 29.0F) * 6.0F});
    }
    if (flying && boss.moleLeapTotal > 0)
    {
        // The leap: up and over, larger at the top, its shadow shrinking on the ground below.
        const float p = std::clamp(1.0F - boss.stateTimer / boss.moleLeapTotal, 0.0F, 1.0F);
        const float h = std::sin(std::numbers::pi_v<float> * p);
        scale *= 1.0F + 0.35F * h;
        reach = std::cos(2.0F * std::numbers::pi_v<float> *
                         p); // claws reach, sweep back, reach again to land
        DrawCircleV(center, R * 0.9F * (1.0F - 0.35F * h), Fade(BLACK, 0.5F * seen));
        at.y -= R * 0.9F * h;
        splash(boss.moleLeapFrom, p * boss.moleLeapTotal);
    }
    const Color hot = paint(ColorLerp(molten, gold, heat), 1.0F);
    rlPushMatrix();
    rlTranslatef(at.x, at.y, 0);
    rlRotatef(std::atan2(face.y, face.x) * RAD2DEG, 0, 0, 1);
    rlScalef(scale, scale, 1);
    rlTranslatef(-R * 0.25F, 0, 0); // +x is forward from here on
    for (const float side : {1.0F, -1.0F}) // claws
    {
        const Vector2 shoulder{.x = R * 0.35F, .y = side * R * 0.8F};
        const float a = side * (95.0F - 60.0F * reach) * DEG2RAD;
        const Vector2 dir{.x = std::cos(a), .y = std::sin(a)};
        const Vector2 paw = Vector2Add(shoulder, Vector2Scale(dir, R * 0.7F));
        DrawLineEx(shoulder, paw, R * 0.5F, paint(basalt, 1.0F));
        DrawCircleV(shoulder, R * 0.25F, paint(basalt, 1.0F));
        for (int32_t k = -1; k <= 1; k++)
        {
            const Vector2 d = Vector2Rotate(dir, static_cast<float>(k) * 0.5F);
            const Vector2 n{.x = -d.y, .y = d.x};
            const Vector2 base = Vector2Add(paw, Vector2Scale(d, R * 0.1F));
            flatTriangle(Vector2Add(base, Vector2Scale(n, R * 0.13F)), Vector2Subtract(base, Vector2Scale(n, R * 0.13F)),
                         Vector2Add(paw, Vector2Scale(d, R * 0.8F)), paint(clawBone, 1.0F));
        }
        DrawCircleV(paw, R * 0.34F, paint(hide, 1.0F));
    }
    DrawCircleV(Vector2{.x = -R * 0.05F, .y = 0}, R * 0.95F, paint(hide, 1.0F)); // body
    { // shovel rump, wide hot edge at the rear
        const Vector2 a{.x = 0, .y = -R * 0.8F};
        const Vector2 b{.x = 0, .y = R * 0.8F};
        const Vector2 c{.x = -R * 0.9F, .y = R * 1.3F};
        const Vector2 d{.x = -R * 0.9F, .y = -R * 1.3F};
        const Vector2 n{.x = -R * 1.17F, .y = 0};
        flatTriangle(a, b, c, paint(hideLite, 1.0F));
        flatTriangle(a, c, d, paint(hideLite, 1.0F));
        flatTriangle(d, c, n, paint(hideLite, 1.0F));
        DrawLineEx(d, n, R * (0.09F + 0.07F * open), hot);
        DrawLineEx(c, n, R * (0.09F + 0.07F * open), hot);
        flatTriangle(Vector2{.x = -R * 0.15F, .y = -R * 0.45F}, Vector2{.x = -R * 0.15F, .y = R * 0.45F},
                     Vector2{.x = -R * 0.75F, .y = 0}, paint(hide, 1.0F));
    }
    {
        // Flower head: closed, the petals meet in a point over the head; open, they stand out
        // all round a molten core.
        const Vector2 head{.x = R * 0.9F, .y = 0};
        DrawCircleV(head, R * 0.6F, paint(basalt, 1.0F));
        DrawCircleV(head, R * 0.44F * open, hot);
        const Vector2 apex = Vector2Add(head, Vector2{.x = R * 1.35F, .y = 0});
        for (const float deg : {130.0F, -130.0F, 75.0F, -75.0F, 25.0F, -25.0F})
        {
            const Vector2 out{.x = std::cos(deg * DEG2RAD), .y = std::sin(deg * DEG2RAD)};
            const Vector2 root = Vector2Add(head, Vector2Scale(out, R * 0.5F));
            const Vector2 tip = Vector2Lerp(apex, Vector2Add(root, Vector2Scale(out, R)), open);
            const Vector2 d = Vector2Normalize(Vector2Subtract(tip, root));
            const Vector2 n{.x = -d.y, .y = d.x};
            const Vector2 mid = Vector2Lerp(root, tip, 0.4F);
            const Vector2 l = Vector2Add(mid, Vector2Scale(n, R * 0.3F));
            const Vector2 r = Vector2Subtract(mid, Vector2Scale(n, R * 0.3F));
            const Color c = paint(ColorLerp(std::abs(deg) == 75.0F ? hide : hideLite, petalHot, open * 0.75F), 1.0F);
            flatTriangle(root, l, r, c);
            flatTriangle(l, r, tip, c);
            DrawLineEx(root, Vector2Lerp(root, tip, 0.8F), R * 0.05F, paint(ColorLerp(basalt, gold, open), 1.0F));
        }
    }
    rlPopMatrix();
    if (swallowing && game.run.qtePrompt != QtePrompt::None && game.run.qtePrompt != QtePrompt::Mash &&
        game.run.qteTimerMax > 0)
    {
        const float left = std::clamp(game.run.qteTimer / game.run.qteTimerMax, 0.0F, 1.0F);
        const Vector2 gob = Vector2Subtract(game.run.player.position, Vector2Scale(game.run.qteThreatDir, 240.0F * left));
        drawSlagGob(gob, 13.0F, game.run.qteThreatDir);
    }
}

} // namespace DrawDetail
