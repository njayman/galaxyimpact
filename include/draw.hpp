#pragma once

#include "game.hpp"
#include "raylib.h"
#include <cstdint>

void drawText(const Font& font, const char* text, int32_t x, int32_t y, int32_t size, Color color);
void drawText(const Game& game, const char* text, int32_t x, int32_t y, int32_t size, Color color);
auto measureText(const Font& font, const char* text, int32_t size) -> int32_t;
auto measureText(const Game& game, const char* text, int32_t size) -> int32_t;

auto letterBoxRect(const Game& game) -> Rectangle;

auto drawGame(Game& game) -> void;
