package main

import rl "github.com/gen2brain/raylib-go/raylib"

// Row layout for every menu/picker screen - shared by both the drawXxx
// function that renders a screen and the updateXxx/hoveredRow call that
// hit-tests it, so the two can never drift out of sync with each other.
const (
	menuLineHeight = int32(35)

	titleMenuY    = int32(220)
	pausedMenuY   = int32(300)
	gameOverMenuY = int32(420)

	levelUpMenuY      = int32(200)
	levelUpLineHeight = int32(62)

	settingsMenuY      = int32(240)
	settingsLineHeight = int32(45)
)

// mouseUIPos maps the real mouse position (window space) into the fixed
// logical ScreenWidth/ScreenHeight space that all menu/HUD drawing uses,
// inverting the letterbox scale/offset applied when blitting to the window.
func mouseUIPos(g *Game) rl.Vector2 {
	mouse := rl.GetMousePosition()
	rect := letterboxRect(g)
	scale := rect.Width / float32(g.ScreenWidth)
	if scale <= 0 {
		return mouse
	}
	return rl.NewVector2((mouse.X-rect.X)/scale, (mouse.Y-rect.Y)/scale)
}

// hoveredRow returns the row index the mouse is currently over, given rows
// laid out as full-width bands starting at y, lineHeight apart - matching
// drawMenu/drawLevelUp/drawSettings' layout - or false if the mouse isn't
// over any row.
func hoveredRow(g *Game, count int32, y int32, lineHeight int32) (int32, bool) {
	mouse := mouseUIPos(g)
	if mouse.X < 0 || mouse.X > float32(g.ScreenWidth) {
		return 0, false
	}

	for i := int32(0); i < count; i++ {
		rowY := y + i*lineHeight - lineHeight/3
		if mouse.Y >= float32(rowY) && mouse.Y < float32(rowY+lineHeight) {
			return i, true
		}
	}

	return 0, false
}

// updateMenuSelection moves index with W/S/Up/Down or mouse hover over a
// row, and reports a confirm on Enter/Space or a left-click on the hovered
// row.
func updateMenuSelection(g *Game, index int32, optionCount int32, y int32, lineHeight int32) (int32, bool) {
	if rl.IsKeyPressed(rl.KeyW) || rl.IsKeyPressed(rl.KeyUp) {
		index--
	}
	if rl.IsKeyPressed(rl.KeyS) || rl.IsKeyPressed(rl.KeyDown) {
		index++
	}

	if index < 0 {
		index = optionCount - 1
	}
	if index >= optionCount {
		index = 0
	}

	confirmed := rl.IsKeyPressed(rl.KeyEnter) || rl.IsKeyPressed(rl.KeySpace)

	if row, hovering := hoveredRow(g, optionCount, y, lineHeight); hovering {
		index = row
		if rl.IsMouseButtonPressed(rl.MouseButtonLeft) {
			confirmed = true
		}
	}

	return index, confirmed
}
