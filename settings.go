package main

type Difficulty int32

const (
	DifficultyEasy Difficulty = iota
	DifficultyNormal
	DifficultyHard
	difficultyCount
)

type DifficultyDef struct {
	Name            string
	EnemyHealthMult float32
	EnemyDamageMult float32
	SpawnRateMult   float32 // multiplies spawn interval: >1 slower, <1 faster
}

var difficultyDefs = [difficultyCount]DifficultyDef{
	DifficultyEasy:   {"Easy", 0.75, 0.75, 1.3},
	DifficultyNormal: {"Normal", 1.0, 1.0, 1.0},
	DifficultyHard:   {"Hard", 1.35, 1.35, 0.75},
}

// ResolutionOption is a windowed-mode size preset. The game's internal
// design resolution (Game.ScreenWidth/Height) never changes - this only
// resizes the OS window; the existing letterbox scaling handles the rest.
type ResolutionOption struct {
	Width, Height int32
}

var resolutionOptions = []ResolutionOption{
	{1280, 720},
	{1600, 900},
	{1920, 1080},
	{2560, 1440},
	{3840, 2160},
}

// Settings persists for the whole process (not reset per-run by resetRun).
type Settings struct {
	ResolutionIndex int32
	Difficulty      Difficulty
	BGMOn           bool
	SoundOn         bool
}

// defaultResolutionIndex finds the 1920x1080 entry in resolutionOptions so
// the settings menu starts in sync with the actual default window size.
func defaultResolutionIndex() int32 {
	for i, r := range resolutionOptions {
		if r.Width == defaultWindowWidth && r.Height == defaultWindowHeight {
			return int32(i)
		}
	}
	return 0
}
