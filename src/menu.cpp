#include "menu.hpp"

#include "draw.hpp"

auto mouseUIPos(const Game& game) -> Vector2
{
    const Vector2 mouse = GetMousePosition();
    const Rectangle rect = letterBoxRect(game);
    const float scale = rect.width / static_cast<float>(game.screenWidth);
    if (scale <= 0)
    {
        return mouse;
    }
    return Vector2{.x = (mouse.x - rect.x) / scale, .y = (mouse.y - rect.y) / scale};
}

auto hoveredRow(Game& game, int32_t count, int32_t y, int32_t lineHeight) -> std::optional<int32_t>
{
    const Vector2 mouse = mouseUIPos(game);
    if (mouse.x < 0 || mouse.x > static_cast<float>(game.screenWidth))
    {
        return std::nullopt;
    }

    for (int32_t i = 0; i < count; i++)
    {
        const int32_t rowY = y + i * lineHeight - lineHeight / 3;
        if (mouse.y >= static_cast<float>(rowY) && mouse.y < static_cast<float>(rowY + lineHeight))
        {
            return i;
        }
    }

    return std::nullopt;
}

auto updateMenuSelection(Game& game, int32_t index, int32_t optionCount, int32_t y,
                         int32_t lineHeight) -> MenuSelection
{
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
    {
        index--;
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
    {
        index++;
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

    if (const auto row = hoveredRow(game, optionCount, y, lineHeight); row.has_value())
    {
        index = *row;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            confirmed = true;
        }
    }

    return MenuSelection{.index = index, .confirmed = confirmed};
}
