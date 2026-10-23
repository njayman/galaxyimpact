# UI revamp

**Status:** Phase 1 built, 2026-10-23. Theme chosen: **minimal floating**. The in-run HUD and menus look basic: flat bars, plain text stats, empty slot boxes, a nameless boss bar, a permanent controls line, and an Instructions button over the HP bar.

A ship-anchored theme (HP and Nerve arcs around the ship) was considered and set aside.

## Shared pieces

One small set of drawing helpers used everywhere, so every screen matches: a thin segmented line, a pip row, a small label, a toast. Colours stay on the existing `Palette` (matte darks, red accent, warm crit).

## Phase 1: minimal floating HUD

No panels or boxes: thin lines and small numbers at the screen edges, fading back when nothing changes.

- **Top left:** HP as a thin line split into one segment per hit point, the count beside it; Nerve as a hairline under it that brightens when a burst is ready; dash charges and shield stacks as small pips at the line's end. Damage flashes the lost segment; low HP pulses.
- **Top edge:** a hairline XP line across the full width, the level as a small number at its left.
- **Top right:** wave and score as small numbers (`W12   1,540`), DMG under them; the biome name fades in under them when it changes.
- **Bottom left:** weapon slots as small bare icons with level dots under them, a thin cooldown underline and an evolved tint; no boxes.
- **Boss:** a long hairline across the bottom centre with the name in small caps above it, phase ticks, and a pale damage trail behind the fill; plates or shields as pips.
- **Toasts:** small text that fades in under the top-right numbers and drifts up; several stack.
- **Quiet when calm:** vitals dim after a few seconds with nothing changing and light up on damage, pickups or charge.
- **Controls line:** shown for the first seconds of a run, then fades out.
- **Instructions button:** moved off the HP area.

## Phase 1 as built (2026-10-23)

- XP is a 2 px line along the top edge. The level sits with the wave under the score (`WAVE 1   LV 1`), not at the left of the XP line.
- HP segments are 4 px; Nerve is a 1.5 px line under them (`SPACE` beside it when ready); dash dots and shield chevrons sit below. Vitals dim to 45% after 3 s with no change.
- Weapon slots show bare icons with level dots and an evolved glow; there is no cooldown underline. The tooltip opens above the slots.
- Boss lines are all drawn in the accent red (a boss's own colour changes with its state), with ticks at 50% and 20% for director bosses, which change phase there.
- One toast at a time, under the top-right numbers.
- The controls line sits at the top centre for the first 8 s of a run. The sandbox button moved to the top centre.

## Phase 2: menus and popups

In the same theme: buttons (thin outline, hover and press states), title and pause menus, level-up cards, settings inputs (sliders, toggles, steppers), ship select, game over and score screens, high scores, achievements, bestiary, skill tree, sandbox menu, tooltips and confirm prompts.
