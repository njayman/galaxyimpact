#pragma once

#include "game.hpp"
#include "raylib.h"
#include <cstdint>

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
}

void drawText(const Font& font, const char* text, int32_t x, int32_t y, int32_t size, Color color);
void drawText(const Game& game, const char* text, int32_t x, int32_t y, int32_t size, Color color);
auto measureText(const Font& font, const char* text, int32_t size) -> int32_t;
auto measureText(const Game& game, const char* text, int32_t size) -> int32_t;

auto letterBoxRect(const Game& game) -> Rectangle;

auto drawGame(Game& game) -> void;
