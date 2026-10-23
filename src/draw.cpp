#include "draw.hpp"

#include "bestiary.hpp"
#include "entities/boss.hpp"
#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "entities/player.hpp"
#include "entities/ship.hpp"
#include "entities/space.hpp"
#include "guitheme.hpp"
#include "menu.hpp"
#include "palette.hpp"
#include "progression.hpp"
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "sandbox.hpp"
#include "settings.hpp"
#include "ships/bastion.hpp"
#include "ships/interceptor.hpp"
#include "ships/ranger.hpp"
#include "update.hpp"
#include "update_constants.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

void drawText(const Font& font, const char* text, int32_t x, int32_t y, int32_t size, Color color)
{
    DrawTextEx(font, text, Vector2{.x = static_cast<float>(x), .y = static_cast<float>(y)},
               static_cast<float>(size), 1, color);
}

void drawText(const Game& game, const char* text, int32_t x, int32_t y, int32_t size, Color color)
{
    drawText(game.resources.font, text, x, y, size, color);
}

auto measureText(const Font& font, const char* text, int32_t size) -> int32_t
{
    return static_cast<int32_t>(MeasureTextEx(font, text, static_cast<float>(size), 1).x);
}

auto measureText(const Game& game, const char* text, int32_t size) -> int32_t
{
    return measureText(game.resources.font, text, size);
}

auto letterBoxRect(const Game& game) -> Rectangle
{
    const float scale = game.resources.renderScale;

    auto width = static_cast<float>(game.resources.screenWidth) * scale;
    auto height = static_cast<float>(game.resources.screenHeight) * scale;

    return Rectangle{.x = (static_cast<float>(game.resources.windowWidth) - width) / 2,
                     .y = (static_cast<float>(game.resources.windowHeight) - height) / 2,
                     .width = width,
                     .height = height};
}

namespace
{

void drawTitle(const Game& game);
void drawEnemyShape(EnemyShape shape, Vector2 center, float radius, float rotationDeg, Color color);
void drawBossHull(BossShape shape, Vector2 center, Vector2 size, Color color);
void drawShipSelect(const Game& game);
void drawGameOver(const Game& game);
void drawEnding(const Game& game);
void drawOverlay(const Game& game);
auto easeOutBack(float t) -> float;
auto popScale(const Game& game, int32_t index) -> float;
void beginPop(Vector2 center, float scale);
void endPop();
void drawSettings(const Game& game);
void drawSkillIcon(SkillType id, Vector2 c, float s, Color color, bool evolved = false);
void drawWeaponIcon(WeaponType kind, Vector2 c, float s, Color color, bool evolved = false);
void drawPassiveIcon(SkillType id, Vector2 c, float s, Color color);
auto evolutionHint(const Game& game, SkillType id) -> std::string;
void drawAbilitySlots(const Game& game, int32_t x, int32_t y);
void drawLevelUp(const Game& game);
void drawMenu(const Game& game, const std::string& heading, const std::vector<std::string>& options,
              int32_t y);
void drawHighScores(const Game& game, int32_t x, int32_t y, const std::vector<int32_t>& scores);
void drawGameplayWorld(const Game& game);
void drawPunctumLightning(const Game& game);
void drawDebrisSquall(const Game& game);
void drawRustPatches(const Game& game);
void drawShipHull(ShipClass shipClass, Vector2 p, float r, float angle, Color shipColor);
void drawShip(const Game& game);
void drawEngineFlame(Vector2 p, float r, float angle);
void drawEnemy(const Game& game, const Enemy& enemy, bool buffed);
void drawEliteHazard(const EliteHazard& hazard, Color body);
void drawBoss(const Game& game, const Boss& boss);
void drawOrbitBlades(const Game& game);
void drawOrbitBladeProjectiles(const Game& game);
void drawNerveBallProjectiles(const Game& game);
void drawNerveSpiralProjectiles(const Game& game);
void drawShieldIndicator(const Player& player);
void drawChargeParticles(Vector2 center, float fraction, Color color, float maxRadius,
                         float minRadius);
void drawComet(const BossProjectile& projectile);
void drawMeteorProjectile(const BossProjectile& projectile);
void drawSporeProjectile(const BossProjectile& projectile);
void drawKrakenHurledEnemy(const BossProjectile& projectile);
void drawFluidBlob(Vector2 center, float radius, Color color, float seed, Vector2 trailDir);
void drawLavaChunk(Vector2 center, float radius, float seed, Vector2 velocity);
void drawWreckwormHeadShape(Vector2 center, float radius, float seed, Color color,
                            Vector2 velocity, Vector2 facing, float mouthOpen);
void drawKrakenShape(Vector2 center, float radius, float seed, Color color, Vector2 facing = Vector2{.x = 0, .y = 1});
void drawBendyTentacle(Vector2 anchor, Vector2 tip, Vector2 bodyCenter, float bodyRadius,
                       float seed, float thicknessScale, float waveScale, bool calm, Color color,
                       Color tipColor);
void drawKrakenTentacle(const Boss& boss, int32_t slot);
void drawKrakenSnatch(const Boss& boss);
void drawBossQteHud(const Game& game);
void drawKrakenStaggeredTentacle(const Boss& boss, int32_t slot);
void drawKrakenLimb(const Boss& boss, int32_t slot);
void drawKrakenGrabbedEnemy(Vector2 pos, int32_t kindIndex, float scale, float facingAngle);
void drawBanishedShape(Vector2 center, const Boss& boss);
void drawBanishedTentacle(Vector2 center, const Boss& boss, int32_t slot);
void drawBanishedEye(const Boss& boss);
void drawPanelShape(Vector2 center, float radius, int32_t sides, float rotationDeg, Color baseColor,
                    float seed);
void drawPanelHex(Vector2 center, float radius, float rotationDeg, Color baseColor, float seed);
void drawHUD(const Game& game);
void drawTouchControls(const Game& game);
void drawStatusRow(const Game& game, int32_t x, int32_t y);
void drawDownwardTriangleIcon(float cx, int32_t y, float size, Color fillColor, bool filled);
void drawHealthBar(const Game& game, int32_t x, int32_t y);
auto healthBarWidthWithLabel() -> int32_t;
void drawShieldStackPips(const Game& game, int32_t x, int32_t y);
void drawNerveBar(const Game& game, int32_t x, int32_t y);
auto drawDamageMeter(const Game& game, int32_t x, int32_t y) -> int32_t;
void drawChargePips(const Game& game, int32_t x, int32_t y);
void drawOrganicEnemyShape(Vector2 center, float baseRadius, float seed, Color color);
void drawHuskSentinelShape(Vector2 center, float radius, float seed, Vector2 aim, Color color);
void drawCarrionSpots(Vector2 center, float radius, float seed, Color color);
void drawClusterEnemyShape(Vector2 center, float baseRadius, float seed, Color color);
void drawMeteorBossShape(Vector2 center, float radius, int32_t seed, Color baseColor);
void drawSlagmawShape(Vector2 center, float radius, float seed, Vector2 facing, float mawOpen, float heat,
                      float flash, float alpha);
void drawDevourCone(Vector2 maw, Vector2 dir);
void drawSlagGob(Vector2 position, float radius, Vector2 velocity);
void drawMagmaPools(const Game& game);
void drawSandboxToggleIndicator(const Game& game);
void drawBossDirectorTells(const Boss& boss, Vector2 center);
void drawSandboxMenu(const Game& game);
void drawSandboxBrowser(const Game& game);
void drawLaserBolt(const Bullet& b);
void drawPickupIcon(PickupType type, ElementType element, Vector2 position, float alpha);
void drawWormholeMouth(Vector2 position, WormholeFacing facing, float radius, Color rim);

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
}

void beginPixelZoom(const Game& game)
{
    // Stars are tiled centered on world-space origin (see drawBackgroundStars /
    // tiledWorldPos), so the camera needs the same origin-at-screen-center offset
    // beginWorldCamera uses - a zero offset put the whole starfield in the
    // top-left, empty everywhere else, since half the tiled range is negative.
    const float zoom = game.resources.renderScale;
    const Vector2 center{.x = static_cast<float>(game.resources.screenWidth) / 2,
                         .y = static_cast<float>(game.resources.screenHeight) / 2};
    BeginMode2D(Camera2D{.offset = Vector2Scale(center, zoom),
                         .target = Vector2{},
                         .rotation = 0,
                         .zoom = zoom});
}

constexpr float transitionCameraPush = 0.08F;
auto transitionPush(const Game& game) -> float;

void beginWorldCamera(const Game& game)
{
    const float zoom = game.resources.renderScale * (1.0F + transitionCameraPush * transitionPush(game));
    const Vector2 center{.x = static_cast<float>(game.resources.screenWidth) / 2,
                         .y = static_cast<float>(game.resources.screenHeight) / 2};
    Vector2 offset = Vector2Scale(center, zoom);

    if (game.run.shakeTimer > 0 && game.run.shakeDuration > 0)
    {
        const float amplitude =
            game.run.shakeIntensity * (game.run.shakeTimer / game.run.shakeDuration);
        offset.x += static_cast<float>(GetRandomValue(-100, 100)) / 100 * amplitude * zoom;
        offset.y += static_cast<float>(GetRandomValue(-100, 100)) / 100 * amplitude * zoom;
    }

    BeginMode2D(Camera2D{
        .offset = offset, .target = game.run.player.position, .rotation = 0, .zoom = zoom});
}

auto tiledWorldPos(Vector2 relativeTo, Vector2 offset, float tileW, float tileH) -> Vector2
{
    float relX = std::fmod(offset.x - relativeTo.x, tileW);
    if (relX > tileW / 2)
    {
        relX -= tileW;
    }
    else if (relX < -tileW / 2)
    {
        relX += tileW;
    }

    float relY = std::fmod(offset.y - relativeTo.y, tileH);
    if (relY > tileH / 2)
    {
        relY -= tileH;
    }
    else if (relY < -tileH / 2)
    {
        relY += tileH;
    }

    return Vector2Add(relativeTo, Vector2{.x = relX, .y = relY});
}

auto biomeVoidColor(const Game& game) -> Color
{
    switch (currentBiome(game.run.waveNumber))
    {
    case Biome::ShatteredBelt:
        return Palette::Void;
    case Biome::Rustbloom:
        return Palette::RustbloomVoid;
    case Biome::SolarForge:
        return Palette::SolarForgeVoid;
    case Biome::Punctum:
        return Palette::PunctumVoid;
    }
    return Palette::Void;
}

auto biomeHazeColor(const Game& game) -> Color
{
    switch (currentBiome(game.run.waveNumber))
    {
    case Biome::ShatteredBelt:
        return Palette::Haze;
    case Biome::Rustbloom:
        return Palette::RustbloomHaze;
    case Biome::SolarForge:
        return Palette::SolarForgeHaze;
    case Biome::Punctum:
        return Palette::PunctumHaze;
    }
    return Palette::Haze;
}

void drawBackgroundStars(const Game& game, Vector2 relativeTo)
{
    const auto tileW = static_cast<float>(game.resources.screenWidth);
    const auto tileH = static_cast<float>(game.resources.screenHeight);

    for (const auto& p : game.run.bgParticles)
    {
        DrawCircleV(tiledWorldPos(relativeTo, p.position, tileW, tileH), p.radius, p.color);
    }

    const Color haze = biomeHazeColor(game);
    for (const auto& s : game.run.stars)
    {
        DrawCircleV(tiledWorldPos(relativeTo, s.position, tileW, tileH), s.radius, haze);
    }
}

void windowText(const Font& font, const Game& game, const char* text, int32_t centerX, int32_t y,
                int32_t size, Color color)
{
    const float scale = guiUiScale(game);
    const auto scaledSize = static_cast<int32_t>(static_cast<float>(size) * scale);
    const int32_t width = measureText(font, text, scaledSize);
    drawText(font, text, centerX - width / 2, static_cast<int32_t>(static_cast<float>(y) * scale),
             scaledSize, color);
}

void windowText(const Game& game, const char* text, int32_t centerX, int32_t y, int32_t size,
                Color color)
{
    windowText(game.resources.font, game, text, centerX, y, size, color);
}

void drawTitle(const Game& game)
{
    applyGuiScale(game);
    const TitleLayout layout = titleLayout();
    windowText(game, "GALAXY IMPACT", game.resources.windowWidth / 2, layout.titleY, 100,
              Palette::Accent);

    drawMenu(game, "", {"Start", "Skill Tree", "Achievements", "Bestiary", "Settings", "Exit"}, layout.menuY);
    drawHighScores(game, game.resources.windowWidth / 2, layout.highScoreY,
                   game.resources.highScores);
}

void drawShipSelect(const Game& game)
{
    applyGuiScale(game);
    const auto shipClass = static_cast<ShipClass>(game.menuIndex);
    const ShipDef& highlighted = ships.at(static_cast<size_t>(game.menuIndex));

    const ShipSelectLayout layout = shipSelectLayout(game);
    beginPop(layout.previewPos, popScale(game, 0) * easeOutBack(game.menuIndexAge / 0.22F));
    windowText(game, std::string(highlighted.name).c_str(), game.resources.windowWidth / 2,
              layout.nameY, 40, Palette::Accent);
    const Vector2 dir = aimAtMouse(game);
    const float previewAngle = std::atan2(dir.y, dir.x) + std::numbers::pi_v<float> / 2;
    if (shipClass != ShipClass::Bastion && shipClass != ShipClass::Interceptor &&
        shipClass != ShipClass::Ranger)
    {
        drawEngineFlame(layout.previewPos, layout.previewRadius, previewAngle);
    }
    drawShipHull(shipClass, layout.previewPos, layout.previewRadius, previewAngle,
                highlighted.color);
    endPop();

    const Vector2 mouse = GetMousePosition();
    const auto drawChevron = [&](Vector2 center, bool left, Rectangle tapZone)
    {
        const bool hovered = hoverActive(game) && CheckCollisionPointRec(mouse, tapZone);
        const Color color = hovered ? Palette::Accent : Palette::StructLight;
        const float size = layout.arrowHitRadius * 0.5F;
        const float sign = left ? -1.0F : 1.0F;
        const Vector2 tip{.x = center.x + sign * size, .y = center.y};
        const Vector2 top{.x = center.x - sign * size, .y = center.y - size};
        const Vector2 bottom{.x = center.x - sign * size, .y = center.y + size};
        const float thickness = 4.0F * guiUiScale(game);
        DrawLineEx(top, tip, thickness, color);
        DrawLineEx(tip, bottom, thickness, color);
    };
    drawChevron(layout.leftArrow, true, layout.leftTapZone);
    drawChevron(layout.rightArrow, false, layout.rightTapZone);

    windowText(game, std::string(highlighted.description).c_str(), game.resources.windowWidth / 2,
              layout.descY, 18, Palette::StructLight);
    if (!isShipUnlocked(game, shipClass))
    {
        windowText(game, "LOCKED - unlock it in the Skill Tree", game.resources.windowWidth / 2, layout.descY - 34, 20,
                   Palette::Accent);
    }

    const std::string stats = std::format(
        "HP {:.0f}   Armor {:.0f}   Dmg {:+.0f}%   Shields {}", highlighted.maxHealth,
        highlighted.armor, (highlighted.damageMult - 1.0F) * 100.0F, highlighted.maxShieldStacks);
    windowText(game, stats.c_str(), game.resources.windowWidth / 2, layout.statsY, 18,
              Palette::StructMid);

    const bool selectHovered = CheckCollisionPointRec(mouse, layout.selectRect);
    if (selectHovered)
    {
        GuiSetState(STATE_FOCUSED);
    }
    beginPop(Vector2{.x = layout.selectRect.x + layout.selectRect.width / 2,
                     .y = layout.selectRect.y + layout.selectRect.height / 2},
             popScale(game, 2));
    GuiButton(layout.selectRect, "Select");
    endPop();
    GuiSetState(STATE_NORMAL);
}

void drawGameOver(const Game& game)
{
    applyGuiScale(game);
    beginPop(Vector2{.x = static_cast<float>(game.resources.windowWidth) / 2, .y = 90.0F * guiUiScale(game)},
             popScale(game, 0));
    windowText(game, "YOU WERE DEFEATED!", game.resources.windowWidth / 2, 70, 40, Palette::Accent);
    endPop();

    const std::string statsText =
        std::format("Score: {}   Level: {}   Wave: {}   Time: {:.0f}s", game.run.score,
                    game.run.level, game.run.waveNumber, game.run.runTime);
    windowText(game, statsText.c_str(), game.resources.windowWidth / 2, 120, 20,
               Palette::StructLight);

    drawHighScores(game, game.resources.windowWidth / 2, 160, game.resources.highScores);
    drawMenu(game, "", {"New Game", "Exit"}, MenuLayout::gameOverMenuY);
}

void drawEnding(const Game& game)
{
    const auto windowW = static_cast<float>(game.resources.windowWidth);
    const auto windowH = static_cast<float>(game.resources.windowHeight);
    const auto t = static_cast<float>(GetTime());
    const float parallax = game.run.endingTimer * endingDriftSpeed * 0.15F;

    for (int32_t i = 0; i < 60; i++)
    {
        const float seed = static_cast<float>(i);
        const float twinkle =
            0.35F + 0.35F * std::sin(t * (0.6F + hashNoise(seed) * 1.4F) + seed * 5.0F);
        const float x = std::fmod(hashNoise(seed * 3.1F) * windowW - parallax, windowW);
        DrawCircleV(Vector2{.x = x < 0 ? x + windowW : x, .y = hashNoise(seed * 7.7F) * windowH},
                    1.2F, Fade(WHITE, twinkle));
    }

    const float sunPulse = 1.0F + 0.06F * std::sin(t * 1.3F);
    const Vector2 sunPos{.x = windowW * 0.85F - parallax * 0.3F, .y = windowH * 0.2F};
    DrawCircleV(sunPos, 60 * sunPulse, Fade(Palette::Accent, 0.25F));
    DrawCircleV(sunPos, 42, Palette::Accent);

    const Vector2 earthPos{.x = windowW * 0.62F - parallax * 0.5F, .y = windowH * 0.42F};
    DrawCircleV(earthPos, 28, Palette::Shield);
    const float moonOrbit = t * 0.5F;
    const Vector2 moonPos = Vector2Add(
        earthPos, Vector2{.x = std::cos(moonOrbit) * 13.0F, .y = std::sin(moonOrbit) * 13.0F});
    DrawCircleV(moonPos, 9, Palette::Heal);

    const Vector2 driftPlanet1{.x = windowW * 0.3F - parallax * 0.8F,
                               .y = windowH * 0.72F + std::sin(t * 0.7F) * 6.0F};
    DrawCircleV(driftPlanet1, 9, Palette::StructMid);
    const Vector2 driftPlanet2{.x = windowW * 0.14F - parallax,
                               .y = windowH * 0.28F + std::sin(t * 0.9F + 2.0F) * 4.0F};
    DrawCircleV(driftPlanet2, 5, Palette::AccentDim);

    drawShip(game);

    applyGuiScale(game);
    constexpr float textFadeInDuration = 1.8F;
    const float textAlpha = std::clamp(game.run.endingTimer / textFadeInDuration, 0.0F, 1.0F);
    // windowText scales its y by guiUiScale(), so bottom-anchored positions must be
    // pre-divided by that scale or they land below the visible window on tall resolutions.
    const float scale = guiUiScale(game);
    const auto windowHeightF = static_cast<float>(game.resources.windowHeight);
    windowText(game, "Thank you for playing this game.", game.resources.windowWidth / 2,
               static_cast<int32_t>((windowHeightF - 140) / scale), 26, Fade(WHITE, textAlpha));
    windowText(game, "Never go to space alone.", game.resources.windowWidth / 2,
               static_cast<int32_t>((windowHeightF - 100) / scale), 26, Fade(WHITE, textAlpha));
    if (game.run.endingTimer >= endingSkipGraceDuration)
    {
        const float promptPulse = 0.6F + 0.4F * std::sin(t * 3.0F);
        windowText(game, "click to continue", game.resources.windowWidth / 2,
                   static_cast<int32_t>((windowHeightF - 50) / scale), 16,
                   Fade(Palette::StructLight, promptPulse));
    }
}

// --- Screen transitions --------------------------------------------------------
//
// Opening a screen grows a dark matte iris out of its anchor (the ship when it opens over
// gameplay, otherwise the pointer), while the world camera pushes in a little; the screen's
// items then pop in one after another with a slight overshoot. Returning to gameplay shrinks
// the iris back into the ship.
// -------------------------------------------------------------------------------

constexpr float irisOpenDuration = 0.32F;
constexpr float irisCloseDuration = 0.22F;

auto easeOutCubic(float t) -> float
{
    const float u = 1.0F - std::clamp(t, 0.0F, 1.0F);
    return 1.0F - u * u * u;
}

auto easeOutBack(float t) -> float
{
    constexpr float c1 = 1.70158F;
    constexpr float c3 = c1 + 1.0F;
    const float u = std::clamp(t, 0.0F, 1.0F) - 1.0F;
    return 1.0F + c3 * u * u * u + c1 * u * u;
}

// Screens drawn on top of the running world.
auto overGameplay(const Game& game, GameState state) -> bool
{
    switch (state)
    {
    case GameState::PAUSED:
    case GameState::LEVEL_UP:
    case GameState::GAME_OVER:
    case GameState::SANDBOX_MENU:
        return true;
    case GameState::SHIP_SELECT:
    case GameState::SETTINGS:
    case GameState::ACHIEVEMENTS:
    case GameState::BESTIARY:
    case GameState::SKILL_TREE:
        return game.settingsReturnState == GameState::PAUSED ||
               game.settingsReturnState == GameState::GAME_OVER ||
               game.settingsReturnState == GameState::SANDBOX_MENU;
    default:
        return false;
    }
}

auto shipWindowPos(const Game& game) -> Vector2
{
    const Rectangle box = letterBoxRect(game);
    return Vector2{.x = box.x + box.width / 2, .y = box.y + box.height / 2};
}

void updateTransitions(Game& game)
{
    const float dt = GetFrameTime();
    if (game.state != game.drawnState)
    {
        const bool fromGameplay = game.drawnState == GameState::GAMEPLAY;
        if (game.state == GameState::GAMEPLAY && overGameplay(game, game.drawnState))
        {
            game.closeTimer = irisCloseDuration;
        }
        game.irisAnchor = fromGameplay || game.state == GameState::GAMEPLAY ? shipWindowPos(game) : GetMousePosition();
        game.drawnState = game.state;
        game.stateAge = 0;
    }
    else
    {
        game.stateAge += dt;
    }
    if (game.menuIndex != game.menuIndexDrawn)
    {
        game.menuIndexDrawn = game.menuIndex;
        game.menuIndexAge = 0;
    }
    else
    {
        game.menuIndexAge += dt;
    }
    game.closeTimer = std::max(0.0F, game.closeTimer - dt);
    game.qteAge = bossQteActive(game) ? game.qteAge + dt : 0;
}

auto irisMaxRadius(const Game& game, Vector2 anchor) -> float
{
    const auto w = static_cast<float>(game.resources.windowWidth);
    const auto h = static_cast<float>(game.resources.windowHeight);
    const float dx = std::max(anchor.x, w - anchor.x);
    const float dy = std::max(anchor.y, h - anchor.y);
    return std::sqrt(dx * dx + dy * dy) + 4.0F;
}

// How far the camera has pushed in on the ship (0..1).
auto transitionPush(const Game& game) -> float
{
    if (overGameplay(game, game.state))
    {
        return easeOutCubic(game.stateAge / irisOpenDuration);
    }
    return game.state == GameState::GAMEPLAY ? easeOutCubic(game.closeTimer / irisCloseDuration) : 0.0F;
}

void drawIrisRim(Vector2 anchor, float radius, float progress)
{
    if (progress < 1.0F)
    {
        DrawRing(anchor, radius - 2.0F, radius, 0, 360, 96, Fade(Palette::Haze, 0.5F * (1.0F - progress)));
    }
}

// The dimmed backdrop of a screen over gameplay: a dark disc growing out of the ship.
void drawOverlay(const Game& game)
{
    const float maxR = irisMaxRadius(game, game.irisAnchor);
    const float progress = easeOutCubic(game.stateAge / irisOpenDuration);
    if (progress >= 1.0F)
    {
        DrawRectangle(0, 0, game.resources.windowWidth, game.resources.windowHeight, Fade(Palette::Void, 0.75F));
        return;
    }
    DrawCircleV(game.irisAnchor, maxR * progress, Fade(Palette::Void, 0.75F));
    drawIrisRim(game.irisAnchor, maxR * progress, progress);
}

// A full screen (not over gameplay) is revealed through the iris: everything outside it stays dark.
void drawIrisReveal(const Game& game)
{
    const float progress = easeOutCubic(game.stateAge / irisOpenDuration);
    if (progress >= 1.0F || game.state == GameState::GAMEPLAY || game.state == GameState::ENDING ||
        overGameplay(game, game.state))
    {
        return;
    }
    const float maxR = irisMaxRadius(game, game.irisAnchor);
    const float r = maxR * progress;
    DrawRing(game.irisAnchor, r, maxR + 8.0F, 0, 360, 96, Palette::Void);
    drawIrisRim(game.irisAnchor, r, progress);
}

// Back to gameplay: the dark disc shrinks into the ship.
void drawIrisClose(const Game& game)
{
    if (game.state != GameState::GAMEPLAY || game.closeTimer <= 0)
    {
        return;
    }
    const float progress = easeOutCubic(game.closeTimer / irisCloseDuration);
    const float r = irisMaxRadius(game, game.irisAnchor) * progress;
    DrawCircleV(game.irisAnchor, r, Fade(Palette::Void, 0.75F * progress));
    drawIrisRim(game.irisAnchor, r, 1.0F - progress);
}

// Item `index` of the current screen pops in after the iris, staggered; 0 until its turn.
auto popScale(const Game& game, int32_t index) -> float
{
    const float t = (game.stateAge - irisOpenDuration * 0.4F - 0.05F * static_cast<float>(index)) / 0.24F;
    return t <= 0 ? 0.0F : easeOutBack(t);
}

void beginPop(Vector2 center, float scale)
{
    rlDrawRenderBatchActive(); // flush what came before, so only this item is scaled
    rlPushMatrix();
    rlTranslatef(center.x, center.y, 0);
    rlScalef(scale, scale, 1);
    rlTranslatef(-center.x, -center.y, 0);
}

void endPop()
{
    rlDrawRenderBatchActive();
    rlPopMatrix();
}

void drawSettings(const Game& game)
{
    applyGuiScale(game);
    const float scale = guiUiScale(game);
    windowText(game, "SETTINGS", game.resources.windowWidth / 2, 130, 34, Palette::Accent);

    const auto& res =
        resolutionOptions.at(static_cast<size_t>(game.resources.settings.resolutionIndex));
    const std::string displayMode = IsWindowFullscreen() ? "Fullscreen" : "Windowed";

    const std::array<std::string, 7> stepperLabels{"Resolution",   "", "", "FPS Cap", "HUD Scale",
                                                   "Display Mode", ""};
    const std::array<std::string, 7> stepperValues{
        std::format("{}x{}", res.width, res.height),
        "",
        "",
        std::format("{}", fpsOptions.at(static_cast<size_t>(game.resources.settings.fpsIndex))),
        std::format("{:.0f}%",
                    hudScaleOptions.at(static_cast<size_t>(game.resources.settings.hudScaleIndex)) *
                        100),
        displayMode,
        "",
    };

    constexpr int32_t rowCount = settingsRowCount;
    for (int32_t i = 0; i < rowCount; i++)
    {
        const Rectangle rect =
            menuColumnRect(game, i, rowCount, MenuLayout::settingsWidth, MenuLayout::settingsHeight,
                           MenuLayout::settingsGap, MenuLayout::settingsMenuY);

        if (i == game.menuIndex)
        {
            GuiSetState(STATE_FOCUSED);
        }

        if (i == 1 || i == 2 || i == 6)
        {
            const bool on = i == 1   ? game.resources.settings.bgmOn
                            : i == 2 ? game.resources.settings.soundOn
                                     : game.resources.settings.swapTouchSticks;
            bool checked = on;
            const char* label = i == 1 ? "BGM" : i == 2 ? "Sound" : "Swap Sticks (Android)";
            const Rectangle box{.x = rect.x,
                                .y = rect.y + rect.height / 2 - 12 * scale,
                                .width = 24 * scale,
                                .height = 24 * scale};
            GuiCheckBox(box, label, &checked);
        }
        else if (i == rowCount - 1)
        {
            GuiButton(rect, "Back");
        }
        else
        {
            const std::string text =
                std::format("{}:   <  {}  >", stepperLabels.at(static_cast<size_t>(i)),
                            stepperValues.at(static_cast<size_t>(i)));
            GuiButton(rect, text.c_str());
        }

        GuiSetState(STATE_NORMAL);
    }

    const int32_t hintY = MenuLayout::settingsMenuY +
                          (MenuLayout::settingsHeight + MenuLayout::settingsGap) * rowCount + 20;
    windowText(game, "Up/Down: select   Left/Right: change   Enter: confirm",
               game.resources.windowWidth / 2, hintY, 16, Palette::StructMid);
}

// Achievements: every one earned is a skill point (see progression.hpp). Two columns; earned
// rows bright with a tick, the rest dim with their progress where there is a count to show.
void drawAchievements(Game& game)
{
    applyGuiScale(game);
    const auto& ach = game.resources.achievements;
    const float scale = guiUiScale(game);

    constexpr int32_t columns = 2;
    constexpr int32_t rowHeight = 30;
    constexpr int32_t columnWidth = 470;
    constexpr int32_t rows = (achievementCount + columns - 1) / columns;
    constexpr float titleYRel = 20.0F;
    constexpr float summaryYRel = titleYRel + 50.0F;
    constexpr float gridTopYRel = summaryYRel + 45.0F;
    constexpr float contentHeightBase = gridTopYRel + static_cast<float>(rows * rowHeight) + 30.0F;

    const float viewportHeight = static_cast<float>(game.resources.windowHeight) - 150.0F * scale;
    const float contentHeight = contentHeightBase * scale;
    game.achievementsScroll.maxOffset = std::max(0.0F, contentHeight - viewportHeight);
    const float blockTop = std::max(0.0F, (viewportHeight - contentHeight) / 2) - game.achievementsScroll.offset;

    BeginScissorMode(0, 0, game.resources.windowWidth, static_cast<int32_t>(viewportHeight));
    windowText(game, "ACHIEVEMENTS", game.resources.windowWidth / 2,
               static_cast<int32_t>((blockTop + titleYRel * scale) / scale), 34, Palette::Accent);
    const auto earned = static_cast<int32_t>(std::ranges::count(ach.earned, true));
    const std::string summary =
        std::format("{} / {} earned   Skill points to spend: {}   Highest wave: {}", earned, achievementCount,
                    skillPoints(game), ach.highestWaveReached);
    windowText(game, summary.c_str(), game.resources.windowWidth / 2,
               static_cast<int32_t>((blockTop + summaryYRel * scale) / scale), 18, Palette::StructLight);

    const float leftX = static_cast<float>(game.resources.windowWidth) / 2 - static_cast<float>(columnWidth) * scale;
    const float topY = blockTop + gridTopYRel * scale;
    for (int32_t id = 0; id < achievementCount; id++)
    {
        const bool done = ach.earned.at(static_cast<size_t>(id));
        const float x = leftX + static_cast<float>(id / rows) * static_cast<float>(columnWidth) * scale;
        const float y = topY + static_cast<float>(id % rows) * static_cast<float>(rowHeight) * scale;
        std::string line = achievementName(id);
        if (!done && id < achievementWeaponMaxed)
        {
            line += std::format("  ({}/{})", ach.killsByWeapon.at(static_cast<size_t>(id)), evolutionKillThreshold);
        }
        else if (!done && id == achievementDashNerveKills)
        {
            line += std::format("  ({}/{})", ach.dashOrNerveKills, flakCannonUnlockKillCount);
        }
        const Color color = done ? Palette::Crit : Palette::StructMid;
        const Vector2 mark{.x = x + 10.0F * scale, .y = y + 10.0F * scale};
        if (done)
        {
            DrawLineEx(Vector2{.x = mark.x - 6 * scale, .y = mark.y}, Vector2{.x = mark.x - 2 * scale, .y = mark.y + 5 * scale},
                       2.0F * scale, color);
            DrawLineEx(Vector2{.x = mark.x - 2 * scale, .y = mark.y + 5 * scale}, Vector2{.x = mark.x + 6 * scale, .y = mark.y - 5 * scale},
                       2.0F * scale, color);
        }
        else
        {
            DrawCircleLines(static_cast<int32_t>(mark.x), static_cast<int32_t>(mark.y), 5.0F * scale, color);
        }
        drawText(game, line.c_str(), static_cast<int32_t>(x + 24.0F * scale), static_cast<int32_t>(y), 17, color);
    }
    EndScissorMode();

    const Rectangle back = backButtonRect(game);
    if (hoverActive(game) && CheckCollisionPointRec(GetMousePosition(), back))
    {
        GuiSetState(STATE_FOCUSED);
    }
    GuiButton(back, "Back");
    GuiSetState(STATE_NORMAL);
}

// Skill tree: matte discs joined to their prerequisites. Owned nodes are lit, buyable ones pulse
// in the accent, the rest stay dark until a neighbour is bought.
void drawSkillTree(const Game& game)
{
    applyGuiScale(game);
    const float r = skillTreeNodeRadius(game);
    const float t = static_cast<float>(GetTime());

    const int32_t points = skillPoints(game);
    windowText(game, "SKILL TREE", game.resources.windowWidth / 2, 28, 34, Palette::Accent);
    const std::string header = game.sandbox ? std::string("Points: unlimited (sandbox)")
                                            : std::format("Points: {}", points);
    windowText(game, header.c_str(), game.resources.windowWidth / 2, 72, 20,
               points > 0 || game.sandbox ? Palette::Crit : Palette::StructLight);

    // Branch labels, beside the first node of each branch.
    const auto label = [&](const char* text, int32_t node, Vector2 off)
    {
        const Vector2 p = Vector2Add(skillTreeNodePos(game, node), Vector2Scale(off, r));
        const int32_t w = MeasureText(text, static_cast<int32_t>(r * 0.5F));
        DrawText(text, static_cast<int32_t>(p.x) - w / 2, static_cast<int32_t>(p.y), static_cast<int32_t>(r * 0.5F),
                 Palette::StructMid);
    };
    label("OFFENCE", 1, Vector2{.x = 0, .y = -2.2F});
    label("DEFENCE", 14, Vector2{.x = 0, .y = -2.2F});
    label("UTILITY", 25, Vector2{.x = 2.3F, .y = -0.25F});

    // Edges first, under the nodes.
    for (int32_t i = 0; i < skillNodeCount; i++)
    {
        for (const int8_t req : skillTree.at(static_cast<size_t>(i)).prereqs)
        {
            if (req < 0)
            {
                continue;
            }
            const Vector2 a = skillTreeNodePos(game, req);
            const Vector2 b = skillTreeNodePos(game, i);
            const Vector2 dir = Vector2Normalize(Vector2Subtract(b, a));
            const Vector2 from = Vector2Add(a, Vector2Scale(dir, r));
            const Vector2 to = Vector2Subtract(b, Vector2Scale(dir, r));
            const bool lit = skillNodeOwned(game, i);
            const bool open = skillNodeOwned(game, req);
            const Color color =
                lit ? Fade(Palette::Crit, 0.7F) : open ? Fade(Palette::Accent, 0.55F) : Palette::StructDark;
            const float len = Vector2Distance(from, to);
            if (len < r * 8.0F)
            {
                DrawLineEx(from, to, lit ? 3.0F : 2.0F, color);
                continue;
            }
            // A long link across branches is dotted, so it reads as a cross-link, not a path.
            for (float d = 0; d < len; d += r * 0.5F)
            {
                DrawCircleV(Vector2Add(from, Vector2Scale(dir, d)), lit ? 1.8F : 1.4F, color);
            }
        }
    }

    for (int32_t i = 0; i < skillNodeCount; i++)
    {
        const SkillNode& node = skillTree.at(static_cast<size_t>(i));
        const Vector2 c = skillTreeNodePos(game, i);
        const bool owned = skillNodeOwned(game, i);
        const bool available = skillNodeAvailable(game, i);
        const bool selected = i == game.skillTreeSelected;
        float s = popScale(game, i % 8);
        if (i == game.skillTreePopNode && game.skillTreePopTimer > 0)
        {
            s *= 1.0F + 0.35F * std::sin(game.skillTreePopTimer / 0.45F * PI);
        }
        if (s <= 0)
        {
            continue;
        }
        beginPop(c, s);
        const float pulse = 0.5F + 0.5F * std::sin(t * 4.0F + static_cast<float>(i));
        const Color ring = owned ? Palette::Crit : available ? Palette::Accent : Palette::StructMid;
        const Color icon = owned ? Palette::Crit : available ? Palette::Haze : Palette::StructMid;
        const float nodeR = node.kind == NodeKind::Ship ? r * 1.25F : r;
        if (available)
        {
            DrawCircleV(c, nodeR * (1.25F + 0.1F * pulse), Fade(Palette::Accent, 0.12F + 0.12F * pulse));
        }
        DrawCircleV(c, nodeR, owned ? Palette::StructDark : Palette::Void);
        DrawRing(c, nodeR - (selected ? 4.0F : 2.0F), nodeR, 0, 360, 48, ring);
        if (selected)
        {
            DrawRing(c, nodeR + 5.0F, nodeR + 7.0F, t * 90.0F, t * 90.0F + 300.0F, 40, Palette::Haze);
        }
        const float iconS = nodeR * 0.48F;
        switch (node.kind)
        {
        case NodeKind::Ship:
            drawShipHull(static_cast<ShipClass>(node.value), c, nodeR * 0.55F, -PI / 2, icon);
            break;
        case NodeKind::Weapon:
            drawWeaponIcon(static_cast<WeaponType>(node.value), c, iconS, icon);
            break;
        case NodeKind::Evolution:
            drawWeaponIcon(static_cast<WeaponType>(node.value), c, iconS, icon, true);
            DrawRing(c, nodeR * 0.78F, nodeR * 0.82F, 0, 360, 40, Fade(icon, 0.6F)); // evolved: inner ring
            break;
        case NodeKind::Passive:
            drawPassiveIcon(static_cast<SkillType>(node.value), c, iconS, icon);
            break;
        case NodeKind::Slot:
        {
            const float h = iconS * 0.8F;
            DrawRectangleLinesEx(Rectangle{.x = c.x - h, .y = c.y - h, .width = h * 2, .height = h * 2}, 2.0F, icon);
            const std::string n = std::to_string(node.value);
            const auto fs = static_cast<int32_t>(h * 1.2F);
            DrawText(n.c_str(), static_cast<int32_t>(c.x) - MeasureText(n.c_str(), fs) / 2,
                     static_cast<int32_t>(c.y) - fs / 2, fs, icon);
            break;
        }
        }
        endPop();
    }

    // The selected node, described along the bottom.
    const int32_t sel = game.skillTreeSelected;
    const SkillNode& node = skillTree.at(static_cast<size_t>(sel));
    constexpr std::array<const char*, 5> kinds{"Ship (with its signature weapon and Nerve burst)", "Weapon",
                                               "Upgrade helper", "Evolution", "Weapon slot"};
    std::string state;
    if (skillNodeOwned(game, sel))
    {
        state = "Unlocked";
    }
    else if (skillNodeAvailable(game, sel))
    {
        state = game.sandbox || points > 0 ? "Click or Enter: unlock for 1 point" : "Needs 1 point: earn achievements";
    }
    else
    {
        state = "Needs: ";
        bool first = true;
        for (const int8_t req : node.prereqs)
        {
            if (req >= 0 && !skillNodeOwned(game, req))
            {
                state += (first ? "" : ", ") + skillNodeName(req);
                first = false;
            }
        }
    }
    const float scale = guiUiScale(game);
    const Rectangle back = backButtonRect(game);
    const auto infoY = static_cast<int32_t>((back.y - 70.0F * scale) / scale);
    const std::string title = std::format("{}  -  {}", skillNodeName(sel), kinds.at(static_cast<size_t>(node.kind)));
    windowText(game, title.c_str(), game.resources.windowWidth / 2, infoY, 22, Palette::Haze);
    windowText(game, state.c_str(), game.resources.windowWidth / 2, infoY + 30, 17,
               skillNodeOwned(game, sel) ? Palette::Crit : Palette::StructLight);

    if (hoverActive(game) && CheckCollisionPointRec(GetMousePosition(), back))
    {
        GuiSetState(STATE_FOCUSED);
    }
    GuiButton(back, "Back");
    GuiSetState(STATE_NORMAL);
}

// Predicts one Bestiary section's drawn height without drawing it, so
// updateBestiary can size its scroll range before drawBestiary runs. Keep
// this in sync with drawBestiarySection's own cellSize/gap/labelGap/columns
// math below.
auto bestiarySectionHeight(const Game& game, float scale, size_t count) -> float
{
    constexpr float cellSize = 52.0F;
    constexpr float gap = 14.0F;
    constexpr float labelGap = 20.0F;
    const float cellStep = (cellSize + gap) * scale;
    const float rowHeight = (cellSize + labelGap + gap) * scale;
    const float usableWidth = static_cast<float>(game.resources.windowWidth) * 0.88F;
    const auto columns = std::max(1, static_cast<int32_t>(usableWidth / cellStep));
    const auto rows = (static_cast<int32_t>(count) + columns - 1) / columns;
    return 34.0F * scale + static_cast<float>(rows) * rowHeight + 16.0F * scale;
}

auto bestiaryContentHeight(const Game& game, float scale) -> float
{
    constexpr float headerHeight = 140.0F; // "BESTIARY" title + summary line, unscaled
    return headerHeight * scale + bestiarySectionHeight(game, scale, enemyKinds.size()) +
          bestiarySectionHeight(game, scale, bestiaryBossKindCount) +
          bestiarySectionHeight(game, scale, bestiaryHazardKindCount);
}

namespace
{
auto drawBestiarySection(const Game& game, std::string_view title, float y, float scale,
                         size_t count, const std::function<bool(size_t)>& isUnlocked,
                         const std::function<std::string_view(size_t)>& nameFor,
                         const std::function<void(size_t, Vector2, float, Color)>& drawIcon,
                         const std::function<Color(size_t)>& colorFor) -> float
{
    constexpr float cellSize = 52.0F;
    constexpr float gap = 14.0F;
    constexpr float labelGap = 20.0F;
    const float cellStep = (cellSize + gap) * scale;
    const float rowHeight = (cellSize + labelGap + gap) * scale;

    const auto titleSize = static_cast<int32_t>(22 * scale);
    const std::string titleStr(title);
    const int32_t titleWidth = measureText(game, titleStr.c_str(), titleSize);
    drawText(game, titleStr.c_str(), game.resources.windowWidth / 2 - titleWidth / 2,
             static_cast<int32_t>(y), titleSize, Palette::Accent);
    y += 34 * scale;

    const float usableWidth = static_cast<float>(game.resources.windowWidth) * 0.88F;
    const auto columns = std::max(1, static_cast<int32_t>(usableWidth / cellStep));
    const float gridWidth = static_cast<float>(columns) * cellStep;
    const float leftX = (static_cast<float>(game.resources.windowWidth) - gridWidth) / 2;

    const Vector2 mouse = GetMousePosition();
    std::string hoverName;
    bool hovering = false;

    for (size_t i = 0; i < count; i++)
    {
        const auto column = static_cast<int32_t>(i) % columns;
        const auto row = static_cast<int32_t>(i) / columns;
        const float x = leftX + static_cast<float>(column) * cellStep;
        const float cellY = y + static_cast<float>(row) * rowHeight;
        const bool unlocked = isUnlocked(i);
        const Rectangle box{
            .x = x, .y = cellY, .width = cellSize * scale, .height = cellSize * scale};

        DrawRectangleLines(static_cast<int32_t>(box.x), static_cast<int32_t>(box.y),
                           static_cast<int32_t>(box.width), static_cast<int32_t>(box.height),
                           Fade(unlocked ? Palette::StructLight : Palette::StructMid, 0.6F));

        const Vector2 center{.x = box.x + box.width / 2, .y = box.y + box.height / 2};
        if (unlocked)
        {
            drawIcon(i, center, cellSize * scale * 0.36F, colorFor(i));
        }
        else
        {
            drawText(game, "?", static_cast<int32_t>(center.x - 7 * scale),
                     static_cast<int32_t>(center.y - 11 * scale), static_cast<int32_t>(22 * scale),
                     Palette::StructMid);
        }

        const std::string label = unlocked ? std::string(nameFor(i)) : std::string("???");
        const int32_t labelWidth = measureText(game, label.c_str(), 12);
        drawText(game, label.c_str(),
                 static_cast<int32_t>(x + box.width / 2 - static_cast<float>(labelWidth) / 2),
                 static_cast<int32_t>(cellY + box.height + 2 * scale), 12,
                 unlocked ? Palette::StructLight : Palette::StructMid);

        if (hoverActive(game) && CheckCollisionPointRec(mouse, box))
        {
            hovering = true;
            hoverName = unlocked ? std::string(nameFor(i)) : "??? (undiscovered)";
        }
    }

    const auto rows = (static_cast<int32_t>(count) + columns - 1) / columns;
    const float sectionEnd = y + static_cast<float>(rows) * rowHeight;

    if (hovering)
    {
        const auto hoverSize = static_cast<int32_t>(16 * scale);
        const int32_t hoverWidth = measureText(game, hoverName.c_str(), hoverSize);
        drawText(game, hoverName.c_str(), game.resources.windowWidth / 2 - hoverWidth / 2,
                 static_cast<int32_t>(sectionEnd), hoverSize, Palette::Crit);
        return sectionEnd + 30 * scale;
    }

    return sectionEnd + 16 * scale;
}
}

void drawBestiary(Game& game)
{
    applyGuiScale(game);
    const auto& bestiary = game.resources.bestiary;
    const float scale = guiUiScale(game);

    // Whole screen (title through footer) is one invisible container,
    // centered in the space above the Back button - and, since the section
    // grids' height depends on window width, scrollable if it doesn't fit.
    const float viewportHeight = static_cast<float>(game.resources.windowHeight) - 150.0F * scale;
    const float contentHeight = bestiaryContentHeight(game, scale);
    game.bestiaryScroll.maxOffset = std::max(0.0F, contentHeight - viewportHeight);
    const float blockTop =
        std::max(0.0F, (viewportHeight - contentHeight) / 2) - game.bestiaryScroll.offset;

    BeginScissorMode(0, 0, game.resources.windowWidth, static_cast<int32_t>(viewportHeight));

    windowText(game, "BESTIARY", game.resources.windowWidth / 2,
              static_cast<int32_t>((blockTop + 60.0F * scale) / scale), 34, Palette::Accent);

    const auto enemyCount =
        std::count(bestiary.enemyKilled.begin(), bestiary.enemyKilled.end(), true);
    const auto bossCount = std::count(bestiary.bossKilled.begin(), bestiary.bossKilled.end(), true);
    const auto hazardCount =
        std::count(bestiary.hazardKilled.begin(), bestiary.hazardKilled.end(), true);
    const std::string summary = std::format(
        "Enemies: {} of {}   Bosses: {} of {}   Hazards: {} of {}", enemyCount, enemyKinds.size(),
        bossCount, bestiaryBossKindCount, hazardCount, bestiaryHazardKindCount);
    windowText(game, summary.c_str(), game.resources.windowWidth / 2,
              static_cast<int32_t>((blockTop + 100.0F * scale) / scale), 18, Palette::StructLight);

    float y = blockTop + 140.0F * scale;
    y = drawBestiarySection(
        game, "ENEMIES", y, scale, enemyKinds.size(),
        [&](size_t i) { return bestiary.enemyKilled.at(i); }, [](size_t i) -> std::string_view
        { return enemyKinds.at(i).name; }, [](size_t i, Vector2 c, float r, Color col)
        { drawEnemyShape(enemyKinds.at(i).shape, c, r, 0, col); },
        [](size_t i) { return enemyKinds.at(i).color; });

    y = drawBestiarySection(
        game, "BOSSES", y, scale, bestiaryBossKindCount,
        [&](size_t i) { return bestiary.bossKilled.at(i); },
        [](size_t i) { return bossBestiaryName(static_cast<int32_t>(i)); },
        [](size_t i, Vector2 c, float r, Color col)
        {
            drawBossHull(bossBestiaryShape(static_cast<int32_t>(i)), c,
                         Vector2{.x = r * 2, .y = r * 2}, col);
        },
        [](size_t i) { return bossBestiaryColor(static_cast<int32_t>(i)); });

    y = drawBestiarySection(
        game, "HAZARDS", y, scale, bestiaryHazardKindCount,
        [&](size_t i) { return bestiary.hazardKilled.at(i); },
        [](size_t i) { return hazardBestiaryName(static_cast<EliteHazardRole>(i)); },
        [](size_t /*i*/, Vector2 c, float r, Color col)
        {
            DrawCircleV(c, r, Palette::StructDark);
            DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), r, col);
            DrawPolyLines(c, 6, r * 1.3F, 0, Fade(col, 0.7F));
        },
        [](size_t i)
        {
            return static_cast<EliteHazardRole>(i) == EliteHazardRole::Warlord ? Palette::Accent
                                                                               : Palette::Shield;
        });

    const auto footerSize = static_cast<int32_t>(16 * scale);
    const char* footerText =
        touchInputActive(game) ? "Tap & hold an entry for its name" : "Hover an entry for its name";
    const int32_t footerWidth = measureText(game, footerText, footerSize);
    drawText(game, footerText, game.resources.windowWidth / 2 - footerWidth / 2,
             static_cast<int32_t>(y), footerSize, Palette::StructMid);

    EndScissorMode();

    const Vector2 mouse = GetMousePosition();
    const Rectangle back = backButtonRect(game);
    if (hoverActive(game) && CheckCollisionPointRec(mouse, back))
    {
        GuiSetState(STATE_FOCUSED);
    }
    GuiButton(back, "Back");
    GuiSetState(STATE_NORMAL);
}

void drawSkillIcon(SkillType id, Vector2 c, float s, Color color, bool evolved)
{
    if (const auto kind = weaponForGrantSkill(id); kind.has_value())
    {
        drawWeaponIcon(*kind, c, s, color, evolved);
        return;
    }
    drawPassiveIcon(id, c, s, color);
}

void drawWeaponIcon(WeaponType kind, Vector2 c, float s, Color color, bool evolved)
{
    switch (kind)
    {
    case WeaponType::Forward:
        DrawTriangle(Vector2{.x = c.x, .y = c.y - s},
                     Vector2{.x = c.x - s * 0.7F, .y = c.y + s * 0.7F},
                     Vector2{.x = c.x + s * 0.7F, .y = c.y + s * 0.7F}, color);
        break;
    case WeaponType::Orbit:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawCircleV(Vector2{.x = c.x + s, .y = c.y}, s * 0.25F, color);
        break;
    case WeaponType::Homing:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawCircleV(c, s * 0.3F, color);
        DrawLineEx(Vector2{.x = c.x, .y = c.y - s}, Vector2{.x = c.x, .y = c.y - s * 1.7F}, 2,
                   color);
        break;
    case WeaponType::Mine:
        DrawCircleV(c, s * 0.55F, color);
        for (int a = 0; a < 360; a += 60)
        {
            const float rad = static_cast<float>(a) * DEG2RAD;
            const Vector2 tip{.x = c.x + std::cos(rad) * s, .y = c.y + std::sin(rad) * s};
            DrawLineEx(c, tip, 2, color);
        }
        break;
    case WeaponType::Beam:
        DrawLineEx(Vector2{.x = c.x - s, .y = c.y}, Vector2{.x = c.x + s, .y = c.y}, 3, color);
        DrawCircleV(Vector2{.x = c.x - s, .y = c.y}, s * 0.25F, color);
        break;
    case WeaponType::Shock:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s * 0.5F, color);
        break;
    case WeaponType::Ricochet:
        DrawLineEx(Vector2{.x = c.x - s, .y = c.y - s}, Vector2{.x = c.x, .y = c.y + s}, 2, color);
        DrawLineEx(Vector2{.x = c.x, .y = c.y + s}, Vector2{.x = c.x + s, .y = c.y - s}, 2, color);
        break;
    case WeaponType::FollowerDrone:
        DrawCircleV(c, s * 0.4F, color);
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        break;
    case WeaponType::LaserDrone:
        DrawCircleV(c, s * 0.4F, color);
        DrawLineEx(c, Vector2{.x = c.x + s, .y = c.y}, 2, color);
        break;
    case WeaponType::FlakCannon:
        DrawCircleV(c, s * 0.5F, color);
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, Fade(color, 0.5F));
        break;
    case WeaponType::Railgun:
        DrawLineEx(Vector2{.x = c.x - s, .y = c.y}, Vector2{.x = c.x + s, .y = c.y}, 4, color);
        break;
    case WeaponType::ChainLightning:
        DrawLineEx(Vector2{.x = c.x - s, .y = c.y - s}, Vector2{.x = c.x, .y = c.y}, 2, color);
        DrawLineEx(Vector2{.x = c.x, .y = c.y}, Vector2{.x = c.x + s, .y = c.y - s}, 2, color);
        break;
    case WeaponType::TurretDeploy:
        DrawRectangle(static_cast<int32_t>(c.x - s * 0.4F), static_cast<int32_t>(c.y - s * 0.4F),
                      static_cast<int32_t>(s * 0.8F), static_cast<int32_t>(s * 0.8F), color);
        break;
    case WeaponType::Flamethrower:
        DrawTriangle(Vector2{.x = c.x - s, .y = c.y}, Vector2{.x = c.x + s * 0.6F, .y = c.y + s},
                     Vector2{.x = c.x + s * 0.6F, .y = c.y - s}, color);
        break;
    case WeaponType::LaserShots:
        // Evolved: the same streaks, heavier.
        for (const float dy : {-0.5F, 0.0F, 0.5F})
        {
            DrawLineEx(Vector2{.x = c.x - s + (dy == 0 ? s * 0.4F : 0), .y = c.y + dy * s},
                       Vector2{.x = c.x + s * (dy == 0 ? 1.0F : 0.6F), .y = c.y + dy * s}, evolved ? 2.5F : 1.5F,
                       color);
        }
        break;
    case WeaponType::Sniper:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawLineEx(Vector2{.x = c.x - s * 1.3F, .y = c.y}, Vector2{.x = c.x + s * 1.3F, .y = c.y},
                   1.5F, color);
        DrawLineEx(Vector2{.x = c.x, .y = c.y - s * 1.3F}, Vector2{.x = c.x, .y = c.y + s * 1.3F},
                   1.5F, color);
        break;
    case WeaponType::Count:
        break;
    }
}

void drawPassiveIcon(SkillType id, Vector2 c, float s, Color color)
{
    switch (id)
    {
    case SkillType::Damage:
        DrawTriangle(Vector2{.x = c.x, .y = c.y - s}, Vector2{.x = c.x - s * 0.6F, .y = c.y + s},
                     Vector2{.x = c.x + s * 0.6F, .y = c.y + s}, color);
        break;
    case SkillType::Barrier:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s * 0.55F, color);
        break;
    case SkillType::Cooldown:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawLineEx(c, Vector2{.x = c.x, .y = c.y - s * 0.8F}, 2, color);
        DrawLineEx(c, Vector2{.x = c.x + s * 0.5F, .y = c.y}, 2, color);
        break;
    case SkillType::PickupRadius:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s * 0.5F, color);
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, Fade(color, 0.5F));
        break;
    case SkillType::MoveSpeed:
        DrawTriangle(Vector2{.x = c.x - s, .y = c.y - s * 0.6F},
                     Vector2{.x = c.x - s, .y = c.y + s * 0.6F}, Vector2{.x = c.x, .y = c.y},
                     color);
        DrawTriangle(Vector2{.x = c.x, .y = c.y - s * 0.6F}, Vector2{.x = c.x, .y = c.y + s * 0.6F},
                     Vector2{.x = c.x + s, .y = c.y}, color);
        break;
    case SkillType::MaxHp:
        DrawRectangle(static_cast<int32_t>(c.x - s * 0.15F), static_cast<int32_t>(c.y - s),
                      static_cast<int32_t>(s * 0.3F), static_cast<int32_t>(s * 2), color);
        DrawRectangle(static_cast<int32_t>(c.x - s), static_cast<int32_t>(c.y - s * 0.15F),
                      static_cast<int32_t>(s * 2), static_cast<int32_t>(s * 0.3F), color);
        break;
    default:
        break;
    }
}

auto elementEffectSummary(ElementType element) -> std::string_view
{
    switch (element)
    {
    case ElementType::Static:
        return "fully stops movement, attacks, and contact damage";
    case ElementType::Freeze:
        return "halves movement and attack speed";
    case ElementType::Burn:
        return "deals damage over time";
    case ElementType::Count:
        break;
    }
    return "";
}

auto pickupChoiceDescription(PickupType type, ElementType element, ElementMechanism mechanism)
    -> std::string
{
    switch (type)
    {
    case PickupType::LifeOrb:
        return "Instantly heals a small amount of health.";
    case PickupType::Shield:
        return "+1 shield stack (blocks one hit).";
    case PickupType::Regen:
        return "Heals gradually for a while.";
    case PickupType::MagnetPulse:
        return "Instantly pulls every pickup on screen to you.";
    case PickupType::Overcharge:
        return "+50% weapon damage for a while.";
    case PickupType::SecondWind:
        return "Permanent: survive the next otherwise-fatal hit.";
    case PickupType::Overdrive:
        return "Dash and shield-block cost no charge, for a while.";
    case PickupType::Elemental:
    {
        const auto effect = elementEffectSummary(element);
        switch (mechanism)
        {
        case ElementMechanism::Infusion:
            return std::format("Your hits apply {} to enemies for a while ({}).",
                               elementNames.at(static_cast<size_t>(element)), effect);
        case ElementMechanism::Nova:
            return std::format("Instantly applies {} to every enemy on screen ({}).",
                               elementNames.at(static_cast<size_t>(element)), effect);
        case ElementMechanism::Count:
            break;
        }
        return "";
    }
    case PickupType::XP:
    case PickupType::Danger:
    case PickupType::Count:
        break;
    }
    return "Applied instantly.";
}

auto evolutionHint(const Game& game, SkillType id) -> std::string
{

    for (size_t i = 0; i < weaponEvolves.size(); i++)
    {
        const auto kind = static_cast<WeaponType>(i);
        if (weaponEvolves.at(i) && skillLinkedPassive.at(i) == id && hasWeapon(game, kind))
        {
            return std::string(evolvedWeaponName.at(i));
        }
    }
    return "";
}

void drawAbilitySlots(const Game& game, int32_t x, int32_t y)
{
    constexpr float boxSize = 28;
    constexpr float gap = 34;
    constexpr float iconScale = 0.32F;

    struct Slot
    {
        SkillType id;
        Color color;
        int32_t level = 0;
        bool stolen = false;
        bool evolutionReady = false;
        bool evolved = false;
    };
    std::vector<Slot> slots;

    for (const auto& w : game.run.weapons)
    {
        const Color color = w.evolved ? Palette::Crit : Palette::Shield;
        const auto typeIndex = static_cast<size_t>(w.type);
        const bool evolutionReady = weaponEvolves.at(typeIndex) && !w.evolved && isEvolutionUnlocked(game, w.type);
        const bool stolen =
            game.run.weaponDowngrade.has_value() && game.run.weaponDowngrade->type == w.type;
        slots.push_back(Slot{.id = weaponGrantSkill.at(typeIndex),
                             .color = color,
                             .level = w.level,
                             .stolen = stolen,
                             .evolutionReady = evolutionReady,
                             .evolved = w.evolved});
    }

    for (size_t i = 0; i < static_cast<size_t>(SkillType::Count); i++)
    {
        const auto id = static_cast<SkillType>(i);
        if (game.run.skillLevels.at(i) == 0 || isFusedPassive(game, id))
        {
            continue;
        }
        if (weaponForGrantSkill(id).has_value())
        {
            continue;
        }
        slots.push_back(
            Slot{.id = id, .color = Palette::Shield, .level = game.run.skillLevels.at(i)});
    }

    const Vector2 mouse = mouseUIPos(game);
    std::optional<SkillType> hovered;
    Vector2 hoveredBoxPos{};

    for (int32_t i = 0; i < weaponSlotCap(game); i++)
    {
        const float bx = static_cast<float>(x) + static_cast<float>(i) * gap;
        const auto by = static_cast<float>(y);
        DrawRectangleLines(static_cast<int32_t>(bx), static_cast<int32_t>(by),
                           static_cast<int32_t>(boxSize), static_cast<int32_t>(boxSize),
                           Fade(Palette::StructMid, 0.6F));

        if (static_cast<size_t>(i) < slots.size())
        {
            const Slot& slot = slots.at(static_cast<size_t>(i));
            const Vector2 center{.x = bx + boxSize / 2, .y = by + boxSize / 2};
            drawSkillIcon(slot.id, center, boxSize * iconScale, slot.color, slot.evolved);

            if (slot.level > 0)
            {
                const std::string levelText = std::to_string(slot.level);
                constexpr int32_t levelFontSize = 10;
                const auto badgeX = static_cast<int32_t>(bx + boxSize) - 11;
                const auto badgeY = static_cast<int32_t>(by + boxSize) - 11;
                DrawRectangle(badgeX, badgeY, 11, 11, Fade(Palette::Void, 0.85F));
                drawText(game, levelText.c_str(), badgeX + 2, badgeY, levelFontSize, WHITE);
            }

            if (slot.stolen)
            {
                const auto tx = static_cast<int32_t>(bx);
                const auto ty = static_cast<int32_t>(by);
                DrawTriangle(Vector2{.x = static_cast<float>(tx), .y = static_cast<float>(ty)},
                             Vector2{.x = static_cast<float>(tx) + 9, .y = static_cast<float>(ty)},
                             Vector2{.x = static_cast<float>(tx), .y = static_cast<float>(ty) + 9},
                             Palette::Accent);
            }

            if (slot.evolutionReady)
            {
                const auto ax = bx + boxSize - 8;
                const auto ay = by + 1;
                DrawTriangle(Vector2{.x = ax, .y = ay + 7}, Vector2{.x = ax + 4, .y = ay},
                             Vector2{.x = ax + 8, .y = ay + 7}, Palette::Heal);
            }

            if (hoverActive(game) &&
                CheckCollisionPointRec(
                    mouse, Rectangle{.x = bx, .y = by, .width = boxSize, .height = boxSize}))
            {
                hovered = slot.id;
                hoveredBoxPos = Vector2{.x = bx, .y = by};
            }
        }
    }

    if (hovered.has_value())
    {
        const auto& def = Skills.at(static_cast<size_t>(*hovered));
        const int lvl = game.run.skillLevels.at(static_cast<size_t>(*hovered));
        std::string_view skillName = def.name;
        if (const auto weapon = weaponForGrantSkill(*hovered);
            weapon.has_value() && weaponEvolved(game, *weapon))
        {
            skillName = evolvedWeaponName.at(static_cast<size_t>(*weapon));
        }
        const std::string name = std::format("{} (Lv {})", skillName, lvl);
        const std::string desc(def.description);

        constexpr int32_t nameFontSize = 16;
        constexpr int32_t descFontSize = 13;
        const int32_t boxWidth = std::max(measureText(game, name.c_str(), nameFontSize),
                                          measureText(game, desc.c_str(), descFontSize)) +
                                 16;
        const auto scaledScreenWidth =
            static_cast<int32_t>(static_cast<float>(game.resources.screenWidth) / hudScale(game));
        auto tipX = static_cast<int32_t>(hoveredBoxPos.x);
        if (tipX + boxWidth > scaledScreenWidth)
        {
            tipX = scaledScreenWidth - boxWidth;
        }
        const int32_t tipY =
            static_cast<int32_t>(hoveredBoxPos.y) + static_cast<int32_t>(boxSize) + 6;

        DrawRectangle(tipX, tipY, boxWidth, 46, Fade(Palette::Void, 0.9F));
        DrawRectangleLines(tipX, tipY, boxWidth, 46, Palette::StructMid);
        drawText(game, name.c_str(), tipX + 8, tipY + 6, nameFontSize, Palette::Accent);
        drawText(game, desc.c_str(), tipX + 8, tipY + 26, descFontSize, Palette::StructLight);
    }
}

void drawLevelUp(const Game& game)
{
    applyGuiScale(game);
    const float scale = guiUiScale(game);
    beginPop(Vector2{.x = static_cast<float>(game.resources.windowWidth) / 2, .y = 117.0F * scale}, popScale(game, 0));
    windowText(game, std::format("LEVEL {}", game.run.level).c_str(),
               game.resources.windowWidth / 2, 100, 34, Palette::Accent);
    endPop();

    const auto count = static_cast<int32_t>(game.run.pendingChoices.size());
    constexpr int32_t nameFontSize = 24;
    constexpr int32_t descFontSize = 15;
    constexpr int32_t iconOffsetX = 24;
    constexpr float iconRadius = 11;

    for (int32_t i = 0; i < count; i++)
    {
        const auto& choice = game.run.pendingChoices.at(static_cast<size_t>(i));
        const Rectangle rect =
            menuColumnRect(game, i, count, MenuLayout::levelUpWidth, MenuLayout::levelUpHeight,
                           MenuLayout::levelUpGap, MenuLayout::levelUpMenuY);

        Color color = Palette::StructLight;
        std::string name;
        std::string desc;
        SkillType icon{};
        bool hasIcon = false;
        bool evolvedIcon = false;

        switch (choice.type)
        {
        case ChoiceType::Evolve:
            name = std::format("EVOLVE: {}",
                               evolvedWeaponName.at(static_cast<size_t>(choice.weapon.value())));
            desc = "Fuses the weapon and its linked passive into a super weapon.";
            icon = weaponGrantSkill.at(static_cast<size_t>(choice.weapon.value()));
            hasIcon = true;
            evolvedIcon = true;
            if (i == game.menuIndex)
            {
                color = Palette::Crit;
            }
            break;
        case ChoiceType::Pickup:
        {

            const auto found = std::ranges::find_if(pickupCatalog,
                                                    [&](const PickupCatalogEntry& e)
                                                    {
                                                        return e.type == choice.pickupType &&
                                                               e.element == choice.element &&
                                                               e.mechanism == choice.mechanism;
                                                    });
            name = found != pickupCatalog.end() ? std::string(found->name) : "Pickup";
            desc = pickupChoiceDescription(choice.pickupType, choice.element, choice.mechanism);
            if (choice.pickupType == PickupType::Elemental)
            {
                color = elementColors.at(static_cast<size_t>(choice.element));
            }
            break;
        }
        case ChoiceType::Skill:
        default:
        {
            const auto& def = Skills.at(static_cast<size_t>(choice.skill));
            const int lvl = game.run.skillLevels.at(static_cast<size_t>(choice.skill));
            std::string_view skillName = def.name;
            if (const auto weapon = weaponForGrantSkill(choice.skill);
                weapon.has_value() && weaponEvolved(game, *weapon))
            {
                skillName = evolvedWeaponName.at(static_cast<size_t>(*weapon));
            }
            name = std::format("{} (Lv {})", skillName, lvl + 1);
            desc = def.description;
            if (const auto hint = evolutionHint(game, choice.skill); !hint.empty())
            {
                desc = std::format("{}  ->  builds toward {}", desc, hint);
            }
            icon = choice.skill;
            hasIcon = true;
            break;
        }
        }

        if (i == game.menuIndex)
        {
            if (choice.type == ChoiceType::Evolve)
            {
                color = Palette::Crit;
            }
            else if (choice.type == ChoiceType::Pickup &&
                     choice.pickupType == PickupType::Elemental)
            {
                color = elementColors.at(static_cast<size_t>(choice.element));
            }
            else
            {
                color = Palette::Accent;
            }
        }

        beginPop(Vector2{.x = rect.x + rect.width / 2, .y = rect.y + rect.height / 2}, popScale(game, i + 1));
        const auto nameSize = static_cast<int32_t>(static_cast<float>(nameFontSize) * scale);
        const auto descSize = static_cast<int32_t>(static_cast<float>(descFontSize) * scale);
        const int32_t nameY = static_cast<int32_t>(rect.y + rect.height * 0.28F) - nameSize / 2;
        const int32_t descY = static_cast<int32_t>(rect.y + rect.height * 0.68F) - descSize / 2;
        const int32_t nameX = static_cast<int32_t>(rect.x + rect.width / 2) -
                              measureText(game, name.c_str(), nameSize) / 2;

        if (hasIcon)
        {
            drawSkillIcon(
                icon,
                Vector2{.x = static_cast<float>(nameX) - static_cast<float>(iconOffsetX) * scale,
                        .y = static_cast<float>(nameY) + static_cast<float>(nameSize) / 2},
                iconRadius * scale, color, evolvedIcon);
        }
        drawText(game, name.c_str(), nameX, nameY, nameSize, color);
        windowText(game, desc.c_str(), static_cast<int32_t>(rect.x + rect.width / 2),
                   static_cast<int32_t>(static_cast<float>(descY) / scale), descFontSize,
                   Palette::StructMid);
        endPop();
    }
}

void drawMenu(const Game& game, const std::string& heading, const std::vector<std::string>& options,
              int32_t y)
{
    applyGuiScale(game);
    if (!heading.empty())
    {
        beginPop(Vector2{.x = static_cast<float>(game.resources.windowWidth) / 2,
                         .y = static_cast<float>(y - 35) * guiUiScale(game)},
                 popScale(game, 0));
        windowText(game, heading.c_str(), game.resources.windowWidth / 2, y - 50, 30,
                   Palette::StructLight);
        endPop();
    }

    const auto count = static_cast<int32_t>(options.size());
    for (int32_t i = 0; i < count; i++)
    {
        const Rectangle rect = menuColumnRect(game, i, count, MenuLayout::buttonWidth,
                                              MenuLayout::buttonHeight, MenuLayout::buttonGap, y);
        if (i == game.menuIndex)
        {
            GuiSetState(STATE_FOCUSED);
        }
        beginPop(Vector2{.x = rect.x + rect.width / 2, .y = rect.y + rect.height / 2}, popScale(game, i + 1));
        GuiButton(rect, options.at(static_cast<size_t>(i)).c_str());
        endPop();
        GuiSetState(STATE_NORMAL);
    }
}

// x/y are unscaled (like windowText) - scaling happens internally, so the
// block's own height always stays proportional to the gap callers reserve
// for it, at any guiUiScale.
void drawHighScores(const Game& game, int32_t centerX, int32_t y, const std::vector<int32_t>& scores)
{
    windowText(game, "Top Scores", centerX, y, 26, Palette::AccentDim);

    if (scores.empty())
    {
        windowText(game, "(none yet)", centerX, y + 36, 22, Palette::StructMid);
        return;
    }

    for (size_t i = 0; i < scores.size(); i++)
    {
        const std::string line = std::format("{}. {}", i + 1, scores.at(i));
        windowText(game, line.c_str(), centerX, y + 36 + static_cast<int32_t>(i) * 28, 22,
                  Palette::StructLight);
    }
}

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
} // namespace

void drawBeltPlanet(const Game& game)
{
    constexpr int32_t zonesPerSide = 3;
    constexpr float visibility = 0.05F; // how far toward full colour the planet is drawn

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
    const Vector2 center = Vector2Add(player, Vector2Scale(rel, drift));
    const auto proj = [&](Vector3 v) { return Vector2{.x = center.x + v.x * radius, .y = center.y - v.y * radius}; };
    const Vector3 light = planet.light;
    const Color rock = planet.rock;
    const auto t = static_cast<float>(GetTime());
    const float pulse = 0.75F + 0.25F * std::sin(t * 1.3F + planet.pulsePhase);
    const Color emberHot = planet.coreHot;
    const Color emberDeep = ColorBrightness(planet.coreHot, -0.55F);

    // Debris ring: a tilted ellipse along the dust-lane angle, slowly orbiting. The far half
    // goes behind the body and the near half in front.
    const float laneRad = beltDustLaneAngleDeg * DEG2RAD;
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

void drawShatteredBeltBackground(const Game& game)
{
    if (currentBiome(game.run.waveNumber) != Biome::ShatteredBelt)
    {
        return;
    }

    const Vector2 player = game.run.player.position;
    const auto halfW = static_cast<float>(game.resources.screenWidth) / 2;
    const auto halfH = static_cast<float>(game.resources.screenHeight) / 2;
    const auto t = static_cast<float>(GetTime());
    const Color amber = Palette::ShatteredBeltAccent;

    // Mid (0.3): faint diagonal amber dust lanes, dashes drifting slowly along the lane.
    {
        constexpr float parallax = 0.3F;
        constexpr float laneSpacing = 700.0F;
        constexpr float dashLength = 240.0F;
        constexpr float driftSpeed = 18.0F;
        const float angle = beltDustLaneAngleDeg * DEG2RAD;
        const Vector2 along{.x = std::cos(angle), .y = std::sin(angle)};
        const Vector2 across{.x = -along.y, .y = along.x};
        const Vector2 layerCenter = Vector2Scale(player, parallax);
        const Vector2 shift = Vector2Scale(player, 1.0F - parallax);
        const float reach = std::sqrt(halfW * halfW + halfH * halfH) + dashLength;
        const float acrossCenter = Vector2DotProduct(layerCenter, across);
        const float alongCenter = Vector2DotProduct(layerCenter, along);
        const float drift = std::fmod(t * driftSpeed, dashLength);
        for (float k = std::floor((acrossCenter - reach) / laneSpacing); k * laneSpacing < acrossCenter + reach; k++)
        {
            // Irregular: about half the lanes are absent and the rest sit anywhere in their band.
            const float laneSeed = hashNoise(k * 3.1F + 11.0F);
            if (hashNoise(k * 8.3F + 4.0F) < 0.5F)
            {
                continue;
            }
            const float laneAcross = k * laneSpacing + (laneSeed - 0.5F) * laneSpacing * 0.9F;
            const float width = 1.5F + laneSeed * 2.0F;
            for (float d = std::floor((alongCenter - reach) / dashLength); d * dashLength < alongCenter + reach; d++)
            {
                const float density = hashNoise(k * 5.7F + d * 1.9F + 23.0F);
                if (density < 0.55F)
                {
                    continue;
                }
                const float start = d * dashLength + drift;
                const Vector2 a = Vector2Add(
                    shift, Vector2Add(Vector2Scale(across, laneAcross), Vector2Scale(along, start)));
                const Vector2 b = Vector2Add(a, Vector2Scale(along, dashLength * (0.3F + 0.65F * density)));
                DrawLineEx(a, b, width, Fade(amber, 0.03F + 0.05F * density));
            }
        }
    }
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

// Rustbloom mid (0.3) and near (0.8) layers. The far derelicts are drawn separately, before the
// stars.
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

            // A tendril carries the blend of its two pods' pulses.
            if (podPresence(x + cellSize, y) <= podChance)
            {
                const Vector2 neighbor{.x = x + cellSize * 1.5F + shift.x,
                                       .y = y + cellSize / 2 + shift.y};
                const float flow = (pulse + podPulse(x + cellSize, y)) / 2;
                DrawLineEx(podCenter, neighbor, 2.5F,
                           Fade(Palette::RustbloomHaze, 0.12F + 0.1F * flow));
            }
            if (podPresence(x, y + cellSize) <= podChance)
            {
                const Vector2 neighbor{.x = x + cellSize / 2 + shift.x,
                                       .y = y + cellSize * 1.5F + shift.y};
                const float flow = (pulse + podPulse(x, y + cellSize)) / 2;
                DrawLineEx(podCenter, neighbor, 2.5F,
                           Fade(Palette::RustbloomHaze, 0.12F + 0.1F * flow));
            }

            const float sizeSeed = hashNoise(x * 0.043F + y * 0.017F + 9.0F);
            const float podRadius = cellSize * (0.14F + sizeSeed * 0.06F) * (0.94F + 0.06F * pulse);
            DrawCircleV(podCenter, podRadius, Fade(Palette::RustbloomAccent, 0.22F));
            DrawCircleV(
                Vector2Add(podCenter, Vector2{.x = podRadius * 0.5F, .y = podRadius * 0.3F}),
                podRadius * 0.6F, Fade(Palette::RustbloomHaze, 0.2F));
            DrawCircleV(podCenter, podRadius * 0.3F, Fade(Palette::Heal, 0.2F + 0.3F * pulse));
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

    const float angle = beltDustLaneAngleDeg * DEG2RAD;
    const Vector2 along{.x = std::cos(angle) * game.run.debrisSquallDir,
                        .y = std::sin(angle) * game.run.debrisSquallDir};
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

void drawGameplayWorld(const Game& game)
{
    if (currentBiome(game.run.waveNumber) == Biome::ShatteredBelt)
    {
        drawBeltPlanet(game); // before the stars, so they shine in front of it
    }
    else if (currentBiome(game.run.waveNumber) == Biome::Rustbloom)
    {
        drawRustbloomDerelicts(game); // likewise
    }
    drawBackgroundStars(game, game.run.player.position);
    drawPunctumLightning(game);
    drawShatteredBeltBackground(game);
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
            DrawCircleV(p.position, p.radius, Palette::BossSpread);
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
        if (b.streak > 0)
        {
            drawLaserBolt(b);
            continue;
        }
        if (b.phasing)
        {
            DrawCircleV(b.position, b.radius * 1.6F, Fade(WHITE, 0.35F));
        }
        DrawCircleV(b.position, b.radius, b.color);
    }

    for (const auto& p : game.run.deathParticles)
    {
        DrawCircleV(p.position, p.radius, Fade(p.color, p.life / p.maxLife));
    }

    // Dash afterimages: sharp wedges pointing back along the dash, alternating a little side to
    // side so the trail reads spiky; each shrinks and fades.
    for (size_t i = 0; i < game.run.dashWedges.size(); i++)
    {
        const auto& w = game.run.dashWedges.at(i);
        const float fade = w.life / w.maxLife;
        const Vector2 side{.x = -w.velocity.y, .y = w.velocity.x};
        const float jog = (i % 2 == 0 ? 1.0F : -1.0F) * 4.0F;
        const Vector2 base = Vector2Add(w.position, Vector2Scale(side, jog));
        const float halfWidth = 10.0F * fade;
        const Vector2 left = Vector2Add(base, Vector2Scale(side, halfWidth));
        const Vector2 right = Vector2Subtract(base, Vector2Scale(side, halfWidth));
        const Vector2 apex = Vector2Subtract(base, Vector2Scale(w.velocity, w.radius * (0.4F + 0.6F * fade)));
        DrawTriangle(left, apex, right, Fade(w.color, 0.75F * fade));
        DrawTriangle(left, right, apex, Fade(w.color, 0.75F * fade));
        DrawLineEx(left, apex, 1.0F, Fade(Palette::Crit, 0.7F * fade));
        DrawLineEx(right, apex, 1.0F, Fade(Palette::Crit, 0.7F * fade));
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

    if (game.run.player.dashing && !game.run.player.shieldActive &&
        Vector2Length(game.run.player.dashVelocity) > 0.01F)
    {
        // Dash bow-wave: nested pointed arches wrapped round the ship, open at the back, with
        // streaks running off behind. Matte gold layers; only the leading edge is bright.
        const Vector2 fwd = Vector2Normalize(game.run.player.dashVelocity);
        const Vector2 side{.x = -fwd.y, .y = fwd.x};
        const float t = static_cast<float>(GetTime());
        const auto archPoint = [&](float s, float scale)
        {
            // s in [-1, 1] across; |s|^1.6 bends the sides into a pointed arch.
            const float size = r * 1.6F * scale;
            const float ahead = (2.6F - 4.6F * std::pow(std::fabs(s), 1.6F)) * size;
            return Vector2Add(p, Vector2Add(Vector2Scale(fwd, ahead), Vector2Scale(side, s * 1.7F * size)));
        };
        constexpr int32_t steps = 16;
        for (const auto& [scale, alpha] : {std::pair{1.15F, 0.2F}, std::pair{0.95F, 0.26F}, std::pair{0.75F, 0.34F}})
        {
            for (int32_t i = 0; i < steps; i++)
            {
                const float s0 = -1.0F + 2.0F * static_cast<float>(i) / steps;
                const float s1 = -1.0F + 2.0F * static_cast<float>(i + 1) / steps;
                const Vector2 a = archPoint(s0, scale);
                const Vector2 b = archPoint(s1, scale);
                DrawTriangle(p, a, b, Fade(Palette::Charge, alpha));
                DrawTriangle(p, b, a, Fade(Palette::Charge, alpha));
            }
        }
        for (int32_t i = 0; i < steps; i++)
        {
            // The leading edge, brightest at the apex.
            const float s0 = -1.0F + 2.0F * static_cast<float>(i) / steps;
            const float s1 = -1.0F + 2.0F * static_cast<float>(i + 1) / steps;
            const float bright = 1.0F - std::fabs(s0 + s1) * 0.5F;
            DrawLineEx(archPoint(s0, 1.15F), archPoint(s1, 1.15F), 1.5F, Fade(Palette::Crit, 0.25F + 0.6F * bright));
        }
        for (int32_t k = 0; k < 5; k++)
        {
            // Streaks off the back, flickering in length.
            const float across = (-1.0F + 0.5F * static_cast<float>(k)) * 1.3F * r;
            const float len = r * (2.0F + 1.5F * std::fabs(std::sin(t * 23.0F + static_cast<float>(k) * 1.7F)));
            const Vector2 start = Vector2Add(Vector2Subtract(p, Vector2Scale(fwd, r * 1.2F)), Vector2Scale(side, across));
            DrawLineEx(start, Vector2Subtract(start, Vector2Scale(fwd, len)), 1.5F, Fade(Palette::Charge, 0.45F));
        }
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
        DrawCircleV(proj.position, proj.radius, Fade(Palette::Accent, 0.55F));
        DrawCircleLines(static_cast<int32_t>(proj.position.x),
                        static_cast<int32_t>(proj.position.y), proj.radius, Fade(WHITE, 0.8F));
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
            DrawCircleV(bladePos, 7, Palette::Shield);
        }
    }
}

void drawEnemyShape(EnemyShape shape, Vector2 center, float radius, float rotationDeg, Color color)
{
    switch (shape)
    {
    case EnemyShape::Circle:
        DrawCircleV(center, radius, color);
        break;
    case EnemyShape::Triangle:
        DrawPoly(center, 3, radius, rotationDeg - 90, color);
        break;
    case EnemyShape::Square:
        DrawPoly(center, 4, radius, rotationDeg + 45, color);
        break;
    case EnemyShape::Diamond:
        DrawPoly(center, 4, radius, rotationDeg, color);
        break;
    case EnemyShape::Pentagon:
        DrawPoly(center, 5, radius, rotationDeg, color);
        break;
    case EnemyShape::Hexagon:
        DrawPoly(center, 6, radius, rotationDeg, color);
        break;
    case EnemyShape::Octagon:
        DrawPoly(center, 8, radius, rotationDeg, color);
        break;
    case EnemyShape::Ring:
        DrawRing(center, radius * 0.5F, radius, 0, 360, 24, color);
        DrawCircleV(center, radius * 0.25F, color);
        break;
    case EnemyShape::Star:
    {
        constexpr size_t points = 5;
        const float rot = rotationDeg * DEG2RAD;
        std::array<Vector2, points * 2> verts{};
        for (size_t i = 0; i < points * 2; i++)
        {
            const float rad = (i % 2 == 0) ? radius : radius * 0.42F;
            const float a = rot + static_cast<float>(i) * std::numbers::pi_v<float> /
                                      static_cast<float>(points);
            verts.at(static_cast<size_t>(i)) =
                Vector2{.x = center.x + std::cos(a) * rad, .y = center.y + std::sin(a) * rad};
        }
        for (size_t i = 0; i < verts.size(); i++)
        {

            DrawTriangle(center, verts.at((i + 1) % verts.size()), verts.at(i), color);
        }
        break;
    }
    case EnemyShape::Organic:
    case EnemyShape::Cluster:

        break;
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

    if (kind.pattern == EnemyPattern::Turret &&
        Vector2Distance(enemy.position, game.run.player.position) < 700.0F)
    {
        float blink = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 12.0F);
        DrawLineEx(enemy.position, game.run.player.position, 2, Fade(Palette::Crit, blink * 0.5F));
    }

    const float spin = std::fmod(static_cast<float>(GetTime()) * 20.0F +
                                     (enemy.position.x + enemy.position.y) * 0.15F,
                                 360.0F);
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
        drawEnemyShape(kind.shape, enemy.position, kind.radius, spin, color);
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

    DrawCircleLines(static_cast<int32_t>(hazard.position.x),
                    static_cast<int32_t>(hazard.position.y), EliteHazardConstants::auraRadius,
                    Fade(auraColor, 0.12F));

    for (int i = 0; i < 6; i++)
    {
        const float angle = static_cast<float>(i) * 60.0F * DEG2RAD;
        const Vector2 tip = Vector2Add(
            hazard.position, Vector2{.x = std::cos(angle) * (EliteHazardConstants::radius + 8),
                                     .y = std::sin(angle) * (EliteHazardConstants::radius + 8)});
        DrawLineEx(hazard.position, tip, 3, Fade(auraColor, 0.7F));
    }

    DrawCircleV(hazard.position, EliteHazardConstants::radius, body);
    DrawCircleLines(static_cast<int32_t>(hazard.position.x),
                    static_cast<int32_t>(hazard.position.y), EliteHazardConstants::radius,
                    auraColor);

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

void drawBossHull(BossShape shape, Vector2 center, Vector2 size, Color color)
{
    const auto cx = static_cast<int32_t>(center.x);
    const auto cy = static_cast<int32_t>(center.y);
    const float spin = std::fmod(static_cast<float>(GetTime()) * 8.0F, 360.0F);

    switch (shape)
    {
    case BossShape::Saucer:
        DrawEllipse(cx, cy + static_cast<int32_t>(size.y / 4), size.x / 2, size.y / 4, color);
        DrawCircle(cx, cy - static_cast<int32_t>(size.y / 8), size.x / 3.5F, Fade(color, 0.8F));
        break;
    case BossShape::SpikedRing:
    {
        DrawEllipse(cx, cy + static_cast<int32_t>(size.y / 4), size.x / 2, size.y / 4, color);
        DrawCircle(cx, cy - static_cast<int32_t>(size.y / 8), size.x / 3.5F, Fade(color, 0.8F));
        constexpr int spikeCount = 8;
        for (int i = 0; i < spikeCount; i++)
        {
            const float a = (static_cast<float>(i) * 360.0F / spikeCount + spin) * DEG2RAD;
            const Vector2 dir{.x = std::cos(a), .y = std::sin(a)};
            const Vector2 perp{.x = -dir.y, .y = dir.x};
            const Vector2 base = Vector2Add(center, Vector2Scale(dir, size.x * 0.46F));
            const Vector2 tip = Vector2Add(center, Vector2Scale(dir, size.x * 0.64F));
            const Vector2 sideA = Vector2Add(base, Vector2Scale(perp, size.x * 0.05F));
            const Vector2 sideB = Vector2Subtract(base, Vector2Scale(perp, size.x * 0.05F));
            DrawTriangle(sideA, tip, sideB, color);
        }
        break;
    }
    case BossShape::TwinDome:
        DrawEllipse(cx, cy + static_cast<int32_t>(size.y / 4), size.x / 2, size.y / 4, color);
        DrawCircle(cx - static_cast<int32_t>(size.x / 4), cy - static_cast<int32_t>(size.y / 10),
                   size.x / 4.5F, Fade(color, 0.8F));
        DrawCircle(cx + static_cast<int32_t>(size.x / 4), cy - static_cast<int32_t>(size.y / 10),
                   size.x / 4.5F, Fade(color, 0.8F));
        break;
    case BossShape::Crystal:
        DrawPoly(center, 6, size.x / 2, spin, color);
        DrawPolyLines(center, 6, size.x / 2 * 1.18F, -spin * 0.6F, Fade(color, 0.5F));
        DrawPoly(center, 6, size.x / 4, spin * 1.4F, Fade(Palette::StructLight, 0.6F));
        break;
    case BossShape::HexPlated:
        DrawPoly(center, 6, size.x / 2, 0, color);
        DrawPolyLines(center, 6, size.x / 2, 0, Fade(Palette::StructDark, 0.8F));
        DrawPolyLines(center, 6, size.x / 2 * 0.65F, 0, Fade(Palette::StructDark, 0.6F));
        break;
    case BossShape::Segment:

        DrawCircle(cx, cy, size.x / 2, color);
        DrawCircle(cx, cy, size.x / 2 * 0.55F, Fade(Palette::StructLight, 0.3F));
        DrawCircleLines(cx, cy, size.x / 2, Fade(Palette::StructDark, 0.6F));
        break;
    }
}

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

const Color wreckwormSlime = ColorLerp(Palette::RustbloomAccent, Palette::RustbloomVoid, 0.5F);

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
        const auto t = static_cast<float>(GetTime());
        const bool winding = boss.state == BossState::WINDING_UP;
        const bool shooting = boss.state == BossState::SHOOTING;
        const float windup =
            winding ? std::clamp(1.0F - boss.stateTimer / (boss.attack == BossAttack::Barrage
                                                                  ? slagSprayTell
                                                                  : bossWindupDuration(boss.attack)),
                                 0.0F, 1.0F)
                    : 0.0F;
        const bool swallowing = game.run.qteBossId == boss.instanceId;
        float mawOpen = 0.15F + 0.08F * std::sin(t * 1.5F);
        float heat = 0.15F;
        if (swallowing || (shooting && boss.attack == BossAttack::Devour))
        {
            mawOpen = 1.0F;
            heat = 1.0F;
        }
        else if (winding && boss.attack == BossAttack::Devour)
        {
            mawOpen = 0.3F + 0.7F * windup; // the maw glows gold for the tell
            heat = windup;
        }
        else if (boss.attack == BossAttack::Barrage && (winding || shooting))
        {
            mawOpen = winding ? 0.6F * windup : 0.7F; // Slag Spray: the body flares gold
            heat = winding ? windup : 0.8F;
        }
        if (boss.slagmawPunishTimer > 0)
        {
            mawOpen = 0.5F + 0.25F * std::sin(t * 22.0F); // choking
            heat = 0.4F + 0.4F * std::sin(t * 9.0F);
        }
        Vector2 center = bossCenter;
        if (swallowing)
        {
            center = Vector2Add(center, Vector2{.x = std::sin(t * 37.0F) * 6.0F, .y = std::cos(t * 29.0F) * 6.0F});
        }
        if (shooting && boss.attack == BossAttack::Devour)
        {
            drawDevourCone(slagmawMawCenter(boss), boss.devourDir);
        }
        const float flash = boss.hitFlashTimer > 0 ? boss.hitFlashTimer / UpdateConstants::hitFlashDuration : 0.0F;
        drawSlagmawShape(center, boss.size.x / 2, static_cast<float>(boss.instanceId), boss.slagmawFacing,
                         mawOpen, heat, flash, boss.ghostFadeAlpha);
        if (swallowing && game.run.qtePrompt != QtePrompt::None && game.run.qtePrompt != QtePrompt::Mash &&
            game.run.qteTimerMax > 0)
        {
            // The slag gob the current prompt answers, flying in from its angle.
            const float left = std::clamp(game.run.qteTimer / game.run.qteTimerMax, 0.0F, 1.0F);
            const Vector2 gob = Vector2Subtract(game.run.player.position, Vector2Scale(game.run.qteThreatDir, 240.0F * left));
            drawSlagGob(gob, 13.0F, game.run.qteThreatDir);
        }
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
        const float progress =
            std::clamp(1 - boss.stateTimer / bossWindupDuration(boss.attack), 0.0F, 1.0F);
        drawChargeParticles(bossCenter, progress, boss.color, 100, 65);

        if (boss.isWreckwormHead && !boss.krakenSnakeVariant && boss.attack == BossAttack::Barrage)
        {
            // Spore Spit tell: a pulsing green ring round the flaring head.
            const float pulse = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 24.0F);
            const float r = boss.size.x / 2;
            DrawRing(bossCenter, r * 1.15F, r * (1.25F + 0.1F * pulse), 0, 360, 48,
                     ColorLerp(Palette::RustbloomAccent, WHITE, 0.2F + 0.3F * pulse));
        }

        if (boss.attack == BossAttack::Beam || boss.attack == BossAttack::WormholeBeam)
        {
            // A thin tell line from the beam's origin through the player, firming up to the shot.
            const Vector2 origin = boss.attack == BossAttack::WormholeBeam && Vector2LengthSqr(boss.wormholeBeamOrigin) > 0
                                       ? boss.wormholeBeamOrigin
                                       : bossCenter;
            const Vector2 through = Vector2Normalize(Vector2Subtract(game.run.player.position, origin));
            DrawLineEx(origin, Vector2Add(origin, Vector2Scale(through, 2000.0F)), 1.0F + progress,
                       Fade(Palette::Crit, 0.15F + 0.45F * progress));
            const float blink = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 10.0F);
            const float markerRadius = game.run.player.radius + 10 + 4 * progress;
            DrawCircleLines(static_cast<int32_t>(game.run.player.position.x),
                            static_cast<int32_t>(game.run.player.position.y), markerRadius,
                            Fade(Palette::Crit, blink));
            DrawCircleLines(static_cast<int32_t>(game.run.player.position.x),
                            static_cast<int32_t>(game.run.player.position.y), markerRadius * 0.7F,
                            Fade(Palette::Accent, blink * 0.7F));
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

void drawOrbitBlades(const Game& game)
{
    for (const auto& w : game.run.weapons)
    {
        const Color color = w.evolved ? Palette::Crit : Palette::Shield;

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
                DrawCircleV(pos, 5, color);
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
        const float angle = static_cast<float>(GetTime()) * 720.0F;
        DrawPoly(proj.position, 4, proj.radius, angle, Palette::Shield);
        DrawCircleV(proj.position, proj.radius * 0.4F, Fade(WHITE, 0.8F));
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
void drawDevourCone(Vector2 maw, Vector2 dir)
{
    constexpr Color gold{.r = 255, .g = 188, .b = 72, .a = 255};
    const auto t = static_cast<float>(GetTime());
    const float half = devourConeHalfDeg * DEG2RAD;
    const Vector2 a = Vector2Add(maw, Vector2Scale(Vector2Rotate(dir, half), devourRange));
    const Vector2 b = Vector2Add(maw, Vector2Scale(Vector2Rotate(dir, -half), devourRange));
    DrawTriangle(maw, a, b, Fade(gold, 0.06F));
    DrawTriangle(maw, b, a, Fade(gold, 0.06F));
    DrawLineEx(maw, a, 2.0F, Fade(gold, 0.22F));
    DrawLineEx(maw, b, 2.0F, Fade(gold, 0.22F));
    for (int32_t k = 0; k < 14; k++)
    {
        const auto kf = static_cast<float>(k);
        const float angle = (hashNoise(kf * 3.1F) - 0.5F) * 2.0F * half;
        const float frac = std::fmod(t * 1.4F + hashNoise(kf * 7.7F), 1.0F); // 0 far .. 1 at the maw
        const Vector2 along = Vector2Rotate(dir, angle * (1.0F - frac * 0.7F));
        const Vector2 p = Vector2Add(maw, Vector2Scale(along, devourRange * (1.0F - frac)));
        DrawLineEx(p, Vector2Add(p, Vector2Scale(along, 46.0F)), 2.0F, Fade(gold, 0.55F * frac));
    }
}

// Slagmaw: a jagged basalt mass, every facet lit from the upper left, split by molten gold seams
// running back from a maw that faces `facing`. mawOpen 0..1 parts the toothed jaws on a glowing
// throat; heat 0..1 brightens the seams, throat and haze (tells, Devour).
void drawSlagmawShape(Vector2 center, float radius, float seed, Vector2 facing, float mawOpen, float heat,
                      float flash, float alpha)
{
    const auto t = static_cast<float>(GetTime());
    const auto paint = [&](Color c, float a) { return Fade(ColorLerp(c, WHITE, 0.2F * flash), a * alpha); };
    constexpr Color basalt{.r = 34, .g = 27, .b = 27, .a = 255};
    constexpr Color basaltLit{.r = 104, .g = 84, .b = 76, .a = 255}; // warm, catching the light
    constexpr Color gold{.r = 255, .g = 188, .b = 72, .a = 255};
    const Color ember = Palette::ElementBurn;
    const float glow = std::clamp(0.35F + 0.2F * std::sin(t * 2.0F + seed) + 0.6F * heat, 0.0F, 1.0F);
    const Vector2 face = Vector2Length(facing) > 0.01F ? Vector2Normalize(facing) : Vector2{.x = 0, .y = 1};
    const Vector2 side{.x = -face.y, .y = face.x};
    const Vector2 light = Vector2Normalize(Vector2{.x = -1, .y = -1});

    DrawCircleV(center, radius * 1.2F, paint(ember, 0.05F + 0.1F * heat));
    DrawCircleV(center, radius * 1.07F, paint(ember, 0.07F + 0.12F * heat));

    constexpr int32_t sides = 22;
    std::array<Vector2, sides> verts{};
    for (int32_t i = 0; i < sides; i++)
    {
        const float a = static_cast<float>(i) * 2.0F * PI / sides;
        const float r = radius * (0.86F + 0.14F * hashNoise(seed * 5.3F + static_cast<float>(i) * 1.7F));
        verts.at(static_cast<size_t>(i)) = Vector2Add(center, Vector2{.x = std::cos(a) * r, .y = std::sin(a) * r});
    }
    for (int32_t i = 0; i < sides; i++)
    {
        const Vector2 v0 = verts.at(static_cast<size_t>(i));
        const Vector2 v1 = verts.at(static_cast<size_t>((i + 1) % sides));
        const float lit = 0.5F + 0.5F * Vector2DotProduct(
                                           Vector2Normalize(Vector2Subtract(Vector2Lerp(v0, v1, 0.5F), center)), light);
        DrawTriangle(center, v1, v0, paint(ColorLerp(basalt, basaltLit, 0.65F * lit * lit), 1.0F));
        DrawLineEx(v0, v1, radius * 0.018F, paint(basaltLit, 0.5F * lit));
    }

    // Raised plates, kept off the maw side.
    for (int32_t k = 0; k < 6; k++)
    {
        const auto kf = static_cast<float>(k);
        const float a = hashNoise(seed * 2.3F + kf * 4.1F) * 2.0F * PI;
        const Vector2 dir{.x = std::cos(a), .y = std::sin(a)};
        if (Vector2DotProduct(dir, face) > 0.3F)
        {
            continue;
        }
        const Vector2 pos = Vector2Add(center, Vector2Scale(dir, radius * (0.3F + 0.35F * hashNoise(seed + kf))));
        const float size = radius * (0.1F + 0.06F * hashNoise(seed * 1.9F + kf));
        DrawPoly(pos, 5, size, a * RAD2DEG, paint(ColorLerp(basalt, basaltLit, 0.45F), 1.0F));
        DrawPolyLinesEx(pos, 5, size, a * RAD2DEG, 1.5F, paint(Palette::Void, 0.6F));
    }

    // Molten seams: jagged veins, each starting at its own point round the back of the maw and
    // running out toward the rim.
    const Vector2 mawCenter = Vector2Add(center, Vector2Scale(face, radius * slagmawMawOffset));
    const float backAngle = std::atan2(-face.y, -face.x);
    for (int32_t k = 0; k < 6; k++)
    {
        const auto kf = static_cast<float>(k);
        float a = backAngle + (kf - 2.5F) * 0.55F;
        Vector2 p = Vector2Add(mawCenter, Vector2{.x = std::cos(a) * radius * 0.42F, .y = std::sin(a) * radius * 0.42F});
        for (int32_t seg = 0; seg < 3; seg++)
        {
            a += (hashNoise(seed * 3.3F + kf * 5.0F + static_cast<float>(seg)) - 0.5F) * 0.8F;
            const Vector2 q = Vector2Add(p, Vector2{.x = std::cos(a) * radius * 0.22F, .y = std::sin(a) * radius * 0.22F});
            const float taper = 1.0F - static_cast<float>(seg) * 0.22F;
            DrawLineEx(p, q, radius * 0.05F * taper, paint(ember, 0.35F * glow));
            DrawLineEx(p, q, radius * 0.022F * taper, paint(gold, glow));
            p = q;
        }
        DrawCircleV(p, radius * 0.025F, paint(gold, glow));
    }

    // The maw: jaws parting on a glowing throat, ringed by basalt teeth.
    const Vector2 maw = Vector2Add(center, Vector2Scale(face, radius * slagmawMawOffset));
    const float halfWidth = radius * (0.34F + 0.06F * mawOpen);
    const float halfDepth = radius * (0.05F + 0.2F * mawOpen);
    const auto point = [&](float a, float s)
    {
        return Vector2Add(maw, Vector2Add(Vector2Scale(side, std::cos(a) * halfWidth * s),
                                          Vector2Scale(face, std::sin(a) * halfDepth * s)));
    };
    constexpr int32_t mawVerts = 24;
    const auto ellipse = [&](float s, Color c)
    {
        for (int32_t i = 0; i < mawVerts; i++)
        {
            const float a0 = static_cast<float>(i) * 2.0F * PI / mawVerts;
            const float a1 = static_cast<float>(i + 1) * 2.0F * PI / mawVerts;
            DrawTriangle(maw, point(a1, s), point(a0, s), c);
            DrawTriangle(maw, point(a0, s), point(a1, s), c);
        }
    };
    ellipse(1.14F, paint(ColorLerp(basalt, Palette::Void, 0.4F), 1.0F));
    ellipse(1.0F, paint(Palette::Void, 1.0F));
    ellipse(0.62F, paint(gold, (0.25F + 0.65F * heat) * mawOpen));
    // Four uneven fangs per jaw.
    for (const float lip : {0.0F, PI})
    {
        for (int32_t k = 1; k <= 4; k++)
        {
            const auto kf = static_cast<float>(k);
            const float a = lip + PI * kf / 5.0F;
            const Vector2 b1 = point(a - PI / 12.0F, 1.05F);
            const Vector2 b2 = point(a + PI / 12.0F, 1.05F);
            const Vector2 tip = Vector2Lerp(point(a, 1.0F), maw, 0.35F + 0.3F * hashNoise(seed + lip + kf));
            const Color tooth = paint(ColorLerp(basalt, Palette::Haze, 0.35F), 1.0F);
            DrawTriangle(b1, tip, b2, tooth);
            DrawTriangle(b1, b2, tip, tooth);
        }
    }
}

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

void drawKrakenPortal(Vector2 portal, float t, float open = 1.0F)
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

void drawBossHealthBars(const Game& game)
{
    const auto scaledScreenWidth =
        static_cast<int32_t>(static_cast<float>(game.resources.screenWidth) / hudScale(game));
    const auto scaledScreenHeight =
        static_cast<int32_t>(static_cast<float>(game.resources.screenHeight) / hudScale(game));

    constexpr int32_t barWidth = 520;
    constexpr int32_t barHeight = 20;
    constexpr int32_t barGap = 8;
    const int32_t barX = (scaledScreenWidth - barWidth) / 2;
    int32_t barY = scaledScreenHeight - 70;

    if (currentBiome(game.run.waveNumber) == Biome::SolarForge)
    {
        const float heatFrac =
            std::clamp(game.run.solarForgeHeatMeter / solarForgeMeltThreshold, 0.0F, 1.0F);
        const bool melting = game.run.solarForgeHeatMeter > solarForgeMeltThreshold;
        DrawRectangle(barX, barY, barWidth, barHeight, Fade(Palette::StructMid, 0.5F));
        const Color fireColor = ColorLerp(Palette::ElementBurn, Palette::Crit, heatFrac);
        DrawRectangle(barX, barY, static_cast<int32_t>(static_cast<float>(barWidth) * heatFrac),
                      barHeight, fireColor);
        DrawRectangleLines(barX, barY, barWidth, barHeight, Palette::StructDark);
        if (melting)
        {
            constexpr const char* meltingLabel = "MELTING";
            const auto textWidth =
                static_cast<int32_t>(MeasureTextEx(game.resources.font, meltingLabel, 16, 1).x);
            const float pulse = 0.6F + 0.4F * std::sin(static_cast<float>(GetTime()) * 10.0F);
            drawText(game, meltingLabel, barX + (barWidth - textWidth) / 2, barY + 1, 16,
                     Fade(Palette::StructLight, pulse));
        }
        barY -= barHeight + barGap;
    }

    for (const auto& boss : game.run.bosses)
    {
        if (boss.health <= 0 || boss.isBeltbreakerPlate || boss.isWreckwormSegment)
        {
            continue;
        }

        const float frac = std::clamp(
            static_cast<float>(boss.health) / static_cast<float>(boss.maxHealth), 0.0F, 1.0F);
        DrawRectangle(barX, barY, barWidth, barHeight, Fade(Palette::StructMid, 0.5F));
        DrawRectangle(barX, barY, static_cast<int32_t>(static_cast<float>(barWidth) * frac),
                      barHeight, boss.color);
        DrawRectangleLines(barX, barY, barWidth, barHeight, Palette::StructDark);

        if (boss.isBeltbreaker && boss.beltbreakerShielded)
        {
            constexpr const char* shieldedLabel = "SHIELDED";
            const auto textWidth =
                static_cast<int32_t>(MeasureTextEx(game.resources.font, shieldedLabel, 16, 1).x);
            drawText(game, shieldedLabel, barX + (barWidth - textWidth) / 2, barY + 1, 16,
                     Palette::StructLight);
        }

        barY -= barHeight + barGap;
    }
}

// Boss QTE cutscene HUD ----------------------------------------------------

void drawQteArrow(Vector2 center, Vector2 dir, float size, Color color)
{
    const Vector2 perp{.x = -dir.y, .y = dir.x};
    const Vector2 tip = Vector2Add(center, Vector2Scale(dir, size));
    const Vector2 back = Vector2Subtract(center, Vector2Scale(dir, size * 0.6F));
    DrawTriangle(tip, Vector2Subtract(back, Vector2Scale(perp, size * 0.7F)),
                 Vector2Add(back, Vector2Scale(perp, size * 0.7F)), color);
    DrawTriangle(tip, Vector2Add(back, Vector2Scale(perp, size * 0.7F)),
                 Vector2Subtract(back, Vector2Scale(perp, size * 0.7F)), color);
}

// A keyboard key that bobs down on the mash beat (press 0..1).
void drawQteKeycap(const Game& game, const char* label, Vector2 center, float width, float press,
                   Color color)
{
    constexpr float height = 56.0F;
    constexpr float depth = 8.0F;
    const float x = center.x - width / 2;
    const float y = center.y - height / 2;
    DrawRectangleRec(Rectangle{.x = x, .y = y + depth, .width = width, .height = height},
                     Fade(BLACK, 0.8F));
    const Rectangle top{.x = x, .y = y + depth * press, .width = width, .height = height};
    DrawRectangleRec(top, Palette::StructDark);
    DrawRectangleLinesEx(top, 3.0F, color);
    const int32_t textWidth = measureText(game, label, 24);
    drawText(game, label, static_cast<int32_t>(center.x) - textWidth / 2,
             static_cast<int32_t>(top.y + height / 2 - 12), 24, color);
}

// A mouse with the asked buttons flashing.
void drawQteMouse(Vector2 center, bool left, bool right, float press, Color color)
{
    constexpr float width = 52.0F;
    constexpr float height = 76.0F;
    const Rectangle body{.x = center.x - width / 2,
                         .y = center.y - height / 2,
                         .width = width,
                         .height = height};
    const float buttonHeight = height * 0.42F;
    DrawRectangleRec(body, Palette::StructDark);
    const Color lit = Fade(color, 0.35F + 0.65F * press);
    if (left)
    {
        DrawRectangleRec(
            Rectangle{.x = body.x, .y = body.y, .width = width / 2, .height = buttonHeight}, lit);
    }
    if (right)
    {
        DrawRectangleRec(Rectangle{.x = center.x, .y = body.y, .width = width / 2,
                                   .height = buttonHeight},
                         lit);
    }
    DrawRectangleLinesEx(body, 3.0F, color);
    DrawLineEx(Vector2{.x = body.x, .y = body.y + buttonHeight},
               Vector2{.x = body.x + width, .y = body.y + buttonHeight}, 3.0F, color);
    DrawLineEx(Vector2{.x = center.x, .y = body.y},
               Vector2{.x = center.x, .y = body.y + buttonHeight}, 3.0F, color);
}

// Touch: a round button named like the on-screen control it stands for.
void drawQteTouchButton(const Game& game, const char* label, Vector2 center, float press,
                        Color color)
{
    const float radius = 38.0F + 4.0F * press;
    DrawCircleV(center, radius, Fade(color, 0.2F + 0.35F * press));
    DrawCircleLinesV(center, radius, color);
    const int32_t textWidth = measureText(game, label, 16);
    drawText(game, label, static_cast<int32_t>(center.x) - textWidth / 2,
             static_cast<int32_t>(center.y) - 8, 16, color);
}

void drawQteButtons(const Game& game, QtePrompt prompt, Vector2 center, float press, Color color)
{
    const bool touch = touchInputActive(game);
    switch (prompt)
    {
    case QtePrompt::Up:
    case QtePrompt::Left:
    case QtePrompt::Right:
    case QtePrompt::Down:
    {
        static constexpr std::array<const char*, 4> keys{"W", "A", "D", "S"};
        static constexpr std::array<Vector2, 4> dirs{
            Vector2{.x = 0, .y = -1}, Vector2{.x = -1, .y = 0}, Vector2{.x = 1, .y = 0},
            Vector2{.x = 0, .y = 1}};
        const auto index = static_cast<size_t>(prompt) - static_cast<size_t>(QtePrompt::Up);
        if (touch)
        {
            // Flick the move stick that way.
            DrawCircleLinesV(center, 40.0F, color);
            drawQteArrow(Vector2Add(center, Vector2Scale(dirs.at(index), 10.0F * press)),
                         dirs.at(index), 24.0F, color);
        }
        else
        {
            drawQteKeycap(game, keys.at(index), center, 56.0F, press, color);
            drawQteArrow(Vector2Add(center, Vector2Scale(dirs.at(index), 62.0F + 8.0F * press)),
                         dirs.at(index), 14.0F, color);
        }
        break;
    }
    case QtePrompt::Shield:
        touch ? drawQteTouchButton(game, "SHIELD", center, press, color)
              : drawQteMouse(center, false, true, press, color);
        break;
    case QtePrompt::Dash:
        touch ? drawQteTouchButton(game, "DASH", center, press, color)
              : drawQteMouse(center, true, false, press, color);
        break;
    case QtePrompt::Parry:
        touch ? drawQteTouchButton(game, "COMBO", center, press, color)
              : drawQteMouse(center, true, true, press, color);
        break;
    case QtePrompt::Nerve:
        touch ? drawQteTouchButton(game, "NERVE", center, press, color)
              : drawQteKeycap(game, "SPACE", center, 170.0F, press, color);
        break;
    case QtePrompt::Mash:
        touch ? drawQteTouchButton(game, "NERVE", center, press, color)
              : drawQteKeycap(game, "SPACE", center, 170.0F, press, color);
        break;
    case QtePrompt::None:
        break;
    }
}

// Cutscene mode: the regular HUD is gone; only letterbox bars, the button to
// mash, its timer and a marker on the side the threat comes from.
void drawBossQteHud(const Game& game)
{
    const float width = static_cast<float>(game.resources.screenWidth) / hudScale(game);
    const float height = static_cast<float>(game.resources.screenHeight) / hudScale(game);
    const float barHeight = height * 0.09F * easeOutCubic(game.qteAge / 0.25F); // letterbox slides in
    DrawRectangleRec(Rectangle{.x = 0, .y = 0, .width = width, .height = barHeight}, BLACK);
    DrawRectangleRec(
        Rectangle{.x = 0, .y = height - barHeight, .width = width, .height = barHeight}, BLACK);

    const QtePrompt prompt = game.run.qtePrompt;
    if (prompt == QtePrompt::None)
    {
        return;
    }

    const auto time = static_cast<float>(GetTime());
    const float press = 0.5F + 0.5F * std::sin(time * 28.0F);
    const bool isAction = prompt >= QtePrompt::Shield;
    const Color color = isAction ? Palette::Charge : Palette::Crit;
    const Vector2 screenCenter{.x = width / 2, .y = height / 2};

    // Threat marker at the edge the attack comes from.
    if (Vector2LengthSqr(game.run.qteThreatDir) > 0)
    {
        const Vector2 from = Vector2Negate(game.run.qteThreatDir);
        const float edge = std::min(width, height) * 0.4F;
        const float blink = 0.5F + 0.5F * std::sin(time * 20.0F);
        drawQteArrow(Vector2Add(screenCenter, Vector2Scale(from, edge)), game.run.qteThreatDir,
                     20.0F, Fade(Palette::Crit, 0.5F + 0.5F * blink));
    }

    const Vector2 center{.x = screenCenter.x, .y = height * 0.3F};
    // Each new prompt pops in; untimed ones (Mash) pop once with the QTE.
    const float promptAge = game.run.qteTimerMax > 0 ? game.run.qteTimerMax - game.run.qteTimer : game.qteAge;
    beginPop(center, easeOutBack(promptAge / 0.16F));
    drawQteButtons(game, prompt, center, press, color);
    endPop();

    if (game.run.qteTimerMax > 0)
    {
        constexpr float timerWidth = 180.0F;
        const float frac = std::clamp(game.run.qteTimer / game.run.qteTimerMax, 0.0F, 1.0F);
        const float y = center.y + 70.0F;
        DrawRectangleRec(Rectangle{.x = center.x - timerWidth / 2, .y = y, .width = timerWidth,
                                   .height = 6.0F},
                         Fade(Palette::StructMid, 0.5F));
        DrawRectangleRec(Rectangle{.x = center.x - timerWidth / 2 * frac, .y = y,
                                   .width = timerWidth * frac, .height = 6.0F},
                         color);
    }

    if (prompt == QtePrompt::Mash && game.run.qteMashTarget > 0)
    {
        constexpr float mashWidth = 260.0F;
        constexpr float mashHeight = 16.0F;
        const float frac = std::clamp(static_cast<float>(game.run.qteMashCount) /
                                          static_cast<float>(game.run.qteMashTarget),
                                      0.0F, 1.0F);
        const Rectangle bar{.x = center.x - mashWidth / 2, .y = center.y + 86.0F, .width = mashWidth,
                            .height = mashHeight};
        DrawRectangleRec(bar, Fade(Palette::StructMid, 0.5F));
        DrawRectangleRec(Rectangle{.x = bar.x, .y = bar.y, .width = mashWidth * frac, .height = mashHeight},
                         color);
        DrawRectangleLinesEx(bar, 2.0F, color);
        const int32_t textWidth = measureText(game, "MASH!", 20);
        drawText(game, "MASH!", static_cast<int32_t>(center.x) - textWidth / 2,
                 static_cast<int32_t>(bar.y + mashHeight + 8.0F), 20, color);
    }
}

void drawHUD(const Game& game)
{
    if (bossQteActive(game))
    {
        drawBossQteHud(game);
        return;
    }

    drawBossHealthBars(game);
    drawHealthBar(game, 10, 10);
    drawShieldStackPips(game, 10 + healthBarWidthWithLabel() + 20, 10);

    const std::string statsText = std::format("Score: {}   Wave: {}   Lv: {}", game.run.score,
                                              game.run.waveNumber, game.run.level);
    drawText(game, statsText.c_str(), 10, 38, 20, Palette::StructLight);

    const float xpFrac =
        std::min(static_cast<float>(game.run.xp) / static_cast<float>(game.run.xpToNext), 1.0F);
    DrawRectangle(10, 64, 300, 10, Fade(Palette::StructMid, 0.5F));
    DrawRectangle(10, 64, static_cast<int32_t>(300 * xpFrac), 10, Palette::Charge);
    DrawRectangleLines(10, 64, 300, 10, Palette::StructDark);

    constexpr int32_t nerveBarHeight = 18;
    constexpr int32_t damageMeterX = 210;
    drawNerveBar(game, 10, 82);
    const int32_t meterHeight = drawDamageMeter(game, damageMeterX, 82);

    int32_t y = 82 + std::max(nerveBarHeight, meterHeight) + 6;
    drawChargePips(game, 10, y);

    constexpr int32_t abilitySlotCount = ItemConstants::maxAbilitySlots;
    constexpr int32_t abilityBoxSize = 28;
    constexpr int32_t abilityGap = 34;
    const int32_t abilitySlotsWidth = (abilitySlotCount - 1) * abilityGap + abilityBoxSize;
    const auto scaledScreenWidth =
        static_cast<int32_t>(static_cast<float>(game.resources.screenWidth) / hudScale(game));
    const auto scaledScreenHeight =
        static_cast<int32_t>(static_cast<float>(game.resources.screenHeight) / hudScale(game));
    const int32_t abilitySlotsX = scaledScreenWidth - abilitySlotsWidth - 10;
    drawAbilitySlots(game, abilitySlotsX, 10);
    drawStatusRow(game, abilitySlotsX, 10 + abilityBoxSize + 10);

    const char* controlHint =
        touchInputActive(game)
            ? "Move: Left Stick | Aim: Right Stick | Shield/Combo/Dash: Either Panel | "
              "Burst: Nerve | Pause: Top Button"
            : "Move: WASD | Dash: L-Click | Shield: R-Click | Burst: Space | Pause: Esc | F11: "
              "Fullscreen";
    drawText(game, controlHint, 10, scaledScreenHeight - 28, 18, Palette::StructMid);

    if (game.run.achievementToastTimer > 0)
    {
        const auto textWidth = static_cast<int32_t>(
            MeasureTextEx(game.resources.font, game.run.achievementToast.c_str(), 24, 1).x);
        drawText(game, game.run.achievementToast.c_str(), (scaledScreenWidth - textWidth) / 2, 100,
                 24, Palette::Charge);
    }
}

void drawTouchControls(const Game& game)
{
    if (!game.touch.visible || game.autoplay)
    {
        return;
    }

    const auto& touch = game.touch;
    const TouchLayout layout = touchControlLayout(game);
    const auto fontSize = [&](float base) -> int32_t
    { return static_cast<int32_t>(base * guiUiScale(game)); };

    // Very faint - the stick's own track shouldn't compete visually with the
    // gameplay behind it, only the knob (your thumb's actual position) needs
    // to read clearly.
    const auto drawStick = [&](const TouchStick& stick, Vector2 anchor)
    {
        const Vector2 knob = Vector2Add(anchor, stick.offset);
        const bool active = stick.touchId != -1;

        DrawCircleV(anchor, layout.stickRadius, Fade(Palette::StructMid, 0.10F));
        DrawCircleLinesV(anchor, layout.stickRadius, Fade(Palette::StructLight, 0.10F));
        DrawCircleV(knob, layout.stickRadius * 0.45F, Fade(Palette::Accent, active ? 0.55F : 0.3F));
    };

    drawStick(touch.moveStick, layout.moveStickAnchor);
    drawStick(touch.aimStick, layout.aimStickAnchor);

    const auto drawCircleButton = [&](const TouchButton& button, Vector2 center, float radius,
                                      const char* label)
    {
        DrawCircleV(
            center, radius,
            Fade(button.down ? Palette::Accent : Palette::StructMid, button.down ? 0.55F : 0.3F));
        DrawCircleLinesV(center, radius, Fade(Palette::StructLight, 0.6F));
        const int32_t size = fontSize(18.0F);
        const int32_t textWidth = measureText(game, label, size);
        drawText(game, label, static_cast<int32_t>(center.x) - textWidth / 2,
                 static_cast<int32_t>(center.y) - size / 2, size,
                 button.down ? Palette::Accent : Palette::StructLight);
    };

    drawCircleButton(touch.nerve, layout.nerveCenter, layout.nerveRadius, "NERVE");

    // Dash/shield/combo: three separate chamfered-corner button panels per
    // side (shield/dash stacked, combo as its own full-height bar next to
    // them) - real distinct filled buttons, not one connected outline shape.
    // Filled solid with the current ship's own color (brightened when held);
    // outlined by drawing the same chamfered shape slightly larger in a dark
    // color first, then the true-size fill on top.
    const Color shipColor = currentShip(game).color;
    const float chamfer = layout.buttonChamfer;
    const float outlineExtra = 4.0F * guiUiScale(game);

    const auto chamferPoints = [](Rectangle r, float c) -> std::array<Vector2, 8>
    {
        const float cc = std::min(c, std::min(r.width, r.height) / 2.0F);
        const float x0 = r.x;
        const float y0 = r.y;
        const float x1 = r.x + r.width;
        const float y1 = r.y + r.height;
        return {
            Vector2{.x = x0 + cc, .y = y0}, Vector2{.x = x1 - cc, .y = y0},
            Vector2{.x = x1, .y = y0 + cc}, Vector2{.x = x1, .y = y1 - cc},
            Vector2{.x = x1 - cc, .y = y1}, Vector2{.x = x0 + cc, .y = y1},
            Vector2{.x = x0, .y = y1 - cc}, Vector2{.x = x0, .y = y0 + cc},
        };
    };
    const auto drawButtonPanel = [&](Rectangle rect, bool down)
    {
        const Color fillColor = down ? ColorBrightness(shipColor, 0.35F) : shipColor;
        const Rectangle outlineRect{.x = rect.x - outlineExtra,
                                    .y = rect.y - outlineExtra,
                                    .width = rect.width + outlineExtra * 2,
                                    .height = rect.height + outlineExtra * 2};
        auto outlinePts = chamferPoints(outlineRect, chamfer + outlineExtra);
        DrawTriangleFan(outlinePts.data(), static_cast<int32_t>(outlinePts.size()),
                        Palette::StructDark);
        auto fillPts = chamferPoints(rect, chamfer);
        DrawTriangleFan(fillPts.data(), static_cast<int32_t>(fillPts.size()), fillColor);
    };
    drawButtonPanel(layout.leftShieldRect, touch.leftShield.down);
    drawButtonPanel(layout.leftComboRect, touch.leftCombo.down);
    drawButtonPanel(layout.leftDashRect, touch.leftDash.down);
    drawButtonPanel(layout.rightShieldRect, touch.rightShield.down);
    drawButtonPanel(layout.rightComboRect, touch.rightCombo.down);
    drawButtonPanel(layout.rightDashRect, touch.rightDash.down);

    DrawCircleV(layout.pauseCenter, layout.pauseRadius, Fade(Palette::StructMid, 0.3F));
    DrawCircleLinesV(layout.pauseCenter, layout.pauseRadius, Fade(Palette::StructLight, 0.6F));
    const float pauseBarWidth = layout.pauseRadius * 0.22F;
    const float pauseBarHeight = layout.pauseRadius * 0.9F;
    const float pauseBarGap = layout.pauseRadius * 0.18F;
    DrawRectangle(static_cast<int32_t>(layout.pauseCenter.x - pauseBarGap - pauseBarWidth),
                  static_cast<int32_t>(layout.pauseCenter.y - pauseBarHeight / 2),
                  static_cast<int32_t>(pauseBarWidth), static_cast<int32_t>(pauseBarHeight),
                  Palette::StructLight);
    DrawRectangle(static_cast<int32_t>(layout.pauseCenter.x + pauseBarGap),
                  static_cast<int32_t>(layout.pauseCenter.y - pauseBarHeight / 2),
                  static_cast<int32_t>(pauseBarWidth), static_cast<int32_t>(pauseBarHeight),
                  Palette::StructLight);
}

void drawDownwardTriangleIcon(float cx, int32_t y, float size, Color fillColor, bool filled)
{
    const Vector2 bottom{.x = cx, .y = static_cast<float>(y) + size * 2};
    const Vector2 topLeft{.x = cx - size * 0.8F, .y = static_cast<float>(y)};
    const Vector2 topRight{.x = cx + size * 0.8F, .y = static_cast<float>(y)};

    if (filled)
    {
        DrawTriangle(bottom, topRight, topLeft, fillColor);
    }
    else
    {
        DrawTriangleLines(bottom, topRight, topLeft, Fade(Palette::StructMid, 0.6F));
    }
}

constexpr int32_t healthBarWidth = 200;
constexpr int32_t healthBarHeight = 14;

void drawHealthBar(const Game& game, int32_t x, int32_t y)
{
    const float frac = std::clamp(game.run.player.health / game.run.player.maxHealth, 0.0F, 1.0F);

    DrawRectangle(x, y, healthBarWidth, healthBarHeight, Fade(Palette::StructMid, 0.5F));
    DrawRectangle(x, y, static_cast<int32_t>(static_cast<float>(healthBarWidth) * frac),
                  healthBarHeight, Palette::Accent);
    DrawRectangleLines(x, y, healthBarWidth, healthBarHeight, Palette::StructDark);

    const std::string label =
        std::format("{:.0f}/{:.0f}", game.run.player.health, game.run.player.maxHealth);
    drawText(game, label.c_str(), x + healthBarWidth + 8, y - 3, 16, Palette::StructLight);
}

auto healthBarWidthWithLabel() -> int32_t { return healthBarWidth + 60; }

void drawShieldStackPips(const Game& game, int32_t x, int32_t y)
{
    constexpr float size = 11;
    constexpr float gap = 24;

    for (int32_t i = 0; i < currentShip(game).maxShieldStacks; i++)
    {
        const float cx = static_cast<float>(x) + size + static_cast<float>(i) * gap;
        const Color fillColor = Fade(Palette::Shield, 0.85F);

        drawDownwardTriangleIcon(cx, y, size, fillColor, i < game.run.player.shieldStacks);
    }
}

void drawNerveBar(const Game& game, int32_t x, int32_t y)
{
    constexpr int32_t width = 120;
    constexpr int32_t height = 8;
    const float frac = nerveFrac(game);
    const bool ready = frac >= 1.0F;
    const float pulse = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 8.0F);

    DrawRectangle(x, y, width, height, Fade(Palette::StructMid, 0.5F));
    DrawRectangle(x, y, static_cast<int32_t>(static_cast<float>(width) * frac), height,
                  ready ? ColorLerp(Palette::Charge, WHITE, pulse * 0.5F)
                        : ColorLerp(Palette::Charge, Palette::Crit, frac));
    DrawRectangleLines(x, y, width, height, Palette::StructDark);

    if (ready)
    {
        DrawRectangleLines(x - 2, y - 2, width + 4, height + 4, Fade(Palette::Charge, pulse));
    }

    drawText(game, "Nerve", x + width + 8, y - 6, 16, Palette::StructMid);

    if (game.run.player.nerveCharging)
    {
        const float chargeProgress =
            1.0F - game.run.player.nerveChargeTimer / UpdateConstants::nerveBurstWindup;
        DrawRectangle(x, y + height + 3,
                      static_cast<int32_t>(static_cast<float>(width) * chargeProgress), 3,
                      Palette::Crit);
        drawText(game, "CHARGING...", x, y + height + 8, 14, Palette::Crit);
    }
    else if (ready)
    {
        drawText(game, "SPACE", x, y + height + 8, 14, Fade(Palette::Charge, 0.6F + 0.4F * pulse));
    }
}

auto drawDamageMeter(const Game& game, int32_t x, int32_t y) -> int32_t
{
    const auto& meter = game.run.damageMeterDisplay;
    const std::string totalText = std::format("DMG: {}", meter.total);
    drawText(game, totalText.c_str(), x, y, 18,
             meter.total > 0 ? Palette::Crit : Palette::StructMid);

    return 22;
}

void drawChargePips(const Game& game, int32_t x, int32_t y)
{
    constexpr float pipRadius = 9;
    constexpr float gap = 24;

    for (int32_t i = 0; i < UpdateConstants::maxCharges; i++)
    {
        const Vector2 center{.x = static_cast<float>(x) + pipRadius + static_cast<float>(i) * gap,
                             .y = static_cast<float>(y) + pipRadius};

        if (i < game.run.player.charges)
        {
            DrawCircleV(center, pipRadius, Palette::Shield);
            DrawCircleLines(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y),
                            pipRadius, Palette::StructDark);
            continue;
        }

        DrawCircleLines(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y), pipRadius,
                        Fade(Palette::StructMid, 0.6F));
        if (i == game.run.player.charges)
        {
            float progress = 1;
            if (const float d = chargeRegenDuration(game); d > 0)
            {
                progress = 1 - game.run.player.chargeRegenTimer / d;
            }
            const Color ringColor = isNerveChargeFeeding(game) ? Palette::Charge : Palette::Shield;
            DrawRing(center, pipRadius - 3, pipRadius, -90, -90 + 360 * progress, 16, ringColor);
            if (isNerveChargeFeeding(game))
            {
                const float pulse = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 10.0F);
                DrawCircleLines(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y),
                                pipRadius + 3, Fade(Palette::Charge, 0.5F * pulse));
            }
        }
    }
}

enum class StatusIconKind : std::uint8_t
{
    Dot,
    Weapon,
    Exclaim
};

struct StatusEntry
{
    StatusIconKind iconKind;
    Color color;
    WeaponType weaponType = WeaponType::Forward;
    float fracRemaining = -1;
    std::string label;
};

void drawStatusRing(Vector2 center, float radius, float frac, Color color)
{

    constexpr float ringThickness = 2.0F;
    DrawRing(center, radius, radius + ringThickness, 0, 360, 24, Fade(Palette::StructDark, 0.7F));
    if (frac > 0)
    {
        const float endAngle = -90.0F + 360.0F * std::clamp(frac, 0.0F, 1.0F);
        DrawRing(center, radius, radius + ringThickness, -90.0F, endAngle, 24, color);
    }
}

void drawStatusIcon(const Game& game, const StatusEntry& entry, Vector2 center, float radius)
{
    switch (entry.iconKind)
    {
    case StatusIconKind::Dot:
        DrawCircleV(center, radius, entry.color);
        break;
    case StatusIconKind::Weapon:
        drawWeaponIcon(entry.weaponType, center, radius, entry.color);
        break;
    case StatusIconKind::Exclaim:
        DrawCircleV(center, radius, entry.color);
        drawText(game, "!", static_cast<int32_t>(center.x) - 3, static_cast<int32_t>(center.y) - 7,
                 14, Palette::Void);
        break;
    }
}

void drawStatusRow(const Game& game, int32_t x, int32_t y)
{
    const auto& player = game.run.player;
    std::vector<StatusEntry> entries;

    for (size_t e = 0; e < static_cast<size_t>(ElementType::Count); e++)
    {
        const float timer = player.elementalBuffTimer.at(e);
        if (timer <= 0)
        {
            continue;
        }
        entries.push_back(
            StatusEntry{.iconKind = StatusIconKind::Dot,
                        .color = elementColors.at(e),
                        .fracRemaining = timer / pickupEffectDuration,
                        .label = std::format("{} {:.0f}s", elementNames.at(e), std::ceil(timer))});
    }

    if (player.regenTimer > 0)
    {
        entries.push_back(
            StatusEntry{.iconKind = StatusIconKind::Dot,
                        .color = Palette::Accent,
                        .fracRemaining = player.regenTimer / pickupEffectDuration,
                        .label = std::format("Regen {:.0f}s", std::ceil(player.regenTimer))});
    }

    if (player.overchargeTimer > 0)
    {
        entries.push_back(StatusEntry{
            .iconKind = StatusIconKind::Dot,
            .color = Palette::Charge,
            .fracRemaining = player.overchargeTimer / overchargeDuration,
            .label = std::format("Overcharge {:.0f}s", std::ceil(player.overchargeTimer))});
    }

    if (player.secondWindReady)
    {
        entries.push_back(StatusEntry{.iconKind = StatusIconKind::Dot,
                                      .color = Palette::Crit,
                                      .fracRemaining = -1,
                                      .label = "Second Wind ready"});
    }

    if (player.overdriveTimer > 0)
    {
        entries.push_back(StatusEntry{
            .iconKind = StatusIconKind::Dot,
            .color = Palette::Crit,
            .fracRemaining = player.overdriveTimer / overdriveDuration,
            .label = std::format("Overdrive {:.0f}s", std::ceil(player.overdriveTimer))});
    }

    if (game.run.weaponDowngrade.has_value())
    {
        const auto& downgrade = *game.run.weaponDowngrade;
        entries.push_back(
            StatusEntry{.iconKind = StatusIconKind::Weapon,
                        .color = Palette::Accent,
                        .weaponType = downgrade.type,
                        .fracRemaining = downgrade.timer / dangerDowngradeDuration,
                        .label = std::format("{} stolen {:.0f}s", weaponDisplayName(downgrade.type),
                                             std::ceil(downgrade.timer))});
    }

    const bool suppressed = std::any_of(
        game.run.eliteHazards.begin(), game.run.eliteHazards.end(), [](const EliteHazard& hazard)
        { return hazard.active && hazard.role == EliteHazardRole::Suppressor; });
    if (suppressed)
    {
        entries.push_back(StatusEntry{.iconKind = StatusIconKind::Exclaim,
                                      .color = Palette::Shield,
                                      .fracRemaining = -1,
                                      .label = "Suppressed: weapon cooldowns +40%"});
    }

    if (entries.empty())
    {
        return;
    }

    constexpr float iconRadius = 8.0F;
    constexpr float iconGap = 24.0F;
    const Vector2 mouse = mouseUIPos(game);
    std::optional<size_t> hoveredIdx;

    for (size_t i = 0; i < entries.size(); i++)
    {
        const Vector2 center{.x = static_cast<float>(x) + iconRadius +
                                  static_cast<float>(i) * iconGap,
                             .y = static_cast<float>(y) + iconRadius};
        const auto& entry = entries.at(i);
        if (entry.fracRemaining >= 0)
        {
            drawStatusRing(center, iconRadius, entry.fracRemaining, entry.color);
        }
        drawStatusIcon(game, entry, center, iconRadius * 0.72F);

        if (hoverActive(game) && CheckCollisionPointCircle(mouse, center, iconRadius + 4))
        {
            hoveredIdx = i;
        }
    }

    if (hoveredIdx.has_value())
    {
        const auto& entry = entries.at(*hoveredIdx);
        const Vector2 center{.x = static_cast<float>(x) + iconRadius +
                                  static_cast<float>(*hoveredIdx) * iconGap,
                             .y = static_cast<float>(y) + iconRadius};
        constexpr int32_t tipFontSize = 14;
        const int32_t boxWidth = measureText(game, entry.label.c_str(), tipFontSize) + 14;
        const auto tipX = static_cast<int32_t>(center.x) - boxWidth / 2;
        const auto tipY = static_cast<int32_t>(center.y) + static_cast<int32_t>(iconRadius) + 6;

        DrawRectangle(tipX, tipY, boxWidth, 22, Fade(Palette::Void, 0.9F));
        DrawRectangleLines(tipX, tipY, boxWidth, 22, entry.color);
        drawText(game, entry.label.c_str(), tipX + 7, tipY + 4, tipFontSize, Palette::StructLight);
    }
}

void drawBrowserParticleDemo(int32_t style, Vector2 center)
{
    const auto t = static_cast<float>(GetTime());
    switch (style)
    {
    case 0:
        for (int32_t i = 0; i < 10; i++)
        {
            const float seed = static_cast<float>(i) * 17.0F;
            const float angle = std::fmod(seed + t * 1.5F, 2.0F * std::numbers::pi_v<float>);
            const float bob = 0.5F + 0.5F * std::sin(t * 9.0F + seed);
            const Vector2 base{.x = center.x + std::cos(angle) * 40,
                               .y = center.y + std::sin(angle) * 40 * 0.5F + 40};
            const Vector2 tip{.x = base.x + std::sin(t * 14.0F + seed) * 4,
                              .y = base.y - (12.0F + bob * 20.0F)};
            const Color flameColor =
                bob > 0.6F ? Palette::ElementBurn : ColorLerp(Palette::ElementBurn, YELLOW, 0.5F);
            DrawTriangle(Vector2{.x = base.x - 4, .y = base.y}, tip,
                         Vector2{.x = base.x + 4, .y = base.y}, Fade(flameColor, 0.75F));
        }
        break;
    case 1:
    default:
        for (int32_t i = 0; i < 16; i++)
        {
            const float angle = static_cast<float>(i) * (360.0F / 16.0F) * DEG2RAD;
            const float life = std::fmod(t * 1.2F + static_cast<float>(i) * 0.05F, 1.0F);
            const Vector2 pos{.x = center.x + std::cos(angle) * life * 70,
                              .y = center.y + std::sin(angle) * life * 70};
            const Color debrisColor = i % 2 == 0 ? Palette::Accent : Palette::Crit;
            DrawCircleV(pos, 4 * (1 - life), Fade(debrisColor, 1 - life));
        }
        break;
    }
}

void drawSandboxBrowser(const Game& game)
{
    for (int32_t i = 0; i < browserCategoryCount; i++)
    {
        const Rectangle rect = sandboxBrowserCategoryButtonRect(game, i);
        if (i == game.browserCategoryIndex)
        {
            GuiSetState(STATE_FOCUSED);
        }
        GuiButton(rect, std::string(browserCategoryLabel(i)).c_str());
        GuiSetState(STATE_NORMAL);
    }

    const Rectangle preview = sandboxBrowserPreviewRect(game);
    DrawRectangleRec(preview, Fade(Palette::StructDark, 0.6F));
    DrawRectangleLinesEx(preview, 2, Palette::StructLight);
    const Vector2 center{.x = preview.x + preview.width / 2, .y = preview.y + preview.height / 2};

    switch (game.browserCategoryIndex)
    {
    case 0:
    {
        const auto& kind = enemyKinds.at(static_cast<size_t>(game.sandboxKindIndex));
        Enemy previewEnemy{.kind = game.sandboxKindIndex,
                           .position = center,
                           .velocity = Vector2{},
                           .health = kind.health,
                           .active = true,
                           .stateTimer = 1,
                           .charging = false,
                           .telegraphing = false,
                           .phased = false,
                           .orbitAngle = 0,
                           .orbitDist = 0,
                           .orbitCenterCurrent = center,
                           .isElite = false,
                           .hitByDash = false,
                           .hitFlashTimer = 0};
        drawEnemy(game, previewEnemy, false);
        break;
    }
    case 1:
    {
        const auto& type = bossTypes.at(static_cast<size_t>(game.browserBossTypeIndex));
        constexpr float previewBossSize = 110.0F;
        Boss previewBoss{.position = Vector2{.x = center.x - previewBossSize / 2,
                                             .y = center.y - previewBossSize / 2},
                         .size = Vector2{.x = previewBossSize, .y = previewBossSize},
                         .color = type.color,
                         .baseColor = type.color,
                         .health = 100,
                         .maxHealth = 100,
                         .state = BossState::IDLE,
                         .shape = type.shape};
        drawBoss(game, previewBoss);
        break;
    }
    case 2:
    {
        const auto shipClass = static_cast<ShipClass>(game.browserShipIndex);
        drawShipHull(shipClass, center, 20, 0,
                     ships.at(static_cast<size_t>(game.browserShipIndex)).color);
        break;
    }
    case 3:
    {
        const auto option = sandboxPickupPreview(game.sandboxPickupIndex);
        drawPickupIcon(option.type, option.element, center, 1.0F);
        break;
    }
    case 4:
        drawWeaponIcon(static_cast<WeaponType>(game.browserWeaponIndex), center, 26,
                       Palette::Accent);
        break;
    case 5:
        drawBrowserParticleDemo(game.browserParticleIndex, center);
        break;
    default:
        break;
    }

    windowText(game, browserCurrentName(game).c_str(), static_cast<int32_t>(center.x),
               static_cast<int32_t>(preview.y + preview.height + 30), 22, Palette::Crit);

    if (const std::string details = browserCurrentDetails(game); !details.empty())
    {
        windowText(game, details.c_str(), static_cast<int32_t>(center.x),
                   static_cast<int32_t>(preview.y + preview.height + 62), 16, Palette::StructLight);
    }

    GuiButton(sandboxBrowserPrevButtonRect(game), "<");
    GuiButton(sandboxBrowserNextButtonRect(game), ">");
}

void drawPickupIcon(PickupType type, ElementType element, Vector2 position, float alpha)
{
    switch (type)
    {
    case PickupType::LifeOrb:
        DrawCircleV(position, 5, Fade(Palette::Accent, alpha));
        DrawCircleLinesV(position, 6, Fade(Palette::Crit, 0.8F * alpha));
        break;
    case PickupType::Shield:
        DrawCircleV(position, 6, Fade(Palette::Shield, alpha));
        DrawCircleLinesV(position, 7, Fade(Palette::Haze, 0.8F * alpha));
        break;
    case PickupType::Elemental:
    {
        const Color color = elementColors.at(static_cast<size_t>(element));
        DrawPoly(position, 4, 7, static_cast<float>(GetTime()) * 90.0F, Fade(color, alpha));
        DrawCircleLinesV(position, 9, Fade(color, 0.6F * alpha));
        break;
    }
    case PickupType::Regen:
        DrawCircleV(position, 6, Fade(Palette::Heal, alpha));
        DrawCircleLinesV(position, 7, Fade(WHITE, 0.6F * alpha));
        break;
    case PickupType::MagnetPulse:
        DrawCircleLinesV(position, 8, Fade(Palette::Shield, alpha));
        DrawCircleLinesV(position, 5, Fade(Palette::Shield, 0.6F * alpha));
        break;
    case PickupType::Overcharge:
        DrawPoly(position, 6, 7, static_cast<float>(GetTime()) * 120.0F,
                 Fade(Palette::Charge, alpha));
        break;
    case PickupType::SecondWind:
        DrawCircleV(position, 6, Fade(Palette::Crit, alpha));
        DrawCircleLinesV(position, 8, Fade(Palette::Crit, 0.5F * alpha));
        break;
    case PickupType::Danger:
    {
        const float pulse = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 10.0F);
        DrawPoly(position, 5, 9, static_cast<float>(GetTime()) * -140.0F,
                 Fade(Palette::Void, alpha));
        DrawPolyLines(position, 5, 10, static_cast<float>(GetTime()) * -140.0F,
                      Fade(Palette::Crit, alpha * pulse));
        break;
    }
    default:
        DrawCircleV(position, 2.5F, Fade(Palette::Charge, alpha));
        DrawCircleLinesV(position, 3.5F, Fade(Palette::Crit, 0.6F * alpha));
        break;
    }
}

void drawSandboxToggleIndicator(const Game& game)
{
    const Rectangle rect = sandboxToggleIndicatorRect(game);
    GuiButton(rect, "Instructions [TAB]");

    // M12 director debug panel: what the bot/player is doing and how each boss is responding.
    const float scale = guiUiScale(game);
    const auto px = [scale](float v) { return static_cast<int32_t>(v * scale); };
    const int32_t font = px(20);
    const int32_t line = px(24);
    const int32_t barW = px(140);
    const int32_t labelW = px(125);
    const int32_t panelW = labelW + barW + px(170);
    // Right side, under the ability icons: the left side holds the HP/score HUD.
    const int32_t x = game.resources.windowWidth - panelW - px(6);
    int32_t y = px(64);

    int32_t directorBosses = 0;
    for (const auto& boss : game.run.bosses)
    {
        directorBosses += static_cast<int32_t>(boss.usesDirector && boss.health > 0);
    }
    const int32_t rows = tacticCount + 1 + (game.autoplay ? 1 : 0) + std::max(1, directorBosses * 2);
    DrawRectangle(x - px(6), y - px(4), panelW, rows * line + px(8),
                  Fade(BLACK, 0.6F));

    if (game.autoplay)
    {
        const std::string botLine = std::string("BOT: ") + autoplayHabitName(game);
        drawText(game, botLine.c_str(), x, y, font, WHITE);
        y += line;
    }
    drawText(game, "Player habits (8s)", x, y, font, Palette::Shield);
    y += line;
    for (int i = 0; i < tacticCount; i++)
    {
        const float s = game.run.tactics.score.at(static_cast<size_t>(i));
        const Color c = s > 0.45F ? Palette::Crit : Palette::StructMid;
        drawText(game, std::string(tacticNames.at(static_cast<size_t>(i))).c_str(), x, y, font, c);
        DrawRectangle(x + labelW, y + px(4), barW, font - px(6), Fade(WHITE, 0.15F));
        DrawRectangle(x + labelW, y + px(4), static_cast<int32_t>(static_cast<float>(barW) * s),
                      font - px(6), c);
        drawText(game, (std::to_string(static_cast<int>(s * 100)) + "%").c_str(),
                 x + labelW + barW + px(8), y, font, c);
        y += line;
    }

    if (directorBosses == 0)
    {
        drawText(game, "No director boss (Spawn Boss)", x, y, font, Palette::StructMid);
        return;
    }
    for (const auto& boss : game.run.bosses)
    {
        if (!boss.usesDirector || boss.health <= 0)
        {
            continue;
        }
        const int hpPct = boss.health * 100 / std::max(1, boss.maxHealth);
        const std::string head = std::string(bossPatternNames.at(static_cast<size_t>(boss.pattern))) +
                                 "  P" + std::to_string(boss.phase + 1) + "  HP " +
                                 std::to_string(hpPct) + "%";
        drawText(game, head.c_str(), x, y, font, Palette::Shield);
        y += line;
        std::string status;
        if (boss.phaseInvulnTimer > 0)
        {
            status = "PHASE SHIFT (invulnerable)";
        }
        else if (boss.dazedTimer > 0)
        {
            status = std::format("DAZED {:.1f}s (x1.5 dmg)", boss.dazedTimer);
        }
        else
        {
            status = std::format("{}  step {}  t {:.1f}s{}", bossAttackNames.at(static_cast<size_t>(boss.attack)),
                                 boss.patternStep, std::max(0.0F, boss.patternTimer),
                                 boss.readTellTimer > 0 ? "  READ YOU" : "");
        }
        drawText(game, status.c_str(), x + px(12), y, font,
                 boss.readTellTimer > 0 ? Palette::Crit : Palette::Accent);
        y += line;
    }
}

void drawSandboxMenu(const Game& game)
{
    applyGuiScale(game);
    windowText(game, "SANDBOX", game.resources.windowWidth / 2, 80, 34, Palette::Accent);

    GuiButton(sandboxBrowserModeToggleRect(game), game.sandboxBrowserMode ? "Actions" : "Browser");
    GuiButton(sandboxMenuCloseButtonRect(game), "Close [Esc]");

    if (game.sandboxBrowserMode)
    {
        drawSandboxBrowser(game);
        return;
    }

    const auto& steppers = sandboxMenuSteppers();
    for (int32_t i = 0; i < static_cast<int32_t>(steppers.size()); i++)
    {
        const auto& stepper = steppers.at(static_cast<size_t>(i));
        const Rectangle row = sandboxStepperRowRect(game, i);
        // Label sits left of the row, right-aligned and vertically centered on it.
        const float scale = guiUiScale(game);
        const auto labelSize = static_cast<int32_t>(16 * scale);
        const std::string label(stepper.label);
        drawText(game, label.c_str(),
                 static_cast<int32_t>(row.x - 12 * scale) - measureText(game, label.c_str(), labelSize),
                 static_cast<int32_t>(row.y + (row.height - static_cast<float>(labelSize)) / 2),
                 labelSize, Palette::StructLight);

        GuiButton(sandboxStepperMinusRect(game, i), "-");
        std::string value = stepper.valueText(game);
        GuiTextBox(sandboxStepperValueRect(game, i), value.data(),
                   static_cast<int32_t>(value.size()) + 1, false);
        GuiButton(sandboxStepperPlusRect(game, i), "+");
    }

    const auto& buttons = sandboxMenuButtons();
    for (int32_t i = 0; i < static_cast<int32_t>(buttons.size()); i++)
    {
        GuiButton(sandboxMenuButtonRect(game, i),
                  buttons.at(static_cast<size_t>(i)).label(game).c_str());
    }
}

}

auto drawGame(Game& game) -> void
{
    updateTransitions(game);
    BeginTextureMode(game.resources.worldTarget);
    ClearBackground(biomeVoidColor(game));

    switch (game.state)
    {
    case GameState::TITLE:
    case GameState::ENDING:
        beginPixelZoom(game);
        drawBackgroundStars(game, Vector2{});
        EndMode2D();
        break;
    case GameState::GAMEPLAY:
    case GameState::PAUSED:
    case GameState::LEVEL_UP:
    case GameState::GAME_OVER:
    case GameState::SANDBOX_MENU:
        beginWorldCamera(game);
        drawGameplayWorld(game);
        EndMode2D();
        break;
    case GameState::SHIP_SELECT:
    case GameState::SETTINGS:
    case GameState::ACHIEVEMENTS:
    case GameState::BESTIARY:
    case GameState::SKILL_TREE:
        if (overGameplay(game, game.state))
        {
            beginWorldCamera(game);
            drawGameplayWorld(game);
        }
        else
        {
            beginPixelZoom(game);
            drawBackgroundStars(game, Vector2{});
        }
        EndMode2D();
        break;
    }

    EndTextureMode();

    BeginTextureMode(game.resources.pixelTarget);
    ClearBackground(Palette::Void);

    const auto worldSrc =
        Rectangle{.x = 0,
                  .y = 0,
                  .width = static_cast<float>(game.resources.worldTarget.get().texture.width),
                  .height = -static_cast<float>(game.resources.worldTarget.get().texture.height)};
    const auto worldDst =
        Rectangle{.x = 0,
                  .y = 0,
                  .width = static_cast<float>(game.resources.worldTarget.get().texture.width),
                  .height = static_cast<float>(game.resources.worldTarget.get().texture.height)};
    DrawTexturePro(game.resources.worldTarget.get().texture, worldSrc, worldDst, Vector2{}, 0,
                   WHITE);

    EndTextureMode();

    BeginDrawing();
    ClearBackground(Palette::Void);

    for (size_t i = 0; i < game.run.borderStars.size(); ++i)
    {
        auto gameBorderStar = game.run.borderStars[i];
        if (gameBorderStar.position.x <= static_cast<float>(game.resources.windowWidth) &&
            gameBorderStar.position.y <= static_cast<float>(game.resources.windowHeight))
        {
            DrawCircleV(gameBorderStar.position, gameBorderStar.radius, Palette::Haze);
        }
    }

    const auto srcRec =
        Rectangle{.x = 0,
                  .y = 0,
                  .width = static_cast<float>(game.resources.pixelTarget.get().texture.width),
                  .height = -static_cast<float>(game.resources.pixelTarget.get().texture.height)};
    const Rectangle letterbox = letterBoxRect(game);
    DrawTexturePro(game.resources.pixelTarget.get().texture, srcRec, letterbox, Vector2{}, 0,
                   WHITE);

    const float uiScale = letterbox.width / static_cast<float>(game.resources.screenWidth);
    BeginMode2D(Camera2D{.offset = Vector2{.x = letterbox.x, .y = letterbox.y},
                         .target = Vector2{},
                         .rotation = 0,
                         .zoom = uiScale * hudScale(game)});

    switch (game.state)
    {
    case GameState::GAMEPLAY:
    case GameState::PAUSED:
    case GameState::LEVEL_UP:
    case GameState::SANDBOX_MENU:
        drawHUD(game);
        break;
    case GameState::SHIP_SELECT:
    case GameState::SETTINGS:
    case GameState::ACHIEVEMENTS:
    case GameState::BESTIARY:
    case GameState::SKILL_TREE:
        if (game.settingsReturnState == GameState::PAUSED)
        {
            drawHUD(game);
        }
        break;
    case GameState::TITLE:
    case GameState::GAME_OVER:
    case GameState::ENDING:
        break;
    }

    EndMode2D();

    switch (game.state)
    {
    case GameState::TITLE:
        drawTitle(game);
        break;
    case GameState::SHIP_SELECT:
        if (game.settingsReturnState != GameState::TITLE)
        {
            drawOverlay(game);
        }
        drawShipSelect(game);
        break;
    case GameState::GAMEPLAY:
        if (game.sandbox && !bossQteActive(game))
        {
            drawSandboxToggleIndicator(game);
        }
        drawTouchControls(game);
        break;
    case GameState::SANDBOX_MENU:
        drawOverlay(game);
        drawSandboxMenu(game);
        break;
    case GameState::PAUSED:
        drawOverlay(game);
        drawMenu(game, "PAUSED",
                 {"Resume", "Achievements", "Bestiary", "Settings", "New Game", "Exit"},
                 MenuLayout::pausedMenuY);
        break;
    case GameState::LEVEL_UP:
        drawOverlay(game);
        drawLevelUp(game);
        break;
    case GameState::GAME_OVER:
        drawOverlay(game);
        drawGameOver(game);
        break;
    case GameState::SETTINGS:
        drawOverlay(game);
        drawSettings(game);
        break;
    case GameState::ACHIEVEMENTS:
        drawOverlay(game);
        drawAchievements(game);
        break;
    case GameState::BESTIARY:
        drawOverlay(game);
        drawBestiary(game);
        break;
    case GameState::SKILL_TREE:
        drawOverlay(game);
        drawSkillTree(game);
        break;
    case GameState::ENDING:
        drawEnding(game);
        break;
    }
    drawIrisReveal(game);
    drawIrisClose(game);

    EndDrawing();
}
