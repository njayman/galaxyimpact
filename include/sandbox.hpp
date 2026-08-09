#pragma once

#include "game.hpp"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

void enterSandbox(Game& game);

void updateSandboxInput(Game& game);

auto sandboxPickupName(int32_t index) -> std::string_view;

// A one-shot/toggle action shown as a raygui button in the sandbox instructions menu
// (GameState::SANDBOX_MENU). Label is a function of game state so toggles (death enabled,
// natural spawn) can show their current ON/OFF value.
struct SandboxMenuButton
{
    std::function<std::string(const Game&)> label;
    std::function<void(Game&)> action;
};

// A cyclable value (enemy kind, boss attack, wave, ship, pickup) shown as a "-" button, a
// readonly value box with a label, and a "+" button.
struct SandboxMenuStepper
{
    std::string_view label;
    std::function<std::string(const Game&)> valueText;
    std::function<void(Game&)> decrement;
    std::function<void(Game&)> increment;
};

auto sandboxMenuButtons() -> const std::vector<SandboxMenuButton>&;
auto sandboxMenuSteppers() -> const std::vector<SandboxMenuStepper>&;

auto sandboxStepperRowRect(const Game& game, int32_t index) -> Rectangle;
auto sandboxStepperMinusRect(const Game& game, int32_t index) -> Rectangle;
auto sandboxStepperValueRect(const Game& game, int32_t index) -> Rectangle;
auto sandboxStepperPlusRect(const Game& game, int32_t index) -> Rectangle;
auto sandboxMenuButtonRect(const Game& game, int32_t index) -> Rectangle;
auto sandboxMenuCloseButtonRect(const Game& game) -> Rectangle;
auto sandboxToggleIndicatorRect(const Game& game) -> Rectangle;

void updateSandboxMenuInput(Game& game);
