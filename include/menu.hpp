#pragma once

#include "game.hpp"
#include "raylib.h"
#include <cstdint>
#include <optional>

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
