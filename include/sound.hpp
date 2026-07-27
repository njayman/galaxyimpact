#pragma once

#include "raylib.h"
#include <cstddef>
#include <vector>
namespace SoundConstants
{
constexpr int audioSampleRate = 22050;

constexpr float droneVolume = 0.0F;
}

struct Sounds
{
    Sound shoot;
    Sound hit;
    Sound explosion;
    Sound menuMove;
    Sound menuConfirm;
    Sound victory;
    Sound defeat;
    Sound critical;
    Sound bossWindUp;
    Sound beamFire;
    Sound homingLaunch;
    Sound spreadBurst;
    Sound slamBoom;
};

auto LoadSounds() -> Sounds;

struct BgmLayers
{
    Music drone{};
    Music intensity{};
    Music upgrade{};
    float intensityVolume = 0;
    float upgradeVolume = 0;
    float calmTimer =
        0;

    std::vector<std::byte> droneWav;
    std::vector<std::byte> intensityWav;
    std::vector<std::byte> upgradeWav;
};

auto loadBGM() -> BgmLayers;
