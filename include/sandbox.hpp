#pragma once

#include "game.hpp"

// enterSandbox resets to a clean, empty arena for manual testing: no waves,
// no auto-spawns, no asteroids/black hole - just the player, spawned on
// demand via updateSandboxInput.
void enterSandbox(Game& game);

// updateSandboxInput handles the sandbox-only hotkeys: summoning enemies/
// bosses, clearing the board, granting abilities via the normal level-up
// picker, and resetting/healing for quick iteration while testing.
void updateSandboxInput(Game& game);
