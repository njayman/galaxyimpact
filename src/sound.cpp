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

// A tone's envelope is inaudible well before it mathematically reaches zero
// - stop generating samples once the exponential decay has run this many
// decaySeconds past the tone's start, instead of processing the rest of the
// (silent) buffer for nothing.
constexpr float toneCutoffDecays = 6.0F;

// Fundamental-to-overtone ratios for addPluck's brighter "plucked" layer: a
// quiet octave-up overtone that decays faster than the fundamental.
constexpr float pluckOvertoneAmpRatio = 0.35F;
constexpr float pluckOvertoneDecayRatio = 0.6F;

// A short decaying tone - the building block for both the percussive
// intensity pulse and the plucked upgrade arpeggio.
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

// Fundamental plus a quiet octave overtone - brighter/more "plucked" than a
// bare sine, used for the upgrade layer's arpeggio.
void addPluck(std::vector<float>& buf, float startSeconds, float freq, float amp,
              float decaySeconds)
{
    addTone(buf, startSeconds, freq, amp, decaySeconds);
    addTone(buf, startSeconds, freq * 2, amp * pluckOvertoneAmpRatio,
            decaySeconds * pluckOvertoneDecayRatio);
}

// drone: a slow, warm A-root chord in a light mid register - root, a true
// octave, and a detuned unison for a gentle chorus - plus one quiet,
// higher-register color tone for a bright shimmer. Always audible.
//
// This used to also add a low-passed noise bed for "the hum of deep space",
// but even quiet it read as a constant background hiss rather than
// atmosphere - cut entirely rather than trying to tune it further. It also
// used to sit two octaves lower (A1 root) - full sub-bass read as a heavy,
// oppressive "deep" drone rather than the light ambient bed intended; moved
// up to A3 for a lighter tone while keeping the exact same chord shape.
//
// The root/octave/detune voices are all within ~1Hz or an exact 2x ratio of
// each other, so they reinforce instead of beating (harmonious); the color
// tone sits nearly two octaves higher and quiet, staying an accent rather
// than colliding with the rest of the chord. Earlier this stacked three sine
// tones only 10-12Hz apart in the same register (root/minor third/tritone) -
// that's squarely in the psychoacoustic "critical band roughness" zone for
// pure sines, so it read as harsh buzzing rather than a chord. The loop
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
        Voice{.freq = 220.00F, .amp = 0.45F, .tremoloHz = 0.07F}, // A3 - root
        Voice{.freq = 220.15F, .amp = 0.28F, .tremoloHz = 0.09F}, // detuned unison - slow chorus
        Voice{.freq = 440.00F, .amp = 0.22F, .tremoloHz = 0.05F}, // A4 - true octave, zero beat
        Voice{.freq = 932.32F,
              .amp = 0.05F,
              .tremoloHz = 0.15F}, // Bb5 - quiet, bright, the shimmer on top
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

// intensity: a pulse on the perfect fifth above the drone's root (E4 -
// the most consonant non-octave interval, so it locks in with the drone
// instead of beating against it) - a steady beat with a syncopated push,
// "more beat with the base beat" as its volume fades in.
auto generateIntensity(size_t loopSamples) -> std::vector<float>
{
    constexpr float intensityFreq = 329.63F; // E4
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
    // One steady beat on 1 and 3, a syncopated push on the "and" of 2 and 4.
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

// upgrade: a bright plucked arpeggio built on the drone's own root/fifth/
// octave (A/E/A), two-plus octaves up, with one passing major-third color
// tone (C#) for character - consonant intervals at a register and note
// length short enough that a color tone doesn't turn into sustained beating.
// Same harmonic DNA as the drone, just elevated, so it reads as a reward
// layered on top rather than a clashing new tune.
auto generateUpgrade(size_t loopSamples) -> std::vector<float>
{
    constexpr float arpeggioStepBeats = 0.5F; // eighth notes
    constexpr float upgradeNoteAmp = 0.16F;
    constexpr float upgradeNoteDecay = 0.3F;

    std::vector<float> out(loopSamples, 0.0F);

    constexpr std::array<float, 4> arpeggio{220.00F, 277.18F, 329.63F, 440.00F}; // A3 C#4 E4 A4
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

// toPcm16 normalizes by the buffer's own peak rather than hard-clamping to
// [-1, 1] - the drone's voices can sum past 1.0 at their tremolo peaks (0.45
// + 0.28 + 0.22 + 0.05 + noise), and clamping a signal that exceeds that
// range flattens its peaks into harsh, buzzing digital clipping instead of
// just playing back slightly quieter.
auto toPcm16(const std::vector<float>& samples) -> std::vector<int16_t>
{
    // Slightly under INT16_MAX (32767) for a little headroom, not a hard
    // clip boundary.
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

// Encodes 16-bit mono PCM as a standard RIFF/WAVE byte buffer, suitable for
// LoadMusicStreamFromMemory(".wav", ...).
auto encodeWAV(const std::vector<int16_t>& samples) -> std::vector<std::byte>
{
    constexpr uint16_t channels = 1;
    constexpr uint16_t bitsPerSample = 16;
    constexpr uint16_t pcmFormatTag = 1;
    constexpr uint32_t fmtChunkSize = 16; // fixed size of a PCM "fmt " subchunk
    constexpr uint32_t wavHeaderSize = 44;
    // RIFF's own size field counts everything after itself (the 8-byte
    // "RIFF"+size fields aren't included) - the classic 44-byte-header WAV
    // formula is headerSize - 8 + dataSize.
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
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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

// loadMusicFromSamples encodes samples as WAV bytes into wavStorage (owned by
// the caller's BgmLayers, not this function) and hands raylib a pointer into
// it. raylib's WAV decoder (dr_wav) doesn't copy that buffer - it keeps
// reading from the same pointer on every UpdateMusicStream call for as long
// as the stream plays, so wavStorage must outlive the Music object, not just
// this call (a local buffer freed on return was a use-after-free heard as
// persistent static/hiss over the music).
auto loadMusicFromSamples(std::vector<std::byte>& wavStorage,
                          const std::vector<float>& samples) -> Music
{
    wavStorage = encodeWAV(toPcm16(samples));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* wavData = reinterpret_cast<const unsigned char*>(wavStorage.data());
    Music music = LoadMusicStreamFromMemory(".wav", wavData, static_cast<int>(wavStorage.size()));
    music.looping = true;
    return music;
}

} // namespace

auto loadBGM() -> BgmLayers
{
    const size_t loopSamples = secondsToSamples(loopSeconds);

    BgmLayers bgm{};
    bgm.drone = loadMusicFromSamples(bgm.droneWav, generateDrone(loopSamples));
    bgm.intensity = loadMusicFromSamples(bgm.intensityWav, generateIntensity(loopSamples));
    bgm.upgrade = loadMusicFromSamples(bgm.upgradeWav, generateUpgrade(loopSamples));
    return bgm;
}
