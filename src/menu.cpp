#include "menu.hpp"

#include "draw.hpp"
#include "raymath.h"
#include <algorithm>
#include <cfloat>
#include <cmath>

void applyScrollInput(ContentScroll& scroll)
{
    constexpr float wheelSpeed = 60.0F;
    scroll.offset -= GetMouseWheelMove() * wheelSpeed;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        scroll.offset -= GetMouseDelta().y;
    }
    scroll.offset = std::clamp(scroll.offset, 0.0F, scroll.maxOffset);
}

auto backButtonRect(const Game& game) -> Rectangle
{
    const float scale = guiUiScale(game);
    constexpr float width = 200.0F;
    constexpr float height = 48.0F;
    constexpr float marginBottom = 40.0F;
    return Rectangle{
        .x = static_cast<float>(game.resources.windowWidth) / 2 - width * scale / 2,
        .y = static_cast<float>(game.resources.windowHeight) - (marginBottom + height) * scale,
        .width = width * scale,
        .height = height * scale,
    };
}

auto titleLayout() -> TitleLayout
{
    constexpr int32_t menuCount = 5; // Start, Achievements, Bestiary, Settings, Exit
    constexpr int32_t menuHeight =
        MenuLayout::buttonHeight * menuCount + MenuLayout::buttonGap * (menuCount - 1);
    constexpr int32_t highScoreHeight = 170; // "Top Scores" heading + up to 5 rows
    constexpr int32_t titleHeight = 110; // matches the 100px title font
    constexpr int32_t gapTitleToScores = 60;
    constexpr int32_t gapScoresToMenu = 50;
    constexpr int32_t totalHeight =
        titleHeight + gapTitleToScores + highScoreHeight + gapScoresToMenu + menuHeight;
    constexpr int32_t referenceHeight = 1080;
    constexpr int32_t blockTop = (referenceHeight - totalHeight) / 2;

    constexpr int32_t titleY = blockTop + 30;
    constexpr int32_t highScoreY = titleY + titleHeight + gapTitleToScores;
    constexpr int32_t menuY = highScoreY + highScoreHeight + gapScoresToMenu;

    return TitleLayout{.titleY = titleY, .highScoreY = highScoreY, .menuY = menuY};
}

auto hudScale(const Game& game) -> float
{
    return hudScaleOptions.at(static_cast<size_t>(game.resources.settings.hudScaleIndex));
}

auto mouseUIPos(const Game& game) -> Vector2
{
    const Vector2 mouse = GetMousePosition();
    const Rectangle rect = letterBoxRect(game);
    const float scale =
        rect.width / static_cast<float>(game.resources.screenWidth) * hudScale(game);
    if (scale <= 0)
    {
        return mouse;
    }
    return Vector2{.x = (mouse.x - rect.x) / scale, .y = (mouse.y - rect.y) / scale};
}

auto guiUiScale(const Game& game) -> float
{
    constexpr float referenceHeight = 1080.0F;
    constexpr float minScale = 0.6F;
    constexpr float maxScale = 2.2F;
    return std::clamp(static_cast<float>(game.resources.windowHeight) / referenceHeight, minScale,
                      maxScale);
}

namespace
{
constexpr float pointerHoldThreshold = 0.3F;
constexpr float pointerTapMaxDrift = 16.0F;
}

void updatePointerHold(Game& game)
{
    auto& hold = game.pointerHold;
    hold.releasedAsTap = false;
    hold.position = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        hold.pressStartTime = static_cast<float>(GetTime());
        hold.pressStartPos = hold.position;
        hold.isHold = false;
    }
    else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        if (static_cast<float>(GetTime()) - hold.pressStartTime >= pointerHoldThreshold)
        {
            hold.isHold = true;
        }
    }
    else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        const float drift = Vector2Distance(hold.position, hold.pressStartPos);
        hold.releasedAsTap = !hold.isHold && drift <= pointerTapMaxDrift;
        hold.isHold = false;
    }
}

auto hoverActive(const Game& game) -> bool
{
    return !IsMouseButtonDown(MOUSE_BUTTON_LEFT) || game.pointerHold.isHold;
}

auto shipSelectLayout(const Game& game) -> ShipSelectLayout
{
    const float scale = guiUiScale(game);
    const auto w = static_cast<float>(game.resources.windowWidth);

    // Design-space (1080-tall reference) block, centered like titleLayout -
    // name/preview/desc/stats/select treated as one invisible container.
    constexpr int32_t previewRadiusBase = 70;
    constexpr int32_t selectHeightBase = 54;
    constexpr int32_t nameToPreviewGap = 110;
    constexpr int32_t previewToDescGap = 30;
    constexpr int32_t descToStatsGap = 30;
    constexpr int32_t statsToSelectGap = 50;
    constexpr int32_t totalHeight = 30 + nameToPreviewGap + previewRadiusBase * 2 +
                                    previewToDescGap + 20 + descToStatsGap + 20 +
                                    statsToSelectGap + selectHeightBase;
    constexpr int32_t referenceHeight = 1080;
    constexpr int32_t blockTop = (referenceHeight - totalHeight) / 2;

    constexpr int32_t nameY = blockTop + 30;
    constexpr int32_t previewCenterYBase = nameY + nameToPreviewGap + previewRadiusBase;
    constexpr int32_t descY = previewCenterYBase + previewRadiusBase + previewToDescGap;
    constexpr int32_t statsY = descY + descToStatsGap;
    constexpr int32_t selectCenterYBase = statsY + statsToSelectGap + selectHeightBase / 2;

    const Vector2 previewPos{.x = w / 2, .y = static_cast<float>(previewCenterYBase) * scale};

    constexpr float arrowOffsetXBase = 260.0F; // more breathing room from the ship than before
    constexpr float arrowHitRadiusBase = 36.0F;
    constexpr float selectWidthBase = 200.0F;
    // Safe strip down the middle (ship preview, desc/stats text, Select
    // button) that stays out of the tap zones below.
    constexpr float centerDeadZoneHalfWidthBase = 180.0F;

    const float scaledSelectWidth = selectWidthBase * scale;
    const float scaledSelectHeight = static_cast<float>(selectHeightBase) * scale;
    const Rectangle selectRect{
        .x = w / 2 - scaledSelectWidth / 2,
        .y = static_cast<float>(selectCenterYBase) * scale - scaledSelectHeight / 2,
        .width = scaledSelectWidth,
        .height = scaledSelectHeight,
    };

    const auto h = static_cast<float>(game.resources.windowHeight);
    const float deadZoneHalfWidth = centerDeadZoneHalfWidthBase * scale;

    return ShipSelectLayout{
        .nameY = nameY,
        .previewPos = previewPos,
        .previewRadius = static_cast<float>(previewRadiusBase) * scale,
        .leftArrow = Vector2{.x = w / 2 - arrowOffsetXBase * scale, .y = previewPos.y},
        .rightArrow = Vector2{.x = w / 2 + arrowOffsetXBase * scale, .y = previewPos.y},
        .arrowHitRadius = arrowHitRadiusBase * scale,
        .leftTapZone = Rectangle{.x = 0, .y = 0, .width = w / 2 - deadZoneHalfWidth, .height = h},
        .rightTapZone = Rectangle{.x = w / 2 + deadZoneHalfWidth,
                                  .y = 0,
                                  .width = w / 2 - deadZoneHalfWidth,
                                  .height = h},
        .descY = descY,
        .statsY = statsY,
        .selectRect = selectRect,
    };
}

auto menuColumnRect(const Game& game, int32_t index, int32_t count, int32_t width, int32_t height,
                    int32_t gap, int32_t topY) -> Rectangle
{
    (void)count;
    const float scale = guiUiScale(game);
    const float scaledWidth = static_cast<float>(width) * scale;
    const float scaledHeight = static_cast<float>(height) * scale;
    const float scaledGap = static_cast<float>(gap) * scale;
    const float scaledTopY = static_cast<float>(topY) * scale;

    return Rectangle{.x = static_cast<float>(game.resources.windowWidth) / 2 - scaledWidth / 2,
                     .y = scaledTopY + static_cast<float>(index) * (scaledHeight + scaledGap),
                     .width = scaledWidth,
                     .height = scaledHeight};
}

auto hoveredColumnRow(int32_t count, int32_t width, int32_t height, int32_t gap, int32_t topY,
                      const Game& game) -> std::optional<int32_t>
{
    const Vector2 mouse = GetMousePosition();
    for (int32_t i = 0; i < count; i++)
    {
        if (CheckCollisionPointRec(mouse, menuColumnRect(game, i, count, width, height, gap, topY)))
        {
            return i;
        }
    }
    return std::nullopt;
}

auto updateMenuSelectionWindow(Game& game, int32_t index, int32_t optionCount, int32_t width,
                               int32_t height, int32_t gap, int32_t topY) -> MenuSelection
{
    bool keyboardMoved = false;
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
    {
        index--;
        keyboardMoved = true;
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
    {
        index++;
        keyboardMoved = true;
    }

    if (index < 0)
    {
        index = optionCount - 1;
    }
    if (index >= optionCount)
    {
        index = 0;
    }

    bool confirmed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    const Vector2 mouseDelta = GetMouseDelta();
    const bool mouseMoved =
        std::abs(mouseDelta.x) > FLT_EPSILON || std::abs(mouseDelta.y) > FLT_EPSILON;
    if (!keyboardMoved && mouseMoved)
    {
        if (const auto row = hoveredColumnRow(optionCount, width, height, gap, topY, game);
            row.has_value())
        {
            index = *row;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (const auto row = hoveredColumnRow(optionCount, width, height, gap, topY, game);
            row.has_value())
        {
            index = *row;
            confirmed = true;
        }
    }

    return MenuSelection{.index = index, .confirmed = confirmed};
}
