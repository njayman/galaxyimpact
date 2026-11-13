#include "sound.hpp"

#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <random>
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

auto noiseSample() -> float
{
    static std::mt19937 rng{std::random_device{}()};
    static std::uniform_real_distribution<float> dist(-1.0F, 1.0F);
    return dist(rng);
}

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

// smooth: one-pole low-pass on the noise. 0 = raw white noise, toward 1 = duller and softer.
void addNoise(std::vector<float>& buf, float startSeconds, float amp, float decaySeconds, float smooth = 0.0F)
{
    float low = 0.0F;
    const size_t start = secondsToSamples(startSeconds);
    for (size_t i = start; i < buf.size(); i++)
    {
        const float elapsed = static_cast<float>(i - start) / sampleRate;
        if (elapsed > decaySeconds * toneCutoffDecays)
        {
            break;
        }
        const float env = std::exp(-elapsed / decaySeconds);
        low += (noiseSample() - low) * (1.0F - smooth);
        buf.at(i) += amp * env * low;
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

// --- Boss tracks per biome ---------------------------------------------------------------
// A full boss track is eight bars written for the biome (B), then the megaboss loop twice (A A):
// 16 bars, so it still lines up with the 4-bar base loop. B is a build-up. It opens sparse, on the
// biome's own voice over a pad, and brings the loop's parts in bar by bar (bass, hats, arpeggio,
// snare, stabs, a roll) until it lands on the full loop. Same tempo and key centre (A) throughout;
// each biome changes the scale, the harmony and the lead voice.

auto midi(int note) -> float
{
    return 440.0F * std::pow(2.0F, static_cast<float>(note - 69) / 12.0F);
}

auto at(int bar, float beat) -> float
{
    return (static_cast<float>(bar) * 4.0F + beat) * beatSeconds;
}

struct Chord
{
    int root;  // bass register (midi)
    int third; // 3 = minor, 4 = major
    int fifth; // 7 = perfect, 6 = diminished
};

// How loud each part of the loop is at each of the build's eight bars.
constexpr std::array<float, 8> buildBass{0.0F, 0.5F, 0.8F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F};
constexpr std::array<float, 8> buildArp{0.0F, 0.0F, 0.3F, 0.5F, 0.7F, 0.85F, 1.0F, 1.0F};
constexpr std::array<float, 8> buildStab{0.0F, 0.0F, 0.0F, 0.0F, 0.6F, 0.8F, 1.0F, 1.0F};

// One bar of the build's drums. Bars 0-1: a bare kick. Bars 2-3: the kick pattern and hats.
// Bars 4-7: the full megaboss kit (kick, snare on 2 and 4, shuffled hats). Bar 7 ends on a rising roll.
// fourOnFloor swaps the kick pattern for one on every beat with sixteenth hats (Solar Forge).
void buildDrumBar(std::vector<float>& out, int bar, bool fourOnFloor)
{
    if (fourOnFloor)
    {
        for (int beat = 0; beat < 4; beat++)
        {
            const auto bt = static_cast<float>(beat);
            if (bar >= 2 || beat % 2 == 0)
            {
                addKick(out, at(bar, bt), 0.28F, 150, 45, 0.08F, 0.11F);
            }
            if (bar >= 2)
            {
                addNoise(out, at(bar, bt + 0.5F), 0.11F, 0.035F);
            }
            for (int s = 0; bar >= 4 && s < 4; s++)
            {
                addNoise(out, at(bar, bt + static_cast<float>(s) * 0.25F), 0.05F, 0.012F);
            }
        }
    }
    else
    {
        addKick(out, at(bar, 0), 0.30F, 150, 45, 0.08F, 0.12F);
        if (bar >= 1)
        {
            addKick(out, at(bar, 2.0F + 1.0F / 3.0F), 0.24F, 150, 40, 0.06F, 0.09F);
            addKick(out, at(bar, 2.5F), 0.28F, 150, 40, 0.06F, 0.09F);
        }
        for (int beat = 0; bar >= 2 && beat < 4; beat++)
        {
            const auto bt = static_cast<float>(beat);
            const float hats = bar >= 4 ? 1.0F : 0.6F;
            addNoise(out, at(bar, bt + 1.0F / 3.0F), 0.08F * hats, 0.02F);
            addNoise(out, at(bar, bt + 0.5F), 0.11F * hats, 0.025F);
            addNoise(out, at(bar, bt + 5.0F / 6.0F), 0.08F * hats, 0.02F);
        }
    }
    if (bar >= 4)
    {
        addNoise(out, at(bar, 1), 0.34F, 0.07F);
        addNoise(out, at(bar, 3), 0.34F, 0.07F);
    }
    if (bar == 7)
    {
        for (int i = 0; i < 8; i++)
        {
            addNoise(out, at(bar, 2.0F + static_cast<float>(i) * 0.25F), 0.10F + 0.03F * static_cast<float>(i), 0.03F);
        }
    }
}

// The megaboss loop's tonal parts for half a bar on one chord: bass with its two pick-up notes,
// the sixteenth-note arpeggio and the chord stabs, each at the level the build has reached by this
// bar. A soft pad holds the chord while they are still coming in. Chords change every half bar,
// like the loop itself.
void bossEngineHalfBar(std::vector<float>& out, int bar, int slot, Chord c)
{
    const float bass = buildBass.at(static_cast<size_t>(bar));
    const float arp = buildArp.at(static_cast<size_t>(bar));
    const float stab = buildStab.at(static_cast<size_t>(bar));
    const float r = midi(c.root + 12);
    const float third = std::pow(2.0F, static_cast<float>(c.third) / 12.0F);
    const float fifth = std::pow(2.0F, static_cast<float>(c.fifth) / 12.0F);
    const float sb = static_cast<float>(slot) * 2.0F;
    for (int beat = 0; beat < 2; beat++)
    {
        const float bt = sb + static_cast<float>(beat);
        addSaw(out, at(bar, bt), r * 0.25F, 0.20F * bass, beatSeconds * 0.9F);
        addSaw(out, at(bar, bt + 0.5F), r * 0.5F, 0.13F * bass, beatSeconds * 0.2F);
        addSaw(out, at(bar, bt + 0.75F), r * 0.5F, 0.12F * bass, beatSeconds * 0.2F);
    }
    for (const float ratio : {0.5F, 1.0F, fifth}) // pad, fading as the stabs take over
    {
        addTone(out, at(bar, sb), r * ratio, 0.07F * (1.0F - stab), beatSeconds * 1.6F);
    }
    const std::array<float, 8> arch{1.0F, third, fifth, 2.0F, 2.0F * third, 2.0F, fifth, third};
    for (size_t n = 0; n < arch.size(); n++)
    {
        const float t = at(bar, sb + static_cast<float>(n) * 0.25F);
        addSquare(out, t, r * arch.at(n), 0.09F * arp, 0.14F);
        addSquare(out, t, r * arch.at(n) * 2.0F, 0.04F * arp, 0.10F);
    }
    for (int beat = 0; beat < 2; beat++)
    {
        const float amp = (beat == 0 ? 1.0F : 0.85F) * stab;
        const float decay = beat == 0 ? 0.26F : 0.22F;
        const float t = at(bar, sb + static_cast<float>(beat));
        addTone(out, t, r, 0.09F * amp, decay);
        addTone(out, t, r * third, 0.07F * amp, decay);
        addTone(out, t, r * fifth, 0.07F * amp, decay);
    }
}

using BarChords = std::array<Chord, 2>; // one chord per half bar

constexpr Chord Am{45, 3, 7};
constexpr Chord Bb{46, 4, 7};
constexpr Chord C{48, 4, 7};
constexpr Chord Dm{38, 3, 7};
constexpr Chord E{40, 4, 7};
constexpr Chord Ebdim{39, 3, 6};
constexpr Chord F{41, 4, 7};
constexpr Chord G{43, 4, 7};
constexpr Chord Gm{43, 3, 7};

// A lead line in eighth notes; 0 is a rest. Square with a quiet sine an octave up.
void leadBar(std::vector<float>& out, int bar, const std::array<int, 8>& notes, float amp, float decay,
             int transpose = 0)
{
    for (size_t n = 0; n < notes.size(); n++)
    {
        if (notes.at(n) == 0)
        {
            continue;
        }
        const float t = at(bar, static_cast<float>(n) * 0.5F);
        addSquare(out, t, midi(notes.at(n) + transpose), amp, decay);
        addTone(out, t, midi(notes.at(n) + transpose + 12), amp * 0.4F, decay * 0.8F);
    }
}

// A held line, one note per half bar: two sines slightly apart.
void voiceBar(std::vector<float>& out, int bar, const std::array<int, 2>& notes, float amp)
{
    for (size_t half = 0; half < 2; half++)
    {
        const float t = at(bar, static_cast<float>(half) * 2.0F);
        addTone(out, t, midi(notes.at(half)) * 1.004F, amp, beatSeconds * 1.6F);
        addTone(out, t, midi(notes.at(half)) * 0.996F, amp, beatSeconds * 1.6F);
    }
}

// Shattered Belt: rock. The Andalusian cadence (Am G F E), first a bar each, then twice as fast;
// tom fills tumbling into every other bar; a leaping lead.
void shatteredBeltSection(std::vector<float>& out)
{
    constexpr std::array<BarChords, 8> chords{{{Am, Am}, {G, G}, {F, F}, {E, E}, {Am, G}, {F, E}, {Am, G}, {F, E}}};
    constexpr std::array<std::array<int, 8>, 8> lead{{{69, 0, 72, 69, 76, 0, 74, 72},
                                                      {67, 0, 71, 67, 74, 0, 72, 71},
                                                      {65, 0, 69, 65, 72, 0, 69, 72},
                                                      {76, 0, 80, 76, 83, 0, 80, 76},
                                                      {69, 0, 72, 76, 74, 0, 71, 67},
                                                      {77, 0, 72, 69, 76, 0, 80, 83},
                                                      {81, 0, 76, 72, 79, 0, 74, 71},
                                                      {77, 76, 72, 69, 76, 80, 83, 88}}};
    for (int bar = 0; bar < 8; bar++)
    {
        buildDrumBar(out, bar, false);
        for (int slot = 0; slot < 2; slot++)
        {
            bossEngineHalfBar(out, bar, slot, chords.at(static_cast<size_t>(bar)).at(static_cast<size_t>(slot)));
        }
        if (bar == 3 || bar == 5) // bar 7 has the roll instead
        {
            for (int i = 0; i < 4; i++)
            {
                const float f = 220.0F - static_cast<float>(i) * 30.0F;
                addKick(out, at(bar, 3.0F + static_cast<float>(i) * 0.25F), 0.20F, f, f * 0.5F, 0.06F, 0.08F);
            }
        }
        leadBar(out, bar, lead.at(static_cast<size_t>(bar)), 0.08F, 0.16F);
    }
}

// Rustbloom: Phrygian, the flat second (Bb) leaning on the root. A detuned voice sways over the
// engine, the bass creeps up from a semitone below, dry rattles in the gaps.
void rustbloomSection(std::vector<float>& out)
{
    constexpr std::array<BarChords, 8> chords{
        {{Am, Bb}, {Am, Gm}, {Am, Bb}, {Dm, E}, {Am, Bb}, {C, Bb}, {Am, Gm}, {Bb, E}}};
    constexpr std::array<std::array<int, 2>, 8> voice{
        {{76, 77}, {76, 74}, {72, 74}, {77, 80}, {81, 82}, {84, 82}, {81, 79}, {77, 76}}};
    for (int bar = 0; bar < 8; bar++)
    {
        buildDrumBar(out, bar, false);
        for (int slot = 0; slot < 2; slot++)
        {
            const Chord c = chords.at(static_cast<size_t>(bar)).at(static_cast<size_t>(slot));
            bossEngineHalfBar(out, bar, slot, c);
            // creeps in from a semitone below, just before the chord lands
            addSaw(out, at(bar, static_cast<float>(slot) * 2.0F + 1.75F), midi(c.root - 1), 0.10F, beatSeconds * 0.2F);
        }
        for (const float beat : {0.75F, 1.25F, 2.75F, 3.25F}) // rattle
        {
            addNoise(out, at(bar, beat), 0.07F, 0.012F);
        }
        voiceBar(out, bar, voice.at(static_cast<size_t>(bar)), 0.08F);
    }
}

// Solar Forge: harmonic minor (G#) and a kick on every beat. Sixteenth hats, anvil clangs on the
// off-beats, a lead that keeps climbing.
void solarForgeSection(std::vector<float>& out)
{
    constexpr std::array<BarChords, 8> chords{{{Am, F}, {Dm, E}, {Am, F}, {E, E}, {Am, F}, {Dm, E}, {F, G}, {E, E}}};
    constexpr std::array<std::array<int, 8>, 8> lead{{{69, 0, 72, 76, 77, 0, 72, 69},
                                                      {74, 0, 77, 74, 76, 0, 80, 83},
                                                      {81, 0, 76, 72, 77, 0, 81, 84},
                                                      {83, 80, 76, 80, 83, 0, 88, 0},
                                                      {69, 72, 76, 81, 77, 0, 72, 69},
                                                      {74, 77, 81, 86, 88, 0, 83, 80},
                                                      {77, 0, 81, 84, 79, 0, 83, 86},
                                                      {88, 86, 84, 83, 80, 83, 88, 0}}};
    for (int bar = 0; bar < 8; bar++)
    {
        buildDrumBar(out, bar, true);
        for (int slot = 0; slot < 2; slot++)
        {
            const Chord c = chords.at(static_cast<size_t>(bar)).at(static_cast<size_t>(slot));
            bossEngineHalfBar(out, bar, slot, c);
            const float f = midi(c.root + 36); // anvil: inharmonic partials, short ring
            const float t = at(bar, static_cast<float>(slot) * 2.0F + 1.5F);
            addTone(out, t, f, 0.07F, 0.09F);
            addTone(out, t, f * 2.76F, 0.05F, 0.07F);
            addTone(out, t, f * 5.4F, 0.03F, 0.05F);
        }
        leadBar(out, bar, lead.at(static_cast<size_t>(bar)), 0.08F, 0.18F, -12);
    }
}

// Punctum: the tritone (Eb against A) keeps pulling the harmony off-centre. An extra heartbeat in
// the kick, static, bells with a bent overtone, and a slow chromatic descent over the engine.
void punctumSection(std::vector<float>& out)
{
    constexpr std::array<BarChords, 8> chords{
        {{Am, Ebdim}, {Am, Ebdim}, {F, E}, {F, E}, {Am, Ebdim}, {Dm, Ebdim}, {F, E}, {E, E}}};
    constexpr std::array<std::array<int, 2>, 8> descent{
        {{81, 78}, {76, 75}, {77, 76}, {72, 71}, {81, 78}, {77, 75}, {72, 68}, {71, 76}}};
    for (int bar = 0; bar < 8; bar++)
    {
        buildDrumBar(out, bar, false);
        addKick(out, at(bar, 0.5F), 0.20F, 140, 45, 0.08F, 0.12F); // heartbeat
        for (int s = 0; s < 16; s++) // static
        {
            addNoise(out, at(bar, static_cast<float>(s) * 0.25F), 0.025F, 0.008F);
        }
        for (int slot = 0; slot < 2; slot++)
        {
            const Chord c = chords.at(static_cast<size_t>(bar)).at(static_cast<size_t>(slot));
            bossEngineHalfBar(out, bar, slot, c);
            const std::array<int, 4> bell{0, c.fifth, c.third, 12};
            for (size_t n = 0; n < bell.size(); n++)
            {
                const float t = at(bar, static_cast<float>(slot) * 2.0F + static_cast<float>(n) * 0.5F);
                const float f = midi(c.root + 36 + bell.at(n));
                addTone(out, t, f, 0.05F, 0.32F);
                addTone(out, t, f * 3.01F, 0.010F, 0.2F); // bent overtone
            }
        }
        voiceBar(out, bar, descent.at(static_cast<size_t>(bar)), 0.07F);
    }
}

auto generateBossTrack(size_t loopSamples, size_t biome) -> std::vector<float>
{
    const std::vector<float> intro = generateMegaboss(loopSamples); // the loop the section builds up to
    std::vector<float> section(loopSamples * 2, 0.0F);
    switch (biome)
    {
    case 0:
        shatteredBeltSection(section);
        break;
    case 1:
        rustbloomSection(section);
        break;
    case 2:
        solarForgeSection(section);
        break;
    default:
        punctumSection(section);
        break;
    }
    for (float& sample : section)
    {
        sample *= 0.87F; // the build's last bars carry the biome's voice on top of the loop's parts: level them with the loop
    }
    std::vector<float> out;
    out.reserve(loopSamples * 4);
    out.insert(out.end(), section.begin(), section.end());
    out.insert(out.end(), intro.begin(), intro.end());
    out.insert(out.end(), intro.begin(), intro.end());
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
constexpr float sfxBusVolume = 0.7F; // every SFX sits under the BGM (base layer 0.6)

auto loadSoundFromSamples(const std::vector<float>& samples,
                          float volume = defaultSfxVolume) -> Sound
{
    const std::vector<std::byte> wavBytes = encodeWAV(toPcm16(samples));
    const auto* wavData = reinterpret_cast<const unsigned char*>(wavBytes.data());
    Wave wave = LoadWaveFromMemory(".wav", wavData, static_cast<int>(wavBytes.size()));
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    SetSoundVolume(sound, volume * sfxBusVolume);
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
    addNoise(out, 0.0F, 0.4F, 0.05F, 0.5F);
    addTone(out, 0.0F, 220.0F, 0.22F, 0.08F);
    return out;
}

auto generateExplosionSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.4F), 0.0F);
    addKick(out, 0.0F, 0.32F, 160, 60, 0.12F, 0.20F);
    addNoise(out, 0.0F, 0.5F, 0.15F, 0.6F);
    addNoise(out, 0.02F, 0.25F, 0.10F, 0.6F);
    return out;
}

auto generateEnemyDeathSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.1F), 0.0F);
    addKick(out, 0.0F, 0.22F, 130, 55, 0.03F, 0.05F);
    addNoise(out, 0.0F, 0.03F, 0.015F);
    return out;
}

auto generateRockBreakSfx() -> std::vector<float>
{

    std::vector<float> out(secondsToSamples(0.22F), 0.0F);
    addKick(out, 0.0F, 0.2F, 110, 45, 0.10F, 0.14F);
    addNoise(out, 0.0F, 0.10F, 0.05F);
    addTone(out, 0.0F, 70.0F, 0.14F, 0.12F);
    return out;
}

auto generateSquishHitSfx() -> std::vector<float>
{

    std::vector<float> out(secondsToSamples(0.18F), 0.0F);
    addKick(out, 0.0F, 0.2F, 220, 90, 0.06F, 0.08F);
    addNoise(out, 0.0F, 0.14F, 0.05F);
    addTone(out, 0.01F, 140.0F, 0.1F, 0.07F);
    return out;
}

auto generateThunderSfx() -> std::vector<float>
{
    // Soft on purpose: no sub-bass and dull noise, the old rumble crackled and strained speakers.
    std::vector<float> out(secondsToSamples(1.0F), 0.0F);
    addKick(out, 0.0F, 0.24F, 170, 75, 0.4F, 0.5F);
    addNoise(out, 0.0F, 0.5F, 0.5F, 0.8F);
    addTone(out, 0.05F, 110.0F, 0.1F, 0.4F);
    return out;
}

// Pickup and debuff cues: short, mid/high pitched, no noise, so they stay clear of the mix.
auto generatePickupSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.14F), 0.0F);
    addTone(out, 0.0F, 988.0F, 0.16F, 0.04F);
    addTone(out, 0.045F, 1319.0F, 0.16F, 0.05F);
    return out;
}

auto generateHealSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.3F), 0.0F);
    addTone(out, 0.0F, 659.25F, 0.13F, 0.08F);
    addTone(out, 0.07F, 880.0F, 0.13F, 0.08F);
    addTone(out, 0.14F, 1108.73F, 0.13F, 0.1F);
    return out;
}

auto generatePowerUpSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.22F), 0.0F);
    addSquare(out, 0.0F, 783.99F, 0.07F, 0.035F);
    addSquare(out, 0.05F, 1046.5F, 0.07F, 0.035F);
    addSquare(out, 0.10F, 1567.98F, 0.08F, 0.06F);
    return out;
}

auto generateDebuffSfx() -> std::vector<float>
{
    std::vector<float> out(secondsToSamples(0.3F), 0.0F);
    addSquare(out, 0.0F, 739.99F, 0.08F, 0.07F);
    addSquare(out, 0.09F, 523.25F, 0.09F, 0.12F);
    addSaw(out, 0.09F, 527.0F, 0.04F, 0.12F);
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
    sounds.shoot = loadSoundFromSamples(generateShootSfx(), 0.08F);
    sounds.hit = loadSoundFromSamples(generateHitSfx());
    sounds.explosion = loadSoundFromSamples(generateExplosionSfx());
    sounds.menuMove = loadSoundFromSamples(generateMenuMoveSfx());
    sounds.menuConfirm = loadSoundFromSamples(generateMenuConfirmSfx());
    sounds.victory = loadSoundFromSamples(generateVictorySfx());
    sounds.defeat = loadSoundFromSamples(generateDefeatSfx());
    sounds.critical = loadSoundFromSamples(generateCriticalSfx());
    sounds.bossWindUp = loadSoundFromSamples(generateBossWindUpSfx());
    sounds.beamFire = loadSoundFromSamples(generateBeamFireSfx());
    sounds.homingLaunch = loadSoundFromSamples(generateHomingLaunchSfx(), 0.16F);
    sounds.spreadBurst = loadSoundFromSamples(generateSpreadBurstSfx());
    sounds.slamBoom = loadSoundFromSamples(generateSlamBoomSfx());
    sounds.nerveCharge = loadSoundFromSamples(generateNerveChargeSfx());
    sounds.nerveRelease = loadSoundFromSamples(generateNerveReleaseSfx());
    sounds.nerveFizzle = loadSoundFromSamples(generateNerveFizzleSfx());
    sounds.rockBreak = loadSoundFromSamples(generateRockBreakSfx());
    sounds.squishHit = loadSoundFromSamples(generateSquishHitSfx());
    sounds.thunder = loadSoundFromSamples(generateThunderSfx());
    sounds.enemyDeath = loadSoundFromSamples(generateEnemyDeathSfx());
    sounds.pickup = loadSoundFromSamples(generatePickupSfx());
    sounds.heal = loadSoundFromSamples(generateHealSfx());
    sounds.powerUp = loadSoundFromSamples(generatePowerUpSfx());
    sounds.debuff = loadSoundFromSamples(generateDebuffSfx());
    return sounds;
}

auto loadBGM() -> BgmLayers
{
    const size_t loopSamples = secondsToSamples(loopSeconds);

    BgmLayers bgm{};
    bgm.base = loadMusicFromSamples(bgm.baseWav, generateBase(loopSamples));
    bgm.intensity = loadMusicFromSamples(bgm.intensityWav, generateIntensity(loopSamples));
    bgm.miniboss = loadMusicFromSamples(bgm.minibossWav, generateMiniboss(loopSamples));
    for (size_t biome = 0; biome < bgm.megaboss.size(); biome++)
    {
        bgm.megaboss.at(biome) = loadMusicFromSamples(bgm.megabossWav.at(biome), generateBossTrack(loopSamples, biome));
    }
    bgm.swarmBoss = loadMusicFromSamples(bgm.swarmBossWav, generateSwarmBoss(loopSamples));
    return bgm;
}
