#pragma once

#include "game.hpp"

// Play starts the tutorial until Settings::tutorialState is set (backlog N1, milestone M20).
void startTutorial(Game& game);
// Returns true while a dialog freezes the run; the caller skips the rest of the frame.
auto updateTutorial(Game& game, float deltaTime) -> bool;
auto tutorialDialog(const Game& game) -> const char*;
auto tutorialObjective(const Game& game) -> const char*;
