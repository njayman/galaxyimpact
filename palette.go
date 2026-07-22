package main

import rl "github.com/gen2brain/raylib-go/raylib"

// A minimalist, mostly-monochrome palette: cool blue-grays for the void and
// structure, one saturated accent (red) reserved for the player and real
// danger, plus a handful of muted attack-telegraph hues kept just distinct
// enough from each other and from the accent to stay readable.
var (
	colorVoid        = rl.NewColor(10, 12, 18, 255)
	colorStructDark  = rl.NewColor(28, 34, 46, 255)
	colorStructMid   = rl.NewColor(56, 66, 84, 255)
	colorStructLight = rl.NewColor(118, 132, 154, 255)
	colorHaze        = rl.NewColor(150, 165, 185, 255)

	colorAccent    = rl.NewColor(196, 58, 58, 255)
	colorAccentDim = rl.NewColor(140, 40, 40, 255)

	colorShield = rl.NewColor(150, 180, 200, 255)
	colorCharge = rl.NewColor(224, 178, 110, 255)
	colorCrit   = rl.NewColor(255, 226, 170, 255)

	colorBossIdle   = rl.NewColor(70, 90, 120, 255)
	colorBossHoming = rl.NewColor(150, 95, 55, 255)
	colorBossSpread = rl.NewColor(140, 70, 95, 255)
)
