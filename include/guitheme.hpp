#pragma once

#include "game.hpp"

void setupGuiTheme(const Game& game);

void applyGuiScale(const Game& game);

// The minimal button used everywhere instead of GuiButton (docs/design/ui.md): a bare label with a
// short tick under it; focused (GuiSetState) adds a soft band fading out to the sides, chevrons
// either side of the label and an underline as wide as the text. Menus with a keyboard selection
// pass hoverHighlights = false so only the selected item lights up. Draw only: each screen's update
// handles input.
void uiButton(Rectangle rect, const char* text, bool hoverHighlights = true);
