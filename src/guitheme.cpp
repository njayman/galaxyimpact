#include "guitheme.hpp"

#include "menu.hpp"
#include "palette.hpp"
#include "raygui.h"

namespace
{
constexpr int32_t referenceTextSize = 22;
}

void applyGuiScale(const Game& game)
{
    GuiSetStyle(DEFAULT, TEXT_SIZE,
                static_cast<int>(static_cast<float>(referenceTextSize) * guiUiScale(game)));
}

void setupGuiTheme(const Game& game)
{
    GuiSetFont(game.font);

    GuiSetStyle(DEFAULT, TEXT_SIZE, referenceTextSize);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 1);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(Palette::Void));
    GuiSetStyle(DEFAULT, LINE_COLOR, ColorToInt(Palette::StructMid));

    GuiSetStyle(DEFAULT, BORDER_WIDTH, 2);
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(Palette::StructMid));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(Palette::StructDark));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(Palette::StructLight));

    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, ColorToInt(Palette::Accent));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, ColorToInt(Palette::StructMid));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt(Palette::Accent));

    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, ColorToInt(Palette::Crit));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, ColorToInt(Palette::AccentDim));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, ColorToInt(Palette::Crit));

    GuiSetStyle(DEFAULT, BORDER_COLOR_DISABLED, ColorToInt(Palette::StructDark));
    GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, ColorToInt(Palette::Void));
    GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, ColorToInt(Palette::StructMid));

    // Buttons read as flat accent-bordered panels rather than raygui's stock
    // bevelled look - matches the rest of the game's flat vector style.
    GuiSetStyle(BUTTON, BORDER_WIDTH, 2);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

    GuiSetStyle(CHECKBOX, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
    GuiSetStyle(CHECKBOX, BASE_COLOR_NORMAL, ColorToInt(Palette::StructDark));
    GuiSetStyle(CHECKBOX, BASE_COLOR_FOCUSED, ColorToInt(Palette::StructMid));
    GuiSetStyle(CHECKBOX, BASE_COLOR_PRESSED, ColorToInt(Palette::Accent));

    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(Palette::StructLight));
}
