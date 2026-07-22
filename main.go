package main

import (
	rl "github.com/gen2brain/raylib-go/raylib"
)

func main() {
	rl.SetConfigFlags(rl.FlagWindowResizable | rl.FlagVsyncHint)
	rl.InitWindow(800, 600, "Galaxy Impact")
	defer rl.CloseWindow()
	rl.SetExitKey(rl.KeyNull) // Escape is our own pause key, not the window-close key
	rl.SetWindowMinSize(320, 240)

	rl.InitAudioDevice()
	defer rl.CloseAudioDevice()

	rl.SetTargetFPS(60)

	game := InitGame()
	defer rl.UnloadRenderTexture(game.PixelTarget)

	shouldExit := false

	for !rl.WindowShouldClose() && !shouldExit {
		deltaTime := rl.GetFrameTime()

		shouldExit = UpdateGame(game, deltaTime)

		DrawGame(game)
	}
}
