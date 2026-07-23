package main

import (
	"fmt"
	"math"

	rl "github.com/gen2brain/raylib-go/raylib"
)

// enterSandbox resets to a clean, empty arena for manual testing: no waves,
// no auto-spawns, no asteroids/black hole - just the player, spawned on
// demand via updateSandboxInput. See main.go's -sandbox flag.
func enterSandbox(g *Game) {
	resetRun(g)
	g.Sandbox = true
	g.State = GAMEPLAY
	g.BlackHole.Active = false
}

// updateSandboxInput handles the sandbox-only hotkeys: summoning enemies/
// bosses, clearing the board, granting abilities via the normal level-up
// picker, and resetting/healing for quick iteration while testing.
func updateSandboxInput(g *Game) {
	if rl.IsKeyPressed(rl.KeyRightBracket) {
		g.SandboxKindIndex = (g.SandboxKindIndex + 1) % int32(len(enemyKinds))
	}
	if rl.IsKeyPressed(rl.KeyLeftBracket) {
		g.SandboxKindIndex = (g.SandboxKindIndex - 1 + int32(len(enemyKinds))) % int32(len(enemyKinds))
	}

	if rl.IsKeyPressed(rl.KeyE) {
		angle := float64(rl.GetRandomValue(0, 359)) * rl.Deg2rad
		dist := float32(rl.GetRandomValue(200, 320))
		pos := rl.Vector2Add(g.Player.Position, rl.NewVector2(float32(math.Cos(angle))*dist, float32(math.Sin(angle))*dist))
		spawnEnemyAt(g, int(g.SandboxKindIndex), pos)
	}

	if rl.IsKeyPressed(rl.KeyB) && !g.BossActive {
		spawnBoss(g)
	}

	if rl.IsKeyPressed(rl.KeyK) {
		g.Enemies = nil
		g.Asteroids = nil
		g.Projectiles = nil
		g.Mines = nil
		g.BossActive = false
		g.BossDeathShockwave = false
	}

	if rl.IsKeyPressed(rl.KeyL) {
		startLevelUp(g)
	}

	if rl.IsKeyPressed(rl.KeyH) {
		g.Player.Health = g.Player.MaxHealth
		g.Player.ShieldStacks = maxShieldStacks
		g.Player.Nerve = nerveMax
	}

	if rl.IsKeyPressed(rl.KeyR) {
		g.Weapons = []Weapon{{Kind: WeaponForward, Level: 1}}
		g.SkillLevels = map[SkillID]int32{SkillForwardShot: 1}
	}
}

func drawSandboxHUD(g *Game) {
	kindName := enemyKinds[g.SandboxKindIndex].Name
	lines := []string{
		"SANDBOX",
		fmt.Sprintf("[ / ]: cycle enemy (%s)   E: spawn enemy   B: spawn boss", kindName),
		"K: clear board   L: level-up picker   H: full heal   R: reset abilities",
	}

	y := g.ScreenHeight - 90
	for _, line := range lines {
		drawText(g, line, 10, y, 18, colorCrit)
		y += 22
	}
}
