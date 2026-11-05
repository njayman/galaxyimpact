#include "guitheme.hpp"

#include "menu.hpp"
#include "palette.hpp"
#include "raygui.h"
#include <algorithm>

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
    GuiSetFont(game.resources.font);

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

    GuiSetStyle(BUTTON, BORDER_WIDTH, 2);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

    GuiSetStyle(CHECKBOX, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
    GuiSetStyle(CHECKBOX, BORDER_WIDTH, 1);
    GuiSetStyle(CHECKBOX, BASE_COLOR_NORMAL, ColorToInt(Palette::StructDark));
    GuiSetStyle(CHECKBOX, BASE_COLOR_FOCUSED, ColorToInt(Palette::StructMid));
    GuiSetStyle(CHECKBOX, BASE_COLOR_PRESSED, ColorToInt(Palette::Accent));

    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(Palette::StructLight));
}

void uiButton(Rectangle rect, const char* text)
{
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    const bool disabled = GuiGetState() == STATE_DISABLED;
    const bool focused = !disabled && (GuiGetState() == STATE_FOCUSED || hovered);
    const bool pressed = focused && hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const float line = std::max(1.0F, rect.height / 26.0F);

    if (focused)
    {
        DrawRectangleRec(rect, Fade(Palette::Accent, pressed ? 0.18F : 0.07F));
        DrawRectangleRec(Rectangle{.x = rect.x, .y = rect.y, .width = line * 2, .height = rect.height}, Palette::Accent);
    }
    DrawRectangleRec(Rectangle{.x = rect.x, .y = rect.y + rect.height - line, .width = rect.width, .height = line},
                     focused ? Palette::Accent : Fade(Palette::StructMid, 0.7F));

    const Font font = GuiGetFont();
    const auto size = static_cast<float>(GuiGetStyle(DEFAULT, TEXT_SIZE));
    const auto spacing = static_cast<float>(GuiGetStyle(DEFAULT, TEXT_SPACING));
    const Vector2 measured = MeasureTextEx(font, text, size, spacing);
    const Color color = disabled ? Palette::StructMid : pressed ? WHITE : focused ? Palette::Crit : Palette::StructLight;
    DrawTextEx(font, text,
               Vector2{.x = rect.x + (rect.width - measured.x) / 2, .y = rect.y + (rect.height - measured.y) / 2},
               size, spacing, color);
}
