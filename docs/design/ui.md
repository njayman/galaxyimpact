# UI revamp

**Status:** Planned, 2026-10-23. The in-run HUD and menus look basic: flat bars, plain text stats, empty slot boxes, a nameless boss bar, a permanent controls line, and an Instructions button over the HP bar.

Two HUD themes are tried in turn, ship-anchored first, then minimal floating; the user picks one after seeing both. Everything else (menus, popups, buttons, inputs, scores) follows the chosen theme.

## Shared pieces

One small set of drawing helpers used everywhere, so every screen matches: a matte chamfered panel, a segmented bar, a pip row, a name plate, a toast pill. Colours stay on the existing `Palette` (matte darks, red accent, warm crit).

## Phase 1: ship-anchored HUD

The eyes never leave the ship.

- **HP:** an arc of segments (one per hit point) around the left of the ship; lost segments break off and fade. Low HP pulses.
- **Nerve:** an arc around the right of the ship, filling; glows when a burst is ready.
- **Dash charges and shield stacks:** small pips under and above the ship.
- **Quiet when calm:** the rings dim after a couple of seconds with nothing changing and light up again on damage, pickups or charge.
- **Top edge:** a thin full-width XP line with the level at its left; wave and score at the top centre (`W12  1,540`), DMG beside them; the biome name drops in under them when it changes.
- **Weapon slots:** bottom centre, matte tiles with the weapon icon, level pips, a cooldown sweep and an evolved glow; passives as smaller tiles beside them.
- **Boss bar:** above the slots; a name plate, phase notches, a pale damage trail behind the fill, and plate or shield pips where the boss has them.
- **Toasts:** a matte pill that slides down under the score; several stack.
- **Controls line:** shown for the first seconds of a run, then fades out.
- **Instructions button:** moved off the HP area.

## Phase 2: minimal floating HUD

The same information with no panels: thin lines and small numbers at the screen edges that fade when nothing changes. A setting switches between the two themes so they can be compared side by side.

## Phase 3: menus and popups

In the chosen theme: buttons (matte chamfered, hover and press states), title and pause menus, level-up cards, settings inputs (sliders, toggles, steppers), ship select, game over and score screens, high scores, achievements, bestiary, skill tree, sandbox menu, tooltips and confirm prompts.
