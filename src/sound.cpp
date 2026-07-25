#include "sound.hpp"

#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <vector>

auto LoadSounds() -> Sounds { return Sounds{}; }

namespace
{

constexpr int sampleRate = SoundConstants::audioSampleRate;
constexpr float bpm = 84.0F;
constexpr float beatSeconds = 60.0F / bpm;
constexpr float barSeconds = beatSeconds * 4;
constexpr float loopSeconds = barSeconds * 4; // 4 bars, ~11.4s
constexpr float loopFadeSeconds = 0.05F;      // drone loop-seam crossfade

auto secondsToSamples(float seconds) -> size_t
{
    return static_cast<size_t>(std::lround(seconds * sampleRate));
}

// A short decaying tone - the building block for both the percussive
// intensity pulse and the plucked upgrade arpeggio.
void addTone(std::vector<float>& buf, float startSeconds, float freq, float amp, float decaySeconds)
{
    const size_t start = secondsToSamples(startSeconds);
    for (size_t i = start; i < buf.size(); i++)
    {
        const float t = static_cast<float>(i - start) / sampleRate;
        if (t > decaySeconds * 6)
        {
            break;
        }
        const float env = std::exp(-t / decaySeconds);
        buf.at(i) += amp * env * std::sin(2 * std::numbers::pi_v<float> * freq * t);
    }
}

// Fundamental plus a quiet octave overtone - brighter/more "plucked" than a
// bare sine, used for the upgrade layer's arpeggio.
void addPluck(std::vector<float>& buf, float startSeconds, float freq, float amp,
              float decaySeconds)
{
    addTone(buf, startSeconds, freq, amp, decaySeconds);
    addTone(buf, startSeconds, freq * 2, amp * 0.35F, decaySeconds * 0.6F);
}

// drone: a slow, warm A-root chord deep in the sub-bass - root, a true
// octave, and a detuned unison for a gentle chorus - plus one quiet,
// higher-register dissonant color tone for the "eerie" edge, and a faint
// low-passed noise bed for "the hum of deep space". Always audible.
//
// The root/octave/detune voices are all within ~1Hz or an exact 2x ratio of
// each other, so they reinforce instead of beating (harmonious); the color
// tone sits nearly two octaves higher and quiet, staying an accent rather
// than colliding with the bass. Earlier this stacked three sine tones only
// 10-12Hz apart in the sub-bass (root/minor third/tritone) - that's squarely
// in the psychoacoustic "critical band roughness" zone for pure sines at
// that register, so it read as harsh buzzing rather than a chord. The loop
// point is crossfaded (folding the natural continuation into the head) so
// the sustained tone has no audible seam click.
auto generateDrone(size_t loopSamples) -> std::vector<float>
{
    struct Voice
    {
        float freq;
        float amp;
        float tremoloHz;
    };
    constexpr std::array<Voice, 4> voices{
        Voice{.freq = 55.00F, .amp = 0.45F, .tremoloHz = 0.07F},  // A1 - root
        Voice{.freq = 55.15F, .amp = 0.28F, .tremoloHz = 0.09F},  // detuned unison - slow chorus
        Voice{.freq = 110.00F, .amp = 0.22F, .tremoloHz = 0.05F}, // A2 - true octave, zero beat
        Voice{
            .freq = 233.08F, .amp = 0.05F, .tremoloHz = 0.15F}, // Bb3 - quiet, distant, the "eerie"
    };

    const size_t fadeSamples = secondsToSamples(loopFadeSeconds);
    const size_t genSamples = loopSamples + fadeSamples;

    std::vector<float> raw(genSamples, 0.0F);
    for (const auto& voice : voices)
    {
        for (size_t i = 0; i < genSamples; i++)
        {
            const float t = static_cast<float>(i) / sampleRate;
            const float tremolo =
                0.8F + 0.2F * std::sin(2 * std::numbers::pi_v<float> * voice.tremoloHz * t);
            raw.at(i) +=
                voice.amp * tremolo * std::sin(2 * std::numbers::pi_v<float> * voice.freq * t);
        }
    }

    float noiseState = 0;
    for (size_t i = 0; i < genSamples; i++)
    {
        const float white = static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0F;
        noiseState += 0.002F * (white - noiseState);
        raw.at(i) += noiseState * 0.06F;
    }

    std::vector<float> out(raw.begin(), raw.begin() + static_cast<std::ptrdiff_t>(loopSamples));
    for (size_t i = 0; i < fadeSamples; i++)
    {
        const float w = static_cast<float>(i) / static_cast<float>(fadeSamples);
        out.at(i) = out.at(i) * (1 - w) + raw.at(loopSamples + i) * w;
    }

    return out;
}

// intensity: a low pulse on the perfect fifth above the drone's root (E2 -
// the most consonant non-octave interval, so it locks in with the drone
// instead of beating against it) - a steady beat with a syncopated push,
// "more beat with the base beat" as its volume fades in.
auto generateIntensity(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);

    constexpr int barCount = 4;
    for (int bar = 0; bar < barCount; bar++)
    {
        const float barStart = static_cast<float>(bar) * barSeconds;
        addTone(out, barStart + 0.0F * beatSeconds, 82.41F, 0.5F, 0.18F);
        addTone(out, barStart + 1.5F * beatSeconds, 82.41F, 0.22F, 0.12F);
        addTone(out, barStart + 2.0F * beatSeconds, 82.41F, 0.5F, 0.18F);
        addTone(out, barStart + 3.5F * beatSeconds, 82.41F, 0.22F, 0.12F);
    }

    return out;
}

// upgrade: a bright plucked arpeggio built on the drone's own root/fifth/
// octave (A/E/A), two-plus octaves up, with one passing major-third color
// tone (C#) for character - consonant intervals at a register and note
// length short enough that a color tone doesn't turn into sustained beating.
// Same harmonic DNA as the drone, just elevated, so it reads as a reward
// layered on top rather than a clashing new tune.
auto generateUpgrade(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);

    constexpr std::array<float, 4> arpeggio{220.00F, 277.18F, 329.63F, 440.00F}; // A3 C#4 E4 A4
    const auto stepCount = static_cast<int>((loopSeconds - beatSeconds) / (beatSeconds * 0.5F));
    for (int step = 0; step < stepCount; step++)
    {
        const float t = static_cast<float>(step) * beatSeconds * 0.5F;
        addPluck(out, t, arpeggio.at(static_cast<size_t>(step) % arpeggio.size()), 0.16F, 0.3F);
    }

    return out;
}

auto toPcm16(const std::vector<float>& samples) -> std::vector<int16_t>
{
    std::vector<int16_t> pcm(samples.size());
    for (size_t i = 0; i < samples.size(); i++)
    {
        const float clamped = std::clamp(samples.at(i), -1.0F, 1.0F);
        pcm.at(i) = static_cast<int16_t>(clamped * 32000);
    }
    return pcm;
}

// Encodes 16-bit mono PCM as a standard RIFF/WAVE byte buffer, suitable for
// LoadMusicStreamFromMemory(".wav", ...).
auto encodeWAV(const std::vector<int16_t>& samples) -> std::vector<std::byte>
{
    constexpr uint16_t channels = 1;
    constexpr uint16_t bitsPerSample = 16;
    const auto dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    constexpr uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
    constexpr uint16_t blockAlign = channels * bitsPerSample / 8;

    std::vector<std::byte> buf;
    buf.reserve(44 + dataSize);

    const auto writeBytes = [&buf](const void* data, size_t n)
    {
        const auto* p = static_cast<const std::byte*>(data);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        buf.insert(buf.end(), p, p + n);
    };
    const auto writeTag = [&](const char* s) { writeBytes(s, 4); };
    const auto writeU32 = [&](uint32_t v) { writeBytes(&v, sizeof(v)); };
    const auto writeU16 = [&](uint16_t v) { writeBytes(&v, sizeof(v)); };

    writeTag("RIFF");
    writeU32(36 + dataSize);
    writeTag("WAVE");
    writeTag("fmt ");
    writeU32(16);
    writeU16(1); // PCM
    writeU16(channels);
    writeU32(sampleRate);
    writeU32(byteRate);
    writeU16(blockAlign);
    writeU16(bitsPerSample);
    writeTag("data");
    writeU32(dataSize);
    writeBytes(samples.data(), dataSize);

    return buf;
}

auto loadMusicFromSamples(const std::vector<float>& samples) -> Music
{
    const std::vector<std::byte> wav = encodeWAV(toPcm16(samples));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* wavData = reinterpret_cast<const unsigned char*>(wav.data());
    Music music = LoadMusicStreamFromMemory(".wav", wavData, static_cast<int>(wav.size()));
    music.looping = true;
    return music;
}

} // namespace

auto loadBGM() -> BgmLayers
{
    const size_t loopSamples = secondsToSamples(loopSeconds);

    BgmLayers bgm{};
    bgm.drone = loadMusicFromSamples(generateDrone(loopSamples));
    bgm.intensity = loadMusicFromSamples(generateIntensity(loopSamples));
    bgm.upgrade = loadMusicFromSamples(generateUpgrade(loopSamples));
    return bgm;
}
