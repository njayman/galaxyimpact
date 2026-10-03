#pragma once

#include "game.hpp"
#include "raylib.h"
#include <cstdint>
#include <optional>

namespace MenuLayout
{
constexpr int32_t buttonWidth = 340;
constexpr int32_t buttonHeight = 50;
constexpr int32_t buttonGap = 14;

constexpr int32_t pausedMenuY = 360;
constexpr int32_t gameOverMenuY = 420;

constexpr int32_t levelUpWidth = 720;
constexpr int32_t levelUpHeight = 92;
constexpr int32_t levelUpGap = 16;
constexpr int32_t levelUpMenuY = 260;

constexpr int32_t settingsWidth = 600;
constexpr int32_t settingsHeight = 48;
constexpr int32_t settingsGap = 10;
constexpr int32_t settingsMenuY = 260;
}

auto mouseUIPos(const Game& game) -> Vector2;

auto guiUiScale(const Game& game) -> float;

void updatePointerHold(Game& game);

// True when hover-driven UI should treat the pointer as "hovering": always
// true for a plain mouse move (button up), and true for a touch/click only
// once it's been held past the tap threshold (tap-and-hold on touch).
auto hoverActive(const Game& game) -> bool;

auto hudScale(const Game& game) -> float;

auto menuColumnRect(const Game& game, int32_t index, int32_t count, int32_t width, int32_t height,
                    int32_t gap, int32_t topY) -> Rectangle;

auto hoveredColumnRow(int32_t count, int32_t width, int32_t height, int32_t gap, int32_t topY,
                      const Game& game) -> std::optional<int32_t>;

struct MenuSelection
{
    int32_t index;
    bool confirmed;
};
auto updateMenuSelectionWindow(Game& game, int32_t index, int32_t optionCount, int32_t width,
                               int32_t height, int32_t gap, int32_t topY) -> MenuSelection;

// Layout for the sideways ship-select carousel: a preview in the middle
// flanked by prev/next arrows, and a Select button below. Shared between
// drawShipSelect (render) and updateShipSelect (hit-testing) so they can't
// drift apart.
struct ShipSelectLayout
{
    int32_t nameY; // unscaled, for windowText
    Vector2 previewPos;
    float previewRadius;
    Vector2 leftArrow;
    Vector2 rightArrow;
    float arrowHitRadius; // visual only now; tap zones below are the real hit-test
    // Whole left/right side of the screen (minus a safe strip around the
    // preview/Select button) changes ship - not just the small arrow icon,
    // since tapping precisely on it turned out fiddly.
    Rectangle leftTapZone;
    Rectangle rightTapZone;
    int32_t descY; // unscaled, for windowText
    int32_t statsY;
    Rectangle selectRect;
};
auto shipSelectLayout(const Game& game) -> ShipSelectLayout;

// Title screen's title/high-scores/menu block, treated as one invisible
// container and vertically centered as a unit (in the same 1080-tall design
// space guiUiScale scales everything else from). Shared between drawTitle
// and updateTitle so the menu's hit-testing can't drift from where it's
// actually drawn.
struct TitleLayout
{
    int32_t titleY;
    int32_t highScoreY;
    int32_t menuY;
};
auto titleLayout() -> TitleLayout;

// Bottom-center "Back" button for screens with dynamic-height content
// (Achievements, Bestiary) that would rather anchor to the window edge than
// track exactly where their content happens to end. Shared between draw and
// update so the drawn button and its hit-test can't drift apart. Also
// sidesteps a real bug the old "any click/tap closes" behavior had: opening
// one of these screens via a click leaves that click's *release* to land on
// the new screen's first frame, which a release-triggered close (like
// quickTapReleased) misreads as the closing gesture - a press-triggered
// button (IsMouseButtonPressed, like Settings' Back) doesn't have that
// problem, since the press edge already happened on the previous screen.
auto backButtonRect(const Game& game) -> Rectangle;

// Mouse wheel + click-drag scrolling for a ContentScroll, clamped to
// [0, maxOffset] (maxOffset is set by that screen's draw pass, from what it
// just measured - see ContentScroll).
void applyScrollInput(ContentScroll& scroll);
