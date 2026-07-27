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
constexpr float loopSeconds = barSeconds * 4;
constexpr float loopFadeSeconds = 0.05F;

auto secondsToSamples(float seconds) -> size_t
{
    return static_cast<size_t>(std::lround(seconds * sampleRate));
}

constexpr float toneCutoffDecays = 6.0F;

constexpr float pluckOvertoneAmpRatio = 0.35F;
constexpr float pluckOvertoneDecayRatio = 0.6F;

void addTone(std::vector<float>& buf, float startSeconds, float freq, float amp, float decaySeconds)
{
    const size_t start = secondsToSamples(startSeconds);
    for (size_t i = start; i < buf.size(); i++)
    {
        const float elapsed = static_cast<float>(i - start) / sampleRate;
        if (elapsed > decaySeconds * toneCutoffDecays)
        {
            break;
        }
        const float env = std::exp(-elapsed / decaySeconds);
        buf.at(i) += amp * env * std::sin(2 * std::numbers::pi_v<float> * freq * elapsed);
    }
}

void addPluck(std::vector<float>& buf, float startSeconds, float freq, float amp,
              float decaySeconds)
{
    addTone(buf, startSeconds, freq, amp, decaySeconds);
    addTone(buf, startSeconds, freq * 2, amp * pluckOvertoneAmpRatio,
            decaySeconds * pluckOvertoneDecayRatio);
}

auto generateDrone(size_t loopSamples) -> std::vector<float>
{
    struct Voice
    {
        float freq;
        float amp;
        float tremoloHz;
    };
    constexpr std::array<Voice, 4> voices{
        Voice{.freq = 220.00F, .amp = 0.45F, .tremoloHz = 0.07F},
        Voice{.freq = 220.15F, .amp = 0.28F, .tremoloHz = 0.09F},
        Voice{.freq = 440.00F, .amp = 0.22F, .tremoloHz = 0.05F},
        Voice{.freq = 932.32F,
              .amp = 0.05F,
              .tremoloHz = 0.15F},
    };

    const size_t fadeSamples = secondsToSamples(loopFadeSeconds);
    const size_t genSamples = loopSamples + fadeSamples;

    std::vector<float> raw(genSamples, 0.0F);
    for (const auto& voice : voices)
    {
        for (size_t i = 0; i < genSamples; i++)
        {
            const float time = static_cast<float>(i) / sampleRate;
            const float tremolo =
                0.8F + (0.2F * std::sin(2 * std::numbers::pi_v<float> * voice.tremoloHz * time));
            raw.at(i) +=
                voice.amp * tremolo * std::sin(2 * std::numbers::pi_v<float> * voice.freq * time);
        }
    }

    std::vector<float> out(raw.begin(), raw.begin() + static_cast<std::ptrdiff_t>(loopSamples));
    for (size_t i = 0; i < fadeSamples; i++)
    {
        const float fadeWeight = static_cast<float>(i) / static_cast<float>(fadeSamples);
        out.at(i) = (out.at(i) * (1 - fadeWeight)) + (raw.at(loopSamples + i) * fadeWeight);
    }

    return out;
}

auto generateIntensity(size_t loopSamples) -> std::vector<float>
{
    constexpr float intensityFreq = 329.63F;
    constexpr float strongBeatAmp = 0.5F;
    constexpr float strongBeatDecay = 0.18F;
    constexpr float weakBeatAmp = 0.22F;
    constexpr float weakBeatDecay = 0.12F;

    struct Beat
    {
        float beatOffset;
        float amp;
        float decaySeconds;
    };

    constexpr std::array<Beat, 4> pattern{
        Beat{.beatOffset = 0.0F, .amp = strongBeatAmp, .decaySeconds = strongBeatDecay},
        Beat{.beatOffset = 1.5F, .amp = weakBeatAmp, .decaySeconds = weakBeatDecay},
        Beat{.beatOffset = 2.0F, .amp = strongBeatAmp, .decaySeconds = strongBeatDecay},
        Beat{.beatOffset = 3.5F, .amp = weakBeatAmp, .decaySeconds = weakBeatDecay},
    };

    std::vector<float> out(loopSamples, 0.0F);

    constexpr int barCount = 4;
    for (int bar = 0; bar < barCount; bar++)
    {
        const float barStart = static_cast<float>(bar) * barSeconds;
        for (const auto& beat : pattern)
        {
            addTone(out, barStart + (beat.beatOffset * beatSeconds), intensityFreq, beat.amp,
                    beat.decaySeconds);
        }
    }

    return out;
}

auto generateUpgrade(size_t loopSamples) -> std::vector<float>
{
    constexpr float arpeggioStepBeats = 0.5F;
    constexpr float upgradeNoteAmp = 0.16F;
    constexpr float upgradeNoteDecay = 0.3F;

    std::vector<float> out(loopSamples, 0.0F);

    constexpr std::array<float, 4> arpeggio{220.00F, 277.18F, 329.63F, 440.00F};
    const auto stepCount =
        static_cast<int>((loopSeconds - beatSeconds) / (beatSeconds * arpeggioStepBeats));
    for (int step = 0; step < stepCount; step++)
    {
        const float time = static_cast<float>(step) * beatSeconds * arpeggioStepBeats;
        addPluck(out, time, arpeggio.at(static_cast<size_t>(step) % arpeggio.size()),
                 upgradeNoteAmp, upgradeNoteDecay);
    }

    return out;
}

auto toPcm16(const std::vector<float>& samples) -> std::vector<int16_t>
{

    constexpr float pcmScale = 32000.0F;

    float peak = 0.0F;
    for (const float sample : samples)
    {
        peak = std::max(peak, std::abs(sample));
    }
    const float gain = peak > 1.0F ? 1.0F / peak : 1.0F;

    std::vector<int16_t> pcm(samples.size());
    for (size_t i = 0; i < samples.size(); i++)
    {
        pcm.at(i) = static_cast<int16_t>(samples.at(i) * gain * pcmScale);
    }
    return pcm;
}

auto encodeWAV(const std::vector<int16_t>& samples) -> std::vector<std::byte>
{
    constexpr uint16_t channels = 1;
    constexpr uint16_t bitsPerSample = 16;
    constexpr uint16_t pcmFormatTag = 1;
    constexpr uint32_t fmtChunkSize = 16;
    constexpr uint32_t wavHeaderSize = 44;

    constexpr uint32_t riffChunkSizeBase = wavHeaderSize - 8;
    constexpr size_t tagLength = 4;

    const auto dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    constexpr uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
    constexpr uint16_t blockAlign = channels * bitsPerSample / 8;

    std::vector<std::byte> buf;
    buf.reserve(wavHeaderSize + dataSize);

    const auto writeBytes = [&buf](const void* data, size_t byteCount) -> void
    {
        const auto* bytePtr = static_cast<const std::byte*>(data);

        buf.insert(buf.end(), bytePtr, bytePtr + byteCount);
    };
    const auto writeTag = [&](const char* tag) -> void { writeBytes(tag, tagLength); };
    const auto writeU32 = [&](uint32_t value) -> void { writeBytes(&value, sizeof(value)); };
    const auto writeU16 = [&](uint16_t value) -> void { writeBytes(&value, sizeof(value)); };

    writeTag("RIFF");
    writeU32(riffChunkSizeBase + dataSize);
    writeTag("WAVE");
    writeTag("fmt ");
    writeU32(fmtChunkSize);
    writeU16(pcmFormatTag);
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

auto loadMusicFromSamples(std::vector<std::byte>& wavStorage,
                          const std::vector<float>& samples) -> Music
{
    wavStorage = encodeWAV(toPcm16(samples));

    const auto* wavData = reinterpret_cast<const unsigned char*>(wavStorage.data());
    Music music = LoadMusicStreamFromMemory(".wav", wavData, static_cast<int>(wavStorage.size()));
    music.looping = true;
    return music;
}

}

auto loadBGM() -> BgmLayers
{
    const size_t loopSamples = secondsToSamples(loopSeconds);

    BgmLayers bgm{};
    bgm.drone = loadMusicFromSamples(bgm.droneWav, generateDrone(loopSamples));
    bgm.intensity = loadMusicFromSamples(bgm.intensityWav, generateIntensity(loopSamples));
    bgm.upgrade = loadMusicFromSamples(bgm.upgradeWav, generateUpgrade(loopSamples));
    return bgm;
}
