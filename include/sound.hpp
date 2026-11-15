#pragma once

#include "raylib.h"
#include <array>
#include <cstddef>
#include <cstdint>
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

// Signature-boss music: build-up once, then the biome's action loop; the old boss loop for the
// boss's desperate phase; fades out when it dies. See the player in src/sound.cpp.
enum class BossMusicSegment : std::uint8_t
{
    None,
    Build,
    Action,
    Desperate,
};

struct BossMusic
{
    AudioStream stream{};
    std::array<std::vector<int16_t>, SoundConstants::biomeCount> build;  // in Biome order
    std::array<std::vector<int16_t>, SoundConstants::biomeCount> action; // in Biome order
    std::vector<int16_t> desperate;
    BossMusicSegment segment = BossMusicSegment::None;
    BossMusicSegment pending = BossMusicSegment::None; // takes over at the next beat or bar line
    float fade = 1.0F; // 1 while the fight lasts; ramps to 0 once the boss is gone
    size_t biome = 0;
    size_t pos = 0;
};

auto loadBossMusic() -> BossMusic;
void startBossMusic(BossMusic& music, size_t biome);                  // from the build-up
void requestBossMusic(BossMusic& music, BossMusicSegment next);        // Desperate, in time
void stopBossMusic(BossMusic& music);                                 // at once
void pumpBossMusic(BossMusic& music);                                 // call every frame

struct BgmLayers
{
    Music base{};
    Music intensity{};
    Music miniboss{};
    BossMusic boss;
    Music swarmBoss{};
    float intensityVolume = 0;
    float minibossVolume = 0;
    float bossMix = 0; // 0..1: how far the boss music has taken over from the base layers
    float swarmBossVolume = 0;
    float calmTimer = 0;
    float duckTimer = 0;

    std::vector<std::byte> baseWav;
    std::vector<std::byte> intensityWav;
    std::vector<std::byte> minibossWav;
    std::vector<std::byte> swarmBossWav;
};

auto loadBGM() -> BgmLayers;
