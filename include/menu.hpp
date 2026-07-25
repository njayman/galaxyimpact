#pragma once

#include "game.hpp"
#include "raylib.h"
#include <cstdint>
#include <optional>

// mouseUIPos maps the real mouse position (window space) into the fixed
// logical screenWidth/screenHeight space that all menu/HUD drawing uses,
// inverting the letterbox scale/offset applied when blitting to the window.
auto mouseUIPos(const Game& game) -> Vector2;

// hoveredRow returns the row index the mouse is currently over, given rows
// laid out as full-width bands starting at y, lineHeight apart - matching
// drawMenu/drawLevelUp/drawSettings' layout - or nullopt if the mouse isn't
// over any row.
auto hoveredRow(Game& game, int32_t count, int32_t y, int32_t lineHeight) -> std::optional<int32_t>;

// updateMenuSelection moves index with W/S/Up/Down or mouse hover over a
// row, and reports a confirm on Enter/Space or a left-click on the hovered
// row.
struct MenuSelection
{
    int32_t index;
    bool confirmed;
};
auto updateMenuSelection(Game& game, int32_t index, int32_t optionCount, int32_t y,
                         int32_t lineHeight) -> MenuSelection;
