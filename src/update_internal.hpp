#pragma once

// Declarations shared by the files split out of src/update.cpp; not a public interface.

#include "update.hpp"
#include "democonfig.hpp"
#include "draw.hpp"
#include "entities/item.hpp"
#include "entities/ship.hpp"
#include "highscore.hpp"
#include "menu.hpp"
#include "palette.hpp"
#include "raylib.h"
#include "raymath.h"
#include "sandbox.hpp"
#include "update_constants.hpp"
#include "progression.hpp"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

void updateHud(Game& game, float deltaTime);
auto updateSkillTree(Game& game) -> bool;
auto signatureEvolved(const Game& game) -> bool;
auto dronePosition(Game& game, size_t index) -> Vector2;
void updateDroneContactDamage(Game& game, float deltaTime, int32_t followerDmg, int32_t laserDmg);
