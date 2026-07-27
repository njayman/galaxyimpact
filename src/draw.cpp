#include "draw.hpp"

#include "entities/boss.hpp"
#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "entities/player.hpp"
#include "entities/space.hpp"
#include "guitheme.hpp"
#include "menu.hpp"
#include "palette.hpp"
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
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

void drawText(const Game& game, const char* text, int32_t x, int32_t y, int32_t size, Color color)
{
    DrawTextEx(game.font, text, Vector2{.x = static_cast<float>(x), .y = static_cast<float>(y)},
               static_cast<float>(size), 1, color);
}

auto measureText(const Game& game, const char* text, int32_t size) -> int32_t
{
    return static_cast<int32_t>(MeasureTextEx(game.font, text, static_cast<float>(size), 1).x);
}

auto letterBoxRect(const Game& game) -> Rectangle
{
    auto scale = static_cast<float>(game.windowWidth) / static_cast<float>(game.screenWidth);
    auto heightScale =
        static_cast<float>(game.windowHeight) / static_cast<float>(game.screenHeight);

    scale = std::min(scale, heightScale);

    auto width = static_cast<float>(game.screenWidth) * scale;
    auto height = static_cast<float>(game.screenHeight) * scale;

    return Rectangle{.x = (static_cast<float>(game.windowWidth) - width) / 2,
                     .y = (static_cast<float>(game.windowHeight) - height) / 2,
                     .width = width,
                     .height = height};
}

namespace
{

void drawTitle(const Game& game);
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
void drawShip(const Game& game);
void drawEnemy(const Game& game, const Enemy& enemy, bool buffed);
void drawEliteHazard(const EliteHazard& hazard);
void drawBoss(const Game& game, const Boss& boss);
void drawOrbitBlades(const Game& game);
void drawShieldIndicator(const Player& player);
void drawChargeParticles(Vector2 center, float fraction, Color color, float maxRadius,
                         float minRadius);
void drawComet(const BossProjectile& projectile);
void drawHUD(const Game& game);
void drawDownwardTriangleIcon(float cx, int32_t y, float size, Color fillColor, bool filled);
void drawHealthPips(const Game& game, int32_t x, int32_t y);
auto healthPipsWidth(const Game& game) -> int32_t;
void drawShieldStackPips(const Game& game, int32_t x, int32_t y);
void drawNerveBar(const Game& game, int32_t x, int32_t y);
auto drawDamageMeter(const Game& game, int32_t x, int32_t y) -> int32_t;
void drawChargePips(const Game& game, int32_t x, int32_t y);
void drawDebuffIndicator(const Game& game, int32_t x, int32_t y);
void drawSandboxHUD(const Game& game);
void drawWormholeMouth(Vector2 position, WormholeFacing facing, float radius);

void beginPixelZoom()
{
    BeginMode2D(Camera2D{.offset = Vector2{},
                         .target = Vector2{},
                         .rotation = 0,
                         .zoom = 1.0F / static_cast<float>(GameConstants::pixelScale)});
}

void beginWorldCamera(const Game& game)
{
    const float zoom = 1.0F / static_cast<float>(GameConstants::pixelScale);
    const Vector2 center{.x = static_cast<float>(game.screenWidth) / 2,
                         .y = static_cast<float>(game.screenHeight) / 2};
    Vector2 offset = Vector2Scale(center, zoom);

    if (game.shakeTimer > 0 && game.shakeDuration > 0)
    {
        const float amplitude = game.shakeIntensity * (game.shakeTimer / game.shakeDuration);
        offset.x += static_cast<float>(GetRandomValue(-100, 100)) / 100 * amplitude * zoom;
        offset.y += static_cast<float>(GetRandomValue(-100, 100)) / 100 * amplitude * zoom;
    }

    BeginMode2D(
        Camera2D{.offset = offset, .target = game.player.position, .rotation = 0, .zoom = zoom});
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

void drawBackgroundStars(const Game& game, Vector2 relativeTo)
{
    const auto tileW = static_cast<float>(game.screenWidth);
    const auto tileH = static_cast<float>(game.screenHeight);

    for (const auto& p : game.bgParticles)
    {
        DrawCircleV(tiledWorldPos(relativeTo, p.position, tileW, tileH), p.radius, p.color);
    }

    for (const auto& s : game.stars)
    {
        DrawCircleV(tiledWorldPos(relativeTo, s.position, tileW, tileH), s.radius, Palette::Haze);
    }
}

void windowText(const Game& game, const char* text, int32_t centerX, int32_t y, int32_t size,
                Color color)
{
    const float scale = guiUiScale(game);
    const auto scaledSize = static_cast<int32_t>(static_cast<float>(size) * scale);
    const int32_t width = measureText(game, text, scaledSize);
    drawText(game, text, centerX - width / 2, static_cast<int32_t>(static_cast<float>(y) * scale),
             scaledSize, color);
}

void drawTitle(const Game& game)
{
    applyGuiScale(game);
    windowText(game, "GALAXY IMPACT", game.windowWidth / 2, 80, 50, Palette::Accent);

    drawMenu(game, "", {"Start", "Settings", "Exit"}, MenuLayout::titleMenuY);
    drawHighScores(game, game.windowWidth / 2 - static_cast<int32_t>(80 * guiUiScale(game)),
                   static_cast<int32_t>(280 * guiUiScale(game)), game.highScores);
}

void drawGameOver(const Game& game)
{
    applyGuiScale(game);
    windowText(game, "YOU WERE DEFEATED!", game.windowWidth / 2, 70, 40, Palette::Accent);

    const std::string statsText =
        std::format("Score: {}   Level: {}   Wave: {}   Time: {:.0f}s", game.score, game.level,
                    game.waveNumber, game.runTime);
    windowText(game, statsText.c_str(), game.windowWidth / 2, 120, 20, Palette::StructLight);

    drawHighScores(game, game.windowWidth / 2 - static_cast<int32_t>(80 * guiUiScale(game)),
                   static_cast<int32_t>(160 * guiUiScale(game)), game.highScores);
    drawMenu(game, "", {"New Game", "Exit"}, MenuLayout::gameOverMenuY);
}

void drawOverlay(const Game& game)
{
    DrawRectangle(0, 0, game.windowWidth, game.windowHeight, Fade(Palette::Void, 0.75F));
}

void drawSettings(const Game& game)
{
    applyGuiScale(game);
    const float scale = guiUiScale(game);
    windowText(game, "SETTINGS", game.windowWidth / 2, 130, 34, Palette::Accent);

    const auto& res = resolutionOptions.at(static_cast<size_t>(game.settings.resolutionIndex));
    const std::string displayMode = IsWindowFullscreen() ? "Fullscreen" : "Windowed";

    const bool difficultyLocked = game.settingsReturnState == GameState::PAUSED;
    const std::string difficultyValue =
        std::string(difficultyDefs.at(static_cast<size_t>(game.settings.difficulty)).name) +
        (difficultyLocked ? " (locked)" : "");

    const std::array<std::string, 6> stepperLabels{"Resolution", "Difficulty", "",
                                                   "",           "FPS Cap",    "Display Mode"};
    const std::array<std::string, 6> stepperValues{
        std::format("{}x{}", res.width, res.height),
        difficultyValue,
        "",
        "",
        std::format("{}", fpsOptions.at(static_cast<size_t>(game.settings.fpsIndex))),
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

        if (i == 2 || i == 3)
        {
            const bool on = i == 2 ? game.settings.bgmOn : game.settings.soundOn;
            bool checked = on;
            const char* label = i == 2 ? "BGM" : "Sound";
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
    windowText(game, "Up/Down: select   Left/Right: change   Enter: confirm", game.windowWidth / 2,
               hintY, 16, Palette::StructMid);
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

auto evolutionHint(const Game& game, SkillType id) -> std::string
{
    for (size_t i = 0; i < weaponGrantSkill.size(); i++)
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

    struct Slot
    {
        SkillType id;
        Color color;
    };
    std::vector<Slot> slots;

    for (const auto& w : game.weapons)
    {
        const Color color = w.evolved ? Palette::Crit : Palette::Shield;
        slots.push_back(
            Slot{.id = weaponGrantSkill.at(static_cast<size_t>(w.type)), .color = color});
    }

    for (size_t i = 0; i < static_cast<size_t>(SkillType::Count); i++)
    {
        const auto id = static_cast<SkillType>(i);
        if (game.skillLevels.at(i) == 0 || isFusedPassive(game, id))
        {
            continue;
        }
        if (weaponForGrantSkill(id).has_value())
        {
            continue;
        }
        slots.push_back(Slot{.id = id, .color = Palette::Shield});
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
            const Vector2 center{.x = bx + boxSize / 2, .y = by + boxSize / 2};
            drawSkillIcon(slots.at(static_cast<size_t>(i)).id, center, boxSize * iconScale,
                          slots.at(static_cast<size_t>(i)).color);

            if (CheckCollisionPointRec(
                    mouse, Rectangle{.x = bx, .y = by, .width = boxSize, .height = boxSize}))
            {
                hovered = slots.at(static_cast<size_t>(i)).id;
                hoveredBoxPos = Vector2{.x = bx, .y = by};
            }
        }
    }

    if (hovered.has_value())
    {
        const auto& def = Skills.at(static_cast<size_t>(*hovered));
        const int lvl = game.skillLevels.at(static_cast<size_t>(*hovered));
        const std::string name = std::format("{} (Lv {})", def.name, lvl);
        const std::string desc(def.description);

        constexpr int32_t nameFontSize = 16;
        constexpr int32_t descFontSize = 13;
        const int32_t boxWidth = std::max(measureText(game, name.c_str(), nameFontSize),
                                          measureText(game, desc.c_str(), descFontSize)) +
                                 16;
        int32_t tipX = static_cast<int32_t>(hoveredBoxPos.x);
        if (tipX + boxWidth > game.screenWidth)
        {
            tipX = game.screenWidth - boxWidth;
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
    windowText(game, std::format("LEVEL {}", game.level).c_str(), game.windowWidth / 2, 100, 34,
               Palette::Accent);

    const auto count = static_cast<int32_t>(game.pendingChoices.size());
    constexpr int32_t nameFontSize = 24;
    constexpr int32_t descFontSize = 15;
    constexpr int32_t iconOffsetX = 24;
    constexpr float iconRadius = 11;

    for (int32_t i = 0; i < count; i++)
    {
        const auto& choice = game.pendingChoices.at(static_cast<size_t>(i));
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
        case ChoiceType::LifeOrb:
            name = std::format("{} x Life Orb", choice.count);
            desc = "All slots full and maxed - a health reward, plus a small permanent damage "
                   "bump.";
            break;
        case ChoiceType::Shield:
            name = std::format("{} x Shield", choice.count);
            desc = "All slots full and maxed - a shield reward, plus a small permanent damage "
                   "bump.";
            break;
        case ChoiceType::Skill:
        default:
        {
            const auto& def = Skills.at(static_cast<size_t>(choice.skill));
            const int lvl = game.skillLevels.at(static_cast<size_t>(choice.skill));
            name = std::format("{} (Lv {})", def.name, lvl + 1);
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
            color = choice.type == ChoiceType::Evolve ? Palette::Crit : Palette::Accent;
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
        windowText(game, heading.c_str(), game.windowWidth / 2, y - 50, 30, Palette::StructLight);
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
    drawBackgroundStars(game, game.player.position);

    for (const auto& c : game.gasClouds)
    {
        DrawCircleV(c.position, c.radius, c.color);
    }

    if (game.blackhole.active)
    {
        DrawCircleV(game.blackhole.position, game.blackhole.radius, Palette::Void);
        DrawCircleLines(static_cast<int32_t>(game.blackhole.position.x),
                        static_cast<int32_t>(game.blackhole.position.y), game.blackhole.radius,
                        Palette::StructMid);
        DrawRing(game.blackhole.position, game.blackhole.radius + 6, game.blackhole.radius + 9,
                 static_cast<float>(GetTime()) * 40, static_cast<float>(GetTime()) * 40 + 120, 16,
                 Fade(Palette::StructMid, 0.6F));
        DrawRing(game.blackhole.position, game.blackhole.radius + 14, game.blackhole.radius + 17,
                 -static_cast<float>(GetTime()) * 30, -static_cast<float>(GetTime()) * 30 + 90, 16,
                 Fade(Palette::Haze, 0.4F));
    }

    if (game.wormhole.active)
    {
        drawWormholeMouth(game.wormhole.positionA, game.wormhole.facingA, game.wormhole.radius);
        drawWormholeMouth(game.wormhole.positionB, game.wormhole.facingB, game.wormhole.radius);
    }

    for (const auto& a : game.asteroids)
    {
        DrawCircleV(a.position, a.radius, Palette::StructMid);
        DrawCircleV(Vector2{.x = a.position.x - a.radius / 3, .y = a.position.y - a.radius / 4},
                    a.radius / 4, Palette::StructDark);
        DrawCircleV(Vector2{.x = a.position.x + a.radius / 4, .y = a.position.y + a.radius / 3},
                    a.radius / 5, Palette::StructDark);
    }

    const bool warlordActive = std::any_of(
        game.eliteHazards.begin(), game.eliteHazards.end(), [](const EliteHazard& hazard)
        { return hazard.active && hazard.role == EliteHazardRole::Warlord; });
    for (const auto& e : game.enemies)
    {
        drawEnemy(game, e, warlordActive);
    }

    for (const auto& hazard : game.eliteHazards)
    {
        drawEliteHazard(hazard);
    }

    for (const auto& p : game.pickups)
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
        default:
            DrawCircleV(p.position, 4, Fade(Palette::Charge, alpha));
            DrawCircleLinesV(p.position, 5, Fade(Palette::Crit, 0.6F * alpha));
            break;
        }
    }

    for (const auto& boss : game.bosses)
    {
        drawBoss(game, boss);
    }

    for (const auto& m : game.mines)
    {
        if (!m.active)
        {
            continue;
        }
        DrawCircleV(m.position, 6, Palette::Crit);
        DrawCircleLines(static_cast<int32_t>(m.position.x), static_cast<int32_t>(m.position.y),
                        m.radius, Fade(Palette::Crit, 0.2F));
    }

    for (const auto& wave : game.bossDeathShockwaves)
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

    for (const auto& p : game.bossProjectiles)
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

    if (game.player.blackHoleCoreTimer > 0)
    {
        DrawCircleLines(static_cast<int32_t>(game.player.position.x),
                        static_cast<int32_t>(game.player.position.y), game.player.radius + 6,
                        Fade(Palette::Accent, game.player.blackHoleCoreTimer));
    }

    drawOrbitBlades(game);

    const bool shipVisible = game.player.health > 0 && (game.player.immunityTimer <= 0 ||
                                                        static_cast<int>(GetTime() * 10) % 2 == 0);
    if (shipVisible)
    {
        drawShip(game);
    }

    if (game.player.shieldActive)
    {
        const float shieldRadius = game.player.radius + 5;
        DrawCircleV(game.player.position, shieldRadius, Fade(Palette::Shield, 0.5F));
        DrawCircleLines(static_cast<int32_t>(game.player.position.x),
                        static_cast<int32_t>(game.player.position.y), shieldRadius,
                        Palette::Shield);
    }
    else if (game.player.shieldCooldownTimer > 0)
    {
        drawShieldIndicator(game.player);
    }

    for (const auto& b : game.bullets)
    {
        DrawCircleV(b.position, b.radius, b.color);
    }

    for (const auto& p : game.deathParticles)
    {
        DrawCircleV(p.position, p.radius, Fade(p.color, p.life / p.maxLife));
    }
}

void drawShip(const Game& game)
{
    const Vector2 p = game.player.position;
    const float r = game.player.radius;

    const Vector2 dir = aimAtMouse(game);
    const float angle = std::atan2(dir.y, dir.x) + std::numbers::pi_v<float> / 2;

    const auto rotate = [angle](Vector2 v) -> Vector2
    {
        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);
        return Vector2{.x = v.x * cosA - v.y * sinA, .y = v.x * sinA + v.y * cosA};
    };

    if (game.player.dashing)
    {
        const Vector2 dashDir = Vector2Normalize(game.player.dashVelocity);
        for (int i = 1; i <= 3; i++)
        {
            const Vector2 trailPos =
                Vector2Subtract(p, Vector2Scale(dashDir, static_cast<float>(i) * 10));
            DrawCircleV(trailPos, r * (1 - static_cast<float>(i) * 0.2F),
                        Fade(Palette::Crit, 0.35F / static_cast<float>(i)));
        }
    }

    const Color shipColor = game.player.dashing ? Palette::Crit : game.player.color;

    const Vector2 tip = Vector2Add(p, rotate(Vector2{.x = 0, .y = -r}));
    const Vector2 left = Vector2Add(p, rotate(Vector2{.x = -r * 0.8F, .y = r}));
    const Vector2 right = Vector2Add(p, rotate(Vector2{.x = r * 0.8F, .y = r}));

    DrawTriangle(tip, left, right, shipColor);
    DrawCircleV(p, r * 0.3F, Palette::StructLight);
}

void drawEnemy(const Game& game, const Enemy& enemy, bool buffed)
{
    const auto& kind = enemyKinds.at(static_cast<size_t>(enemy.kind));
    Color color = kind.color;
    if (enemy.phased)
    {
        color = Fade(color, 0.35F);
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

    DrawCircleV(enemy.position, kind.radius, color);

    auto maxHealth = static_cast<int32_t>(
        static_cast<float>(kind.health) *
        difficultyDefs.at(static_cast<size_t>(game.settings.difficulty)).enemyHealthMult *
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

void drawBoss(const Game& game, const Boss& boss)
{
    const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                             .y = boss.position.y + boss.size.y / 2};
    const Color ufoColor = boss.health <= 0 ? Palette::StructDark : boss.color;

    DrawEllipse(static_cast<int32_t>(bossCenter.x),
                static_cast<int32_t>(bossCenter.y) + static_cast<int32_t>(boss.size.y / 4),
                boss.size.x / 2, boss.size.y / 4, ufoColor);
    DrawCircle(static_cast<int32_t>(bossCenter.x),
               static_cast<int32_t>(bossCenter.y) - static_cast<int32_t>(boss.size.y / 8),
               boss.size.x / 3.5F, Fade(ufoColor, 0.8F));

    if (boss.state == BossState::WINDING_UP)
    {
        const float progress =
            std::clamp(1 - boss.stateTimer / bossWindupDuration(boss.attack), 0.0F, 1.0F);
        drawChargeParticles(bossCenter, progress, boss.color, 100, 65);

        if (boss.attack == BossAttack::Beam || boss.attack == BossAttack::WormholeBeam)
        {
            const float blink = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 10.0F);
            const float markerRadius = game.player.radius + 10 + 4 * progress;
            DrawCircleLines(static_cast<int32_t>(game.player.position.x),
                            static_cast<int32_t>(game.player.position.y), markerRadius,
                            Fade(Palette::Crit, blink));
            DrawCircleLines(static_cast<int32_t>(game.player.position.x),
                            static_cast<int32_t>(game.player.position.y), markerRadius * 0.7F,
                            Fade(Palette::Accent, blink * 0.7F));
        }
    }

    if (boss.health > 0)
    {
        const float healthPercentage =
            static_cast<float>(boss.health) / static_cast<float>(boss.maxHealth);
        const float healthBarWidth = boss.size.x * healthPercentage;
        DrawRectangle(static_cast<int32_t>(boss.position.x),
                      static_cast<int32_t>(boss.position.y) - 20, static_cast<int32_t>(boss.size.x),
                      15, Fade(Palette::Haze, 0.25F));
        DrawRectangle(static_cast<int32_t>(boss.position.x),
                      static_cast<int32_t>(boss.position.y) - 20,
                      static_cast<int32_t>(healthBarWidth), 15, Palette::Haze);
        DrawRectangleLines(static_cast<int32_t>(boss.position.x),
                           static_cast<int32_t>(boss.position.y) - 20,
                           static_cast<int32_t>(boss.size.x), 15, Palette::StructDark);
    }

    if (boss.state == BossState::SHOOTING && boss.attack == BossAttack::Beam)
    {
        const Vector2 beamStart = bossCenter;
        const Vector2 direction =
            Vector2Normalize(Vector2Subtract(boss.targetPosition, bossCenter));
        const Vector2 beamEnd = Vector2Add(beamStart, Vector2Scale(direction, 2000));

        float beamLength = 2000;
        for (const auto& a : game.asteroids)
        {
            if (CheckCollisionCircleLine(a.position, a.radius, beamStart, beamEnd))
            {
                if (const float dist = Vector2Distance(bossCenter, a.position) - a.radius;
                    dist < beamLength)
                {
                    beamLength = dist;
                }
            }
        }

        const Rectangle beamRec{
            .x = bossCenter.x, .y = bossCenter.y, .width = beamLength, .height = 20};
        const Vector2 beamOrigin{.x = 0, .y = beamRec.height / 2};

        DrawRectanglePro(beamRec, beamOrigin, boss.beamRotation, Fade(Palette::Accent, 0.7F));
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
        DrawLineEx(game.player.position, bossCenter, 2, Fade(Palette::StructLight, 0.4F));

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
    for (const auto& w : game.weapons)
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
            const int32_t count = 2 + w.level / 2;

            for (int32_t s = 0; s < count; s++)
            {
                const float angle = static_cast<float>(GetTime()) * 2 +
                                    static_cast<float>(s) * 2 * std::numbers::pi_v<float> /
                                        static_cast<float>(count);
                const Vector2 pos =
                    Vector2Add(game.player.position, Vector2{.x = std::cos(angle) * radius,
                                                             .y = std::sin(angle) * radius});
                DrawCircleV(pos, 5, color);
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
                DrawCircleLines(static_cast<int32_t>(game.player.position.x),
                                static_cast<int32_t>(game.player.position.y), pulseRadius,
                                Fade(color, 0.7F * (1 - progress)));
            }
            DrawCircleLines(static_cast<int32_t>(game.player.position.x),
                            static_cast<int32_t>(game.player.position.y), maxRadius,
                            Fade(color, 0.15F));
            break;
        }
        case WeaponType::Beam:
        {
            float length = 300 + static_cast<float>(w.level) * 15;
            if (w.evolved)
            {
                length *= 1.3F;
            }
            const Vector2 dir = aimAtMouse(game);
            const Vector2 end = Vector2Add(game.player.position, Vector2Scale(dir, length));
            DrawLineEx(game.player.position, end, 2, Fade(color, 0.3F));
            break;
        }
        default:
            break;
        }
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

void drawHUD(const Game& game)
{
    drawHealthPips(game, 10, 10);
    drawShieldStackPips(game, 10 + healthPipsWidth(game) + 20, 10);

    const std::string statsText =
        std::format("Score: {}   Wave: {}   Lv: {}", game.score, game.waveNumber, game.level);
    drawText(game, statsText.c_str(), 10, 38, 20, Palette::StructLight);

    const float xpFrac =
        std::min(static_cast<float>(game.xp) / static_cast<float>(game.xpToNext), 1.0F);
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

    constexpr int32_t abilitySlotCount = ItemConstants::maxAbilitySlots;
    constexpr int32_t abilityBoxSize = 28;
    constexpr int32_t abilityGap = 34;
    const int32_t abilitySlotsWidth = (abilitySlotCount - 1) * abilityGap + abilityBoxSize;
    drawAbilitySlots(game, game.screenWidth - abilitySlotsWidth - 10, 10);

    drawText(game, "Move: WASD | Dash: L-Click | Shield: R-Click | Pause: Esc | F11: Fullscreen",
             10, game.screenHeight - 28, 18, Palette::StructMid);

    if (game.sandbox)
    {
        drawSandboxHUD(game);
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

void drawHealthPips(const Game& game, int32_t x, int32_t y)
{
    constexpr float size = 11;
    constexpr float gap = 24;

    for (int32_t i = 0; i < game.player.maxHealth; i++)
    {
        const float cx = static_cast<float>(x) + size + static_cast<float>(i) * gap;

        if (i < game.player.health)
        {
            drawDownwardTriangleIcon(cx, y, size, Palette::Accent, true);
        }
        else if (i == game.player.health && game.player.halfLifeOrb)
        {
            drawDownwardTriangleIcon(cx, y, size, Fade(Palette::Accent, 0.45F), true);
        }
        else
        {
            drawDownwardTriangleIcon(cx, y, size, Color{}, false);
        }
    }
}

auto healthPipsWidth(const Game& game) -> int32_t
{
    constexpr float size = 11;
    constexpr float gap = 24;
    return static_cast<int32_t>(size + static_cast<float>(game.player.maxHealth - 1) * gap +
                                size * 0.8F);
}

void drawShieldStackPips(const Game& game, int32_t x, int32_t y)
{
    constexpr float size = 11;
    constexpr float gap = 24;

    for (int32_t i = 0; i < playerConstants::maxShieldStack; i++)
    {
        const float cx = static_cast<float>(x) + size + static_cast<float>(i) * gap;
        const Color fillColor = Fade(Palette::Shield, 0.85F);

        drawDownwardTriangleIcon(cx, y, size, fillColor, i < game.player.shieldStacks);
    }
}

void drawNerveBar(const Game& game, int32_t x, int32_t y)
{
    constexpr int32_t width = 120;
    constexpr int32_t height = 8;
    const float frac = nerveFrac(game);

    DrawRectangle(x, y, width, height, Fade(Palette::StructMid, 0.5F));
    DrawRectangle(x, y, static_cast<int32_t>(static_cast<float>(width) * frac), height,
                  ColorLerp(Palette::Charge, Palette::Crit, frac));
    DrawRectangleLines(x, y, width, height, Palette::StructDark);
    drawText(game, "Nerve", x + width + 8, y - 6, 16, Palette::StructMid);
}

auto drawDamageMeter(const Game& game, int32_t x, int32_t y) -> int32_t
{
    const auto& meter = game.damageMeterDisplay;
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

        if (i < game.player.charges)
        {
            DrawCircleV(center, pipRadius, Palette::Shield);
            DrawCircleLines(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y),
                            pipRadius, Palette::StructDark);
            continue;
        }

        DrawCircleLines(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y), pipRadius,
                        Fade(Palette::StructMid, 0.6F));
        if (i == game.player.charges)
        {
            float progress = 1;
            if (const float d = chargeRegenDuration(game); d > 0)
            {
                progress = 1 - game.player.chargeRegenTimer / d;
            }
            DrawRing(center, pipRadius - 3, pipRadius, -90, -90 + 360 * progress, 16,
                     Palette::Shield);
        }
    }
}

void drawDebuffIndicator(const Game& game, int32_t x, int32_t y)
{
    const bool suppressed = std::any_of(
        game.eliteHazards.begin(), game.eliteHazards.end(), [](const EliteHazard& hazard)
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

void drawSandboxHUD(const Game& game)
{
    const std::string kindName(enemyKinds.at(static_cast<size_t>(game.sandboxKindIndex)).name);
    const std::string attackName(
        bossAttackNames.at(static_cast<size_t>(game.sandboxBossAttackIndex)));
    const std::array<std::string, 6> lines{
        "SANDBOX",
        std::format("[ / ]: cycle enemy ({})   E: spawn enemy   B: spawn boss", kindName),
        "K: clear board   L: level-up picker   H: full heal   R: reset abilities",
        std::format("G: death {}   , / . : cycle boss attack ({})   O: command nearest boss",
                    game.sandboxDeathEnabled ? "ON" : "OFF", attackName),
        std::format("- / + : wave ({})", game.waveNumber),
        "N: spawn black hole   M: spawn wormhole   U: spawn hazard (+Shift: debuff)",
    };

    int32_t y = game.screenHeight - 180;
    for (const auto& line : lines)
    {
        drawText(game, line.c_str(), 10, y, 18, Palette::Crit);
        y += 22;
    }
}

}

auto drawGame(Game& game) -> void
{
    BeginTextureMode(game.worldTarget);
    ClearBackground(Palette::Void);

    switch (game.state)
    {
    case GameState::TITLE:
        beginPixelZoom();
        drawBackgroundStars(game, Vector2{});
        EndMode2D();
        break;
    case GameState::GAMEPLAY:
    case GameState::PAUSED:
    case GameState::LEVEL_UP:
    case GameState::GAME_OVER:
        beginWorldCamera(game);
        drawGameplayWorld(game);
        EndMode2D();
        break;
    case GameState::SETTINGS:
        if (game.settingsReturnState == GameState::PAUSED)
        {
            beginWorldCamera(game);
            drawGameplayWorld(game);
        }
        else
        {
            beginPixelZoom();
            drawBackgroundStars(game, Vector2{});
        }
        EndMode2D();
        break;
    }

    EndTextureMode();

    BeginTextureMode(game.pixelTarget);
    ClearBackground(Palette::Void);

    const auto worldSrc = Rectangle{.x = 0,
                                    .y = 0,
                                    .width = static_cast<float>(game.worldTarget.texture.width),
                                    .height = -static_cast<float>(game.worldTarget.texture.height)};
    const auto worldDst = Rectangle{.x = 0,
                                    .y = 0,
                                    .width = static_cast<float>(game.screenWidth),
                                    .height = static_cast<float>(game.screenHeight)};
    DrawTexturePro(game.worldTarget.texture, worldSrc, worldDst, Vector2{}, 0, WHITE);

    EndTextureMode();

    BeginDrawing();
    ClearBackground(Palette::Void);

    for (size_t i = 0; i < game.borderStars.size(); ++i)
    {
        auto gameBorderStar = game.borderStars[i];
        if (gameBorderStar.position.x <= static_cast<float>(game.windowWidth) &&
            gameBorderStar.position.y <= static_cast<float>(game.windowHeight))
        {
            DrawCircleV(gameBorderStar.position, gameBorderStar.radius, Palette::Haze);
        }
    }

    const auto srcRec = Rectangle{.x = 0,
                                  .y = 0,
                                  .width = static_cast<float>(game.pixelTarget.texture.width),
                                  .height = -static_cast<float>(game.pixelTarget.texture.height)};
    const Rectangle letterbox = letterBoxRect(game);
    DrawTexturePro(game.pixelTarget.texture, srcRec, letterbox, Vector2{}, 0, WHITE);

    const float uiScale = letterbox.width / static_cast<float>(game.screenWidth);
    BeginMode2D(Camera2D{.offset = Vector2{.x = letterbox.x, .y = letterbox.y},
                         .target = Vector2{},
                         .rotation = 0,
                         .zoom = uiScale});

    switch (game.state)
    {
    case GameState::GAMEPLAY:
    case GameState::PAUSED:
    case GameState::LEVEL_UP:
        drawHUD(game);
        break;
    case GameState::SETTINGS:
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
    case GameState::GAMEPLAY:
        break;
    case GameState::PAUSED:
        drawOverlay(game);
        drawMenu(game, "PAUSED", {"Resume", "Settings", "New Game", "Exit"},
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
    }

    EndDrawing();
}
