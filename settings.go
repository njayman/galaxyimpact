package main

import (
	"fmt"
	"os"
)

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

const settingsFile = "settings.txt"

// loadSettings reads persisted settings (resolution/difficulty/BGM/sound)
// from disk, falling back to defaults if the file is missing or malformed -
// display mode is deliberately not persisted here (the game always launches
// windowed, per earlier design).
func loadSettings() Settings {
	s := Settings{
		ResolutionIndex: defaultResolutionIndex(),
		Difficulty:      DifficultyNormal,
		BGMOn:           true,
		SoundOn:         true,
	}

	data, err := os.ReadFile(settingsFile)
	if err != nil {
		return s
	}

	var resIdx, difficulty, bgmOn, soundOn int32
	if _, err := fmt.Sscanf(string(data), "%d %d %d %d", &resIdx, &difficulty, &bgmOn, &soundOn); err != nil {
		return s
	}

	if resIdx >= 0 && int(resIdx) < len(resolutionOptions) {
		s.ResolutionIndex = resIdx
	}
	if difficulty >= 0 && difficulty < int32(difficultyCount) {
		s.Difficulty = Difficulty(difficulty)
	}
	s.BGMOn = bgmOn != 0
	s.SoundOn = soundOn != 0

	return s
}

func saveSettings(s Settings) {
	bgmOn, soundOn := 0, 0
	if s.BGMOn {
		bgmOn = 1
	}
	if s.SoundOn {
		soundOn = 1
	}

	data := fmt.Sprintf("%d %d %d %d", s.ResolutionIndex, int32(s.Difficulty), bgmOn, soundOn)
	_ = os.WriteFile(settingsFile, []byte(data), 0644)
}
