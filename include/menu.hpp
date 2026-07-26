#pragma once

#include "game.hpp"
#include "raylib.h"
#include <cstdint>
#include <optional>

// mouseUIPos maps the real mouse position (window space) into the fixed
// logical screenWidth/screenHeight space that gameplay HUD drawing uses,
// inverting the letterbox scale/offset applied when blitting to the window.
// Menus don't use this - see menuColumnRect below, they're laid out directly
// in real window space so raygui's own mouse hit-testing (which always reads
// GetMousePosition() in window space) lines up with what's drawn.
auto mouseUIPos(const Game& game) -> Vector2;

// guiUiScale is how much bigger/smaller menu buttons and their text should
// be drawn at the current window size, relative to the 1080-tall reference
// size everything was tuned at - this is what makes the raygui menu screens
// actually responsive (grow/shrink with the real window) rather than a
// fixed-size UI merely letterboxed like the gameplay HUD is. Clamped so an
// extreme window size doesn't produce illegibly tiny or absurdly huge
// controls.
auto guiUiScale(const Game& game) -> float;

// menuColumnRect computes the Rectangle for row `index` of a `count`-row
// vertical stack of buttons, each `width`x`height` (pre-guiUiScale - this
// function applies the scale itself) with `gap` between them, centered
// horizontally in the real window and starting `topY` down (also
// pre-scale) - shared by hoveredColumnRow (input hit-testing) and the
// raygui draw calls (rendering) so they can never drift apart.
auto menuColumnRect(const Game& game, int32_t index, int32_t count, int32_t width, int32_t height,
                    int32_t gap, int32_t topY) -> Rectangle;

// hoveredColumnRow returns the row index the mouse is currently over, given
// the same column layout menuColumnRect describes, or nullopt if the mouse
// isn't over any row.
auto hoveredColumnRow(int32_t count, int32_t width, int32_t height, int32_t gap, int32_t topY,
                      const Game& game) -> std::optional<int32_t>;

// updateMenuSelectionWindow moves index with W/S/Up/Down or mouse hover over
// a row, and reports a confirm on Enter/Space or a left-click on the hovered
// row - the window-space equivalent of the old fixed-canvas menu selection,
// used by every raygui-driven menu screen (Title/Paused/GameOver/LevelUp).
// Mouse hover only steals the highlighted row when the mouse actually moved
// this frame, so a keyboard nav press isn't immediately overwritten by the
// mouse just sitting on the old row (see the History: this was a real bug).
struct MenuSelection
{
    int32_t index;
    bool confirmed;
};
auto updateMenuSelectionWindow(Game& game, int32_t index, int32_t optionCount, int32_t width,
                               int32_t height, int32_t gap, int32_t topY) -> MenuSelection;
