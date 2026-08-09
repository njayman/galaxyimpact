#include "draw.hpp"

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
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
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
void drawShipSelect(const Game& game);
void drawGameOver(const Game& game);
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
void drawHUD(const Game& game);
void drawActiveBuffs(const Game& game, int32_t x, int32_t y);
void drawDownwardTriangleIcon(float cx, int32_t y, float size, Color fillColor, bool filled);
void drawHealthBar(const Game& game, int32_t x, int32_t y);
auto healthBarWidthWithLabel() -> int32_t;
void drawShieldStackPips(const Game& game, int32_t x, int32_t y);
void drawNerveBar(const Game& game, int32_t x, int32_t y);
auto drawDamageMeter(const Game& game, int32_t x, int32_t y) -> int32_t;
void drawChargePips(const Game& game, int32_t x, int32_t y);
void drawDebuffIndicator(const Game& game, int32_t x, int32_t y);
void drawSandboxToggleIndicator(const Game& game);
void drawSandboxMenu(const Game& game);
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

    drawMenu(game, "", {"Start", "Achievements", "Settings", "Exit"}, MenuLayout::titleMenuY);
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

// Mirrors the actual unlock logic in achievements.cpp - kept as display-only text/progress here
// so the player can see exactly what they're chasing, not just a locked/unlocked flag.
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
        DrawTriangle(Vector2{.x = c.x - s, .y = c.y}, Vector2{.x = c.x + s * 0.6F, .y = c.y - s},
                     Vector2{.x = c.x + s * 0.6F, .y = c.y + s}, color);
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
    case ElementType::Confuse:
        return "randomizes movement (and Turret retargeting)";
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

    for (int32_t i = 0; i < ItemConstants::maxAbilitySlots; i++)
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

void drawGameplayWorld(const Game& game)
{
    drawBackgroundStars(game, game.run.player.position);

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
        DrawCircleV(game.run.blackhole.position, game.run.blackhole.radius, Palette::Void);
        DrawCircleLines(static_cast<int32_t>(game.run.blackhole.position.x),
                        static_cast<int32_t>(game.run.blackhole.position.y),
                        game.run.blackhole.radius, Palette::StructMid);
        DrawRing(game.run.blackhole.position, game.run.blackhole.radius + 6,
                 game.run.blackhole.radius + 9, static_cast<float>(GetTime()) * 40,
                 static_cast<float>(GetTime()) * 40 + 120, 16, Fade(Palette::StructMid, 0.6F));
        DrawRing(game.run.blackhole.position, game.run.blackhole.radius + 14,
                 game.run.blackhole.radius + 17, -static_cast<float>(GetTime()) * 30,
                 -static_cast<float>(GetTime()) * 30 + 90, 16, Fade(Palette::Haze, 0.4F));
    }

    if (game.run.wormhole.active)
    {
        drawWormholeMouth(game.run.wormhole.positionA, game.run.wormhole.facingA,
                          game.run.wormhole.radius);
        drawWormholeMouth(game.run.wormhole.positionB, game.run.wormhole.facingB,
                          game.run.wormhole.radius);
    }

    for (const auto& pocket : game.run.shadowPockets)
    {
        DrawCircleV(pocket.position, pocket.radius, Fade(Palette::ElementFreeze, 0.12F));
        DrawCircleLinesV(pocket.position, pocket.radius, Fade(Palette::ElementFreeze, 0.45F));
    }

    for (const auto& a : game.run.asteroids)
    {
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

        switch (p.type)
        {
        case PickupType::LifeOrb:
            DrawCircleV(p.position, 5, Fade(Palette::Accent, alpha));
            DrawCircleLinesV(p.position, 6, Fade(Palette::Crit, 0.8F * alpha));
            break;
        case PickupType::Shield:
            DrawCircleV(p.position, 6, Fade(Palette::Shield, alpha));
            DrawCircleLinesV(p.position, 7, Fade(Palette::Haze, 0.8F * alpha));
            break;
        case PickupType::Elemental:
        {
            const Color color = elementColors.at(static_cast<size_t>(p.element));
            DrawPoly(p.position, 4, 7, static_cast<float>(GetTime()) * 90.0F, Fade(color, alpha));
            DrawCircleLinesV(p.position, 9, Fade(color, 0.6F * alpha));
            break;
        }
        case PickupType::Regen:
            DrawCircleV(p.position, 6, Fade(Palette::Heal, alpha));
            DrawCircleLinesV(p.position, 7, Fade(WHITE, 0.6F * alpha));
            break;
        case PickupType::DashTrail:
            DrawPoly(p.position, 3, 7, static_cast<float>(GetTime()) * 60.0F,
                     Fade(Palette::Accent, alpha));
            break;
        case PickupType::MagnetPulse:
            DrawCircleLinesV(p.position, 8, Fade(Palette::Shield, alpha));
            DrawCircleLinesV(p.position, 5, Fade(Palette::Shield, 0.6F * alpha));
            break;
        case PickupType::Overcharge:
            DrawPoly(p.position, 6, 7, static_cast<float>(GetTime()) * 120.0F,
                     Fade(Palette::Charge, alpha));
            break;
        case PickupType::SecondWind:
            DrawCircleV(p.position, 6, Fade(Palette::Crit, alpha));
            DrawCircleLinesV(p.position, 8, Fade(Palette::Crit, 0.5F * alpha));
            break;
        case PickupType::Danger:
        {

            const float pulse = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 10.0F);
            DrawPoly(p.position, 5, 9, static_cast<float>(GetTime()) * -140.0F,
                     Fade(Palette::Void, alpha));
            DrawPolyLines(p.position, 5, 10, static_cast<float>(GetTime()) * -140.0F,
                          Fade(Palette::Crit, alpha * pulse));
            break;
        }
        default:
            DrawCircleV(p.position, 2.5F, Fade(Palette::Charge, alpha));
            DrawCircleLinesV(p.position, 3.5F, Fade(Palette::Crit, 0.6F * alpha));
            break;
        }
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
        DrawCircleV(drone.position, 8, Palette::Accent);
        DrawCircleLines(static_cast<int32_t>(drone.position.x),
                        static_cast<int32_t>(drone.position.y), 8.0F, Palette::StructLight);
    }

    for (const auto& drone : game.run.laserDrones)
    {
        DrawCircleV(drone.position, 7, Palette::Crit);
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
        if (p.homing)
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
    const float r = game.run.player.radius;

    const Vector2 dir = aimAtMouse(game);
    const float angle = std::atan2(dir.y, dir.x) + std::numbers::pi_v<float> / 2;

    const auto rotate = [angle](Vector2 v) -> Vector2
    {
        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);
        return Vector2{.x = v.x * cosA - v.y * sinA, .y = v.x * sinA + v.y * cosA};
    };

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

    DrawLineEx(start, end, 10 * flashFrac, Fade(WHITE, flashFrac));
    DrawLineEx(start, end, 20 * flashFrac, Fade(Palette::Charge, flashFrac * 0.6F));
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
            DrawTriangle(center, verts.at(i), verts.at((i + 1) % verts.size()), color);
        }
        break;
    }
    }
}

// Deterministic pseudo-random in [0,1) from a float seed - draw code must never call
// GetRandomValue/SetRandomSeed itself, that would desync the shared gameplay RNG (drop rolls,
// spawn patterns, etc. all draw from the same stream).
auto hashNoise(float seed) -> float
{
    const float v = std::sin(seed) * 43758.5453F;
    return v - std::floor(v);
}

// Per-debuff status VFX drawn on top of an already-rendered enemy/boss body - burn gets small
// flickering flame licks (echoes the flamethrower weapon's own particle look), static gets
// crackling zigzag bolts around the target, freeze gets a semi-transparent icy ring hugging the
// border. Purely procedural (position/time-seeded), no persistent particle state needed.
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

    if (kind.contactAppliesConfuse && confusePulseActive(kind.confuseTelegraphDuration))
    {
        DrawCircleLines(static_cast<int32_t>(enemy.position.x),
                        static_cast<int32_t>(enemy.position.y), kind.radius + 6,
                        Palette::ElementConfuse);
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
    drawEnemyShape(kind.shape, enemy.position, kind.radius, spin, color);
    drawElementalDebuffEffects(enemy.position, kind.radius, enemy.burnDps > 0, enemy.debuffStatic,
                               enemy.debuffFreeze);

    constexpr float enemyHealthMult = 1.2F;
    auto maxHealth = static_cast<int32_t>(static_cast<float>(kind.health) * enemyHealthMult *
                                          waveEnemyScale(game));
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

    drawBossHull(boss.shape, bossCenter, boss.size, ufoColor);
    drawElementalDebuffEffects(bossCenter, std::max(boss.size.x, boss.size.y) / 2, boss.burnDps > 0,
                               boss.debuffStatic, boss.debuffFreeze);

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
            // Drawn as fading particle puffs (see game.run.flameParticles, spawned by
            // updateFlamethrower) rather than a flat cone shape here - rendered alongside the
            // dash trail particles below.
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
    y += 24;
    drawDebuffIndicator(game, 10, y);
    y += 24;
    drawActiveBuffs(game, 10, y);

    constexpr int32_t abilitySlotCount = ItemConstants::maxAbilitySlots;
    constexpr int32_t abilityBoxSize = 28;
    constexpr int32_t abilityGap = 34;
    const int32_t abilitySlotsWidth = (abilitySlotCount - 1) * abilityGap + abilityBoxSize;
    const auto scaledScreenWidth =
        static_cast<int32_t>(static_cast<float>(game.resources.screenWidth) / hudScale(game));
    const auto scaledScreenHeight =
        static_cast<int32_t>(static_cast<float>(game.resources.screenHeight) / hudScale(game));
    drawAbilitySlots(game, scaledScreenWidth - abilitySlotsWidth - 10, 10);

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

void drawDebuffIndicator(const Game& game, int32_t x, int32_t y)
{
    const bool suppressed = std::any_of(
        game.run.eliteHazards.begin(), game.run.eliteHazards.end(), [](const EliteHazard& hazard)
        { return hazard.active && hazard.role == EliteHazardRole::Suppressor; });
    if (!suppressed)
    {
        return;
    }

    DrawCircleV(Vector2{.x = static_cast<float>(x) + 7, .y = static_cast<float>(y) + 9}, 7,
                Palette::Shield);
    drawText(game, "!", x + 4, y + 1, 14, Palette::Void);
    drawText(game, "SUPPRESSED: weapon cooldowns +40%", x + 20, y, 16, Palette::Shield);
}

void drawActiveBuffs(const Game& game, int32_t x, int32_t y)
{
    const auto& player = game.run.player;
    int32_t cursorX = x;

    for (size_t e = 0; e < static_cast<size_t>(ElementType::Count); e++)
    {
        const float timer = player.elementalBuffTimer.at(e);
        if (timer <= 0)
        {
            continue;
        }
        const Color color = elementColors.at(e);
        DrawCircleV(Vector2{.x = static_cast<float>(cursorX) + 7, .y = static_cast<float>(y) + 9},
                    7, color);
        const std::string label = std::format("{} {:.0f}s", elementNames.at(e), std::ceil(timer));
        drawText(game, label.c_str(), cursorX + 18, y, 16, color);
        cursorX += 18 + static_cast<int32_t>(label.size()) * 9 + 14;
    }

    if (player.regenTimer > 0)
    {
        DrawCircleV(Vector2{.x = static_cast<float>(cursorX) + 7, .y = static_cast<float>(y) + 9},
                    7, Palette::Accent);
        const std::string label = std::format("Regen {:.0f}s", std::ceil(player.regenTimer));
        drawText(game, label.c_str(), cursorX + 18, y, 16, Palette::Accent);
        cursorX += 18 + static_cast<int32_t>(label.size()) * 9 + 14;
    }

    if (player.overchargeTimer > 0)
    {
        DrawCircleV(Vector2{.x = static_cast<float>(cursorX) + 7, .y = static_cast<float>(y) + 9},
                    7, Palette::Charge);
        const std::string label =
            std::format("Overcharge {:.0f}s", std::ceil(player.overchargeTimer));
        drawText(game, label.c_str(), cursorX + 18, y, 16, Palette::Charge);
        cursorX += 18 + static_cast<int32_t>(label.size()) * 9 + 14;
    }

    if (player.secondWindReady)
    {
        DrawCircleV(Vector2{.x = static_cast<float>(cursorX) + 7, .y = static_cast<float>(y) + 9},
                    7, Palette::Crit);
        drawText(game, "Second Wind", cursorX + 18, y, 16, Palette::Crit);
        cursorX += 18 + 11 * 9 + 14;
    }

    if (player.dashTrailTimer > 0)
    {
        DrawCircleV(Vector2{.x = static_cast<float>(cursorX) + 7, .y = static_cast<float>(y) + 9},
                    7, Palette::Shield);
        const std::string label =
            std::format("Dash Trail {:.0f}s", std::ceil(player.dashTrailTimer));
        drawText(game, label.c_str(), cursorX + 18, y, 16, Palette::Shield);
        cursorX += 18 + static_cast<int32_t>(label.size()) * 9 + 14;
    }

    if (player.overdriveTimer > 0)
    {
        DrawCircleV(Vector2{.x = static_cast<float>(cursorX) + 7, .y = static_cast<float>(y) + 9},
                    7, Palette::Crit);
        const std::string label =
            std::format("Overdrive {:.0f}s", std::ceil(player.overdriveTimer));
        drawText(game, label.c_str(), cursorX + 18, y, 16, Palette::Crit);
        cursorX += 18 + static_cast<int32_t>(label.size()) * 9 + 14;
    }

    if (game.run.weaponDowngrade.has_value())
    {
        const auto& downgrade = *game.run.weaponDowngrade;
        const Vector2 iconCenter{.x = static_cast<float>(cursorX) + 9,
                                 .y = static_cast<float>(y) + 9};
        drawWeaponIcon(downgrade.type, iconCenter, 7, Palette::Accent);
        DrawTriangle(Vector2{.x = iconCenter.x - 3, .y = iconCenter.y + 8},
                    Vector2{.x = iconCenter.x + 3, .y = iconCenter.y + 8},
                    Vector2{.x = iconCenter.x, .y = iconCenter.y + 13}, Palette::Accent);
        const std::string label =
            std::format("{} stolen {:.0f}s", weaponDisplayName(downgrade.type),
                        std::ceil(downgrade.timer));
        drawText(game, label.c_str(), cursorX + 24, y, 16, Palette::Accent);
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

    GuiButton(sandboxMenuCloseButtonRect(game), "Close [Esc]");
}

}

auto drawGame(Game& game) -> void
{
    BeginTextureMode(game.resources.worldTarget);
    ClearBackground(biomeVoidColor(game));

    switch (game.state)
    {
    case GameState::TITLE:
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
        if (game.settingsReturnState == GameState::PAUSED)
        {
            drawHUD(game);
        }
        break;
    case GameState::TITLE:
    case GameState::GAME_OVER:
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
        drawMenu(game, "PAUSED", {"Resume", "Achievements", "Settings", "New Game", "Exit"},
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
    }

    EndDrawing();
}
