package main

import rl "github.com/gen2brain/raylib-go/raylib"

// updateMenuSelection moves index with W/S or Up/Down and reports Enter/Space as confirm.
func updateMenuSelection(index int32, optionCount int32) (int32, bool) {
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

	return index, confirmed
}
