#pragma once

#include "game.hpp"
#include "raylib.h"
#include <cstdint>
#include <optional>

auto mouseUIPos(const Game& game) -> Vector2;

auto guiUiScale(const Game& game) -> float;

auto hudScale(const Game& game) -> float;

auto menuColumnRect(const Game& game, int32_t index, int32_t count, int32_t width, int32_t height,
                    int32_t gap, int32_t topY) -> Rectangle;

auto hoveredColumnRow(int32_t count, int32_t width, int32_t height, int32_t gap, int32_t topY,
                      const Game& game) -> std::optional<int32_t>;

struct MenuSelection
{
    int32_t index;
    bool confirmed;
};
auto updateMenuSelectionWindow(Game& game, int32_t index, int32_t optionCount, int32_t width,
                               int32_t height, int32_t gap, int32_t topY) -> MenuSelection;
