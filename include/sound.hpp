#pragma once

#include "raylib.h"
#include <cstddef>
#include <vector>
namespace SoundConstants
{
constexpr int audioSampleRate = 22050;

// The drone plays continuously at this fixed volume - unlike
// intensity/upgrade (which ramp between 0 and 1, see updateBgmLayers), it
// has no quiet state. Muted: the sustained held chord read as a constant
// background tone competing with the intensity layer's actual rhythm, which
// is what the score should be heard as.
constexpr float droneVolume = 0.0F;
} // namespace SoundConstants

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
    Music drone{};
    Music intensity{};
    Music upgrade{};
    float intensityVolume = 0;
    float upgradeVolume = 0;
    float calmTimer =
        0; // >0 right after a boss kill: caps intensity until it decays or enemies swarm back

    // raylib's WAV decoder (dr_wav) doesn't copy the buffer passed to
    // LoadMusicStreamFromMemory - it keeps reading from it on every
    // UpdateMusicStream call for as long as the stream plays. These own the
    // encoded bytes for exactly that long (see loadMusicFromSamples in
    // sound.cpp); freeing them earlier is a use-after-free that reads
    // garbage/reused heap memory as audio, heard as persistent static.
    std::vector<std::byte> droneWav;
    std::vector<std::byte> intensityWav;
    std::vector<std::byte> upgradeWav;
};

auto loadBGM() -> BgmLayers;
