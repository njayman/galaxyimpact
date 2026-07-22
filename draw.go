package main

import (
	"fmt"
	"math"

	rl "github.com/gen2brain/raylib-go/raylib"
)

func DrawGame(g *Game) {
	rl.BeginTextureMode(g.PixelTarget)
	rl.ClearBackground(colorVoid)

	beginPixelZoom(g)
	for _, p := range g.BgParticles {
		rl.DrawCircleV(p.Position, p.Radius, p.Color)
	}

	for _, s := range g.Stars {
		rl.DrawCircleV(s.Position, s.Radius, colorHaze)
	}
	rl.EndMode2D()

	switch g.State {
	case TITLE:
		beginPixelZoom(g)
		drawTitle(g)
		rl.EndMode2D()
	case GAMEPLAY:
		beginShake(g)
		drawGameplayWorld(g)
		rl.EndMode2D()
	case PAUSED:
		beginShake(g)
		drawGameplayWorld(g)
		rl.EndMode2D()
		beginPixelZoom(g)
		drawOverlay(g)
		drawMenu(g, "PAUSED", []string{"Resume", "New Game", "Exit"}, 300)
		rl.EndMode2D()
	case GAME_OVER:
		beginShake(g)
		drawGameplayWorld(g)
		rl.EndMode2D()
		beginPixelZoom(g)
		drawOverlay(g)
		drawGameOver(g)
		rl.EndMode2D()
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
// drawing calls down to fit the low-resolution render target.
func beginPixelZoom(g *Game) {
	rl.BeginMode2D(rl.Camera2D{Zoom: 1 / float32(pixelScale)})
}

// beginShake is beginPixelZoom plus a random offset that shrinks as the
// shake's remaining time runs out, so impacts read as a jolt that settles.
func beginShake(g *Game) {
	zoom := 1 / float32(pixelScale)
	offset := rl.Vector2{}

	if g.ShakeTimer > 0 && g.ShakeDuration > 0 {
		amplitude := g.ShakeIntensity * (g.ShakeTimer / g.ShakeDuration)
		offset.X = float32(rl.GetRandomValue(-100, 100)) / 100 * amplitude * zoom
		offset.Y = float32(rl.GetRandomValue(-100, 100)) / 100 * amplitude * zoom
	}

	rl.BeginMode2D(rl.Camera2D{Offset: offset, Zoom: zoom})
}

func drawTitle(g *Game) {
	title := "GALAXY IMPACT"
	rl.DrawText(title, g.ScreenWidth/2-rl.MeasureText(title, 50)/2, 80, 50, colorAccent)

	drawMenu(g, "", []string{"Start", "Exit"}, 220)
	drawHighScores(g.ScreenWidth/2-80, 330, g.HighScores)
}

func drawGameOver(g *Game) {
	var message string
	if g.Player.Health <= 0 {
		message = "YOU WERE DEFEATED!"
	} else {
		message = "BOSS DEFEATED!"
	}

	rl.DrawText(message, g.ScreenWidth/2-rl.MeasureText(message, 40)/2, 90, 40, colorAccent)

	scoreText := fmt.Sprintf("Score: %d", g.Score)
	rl.DrawText(scoreText, g.ScreenWidth/2-rl.MeasureText(scoreText, 24)/2, 145, 24, colorStructLight)

	drawHighScores(g.ScreenWidth/2-80, 195, g.HighScores)
	drawMenu(g, "", []string{"New Game", "Exit"}, 420)
}

func drawOverlay(g *Game) {
	rl.DrawRectangle(0, 0, g.ScreenWidth, g.ScreenHeight, rl.Fade(colorVoid, 0.75))
}

// drawMenu renders an optional heading followed by a vertical list of options,
// highlighting the currently selected one, starting at the given y.
func drawMenu(g *Game, heading string, options []string, y int32) {
	if heading != "" {
		rl.DrawText(heading, g.ScreenWidth/2-rl.MeasureText(heading, 30)/2, y-50, 30, colorStructLight)
	}

	for i, option := range options {
		textWidth := rl.MeasureText(option, 24)
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

		rl.DrawText(option, textX, rowY, 24, color)
	}
}

func drawHighScores(x, y int32, scores []int32) {
	rl.DrawText("Top Scores", x, y, 26, colorAccentDim)

	if len(scores) == 0 {
		rl.DrawText("(none yet)", x, y+32, 24, colorStructMid)
		return
	}

	for i, s := range scores {
		rl.DrawText(fmt.Sprintf("%d. %d", i+1, s), x, y+32+int32(i)*30, 24, colorStructLight)
	}
}

func drawGameplayWorld(g *Game) {
	if g.BlackHole.Active {
		rl.DrawCircleV(g.BlackHole.Position, g.BlackHole.Radius, colorVoid)
		rl.DrawCircleLines(int32(g.BlackHole.Position.X), int32(g.BlackHole.Position.Y), g.BlackHole.Radius, colorStructMid)
		rl.DrawRing(g.BlackHole.Position, g.BlackHole.Radius+6, g.BlackHole.Radius+9, float32(rl.GetTime())*40, float32(rl.GetTime())*40+120, 16, rl.Fade(colorStructMid, 0.6))
		rl.DrawRing(g.BlackHole.Position, g.BlackHole.Radius+14, g.BlackHole.Radius+17, -float32(rl.GetTime())*30, -float32(rl.GetTime())*30+90, 16, rl.Fade(colorHaze, 0.4))
	}

	for i := range int(g.Player.Health) {
		posX := float32(g.ScreenWidth - int32(30) - (int32(i) * int32(25)))
		v1 := rl.NewVector2(posX, 10)
		v2 := rl.NewVector2(posX-10, 30)
		v3 := rl.NewVector2(posX+10, 30)

		rl.DrawTriangle(v1, v2, v3, colorAccent)
	}

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

	for _, a := range g.Asteroids {
		rl.DrawCircleV(a.Position, a.Radius, colorStructMid)
		rl.DrawCircleV(rl.NewVector2(a.Position.X-a.Radius/3, a.Position.Y-a.Radius/4), a.Radius/4, colorStructDark)
		rl.DrawCircleV(rl.NewVector2(a.Position.X+a.Radius/4, a.Position.Y+a.Radius/3), a.Radius/5, colorStructDark)
	}

	if g.Boss.State == SHOOTING && g.Boss.Attack == AttackBeam {
		beamStart := bossCenter
		direction := rl.Vector2Normalize(rl.Vector2Subtract(g.Boss.TargetPosition, bossCenter))
		beamEnd := rl.Vector2Add(beamStart, rl.Vector2Scale(direction, float32(g.ScreenWidth)+1.5))

		beamLength := float32(g.ScreenWidth) * 1.5

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

	shipVisible := g.Player.ImmunityTimer <= 0 || int(rl.GetTime()*10)%2 == 0
	if shipVisible {
		p := g.Player.Position
		r := g.Player.Radius

		shipColor := g.Player.Color
		if g.Player.Charging {
			fraction := g.Player.ChargeTimer / chargeFullTime
			if fraction > 1 {
				fraction = 1
			}

			particleColor := colorCharge
			if fraction >= 1 {
				pulse := 0.7 + 0.3*float32(math.Sin(float64(rl.GetTime())*8))
				shipColor = rl.Fade(colorCrit, pulse)
				particleColor = colorCrit
			} else {
				shipColor = colorCharge
			}

			drawChargeParticles(p, fraction, particleColor, 45, 18)
		}

		rl.DrawTriangle(rl.NewVector2(p.X, p.Y-r), rl.NewVector2(p.X-r*0.8, p.Y+r), rl.NewVector2(p.X+r*0.8, p.Y+r), shipColor)
		rl.DrawCircleV(rl.NewVector2(p.X, p.Y), r*0.3, colorStructLight)
	}

	if g.Player.ShieldActive {
		shieldRadius := g.Player.Radius + 5
		rl.DrawCircleV(g.Player.Position, shieldRadius, rl.Fade(colorShield, 0.5))
		rl.DrawCircleLines(int32(g.Player.Position.X), int32(g.Player.Position.Y), shieldRadius, colorShield)
	} else {
		drawShieldIndicator(g.Player)
	}

	for _, b := range g.Bullets {
		rl.DrawCircleV(b.Position, b.Radius, b.Color)
	}

	rl.DrawText(fmt.Sprintf("Boss Health: %.0f%%  Score: %d", (float32(g.Boss.Health)/float32(g.Boss.MaxHealth))*100, g.Score), 10, 10, 24, colorStructLight)

	rl.DrawText("Move: WASD | Shoot: Hold Space | Shield: J | Pause: Esc | Fullscreen: F11", 10, 44, 22, colorStructMid)
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

// drawComet renders a homing boss projectile as a bright head with a fading
// tail streaming back opposite its direction of travel.
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
