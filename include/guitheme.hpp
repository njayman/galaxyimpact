#pragma once

#include "game.hpp"

// setupGuiTheme points raygui at the game's own font and repaints its global
// style with the game's Palette colors instead of raygui's stock light-gray
// look, so every raygui-driven menu (see draw.cpp's drawTitleMenu/
// drawPausedMenu/drawSettingsMenu/drawLevelUpMenu/drawGameOverMenu) reads as
// part of the same game instead of a bolted-on generic UI kit. Call once,
// after InitGame() has loaded game.font.
void setupGuiTheme(const Game& game);

// applyGuiScale re-applies the text-size portion of the theme for the
// current window size (see menu.hpp's guiUiScale) - call once per frame,
// before drawing any raygui menu screen, so resizing the window live keeps
// menu text readable/proportionate instead of staying pinned to whatever
// size it was at startup.
void applyGuiScale(const Game& game);
