#pragma once

#include "raylib.h"
namespace SoundConstants
{
constexpr int audioSampleRate = 22050;
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

// BgmLayers is the adaptive background score: three synced, looping streams
// mixed by volume alone so they always stay harmonically/rhythmically locked.
// drone is always audible; intensity fades in with enemy pressure; upgrade
// fades in (and stays) once the player has their first weapon upgrade.
struct BgmLayers
{
    Music drone;
    Music intensity;
    Music upgrade;
    float intensityVolume = 0;
    float upgradeVolume = 0;
    float calmTimer =
        0; // >0 right after a boss kill: caps intensity until it decays or enemies swarm back
};

auto loadBGM() -> BgmLayers;
