#include "sound.hpp"

#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <numbers>
#include <vector>

namespace
{

constexpr int sampleRate = SoundConstants::audioSampleRate;
constexpr float bpm = 150.0F;
constexpr float beatSeconds = 60.0F / bpm;
constexpr float barSeconds = beatSeconds * 4;
constexpr float loopSeconds = barSeconds * 4;

auto secondsToSamples(float seconds) -> size_t
{
    return static_cast<size_t>(std::lround(seconds * sampleRate));
}

constexpr float toneCutoffDecays = 6.0F;

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

void addSquare(std::vector<float>& buf, float startSeconds, float freq, float amp,
               float decaySeconds)
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
        const float square =
            std::sin(2 * std::numbers::pi_v<float> * freq * elapsed) >= 0 ? 1.0F : -1.0F;
        buf.at(i) += amp * env * square;
    }
}

void addSaw(std::vector<float>& buf, float startSeconds, float freq, float amp, float decaySeconds)
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
        const float saw = 2.0F * (freq * elapsed - std::floor(freq * elapsed)) - 1.0F;
        buf.at(i) += amp * env * saw;
    }
}

void addNoise(std::vector<float>& buf, float startSeconds, float amp, float decaySeconds)
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
        buf.at(i) += amp * env *
                     (2.0F * static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 1.0F);
    }
}

void addKick(std::vector<float>& buf, float t0, float amp, float freq, float endFreq,
             float sweepDur, float decay)
{
    const size_t start = secondsToSamples(t0);
    for (size_t i = start; i < buf.size(); i++)
    {
        const float elapsed = static_cast<float>(i - start) / sampleRate;
        if (elapsed > decay * toneCutoffDecays)
        {
            break;
        }
        const float env = std::exp(-elapsed / decay);
        const float sweep = freq - (freq - endFreq) * std::min(elapsed / sweepDur, 1.0F);
        buf.at(i) += amp * env * std::sin(2 * std::numbers::pi_v<float> * sweep * elapsed);
    }
}

auto generateBase(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);

    for (size_t i = 0; i < loopSamples; i++)
    {
        const float t = static_cast<float>(i) / sampleRate;
        out.at(i) += 0.05F * std::sin(2 * std::numbers::pi_v<float> * 220.00F * t);
        out.at(i) += 0.04F * std::sin(2 * std::numbers::pi_v<float> * 220.22F * t);
        out.at(i) += 0.03F * std::sin(2 * std::numbers::pi_v<float> * 329.63F * t);
    }

    constexpr std::array<float, 8> motif{440.00F, 523.25F, 587.33F, 523.25F,
                                         392.00F, 440.00F, 523.25F, 440.00F};

    for (int bar = 0; bar < 4; bar++)
    {
        const float b = static_cast<float>(bar) * barSeconds;

        for (int n = 0; n < 8; n++)
        {
            const float t = b + static_cast<float>(n) * beatSeconds * 0.5F;
            const float f = motif.at(static_cast<size_t>(n) % motif.size());
            addTone(out, t, f, 0.05F, 0.15F);
            addTone(out, t + 0.03F, f * 1.01F, 0.02F, 0.10F);
        }

        addTone(out, b, 55.0F, 0.10F, 0.40F);
        addTone(out, b + 2 * beatSeconds, 55.0F, 0.08F, 0.35F);
    }

    return out;
}

auto generateIntensity(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);

    constexpr std::array<float, 8> melody{523.25F, 659.25F, 783.99F, 659.25F,
                                          587.33F, 523.25F, 440.00F, 523.25F};
    for (int bar = 0; bar < 4; bar++)
    {
        const float b = static_cast<float>(bar) * barSeconds;
        for (int n = 0; n < 8; n++)
        {
            const float t = b + static_cast<float>(n) * beatSeconds * 0.5F;
            const float f = melody.at(static_cast<size_t>(n) % melody.size());
            addTone(out, t, f, 0.07F, 0.18F);
            addTone(out, t + 0.12F, f * 1.008F, 0.03F, 0.12F);
        }

        for (int eighth = 0; eighth < 8; eighth++)
        {
            addNoise(out, b + static_cast<float>(eighth) * beatSeconds * 0.5F, 0.05F, 0.02F);
        }
    }

    return out;
}

auto generateMiniboss(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);
    constexpr std::array<float, 4> roots{220.00F, 220.00F, 174.61F, 174.61F};
    constexpr std::array<float, 8> arpRatio{1.0F, 1.2F, 1.5F, 2.0F, 2.4F, 2.0F, 1.5F, 1.2F};
    constexpr std::array<float, 4> stabBeat{0.0F, 0.5F, 1.5F, 2.5F};
    constexpr std::array<float, 4> stringRatio{1.2F, 1.35F, 1.2F, 1.5F};

    for (int bar = 0; bar < 4; bar++)
    {
        const float b = static_cast<float>(bar) * barSeconds;
        const float r = roots.at(static_cast<size_t>(bar));
        for (int beat = 0; beat < 4; beat++)
        {
            addTone(out, b + static_cast<float>(beat) * beatSeconds,
                    r * stringRatio.at(static_cast<size_t>(beat)), 0.06F, beatSeconds * 0.95F);
        }
        addKick(out, b, 0.30F, 150, 45, 0.08F, 0.12F);
        addKick(out, b + 2 * beatSeconds + beatSeconds * (1.0F / 3.0F), 0.24F, 150, 40, 0.06F,
                0.09F);
        addKick(out, b + 2 * beatSeconds + beatSeconds * 0.5F, 0.28F, 150, 40, 0.06F, 0.09F);
        addNoise(out, b + 1 * beatSeconds, 0.34F, 0.07F);
        addNoise(out, b + 3 * beatSeconds, 0.34F, 0.07F);
        for (int beat = 0; beat < 4; beat++)
        {
            const float bt = b + static_cast<float>(beat) * beatSeconds;
            addNoise(out, bt + beatSeconds * (1.0F / 3.0F), 0.08F, 0.02F);
            addNoise(out, bt + beatSeconds * 0.5F, 0.11F, 0.025F);
            addNoise(out, bt + beatSeconds * (5.0F / 6.0F), 0.08F, 0.02F);
            addSaw(out, bt, r * 0.25F, 0.20F, beatSeconds * 0.9F);
            addSaw(out, bt + beatSeconds * 0.5F, r * 0.5F, 0.13F, beatSeconds * 0.2F);
            addSaw(out, bt + beatSeconds * 0.75F, r * 0.5F, 0.12F, beatSeconds * 0.2F);
        }
        for (int n = 0; n < 8; n++)
        {
            const float t = b + static_cast<float>(n) * beatSeconds * 0.25F;
            const float ratio = arpRatio.at(static_cast<size_t>(n));
            addSquare(out, t, r * ratio, 0.09F, 0.14F);
            addSquare(out, t, r * ratio * 2.0F, 0.04F, 0.10F);
        }
        for (float sb : stabBeat)
        {
            const float st = b + sb * beatSeconds;
            addTone(out, st, r, 0.09F, 0.22F);
            addTone(out, st, r * 1.2F, 0.07F, 0.22F);
            addTone(out, st, r * 1.5F, 0.07F, 0.22F);
        }
    }
    return out;
}

auto generateMegaboss(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);
    constexpr std::array<float, 8> chordRoot{174.61F, 246.94F, 164.81F, 293.66F,
                                             174.61F, 293.66F, 164.81F, 164.81F};
    constexpr std::array<float, 8> chordThird{1.25F, 1.2F, 1.25F, 1.2F, 1.25F, 1.2F, 1.25F, 1.25F};
    constexpr std::array<float, 8> chordFifth{1.5F, 1.4F, 1.5F, 1.5F, 1.5F, 1.5F, 1.5F, 1.5F};

    for (int bar = 0; bar < 4; bar++)
    {
        const float b = static_cast<float>(bar) * barSeconds;
        addKick(out, b, 0.30F, 150, 45, 0.08F, 0.12F);
        addKick(out, b + 2 * beatSeconds + beatSeconds * (1.0F / 3.0F), 0.24F, 150, 40, 0.06F,
                0.09F);
        addKick(out, b + 2 * beatSeconds + beatSeconds * 0.5F, 0.28F, 150, 40, 0.06F, 0.09F);
        addNoise(out, b + 1 * beatSeconds, 0.34F, 0.07F);
        addNoise(out, b + 3 * beatSeconds, 0.34F, 0.07F);
        for (int slot = 0; slot < 2; slot++)
        {
            const auto idx = static_cast<size_t>(bar) * 2 + static_cast<size_t>(slot);
            const float r = chordRoot.at(idx);
            const float third = chordThird.at(idx);
            const float fifth = chordFifth.at(idx);
            const float sb = b + static_cast<float>(slot) * 2 * beatSeconds;
            const std::array<float, 8> arch{1.0F,         third, fifth, 2.0F,
                                            2.0F * third, 2.0F,  fifth, third};
            for (int beat = 0; beat < 2; beat++)
            {
                const float bt = sb + static_cast<float>(beat) * beatSeconds;
                addNoise(out, bt + beatSeconds * (1.0F / 3.0F), 0.08F, 0.02F);
                addNoise(out, bt + beatSeconds * 0.5F, 0.11F, 0.025F);
                addNoise(out, bt + beatSeconds * (5.0F / 6.0F), 0.08F, 0.02F);
                addSaw(out, bt, r * 0.25F, 0.20F, beatSeconds * 0.9F);
                addSaw(out, bt + beatSeconds * 0.5F, r * 0.5F, 0.13F, beatSeconds * 0.2F);
                addSaw(out, bt + beatSeconds * 0.75F, r * 0.5F, 0.12F, beatSeconds * 0.2F);
            }
            for (int n = 0; n < 8; n++)
            {
                const float t = sb + static_cast<float>(n) * beatSeconds * 0.25F;
                const float ratio = arch.at(static_cast<size_t>(n));
                addSquare(out, t, r * ratio, 0.09F, 0.14F);
                addSquare(out, t, r * ratio * 2.0F, 0.04F, 0.10F);
            }
            addTone(out, sb, r, 0.09F, 0.26F);
            addTone(out, sb, r * third, 0.07F, 0.26F);
            addTone(out, sb, r * fifth, 0.07F, 0.26F);
            addTone(out, sb + beatSeconds, r, 0.08F, 0.22F);
            addTone(out, sb + beatSeconds, r * third, 0.06F, 0.22F);
            addTone(out, sb + beatSeconds, r * fifth, 0.06F, 0.22F);
        }
    }
    return out;
}

auto generateSwarmBoss(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);
    constexpr std::array<float, 4> chordRoot{110.00F, 116.54F, 110.00F, 155.56F};

    addTone(out, 0.0F, 32.7F, 0.16F, loopSeconds * 1.05F);

    for (int bar = 0; bar < 4; bar++)
    {
        const float b = static_cast<float>(bar) * barSeconds;
        const float r = chordRoot.at(static_cast<size_t>(bar));
        addTone(out, b, r * 0.25F, 0.12F, barSeconds * 1.05F);
        addTone(out, b, r * 0.5F, 0.16F, barSeconds * 1.05F);
        addTone(out, b, r * 0.5F * 1.2F, 0.10F, barSeconds * 1.05F);
        addTone(out, b, r * 0.5F * std::numbers::sqrt2_v<float>, 0.09F, barSeconds * 1.05F);
        addTone(out, b, r, 0.08F, barSeconds * 1.05F);
        addTone(out, b, r * 1.2F, 0.05F, barSeconds * 1.05F);
        addTone(out, b, r * std::numbers::sqrt2_v<float>, 0.05F, barSeconds * 1.05F);
        addTone(out, b, r * 1.6819F, 0.04F, barSeconds * 1.05F);

        for (int beat = 0; beat < 4; beat++)
        {
            const float t = b + static_cast<float>(beat) * beatSeconds;
            const float trem = (beat % 2 == 0) ? 0.05F : 0.03F;
            addSquare(out, t, r * 0.5F * std::numbers::sqrt2_v<float>, trem, 0.9F);
        }

        constexpr float voiceDetune1 = 1.004F;
        constexpr float voiceDetune2 = 0.996F;
        for (float ratio : {1.0F, 1.2F, std::numbers::sqrt2_v<float>})
        {
            const float voiceFreq = r * ratio * 2.0F;
            addTone(out, b, voiceFreq, 0.06F, barSeconds * 1.0F);
            addTone(out, b, voiceFreq * voiceDetune1, 0.05F, barSeconds * 1.0F);
            addTone(out, b, voiceFreq * voiceDetune2, 0.05F, barSeconds * 1.0F);
        }
    }

    constexpr std::array<float, 16> dirge{220.00F, 207.65F, 196.00F, 185.00F, 174.61F, 164.81F,
                                          155.56F, 146.83F, 138.59F, 130.81F, 123.47F, 116.54F,
                                          110.00F, 103.83F, 98.00F,  92.50F};
    for (int n = 0; n < 16; n++)
    {
        addSquare(out, static_cast<float>(n) * beatSeconds, dirge.at(static_cast<size_t>(n)), 0.10F,
                  beatSeconds * 0.9F);
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

constexpr float defaultSfxVolume = 0.35F;

auto loadSoundFromSamples(const std::vector<float>& samples,
                          float volume = defaultSfxVolume) -> Sound
{
    const std::vector<std::byte> wavBytes = encodeWAV(toPcm16(samples));
    const auto* wavData = reinterpret_cast<const unsigned char*>(wavBytes.data());
    Wave wave = LoadWaveFromMemory(".wav", wavData, static_cast<int>(wavBytes.size()));
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    SetSoundVolume(sound, volume);
    return sound;
}

auto generateShootSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.10F), 0.0F);
    addSquare(out, 0.0F, 900.0F, 0.14F, 0.04F);
    addSquare(out, 0.0F, 1400.0F, 0.06F, 0.025F);
    return out;
}

auto generateHitSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.15F), 0.0F);
    addNoise(out, 0.0F, 0.35F, 0.05F);
    addTone(out, 0.0F, 180.0F, 0.2F, 0.08F);
    return out;
}

auto generateExplosionSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.4F), 0.0F);
    addKick(out, 0.0F, 0.35F, 140, 40, 0.12F, 0.20F);
    addNoise(out, 0.0F, 0.4F, 0.15F);
    addNoise(out, 0.02F, 0.2F, 0.10F);
    return out;
}

auto generateMenuMoveSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.08F), 0.0F);
    addSquare(out, 0.0F, 700.0F, 0.15F, 0.04F);
    return out;
}

auto generateMenuConfirmSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.18F), 0.0F);
    addSquare(out, 0.0F, 700.0F, 0.15F, 0.05F);
    addSquare(out, 0.06F, 1050.0F, 0.18F, 0.08F);
    return out;
}

auto generateVictorySfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.6F), 0.0F);
    constexpr std::array<float, 4> notes{523.25F, 659.25F, 783.99F, 1046.50F};
    for (size_t i = 0; i < notes.size(); i++)
    {
        addTone(out, static_cast<float>(i) * 0.1F, notes.at(i), 0.18F, 0.18F);
    }
    return out;
}

auto generateDefeatSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.8F), 0.0F);
    constexpr std::array<float, 3> notes{392.00F, 329.63F, 220.00F};
    for (size_t i = 0; i < notes.size(); i++)
    {
        addTone(out, static_cast<float>(i) * 0.2F, notes.at(i), 0.22F, 0.35F);
    }
    return out;
}

auto generateCriticalSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.15F), 0.0F);
    addSquare(out, 0.0F, 1600.0F, 0.22F, 0.05F);
    addSquare(out, 0.0F, 2200.0F, 0.12F, 0.04F);
    return out;
}

auto generateBossWindUpSfx() -> std::vector<float>
{
    constexpr float duration = 0.6F;
    std::vector<float> out(secondsToSamples(duration), 0.0F);
    for (int i = 0; i < 12; i++)
    {
        const float t = static_cast<float>(i) * (duration / 12.0F);
        const float freq = 100.0F + static_cast<float>(i) * 40.0F;
        addSaw(out, t, freq, 0.1F, 0.08F);
    }
    return out;
}

auto generateBeamFireSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.25F), 0.0F);
    addSaw(out, 0.0F, 500.0F, 0.2F, 0.20F);
    addSaw(out, 0.0F, 505.0F, 0.15F, 0.20F);
    return out;
}

auto generateHomingLaunchSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.3F), 0.0F);
    addNoise(out, 0.0F, 0.15F, 0.15F);
    addKick(out, 0.0F, 0.2F, 300, 700, 0.15F, 0.15F);
    return out;
}

auto generateSpreadBurstSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.15F), 0.0F);
    addNoise(out, 0.0F, 0.3F, 0.05F);
    addSquare(out, 0.0F, 300.0F, 0.2F, 0.06F);
    return out;
}

auto generateSlamBoomSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.5F), 0.0F);
    addKick(out, 0.0F, 0.4F, 120, 30, 0.15F, 0.25F);
    addNoise(out, 0.0F, 0.35F, 0.2F);
    return out;
}

auto generateNerveChargeSfx() -> std::vector<float>
{
    constexpr float duration = 0.35F;
    std::vector<float> out(secondsToSamples(duration), 0.0F);
    addKick(out, 0.0F, 0.22F, 200, 900, duration, duration * 0.9F);
    addSaw(out, 0.0F, 200.0F, 0.05F, duration);
    return out;
}

auto generateNerveReleaseSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.4F), 0.0F);
    addKick(out, 0.0F, 0.4F, 180, 50, 0.12F, 0.22F);
    addNoise(out, 0.0F, 0.3F, 0.15F);
    addSquare(out, 0.0F, 220.0F, 0.15F, 0.18F);
    addSquare(out, 0.0F, 330.0F, 0.10F, 0.18F);
    return out;
}

auto generateNerveFizzleSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.2F), 0.0F);
    addKick(out, 0.0F, 0.18F, 500, 120, 0.15F, 0.12F);
    addNoise(out, 0.0F, 0.15F, 0.08F);
    return out;
}

}

auto LoadSounds() -> Sounds
{
    Sounds sounds{};
    sounds.shoot = loadSoundFromSamples(generateShootSfx(), 0.16F);
    sounds.hit = loadSoundFromSamples(generateHitSfx());
    sounds.explosion = loadSoundFromSamples(generateExplosionSfx());
    sounds.menuMove = loadSoundFromSamples(generateMenuMoveSfx());
    sounds.menuConfirm = loadSoundFromSamples(generateMenuConfirmSfx());
    sounds.victory = loadSoundFromSamples(generateVictorySfx());
    sounds.defeat = loadSoundFromSamples(generateDefeatSfx());
    sounds.critical = loadSoundFromSamples(generateCriticalSfx());
    sounds.bossWindUp = loadSoundFromSamples(generateBossWindUpSfx());
    sounds.beamFire = loadSoundFromSamples(generateBeamFireSfx());
    sounds.homingLaunch = loadSoundFromSamples(generateHomingLaunchSfx());
    sounds.spreadBurst = loadSoundFromSamples(generateSpreadBurstSfx());
    sounds.slamBoom = loadSoundFromSamples(generateSlamBoomSfx());
    sounds.nerveCharge = loadSoundFromSamples(generateNerveChargeSfx());
    sounds.nerveRelease = loadSoundFromSamples(generateNerveReleaseSfx());
    sounds.nerveFizzle = loadSoundFromSamples(generateNerveFizzleSfx());
    return sounds;
}

auto loadBGM() -> BgmLayers
{
    const size_t loopSamples = secondsToSamples(loopSeconds);

    BgmLayers bgm{};
    bgm.base = loadMusicFromSamples(bgm.baseWav, generateBase(loopSamples));
    bgm.intensity = loadMusicFromSamples(bgm.intensityWav, generateIntensity(loopSamples));
    bgm.miniboss = loadMusicFromSamples(bgm.minibossWav, generateMiniboss(loopSamples));
    bgm.megaboss = loadMusicFromSamples(bgm.megabossWav, generateMegaboss(loopSamples));
    bgm.swarmBoss = loadMusicFromSamples(bgm.swarmBossWav, generateSwarmBoss(loopSamples));
    return bgm;
}
