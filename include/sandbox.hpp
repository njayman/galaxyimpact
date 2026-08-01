#pragma once

#include "game.hpp"
#include <string_view>

void enterSandbox(Game& game);

void updateSandboxInput(Game& game);

auto sandboxPickupName(int32_t index) -> std::string_view;
