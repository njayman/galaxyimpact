#pragma once

#include "raylib.h"
#include <array>
#include <cstddef>
#include <vector>
namespace SoundConstants
{
constexpr int audioSampleRate = 22050;

constexpr float baseVolume = 0.6F;
constexpr size_t biomeCount = 4;
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
    Sound nerveCharge;
    Sound nerveRelease;
    Sound nerveFizzle;
    Sound rockBreak;
    Sound squishHit;
    Sound thunder;
    Sound enemyDeath;
    Sound pickup;
    Sound heal;
    Sound powerUp;
    Sound debuff;
};

auto LoadSounds() -> Sounds;

struct BgmLayers
{
    Music base{};
    Music intensity{};
    Music miniboss{};
    std::array<Music, SoundConstants::biomeCount> megaboss{}; // one full boss track per biome, in Biome order
    Music swarmBoss{};
    float intensityVolume = 0;
    float minibossVolume = 0;
    float megabossVolume = 0;
    float swarmBossVolume = 0;
    float calmTimer = 0;
    float duckTimer = 0;

    std::vector<std::byte> baseWav;
    std::vector<std::byte> intensityWav;
    std::vector<std::byte> minibossWav;
    std::array<std::vector<std::byte>, SoundConstants::biomeCount> megabossWav;
    std::vector<std::byte> swarmBossWav;
};

auto loadBGM() -> BgmLayers;
