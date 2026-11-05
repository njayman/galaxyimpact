#pragma once

#include "game.hpp"

void setupGuiTheme(const Game& game);

void applyGuiScale(const Game& game);

// The minimal button used everywhere instead of GuiButton (docs/design/ui.md): no box, a
// hairline under it; focused (GuiSetState) or hovered adds an accent bar, a warm label and a faint
// fill. Draw only: input is handled by each screen's update.
void uiButton(Rectangle rect, const char* text);
