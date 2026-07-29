#include "menu.hpp"

#include "draw.hpp"
#include <algorithm>
#include <cfloat>
#include <cmath>

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
