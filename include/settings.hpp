#pragma once

#include <array>
#include <cstdint>
#include <string_view>

enum class Difficulty : std::uint8_t
{
    Easy,
    Normal,
    Hard,

    Count
};

struct DifficultyDef
{
    std::string_view name;
    float enemyHealthMult;
    float enemyDamageMult;
    float spawnRateMult; // multiplies spawn interval: >1 slower, <1 faster
};

constexpr std::array<DifficultyDef, static_cast<size_t>(Difficulty::Count)> difficultyDefs{
    DifficultyDef{
        .name = "Easy", .enemyHealthMult = 0.75, .enemyDamageMult = 0.75, .spawnRateMult = 1.3},
    DifficultyDef{
        .name = "Normal", .enemyHealthMult = 1.0, .enemyDamageMult = 1.0, .spawnRateMult = 1.0},
    DifficultyDef{
        .name = "Hard", .enemyHealthMult = 1.35, .enemyDamageMult = 1.35, .spawnRateMult = 0.75},
};

// ResolutionOption is a windowed-mode size preset. The game's internal design
// resolution never changes - this only resizes the OS window.
struct ResolutionOption
{
    int32_t width;
    int32_t height;
};

constexpr std::array<ResolutionOption, 5> resolutionOptions{
    ResolutionOption{.width = 1280, .height = 720},
    ResolutionOption{.width = 1600, .height = 900},
    ResolutionOption{.width = 1920, .height = 1080},
    ResolutionOption{.width = 2560, .height = 1440},
    ResolutionOption{.width = 3840, .height = 2160},
};

// fpsOptions are the selectable target frame rate caps. All gameplay logic
// is deltaTime-scaled (not frame-counted), so raising the cap is purely a
// smoothness/input-latency choice, not a simulation-speed one.
constexpr std::array<int32_t, 3> fpsOptions{60, 120, 240};

// Settings persists for the whole process (not reset per-run).
struct Settings
{
    int32_t resolutionIndex{};
    Difficulty difficulty{};
    bool bgmOn{};
    bool soundOn{};
    int32_t fpsIndex{};
};

// loadSettings reads persisted settings (resolution/difficulty/BGM/sound)
// from disk, falling back to defaults if the file is missing or malformed -
// display mode is deliberately not persisted here (the game always launches
// windowed, per earlier design).
auto loadSettings() -> Settings;
void saveSettings(const Settings& settings);
