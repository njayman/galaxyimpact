#include "guitheme.hpp"

#include "menu.hpp"
#include "palette.hpp"
#include "raygui.h"
#include <algorithm>
#include <cmath>

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

void uiButton(Rectangle rect, const char* text, bool hoverHighlights)
{
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    const bool disabled = GuiGetState() == STATE_DISABLED;
    const bool focused = !disabled && (GuiGetState() == STATE_FOCUSED || (hoverHighlights && hovered));
    const bool pressed = focused && hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const float unit = std::max(1.0F, rect.height / 26.0F); // 1 px at the reference size
    const auto time = static_cast<float>(GetTime());

    const Font font = GuiGetFont();
    const auto size = static_cast<float>(GuiGetStyle(DEFAULT, TEXT_SIZE));
    const auto spacing = static_cast<float>(GuiGetStyle(DEFAULT, TEXT_SPACING));
    const Vector2 measured = MeasureTextEx(font, text, size, spacing);
    const float cx = rect.x + rect.width / 2;
    const float cy = rect.y + rect.height / 2;
    const float underY = cy + measured.y / 2 + 3 * unit;

    if (focused)
    {
        // A band brightest at the centre, fading out to both sides.
        const Color mid = Fade(Palette::Accent, pressed ? 0.26F : 0.12F);
        const Color clear = Fade(Palette::Accent, 0.0F);
        const float half = rect.width / 2;
        DrawRectangleGradientH(static_cast<int32_t>(rect.x), static_cast<int32_t>(rect.y), static_cast<int32_t>(half),
                               static_cast<int32_t>(rect.height), clear, mid);
        DrawRectangleGradientH(static_cast<int32_t>(cx), static_cast<int32_t>(rect.y), static_cast<int32_t>(half),
                               static_cast<int32_t>(rect.height), mid, clear);
        // Hairlines on the band's top and bottom edges, fading the same way.
        for (const float y : {rect.y, rect.y + rect.height - unit})
        {
            DrawRectangleGradientH(static_cast<int32_t>(rect.x), static_cast<int32_t>(y), static_cast<int32_t>(half),
                                   static_cast<int32_t>(unit), clear, Fade(Palette::Accent, 0.5F));
            DrawRectangleGradientH(static_cast<int32_t>(cx), static_cast<int32_t>(y), static_cast<int32_t>(half),
                                   static_cast<int32_t>(unit), Fade(Palette::Accent, 0.5F), clear);
        }
        // Chevrons either side of the label, nudging inward.
        const float bob = 2.0F * unit * (0.5F + 0.5F * std::sin(time * 6.0F));
        const float gap = measured.x / 2 + 12 * unit + bob;
        const float h = 5 * unit;
        for (const float dir : {-1.0F, 1.0F})
        {
            const float x = cx + dir * gap;
            DrawLineEx(Vector2{.x = x + dir * h, .y = cy - h}, Vector2{.x = x, .y = cy}, 2 * unit, Palette::Accent);
            DrawLineEx(Vector2{.x = x, .y = cy}, Vector2{.x = x + dir * h, .y = cy + h}, 2 * unit, Palette::Accent);
        }
        DrawRectangleRec(Rectangle{.x = cx - measured.x / 2, .y = underY, .width = measured.x, .height = unit},
                         Fade(Palette::Crit, 0.7F));
    }
    else
    {
        DrawRectangleRec(Rectangle{.x = cx - 8 * unit, .y = underY, .width = 16 * unit, .height = unit},
                         Fade(Palette::StructMid, 0.8F));
    }

    const Color color = disabled ? Palette::StructMid : pressed ? WHITE : focused ? Palette::Crit : Palette::StructLight;
    DrawTextEx(font, text, Vector2{.x = cx - measured.x / 2, .y = cy - measured.y / 2}, size, spacing, color);
}
