#pragma once

#include "game.hpp"
#include "raylib.h"
#include <cstdint>

// Button-column layout for every raygui menu/picker screen - shared by the
// drawXxx functions here and the matching updateXxx/hoveredColumnRow hit
// testing (see menu.hpp's menuColumnRect), so the two can never drift out of
// sync with each other. All values are in window-space pixels at the
// 1080-tall reference size - menuColumnRect applies guiUiScale itself, so
// these don't need adjusting for actual window size.
namespace MenuLayout
{
constexpr int32_t buttonWidth = 340;
constexpr int32_t buttonHeight = 50;
constexpr int32_t buttonGap = 14;

constexpr int32_t titleMenuY = 430;
constexpr int32_t pausedMenuY = 360;
constexpr int32_t gameOverMenuY = 420;

constexpr int32_t levelUpWidth = 720;
constexpr int32_t levelUpHeight = 92;
constexpr int32_t levelUpGap = 16;
constexpr int32_t levelUpMenuY = 260;

constexpr int32_t settingsWidth = 600;
constexpr int32_t settingsHeight = 48;
constexpr int32_t settingsGap = 10;
constexpr int32_t settingsMenuY = 260;
} // namespace MenuLayout

// drawText/measureText wrap raylib's default-font-only DrawText/MeasureText
// to use the loaded readable font (see loadReadableFont) instead.
void drawText(const Game& game, const char* text, int32_t x, int32_t y, int32_t size, Color color);
auto measureText(const Game& game, const char* text, int32_t size) -> int32_t;

// letterBoxRect scales the fixed logical frame up (or down) to fit as much of
// the actual window as possible while preserving its aspect ratio, centering
// it and leaving black bars rather than stretching.
auto letterBoxRect(const Game& game) -> Rectangle;

// drawGame renders in three passes: worldTarget (low-res, pixel-art game
// world) -> pixelTarget (native resolution, world scaled up + UI text drawn
// crisp on top) -> the actual window (letterbox-scaled to fit).
auto drawGame(Game& game) -> void;
