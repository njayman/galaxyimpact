package main

import (
	"math"
	"os"

	rl "github.com/gen2brain/raylib-go/raylib"

	"github.com/njayman/galaxyimpact/highscore"
)

type GameState int

const (
	TITLE GameState = iota
	GAMEPLAY
	PAUSED
	GAME_OVER
	LEVEL_UP
	SETTINGS
)

// arenaHalf bounds the otherwise-open world: play reads as infinite (no
// visible edges under normal movement) but is backed by this large finite
// square so spawn-ring/background math stays simple bounded arithmetic.
const arenaHalf = float32(4000)

// waveDuration is how long (seconds) each wave lasts before the next begins.
const waveDuration = float32(25)

const highScoreFile = "highscores.txt"

type Game struct {
	// ScreenWidth/ScreenHeight are the fixed logical design resolution that all
	// game logic and drawing coordinates are expressed in - never changed at
	// runtime. WindowWidth/WindowHeight track the actual OS window size (which
	// does change, e.g. in fullscreen) purely so the final frame can be scaled
	// and letterboxed to fit it without anything in the game moving or resizing.
	ScreenWidth             int32
	ScreenHeight            int32
	WindowWidth             int32
	WindowHeight            int32
	State                   GameState
	Player                  Player
	Boss                    Boss
	BossActive              bool
	Bullets                 []Bullet
	Asteroids               []Asteroid
	Projectiles             []BossProjectile
	Enemies                 []Enemy
	Pickups                 []Pickup
	Mines                   []Mine
	Weapons                 []Weapon
	SkillLevels             map[SkillID]int32
	PendingChoices          []LevelUpChoice
	PostCapDamageLevels     int32
	BossDeathShockwave      bool
	BossDeathShockwaveTimer float32
	BossDeathShockwavePos   rl.Vector2
	BossDeathShockwaveHit   bool
	BlackHole               BlackHole
	Stars                   []Star
	BgParticles             []BgParticle
	BorderStars             []Star
	BoundaryClouds          []GasCloud
	DeathParticles          []Particle
	AsteroidSpawnTimer      float32
	EnemySpawnTimer         float32
	SpreadWindupShots       int32
	XP                      int32
	Level                   int32
	XPToNext                int32
	WaveNumber              int32
	WaveTimer               float32
	RunTime                 float32
	Score                   int32
	HighScores              []int32
	HighScoreRepo           highscore.Repository
	MenuIndex               int32
	ScoreRecorded           bool
	Sounds                  Sounds
	ShakeTimer              float32
	ShakeDuration           float32
	ShakeIntensity          float32
	HitPauseTimer           float32
	WorldTarget             rl.RenderTexture2D
	PixelTarget             rl.RenderTexture2D
	Font                    rl.Font
	Settings                Settings
	SettingsReturnState     GameState
	BGM                     rl.Music
	Sandbox                 bool
	SandboxKindIndex        int32
}

// pixelScale downscales WorldTarget (game-world shapes only: ship, enemies,
// bullets, background, ...) before it's scaled back up with nearest-neighbor
// filtering onto PixelTarget - this is what gives the deliberate chunky
// pixel-art look. Text is drawn separately, straight onto PixelTarget at its
// native ScreenWidth/ScreenHeight resolution (see DrawGame), so it stays
// crisp instead of inheriting the same blur a low-res-then-upscaled font
// would have. The final PixelTarget->window blit (see letterboxRect) is a
// separate, resolution-independence concern, unrelated to this chunkiness.
const pixelScale = int32(3)

// defaultWindowWidth/Height is the default logical resolution and starting
// window size - Full HD, so text/HUD have plenty of native pixels to work
// with even before any fullscreen/letterbox scaling.
const (
	defaultWindowWidth  = int32(1920)
	defaultWindowHeight = int32(1080)
)

func InitGame() *Game {
	g := &Game{}

	g.ScreenWidth = defaultWindowWidth
	g.ScreenHeight = defaultWindowHeight
	g.WindowWidth = defaultWindowWidth
	g.WindowHeight = defaultWindowHeight
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

	g.BoundaryClouds = generateBoundaryClouds()

	g.HighScoreRepo = highscore.NewFileRepository(highScoreFile)
	g.HighScores, _ = g.HighScoreRepo.Load()

	g.Sounds = LoadSounds()
	g.Font = loadReadableFont()

	g.Settings = loadSettings()

	g.BGM = loadBGM()
	rl.PlayMusicStream(g.BGM)

	g.WorldTarget = rl.LoadRenderTexture(g.ScreenWidth/pixelScale, g.ScreenHeight/pixelScale)
	rl.SetTextureFilter(g.WorldTarget.Texture, rl.FilterPoint)

	g.PixelTarget = rl.LoadRenderTexture(g.ScreenWidth, g.ScreenHeight)
	rl.SetTextureFilter(g.PixelTarget.Texture, rl.FilterPoint)

	resetRun(g)
	g.State = TITLE

	return g
}

// generateBoundaryClouds builds an irregular nebula ring around arenaHalf -
// clusters of overlapping soft blobs, jittered in angle/distance/size, so
// the world's edge reads as an organic gas cloud rather than a perfect
// circle or an invisible wall. The actual clamp is still just a circular
// distance check (see updatePlayerMovement) - this is decoration only.
func generateBoundaryClouds() []GasCloud {
	cloudColors := []rl.Color{rl.Fade(colorHaze, 0.12), rl.Fade(colorStructMid, 0.15), rl.Fade(colorAccentDim, 0.08)}

	var clouds []GasCloud
	const clusterCount = 40

	for i := 0; i < clusterCount; i++ {
		baseAngle := float64(i) * 360.0 / clusterCount
		angle := (baseAngle + float64(rl.GetRandomValue(-8, 8))) * rl.Deg2rad
		dist := arenaHalf + float32(rl.GetRandomValue(-800, 800))
		center := rl.NewVector2(float32(math.Cos(angle))*dist, float32(math.Sin(angle))*dist)

		blobCount := int(rl.GetRandomValue(3, 5))
		for b := 0; b < blobCount; b++ {
			offset := rl.NewVector2(float32(rl.GetRandomValue(-150, 150)), float32(rl.GetRandomValue(-150, 150)))
			clouds = append(clouds, GasCloud{
				Position: rl.Vector2Add(center, offset),
				Radius:   float32(rl.GetRandomValue(80, 220)),
				Color:    cloudColors[rl.GetRandomValue(0, int32(len(cloudColors)-1))],
			})
		}
	}

	return clouds
}

// loadReadableFont picks a real, legible system sans-serif over raylib's tiny
// built-in bitmap font (which is what made text unreadable). Loaded at a
// large base size and downsampled at draw time for crisp text at any size.
// Falls back to the default font if none of the common paths exist.
func loadReadableFont() rl.Font {
	candidates := []string{
		"/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
		"/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
		"/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
		"/Library/Fonts/Arial Bold.ttf",
		"C:\\Windows\\Fonts\\arialbd.ttf",
	}

	for _, path := range candidates {
		if _, err := os.Stat(path); err == nil {
			font := rl.LoadFontEx(path, 96, nil)
			if font.Texture.ID != 0 {
				rl.SetTextureFilter(font.Texture, rl.FilterBilinear)
				return font
			}
		}
	}

	return rl.GetFontDefault()
}

// toggleFullscreen switches between windowed and fullscreen. Going fullscreen
// resizes the window to the current monitor's native resolution first (so it
// fills displays up to 8K), then hands off to raylib's fullscreen mode. The
// game's own logical resolution never changes - see syncScreenSize.
func toggleFullscreen(g *Game) {
	if rl.IsWindowFullscreen() {
		rl.ToggleFullscreen()
		rl.SetWindowSize(int(defaultWindowWidth), int(defaultWindowHeight))
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
// The player always starts at the world origin - the camera keeps them
// pinned at screen center regardless (see beginWorldCamera in draw.go).
func resetRun(g *Game) {
	g.Player = Player{
		Position:  rl.Vector2{},
		Radius:    15,
		Color:     colorAccent,
		Speed:     5,
		Health:    5,
		MaxHealth: 5,
		Charges:   maxCharges,
	}

	g.Boss = Boss{Size: rl.NewVector2(100, 100)}
	g.BossActive = false

	g.Bullets = []Bullet{}
	g.Asteroids = []Asteroid{}
	g.Projectiles = []BossProjectile{}
	g.Enemies = []Enemy{}
	g.Pickups = []Pickup{}
	g.Mines = []Mine{}
	g.DeathParticles = []Particle{}
	g.Weapons = []Weapon{{Kind: WeaponForward, Level: 1}}
	g.SkillLevels = map[SkillID]int32{SkillForwardShot: 1}
	g.PostCapDamageLevels = 0
	g.BossDeathShockwave = false
	g.BossDeathShockwaveTimer = 0
	g.BossDeathShockwaveHit = false

	g.BlackHole = BlackHole{Timer: float32(rl.GetRandomValue(30, 60)) / 10.0}
	g.AsteroidSpawnTimer = 1.0
	g.EnemySpawnTimer = 1.0
	g.SpreadWindupShots = 0

	g.XP = 0
	g.Level = 1
	g.XPToNext = 100
	g.WaveNumber = 1
	g.WaveTimer = waveDuration
	g.RunTime = 0

	g.ShakeTimer = 0
	g.ShakeDuration = 0
	g.ShakeIntensity = 0
	g.HitPauseTimer = 0
	g.Score = 0
	g.ScoreRecorded = false
	g.State = GAMEPLAY
}
