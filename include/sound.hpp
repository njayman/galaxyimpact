#pragma once

#include "raylib.h"
#include <cstddef>
#include <vector>
namespace SoundConstants
{
constexpr int audioSampleRate = 22050;

constexpr float baseVolume = 0.6F;
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
    Music base{};
    Music intensity{};
    Music miniboss{};
    Music megaboss{};
    Music swarmBoss{};
    float intensityVolume = 0;
    float minibossVolume = 0;
    float megabossVolume = 0;
    float swarmBossVolume = 0;
    float calmTimer = 0;

    std::vector<std::byte> baseWav;
    std::vector<std::byte> intensityWav;
    std::vector<std::byte> minibossWav;
    std::vector<std::byte> megabossWav;
    std::vector<std::byte> swarmBossWav;
};

auto loadBGM() -> BgmLayers;
