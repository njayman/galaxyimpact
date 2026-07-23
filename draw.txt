package main

import (
	"fmt"
	"math"

	rl "github.com/gen2brain/raylib-go/raylib"
)

// drawText/measureText wrap raylib's default-font-only DrawText/MeasureText
// to use the loaded readable font (see loadReadableFont in game.go) instead.
func drawText(g *Game, text string, x, y, size int32, color rl.Color) {
	rl.DrawTextEx(g.Font, text, rl.NewVector2(float32(x), float32(y)), float32(size), 1, color)
}

func measureText(g *Game, text string, size int32) int32 {
	return int32(rl.MeasureTextEx(g.Font, text, float32(size), 1).X)
}

// DrawGame renders in three passes:
//  1. WorldTarget (low-res, pixelScale) - all game-world shapes (ship,
//     enemies, bullets, background stars, ...) go chunky/pixel-art here.
//  2. PixelTarget (native ScreenWidth x ScreenHeight) - WorldTarget scaled
//     back up with nearest-neighbor (crisp big pixels, not blurry AA), then
//     every bit of UI text drawn on top at native resolution so it stays sharp
//     regardless of how chunky the world looks.
//  3. The actual window - PixelTarget letterbox-scaled to fit, resolution
//     independent of both of the above.
func DrawGame(g *Game) {
	rl.BeginTextureMode(g.WorldTarget)
	rl.ClearBackground(colorVoid)

	switch g.State {
	case TITLE:
		beginPixelZoom(g)
		drawBackgroundStars(g, rl.Vector2{})
		rl.EndMode2D()
	case GAMEPLAY, PAUSED, LEVEL_UP, GAME_OVER:
		beginWorldCamera(g)
		drawGameplayWorld(g)
		rl.EndMode2D()
	case SETTINGS:
		if g.SettingsReturnState == PAUSED {
			beginWorldCamera(g)
			drawGameplayWorld(g)
			rl.EndMode2D()
		} else {
			beginPixelZoom(g)
			drawBackgroundStars(g, rl.Vector2{})
			rl.EndMode2D()
		}
	}

	rl.EndTextureMode()

	rl.BeginTextureMode(g.PixelTarget)
	rl.ClearBackground(colorVoid)

	worldSrc := rl.Rectangle{X: 0, Y: 0, Width: float32(g.WorldTarget.Texture.Width), Height: -float32(g.WorldTarget.Texture.Height)}
	worldDst := rl.Rectangle{X: 0, Y: 0, Width: float32(g.ScreenWidth), Height: float32(g.ScreenHeight)}
	rl.DrawTexturePro(g.WorldTarget.Texture, worldSrc, worldDst, rl.Vector2{}, 0, rl.White)

	switch g.State {
	case TITLE:
		drawTitle(g)
	case GAMEPLAY:
		drawHUD(g)
	case PAUSED:
		drawHUD(g)
		drawOverlay(g)
		drawMenu(g, "PAUSED", []string{"Resume", "Settings", "New Game", "Exit"}, 300)
	case LEVEL_UP:
		drawHUD(g)
		drawOverlay(g)
		drawLevelUp(g)
	case GAME_OVER:
		drawOverlay(g)
		drawGameOver(g)
	case SETTINGS:
		if g.SettingsReturnState == PAUSED {
			drawHUD(g)
		}
		drawOverlay(g)
		drawSettings(g)
	}

	rl.EndTextureMode()

	rl.BeginDrawing()
	rl.ClearBackground(colorVoid)

	for _, s := range g.BorderStars {
		if s.Position.X <= float32(g.WindowWidth) && s.Position.Y <= float32(g.WindowHeight) {
			rl.DrawCircleV(s.Position, s.Radius, colorHaze)
		}
	}

	srcRec := rl.Rectangle{X: 0, Y: 0, Width: float32(g.PixelTarget.Texture.Width), Height: -float32(g.PixelTarget.Texture.Height)}
	rl.DrawTexturePro(g.PixelTarget.Texture, srcRec, letterboxRect(g), rl.Vector2{}, 0, rl.White)

	rl.EndDrawing()
}

// letterboxRect scales the fixed logical frame up (or down) to fit as much of
// the actual window as possible while preserving its aspect ratio, centering
// it and leaving black bars rather than stretching - this is what keeps every
// element, text included, the same relative size on any window/monitor size.
func letterboxRect(g *Game) rl.Rectangle {
	scale := float32(g.WindowWidth) / float32(g.ScreenWidth)
	if heightScale := float32(g.WindowHeight) / float32(g.ScreenHeight); heightScale < scale {
		scale = heightScale
	}

	width := float32(g.ScreenWidth) * scale
	height := float32(g.ScreenHeight) * scale

	return rl.Rectangle{
		X:      (float32(g.WindowWidth) - width) / 2,
		Y:      (float32(g.WindowHeight) - height) / 2,
		Width:  width,
		Height: height,
	}
}

// beginPixelZoom starts a 2D camera that scales the (unchanged, 800x600-space)
// drawing calls down to fit the low-resolution render target, with no camera
// follow - used for UI (menus, title, overlays) which never scrolls.
func beginPixelZoom(g *Game) {
	rl.BeginMode2D(rl.Camera2D{Zoom: 1 / float32(pixelScale)})
}

// beginWorldCamera is beginPixelZoom plus following the player (who always
// renders pinned at screen center while the world scrolls past) and the
// screen-shake jolt on top of that centering.
func beginWorldCamera(g *Game) {
	zoom := 1 / float32(pixelScale)
	center := rl.Vector2{X: float32(g.ScreenWidth) / 2, Y: float32(g.ScreenHeight) / 2}
	offset := rl.Vector2Scale(center, zoom)

	if g.ShakeTimer > 0 && g.ShakeDuration > 0 {
		amplitude := g.ShakeIntensity * (g.ShakeTimer / g.ShakeDuration)
		offset.X += float32(rl.GetRandomValue(-100, 100)) / 100 * amplitude * zoom
		offset.Y += float32(rl.GetRandomValue(-100, 100)) / 100 * amplitude * zoom
	}

	rl.BeginMode2D(rl.Camera2D{Target: g.Player.Position, Offset: offset, Zoom: zoom})
}

// tiledWorldPos maps a BgParticle/Star's tile-space offset to the nearest
// copy of it relative to the given world position, so a small fixed set of
// decorations reads as an infinite tiling backdrop around wherever that
// position roams.
func tiledWorldPos(relativeTo rl.Vector2, offset rl.Vector2, tileW, tileH float32) rl.Vector2 {
	relX := float32(math.Mod(float64(offset.X-relativeTo.X), float64(tileW)))
	if relX > tileW/2 {
		relX -= tileW
	} else if relX < -tileW/2 {
		relX += tileW
	}

	relY := float32(math.Mod(float64(offset.Y-relativeTo.Y), float64(tileH)))
	if relY > tileH/2 {
		relY -= tileH
	} else if relY < -tileH/2 {
		relY += tileH
	}

	return rl.Vector2Add(relativeTo, rl.NewVector2(relX, relY))
}

func drawBackgroundStars(g *Game, relativeTo rl.Vector2) {
	tileW, tileH := float32(g.ScreenWidth), float32(g.ScreenHeight)

	for _, p := range g.BgParticles {
		rl.DrawCircleV(tiledWorldPos(relativeTo, p.Position, tileW, tileH), p.Radius, p.Color)
	}

	for _, s := range g.Stars {
		rl.DrawCircleV(tiledWorldPos(relativeTo, s.Position, tileW, tileH), s.Radius, colorHaze)
	}
}

func drawTitle(g *Game) {
	title := "GALAXY IMPACT"
	drawText(g, title, g.ScreenWidth/2-measureText(g, title, 50)/2, 80, 50, colorAccent)

	drawMenu(g, "", []string{"Start", "Settings", "Exit"}, 220)
	drawHighScores(g, g.ScreenWidth/2-80, 330, g.HighScores)
}

func drawGameOver(g *Game) {
	message := "YOU WERE DEFEATED!"

	drawText(g, message, g.ScreenWidth/2-measureText(g, message, 40)/2, 70, 40, colorAccent)

	statsText := fmt.Sprintf("Score: %d   Level: %d   Wave: %d   Time: %.0fs", g.Score, g.Level, g.WaveNumber, g.RunTime)
	drawText(g, statsText, g.ScreenWidth/2-measureText(g, statsText, 20)/2, 120, 20, colorStructLight)

	drawHighScores(g, g.ScreenWidth/2-80, 160, g.HighScores)
	drawMenu(g, "", []string{"New Game", "Exit"}, 420)
}

func drawOverlay(g *Game) {
	rl.DrawRectangle(0, 0, g.ScreenWidth, g.ScreenHeight, rl.Fade(colorVoid, 0.75))
}

// drawSettings shows the settings rows with the selected row highlighted and
// its current value alongside - Left/Right (or Enter) changes the value.
func drawSettings(g *Game) {
	heading := "SETTINGS"
	drawText(g, heading, g.ScreenWidth/2-measureText(g, heading, 34)/2, 130, 34, colorAccent)

	res := resolutionOptions[g.Settings.ResolutionIndex]
	displayMode := "Windowed"
	if rl.IsWindowFullscreen() {
		displayMode = "Fullscreen"
	}
	onOff := func(on bool) string {
		if on {
			return "On"
		}
		return "Off"
	}

	rows := []struct{ label, value string }{
		{"Resolution", fmt.Sprintf("%dx%d", res.Width, res.Height)},
		{"Difficulty", difficultyDefs[g.Settings.Difficulty].Name},
		{"BGM", onOff(g.Settings.BGMOn)},
		{"Sound", onOff(g.Settings.SoundOn)},
		{"Display Mode", displayMode},
		{"Back", ""},
	}

	y := int32(240)
	for i, row := range rows {
		color := colorStructLight
		if int32(i) == g.MenuIndex {
			color = colorAccent
		}

		text := row.label
		if row.value != "" {
			text = fmt.Sprintf("%s:  < %s >", row.label, row.value)
		}

		drawText(g, text, g.ScreenWidth/2-measureText(g, text, 26)/2, y, 26, color)
		y += 45
	}

	hint := "Up/Down: select   Left/Right: change   Enter: confirm"
	drawText(g, hint, g.ScreenWidth/2-measureText(g, hint, 16)/2, y+20, 16, colorStructMid)
}

// drawLevelUp shows the rolled skill/evolve choices as a non-cancelable picker.
func drawLevelUp(g *Game) {
	heading := fmt.Sprintf("LEVEL %d", g.Level)
	drawText(g, heading, g.ScreenWidth/2-measureText(g, heading, 34)/2, 100, 34, colorAccent)

	y := int32(200)
	for i, choice := range g.PendingChoices {
		color := colorStructLight
		if int32(i) == g.MenuIndex {
			color = colorAccent
		}

		var name, desc string
		switch choice.Kind {
		case ChoiceEvolve:
			name = fmt.Sprintf("EVOLVE: %s", evolvedWeaponName[choice.Weapon])
			desc = "Fuses the weapon and its linked passive into a super weapon."
			if int32(i) == g.MenuIndex {
				color = colorCrit
			}
		case ChoiceLifeOrbs:
			name = fmt.Sprintf("%d x Life Orb", choice.Count)
			desc = "All slots are full and maxed - a direct health reward instead."
		case ChoiceShields:
			name = fmt.Sprintf("%d x Shield", choice.Count)
			desc = "All slots are full and maxed - a direct shield reward instead."
		default:
			def := skillDefs[choice.Skill]
			lvl := g.SkillLevels[choice.Skill]
			name = fmt.Sprintf("%s (Lv %d)", def.Name, lvl+1)
			desc = def.Description
		}

		drawText(g, name, g.ScreenWidth/2-measureText(g, name, 24)/2, y, 24, color)
		drawText(g, desc, g.ScreenWidth/2-measureText(g, desc, 15)/2, y+27, 15, colorStructMid)

		y += 62
	}
}

// drawMenu renders an optional heading followed by a vertical list of options,
// highlighting the currently selected one, starting at the given y.
func drawMenu(g *Game, heading string, options []string, y int32) {
	if heading != "" {
		drawText(g, heading, g.ScreenWidth/2-measureText(g, heading, 30)/2, y-50, 30, colorStructLight)
	}

	for i, option := range options {
		textWidth := measureText(g, option, 24)
		textX := g.ScreenWidth/2 - textWidth/2
		rowY := y + int32(i)*35

		color := colorStructLight
		if int32(i) == g.MenuIndex {
			color = colorAccent

			arrowX := float32(textX - 20)
			arrowY := float32(rowY) + 12
			rl.DrawTriangle(
				rl.NewVector2(arrowX, arrowY-8),
				rl.NewVector2(arrowX, arrowY+8),
				rl.NewVector2(arrowX+14, arrowY),
				colorAccent,
			)
		}

		drawText(g, option, textX, rowY, 24, color)
	}
}

func drawHighScores(g *Game, x, y int32, scores []int32) {
	drawText(g, "Top Scores", x, y, 26, colorAccentDim)

	if len(scores) == 0 {
		drawText(g, "(none yet)", x, y+32, 24, colorStructMid)
		return
	}

	for i, s := range scores {
		drawText(g, fmt.Sprintf("%d. %d", i+1, s), x, y+32+int32(i)*30, 24, colorStructLight)
	}
}

func drawGameplayWorld(g *Game) {
	drawBackgroundStars(g, g.Player.Position)

	if g.BlackHole.Active {
		rl.DrawCircleV(g.BlackHole.Position, g.BlackHole.Radius, colorVoid)
		rl.DrawCircleLines(int32(g.BlackHole.Position.X), int32(g.BlackHole.Position.Y), g.BlackHole.Radius, colorStructMid)
		rl.DrawRing(g.BlackHole.Position, g.BlackHole.Radius+6, g.BlackHole.Radius+9, float32(rl.GetTime())*40, float32(rl.GetTime())*40+120, 16, rl.Fade(colorStructMid, 0.6))
		rl.DrawRing(g.BlackHole.Position, g.BlackHole.Radius+14, g.BlackHole.Radius+17, -float32(rl.GetTime())*30, -float32(rl.GetTime())*30+90, 16, rl.Fade(colorHaze, 0.4))
	}

	for _, a := range g.Asteroids {
		rl.DrawCircleV(a.Position, a.Radius, colorStructMid)
		rl.DrawCircleV(rl.NewVector2(a.Position.X-a.Radius/3, a.Position.Y-a.Radius/4), a.Radius/4, colorStructDark)
		rl.DrawCircleV(rl.NewVector2(a.Position.X+a.Radius/4, a.Position.Y+a.Radius/3), a.Radius/5, colorStructDark)
	}

	for _, e := range g.Enemies {
		drawEnemy(g, e)
	}

	for _, p := range g.Pickups {
		switch p.Kind {
		case PickupLifeOrb:
			rl.DrawCircleV(p.Position, 5, colorAccent)
			rl.DrawCircleLines(int32(p.Position.X), int32(p.Position.Y), 6, rl.Fade(colorCrit, 0.8))
		case PickupShield:
			rl.DrawCircleV(p.Position, 6, colorShield)
			rl.DrawCircleLines(int32(p.Position.X), int32(p.Position.Y), 7, rl.Fade(colorHaze, 0.8))
		default:
			rl.DrawCircleV(p.Position, 4, colorCharge)
			rl.DrawCircleLines(int32(p.Position.X), int32(p.Position.Y), 5, rl.Fade(colorCrit, 0.6))
		}
	}

	if g.BossActive {
		drawBoss(g)
	}

	for _, m := range g.Mines {
		if !m.Active {
			continue
		}
		rl.DrawCircleV(m.Position, 6, colorCrit)
		rl.DrawCircleLines(int32(m.Position.X), int32(m.Position.Y), m.Radius, rl.Fade(colorCrit, 0.2))
	}

	if g.BossDeathShockwave {
		progress := 1 - g.BossDeathShockwaveTimer/bossDeathShockwaveDuration
		if progress < 0 {
			progress = 0
		}
		if progress > 1 {
			progress = 1
		}
		radius := maxSlamRadius * progress
		rl.DrawCircleLines(int32(g.BossDeathShockwavePos.X), int32(g.BossDeathShockwavePos.Y), radius, rl.Fade(colorCrit, 0.6*(1-progress)))
		rl.DrawCircleLines(int32(g.BossDeathShockwavePos.X), int32(g.BossDeathShockwavePos.Y), radius*0.85, rl.Fade(colorAccent, 0.4*(1-progress)))
	}

	for _, p := range g.Projectiles {
		if p.Homing {
			drawComet(p)
		} else {
			rl.DrawCircleV(p.Position, p.Radius, colorBossSpread)
		}
	}

	if g.Player.BlackHoleCoreTimer > 0 {
		rl.DrawCircleLines(int32(g.Player.Position.X), int32(g.Player.Position.Y), g.Player.Radius+6, rl.Fade(colorAccent, g.Player.BlackHoleCoreTimer))
	}

	drawOrbitBlades(g)

	shipVisible := g.Player.ImmunityTimer <= 0 || int(rl.GetTime()*10)%2 == 0
	if shipVisible {
		drawShip(g)
	}

	if g.Player.ShieldActive {
		shieldRadius := g.Player.Radius + 5
		rl.DrawCircleV(g.Player.Position, shieldRadius, rl.Fade(colorShield, 0.5))
		rl.DrawCircleLines(int32(g.Player.Position.X), int32(g.Player.Position.Y), shieldRadius, colorShield)
	} else if g.Player.ShieldCooldownTimer > 0 {
		drawShieldIndicator(g.Player)
	}

	for _, b := range g.Bullets {
		rl.DrawCircleV(b.Position, b.Radius, b.Color)
	}
}

func drawShip(g *Game) {
	p := g.Player.Position
	r := g.Player.Radius

	dir := aimAtMouse(g)
	angle := float32(math.Atan2(float64(dir.Y), float64(dir.X))) + math.Pi/2

	rotate := func(v rl.Vector2) rl.Vector2 {
		cos, sin := float32(math.Cos(float64(angle))), float32(math.Sin(float64(angle)))
		return rl.NewVector2(v.X*cos-v.Y*sin, v.X*sin+v.Y*cos)
	}

	if g.Player.Dashing {
		dashDir := rl.Vector2Normalize(g.Player.DashVelocity)
		for i := 1; i <= 3; i++ {
			trailPos := rl.Vector2Subtract(p, rl.Vector2Scale(dashDir, float32(i)*10))
			rl.DrawCircleV(trailPos, r*(1-float32(i)*0.2), rl.Fade(colorCrit, 0.35/float32(i)))
		}
	}

	shipColor := g.Player.Color
	if g.Player.Dashing {
		shipColor = colorCrit
	}

	tip := rl.Vector2Add(p, rotate(rl.NewVector2(0, -r)))
	left := rl.Vector2Add(p, rotate(rl.NewVector2(-r*0.8, r)))
	right := rl.Vector2Add(p, rotate(rl.NewVector2(r*0.8, r)))

	rl.DrawTriangle(tip, left, right, shipColor)
	rl.DrawCircleV(p, r*0.3, colorStructLight)
}

func drawEnemy(g *Game, e Enemy) {
	kind := enemyKinds[e.Kind]
	color := kind.Color
	if e.Phased {
		color = rl.Fade(color, 0.35)
	}
	if e.IsElite {
		rl.DrawCircleLines(int32(e.Position.X), int32(e.Position.Y), kind.Radius+4, colorCrit)
	}

	rl.DrawCircleV(e.Position, kind.Radius, color)

	maxHealth := int32(float32(kind.Health) * difficultyDefs[g.Settings.Difficulty].EnemyHealthMult)
	if e.IsElite {
		maxHealth *= 2
	}
	healthFrac := float32(e.Health) / float32(maxHealth)
	if healthFrac < 1 && healthFrac > 0 {
		barWidth := kind.Radius * 2
		rl.DrawRectangle(int32(e.Position.X-kind.Radius), int32(e.Position.Y-kind.Radius-8), int32(barWidth), 3, rl.Fade(colorStructDark, 0.6))
		rl.DrawRectangle(int32(e.Position.X-kind.Radius), int32(e.Position.Y-kind.Radius-8), int32(barWidth*healthFrac), 3, colorAccent)
	}
}

func drawBoss(g *Game) {
	bossCenter := rl.NewVector2(g.Boss.Position.X+g.Boss.Size.X/2, g.Boss.Position.Y+g.Boss.Size.Y/2)
	ufoColor := g.Boss.Color
	if g.Boss.Health <= 0 {
		ufoColor = colorStructDark
	}
	rl.DrawEllipse(int32(bossCenter.X), int32(bossCenter.Y)+int32(g.Boss.Size.Y/4), g.Boss.Size.X/2, g.Boss.Size.Y/4, ufoColor)
	rl.DrawCircle(int32(bossCenter.X), int32(bossCenter.Y)-int32(g.Boss.Size.Y/8), g.Boss.Size.X/3.5, rl.Fade(ufoColor, 0.8))

	if g.Boss.State == WINDING_UP {
		progress := 1 - g.Boss.StateTimer/bossWindupDuration(g.Boss.Attack)
		if progress < 0 {
			progress = 0
		}
		if progress > 1 {
			progress = 1
		}
		drawChargeParticles(bossCenter, progress, g.Boss.Color, 100, 65)
	}

	if g.Boss.Health > 0 {
		healthPercentage := float32(g.Boss.Health) / float32(g.Boss.MaxHealth)
		healthBarWidth := float32(g.Boss.Size.X) * healthPercentage
		rl.DrawRectangle(int32(g.Boss.Position.X), int32(g.Boss.Position.Y)-20, int32(g.Boss.Size.X), 15, rl.Fade(colorHaze, 0.25))
		rl.DrawRectangle(int32(g.Boss.Position.X), int32(g.Boss.Position.Y)-20, int32(healthBarWidth), 15, colorHaze)
		rl.DrawRectangleLines(int32(g.Boss.Position.X), int32(g.Boss.Position.Y)-20, int32(g.Boss.Size.X), 15, colorStructDark)
	}

	if g.Boss.State == SHOOTING && g.Boss.Attack == AttackBeam {
		beamStart := bossCenter
		direction := rl.Vector2Normalize(rl.Vector2Subtract(g.Boss.TargetPosition, bossCenter))
		beamEnd := rl.Vector2Add(beamStart, rl.Vector2Scale(direction, 2000))

		beamLength := float32(2000)

		for _, a := range g.Asteroids {
			if CheckCollisionCircleLine(a.Position, a.Radius, beamStart, beamEnd) {
				if dist := rl.Vector2Distance(bossCenter, a.Position); dist < beamLength {
					beamLength = dist
				}
			}
		}

		beamRec := rl.Rectangle{X: bossCenter.X, Y: bossCenter.Y, Width: beamLength, Height: 20}
		beamOrigin := rl.NewVector2(0, beamRec.Height/2)

		rl.DrawRectanglePro(beamRec, beamOrigin, g.Boss.BeamRotation, rl.Fade(colorAccent, 0.7))
	}

	if g.Boss.State == SHOOTING && g.Boss.Attack == AttackSlam {
		progress := 1 - g.Boss.StateTimer/slamDuration
		if progress < 0 {
			progress = 0
		}
		if progress > 1 {
			progress = 1
		}
		rl.DrawCircleLines(int32(bossCenter.X), int32(bossCenter.Y), maxSlamRadius*progress, rl.Fade(colorAccent, 1-progress*0.5))
	}
}

// drawOrbitBlades is purely decorative (the Orbit weapon's actual damage is a
// periodic AoE pulse, see updateWeapons) - small dots rotating at the pulse
// radius so the player can see the weapon's reach.
// drawOrbitBlades draws decorative reach indicators for the AoE/beam weapons
// - Orbit Blades as rotating dots, Shockwave/Mine Layer as a faint radius
// ring, Beam Sweep as a faint line toward the cursor - all purely visual,
// the actual damage is the periodic pulse in updateWeapons.
func drawOrbitBlades(g *Game) {
	for _, w := range g.Weapons {
		color := colorShield
		if w.Evolved {
			color = colorCrit
		}

		switch w.Kind {
		case WeaponOrbit:
			radius := orbitRadius(w.Level)
			if w.Evolved {
				radius *= 1.4
			}
			count := 2 + w.Level/2

			for s := int32(0); s < count; s++ {
				angle := float64(rl.GetTime())*2 + float64(s)*2*math.Pi/float64(count)
				pos := rl.Vector2Add(g.Player.Position, rl.NewVector2(
					float32(math.Cos(angle))*radius,
					float32(math.Sin(angle))*radius,
				))
				rl.DrawCircleV(pos, 5, color)
			}
		case WeaponShock:
			maxRadius := shockwaveRadius(w.Level, w.Evolved)
			if w.FlashTimer > 0 {
				// Pulse expands outward and fades as FlashTimer counts down from
				// shockFlashDuration, so it reads as a wave moving with the player
				// rather than a static ring.
				progress := 1 - w.FlashTimer/shockFlashDuration
				pulseRadius := maxRadius * progress
				rl.DrawCircleLines(int32(g.Player.Position.X), int32(g.Player.Position.Y), pulseRadius, rl.Fade(color, 0.7*(1-progress)))
			}
			rl.DrawCircleLines(int32(g.Player.Position.X), int32(g.Player.Position.Y), maxRadius, rl.Fade(color, 0.15))
		case WeaponBeam:
			length := float32(300 + w.Level*15)
			if w.Evolved {
				length *= 1.3
			}
			dir := aimAtMouse(g)
			end := rl.Vector2Add(g.Player.Position, rl.Vector2Scale(dir, length))
			rl.DrawLineEx(g.Player.Position, end, 2, rl.Fade(color, 0.3))
		}
	}
}

// drawShieldIndicator shows shield readiness when the shield isn't currently
// up: an arc filling in as the cooldown counts down, or a steady ring once
// ready.
func drawShieldIndicator(player Player) {
	const cooldownDuration = float32(2.0)
	ringRadius := player.Radius + 10

	if player.ShieldCooldownTimer > 0 {
		progress := 1 - player.ShieldCooldownTimer/cooldownDuration
		rl.DrawCircleLines(int32(player.Position.X), int32(player.Position.Y), ringRadius, rl.Fade(colorStructMid, 0.5))
		rl.DrawRing(player.Position, ringRadius-2, ringRadius, -90, -90+360*progress, 32, colorShield)
	} else {
		pulse := 0.4 + 0.2*float32(math.Sin(float64(rl.GetTime())*3))
		rl.DrawCircleLines(int32(player.Position.X), int32(player.Position.Y), ringRadius, rl.Fade(colorShield, pulse))
	}
}

// drawChargeParticles draws small motes spiraling inward toward center, closing
// in as fraction approaches 1 (fully charged) to read as energy being gathered.
func drawChargeParticles(center rl.Vector2, fraction float32, color rl.Color, maxRadius, minRadius float32) {
	const count = 6

	t := float32(rl.GetTime())
	radius := maxRadius - (maxRadius-minRadius)*fraction

	for i := 0; i < count; i++ {
		angle := t*4 + float32(i)*(2*math.Pi/count)
		pos := rl.NewVector2(
			center.X+float32(math.Cos(float64(angle)))*radius,
			center.Y+float32(math.Sin(float64(angle)))*radius,
		)
		rl.DrawCircleV(pos, 3, color)
	}
}

// drawComet renders a homing projectile as a bright head with a fading tail
// streaming back opposite its direction of travel.
func drawComet(p BossProjectile) {
	speed := rl.Vector2Length(p.Velocity)
	tailDir := rl.NewVector2(0, -1)
	if speed > 0 {
		tailDir = rl.Vector2Scale(p.Velocity, -1/speed)
	}

	const segments = 5
	for i := segments; i >= 1; i-- {
		frac := float32(i) / segments
		segPos := rl.Vector2Add(p.Position, rl.Vector2Scale(tailDir, p.Radius*2*float32(i)))
		rl.DrawCircleV(segPos, p.Radius*(1-frac*0.6), rl.Fade(colorBossHoming, 0.5*(1-frac)))
	}

	rl.DrawCircleV(p.Position, p.Radius, rl.Fade(colorHaze, 0.9))
	rl.DrawCircleV(p.Position, p.Radius*0.6, colorBossHoming)
}

func drawHUD(g *Game) {
	drawHealthPips(g, 10, 10)

	drawText(g, fmt.Sprintf("Score: %d   Wave: %d   Lv: %d", g.Score, g.WaveNumber, g.Level), 10, 38, 20, colorStructLight)

	xpFrac := float32(g.XP) / float32(g.XPToNext)
	if xpFrac > 1 {
		xpFrac = 1
	}
	rl.DrawRectangle(10, 64, 300, 10, rl.Fade(colorStructMid, 0.5))
	rl.DrawRectangle(10, 64, int32(300*xpFrac), 10, colorCharge)
	rl.DrawRectangleLines(10, 64, 300, 10, colorStructDark)

	drawChargePips(g, 10, 82)
	drawShieldStackPips(g, 70, 82)

	drawText(g, "Move: WASD | Dash: L-Click | Shield: R-Click | Pause: Esc | F11: Fullscreen", 10, 110, 18, colorStructMid)
}

// drawHealthPips shows health as a row of ship-icon "lives" (full MaxHealth
// count) instead of a number - filled/accent for current Health, outlined
// and dim for missing ones. A collected-but-not-yet-converted life orb shows
// as a faint half-filled icon in the next empty slot.
func drawHealthPips(g *Game, x, y int32) {
	const size = float32(11)
	const gap = float32(24)

	drawIcon := func(cx float32, fillColor rl.Color, filled bool) {
		top := rl.NewVector2(cx, float32(y))
		left := rl.NewVector2(cx-size*0.8, float32(y)+size*2)
		right := rl.NewVector2(cx+size*0.8, float32(y)+size*2)

		if filled {
			rl.DrawTriangle(top, left, right, fillColor)
		} else {
			rl.DrawTriangleLines(top, left, right, rl.Fade(colorStructMid, 0.6))
		}
	}

	for i := int32(0); i < g.Player.MaxHealth; i++ {
		cx := float32(x) + size + float32(i)*gap

		switch {
		case i < g.Player.Health:
			drawIcon(cx, colorAccent, true)
		case i == g.Player.Health && g.Player.HalfLifeOrb:
			drawIcon(cx, rl.Fade(colorAccent, 0.45), true)
		default:
			drawIcon(cx, rl.Color{}, false)
		}
	}
}

// drawShieldStackPips shows the rare-drop damage shields (0-3) as small
// hexagon-ish rings, separate from the dash-ability shield pips.
func drawShieldStackPips(g *Game, x, y int32) {
	const pipRadius = float32(8)
	const gap = float32(22)

	for i := int32(0); i < maxShieldStacks; i++ {
		center := rl.NewVector2(float32(x)+pipRadius+float32(i)*gap, float32(y)+pipRadius)

		if i < g.Player.ShieldStacks {
			rl.DrawCircleV(center, pipRadius, rl.Fade(colorShield, 0.85))
			rl.DrawCircleLines(int32(center.X), int32(center.Y), pipRadius, colorHaze)
		} else {
			rl.DrawCircleLines(int32(center.X), int32(center.Y), pipRadius, rl.Fade(colorStructMid, 0.5))
		}
	}
}

// drawChargePips shows the shared dash/shield charge pool as two pips: full
// bright when available, or a fill-arc showing regen progress when spent.
func drawChargePips(g *Game, x, y int32) {
	const pipRadius = float32(9)
	const gap = float32(24)

	for i := int32(0); i < maxCharges; i++ {
		center := rl.NewVector2(float32(x)+pipRadius+float32(i)*gap, float32(y)+pipRadius)

		if i < g.Player.Charges {
			rl.DrawCircleV(center, pipRadius, colorShield)
			rl.DrawCircleLines(int32(center.X), int32(center.Y), pipRadius, colorStructDark)
			continue
		}

		rl.DrawCircleLines(int32(center.X), int32(center.Y), pipRadius, rl.Fade(colorStructMid, 0.6))
		if i == g.Player.Charges {
			progress := float32(1)
			if d := chargeRegenDuration(g); d > 0 {
				progress = 1 - g.Player.ChargeRegenTimer/d
			}
			rl.DrawRing(center, pipRadius-3, pipRadius, -90, -90+360*progress, 16, colorShield)
		}
	}
}
