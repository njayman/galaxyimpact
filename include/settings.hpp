#pragma once

#include <array>
#include <cstdint>

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

constexpr std::array<int32_t, 3> fpsOptions{60, 120, 240};

constexpr std::array<float, 4> hudScaleOptions{1.0F, 1.25F, 1.5F, 1.75F};
constexpr int32_t defaultHudScaleIndex = 1;

struct Settings
{
    int32_t resolutionIndex{};
    bool bgmOn{true};
    bool soundOn{true};
    int32_t fpsIndex{};
    int32_t hudScaleIndex{defaultHudScaleIndex};
    int32_t shipIndex{1};
};

auto loadSettings() -> Settings;
void saveSettings(const Settings& settings);
