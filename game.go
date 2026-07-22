package main

import (
	rl "github.com/gen2brain/raylib-go/raylib"

	"github.com/njayman/galaxyimpact/highscore"
)

type GameState int

const (
	TITLE GameState = iota
	GAMEPLAY
	PAUSED
	GAME_OVER
)

const highScoreFile = "highscores.txt"

type Game struct {
	// ScreenWidth/ScreenHeight are the fixed logical design resolution that all
	// game logic and drawing coordinates are expressed in - never changed at
	// runtime. WindowWidth/WindowHeight track the actual OS window size (which
	// does change, e.g. in fullscreen) purely so the final frame can be scaled
	// and letterboxed to fit it without anything in the game moving or resizing.
	ScreenWidth        int32
	ScreenHeight       int32
	WindowWidth        int32
	WindowHeight       int32
	State              GameState
	Player             Player
	Boss               Boss
	Bullets            []Bullet
	Asteroids          []Asteroid
	Projectiles        []BossProjectile
	BlackHole          BlackHole
	Stars              []Star
	BgParticles        []BgParticle
	BorderStars        []Star
	AsteroidSpawnTimer float32
	SpreadWindupShots  int32
	Score              int32
	HighScores         []int32
	HighScoreRepo      highscore.Repository
	MenuIndex          int32
	ScoreRecorded      bool
	Sounds             Sounds
	ShakeTimer         float32
	ShakeDuration      float32
	ShakeIntensity     float32
	HitPauseTimer      float32
	PixelTarget        rl.RenderTexture2D
}

// pixelScale is how chunky the retro-pixel look is: the game is rendered into
// a render texture this many times smaller than the window, then scaled back
// up with nearest-neighbor filtering. Kept modest (2x) so text stays legible.
const pixelScale = int32(2)

func InitGame() *Game {
	g := &Game{}

	g.ScreenWidth = 800
	g.ScreenHeight = 600
	g.WindowWidth = 800
	g.WindowHeight = 600
	g.State = TITLE

	g.Stars = make([]Star, 80)
	for i := range g.Stars {
		g.Stars[i] = Star{
			Position: rl.NewVector2(float32(rl.GetRandomValue(0, g.ScreenWidth)), float32(rl.GetRandomValue(0, g.ScreenHeight))),
			Radius:   float32(rl.GetRandomValue(10, 20)) / 10.0,
		}
	}

	bgColors := []rl.Color{rl.Fade(colorStructMid, 0.15), rl.Fade(colorHaze, 0.1), rl.Fade(colorAccentDim, 0.08)}
	g.BgParticles = make([]BgParticle, 25)
	for i := range g.BgParticles {
		g.BgParticles[i] = BgParticle{
			Position: rl.NewVector2(float32(rl.GetRandomValue(0, g.ScreenWidth)), float32(rl.GetRandomValue(0, g.ScreenHeight))),
			Velocity: rl.NewVector2(float32(rl.GetRandomValue(-4, 4))/10.0, float32(rl.GetRandomValue(-4, 4))/10.0),
			Radius:   float32(rl.GetRandomValue(15, 40)) / 10.0,
			Color:    bgColors[rl.GetRandomValue(0, int32(len(bgColors)-1))],
		}
	}

	// BorderStars cover a large virtual area (up to 8K) at native resolution so
	// the letterbox bars around the pixel-art frame show more starfield instead
	// of flat black, on any window/monitor size.
	g.BorderStars = make([]Star, 400)
	for i := range g.BorderStars {
		g.BorderStars[i] = Star{
			Position: rl.NewVector2(float32(rl.GetRandomValue(0, 7680)), float32(rl.GetRandomValue(0, 4320))),
			Radius:   float32(rl.GetRandomValue(10, 20)) / 10.0,
		}
	}

	g.HighScoreRepo = highscore.NewFileRepository(highScoreFile)
	g.HighScores, _ = g.HighScoreRepo.Load()

	g.Sounds = LoadSounds()

	g.PixelTarget = rl.LoadRenderTexture(g.ScreenWidth/pixelScale, g.ScreenHeight/pixelScale)
	rl.SetTextureFilter(g.PixelTarget.Texture, rl.FilterPoint)

	resetRun(g)
	g.State = TITLE

	return g
}

// toggleFullscreen switches between windowed and fullscreen. Going fullscreen
// resizes the window to the current monitor's native resolution first (so it
// fills displays up to 8K), then hands off to raylib's fullscreen mode. The
// game's own logical resolution never changes - see syncScreenSize.
func toggleFullscreen(g *Game) {
	if rl.IsWindowFullscreen() {
		rl.ToggleFullscreen()
		rl.SetWindowSize(800, 600)
	} else {
		monitor := rl.GetCurrentMonitor()
		rl.SetWindowSize(rl.GetMonitorWidth(monitor), rl.GetMonitorHeight(monitor))
		rl.ToggleFullscreen()
	}

	syncScreenSize(g)
}

// syncScreenSize tracks the actual window size (whether it changed via
// toggleFullscreen or the player dragging the resizable window's edges) so
// DrawGame can scale+letterbox the fixed-resolution frame to fit it. Nothing
// about the game's own coordinate space or render target changes here - that
// fixed logical resolution is what keeps every element, text included,
// scaling uniformly instead of shrinking relative to a bigger window.
func syncScreenSize(g *Game) {
	g.WindowWidth = int32(rl.GetScreenWidth())
	g.WindowHeight = int32(rl.GetScreenHeight())
}

// resetRun (re)initializes everything needed for a fresh playthrough, without
// touching window-level state (screen size, starfield, loaded high scores).
func resetRun(g *Game) {
	g.Player = Player{
		Position: rl.NewVector2(float32(g.ScreenWidth)/2, float32(g.ScreenHeight)/2),
		Radius:   15,
		Color:    colorAccent,
		Speed:    5,
		Health:   5,
	}

	g.Boss = Boss{
		Position:    rl.NewVector2(float32(g.ScreenWidth)/2-50, 50),
		Size:        rl.NewVector2(100, 100),
		Color:       colorBossIdle,
		Health:      500,
		MaxHealth:   500,
		State:       IDLE,
		Attack:      AttackBeam,
		AttackTimer: float32(rl.GetRandomValue(15, 35)) / 10.0,
	}

	g.Bullets = []Bullet{}
	g.Asteroids = []Asteroid{}
	g.Projectiles = []BossProjectile{}
	g.BlackHole = BlackHole{Timer: float32(rl.GetRandomValue(30, 60)) / 10.0}
	g.AsteroidSpawnTimer = 1.0
	g.SpreadWindupShots = 0
	g.ShakeTimer = 0
	g.ShakeDuration = 0
	g.ShakeIntensity = 0
	g.HitPauseTimer = 0
	g.Score = 0
	g.ScoreRecorded = false
	g.State = GAMEPLAY
}
