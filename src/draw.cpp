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
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include "sandbox.hpp"
#include "settings.hpp"
#include "update.hpp"
#include "update_constants.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
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
void drawEnemyShape(EnemyShape shape, Vector2 center, float radius, float rotationDeg,
                    Color color);
void drawBossHull(BossShape shape, Vector2 center, Vector2 size, Color color);
void drawShipSelect(const Game& game);
void drawGameOver(const Game& game);
void drawEnding(const Game& game);
void drawOverlay(const Game& game);
void drawSettings(const Game& game);
void drawSkillIcon(SkillType id, Vector2 c, float s, Color color);
void drawWeaponIcon(WeaponType kind, Vector2 c, float s, Color color);
void drawPassiveIcon(SkillType id, Vector2 c, float s, Color color);
auto evolutionHint(const Game& game, SkillType id) -> std::string;
void drawAbilitySlots(const Game& game, int32_t x, int32_t y);
void drawLevelUp(const Game& game);
void drawMenu(const Game& game, const std::string& heading, const std::vector<std::string>& options,
              int32_t y);
void drawHighScores(const Game& game, int32_t x, int32_t y, const std::vector<int32_t>& scores);
void drawGameplayWorld(const Game& game);
void drawPunctumLightning(const Game& game);
void drawShipHull(ShipClass shipClass, Vector2 p, float r, float angle, Color shipColor);
void drawShip(const Game& game);
void drawNerveBurstFlash(const Game& game);
void drawEnemy(const Game& game, const Enemy& enemy, bool buffed);
void drawEliteHazard(const EliteHazard& hazard);
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
void drawKrakenHurledEnemy(const BossProjectile& projectile);
void drawFluidBlob(Vector2 center, float radius, Color color, float seed, Vector2 trailDir);
void drawWreckwormHeadShape(Vector2 center, float radius, float seed, Color color, Vector2 velocity);
void drawKrakenShape(Vector2 center, float radius, float seed, Color color);
void drawBendyTentacle(Vector2 anchor, Vector2 tip, Vector2 bodyCenter, float bodyRadius, float seed,
                       float thicknessScale, float waveScale, bool calm, Color color, Color tipColor);
void drawKrakenTentacle(const Boss& boss);
void drawKrakenLimb(const Boss& boss, int32_t slot);
void drawKrakenGrabbedEnemy(Vector2 pos, int32_t kindIndex, float scale, float facingAngle);
void drawBanishedShape(Vector2 center, const Boss& boss);
void drawBanishedTentacle(Vector2 center, const Boss& boss, int32_t slot);
void drawBanishedEye(const Boss& boss);
void drawPanelShape(Vector2 center, float radius, int32_t sides, float rotationDeg, Color baseColor,
                    float seed);
void drawPanelHex(Vector2 center, float radius, float rotationDeg, Color baseColor, float seed);
void drawHUD(const Game& game);
void drawStatusRow(const Game& game, int32_t x, int32_t y);
void drawDownwardTriangleIcon(float cx, int32_t y, float size, Color fillColor, bool filled);
void drawHealthBar(const Game& game, int32_t x, int32_t y);
auto healthBarWidthWithLabel() -> int32_t;
void drawShieldStackPips(const Game& game, int32_t x, int32_t y);
void drawNerveBar(const Game& game, int32_t x, int32_t y);
auto drawDamageMeter(const Game& game, int32_t x, int32_t y) -> int32_t;
void drawChargePips(const Game& game, int32_t x, int32_t y);
void drawOrganicEnemyShape(Vector2 center, float baseRadius, float seed, Color color);
void drawClusterEnemyShape(Vector2 center, float baseRadius, float seed, Color color);
void drawMeteorBossShape(Vector2 center, float radius, int32_t seed, Color baseColor);
auto hashNoise(float seed) -> float;
void drawSandboxToggleIndicator(const Game& game);
void drawSandboxMenu(const Game& game);
void drawSandboxBrowser(const Game& game);
void drawPickupIcon(PickupType type, ElementType element, Vector2 position, float alpha);
void drawWormholeMouth(Vector2 position, WormholeFacing facing, float radius);

void beginPixelZoom(const Game& game)
{
    BeginMode2D(Camera2D{.offset = Vector2{},
                         .target = Vector2{},
                         .rotation = 0,
                         .zoom = game.resources.renderScale});
}

void beginWorldCamera(const Game& game)
{
    const float zoom = game.resources.renderScale;
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
    case Biome::ShatteredBelt: return Palette::Void;
    case Biome::Rustbloom: return Palette::RustbloomVoid;
    case Biome::SolarForge: return Palette::SolarForgeVoid;
    case Biome::Punctum: return Palette::PunctumVoid;
    }
    return Palette::Void;
}

auto biomeHazeColor(const Game& game) -> Color
{
    switch (currentBiome(game.run.waveNumber))
    {
    case Biome::ShatteredBelt: return Palette::Haze;
    case Biome::Rustbloom: return Palette::RustbloomHaze;
    case Biome::SolarForge: return Palette::SolarForgeHaze;
    case Biome::Punctum: return Palette::PunctumHaze;
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
    windowText(game, "GALAXY IMPACT", game.resources.windowWidth / 2, 80, 50, Palette::Accent);

    drawMenu(game, "", {"Start", "Achievements", "Bestiary", "Settings", "Exit"},
             MenuLayout::titleMenuY);
    drawHighScores(game,
                   game.resources.windowWidth / 2 - static_cast<int32_t>(80 * guiUiScale(game)),
                   static_cast<int32_t>(280 * guiUiScale(game)), game.resources.highScores);
}

void drawShipSelect(const Game& game)
{
    applyGuiScale(game);
    windowText(game, "SELECT SHIP", game.resources.windowWidth / 2, 80, 40, Palette::Accent);

    const ShipDef& highlighted = ships.at(static_cast<size_t>(game.menuIndex));

    const float scale = guiUiScale(game);
    const Vector2 previewPos{.x = static_cast<float>(game.resources.windowWidth) / 2,
                             .y = 260 * scale};
    const float previewRadius = 70 * scale;
    const Vector2 dir = aimAtMouse(game);
    const float previewAngle = std::atan2(dir.y, dir.x) + std::numbers::pi_v<float> / 2;
    const auto shipClass = static_cast<ShipClass>(game.menuIndex);
    drawShipHull(shipClass, previewPos, previewRadius, previewAngle, highlighted.color);

    std::vector<std::string> names;
    names.reserve(ships.size());
    for (const auto& ship : ships)
    {
        names.emplace_back(ship.name);
    }
    drawMenu(game, "", names, MenuLayout::titleMenuY);

    const int32_t descY =
        MenuLayout::titleMenuY +
        (MenuLayout::buttonHeight + MenuLayout::buttonGap) * static_cast<int32_t>(ships.size()) +
        30;
    windowText(game, std::string(highlighted.description).c_str(), game.resources.windowWidth / 2,
               descY, 18, Palette::StructLight);

    const std::string stats = std::format(
        "HP {:.0f}   Armor {:.0f}   Dmg {:+.0f}%   Shields {}", highlighted.maxHealth,
        highlighted.armor, (highlighted.damageMult - 1.0F) * 100.0F, highlighted.maxShieldStacks);
    windowText(game, stats.c_str(), game.resources.windowWidth / 2, descY + 30, 18,
               Palette::StructMid);
}

void drawGameOver(const Game& game)
{
    applyGuiScale(game);
    windowText(game, "YOU WERE DEFEATED!", game.resources.windowWidth / 2, 70, 40, Palette::Accent);

    const std::string statsText =
        std::format("Score: {}   Level: {}   Wave: {}   Time: {:.0f}s", game.run.score,
                    game.run.level, game.run.waveNumber, game.run.runTime);
    windowText(game, statsText.c_str(), game.resources.windowWidth / 2, 120, 20,
               Palette::StructLight);

    drawHighScores(game,
                   game.resources.windowWidth / 2 - static_cast<int32_t>(80 * guiUiScale(game)),
                   static_cast<int32_t>(160 * guiUiScale(game)), game.resources.highScores);
    drawMenu(game, "", {"New Game", "Exit"}, MenuLayout::gameOverMenuY);
}

void drawEnding(const Game& game)
{
    const auto windowW = static_cast<float>(game.resources.windowWidth);
    const auto windowH = static_cast<float>(game.resources.windowHeight);

    for (int32_t i = 0; i < 60; i++)
    {
        const float seed = static_cast<float>(i);
        DrawCircleV(Vector2{.x = hashNoise(seed * 3.1F) * windowW, .y = hashNoise(seed * 7.7F) * windowH},
                   1.2F, Fade(WHITE, 0.5F));
    }

    const Vector2 sunPos{.x = windowW * 0.85F, .y = windowH * 0.2F};
    DrawCircleV(sunPos, 60, Fade(Palette::Accent, 0.25F));
    DrawCircleV(sunPos, 42, Palette::Accent);

    const Vector2 earthPos{.x = windowW * 0.62F, .y = windowH * 0.42F};
    DrawCircleV(earthPos, 28, Palette::Shield);
    DrawCircleV(Vector2Add(earthPos, Vector2{.x = 9, .y = -7}), 9, Palette::Heal);

    DrawCircleV(Vector2{.x = windowW * 0.3F, .y = windowH * 0.72F}, 9, Palette::StructMid);
    DrawCircleV(Vector2{.x = windowW * 0.14F, .y = windowH * 0.28F}, 5, Palette::AccentDim);

    drawShip(game);

    applyGuiScale(game);
    windowText(game, "Thank you for playing this game.", game.resources.windowWidth / 2,
              game.resources.windowHeight - 140, 26, WHITE);
    windowText(game, "Never go to space alone.", game.resources.windowWidth / 2,
              game.resources.windowHeight - 100, 26, WHITE);
    windowText(game, "click to continue", game.resources.windowWidth / 2,
              game.resources.windowHeight - 50, 16, Palette::StructLight);
}

void drawOverlay(const Game& game)
{
    DrawRectangle(0, 0, game.resources.windowWidth, game.resources.windowHeight,
                  Fade(Palette::Void, 0.75F));
}

void drawSettings(const Game& game)
{
    applyGuiScale(game);
    const float scale = guiUiScale(game);
    windowText(game, "SETTINGS", game.resources.windowWidth / 2, 130, 34, Palette::Accent);

    const auto& res =
        resolutionOptions.at(static_cast<size_t>(game.resources.settings.resolutionIndex));
    const std::string displayMode = IsWindowFullscreen() ? "Fullscreen" : "Windowed";

    const std::array<std::string, 6> stepperLabels{"Resolution", "", "", "FPS Cap", "HUD Scale",
                                                   "Display Mode"};
    const std::array<std::string, 6> stepperValues{
        std::format("{}x{}", res.width, res.height),
        "",
        "",
        std::format("{}", fpsOptions.at(static_cast<size_t>(game.resources.settings.fpsIndex))),
        std::format("{:.0f}%",
                    hudScaleOptions.at(static_cast<size_t>(game.resources.settings.hudScaleIndex)) *
                        100),
        displayMode,
    };

    constexpr int32_t rowCount = 7;
    for (int32_t i = 0; i < rowCount; i++)
    {
        const Rectangle rect =
            menuColumnRect(game, i, rowCount, MenuLayout::settingsWidth, MenuLayout::settingsHeight,
                           MenuLayout::settingsGap, MenuLayout::settingsMenuY);

        if (i == game.menuIndex)
        {
            GuiSetState(STATE_FOCUSED);
        }

        if (i == 1 || i == 2)
        {
            const bool on =
                i == 1 ? game.resources.settings.bgmOn : game.resources.settings.soundOn;
            bool checked = on;
            const char* label = i == 1 ? "BGM" : "Sound";
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

auto findEnemyKindIndex(std::string_view name) -> int32_t
{
    for (size_t i = 0; i < enemyKinds.size(); i++)
    {
        if (enemyKinds.at(i).name == name)
        {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

struct UnlockHint
{
    std::string condition;
    std::string progress;
};

auto weaponUnlockHint(const Game& game, WeaponType weapon) -> UnlockHint
{
    const auto& ach = game.resources.achievements;

    auto dashKillsFor = [&](std::string_view kindName) -> int32_t
    {
        const int32_t idx = findEnemyKindIndex(kindName);
        return idx >= 0 ? ach.dashKillsByEnemyKind.at(static_cast<size_t>(idx)) : 0;
    };
    auto skillProgress = [&](SkillType id) -> std::string
    {
        return std::format("Lv {}/{}", game.run.skillLevels.at(static_cast<size_t>(id)),
                           Skills.at(static_cast<size_t>(id)).maxLevel);
    };

    switch (weapon)
    {
    case WeaponType::Forward:
        return {"Ranger's default weapon", ""};
    case WeaponType::Orbit:
        return {"Bastion's default weapon", ""};
    case WeaponType::Beam:
        return {"Interceptor's default weapon", ""};
    case WeaponType::Homing:
        return {"Reach wave 3", std::format("Best: wave {}", ach.highestWaveReached)};
    case WeaponType::Mine:
        return {"Reach wave 7", std::format("Best: wave {}", ach.highestWaveReached)};
    case WeaponType::Shock:
        return {"Reach wave 12", std::format("Best: wave {}", ach.highestWaveReached)};
    case WeaponType::Ricochet:
        return {"Dash-kill a Turret enemy",
                std::format("{} dash kills so far", dashKillsFor("Turret"))};
    case WeaponType::FollowerDrone:
        return {"Dash-kill an Orbiter enemy",
                std::format("{} dash kills so far", dashKillsFor("Orbiter"))};
    case WeaponType::LaserDrone:
        return {"Dash-kill a Laser Fence enemy",
                std::format("{} dash kills so far", dashKillsFor("Laser Fence"))};
    case WeaponType::FlakCannon:
        return {"200 dash/nerve-burst kills total",
                std::format("{}/{}", ach.dashOrNerveKills, flakCannonUnlockKillCount)};
    case WeaponType::Railgun:
        return {"Max the Forward Shot skill", skillProgress(SkillType::ForwardShot)};
    case WeaponType::ChainLightning:
        return {"Max the Orbit Blades skill", skillProgress(SkillType::OrbitBlades)};
    case WeaponType::TurretDeploy:
        return {"Max the Beam Sweep skill", skillProgress(SkillType::BeamSweep)};
    case WeaponType::Flamethrower:
        return {"Max the Homing Missiles skill", skillProgress(SkillType::HomingMissiles)};
    case WeaponType::Sniper:
        return {"Max the Railgun skill", skillProgress(SkillType::Railgun)};
    case WeaponType::Count:
        break;
    }
    return {"", ""};
}

void drawAchievements(const Game& game)
{
    applyGuiScale(game);
    const auto& ach = game.resources.achievements;
    const float scale = guiUiScale(game);
    windowText(game, "ACHIEVEMENTS", game.resources.windowWidth / 2, 60, 34, Palette::Accent);

    const std::string summary =
        std::format("Weapon Slots: {} / 6   Highest Wave: {}   Infinite Mode: {}", ach.slotCap,
                    ach.highestWaveReached, ach.infiniteModeUnlocked ? "Unlocked" : "Locked");
    windowText(game, summary.c_str(), game.resources.windowWidth / 2, 110, 18,
               Palette::StructLight);

    constexpr int32_t rowsPerColumn = 7;
    constexpr int32_t rowHeight = 46;
    constexpr int32_t columnWidth = 520;
    constexpr int32_t iconBoxSize = 30;
    constexpr size_t evolvableWeaponCount = 6;
    const int32_t leftX =
        game.resources.windowWidth / 2 - static_cast<int32_t>(static_cast<float>(columnWidth) *
                                                              scale);
    const int32_t topY = 170;

    const Vector2 mouse = GetMousePosition();
    std::optional<WeaponType> hovered;
    Rectangle hoveredRect{};

    for (size_t i = 0; i < static_cast<size_t>(WeaponType::Count); i++)
    {
        const auto weapon = static_cast<WeaponType>(i);
        const bool unlocked = isWeaponTypeUnlocked(game, weapon);
        const int32_t column = static_cast<int32_t>(i) / rowsPerColumn;
        const int32_t row = static_cast<int32_t>(i) % rowsPerColumn;
        const auto x = static_cast<float>(leftX) +
                      static_cast<float>(column) * static_cast<float>(columnWidth) * scale;
        const auto y =
            static_cast<float>(topY) + static_cast<float>(row) * static_cast<float>(rowHeight) * scale;
        const Color color = unlocked ? Palette::StructLight : Palette::StructMid;

        const Rectangle rowRect{.x = x,
                                .y = y,
                                .width = static_cast<float>(columnWidth) * scale,
                                .height = static_cast<float>(rowHeight) * scale};
        if (CheckCollisionPointRec(mouse, rowRect))
        {
            hovered = weapon;
            hoveredRect = rowRect;
        }

        const Vector2 iconCenter{.x = x + static_cast<float>(iconBoxSize) * scale / 2,
                                 .y = y + static_cast<float>(iconBoxSize) * scale / 2};
        DrawRectangleLines(static_cast<int32_t>(x), static_cast<int32_t>(y),
                           static_cast<int32_t>(static_cast<float>(iconBoxSize) * scale),
                           static_cast<int32_t>(static_cast<float>(iconBoxSize) * scale),
                           Fade(Palette::StructMid, 0.6F));
        drawWeaponIcon(weapon, iconCenter, static_cast<float>(iconBoxSize) * scale * 0.32F, color);

        std::string line = std::string(weaponDisplayName(weapon)) + ": " +
                           (unlocked ? "Unlocked" : "Locked") +
                           std::format(" ({} kills)", ach.killsByWeapon.at(i));
        if (i < evolvableWeaponCount)
        {
            line += ach.evolutionUnlocked.at(i) ? "  [Evo unlocked]" : "  [Evo locked]";
        }
        drawText(game, line.c_str(),
                 static_cast<int32_t>(x + static_cast<float>(iconBoxSize) * scale + 8 * scale),
                 static_cast<int32_t>(y + static_cast<float>(iconBoxSize) * scale / 2 - 9 * scale),
                 18, color);
    }

    if (hovered.has_value())
    {
        const bool unlocked = isWeaponTypeUnlocked(game, *hovered);
        const auto hint = weaponUnlockHint(game, *hovered);
        const size_t idx = static_cast<size_t>(*hovered);

        std::vector<std::string> lines;
        lines.push_back(std::string(weaponDisplayName(*hovered)) +
                        (unlocked ? "  (Unlocked)" : "  (Locked)"));
        if (!unlocked && !hint.condition.empty())
        {
            lines.push_back("Unlocks by: " + hint.condition);
            if (!hint.progress.empty())
            {
                lines.push_back(hint.progress);
            }
        }
        if (idx < evolvableWeaponCount)
        {
            lines.push_back(ach.evolutionUnlocked.at(idx)
                                ? "Evolution: unlocked"
                                : std::format("Evolution: {}/{} kills with this weapon",
                                              ach.killsByWeapon.at(idx), evolutionKillThreshold));
        }

        constexpr int32_t lineFontSize = 15;
        constexpr int32_t lineHeight = 20;
        int32_t boxWidth = 0;
        for (const auto& l : lines)
        {
            boxWidth = std::max(boxWidth, measureText(game, l.c_str(), lineFontSize));
        }
        boxWidth += 16;
        const int32_t boxHeight = static_cast<int32_t>(lines.size()) * lineHeight + 12;

        auto tipX = static_cast<int32_t>(hoveredRect.x);
        if (tipX + boxWidth > game.resources.windowWidth)
        {
            tipX = game.resources.windowWidth - boxWidth;
        }
        const auto tipY = static_cast<int32_t>(hoveredRect.y + hoveredRect.height);

        DrawRectangle(tipX, tipY, boxWidth, boxHeight, Fade(Palette::Void, 0.92F));
        DrawRectangleLines(tipX, tipY, boxWidth, boxHeight, Palette::StructMid);
        for (size_t li = 0; li < lines.size(); li++)
        {
            drawText(game, lines.at(li).c_str(), tipX + 8, tipY + 6 + static_cast<int32_t>(li) * lineHeight,
                     lineFontSize, li == 0 ? Palette::Accent : Palette::StructLight);
        }
    }

    windowText(game, "Hover a weapon for unlock details   |   Click / Enter / Esc: back",
               game.resources.windowWidth / 2,
               topY + static_cast<int32_t>(static_cast<float>(rowsPerColumn * rowHeight) * scale) +
                   30,
               16, Palette::StructMid);
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

        if (CheckCollisionPointRec(mouse, box))
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

void drawBestiary(const Game& game)
{
    applyGuiScale(game);
    const auto& bestiary = game.resources.bestiary;
    const float scale = guiUiScale(game);

    windowText(game, "BESTIARY", game.resources.windowWidth / 2, 60, 34, Palette::Accent);

    const auto enemyCount =
        std::count(bestiary.enemyKilled.begin(), bestiary.enemyKilled.end(), true);
    const auto bossCount = std::count(bestiary.bossKilled.begin(), bestiary.bossKilled.end(), true);
    const auto hazardCount =
        std::count(bestiary.hazardKilled.begin(), bestiary.hazardKilled.end(), true);
    const std::string summary =
        std::format("Enemies: {} of {}   Bosses: {} of {}   Hazards: {} of {}", enemyCount,
                    enemyKinds.size(), bossCount, bestiaryBossKindCount, hazardCount,
                    bestiaryHazardKindCount);
    windowText(game, summary.c_str(), game.resources.windowWidth / 2, 100, 18,
               Palette::StructLight);

    float y = 140 * scale;
    y = drawBestiarySection(
        game, "ENEMIES", y, scale, enemyKinds.size(),
        [&](size_t i) { return bestiary.enemyKilled.at(i); },
        [](size_t i) -> std::string_view { return enemyKinds.at(i).name; },
        [](size_t i, Vector2 c, float r, Color col)
        { drawEnemyShape(enemyKinds.at(i).shape, c, r, 0, col); },
        [](size_t i) { return enemyKinds.at(i).color; });

    y = drawBestiarySection(
        game, "BOSSES", y, scale, bestiaryBossKindCount,
        [&](size_t i) { return bestiary.bossKilled.at(i); },
        [](size_t i) { return bossBestiaryName(static_cast<int32_t>(i)); },
        [](size_t i, Vector2 c, float r, Color col) {
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
    constexpr const char* footerText = "Click, Enter, or Esc: back";
    const int32_t footerWidth = measureText(game, footerText, footerSize);
    drawText(game, footerText, game.resources.windowWidth / 2 - footerWidth / 2,
             static_cast<int32_t>(y), footerSize, Palette::StructMid);
}

void drawSkillIcon(SkillType id, Vector2 c, float s, Color color)
{
    if (const auto kind = weaponForGrantSkill(id); kind.has_value())
    {
        drawWeaponIcon(*kind, c, s, color);
        return;
    }
    drawPassiveIcon(id, c, s, color);
}

void drawWeaponIcon(WeaponType kind, Vector2 c, float s, Color color)
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
    case PickupType::DashTrail:
        return "Dashing leaves a damaging trail behind you, for a while.";
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

    constexpr size_t evolvableWeaponCount = 6;
    for (size_t i = 0; i < evolvableWeaponCount; i++)
    {
        const auto kind = static_cast<WeaponType>(i);
        if (skillLinkedPassive.at(i) == id && hasWeapon(game, kind))
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

    constexpr size_t evolvableWeaponCount = 6;

    struct Slot
    {
        SkillType id;
        Color color;
        int32_t level = 0;
        bool stolen = false;
        bool evolutionReady = false;
    };
    std::vector<Slot> slots;

    for (const auto& w : game.run.weapons)
    {
        const Color color = w.evolved ? Palette::Crit : Palette::Shield;
        const auto typeIndex = static_cast<size_t>(w.type);
        const bool evolutionReady =
            typeIndex < evolvableWeaponCount && !w.evolved &&
            game.resources.achievements.evolutionUnlocked.at(typeIndex);
        const bool stolen =
            game.run.weaponDowngrade.has_value() && game.run.weaponDowngrade->type == w.type;
        slots.push_back(Slot{.id = weaponGrantSkill.at(typeIndex),
                             .color = color,
                             .level = w.level,
                             .stolen = stolen,
                             .evolutionReady = evolutionReady});
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
        slots.push_back(Slot{.id = id, .color = Palette::Shield, .level = game.run.skillLevels.at(i)});
    }

    const Vector2 mouse = mouseUIPos(game);
    std::optional<SkillType> hovered;
    Vector2 hoveredBoxPos{};

    for (int32_t i = 0; i < game.resources.achievements.slotCap; i++)
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
            drawSkillIcon(slot.id, center, boxSize * iconScale, slot.color);

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

            if (CheckCollisionPointRec(
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
        const std::string name = std::format("{} (Lv {})", def.name, lvl);
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
    windowText(game, std::format("LEVEL {}", game.run.level).c_str(),
               game.resources.windowWidth / 2, 100, 34, Palette::Accent);

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

        switch (choice.type)
        {
        case ChoiceType::Evolve:
            name = std::format("EVOLVE: {}",
                               evolvedWeaponName.at(static_cast<size_t>(choice.weapon.value())));
            desc = "Fuses the weapon and its linked passive into a super weapon.";
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
                iconRadius * scale, color);
        }
        drawText(game, name.c_str(), nameX, nameY, nameSize, color);
        windowText(game, desc.c_str(), static_cast<int32_t>(rect.x + rect.width / 2),
                   static_cast<int32_t>(static_cast<float>(descY) / scale), descFontSize,
                   Palette::StructMid);
    }
}

void drawMenu(const Game& game, const std::string& heading, const std::vector<std::string>& options,
              int32_t y)
{
    applyGuiScale(game);
    if (!heading.empty())
    {
        windowText(game, heading.c_str(), game.resources.windowWidth / 2, y - 50, 30,
                   Palette::StructLight);
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
        GuiButton(rect, options.at(static_cast<size_t>(i)).c_str());
        GuiSetState(STATE_NORMAL);
    }
}

void drawHighScores(const Game& game, int32_t x, int32_t y, const std::vector<int32_t>& scores)
{
    drawText(game, "Top Scores", x, y, 26, Palette::AccentDim);

    if (scores.empty())
    {
        drawText(game, "(none yet)", x, y + 32, 24, Palette::StructMid);
        return;
    }

    for (size_t i = 0; i < scores.size(); i++)
    {
        const std::string line = std::format("{}. {}", i + 1, scores.at(i));
        drawText(game, line.c_str(), x, y + 32 + static_cast<int32_t>(i) * 30, 24,
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
            const Vector2 blobCenter{.x = cellCenter.x + jx, .y = cellCenter.y + jy};
            const float blobRadius =
                cellSize * (0.74F + (hashNoise(x * 0.037F + y * 0.053F + 1.7F) - 0.5F) * 0.3F);

            DrawCircleV(blobCenter, blobRadius, Fade(rock, 0.38F));
            DrawCircleLines(static_cast<int32_t>(blobCenter.x), static_cast<int32_t>(blobCenter.y),
                            blobRadius, Fade(ColorLerp(rock, WHITE, 0.2F), 0.2F));
            if (shade > 0.78F)
            {
                const float ex = (hashNoise(x * 0.091F + y * 0.061F + 2.3F) - 0.5F) * cellSize * 0.5F;
                const float ey = (hashNoise(x * 0.061F + y * 0.091F + 6.7F) - 0.5F) * cellSize * 0.5F;

                const float glowPulse =
                    0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 1.3F + shade * 30.0F);
                const Color emberColor = ColorLerp(Palette::ElementBurn, YELLOW, glowPulse * 0.5F);
                DrawCircleV(Vector2{.x = cellCenter.x + ex, .y = cellCenter.y + ey}, cellSize * 0.16F,
                           Fade(emberColor, 0.5F + glowPulse * 0.3F));
            }
        }
    }
}

void drawRustbloomBackground(const Game& game)
{
    if (currentBiome(game.run.waveNumber) != Biome::Rustbloom)
    {
        return;
    }

    const auto t = static_cast<float>(GetTime());

    {
        constexpr float fragmentCellSize = 260.0F;
        const float halfW = static_cast<float>(game.resources.screenWidth) / 2 + fragmentCellSize;
        const float halfH = static_cast<float>(game.resources.screenHeight) / 2 + fragmentCellSize;
        const Vector2 fragArea = game.run.player.position;
        const float fragStartX = std::floor((fragArea.x - halfW) / fragmentCellSize) * fragmentCellSize;
        const float fragStartY = std::floor((fragArea.y - halfH) / fragmentCellSize) * fragmentCellSize;
        for (float y = fragStartY; y < fragArea.y + halfH; y += fragmentCellSize)
        {
            for (float x = fragStartX; x < fragArea.x + halfW; x += fragmentCellSize)
            {
                const float presence = hashNoise(x * 0.011F + y * 0.023F + 50.0F);
                if (presence > 0.35F)
                {
                    continue;
                }
                const float jx =
                    (hashNoise(x * 0.031F + y * 0.013F + 61.0F) - 0.5F) * fragmentCellSize * 0.5F;
                const float jy =
                    (hashNoise(x * 0.013F + y * 0.031F + 67.0F) - 0.5F) * fragmentCellSize * 0.5F;
                const Vector2 fragCenter{.x = x + fragmentCellSize / 2 + jx,
                                         .y = y + fragmentCellSize / 2 + jy};
                const float fragRadius = fragmentCellSize * (0.22F + presence * 0.25F);
                const float rot = hashNoise(presence * 71.0F) * 360.0F;

                const float shapeRoll = hashNoise(presence * 53.0F + 4.0F);
                const int32_t sides = shapeRoll < 0.2F ? 0 : 3 + static_cast<int32_t>(shapeRoll * 7.5F) % 6;
                drawPanelShape(fragCenter, fragRadius, sides, rot, Fade(Palette::StructMid, 0.5F),
                              presence * 90.0F);
            }
        }
    }

    constexpr float cellSize = rustbloomPodCellSize;
    const float halfW = static_cast<float>(game.resources.screenWidth) / 2 + cellSize;
    const float halfH = static_cast<float>(game.resources.screenHeight) / 2 + cellSize;
    const Vector2 center = game.run.player.position;
    const float startX = std::floor((center.x - halfW) / cellSize) * cellSize;
    const float startY = std::floor((center.y - halfH) / cellSize) * cellSize;

    constexpr float podChance = rustbloomPodChance;
    auto podPresence = [](float px, float py) -> float { return hashNoise(px * 0.021F + py * 0.037F); };

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
            const Vector2 podCenter{.x = x + cellSize / 2 + jx, .y = y + cellSize / 2 + jy};

            if (podPresence(x + cellSize, y) <= podChance)
            {
                const Vector2 neighbor{.x = x + cellSize * 1.5F, .y = y + cellSize / 2};
                DrawLineEx(podCenter, neighbor, 2.5F, Fade(Palette::RustbloomHaze, 0.25F));
            }
            if (podPresence(x, y + cellSize) <= podChance)
            {
                const Vector2 neighbor{.x = x + cellSize / 2, .y = y + cellSize * 1.5F};
                DrawLineEx(podCenter, neighbor, 2.5F, Fade(Palette::RustbloomHaze, 0.25F));
            }

            const float sizeSeed = hashNoise(x * 0.043F + y * 0.017F + 9.0F);
            const float podRadius = cellSize * (0.14F + sizeSeed * 0.06F);
            DrawCircleV(podCenter, podRadius, Fade(Palette::RustbloomAccent, 0.3F));
            DrawCircleV(Vector2Add(podCenter, Vector2{.x = podRadius * 0.5F, .y = podRadius * 0.3F}),
                       podRadius * 0.6F, Fade(Palette::RustbloomHaze, 0.25F));

            const float pulse = 0.5F + 0.5F * std::sin(t * 1.5F + x * 0.01F + y * 0.01F);
            DrawCircleV(podCenter, podRadius * 0.3F, Fade(Palette::Heal, 0.3F + 0.3F * pulse));
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
        const Vector2 point =
            Vector2Add(Vector2Add(start, Vector2Scale(dir, length * t)), Vector2Scale(perp, jitter));
        DrawLineEx(prev, point, 3.0F * frac, Fade(WHITE, 0.7F * frac));
        DrawLineEx(prev, point, 7.0F * frac, Fade(Palette::PunctumAccent, 0.35F * frac));

        if (i > 1 && i < segments && hashNoise(jitterSeed + 50.0F) > 0.6F)
        {
            const float branchAngle = (hashNoise(jitterSeed + 90.0F) - 0.5F) * 90.0F * DEG2RAD;
            const Vector2 branchDir{.x = dir.x * std::cos(branchAngle) - dir.y * std::sin(branchAngle),
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
        const float baseAngle = static_cast<float>(i) / static_cast<float>(armCount) * 360.0F + rotation;
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

void drawBlackHoleTwirl(Vector2 center, float radius, const std::vector<BlackHoleDustParticle>& dust)
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
            const Vector2 start = Vector2Add(
                center, Vector2{.x = std::cos(angle) * radius * 0.15F,
                               .y = std::sin(angle) * radius * 0.15F});
            const Vector2 end = Vector2Add(
                center, Vector2{.x = std::cos(angle) * radius * 0.95F,
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
            const Vector2 start = Vector2Add(
                center, Vector2{.x = std::cos(angle) * radius * 0.15F,
                               .y = std::sin(angle) * radius * 0.15F});
            const Vector2 end = Vector2Add(
                center, Vector2{.x = std::cos(angle) * radius * 0.95F,
                               .y = std::sin(angle) * radius * 0.95F});
            DrawLineEx(start, end, 2.5F, Fade(Palette::Void, 0.75F * (1.0F - progress)));
        }

        constexpr int32_t fragmentCount = 8;
        for (int32_t i = 0; i < fragmentCount; i++)
        {
            const float seed = static_cast<float>(i) * 19.0F;
            const float angle = hashNoise(seed) * 2.0F * std::numbers::pi_v<float>;
            const float dist = radius * 0.5F * progress;
            const Vector2 pos =
                Vector2Add(center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
            const float rot = hashNoise(seed + 1.0F) * 360.0F;
            DrawPoly(pos, 5, radius * 0.16F * (1.0F - progress * 0.5F), rot,
                    Fade(Palette::StructDark, 1.0F - progress));
        }
    }
}

void drawGameplayWorld(const Game& game)
{
    drawBackgroundStars(game, game.run.player.position);
    drawPunctumLightning(game);
    drawSolarForgeCaveBackground(game);
    drawRustbloomBackground(game);

    if (game.run.bossCutscenePhase == 11 || game.run.bossCutscenePhase == 12)
    {
        drawKrakenRemains(game);
    }

    constexpr float arenaBoundaryVisibleMargin = 2000.0F;
    if (const float distFromCenter = Vector2Length(game.run.player.position);
        distFromCenter > GameConstants::arenaHalf - arenaBoundaryVisibleMargin)
    {
        DrawCircleLines(0, 0, GameConstants::arenaHalf, Fade(Palette::Shield, 0.5F));
        DrawCircleLines(0, 0, GameConstants::arenaHalf - 6, Fade(Palette::Accent, 0.25F));
    }

    for (const auto& c : game.run.gasClouds)
    {
        DrawCircleV(c.position, c.radius, c.color);
    }

    if (game.run.blackhole.active)
    {
        drawBlackHoleTwirl(game.run.blackhole.position, game.run.blackhole.influenceRadius,
                            game.run.blackHoleDust);
    }

    if (game.run.wormhole.active)
    {
        drawWormholeMouth(game.run.wormhole.positionA, game.run.wormhole.facingA,
                          game.run.wormhole.radius);
        drawWormholeMouth(game.run.wormhole.positionB, game.run.wormhole.facingB,
                          game.run.wormhole.radius);
    }

    for (const auto& cloud : game.run.gasHazards)
    {
        const float pulse =
            0.85F + 0.15F * std::sin(static_cast<float>(GetTime()) * 2.2F + cloud.seed);
        const float fadeAlpha = std::min(1.0F, cloud.life / 1.5F);
        const Color cloudColor = cloud.isFire
                                     ? ColorLerp(Palette::Accent, Palette::Crit, 0.3F)
                                     : ColorLerp(Palette::RustbloomAccent, Palette::RustbloomHaze, 0.3F);
        drawFluidBlob(cloud.position, cloud.radius * pulse, Fade(cloudColor, 0.55F * fadeAlpha),
                     cloud.seed, Vector2{});
    }

    for (const auto& a : game.run.asteroids)
    {
        if (a.isFluid)
        {
            const float pulse =
                0.85F + 0.15F * std::sin(static_cast<float>(GetTime()) * 3.0F + a.position.x);
            drawFluidBlob(a.position, a.radius * pulse, Palette::SolarForgeAccent,
                         a.position.x * 0.037F + a.position.y * 0.019F, a.velocity);
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

    for (const auto& hazard : game.run.eliteHazards)
    {
        drawEliteHazard(hazard);
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
            const Vector2 baseA = Vector2Add(c, Vector2Add(Vector2Scale(dir, 3.0F), Vector2Scale(perp, 2.2F)));
            const Vector2 baseB = Vector2Add(c, Vector2Subtract(Vector2Scale(dir, 3.0F), Vector2Scale(perp, 2.2F)));
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
        const Vector2 toTarget = drone.beamFlashTimer > 0
                                     ? Vector2Subtract(drone.beamTarget, c)
                                     : Vector2{.x = 0, .y = -1};
        const Vector2 dir = Vector2Length(toTarget) > 0.01F ? Vector2Normalize(toTarget)
                                                            : Vector2{.x = 0, .y = -1};
        const Vector2 perp{.x = -dir.y, .y = dir.x};
        const Vector2 nose = Vector2Add(c, Vector2Scale(dir, 9.0F));
        const Vector2 tailA = Vector2Add(c, Vector2Add(Vector2Scale(dir, -6.0F), Vector2Scale(perp, 5.0F)));
        const Vector2 tailB = Vector2Add(c, Vector2Subtract(Vector2Scale(dir, -6.0F), Vector2Scale(perp, 5.0F)));
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

    for (const auto& bolt : game.run.chainLightningBolts)
    {
        DrawLineEx(bolt.from, bolt.to, 2, Fade(Palette::Crit, bolt.timer / 0.2F));
    }

    for (const auto& shot : game.run.sniperShots)
    {
        const float fade = shot.timer / sniperLineFlashDuration;
        DrawLineEx(shot.from, shot.to, 3, Fade(Palette::StructLight, fade));
        DrawLineEx(shot.from, shot.to, 1, Fade(WHITE, fade));
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
        else if (p.homing)
        {
            drawComet(p);
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

    if (game.run.nerveBurstFlashTimer > 0)
    {
        drawNerveBurstFlash(game);
    }

    if (game.run.player.shieldActive)
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

    for (const auto& p : game.run.dashTrailParticles)
    {
        const float fade = p.life / p.maxLife;
        const float flicker = 0.75F + 0.25F * std::sin(static_cast<float>(GetTime()) * 24.0F +
                                                       p.position.x * 0.1F + p.position.y * 0.1F);

        DrawCircleV(p.position, p.radius * fade, Fade(p.color, 0.18F * fade * flicker));
        DrawCircleLines(static_cast<int32_t>(p.position.x), static_cast<int32_t>(p.position.y),
                        p.radius * fade * 0.65F, Fade(Palette::Crit, 0.5F * fade));
        DrawCircleV(p.position, p.radius * fade * 0.22F, Fade(WHITE, 0.6F * fade * flicker));
    }

    for (const auto& p : game.run.flameParticles)
    {
        const float fade = p.life / p.maxLife;
        const float flicker = 0.75F + 0.25F * std::sin(static_cast<float>(GetTime()) * 24.0F +
                                                       p.position.x * 0.1F + p.position.y * 0.1F);

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
        DrawPoly(p, 6, r, angle * RAD2DEG, shipColor);
        DrawPolyLines(p, 6, r, angle * RAD2DEG, Fade(Palette::StructDark, 0.6F));

        const Vector2 wingLeftFront =
            Vector2Add(p, rotate(Vector2{.x = -r * 0.5F, .y = -r * 0.3F}));
        const Vector2 wingLeftTip = Vector2Add(p, rotate(Vector2{.x = -r * 1.3F, .y = r * 0.2F}));
        const Vector2 wingLeftBack = Vector2Add(p, rotate(Vector2{.x = -r * 0.5F, .y = r * 0.9F}));
        DrawTriangle(wingLeftFront, wingLeftTip, wingLeftBack, Fade(shipColor, 0.85F));

        const Vector2 wingRightFront =
            Vector2Add(p, rotate(Vector2{.x = r * 0.5F, .y = -r * 0.3F}));
        const Vector2 wingRightTip = Vector2Add(p, rotate(Vector2{.x = r * 1.3F, .y = r * 0.2F}));
        const Vector2 wingRightBack = Vector2Add(p, rotate(Vector2{.x = r * 0.5F, .y = r * 0.9F}));
        DrawTriangle(wingRightTip, wingRightFront, wingRightBack, Fade(shipColor, 0.85F));
        break;
    }
    case ShipClass::Interceptor:
    {
        const Vector2 dartTip = Vector2Add(p, rotate(Vector2{.x = 0, .y = -r * 1.3F}));
        const Vector2 dartLeft = Vector2Add(p, rotate(Vector2{.x = -r * 0.45F, .y = r * 0.9F}));
        const Vector2 dartRight = Vector2Add(p, rotate(Vector2{.x = r * 0.45F, .y = r * 0.9F}));
        DrawTriangle(dartTip, dartLeft, dartRight, shipColor);
        DrawTriangleLines(dartTip, dartLeft, dartRight, Fade(Palette::StructDark, 0.6F));

        const Vector2 finLeftInner = Vector2Add(p, rotate(Vector2{.x = -r * 0.2F, .y = r * 0.5F}));
        const Vector2 finLeftOuter = Vector2Add(p, rotate(Vector2{.x = -r * 1.1F, .y = r * 1.3F}));
        DrawTriangle(dartLeft, finLeftInner, finLeftOuter, Fade(shipColor, 0.8F));

        const Vector2 finRightInner = Vector2Add(p, rotate(Vector2{.x = r * 0.2F, .y = r * 0.5F}));
        const Vector2 finRightOuter = Vector2Add(p, rotate(Vector2{.x = r * 1.1F, .y = r * 1.3F}));
        DrawTriangle(finRightInner, dartRight, finRightOuter, Fade(shipColor, 0.8F));
        break;
    }
    case ShipClass::Ranger:
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

    DrawCircleV(p, r * 0.3F, Palette::StructLight);
}

void drawShip(const Game& game)
{
    const Vector2 p = game.run.player.position;

    const bool twirlingIn = game.run.bossCutscenePhase == 10;
    const float twirlProgress =
        twirlingIn ? std::clamp(1.0F - game.run.bossCutsceneTimer / banishedEntryTwirlDuration,
                                0.0F, 1.0F)
                   : 0.0F;
    const float r = game.run.player.radius * (twirlingIn ? std::max(0.05F, 1.0F - twirlProgress) : 1.0F);

    const Vector2 dir = aimAtMouse(game);
    const float angle = std::atan2(dir.y, dir.x) + std::numbers::pi_v<float> / 2 +
                        (twirlingIn ? twirlProgress * 900.0F * DEG2RAD : 0.0F);

    const auto rotate = [angle](Vector2 v) -> Vector2
    {
        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);
        return Vector2{.x = v.x * cosA - v.y * sinA, .y = v.x * sinA + v.y * cosA};
    };

    if (twirlingIn)
    {
        const auto shipClass = static_cast<ShipClass>(game.resources.settings.shipIndex);
        const Color shipColor = ColorLerp(game.run.player.color, BLACK, twirlProgress);
        drawShipHull(shipClass, p, r, angle, shipColor);
        return;
    }

    if (game.run.player.dashing)
    {
        const Vector2 dashDir = Vector2Normalize(game.run.player.dashVelocity);
        const Color trailColor =
            game.run.player.dashTrailTimer > 0 ? Palette::Accent : Palette::Crit;
        for (int i = 1; i <= 3; i++)
        {
            const Vector2 trailPos =
                Vector2Subtract(p, Vector2Scale(dashDir, static_cast<float>(i) * 10));
            DrawCircleV(trailPos, r * (1 - static_cast<float>(i) * 0.2F),
                        Fade(trailColor, 0.35F / static_cast<float>(i)));
        }
    }

    const Color shipColor = game.run.player.dashing ? Palette::Crit : game.run.player.color;

    const float flicker = 0.65F + 0.35F * std::sin(static_cast<float>(GetTime()) * 28.0F);
    const Vector2 flameTip =
        Vector2Add(p, rotate(Vector2{.x = 0, .y = r * (1.5F + 0.6F * flicker)}));
    const Vector2 flameLeft = Vector2Add(p, rotate(Vector2{.x = -r * 0.35F, .y = r * 0.85F}));
    const Vector2 flameRight = Vector2Add(p, rotate(Vector2{.x = r * 0.35F, .y = r * 0.85F}));
    DrawTriangle(flameLeft, flameTip, flameRight, Fade(Palette::Charge, 0.75F * flicker));
    DrawTriangle(flameLeft,
                 Vector2Add(p, rotate(Vector2{.x = 0, .y = r * (1.0F + 0.3F * flicker)})),
                 flameRight, Fade(Palette::StructLight, 0.6F * flicker));

    const auto shipClass = static_cast<ShipClass>(game.resources.settings.shipIndex);
    drawShipHull(shipClass, p, r, angle, shipColor);

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

void drawNerveBurstFlash(const Game& game)
{
    const float flashFrac = game.run.nerveBurstFlashTimer / 0.15F;
    const Vector2 start = game.run.player.position;
    const Vector2 end = game.run.nerveBurstFlashEnd;
    const Vector2 toEnd = Vector2Subtract(end, start);
    const float range = Vector2Length(toEnd);
    if (range < 0.01F)
    {
        return;
    }
    const Vector2 dir = Vector2Scale(toEnd, 1.0F / range);
    const float halfAngle = nerveConeHalfAngleDeg * DEG2RAD;

    const auto rotate = [&](float angle) -> Vector2
    {
        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);
        return Vector2Add(
            start, Vector2Scale(Vector2{.x = dir.x * cosA - dir.y * sinA,
                                        .y = dir.x * sinA + dir.y * cosA},
                                range));
    };

    constexpr int32_t segments = 12;
    for (int32_t i = 0; i < segments; i++)
    {
        const float a0 = -halfAngle + 2 * halfAngle * static_cast<float>(i) / segments;
        const float a1 = -halfAngle + 2 * halfAngle * static_cast<float>(i + 1) / segments;
        DrawTriangle(start, rotate(a1), rotate(a0), Fade(Palette::Charge, flashFrac * 0.35F));
    }
    DrawLineEx(start, rotate(-halfAngle), 4 * flashFrac, Fade(WHITE, flashFrac));
    DrawLineEx(start, rotate(halfAngle), 4 * flashFrac, Fade(WHITE, flashFrac));
    DrawCircleV(start, 14 * flashFrac, Fade(WHITE, flashFrac * 0.8F));
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
        const Vector2 pos = Vector2Add(
            center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        const float pieceRadius = baseRadius * (0.35F + hashNoise(pieceSeed + 2.0F) * 0.45F);
        const auto colorIdx = static_cast<size_t>(hashNoise(pieceSeed + 3.0F) *
                                                   static_cast<float>(palette.size())) %
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
        const Vector2 pos = Vector2Add(
            center, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
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
    const Vector2 blobCenter = Vector2Add(
        center, Vector2{.x = std::cos(offsetAngle) * offsetDist, .y = std::sin(offsetAngle) * offsetDist});

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

auto hashNoise(float seed) -> float
{
    const float v = std::sin(seed) * 43758.5453F;
    return v - std::floor(v);
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
            float boltSeed = pos.x * 0.7F + pos.y * 0.31F + crackleStep * 3.1F +
                             static_cast<float>(i) * 91.0F;
            const float startAngle = hashNoise(boltSeed) * 2.0F * std::numbers::pi_v<float>;
            Vector2 point{.x = pos.x + std::cos(startAngle) * radius,
                         .y = pos.y + std::sin(startAngle) * radius};
            for (int32_t s = 0; s < segments; s++)
            {
                boltSeed += 7.7F;
                const Vector2 next{
                    .x = pos.x + (hashNoise(boltSeed) - 0.5F) * 2.0F * radius,
                    .y = pos.y + (hashNoise(boltSeed + 3.3F) - 0.5F) * 2.0F * radius};
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

    if (kind.pattern == EnemyPattern::Charge && enemy.telegraphing)
    {
        const Vector2 dir = Vector2Normalize(enemy.velocity);
        const float blink = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 14.0F);

        const float dashDistance = Vector2Length(enemy.velocity) * UpdateConstants::frameScale *
                                   UpdateConstants::enemyChargeDashDuration;
        const Vector2 lineEnd = Vector2Add(enemy.position, Vector2Scale(dir, dashDistance));
        DrawLineEx(enemy.position, lineEnd, 3, Fade(Palette::Crit, blink));
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
    if (kind.shape == EnemyShape::Organic)
    {
        drawOrganicEnemyShape(enemy.position, kind.radius * enemy.organicRadiusMult,
                              enemy.organicSeed, color);
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

void drawEliteHazard(const EliteHazard& hazard)
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

    DrawCircleV(hazard.position, EliteHazardConstants::radius, Palette::StructDark);
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
        const Vector2 craterPos = Vector2Add(
            center, Vector2{.x = std::cos(craterAngle) * craterDist,
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
        const Vector2 base = Vector2Add(
            center, Vector2{.x = std::cos(angle + 1.5708F) * offsetDist,
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

void drawWreckwormSegmentShape(Vector2 center, float radius, float seed, Color baseColor, bool armor)
{
    drawPanelHex(center, radius, seed * 40.0F, baseColor, seed);
    if (armor)
    {
        return;
    }
    const auto t = static_cast<float>(GetTime());
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

    const float deathElapsed = bossDeathAnimDuration - boss.deathAnimTimer;
    if (boss.deathAnimTimer >= 0 && deathElapsed < bossDeathBlinkDuration)
    {
        const float blink = std::sin(static_cast<float>(GetTime()) * 30.0F) > 0 ? 1.0F : 0.0F;
        ufoColor = ColorLerp(ufoColor, WHITE, blink);
    }

    if (boss.isSlagmaw)
    {
        drawMeteorBossShape(bossCenter, boss.size.x / 2, boss.instanceId,
                            Fade(ufoColor, boss.ghostFadeAlpha));
    }
    else if (boss.isWreckwormHead && boss.krakenSnakeVariant)
    {
        drawMeteorBossShape(bossCenter, boss.size.x / 2, boss.instanceId, ufoColor);
    }
    else if (boss.isWreckwormHead)
    {
        drawWreckwormHeadShape(bossCenter, boss.size.x / 2, static_cast<float>(boss.instanceId),
                               ufoColor, boss.wreckwormVelocity);
    }
    else if (boss.isWreckwormSegment && boss.krakenSnakeVariant)
    {
        drawBossHull(BossShape::HexPlated, bossCenter, boss.size, ufoColor);
    }
    else if (boss.isWreckwormSegment)
    {
        drawWreckwormSegmentShape(bossCenter, boss.size.x / 2, static_cast<float>(boss.segmentIndex + 1),
                                  ufoColor, boss.isArmorSegment);
    }
    else if (boss.isKraken && boss.krakenEncounter >= 3 && game.run.bossCutscenePhase == 4)
    {
        const float progress =
            std::clamp(1.0F - game.run.bossCutsceneTimer / krakenOutroTwirlDuration, 0.0F, 1.0F);
        const float shrink = std::max(0.04F, 1.0F - progress);
        const Color shadowColor = ColorLerp(ufoColor, BLACK, progress);
        const float twirlSeed =
            static_cast<float>(boss.instanceId) + progress * 40.0F + static_cast<float>(GetTime()) * 4.0F;
        drawKrakenShape(bossCenter, boss.size.x / 2 * shrink, twirlSeed, shadowColor);
    }
    else if (boss.isKraken)
    {
        drawKrakenShape(bossCenter, boss.size.x / 2, static_cast<float>(boss.instanceId), ufoColor);
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

    if (boss.isKraken && boss.tentaclePhase != 0)
    {
        drawKrakenTentacle(boss);
    }
    if (boss.isKraken)
    {
        for (int32_t slot = 0; slot < Boss::krakenLimbSlots; slot++)
        {
            drawKrakenLimb(boss, slot);
        }
    }
    if (boss.isBanished)
    {
        for (int32_t slot = 0; slot < Boss::banishedTentacleCount; slot++)
        {
            drawBanishedTentacle(bossCenter, boss, slot);
        }
        if (boss.banishedStage == 1 || boss.banishedStage == 2)
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
                const Vector2 start = Vector2Add(
                    bossCenter, Vector2{.x = std::cos(angle) * radius * 0.15F,
                                       .y = std::sin(angle) * radius * 0.15F});
                const Vector2 end = Vector2Add(
                    bossCenter, Vector2{.x = std::cos(angle) * radius * 0.95F,
                                       .y = std::sin(angle) * radius * 0.95F});
                DrawLineEx(start, end, 2.5F, Fade(Palette::Void, 0.75F));
            }
        }
        else if (deathElapsed >= bossDeathBlinkDuration + bossDeathCrackDuration)
        {
            const float gatherElapsed = deathElapsed - bossDeathBlinkDuration - bossDeathCrackDuration;
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
                -90.0F + 360.0F * std::clamp(boss.shieldGenProgress, 0.0F, 1.0F), 48, Palette::Shield);
    }

    if (boss.isBeltbreaker && boss.beltbreakerShielded)
    {

        const float total = UpdateConstants::beltbreakerShieldedDuration(boss.plateCount);
        const float frac = std::clamp(boss.beltbreakerShieldTimer / total, 0.0F, 1.0F);
        const float alpha = 0.35F + 0.25F * (std::sin(static_cast<float>(GetTime()) * 3.0F) + 1.0F) * 0.5F;
        const float ringRadius = boss.size.x * 0.65F;
        DrawCircleLinesV(bossCenter, ringRadius, ColorAlpha(Palette::Shield, alpha * 0.4F));
        DrawRing(bossCenter, ringRadius - 3, ringRadius + 3, -90.0F, -90.0F + 360.0F * frac, 48,
                ColorAlpha(Palette::Shield, alpha + 0.3F));
    }

    if (boss.state == BossState::WINDING_UP)
    {
        const float progress =
            std::clamp(1 - boss.stateTimer / bossWindupDuration(boss.attack), 0.0F, 1.0F);
        drawChargeParticles(bossCenter, progress, boss.color, 100, 65);

        if (boss.attack == BossAttack::Beam || boss.attack == BossAttack::WormholeBeam)
        {
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

    if (boss.state == BossState::SHOOTING && boss.attack == BossAttack::GravityWell)
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

            break;
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
    const float speed = Vector2Length(projectile.velocity);
    Vector2 tailDir{.x = 0, .y = -1};
    if (speed > 0)
    {
        tailDir = Vector2Scale(projectile.velocity, -1 / speed);
    }

    constexpr int segments = 6;
    for (int i = segments; i >= 1; i--)
    {
        const float frac = static_cast<float>(i) / segments;
        const Vector2 segPos = Vector2Add(
            projectile.position, Vector2Scale(tailDir, projectile.radius * 2.2F * static_cast<float>(i)));
        const Color flame =
            i % 2 == 0 ? Palette::ElementBurn : ColorLerp(Palette::ElementBurn, YELLOW, 0.5F);
        DrawCircleV(segPos, projectile.radius * (1 - frac * 0.5F), Fade(flame, 0.55F * (1 - frac * 0.6F)));
    }

    DrawCircleV(projectile.position, projectile.radius * 1.15F, Fade(Palette::ElementBurn, 0.35F));
    DrawCircleV(projectile.position, projectile.radius,
               ColorLerp(Palette::SolarForgeAccent, Palette::Void, 0.4F));
    DrawCircleLines(static_cast<int32_t>(projectile.position.x),
                    static_cast<int32_t>(projectile.position.y), projectile.radius,
                    Fade(ColorLerp(Palette::SolarForgeAccent, WHITE, 0.3F), 0.5F));
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
        const float offset = std::floor(static_cast<float>(i + 1) / 2.0F) * projectile.radius * 0.55F;
        const Vector2 start =
            Vector2Add(projectile.position, Vector2Scale(perp, offset * side));
        const Vector2 end =
            Vector2Add(start, Vector2Scale(tailDir, projectile.radius * (2.5F + static_cast<float>(i))));
        DrawLineEx(start, end, 2.5F, Fade(Palette::PunctumHaze, 0.5F - static_cast<float>(i) * 0.07F));
    }

    const float angle = std::atan2(-tailDir.y, -tailDir.x);
    drawKrakenGrabbedEnemy(projectile.position, projectile.visualEnemyKind, 1.0F, angle);
}

void drawWreckwormHeadShape(Vector2 center, float radius, float seed, Color color, Vector2 velocity)
{
    const float speed = Vector2Length(velocity);
    const float stretchAmount = std::min(speed / 900.0F, 0.35F);
    const Vector2 dir =
        speed > 1.0F ? Vector2Normalize(velocity) : Vector2{.x = 0, .y = -1};
    const Vector2 perp{.x = -dir.y, .y = dir.x};

    const auto t = static_cast<float>(GetTime());
    const float wobble = std::sin(t * 9.0F + seed) * stretchAmount * 0.6F;
    const float stretchAlong = 1.0F + stretchAmount + wobble;
    const float stretchAcross = 1.0F - stretchAmount * 0.5F - wobble * 0.3F;

    constexpr int32_t vertexCount = 16;
    std::array<Vector2, vertexCount> verts{};
    for (int32_t i = 0; i < vertexCount; i++)
    {
        const float angle =
            static_cast<float>(i) * (360.0F / static_cast<float>(vertexCount)) * DEG2RAD;
        const Vector2 local{.x = std::cos(angle), .y = std::sin(angle)};
        const float jiggle = 0.94F + hashNoise(seed * 3.1F + static_cast<float>(i)) * 0.1F +
                             0.03F * std::sin(t * 6.0F + static_cast<float>(i) * 1.7F + seed);
        const float r = radius * jiggle;
        const float alongDir = local.x * dir.x + local.y * dir.y;
        const float alongPerp = local.x * perp.x + local.y * perp.y;
        const Vector2 offset =
            Vector2Add(Vector2Scale(dir, alongDir * stretchAlong * r),
                      Vector2Scale(perp, alongPerp * stretchAcross * r));
        verts.at(static_cast<size_t>(i)) = Vector2Add(center, offset);
    }

    for (size_t i = 0; i < verts.size(); i++)
    {
        DrawTriangle(center, verts.at((i + 1) % verts.size()), verts.at(i), color);
    }

    DrawCircleV(Vector2Add(center, Vector2Scale(perp, -radius * 0.28F)), radius * 0.3F,
               Fade(WHITE, 0.16F));
    DrawCircleV(Vector2Add(center, Vector2{.x = radius * 0.18F, .y = -radius * 0.22F}),
               radius * 0.14F, Fade(WHITE, 0.3F));
    DrawCircleV(Vector2Add(center, Vector2Scale(dir, radius * 0.25F)), radius * 0.18F,
               Fade(ColorLerp(color, Palette::Void, 0.3F), 0.35F));
}

void drawKrakenShape(Vector2 center, float radius, float seed, Color color)
{
    const auto t = static_cast<float>(GetTime());
    drawPanelShape(center, radius, 8, seed * 12.0F, ColorLerp(color, Palette::Void, 0.15F), seed);

    constexpr int32_t podCount = 4;
    for (int32_t i = 0; i < podCount; i++)
    {
        const float angle = static_cast<float>(i) * 90.0F * DEG2RAD + seed;
        const Vector2 podCenter =
            Vector2Add(center, Vector2{.x = std::cos(angle) * radius * 0.78F,
                                       .y = std::sin(angle) * radius * 0.78F});
        drawPanelShape(podCenter, radius * 0.32F, 6, seed * 30.0F + static_cast<float>(i) * 40.0F,
                      ColorLerp(color, Palette::StructDark, 0.25F), seed + static_cast<float>(i));
    }

    const float pulse = 0.6F + 0.4F * std::sin(t * 2.0F + seed);
    DrawCircleV(center, radius * 0.16F, Fade(Palette::PunctumAccent, 0.5F + 0.4F * pulse));
    DrawCircleLines(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y), radius * 0.16F,
                    Fade(WHITE, 0.6F));
}

void drawBendyTentacle(Vector2 anchor, Vector2 tip, Vector2 bodyCenter, float bodyRadius, float seed,
                       float thicknessScale, float waveScale, bool calm, Color color, Color tipColor)
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
        const Vector2 outDir = midDist > 0.01F ? Vector2Scale(Vector2Subtract(mid, bodyCenter), 1.0F / midDist)
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

void drawKrakenTentacle(const Boss& boss)
{
    const Vector2 portal = boss.tentaclePortalPos;
    const auto t = static_cast<float>(GetTime());
    const float ringPulse = 0.7F + 0.3F * std::sin(t * 6.0F);
    DrawCircleV(portal, krakenTentaclePortalRadius * ringPulse, Fade(Palette::Void, 0.7F));
    DrawCircleLines(static_cast<int32_t>(portal.x), static_cast<int32_t>(portal.y),
                    krakenTentaclePortalRadius * ringPulse, Fade(Palette::PunctumAccent, 0.8F));
    DrawCircleLines(static_cast<int32_t>(portal.x), static_cast<int32_t>(portal.y),
                    krakenTentaclePortalRadius * ringPulse * 0.6F, Fade(Palette::PunctumHaze, 0.6F));

    float length = 0;
    float angle = (boss.tentacleSwipeStartAngle + boss.tentacleSwipeEndAngle) / 2.0F;
    if (boss.tentaclePhase == 1)
    {
        const float progress =
            std::clamp(1.0F - boss.tentacleStateTimer / krakenTentacleEmergeDuration, 0.0F, 1.0F);
        length = krakenTentacleReach * progress;
    }
    else if (boss.tentaclePhase == 2)
    {
        const float progress =
            std::clamp(1.0F - boss.tentacleStateTimer / krakenTentacleSwipeDuration, 0.0F, 1.0F);
        angle = boss.tentacleSwipeStartAngle +
                (boss.tentacleSwipeEndAngle - boss.tentacleSwipeStartAngle) * progress;
        length = krakenTentacleReach;
    }
    else if (boss.tentaclePhase == 3)
    {
        const float progress =
            std::clamp(boss.tentacleStateTimer / krakenTentacleRetreatDuration, 0.0F, 1.0F);
        angle = boss.tentacleSwipeEndAngle;
        length = krakenTentacleReach * progress;
    }
    else if (boss.tentaclePhase == 4)
    {
        const float progress =
            std::clamp(boss.tentacleStateTimer / krakenTentacleStaggerRetreatDuration, 0.0F, 1.0F);
        const float jitter = std::sin(t * 40.0F) * 0.08F;
        angle = boss.tentacleSwipeEndAngle + jitter;
        length = krakenTentacleReach * progress;
    }

    if (length < 1.0F)
    {
        return;
    }

    const Vector2 dir{.x = std::cos(angle), .y = std::sin(angle)};
    const Vector2 tip = Vector2Add(portal, Vector2Scale(dir, length));
    const Vector2 bodyCenter{.x = boss.position.x + boss.size.x / 2, .y = boss.position.y + boss.size.y / 2};
    drawBendyTentacle(portal, tip, bodyCenter, boss.size.x / 2, static_cast<float>(boss.instanceId),
                      krakenLimbSizeMult, 20.0F, false, Palette::PunctumHaze, Palette::Crit);
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

    const Vector2 center{.x = boss.position.x + boss.size.x / 2, .y = boss.position.y + boss.size.y / 2};
    const float bodyRadius = boss.size.x / 2;
    const float seed = static_cast<float>(boss.instanceId);
    const float podAngle = static_cast<float>(pod) * 90.0F * DEG2RAD + seed;
    const Vector2 podPos = Vector2Add(
        center, Vector2{.x = std::cos(podAngle) * boss.size.x / 2 * 0.78F,
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
                1.0F - std::clamp(boss.limbThrowTimer[slot] / krakenLimbLeanBackDuration, 0.0F, 1.0F);
            const Vector2 leanBackTarget =
                Vector2Add(podPos, Vector2Scale(Vector2Subtract(podPos, boss.limbGrabTarget[slot]),
                                               krakenLimbNearReach /
                                                   std::max(1.0F, Vector2Distance(podPos,
                                                                                 boss.limbGrabTarget[slot]))));
            tip = Vector2Lerp(boss.limbPortalPos[slot], leanBackTarget, progress);
        }
        else if (boss.limbThrowPhase[slot] == 2)
        {
            grabbing = true;
            grabbedEnemyScale = 1.0F;
            const float progress =
                1.0F - std::clamp(boss.limbThrowTimer[slot] / krakenLimbThrowDuration, 0.0F, 1.0F);
            const Vector2 leanBackTarget =
                Vector2Add(podPos, Vector2Scale(Vector2Subtract(podPos, boss.limbGrabTarget[slot]),
                                               krakenLimbNearReach /
                                                   std::max(1.0F, Vector2Distance(podPos,
                                                                                 boss.limbGrabTarget[slot]))));
            tip = Vector2Lerp(leanBackTarget, boss.limbGrabTarget[slot], progress);
        }
        else if (boss.limbGrabAnimTimer[slot] > 0)
        {
            grabbing = true;
            tip = boss.limbPortalPos[slot];
            const float elapsed = krakenLimbGrabAnimDuration - boss.limbGrabAnimTimer[slot];
            if (elapsed < krakenLimbGrabAnimDuration / 3.0F)
            {
                tip = Vector2Lerp(podPos, boss.limbPortalPos[slot],
                                  std::clamp(elapsed / (krakenLimbGrabAnimDuration / 3.0F), 0.0F, 1.0F));
            }
            else if (elapsed < krakenLimbGrabAnimDuration * 2.0F / 3.0F)
            {
                grabbedEnemyScale = 0.0F;
            }
            else
            {
                grabbedEnemyScale = std::clamp(
                    (elapsed - krakenLimbGrabAnimDuration * 2.0F / 3.0F) / (krakenLimbGrabAnimDuration / 3.0F),
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

    drawBendyTentacle(podPos, tip, center, bodyRadius, seed, krakenLimbSizeMult, 14.0F, grabbing,
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
        const float seed = static_cast<float>(boss.instanceId) * 5.0F + static_cast<float>(i) * 11.0F;
        const float angle = hashNoise(seed) * 2.0F * std::numbers::pi_v<float>;
        const float wobble = std::sin(t * 1.5F + seed) * 5.0F;
        const Vector2 knot = Vector2Add(
            center, Vector2{.x = std::cos(angle) * (coreRadius * 0.5F + wobble),
                            .y = std::sin(angle) * (coreRadius * 0.5F + wobble)});
        DrawCircleV(knot, coreRadius * (0.25F + hashNoise(seed * 3.0F) * 0.2F),
                   Fade(Palette::Void, 0.85F));
    }
    DrawCircleV(center, coreRadius * 0.6F, Fade(Palette::PunctumHaze, 0.5F));
}

void drawBanishedTentacle(Vector2 center, const Boss& boss, int32_t slot)
{
    const int32_t phase = boss.banishedPhase[static_cast<size_t>(slot)];
    const bool staggered = boss.banishedStaggered[static_cast<size_t>(slot)];
    const auto t = static_cast<float>(GetTime());
    const float seed = static_cast<float>(boss.instanceId) * 7.0F + static_cast<float>(slot) * 13.0F;

    if (staggered)
    {
        const float angle =
            static_cast<float>(slot) / static_cast<float>(Boss::banishedTentacleCount) * 2.0F *
                std::numbers::pi_v<float> +
            seed;
        const float r = boss.size.x * 0.3F;
        const Vector2 wrapPos =
            Vector2Add(center, Vector2{.x = std::cos(angle) * r, .y = std::sin(angle) * r});
        const float hitFlash = boss.banishedTentacleHitFlash[static_cast<size_t>(slot)];
        const Color color = hitFlash > 0
                                ? ColorLerp(Palette::Void, WHITE,
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
        const float angle =
            static_cast<float>(slot) / static_cast<float>(Boss::banishedTentacleCount) * 2.0F *
                std::numbers::pi_v<float> +
            seed + std::sin(t * 0.6F + seed) * 0.15F;
        const float r = boss.size.x * (0.32F + std::sin(t * 1.2F + seed) * 0.04F);
        const Vector2 stub =
            Vector2Add(center, Vector2{.x = std::cos(angle) * r, .y = std::sin(angle) * r});
        DrawLineEx(center, stub, 8.0F, Fade(Palette::PunctumHaze, 0.6F));
        return;
    }

    const Vector2 anchor = boss.banishedAnchor[static_cast<size_t>(slot)];
    const Vector2 tip = boss.banishedTip[static_cast<size_t>(slot)];
    if (Vector2Distance(anchor, tip) < 2.0F)
    {
        return;
    }

    drawBendyTentacle(anchor, tip, center, boss.size.x / 2, seed, 0.7F, 12.0F, false, Palette::PunctumHaze,
                      Palette::Crit);
}

void drawBanishedEye(const Boss& boss)
{
    const Vector2 pos = boss.banishedStage == 1
                            ? boss.banishedEyePos
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
    DrawCircleV(Vector2Add(pos, Vector2Scale(pupilDir, radius * 0.35F)), radius * 0.45F, Palette::Void);

    if (boss.banishedStage == 2)
    {
        DrawLineEx(pos, boss.targetPosition, 14.0F, Fade(Palette::Crit, 0.75F));
        DrawLineEx(pos, boss.targetPosition, 5.0F, Fade(WHITE, 0.9F));
    }
}

void drawFluidBlob(Vector2 center, float radius, Color color, float seed, Vector2 trailDir)
{
    const float dirLen = Vector2Length(trailDir);
    const Vector2 dir = dirLen > 0.01F ? Vector2Scale(trailDir, 1.0F / dirLen) : Vector2{.x = 0, .y = -1};
    const Vector2 perp{.x = -dir.y, .y = dir.x};

    DrawCircleV(center, radius, color);

    constexpr int32_t notchCount = 5;
    for (int32_t i = 0; i < notchCount; i++)
    {
        const float frac = static_cast<float>(i) / static_cast<float>(notchCount - 1) - 0.5F;
        const float notchSeed = seed * 9.0F + static_cast<float>(i) * 7.0F;
        const float notchRadius = radius * (0.22F + hashNoise(notchSeed) * 0.3F);
        const Vector2 edge = Vector2Add(
            center, Vector2Add(Vector2Scale(dir, -radius * 0.75F), Vector2Scale(perp, frac * radius * 1.7F)));
        DrawCircleV(edge, notchRadius, ColorLerp(color, Palette::Void, 0.6F));
    }

    DrawCircleV(Vector2Add(center, Vector2Scale(perp, -radius * 0.3F)), radius * 0.22F, Fade(WHITE, 0.55F));
    DrawCircleV(Vector2Add(center, Vector2{.x = radius * 0.15F, .y = -radius * 0.15F}), radius * 0.12F,
               Fade(WHITE, 0.4F));

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

void drawWormholeMouth(Vector2 position, WormholeFacing facing, float radius)
{
    DrawCircleV(position, radius, Fade(Palette::Shield, 0.25F));
    DrawRing(position, radius - 4, radius, static_cast<float>(GetTime()) * 60,
             static_cast<float>(GetTime()) * 60 + 300, 24, Palette::Shield);
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

void drawHUD(const Game& game)
{
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

    drawText(game,
             "Move: WASD | Dash: L-Click | Shield: R-Click | Burst: Space | Pause: Esc | F11: "
             "Fullscreen",
             10, scaledScreenHeight - 28, 18, Palette::StructMid);

    if (game.run.achievementToastTimer > 0)
    {
        const auto textWidth = static_cast<int32_t>(
            MeasureTextEx(game.resources.font, game.run.achievementToast.c_str(), 24, 1).x);
        drawText(game, game.run.achievementToast.c_str(), (scaledScreenWidth - textWidth) / 2, 100,
                 24, Palette::Charge);
    }

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

    int32_t lineY = y + 22;
    for (size_t i = 0; i < damageSourceNames.size(); i++)
    {
        const int32_t amount = meter.bySource.at(i);
        if (amount <= 0)
        {
            continue;
        }
        const std::string line = std::format("{}: {}", damageSourceNames.at(i), amount);
        drawText(game, line.c_str(), x, lineY, 14, Palette::StructLight);
        lineY += 16;
    }

    return lineY - y;
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
        drawText(game, "!", static_cast<int32_t>(center.x) - 3,
                 static_cast<int32_t>(center.y) - 7, 14, Palette::Void);
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

    if (player.dashTrailTimer > 0)
    {
        entries.push_back(StatusEntry{
            .iconKind = StatusIconKind::Dot,
            .color = Palette::Shield,
            .fracRemaining = player.dashTrailTimer / pickupEffectDuration,
            .label = std::format("Dash Trail {:.0f}s", std::ceil(player.dashTrailTimer))});
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
        entries.push_back(StatusEntry{
            .iconKind = StatusIconKind::Weapon,
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

        if (CheckCollisionPointCircle(mouse, center, iconRadius + 4))
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

        for (int32_t i = 0; i < 8; i++)
        {
            const float phase = static_cast<float>(i) * 0.35F;
            const float fade = 1.0F - std::fmod(t * 0.7F + phase, 1.0F);
            const Vector2 pos{.x = center.x - (1.0F - fade) * 90.0F + 45,
                              .y = center.y + std::sin(t * 3 + phase * 6) * 12};
            const float flicker =
                0.75F + 0.25F * std::sin(t * 24.0F + pos.x * 0.1F + pos.y * 0.1F);
            constexpr float dashTrailRadiusDemo = 10.0F;
            DrawCircleV(pos, dashTrailRadiusDemo * fade,
                       Fade(Palette::Accent, 0.18F * fade * flicker));
            DrawCircleLines(static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y),
                            dashTrailRadiusDemo * fade * 0.65F, Fade(Palette::Crit, 0.5F * fade));
            DrawCircleV(pos, dashTrailRadiusDemo * fade * 0.22F,
                       Fade(WHITE, 0.6F * fade * flicker));
        }
        break;
    case 2:
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
        Boss previewBoss{
            .position = Vector2{.x = center.x - previewBossSize / 2,
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
        drawShipHull(shipClass, center, 20, 0, ships.at(static_cast<size_t>(game.browserShipIndex)).color);
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

    windowText(game, browserCurrentName(game).c_str(),
              static_cast<int32_t>(center.x), static_cast<int32_t>(preview.y + preview.height + 30),
              22, Palette::Crit);

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
    case PickupType::DashTrail:
        DrawPoly(position, 3, 7, static_cast<float>(GetTime()) * 60.0F,
                 Fade(Palette::Accent, alpha));
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
        windowText(game, std::string(stepper.label).c_str(), static_cast<int32_t>(row.x),
                   static_cast<int32_t>(row.y) - 18, 14, Palette::StructLight);

        GuiButton(sandboxStepperMinusRect(game, i), "-");
        std::string value = stepper.valueText(game);
        GuiTextBox(sandboxStepperValueRect(game, i), value.data(),
                  static_cast<int32_t>(value.size()) + 1, false);
        GuiButton(sandboxStepperPlusRect(game, i), "+");
    }

    const auto& buttons = sandboxMenuButtons();
    for (int32_t i = 0; i < static_cast<int32_t>(buttons.size()); i++)
    {
        GuiButton(sandboxMenuButtonRect(game, i), buttons.at(static_cast<size_t>(i)).label(game).c_str());
    }
}

}

auto drawGame(Game& game) -> void
{
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
        if (game.settingsReturnState == GameState::PAUSED ||
            game.settingsReturnState == GameState::GAME_OVER)
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
        if (game.sandbox)
        {
            drawSandboxToggleIndicator(game);
        }
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
    case GameState::ENDING:
        drawEnding(game);
        break;
    }

    EndDrawing();
}
