
#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <numbers>
#include <vector>

namespace
{

constexpr int sampleRate = 22050;
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
            break;
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
            break;
        const float env = std::exp(-elapsed / decaySeconds);
        const float sq =
            std::sin(2 * std::numbers::pi_v<float> * freq * elapsed) >= 0 ? 1.0F : -1.0F;
        buf.at(i) += amp * env * sq;
    }
}

void addSaw(std::vector<float>& buf, float startSeconds, float freq, float amp, float decaySeconds)
{
    const size_t start = secondsToSamples(startSeconds);
    for (size_t i = start; i < buf.size(); i++)
    {
        const float elapsed = static_cast<float>(i - start) / sampleRate;
        if (elapsed > decaySeconds * toneCutoffDecays)
            break;
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
            break;
        const float env = std::exp(-elapsed / decaySeconds);
        buf.at(i) += amp * env * (2.0F * static_cast<float>(std::rand()) / RAND_MAX - 1.0F);
    }
}

void addKick_(std::vector<float>& buf, float t0, float amp, float freq, float endFreq,
              float sweepDur, float decay)
{
    const size_t start = secondsToSamples(t0);
    for (size_t i = start; i < buf.size(); i++)
    {
        const float elapsed = static_cast<float>(i - start) / sampleRate;
        if (elapsed > decay * toneCutoffDecays)
            break;
        const float env = std::exp(-elapsed / decay);
        const float sweep = freq - (freq - endFreq) * std::min(elapsed / sweepDur, 1.0F);
        buf.at(i) += amp * env * std::sin(2 * std::numbers::pi_v<float> * sweep * elapsed);
    }
}

auto generateVoidrunnerBase(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);

    for (size_t i = 0; i < loopSamples; i++)
    {
        float t = static_cast<float>(i) / sampleRate;
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
            float t = b + static_cast<float>(n) * beatSeconds * 0.5F;
            float f = motif[static_cast<size_t>(n) % motif.size()];
            addTone(out, t, f, 0.05F, 0.15F);
            addTone(out, t + 0.03F, f * 1.01F, 0.02F, 0.10F);
        }

        addTone(out, b, 55.0F, 0.10F, 0.40F);
        addTone(out, b + 2 * beatSeconds, 55.0F, 0.08F, 0.35F);
    }

    return out;
}

auto generateVoidrunnerIntensity(size_t loopSamples) -> std::vector<float>
{

    std::vector<float> out(loopSamples, 0.0F);

    constexpr std::array<float, 8> melody{523.25F, 659.25F, 783.99F, 659.25F,
                                          587.33F, 523.25F, 440.00F, 523.25F};
    for (int bar = 0; bar < 4; bar++)
    {
        float b = static_cast<float>(bar) * barSeconds;
        for (int n = 0; n < 8; n++)
        {
            float t = b + static_cast<float>(n) * beatSeconds * 0.5F;
            float f = melody[static_cast<size_t>(n) % melody.size()];
            addTone(out, t, f, 0.07F, 0.18F);
            addTone(out, t + 0.12F, f * 1.008F, 0.03F, 0.12F);
        }

        for (int eighth = 0; eighth < 8; eighth++)
            addNoise(out, b + static_cast<float>(eighth) * beatSeconds * 0.5F, 0.05F, 0.02F);
    }

    return out;
}

auto generateVoidrunnerMiniboss(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);
    constexpr std::array<float, 4> roots{220.00F, 220.00F, 174.61F, 174.61F};
    constexpr std::array<float, 8> arpRatio{1.0F, 1.2F, 1.5F, 2.0F, 2.4F, 2.0F, 1.5F, 1.2F};
    constexpr std::array<float, 4> stabBeat{0.0F, 0.5F, 1.5F, 2.5F};
    constexpr std::array<float, 4> stringRatio{1.2F, 1.35F, 1.2F, 1.5F};
    for (int bar = 0; bar < 4; bar++)
    {
        const float b = static_cast<float>(bar) * barSeconds;
        const float r = roots[bar];
        for (int beat = 0; beat < 4; beat++)
            addTone(out, b + static_cast<float>(beat) * beatSeconds, r * stringRatio[beat], 0.06F,
                    beatSeconds * 0.95F);
        addKick_(out, b, 0.30F, 150, 45, 0.08F, 0.12F);
        addKick_(out, b + 2 * beatSeconds + beatSeconds * (1.0F / 3.0F), 0.24F, 150, 40, 0.06F,
                 0.09F);
        addKick_(out, b + 2 * beatSeconds + beatSeconds * 0.5F, 0.28F, 150, 40, 0.06F, 0.09F);
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
            addSquare(out, t, r * arpRatio[n], 0.09F, 0.14F);
            addSquare(out, t, r * arpRatio[n] * 2.0F, 0.04F, 0.10F);
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
auto generateVoidrunnerMegaboss(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);
    constexpr std::array<float, 8> chordRoot{174.61F, 246.94F, 164.81F, 293.66F,
                                             174.61F, 293.66F, 164.81F, 164.81F};
    constexpr std::array<float, 8> chordThird{1.25F, 1.2F, 1.25F, 1.2F, 1.25F, 1.2F, 1.25F, 1.25F};
    constexpr std::array<float, 8> chordFifth{1.5F, 1.4F, 1.5F, 1.5F, 1.5F, 1.5F, 1.5F, 1.5F};
    for (int bar = 0; bar < 4; bar++)
    {
        const float b = static_cast<float>(bar) * barSeconds;
        addKick_(out, b, 0.30F, 150, 45, 0.08F, 0.12F);
        addKick_(out, b + 2 * beatSeconds + beatSeconds * (1.0F / 3.0F), 0.24F, 150, 40, 0.06F,
                 0.09F);
        addKick_(out, b + 2 * beatSeconds + beatSeconds * 0.5F, 0.28F, 150, 40, 0.06F, 0.09F);
        addNoise(out, b + 1 * beatSeconds, 0.34F, 0.07F);
        addNoise(out, b + 3 * beatSeconds, 0.34F, 0.07F);
        for (int slot = 0; slot < 2; slot++)
        {
            const int idx = bar * 2 + slot;
            const float r = chordRoot[idx];
            const float third = chordThird[idx];
            const float fifth = chordFifth[idx];
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
                addSquare(out, t, r * arch[n], 0.09F, 0.14F);
                addSquare(out, t, r * arch[n] * 2.0F, 0.04F, 0.10F);
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

auto generateVoidrunnerSwarmBoss(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);
    constexpr std::array<float, 4> chordRoot{110.00F, 116.54F, 110.00F, 155.56F};

    addTone(out, 0.0F, 32.7F, 0.10F, loopSeconds * 1.05F);

    for (int bar = 0; bar < 4; bar++)
    {
        const float b = static_cast<float>(bar) * barSeconds;
        const float r = chordRoot[bar];
        addTone(out, b, r * 0.5F, 0.14F, barSeconds * 1.05F);
        addTone(out, b, r, 0.11F, barSeconds * 1.05F);
        addTone(out, b, r * 1.2F, 0.07F, barSeconds * 1.05F);
        addTone(out, b, r * 1.4142F, 0.07F, barSeconds * 1.05F);
        addTone(out, b, r * 1.6819F, 0.05F, barSeconds * 1.05F);

        for (int beat = 0; beat < 4; beat++)
        {
            const float t = b + static_cast<float>(beat) * beatSeconds;
            const float trem = (beat % 2 == 0) ? 0.05F : 0.03F;
            addSquare(out, t, r * 1.4142F, trem, 0.9F);
        }
    }

    constexpr std::array<float, 16> dirge{220.00F, 207.65F, 196.00F, 185.00F, 174.61F, 164.81F,
                                          155.56F, 146.83F, 138.59F, 130.81F, 123.47F, 116.54F,
                                          110.00F, 103.83F, 98.00F,  92.50F};
    for (int n = 0; n < 16; n++)
        addSquare(out, static_cast<float>(n) * beatSeconds, dirge[static_cast<size_t>(n)], 0.10F,
                  beatSeconds * 0.9F);

    return out;
}

auto generateVoidrunnerSwarmBossChoir(size_t loopSamples) -> std::vector<float>
{
    std::vector<float> out(loopSamples, 0.0F);
    constexpr std::array<float, 4> chordRoot{110.00F, 116.54F, 110.00F, 155.56F};

    addTone(out, 0.0F, 32.7F, 0.16F, loopSeconds * 1.05F);

    for (int bar = 0; bar < 4; bar++)
    {
        const float b = static_cast<float>(bar) * barSeconds;
        const float r = chordRoot[bar];
        addTone(out, b, r * 0.25F, 0.12F, barSeconds * 1.05F);
        addTone(out, b, r * 0.5F, 0.16F, barSeconds * 1.05F);
        addTone(out, b, r * 0.5F * 1.2F, 0.10F, barSeconds * 1.05F);
        addTone(out, b, r * 0.5F * 1.4142F, 0.09F, barSeconds * 1.05F);
        addTone(out, b, r, 0.08F, barSeconds * 1.05F);
        addTone(out, b, r * 1.2F, 0.05F, barSeconds * 1.05F);
        addTone(out, b, r * 1.4142F, 0.05F, barSeconds * 1.05F);
        addTone(out, b, r * 1.6819F, 0.04F, barSeconds * 1.05F);

        for (int beat = 0; beat < 4; beat++)
        {
            const float t = b + static_cast<float>(beat) * beatSeconds;
            const float trem = (beat % 2 == 0) ? 0.05F : 0.03F;
            addSquare(out, t, r * 0.5F * 1.4142F, trem, 0.9F);
        }

        constexpr float voiceDetune1 = 1.004F;
        constexpr float voiceDetune2 = 0.996F;
        for (float ratio : {1.0F, 1.2F, 1.4142F})
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
        addSquare(out, static_cast<float>(n) * beatSeconds, dirge[static_cast<size_t>(n)], 0.10F,
                  beatSeconds * 0.9F);

    return out;
}

auto toPcm16(const std::vector<float>& samples) -> std::vector<int16_t>
{
    constexpr float pcmScale = 32000.0F;
    float peak = 0.0F;
    for (const float s : samples)
        peak = std::max(peak, std::abs(s));
    const float gain = peak > 1.0F ? 1.0F / peak : 1.0F;
    std::vector<int16_t> pcm(samples.size());
    for (size_t i = 0; i < samples.size(); i++)
        pcm.at(i) = static_cast<int16_t>(samples.at(i) * gain * pcmScale);
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

    const auto writeBytes = [&buf](const void* data, size_t byteCount)
    {
        const auto* bytePtr = static_cast<const std::byte*>(data);
        buf.insert(buf.end(), bytePtr, bytePtr + byteCount);
    };
    const auto writeTag = [&](const char* tag) { writeBytes(tag, tagLength); };
    const auto writeU32 = [&](uint32_t value) { writeBytes(&value, sizeof(value)); };
    const auto writeU16 = [&](uint16_t value) { writeBytes(&value, sizeof(value)); };

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

constexpr int layersPerGroup = 5;
constexpr int groupCount = 2;

struct Layer
{
    std::vector<std::byte> wav;
    Music music{};
    float volume;
};

struct SongGroup
{
    Layer layers[layersPerGroup];
    const char* name;
    float pitch = 1.0F;
};

auto makeLayer(const std::vector<float>& samples) -> Layer
{
    Layer l;
    l.wav = encodeWAV(toPcm16(samples));
    const auto* data = reinterpret_cast<const unsigned char*>(l.wav.data());
    l.music = LoadMusicStreamFromMemory(".wav", data, static_cast<int>(l.wav.size()));
    l.music.looping = true;
    return l;
}

const char* layerLabel(int i)
{
    switch (i)
    {
    case 0:
        return "Base";
    case 1:
        return "Intensity";
    case 2:
        return "Mini Boss";
    case 3:
        return "Main Boss";
    case 4:
        return "Swarm Boss";
    default:
        return "";
    }
}

struct State
{
    int activeGroup = 0;
    bool active[groupCount][layersPerGroup]{};
    SongGroup groups[groupCount];
};

void applyMix(State& state)
{
    for (int g = 0; g < groupCount; g++)
        for (int i = 0; i < layersPerGroup; i++)
            SetMusicVolume(state.groups[g].layers[i].music,
                           state.active[g][i] ? state.groups[g].layers[i].volume : 0.0F);
}

void applyPitch(State& state)
{
    for (int g = 0; g < groupCount; g++)
        for (int i = 0; i < layersPerGroup; i++)
            SetMusicPitch(state.groups[g].layers[i].music, state.groups[g].pitch);
}

void switchGroup(State& state, int newGroup)
{
    state.activeGroup = (newGroup + groupCount) % groupCount;
    applyMix(state);
}

}

auto main() -> int
{
    InitWindow(960, 540, "BGM Prototype - GalaxyImpact");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    InitAudioDevice();
    SetTargetFPS(60);

    const size_t loopSamples = secondsToSamples(loopSeconds);

    State state;
    state.active[0][0] = true;

    auto initGroup =
        [&](int idx, const char* name, auto gen0, auto gen1, auto gen2, auto gen3, auto gen4)
    {
        state.groups[idx].name = name;
        state.groups[idx].pitch = 1.0F;
        state.groups[idx].layers[0] = makeLayer(gen0(loopSamples));
        state.groups[idx].layers[0].volume = 0.6F;
        state.groups[idx].layers[1] = makeLayer(gen1(loopSamples));
        state.groups[idx].layers[1].volume = 0.6F;
        state.groups[idx].layers[2] = makeLayer(gen2(loopSamples));
        state.groups[idx].layers[2].volume = 0.6F;
        state.groups[idx].layers[3] = makeLayer(gen3(loopSamples));
        state.groups[idx].layers[3].volume = 0.3F;
        state.groups[idx].layers[4] = makeLayer(gen4(loopSamples));
        state.groups[idx].layers[4].volume = 1.0F;
    };

    initGroup(0, "Voidrunner", generateVoidrunnerBase, generateVoidrunnerIntensity,
              generateVoidrunnerMiniboss, generateVoidrunnerMegaboss, generateVoidrunnerSwarmBoss);

    initGroup(1, "Voidrunner (Choir Test)", generateVoidrunnerBase, generateVoidrunnerIntensity,
              generateVoidrunnerMiniboss, generateVoidrunnerMegaboss,
              generateVoidrunnerSwarmBossChoir);

    for (int g = 0; g < groupCount; g++)
        for (int i = 0; i < layersPerGroup; i++)
            PlayMusicStream(state.groups[g].layers[i].music);

    applyMix(state);
    applyPitch(state);

    while (!WindowShouldClose())
    {
        int key = GetKeyPressed();
        if (key == KEY_LEFT)
            switchGroup(state, state.activeGroup - 1);
        if (key == KEY_RIGHT)
            switchGroup(state, state.activeGroup + 1);
        if (key >= 49 && key <= 53)
        {
            int idx = key - 49;
            state.active[state.activeGroup][idx] = !state.active[state.activeGroup][idx];
            applyMix(state);
        }
        if (key == 61 || key == 43)
        {
            int g = state.activeGroup;
            state.groups[g].pitch = std::min(state.groups[g].pitch + 0.1F, 2.0F);
            applyPitch(state);
        }
        if (key == 45)
        {
            int g = state.activeGroup;
            state.groups[g].pitch = std::max(state.groups[g].pitch - 0.1F, 0.5F);
            applyPitch(state);
        }
        if (key == 48)
        {
            for (int g = 0; g < groupCount; g++)
                state.groups[g].pitch = 1.0F;
            applyPitch(state);
        }
        if (key == KEY_ESCAPE)
            break;

        for (int g = 0; g < groupCount; g++)
            for (int i = 0; i < layersPerGroup; i++)
                UpdateMusicStream(state.groups[g].layers[i].music);

        BeginDrawing();
        ClearBackground(BLACK);

        int w = GetScreenWidth();
        int gy = 20, gx = 40;
        for (int gi = 0; gi < groupCount; gi++)
        {
            const char* label =
                TextFormat("%s%s", state.groups[gi].name, gi == state.activeGroup ? "<<" : "");
            int tw = MeasureText(label, 13) + 6;
            if (gx + tw > w - 20 && gx > 40)
            {
                gx = 40;
                gy += 18;
            }
            DrawText(label, gx, gy, 13, gi == state.activeGroup ? GREEN : DARKGRAY);
            gx += tw;
        }
        gy += 22;
        DrawLine(20, gy, w - 20, gy, DARKGRAY);
        gy += 6;

        for (int i = 0; i < layersPerGroup; i++)
        {
            int gi = state.activeGroup;
            const char* mark = state.active[gi][i] ? "[x]" : "[ ]";
            Color c = state.active[gi][i] ? GREEN : GRAY;
            DrawText(TextFormat("%d %s %s", i + 1, mark, layerLabel(i)), 40, gy + i * 28, 16, c);
        }

        gy += layersPerGroup * 28 + 4;
        {
            char buf[256] = {};
            int pos = 0;
            for (int gi = 0; gi < groupCount; gi++)
                for (int i = 0; i < layersPerGroup; i++)
                    if (state.active[gi][i])
                    {
                        pos += std::snprintf(buf + pos, sizeof(buf) - pos, "%d ", gi);
                        break;
                    }
            if (pos)
                DrawText(TextFormat("Active groups: %s", buf), 40, gy, 14, DARKGRAY);
            gy += 18;
        }

        int gi = state.activeGroup;
        DrawText(TextFormat("Speed: %.1fx  (+/- change, 0 reset)", state.groups[gi].pitch), 40, gy,
                 14, YELLOW);
        gy += 20;
        DrawText(
            "<-/-> select group | 1-5 toggle layer in current group | +/- speed | 0 reset | ESC",
            40, gy, 14, DARKGRAY);
        DrawFPS(w - 100, 10);

        EndDrawing();
    }

    for (int g = 0; g < groupCount; g++)
        for (int i = 0; i < layersPerGroup; i++)
        {
            StopMusicStream(state.groups[g].layers[i].music);
            UnloadMusicStream(state.groups[g].layers[i].music);
        }

    CloseAudioDevice();
    CloseWindow();

    return 0;
}
