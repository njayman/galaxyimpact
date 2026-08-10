#pragma once

#include "game.hpp"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

void enterSandbox(Game& game);

void updateSandboxInput(Game& game);

auto sandboxPickupName(int32_t index) -> std::string_view;
auto sandboxPickupPreview(int32_t index) -> PickupCatalogEntry;

struct SandboxMenuButton
{
    std::function<std::string(const Game&)> label;
    std::function<void(Game&)> action;
};

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

constexpr int32_t browserCategoryCount = 6;
constexpr int32_t browserParticleStyleCount = 3;

auto browserCategoryLabel(int32_t category) -> std::string_view;
auto browserParticleStyleName(int32_t index) -> std::string_view;
auto browserItemCount(int32_t category) -> int32_t;
auto browserIndexRef(Game& game) -> int&;
auto browserCurrentName(const Game& game) -> std::string;
auto browserCurrentDetails(const Game& game) -> std::string;
void browserCycleItem(Game& game, int32_t dir);

auto sandboxBrowserPreviewRect(const Game& game) -> Rectangle;
auto sandboxBrowserCategoryButtonRect(const Game& game, int32_t index) -> Rectangle;
auto sandboxBrowserPrevButtonRect(const Game& game) -> Rectangle;
auto sandboxBrowserNextButtonRect(const Game& game) -> Rectangle;
auto sandboxBrowserModeToggleRect(const Game& game) -> Rectangle;
