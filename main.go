package main

import (
	"flag"

	rl "github.com/gen2brain/raylib-go/raylib"
)

func main() {
	sandbox := flag.Bool("sandbox", false, "boot directly into an empty test arena (summon enemies/bosses, grant/reset abilities)")
	flag.Parse()

	rl.SetConfigFlags(rl.FlagWindowResizable | rl.FlagVsyncHint)
	rl.InitWindow(defaultWindowWidth, defaultWindowHeight, "Galaxy Impact")
	defer rl.CloseWindow()
	rl.SetExitKey(rl.KeyNull) // Escape is our own pause key, not the window-close key
	rl.SetWindowMinSize(320, 240)

	rl.InitAudioDevice()
	defer rl.CloseAudioDevice()

	rl.SetTargetFPS(60)

	game := InitGame()

	if opt := resolutionOptions[game.Settings.ResolutionIndex]; opt.Width != defaultWindowWidth || opt.Height != defaultWindowHeight {
		rl.SetWindowSize(int(opt.Width), int(opt.Height))
		syncScreenSize(game)
	}
	applyBGMState(game)

	if *sandbox {
		enterSandbox(game)
	}
	defer rl.UnloadRenderTexture(game.WorldTarget)
	defer rl.UnloadRenderTexture(game.PixelTarget)
	defer rl.UnloadMusicStream(game.BGM)

	shouldExit := false

	for !rl.WindowShouldClose() && !shouldExit {
		deltaTime := rl.GetFrameTime()

		shouldExit = UpdateGame(game, deltaTime)

		DrawGame(game)
	}
}
