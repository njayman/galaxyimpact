#pragma once

#include "game.hpp"
#include "raylib.h"
#include <cstdint>

// Row layout for every menu/picker screen - shared by the drawXxx functions
// here and the future updateXxx/hoveredRow hit-testing, so the two can never
// drift out of sync with each other.
namespace MenuLayout
{
constexpr int32_t lineHeight = 35;

constexpr int32_t titleMenuY = 220;
constexpr int32_t pausedMenuY = 300;
constexpr int32_t gameOverMenuY = 420;

constexpr int32_t levelUpMenuY = 200;
constexpr int32_t levelUpLineHeight = 62;

constexpr int32_t settingsMenuY = 240;
constexpr int32_t settingsLineHeight = 45;
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
