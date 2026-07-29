// Synth -- multi-track step sequencer with sound chooser and profiles
//   make synth && ./synth
//
// Left click grid   toggle note (add/remove)
// Right click note  cycle pitch up
// Scroll on note    cycle pitch down
// Click track name  select track
// Click sound name  open sound chooser on that track
// Click sound in    sound chooser assigns to selected track
// [+] / [-] buttons add/remove tracks (max 24)
// [>>] / [  ]       play/stop
// [Save] / [Load]   profile management
// [WAV]             export to wav

#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <numbers>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace
{

constexpr int sampleRate = 22050;
constexpr int maxTracks = 24;
constexpr const char* profileDir = "tools/profiles/";

// ── synth primitives ──────────────────────────────────────────
auto secToSamp(float s) -> size_t
{
    return static_cast<size_t>(std::lround(s * sampleRate));
}

constexpr float cutDecay = 6.0F;

auto noise() -> float
{
    static std::mt19937 rng{std::random_device{}()};
    static std::uniform_real_distribution<float> d(-1.0F, 1.0F);
    return d(rng);
}

void addTone(std::vector<float>& b, float t0, float freq, float amp, float decay)
{
    size_t s = secToSamp(t0);
    for (size_t i = s; i < b.size(); i++)
    {
        float e = static_cast<float>(i - s) / sampleRate;
        if (e > decay * cutDecay) break;
        b[i] += amp * std::exp(-e / decay) * std::sin(2.0F * std::numbers::pi_v<float> * freq * e);
    }
}

void addSquare(std::vector<float>& b, float t0, float freq, float amp, float decay)
{
    size_t s = secToSamp(t0);
    for (size_t i = s; i < b.size(); i++)
    {
        float e = static_cast<float>(i - s) / sampleRate;
        if (e > decay * cutDecay) break;
        float env = std::exp(-e / decay);
        float sq = std::sin(2.0F * std::numbers::pi_v<float> * freq * e) >= 0 ? 1.0F : -1.0F;
        b[i] += amp * env * sq;
    }
}

void addSaw(std::vector<float>& b, float t0, float freq, float amp, float decay)
{
    size_t s = secToSamp(t0);
    for (size_t i = s; i < b.size(); i++)
    {
        float e = static_cast<float>(i - s) / sampleRate;
        if (e > decay * cutDecay) break;
        b[i] += amp * std::exp(-e / decay) * (2.0F * (freq * e - std::floor(freq * e)) - 1.0F);
    }
}

void addNoise(std::vector<float>& b, float t0, float amp, float decay)
{
    size_t s = secToSamp(t0);
    for (size_t i = s; i < b.size(); i++)
    {
        float e = static_cast<float>(i - s) / sampleRate;
        if (e > decay * cutDecay) break;
        b[i] += amp * std::exp(-e / decay) * noise();
    }
}

void addKickSynth(std::vector<float>& b, float t0, float amp, float freq, float endFreq,
                  float sweepDur, float decay)
{
    size_t s = secToSamp(t0);
    for (size_t i = s; i < b.size(); i++)
    {
        float e = static_cast<float>(i - s) / sampleRate;
        if (e > decay * cutDecay) break;
        float sw = freq - (freq - endFreq) * std::min(e / sweepDur, 1.0F);
        b[i] += amp * std::exp(-e / decay) * std::sin(2.0F * std::numbers::pi_v<float> * sw * e);
    }
}

// ── for per-note synthesis ────────────────────────────────────
auto renderNote(int midiNote, int soundIdx, float vol, float decay, float bpm, int steps) -> std::vector<float>
{
    float freq = 440.0F * std::pow(2.0F, (midiNote - 69) / 12.0F);
    float bs = 60.0F / bpm;
    float maxLen = std::min(decay * cutDecay, bs * static_cast<float>(steps));
    int len = static_cast<int>(secToSamp(maxLen));
    if (len < 16) len = secToSamp(bs * 0.5F);
    std::vector<float> out(static_cast<size_t>(len), 0.0F);
    float amp = vol * 0.6F;

    // sound 0 = Kick, 1 = Sine, 2 = Square, 3 = Saw, 4 = Noise, 5+ = SFX (one-shot, handled separately)
    switch (soundIdx)
    {
    case 0: addKickSynth(out, 0, amp, freq, freq * 0.25F, 0.12F, decay); break;
    case 1: addTone(out, 0, freq, amp, decay); break;
    case 2: addSquare(out, 0, freq, amp, decay); break;
    case 3: addSaw(out, 0, freq, amp, decay); break;
    case 4: addNoise(out, 0, amp, decay); break;
    default: addTone(out, 0, freq, amp, decay); break;
    }
    return out;
}

// ── WAV helpers ────────────────────────────────────────────────
auto toPcm16(const std::vector<float>& s) -> std::vector<int16_t>
{
    float peak = 0;
    for (float v : s) peak = std::max(peak, std::abs(v));
    float gain = peak > 1.0F ? 1.0F / peak : 1.0F;
    std::vector<int16_t> out(s.size());
    for (size_t i = 0; i < s.size(); i++) out[i] = static_cast<int16_t>(s[i] * gain * 32000.0F);
    return out;
}

auto encodeWAV(const std::vector<int16_t>& s) -> std::vector<std::byte>
{
    uint32_t dataSz = static_cast<uint32_t>(s.size() * sizeof(int16_t));
    uint32_t rate = sampleRate;
    uint32_t hdr = 44;
    std::vector<std::byte> buf;
    buf.reserve(hdr + dataSz);
    auto wr = [&](const auto* p, size_t n) { auto* bp = reinterpret_cast<const std::byte*>(p); buf.insert(buf.end(), bp, bp + n); };
    auto t4 = [&](const char* t) { wr(t, 4); };
    auto u32 = [&](uint32_t v) { wr(&v, 4); };
    auto u16 = [&](uint16_t v) { wr(&v, 2); };
    t4("RIFF"); u32(hdr - 8 + dataSz); t4("WAVE");
    t4("fmt "); u32(16); u16(1); u16(1); u32(rate); u32(rate * 2); u16(2); u16(16);
    t4("data"); u32(dataSz); wr(s.data(), dataSz);
    return buf;
}

auto samplesToSound(const std::vector<float>& s, float vol = 0.5F) -> Sound
{
    auto wav = encodeWAV(toPcm16(s));
    auto* d = reinterpret_cast<const unsigned char*>(wav.data());
    Wave w = LoadWaveFromMemory(".wav", d, static_cast<int>(wav.size()));
    Sound sn = LoadSoundFromWave(w);
    UnloadWave(w);
    SetSoundVolume(sn, vol);
    return sn;
}

// ── note helpers ────────────────────────────────────────────────
const char* noteLabel(int n)
{
    static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    static char buf[16];
    snprintf(buf, sizeof(buf), "%s%d", names[n % 12], n / 12 - 1);
    return buf;
}

// ── sound registry ─────────────────────────────────────────────
struct SoundDef
{
    std::string name;
    int idx;      // index for lookups
    Sound sound;  // pre-loaded playback sound
    Color color;
};

constexpr Color soundColors[] = {
    {255, 100, 100, 255}, // 0 Kick
    {100, 200, 255, 255}, // 1 Sine
    {100, 255, 100, 255}, // 2 Square
    {255, 200, 100, 255}, // 3 Saw
    {200, 100, 255, 255}, // 4 Noise
    {200, 200, 100, 255}, // 5+ SFX
};

auto loadSFX(int idx, auto gen) -> Sound
{
    return samplesToSound(gen());
}

void initSounds(std::vector<SoundDef>& sounds)
{
    sounds.push_back({"Kick", 0, {}, soundColors[0]});
    sounds.push_back({"Sine", 1, {}, soundColors[1]});
    sounds.push_back({"Square", 2, {}, soundColors[2]});
    sounds.push_back({"Saw", 3, {}, soundColors[3]});
    sounds.push_back({"Noise", 4, {}, soundColors[4]});

    auto addSfx = [&](const char* name, int idx, auto gen) {
        Sound s = loadSFX(idx, gen);
        sounds.push_back({name, idx, s, soundColors[5]});
    };

    addSfx("Shoot", 5, []() {
        std::vector<float> b(secToSamp(0.10F), 0);
        addSquare(b, 0, 900, 0.14F, 0.04F); addSquare(b, 0, 1400, 0.06F, 0.025F); return b;
    });
    addSfx("Hit", 6, []() {
        std::vector<float> b(secToSamp(0.15F), 0);
        addNoise(b, 0, 0.35F, 0.05F); addTone(b, 0, 180, 0.2F, 0.08F); return b;
    });
    addSfx("Explosion", 7, []() {
        std::vector<float> b(secToSamp(0.4F), 0);
        addKickSynth(b, 0, 0.35F, 140, 40, 0.12F, 0.20F);
        addNoise(b, 0, 0.4F, 0.15F); addNoise(b, 0.02F, 0.2F, 0.10F); return b;
    });
    addSfx("Menu Move", 8, []() {
        std::vector<float> b(secToSamp(0.08F), 0);
        addSquare(b, 0, 700, 0.15F, 0.04F); return b;
    });
    addSfx("Menu Confirm", 9, []() {
        std::vector<float> b(secToSamp(0.18F), 0);
        addSquare(b, 0, 700, 0.15F, 0.05F); addSquare(b, 0.06F, 1050, 0.18F, 0.08F); return b;
    });
    addSfx("Victory", 10, []() {
        std::vector<float> b(secToSamp(0.6F), 0);
        addTone(b, 0, 523.25F, 0.18F, 0.18F); addTone(b, 0.1F, 659.25F, 0.18F, 0.18F);
        addTone(b, 0.2F, 783.99F, 0.18F, 0.18F); addTone(b, 0.3F, 1046.50F, 0.18F, 0.18F);
        return b;
    });
    addSfx("Defeat", 11, []() {
        std::vector<float> b(secToSamp(0.8F), 0);
        addTone(b, 0, 392, 0.22F, 0.35F); addTone(b, 0.2F, 329.63F, 0.22F, 0.35F);
        addTone(b, 0.4F, 220, 0.22F, 0.35F); return b;
    });
    addSfx("Critical", 12, []() {
        std::vector<float> b(secToSamp(0.15F), 0);
        addSquare(b, 0, 1600, 0.22F, 0.05F); addSquare(b, 0, 2200, 0.12F, 0.04F); return b;
    });
    addSfx("Boss WindUp", 13, []() {
        std::vector<float> b(secToSamp(0.6F), 0);
        for (int i = 0; i < 12; i++) addSaw(b, i * 0.05F, 100 + i * 40, 0.1F, 0.08F);
        return b;
    });
    addSfx("Beam Fire", 14, []() {
        std::vector<float> b(secToSamp(0.25F), 0);
        addSaw(b, 0, 500, 0.2F, 0.20F); addSaw(b, 0, 505, 0.15F, 0.20F); return b;
    });
    addSfx("Homing Launch", 15, []() {
        std::vector<float> b(secToSamp(0.3F), 0);
        addNoise(b, 0, 0.15F, 0.15F); addKickSynth(b, 0, 0.2F, 300, 700, 0.15F, 0.15F); return b;
    });
    addSfx("Spread Burst", 16, []() {
        std::vector<float> b(secToSamp(0.15F), 0);
        addNoise(b, 0, 0.3F, 0.05F); addSquare(b, 0, 300, 0.2F, 0.06F); return b;
    });
    addSfx("Slam Boom", 17, []() {
        std::vector<float> b(secToSamp(0.5F), 0);
        addKickSynth(b, 0, 0.4F, 120, 30, 0.15F, 0.25F); addNoise(b, 0, 0.35F, 0.2F); return b;
    });
    addSfx("Nerve Charge", 18, []() {
        std::vector<float> b(secToSamp(0.35F), 0);
        addKickSynth(b, 0, 0.22F, 200, 900, 0.35F, 0.315F); addSaw(b, 0, 200, 0.05F, 0.35F);
        return b;
    });
    addSfx("Nerve Release", 19, []() {
        std::vector<float> b(secToSamp(0.4F), 0);
        addKickSynth(b, 0, 0.4F, 180, 50, 0.12F, 0.22F); addNoise(b, 0, 0.3F, 0.15F);
        addSquare(b, 0, 220, 0.15F, 0.18F); addSquare(b, 0, 330, 0.10F, 0.18F); return b;
    });
    addSfx("Nerve Fizzle", 20, []() {
        std::vector<float> b(secToSamp(0.2F), 0);
        addKickSynth(b, 0, 0.18F, 500, 120, 0.15F, 0.12F); addNoise(b, 0, 0.15F, 0.08F);
        return b;
    });
}

// ── data structures ─────────────────────────────────────────────
struct Note
{
    int step;
    int midiNote;
    float vol;
    float decay;
};

struct Track
{
    std::string name;
    int soundIdx; // index into global sounds[]
    bool muted;
    std::vector<Note> notes;
};

struct Profile
{
    std::string name;
    float bpm;
    int steps;
    std::vector<Track> tracks;
};

// ── profile save/load ─────────────────────────────────────────
auto saveProfileText(const Profile& p) -> std::string
{
    std::string s;
    s += "# Galaxy Impact Synth Profile\n";
    s += "name=" + p.name + "\n";
    s += "bpm=" + std::to_string(static_cast<int>(p.bpm)) + "\n";
    s += "steps=" + std::to_string(p.steps) + "\n";
    s += "tracks=" + std::to_string(p.tracks.size()) + "\n";
    for (auto& t : p.tracks)
    {
        s += "\n[track]\n";
        s += "name=" + t.name + "\n";
        s += "sound=" + std::to_string(t.soundIdx) + "\n";
        s += "muted=" + std::to_string(t.muted ? 1 : 0) + "\n";
        s += "notes=";
        for (size_t i = 0; i < t.notes.size(); i++)
        {
            if (i > 0) s += " ";
            auto& n = t.notes[i];
            s += std::to_string(n.step) + ":" + std::to_string(n.midiNote) + ":" +
                 std::to_string(n.vol) + ":" + std::to_string(n.decay);
        }
        s += "\n";
    }
    return s;
}

auto loadProfileText(const std::string& text) -> Profile
{
    Profile p;
    p.name = "New";
    p.bpm = 140;
    p.steps = 16;
    Track curTrack;
    bool inTrack = false;

    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line))
    {
        if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[')
        {
            if (inTrack) { p.tracks.push_back(curTrack); curTrack = {}; }
            inTrack = true;
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if (key == "name") p.name = val;
        else if (key == "bpm") p.bpm = std::stof(val);
        else if (key == "steps") p.steps = std::stoi(val);
        else if (key == "tracks") {} // count, ignore
        else if (inTrack)
        {
            if (key == "name") curTrack.name = val;
            else if (key == "sound") curTrack.soundIdx = std::stoi(val);
            else if (key == "muted") curTrack.muted = std::stoi(val) != 0;
            else if (key == "notes")
            {
                std::istringstream ns(val);
                std::string tok;
                while (ns >> tok)
                {
                    Note n{};
                    if (std::sscanf(tok.c_str(), "%d:%d:%f:%f", &n.step, &n.midiNote, &n.vol, &n.decay) >= 2)
                    {
                        if (n.vol < 0.01F) n.vol = 0.5F;
                        if (n.decay < 0.001F) n.decay = 0.15F;
                        curTrack.notes.push_back(n);
                    }
                }
            }
        }
    }
    if (inTrack) p.tracks.push_back(curTrack);
    return p;
}

// ── built-in BGM profiles ───────────────────────────────────────
const char* baseProfileText = R"(
name=Base BGM
bpm=150
steps=32
tracks=2

[track]
name=Pedal
sound=1
muted=0
notes=0:33:0.3:0.45 8:33:0.25:0.4 16:33:0.3:0.45 24:33:0.25:0.4

[track]
name=Motif
sound=1
muted=0
notes=0:69:0.2:0.18 2:72:0.2:0.18 4:74:0.2:0.18 6:72:0.2:0.18 8:67:0.2:0.18 10:69:0.2:0.18 12:72:0.2:0.18 14:69:0.2:0.18 16:69:0.2:0.18 18:72:0.2:0.18 20:74:0.2:0.18 22:72:0.2:0.18 24:67:0.2:0.18 26:69:0.2:0.18 28:72:0.2:0.18 30:69:0.2:0.18
)";

const char* intensityProfileText = R"(
name=Intensity BGM
bpm=150
steps=32
tracks=2

[track]
name=Melody
sound=1
muted=0
notes=0:72:0.25:0.2 2:76:0.25:0.2 4:79:0.25:0.2 6:76:0.25:0.2 8:74:0.25:0.2 10:72:0.25:0.2 12:69:0.25:0.2 14:72:0.25:0.2 16:72:0.25:0.2 18:76:0.25:0.2 20:79:0.25:0.2 22:76:0.25:0.2 24:74:0.25:0.2 26:72:0.25:0.2 28:69:0.25:0.2 30:72:0.25:0.2

[track]
name=HiHat
sound=4
muted=0
notes=0:72:0.12:0.03 2:72:0.12:0.03 4:72:0.12:0.03 6:72:0.12:0.03 8:72:0.12:0.03 10:72:0.12:0.03 12:72:0.12:0.03 14:72:0.12:0.03 16:72:0.12:0.03 18:72:0.12:0.03 20:72:0.12:0.03 22:72:0.12:0.03 24:72:0.12:0.03 26:72:0.12:0.03 28:72:0.12:0.03 30:72:0.12:0.03
)";

const char* minibossProfileText = R"(
name=Miniboss BGM
bpm=150
steps=32
tracks=5

[track]
name=Kick
sound=0
muted=0
notes=0:36:0.4:0.15 5:36:0.3:0.12 6:36:0.35:0.12 8:36:0.4:0.15 13:36:0.3:0.12 14:36:0.35:0.12 16:36:0.4:0.15 21:36:0.3:0.12 22:36:0.35:0.12 24:36:0.4:0.15 29:36:0.3:0.12 30:36:0.35:0.12

[track]
name=Snare
sound=4
muted=0
notes=2:42:0.35:0.08 10:42:0.35:0.08 18:42:0.35:0.08 26:42:0.35:0.08

[track]
name=Bass
sound=3
muted=0
notes=0:45:0.25:0.3 2:45:0.2:0.15 3:45:0.18:0.12 4:45:0.25:0.3 6:45:0.2:0.15 7:45:0.18:0.12 8:45:0.25:0.3 10:45:0.2:0.15 11:45:0.18:0.12 16:41:0.25:0.3 18:41:0.2:0.15 19:41:0.18:0.12 20:41:0.25:0.3 22:41:0.2:0.15 23:41:0.18:0.12 24:41:0.25:0.3 26:41:0.2:0.15 27:41:0.18:0.12

[track]
name=Arp
sound=2
muted=0
notes=0:57:0.15:0.15 1:60:0.15:0.15 2:64:0.15:0.15 3:69:0.15:0.15 4:72:0.15:0.15 5:69:0.15:0.15 6:64:0.15:0.15 7:60:0.15:0.15 8:57:0.15:0.15 9:60:0.15:0.15 10:64:0.15:0.15 11:69:0.15:0.15 12:72:0.15:0.15 13:69:0.15:0.15 14:64:0.15:0.15 15:60:0.15:0.15 16:53:0.15:0.15 17:56:0.15:0.15 18:60:0.15:0.15 19:65:0.15:0.15 20:69:0.15:0.15 21:65:0.15:0.15 22:60:0.15:0.15 23:56:0.15:0.15 24:53:0.15:0.15 25:56:0.15:0.15 26:60:0.15:0.15 27:65:0.15:0.15 28:69:0.15:0.15 29:65:0.15:0.15 30:60:0.15:0.15 31:56:0.15:0.15

[track]
name=Stabs
sound=1
muted=0
notes=0:57:0.12:0.25 0:60:0.1:0.25 0:64:0.1:0.25 4:57:0.1:0.25 4:60:0.08:0.25 4:64:0.08:0.25 12:57:0.12:0.25 12:60:0.1:0.25 12:64:0.1:0.25 20:53:0.12:0.25 20:56:0.1:0.25 20:60:0.1:0.25 28:53:0.12:0.25 28:56:0.1:0.25 28:60:0.1:0.25
)";

const char* megabossProfileText = R"(
name=Megaboss BGM
bpm=150
steps=32
tracks=5

[track]
name=Kick
sound=0
muted=0
notes=0:36:0.45:0.15 5:36:0.3:0.12 6:36:0.35:0.12 8:36:0.45:0.15 13:36:0.3:0.12 14:36:0.35:0.12 16:36:0.45:0.15 21:36:0.3:0.12 22:36:0.35:0.12 24:36:0.45:0.15 29:36:0.3:0.12 30:36:0.35:0.12

[track]
name=Snare
sound=4
muted=0
notes=2:42:0.4:0.08 10:42:0.4:0.08 18:42:0.4:0.08 26:42:0.4:0.08

[track]
name=Pad
sound=1
muted=0
notes=0:53:0.15:0.3 0:56:0.12:0.3 0:60:0.12:0.3 4:53:0.12:0.25 4:56:0.1:0.25 4:60:0.1:0.25 8:59:0.15:0.3 8:62:0.12:0.3 8:65:0.12:0.3 12:59:0.12:0.25 12:62:0.1:0.25 12:65:0.1:0.25 16:52:0.15:0.3 16:56:0.12:0.3 16:59:0.12:0.3 20:52:0.12:0.25 20:56:0.1:0.25 20:59:0.1:0.25 24:62:0.15:0.3 24:65:0.12:0.3 24:69:0.12:0.3 28:62:0.12:0.25 28:65:0.1:0.25 28:69:0.1:0.25

[track]
name=Bass
sound=3
muted=0
notes=0:41:0.25:0.3 4:41:0.2:0.2 8:47:0.25:0.3 12:47:0.2:0.2 16:40:0.25:0.3 20:40:0.2:0.2 24:47:0.25:0.3 28:47:0.2:0.2

[track]
name=Arp
sound=2
muted=0
notes=0:53:0.12:0.15 1:56:0.12:0.15 2:60:0.12:0.15 3:65:0.12:0.15 4:69:0.12:0.15 5:65:0.12:0.15 6:60:0.12:0.15 7:56:0.12:0.15 8:59:0.12:0.15 9:62:0.12:0.15 10:66:0.12:0.15 11:71:0.12:0.15 12:74:0.12:0.15 13:71:0.12:0.15 14:66:0.12:0.15 15:62:0.12:0.15 16:52:0.12:0.15 17:56:0.12:0.15 18:59:0.12:0.15 19:64:0.12:0.15 20:69:0.12:0.15 21:64:0.12:0.15 22:59:0.12:0.15 23:56:0.12:0.15 24:62:0.12:0.15 25:65:0.12:0.15 26:69:0.12:0.15 27:74:0.12:0.15 28:76:0.12:0.15 29:74:0.12:0.15 30:69:0.12:0.15 31:65:0.12:0.15
)";

const char* swarmbossProfileText = R"(
name=SwarmBoss BGM
bpm=150
steps=32
tracks=4

[track]
name=Pedal
sound=1
muted=0
notes=0:24:0.25:2.0

[track]
name=Dirge
sound=2
muted=0
notes=0:57:0.15:0.35 2:56:0.15:0.35 4:55:0.15:0.35 6:54:0.15:0.35 8:53:0.15:0.35 10:52:0.15:0.35 12:51:0.15:0.35 14:50:0.15:0.35 16:49:0.15:0.35 18:48:0.15:0.35 20:47:0.15:0.35 22:46:0.15:0.35 24:45:0.15:0.35 26:44:0.15:0.35 28:43:0.15:0.35 30:42:0.15:0.35

[track]
name=Drone
sound=1
muted=0
notes=0:45:0.12:2.0 0:48:0.09:2.0 0:51:0.08:2.0 0:54:0.07:2.0 0:57:0.06:2.0 0:60:0.05:2.0 0:63:0.04:2.0 0:66:0.04:2.0

[track]
name=Tremolo
sound=2
muted=0
notes=0:49:0.06:0.35 1:49:0.04:0.35 2:49:0.06:0.35 3:49:0.04:0.35 4:49:0.06:0.35 5:49:0.04:0.35 6:49:0.06:0.35 7:49:0.04:0.35 8:49:0.06:0.35 9:49:0.04:0.35 10:49:0.06:0.35 11:49:0.04:0.35 12:49:0.06:0.35 13:49:0.04:0.35 14:49:0.06:0.35 15:49:0.04:0.35 16:49:0.06:0.35 17:49:0.04:0.35 18:49:0.06:0.35 19:49:0.04:0.35 20:49:0.06:0.35 21:49:0.04:0.35 22:49:0.06:0.35 23:49:0.04:0.35 24:49:0.06:0.35 25:49:0.04:0.35 26:49:0.06:0.35 27:49:0.04:0.35 28:49:0.06:0.35 29:49:0.04:0.35 30:49:0.06:0.35 31:49:0.04:0.35
)";

const char* builtinProfiles[] = {baseProfileText, intensityProfileText, minibossProfileText, megabossProfileText, swarmbossProfileText};
const char* builtinNames[] = {"Base BGM", "Intensity BGM", "Miniboss BGM", "Megaboss BGM", "SwarmBoss BGM"};

struct ProfileEntry
{
    std::string name;
    bool isBuiltin;
    int builtinIdx;      // index into builtinProfiles[] if isBuiltin
    std::string path;    // file path if file profile, empty if builtin
};

// ── state ──────────────────────────────────────────────────────
struct SynthState
{
    Profile profile;
    int selectedTrack = 0;
    int savedSteps = 0;  // for step count resizing

    // playback
    bool playing = false;
    int curStep = 0;
    float beatTimer = 0;
    int lastPlayedStep = -1;
    Sound chSounds[maxTracks]{};
    bool chPlaying[maxTracks]{};

    // UI interaction state
    Vector2 mouse{};
    int hoverTrack = -1;
    int hoverStep = -1;
    bool dragging = false;
    int soundChooserTrack = -1; // -1 = closed, >=0 = track index being edited
    int soundHover = -1;
    std::string statusMsg;
    float statusTimer = 0;

    // profile management
    std::vector<ProfileEntry> profileEntries;
    int profileScroll = 0;     // scroll offset for profile list
    int profileHover = -1;     // index of hovered profile entry
    int profileSectionH = 180; // height of profile section in left panel
};

void scanProfiles(std::vector<ProfileEntry>& entries)
{
    entries.clear();
    for (int i = 0; i < 5; i++)
        entries.push_back({builtinNames[i], true, i, {}});
    auto d = LoadDirectoryFiles(profileDir);
    for (int i = 0; i < d.count; i++)
    {
        std::string fn = d.paths[i];
        if (fn.size() > 6 && fn.substr(fn.size() - 6) == ".synth")
        {
            std::string name;
            auto slash = fn.rfind('/');
            name = (slash != std::string::npos) ? fn.substr(slash + 1) : fn;
            if (name.size() > 6) name = name.substr(0, name.size() - 6);
            entries.push_back({name, false, -1, fn});
        }
    }
    UnloadDirectoryFiles(d);
}

// ── helpers ────────────────────────────────────────────────────
bool isSFX(int idx) { return idx >= 5; }

bool isWaveform(int idx) { return idx >= 0 && idx <= 4; }

const char* soundCategory(int idx)
{
    if (idx <= 4) return "WAVEFORMS";
    return "SFX";
}

Color noteColorForSound(int soundIdx)
{
    if (soundIdx >= 0 && soundIdx <= 5) return soundColors[soundIdx];
    return soundColors[5];
}

} // namespace

auto main() -> int
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1100, 720, "Synth - Galaxy Impact Music Composer");
    InitAudioDevice();
    SetTargetFPS(60);
    SetMasterVolume(0.5F);

    std::vector<SoundDef> sounds;
    initSounds(sounds);

    SynthState st;

    // start with a simple default profile
    {
        Profile p;
        p.name = "New Song";
        p.bpm = 140;
        p.steps = 16;
        p.tracks.resize(4);
        p.tracks[0] = {"Kick", 0, false, {{0,36,0.7F,0.12F},{4,36,0.7F,0.12F},{8,36,0.7F,0.12F},{12,36,0.7F,0.12F}}};
        p.tracks[1] = {"Snare", 4, false, {{2,42,0.4F,0.08F},{6,42,0.4F,0.08F},{10,42,0.4F,0.08F},{14,42,0.4F,0.08F}}};
        p.tracks[2] = {"HiHat", 4, false, {{0,42,0.12F,0.03F},{4,42,0.12F,0.03F},{8,42,0.12F,0.03F},{12,42,0.12F,0.03F}}};
        p.tracks[3] = {"Bass", 3, false, {{0,45,0.25F,0.3F},{4,45,0.2F,0.15F},{8,45,0.25F,0.3F},{12,45,0.2F,0.15F}}};
        st.profile = std::move(p);
    }

    scanProfiles(st.profileEntries);

    Font font = GetFontDefault();

    while (!WindowShouldClose())
    {
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        st.mouse = GetMousePosition();
        bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        bool rclick = IsMouseButtonPressed(MOUSE_RIGHT_BUTTON);
        float scroll = GetMouseWheelMove();
        int key = GetKeyPressed();

        // ── layout ────────────────────────────────────────────
        int leftW = 140;
        int rightW = 180;
        int topH = 44;
        int bottomH = 32;
        int gridPad = 12;

        int gridX = leftW + gridPad;
        int gridY = topH + st.profileSectionH + 4;
        int gridW = w - leftW - rightW - gridPad * 2;
        int gridH = h - topH - bottomH - 20;

        int cellW = std::max(16, std::min(40, (gridW - 8) / st.profile.steps));
        int cellH = 22;
        int trackRowH = cellH + 4;

        int gridAreaW = st.profile.steps * cellW + 8;
        int gridAreaH = static_cast<int>(st.profile.tracks.size()) * trackRowH;

        int rightX = w - rightW;
        int leftX = 0;

        // scroll for grid + track panel
        static int scrollY = 0;
        if (gridAreaH > gridH)
        {
            int maxScroll = gridAreaH - gridH;
            if (IsKeyDown(KEY_DOWN)) scrollY = std::min(scrollY + 8, maxScroll);
            if (IsKeyDown(KEY_UP)) scrollY = std::max(scrollY - 8, 0);
            // mouse wheel scrolls track list area
            if (scroll != 0 && st.mouse.x < rightX)
                scrollY = std::clamp(scrollY - static_cast<int>(scroll) * 20, 0, maxScroll);
        }
        else scrollY = 0;

        // ── hit testing ──────────────────────────────────────
        st.hoverTrack = -1;
        st.hoverStep = -1;
        st.soundHover = -1;

        // grid cells
        int gx0 = gridX + 4;
        int gy0 = gridY - scrollY;
        for (int t = 0; t < static_cast<int>(st.profile.tracks.size()); t++)
        {
            int ry = gy0 + t * trackRowH;
            if (ry + cellH < gridY || ry > gridY + gridH) continue;
            for (int s = 0; s < st.profile.steps; s++)
            {
                int rx = gx0 + s * cellW;
                if (st.mouse.x >= rx && st.mouse.x < rx + cellW &&
                    st.mouse.y >= ry && st.mouse.y < ry + cellH)
                {
                    st.hoverTrack = t;
                    st.hoverStep = s;
                }
            }
        }

        // track panel hit test
        int trackPanelX = 8;
        int trackPanelY = topH + st.profileSectionH + 4 - scrollY;
        for (int t = 0; t < static_cast<int>(st.profile.tracks.size()); t++)
        {
            int ry = trackPanelY + t * trackRowH;
            if (ry + cellH < topH + 10 || ry > h - bottomH) continue;
            // track name area
            if (st.mouse.x >= trackPanelX && st.mouse.x < trackPanelX + leftW - 30 &&
                st.mouse.y >= ry && st.mouse.y < ry + cellH)
            {
                if (clicked) { st.selectedTrack = t; st.soundChooserTrack = -1; }
            }
            // mute button area
            int mx = trackPanelX + leftW - 26;
            if (st.mouse.x >= mx && st.mouse.x < mx + 22 &&
                st.mouse.y >= ry && st.mouse.y < ry + cellH)
            {
                if (clicked) { st.profile.tracks[t].muted = !st.profile.tracks[t].muted; }
            }
            // sound name area
            int sx = trackPanelX + 4;
            if (st.mouse.x >= sx && st.mouse.x < sx + leftW - 30 &&
                st.mouse.y >= ry + 2 && st.mouse.y < ry + 12)
            {
                if (clicked)
                {
                    st.soundChooserTrack = (st.soundChooserTrack == t) ? -1 : t;
                    st.selectedTrack = t;
                }
            }
        }

        // ── grid click ────────────────────────────────────────
        if (clicked && st.hoverTrack >= 0 && st.hoverStep >= 0)
        {
            Track& tr = st.profile.tracks[st.hoverTrack];
            auto it = std::find_if(tr.notes.begin(), tr.notes.end(),
                [&](Note& n) { return n.step == st.hoverStep; });
            if (it != tr.notes.end())
            {
                tr.notes.erase(it);
            }
            else
            {
                Note n;
                n.step = st.hoverStep;
                n.midiNote = 60; // default C4
                n.vol = 0.5F;
                n.decay = 0.15F;
                // set a nice default based on track index
                int rootNotes[] = {36, 42, 48, 60, 36, 42, 48, 60, 72, 84};
                n.midiNote = rootNotes[st.hoverTrack % 10];
                tr.notes.push_back(n);
                st.dragging = true;
            }
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) st.dragging = false;
        if (st.dragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && st.hoverTrack >= 0 && st.hoverStep >= 0)
        {
            Track& tr = st.profile.tracks[st.hoverTrack];
            auto it = std::find_if(tr.notes.begin(), tr.notes.end(),
                [&](Note& n) { return n.step == st.hoverStep; });
            if (it == tr.notes.end())
            {
                Note n;
                n.step = st.hoverStep;
                n.midiNote = 60;
                n.vol = 0.5F;
                n.decay = 0.15F;
                tr.notes.push_back(n);
            }
        }

        // right-click or scroll on note to change pitch
        if (st.hoverTrack >= 0 && st.hoverStep >= 0)
        {
            Track& tr = st.profile.tracks[st.hoverTrack];
            auto it = std::find_if(tr.notes.begin(), tr.notes.end(),
                [&](Note& n) { return n.step == st.hoverStep; });
            if (it != tr.notes.end())
            {
                if (rclick) it->midiNote = std::min(it->midiNote + 2, 127);
                if (scroll > 0) it->midiNote = std::min(it->midiNote + 2, 127);
                if (scroll < 0) it->midiNote = std::max(it->midiNote - 2, 0);
            }
        }

        // ── sound chooser click ─────────────────────────────
        if (st.soundChooserTrack >= 0 && st.soundChooserTrack < static_cast<int>(st.profile.tracks.size()))
        {
            int sy = topH + 14;
            int ssy = sy;
            // hover + click on sound list
            int soundIdx = 0;
            for (auto& sd : sounds)
            {
                if (st.mouse.x >= rightX + 4 && st.mouse.x < rightX + rightW - 4 &&
                    st.mouse.y >= ssy && st.mouse.y < ssy + 18)
                {
                    st.soundHover = soundIdx;
                    if (clicked)
                    {
                        st.profile.tracks[st.soundChooserTrack].soundIdx = soundIdx;
                        st.soundChooserTrack = -1;
                        st.statusMsg = "Assigned " + sd.name;
                        st.statusTimer = 2.0F;
                    }
                }
                ssy += 18;
                soundIdx++;
            }
        }

        // ── top bar buttons ────────────────────────────────────
        int btnY = 6;
        int btnH = 32;

        auto button = [&](int& x, int bw, const char* label, bool enabled) -> bool
        {
            Rectangle r = {(float)x, (float)btnY, (float)bw, (float)btnH};
            bool hover = CheckCollisionPointRec(st.mouse, r);
            Color c = {40, 40, 50, 255};
            if (!enabled) c = {20, 20, 25, 255};
            else if (hover) c = {70, 70, 90, 255};
            DrawRectangleRec(r, c);
            DrawText(label, x + 6, btnY + 8, 13, enabled ? (hover ? WHITE : LIGHTGRAY) : DARKGRAY);
            return enabled && hover && clicked;
        };

        int bx = 10;
        auto playBtn = [&]() {
            Rectangle r = {(float)bx, (float)btnY, 44, (float)btnH};
            bool hover = CheckCollisionPointRec(st.mouse, r);
            Color c = st.playing ? (Color){60, 80, 50, 255} : (Color){40, 50, 40, 255};
            if (hover) c = st.playing ? (Color){80, 110, 70, 255} : (Color){60, 80, 60, 255};
            DrawRectangleRec(r, c);
            DrawText(st.playing ? "[  ]" : "[>>]", bx + 6, btnY + 8, 12, GREEN);
            return hover && clicked;
        };
        if (playBtn())
        {
            st.playing = !st.playing;
            if (st.playing) { st.beatTimer = 0; st.curStep = 0; st.lastPlayedStep = -1; }
        }
        bx += 50;

        // BPM control
        DrawText("BPM:", bx, btnY + 8, 13, WHITE); bx += 36;
        DrawText(TextFormat("%.0f", st.profile.bpm), bx, btnY + 8, 13, YELLOW); bx += 32;
        if (button(bx, 20, "+", st.profile.bpm < 300)) { st.profile.bpm = std::min(st.profile.bpm + 5, 300.0F); }
        bx += 22;
        if (button(bx, 20, "-", st.profile.bpm > 30)) { st.profile.bpm = std::max(st.profile.bpm - 5, 30.0F); }
        bx += 28;

        // Steps control
        DrawText("Steps:", bx, btnY + 8, 13, WHITE); bx += 44;
        DrawText(TextFormat("%d", st.profile.steps), bx, btnY + 8, 13, YELLOW); bx += 30;
        static int stepsOptions[] = {8, 16, 24, 32, 48, 64};
        auto findSteps = [&]() {
            for (int i = 0; i < 6; i++) if (stepsOptions[i] == st.profile.steps) return i;
            return 1;
        };
        int curStepsIdx = findSteps();
        if (button(bx, 20, "+", curStepsIdx < 5))
        {
            int newSteps = stepsOptions[curStepsIdx + 1];
            st.profile.steps = newSteps;
        }
        bx += 22;
        if (button(bx, 20, "-", curStepsIdx > 0))
        {
            int newSteps = stepsOptions[curStepsIdx - 1];
            st.profile.steps = newSteps;
        }
        bx += 28;

        // Profile name
        DrawText("Song:", bx, btnY + 8, 13, WHITE); bx += 40;
        DrawText(st.profile.name.c_str(), bx, btnY + 8, 13, LIGHTGRAY); bx += 20;

        // ── bottom bar buttons ──────────────────────────────────
        int bbY = h - bottomH + 6;

        auto bottomBtn = [&](int& x, int bw, const char* label) -> bool
        {
            Rectangle r = {(float)x, (float)bbY, (float)bw, (float)(bottomH - 8)};
            bool hover = CheckCollisionPointRec(st.mouse, r);
            Color c = hover ? (Color){60, 60, 80, 255} : (Color){30, 30, 40, 255};
            DrawRectangleRec(r, c);
            DrawText(label, x + 6, bbY + 4, 12, hover ? WHITE : LIGHTGRAY);
            return hover && clicked;
        };

        // ── Save ────────────────────────────────────────────────
        int bbx = 10;
        if (bottomBtn(bbx, 60, "Save"))
        {
            std::string path = std::string(profileDir) + st.profile.name + ".synth";
            std::ofstream f(path);
            if (f) { f << saveProfileText(st.profile); st.statusMsg = "Saved: " + path; st.statusTimer = 3.0F; }
            else { st.statusMsg = "Save failed!"; st.statusTimer = 2.0F; }
        }
        bbx += 66;

        // ── New Profile ─────────────────────────────────────────
        if (bottomBtn(bbx, 50, "New"))
        {
            Profile p;
            p.name = "New Song " + std::to_string(static_cast<int>(st.profileEntries.size() + 1));
            p.bpm = 140;
            p.steps = 16;
            p.tracks.resize(4);
            p.tracks[0] = {"Kick", 0, false, {{0,36,0.7F,0.12F},{4,36,0.7F,0.12F},{8,36,0.7F,0.12F},{12,36,0.7F,0.12F}}};
            p.tracks[1] = {"Snare", 4, false, {{2,42,0.4F,0.08F},{6,42,0.4F,0.08F},{10,42,0.4F,0.08F},{14,42,0.4F,0.08F}}};
            p.tracks[2] = {"HiHat", 4, false, {{0,42,0.12F,0.03F},{4,42,0.12F,0.03F},{8,42,0.12F,0.03F},{12,42,0.12F,0.03F}}};
            p.tracks[3] = {"Bass", 3, false, {{0,45,0.25F,0.3F},{4,45,0.2F,0.15F},{8,45,0.25F,0.3F},{12,45,0.2F,0.15F}}};
            st.profile = std::move(p);
            st.statusMsg = "New profile created";
            st.statusTimer = 2.0F;
        }
        bbx += 56;

        // ── Export WAV ──────────────────────────────────────────
        if (bottomBtn(bbx, 70, "Export WAV"))
        {
            float bs = 60.0F / st.profile.bpm;
            float totalLen = bs * st.profile.steps + 0.5F;
            std::vector<float> accum(secToSamp(totalLen), 0);
            for (auto& tr : st.profile.tracks)
            {
                if (tr.muted) continue;
                for (auto& n : tr.notes)
                {
                    if (isSFX(tr.soundIdx)) continue;
                    float freq = 440.0F * std::pow(2.0F, (n.midiNote - 69) / 12.0F);
                    float amp = n.vol * 0.5F;
                    float t0 = static_cast<float>(n.step) * bs;
                    switch (tr.soundIdx)
                    {
                    case 0: addKickSynth(accum, t0, amp, freq, freq*0.25F, 0.12F, n.decay); break;
                    case 1: addTone(accum, t0, freq, amp, n.decay); break;
                    case 2: addSquare(accum, t0, freq, amp, n.decay); break;
                    case 3: addSaw(accum, t0, freq, amp, n.decay); break;
                    case 4: addNoise(accum, t0, amp, n.decay); break;
                    }
                }
            }
            auto wav = encodeWAV(toPcm16(accum));
            std::string wavPath = st.profile.name + ".wav";
            FILE* f = fopen(wavPath.c_str(), "wb");
            if (f) { fwrite(wav.data(), 1, wav.size(), f); fclose(f); st.statusMsg = "Exported: " + wavPath; st.statusTimer = 3.0F; }
            else { st.statusMsg = "Export failed!"; st.statusTimer = 2.0F; }
        }
        bbx += 76;

        // ── Add/Remove tracks ──────────────────────────────────
        if (bottomBtn(bbx, 50, "Add T"))
        {
            if (st.profile.tracks.size() < maxTracks)
            {
                int idx = static_cast<int>(st.profile.tracks.size());
                Track t;
                t.name = "Track" + std::to_string(idx + 1);
                t.soundIdx = idx % static_cast<int>(sounds.size());
                t.muted = false;
                st.profile.tracks.push_back(std::move(t));
            }
        }
        bbx += 56;
        if (bottomBtn(bbx, 70, "Del T"))
        {
            if (st.profile.tracks.size() > 1 && st.selectedTrack < static_cast<int>(st.profile.tracks.size()))
            {
                st.profile.tracks.erase(st.profile.tracks.begin() + st.selectedTrack);
                st.selectedTrack = std::min(st.selectedTrack, static_cast<int>(st.profile.tracks.size()) - 1);
            }
        }
        bbx += 76;

        // ── status message ──────────────────────────────────
        if (st.statusTimer > 0)
        {
            st.statusTimer -= GetFrameTime();
            DrawText(st.statusMsg.c_str(), bbx, bbY + 4, 12, GREEN);
        }

        // ── keyboard shortcuts ─────────────────────────────────
        if (key == KEY_SPACE)
        {
            st.playing = !st.playing;
            if (st.playing) { st.beatTimer = 0; st.curStep = 0; st.lastPlayedStep = -1; }
        }
        if (key == KEY_ESCAPE) break;
        if (key == KEY_TAB && st.profile.tracks.size() > 0)
            st.selectedTrack = (st.selectedTrack + 1) % static_cast<int>(st.profile.tracks.size());
        if (key == KEY_M && st.selectedTrack < static_cast<int>(st.profile.tracks.size()))
            st.profile.tracks[st.selectedTrack].muted = !st.profile.tracks[st.selectedTrack].muted;

        // Delete note at curStep+selectedTrack
        if (key == KEY_DELETE || key == KEY_BACKSPACE)
        {
            if (st.selectedTrack < static_cast<int>(st.profile.tracks.size()))
            {
                auto& notes = st.profile.tracks[st.selectedTrack].notes;
                notes.erase(std::remove_if(notes.begin(), notes.end(),
                    [&](Note& n) { return n.step == st.curStep; }), notes.end());
            }
        }

        // ── profile list interaction ─────────────────────────
        {
            int pRowH = 18;
            int pButtonsH = 24;
            int pListH = st.profileSectionH - 18 - pButtonsH - 8;
            int pCount = static_cast<int>(st.profileEntries.size());
            int pListMax = std::max(1, pListH / pRowH);

            if (pCount > pListMax)
            {
                if (scroll != 0 && st.mouse.x < leftW)
                    st.profileScroll = std::clamp(st.profileScroll - static_cast<int>(scroll), 0, pCount - pListMax);
            }
            else st.profileScroll = 0;

            int pY0 = topH + 20;
            st.profileHover = -1;
            for (int i = st.profileScroll; i < pCount && i < st.profileScroll + pListMax; i++)
            {
                int ry = pY0 + (i - st.profileScroll) * pRowH;
                auto& pe = st.profileEntries[i];
                bool isCurrent = (pe.name == st.profile.name);
                if (st.mouse.x >= 8 && st.mouse.x < leftW - 4 &&
                    st.mouse.y >= ry && st.mouse.y < ry + pRowH)
                {
                    st.profileHover = i;
                    if (clicked && !isCurrent)
                    {
                        if (pe.isBuiltin)
                        {
                            st.profile = loadProfileText(builtinProfiles[pe.builtinIdx]);
                            st.statusMsg = "Loaded: " + pe.name;
                        }
                        else
                        {
                            std::ifstream f(pe.path);
                            if (f)
                            {
                                std::string txt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                                st.profile = loadProfileText(txt);
                                st.statusMsg = "Loaded: " + pe.name;
                            }
                        }
                        st.statusTimer = 2.0F;
                        st.playing = false;
                        st.selectedTrack = 0;
                    }
                }
            }

            // profile button hit testing (no drawing here)
            int pbY = topH + st.profileSectionH - pButtonsH - 2;
            int mbx = 6;
            auto btnHit = [&](int x, int bw) -> bool {
                return clicked && st.mouse.x >= x && st.mouse.x < x + bw &&
                       st.mouse.y >= pbY && st.mouse.y < pbY + pButtonsH - 2;
            };
            if (btnHit(mbx, 46))
            {
                std::string path = std::string(profileDir) + st.profile.name + ".synth";
                std::ofstream f(path);
                if (f) { f << saveProfileText(st.profile); st.statusMsg = "Saved: " + st.profile.name; }
                else { st.statusMsg = "Save failed!"; }
                st.statusTimer = 2.0F;
                scanProfiles(st.profileEntries);
            }
            mbx += 50;
            if (btnHit(mbx, 46))
            {
                scanProfiles(st.profileEntries);
                st.statusMsg = "Profiles refreshed";
                st.statusTimer = 1.5F;
            }
            mbx += 50;
            if (btnHit(mbx, 46))
            {
                st.profile.name = st.profile.name + " (2)";
                st.statusMsg = "Renamed to: " + st.profile.name;
                st.statusTimer = 2.0F;
            }
        }

        // ── real-time sequencer ────────────────────────────────
        if (st.playing && st.profile.steps > 0)
        {
            float bs = 60.0F / st.profile.bpm;
            st.beatTimer += GetFrameTime();
            int newStep = static_cast<int>(st.beatTimer / bs) % st.profile.steps;

            if (newStep != st.lastPlayedStep)
            {
                st.lastPlayedStep = newStep;
                st.curStep = newStep;

                for (int t = 0; t < static_cast<int>(st.profile.tracks.size()); t++)
                {
                    auto& tr = st.profile.tracks[t];
                    if (tr.muted) continue;

                    for (auto& n : tr.notes)
                    {
                        if (n.step != newStep) continue;

                        if (isSFX(tr.soundIdx))
                        {
                            int sfxIdx = tr.soundIdx - 5;
                            if (sfxIdx >= 0 && sfxIdx + 5 < static_cast<int>(sounds.size()))
                            {
                                PlaySound(sounds[sfxIdx + 5].sound);
                            }
                        }
                        else
                        {
                            auto samples = renderNote(n.midiNote, tr.soundIdx, n.vol, n.decay,
                                                       st.profile.bpm, st.profile.steps);
                            if (samples.empty()) continue;

                            if (st.chPlaying[t] && IsSoundPlaying(st.chSounds[t]))
                            {
                                StopSound(st.chSounds[t]);
                                UnloadSound(st.chSounds[t]);
                                st.chPlaying[t] = false;
                            }
                            st.chSounds[t] = samplesToSound(samples, 0.4F);
                            st.chPlaying[t] = true;
                            PlaySound(st.chSounds[t]);
                        }
                    }
                }
            }

            for (int t = 0; t < static_cast<int>(st.profile.tracks.size()); t++)
            {
                if (st.chPlaying[t] && !IsSoundPlaying(st.chSounds[t]))
                {
                    UnloadSound(st.chSounds[t]);
                    st.chPlaying[t] = false;
                }
            }
        }

        // ── draw ────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground({8, 8, 12, 255});

        // top bar bg
        DrawRectangle(0, 0, w, topH, {15, 15, 22, 255});

        // ── left panel ─────────────────────────────────────────
        DrawRectangle(leftX, topH, leftW, h - topH - bottomH, {12, 12, 18, 255});

        // ── profile section ───────────────────────────────────
        int pRowH = 18;
        int pHeaderH = 18;
        int pButtonsH = 24;
        int pListH = st.profileSectionH - pHeaderH - pButtonsH - 8;
        int pCount = static_cast<int>(st.profileEntries.size());
        int pListMax = std::max(1, pListH / pRowH);

        // profile section bg
        DrawRectangle(leftX, topH, leftW, st.profileSectionH, {18, 16, 22, 255});
        DrawText("PROFILES", 8, topH + 2, 10, DARKGRAY);

        int pY0 = topH + pHeaderH + 2;
        for (int i = st.profileScroll; i < pCount && i < st.profileScroll + pListMax; i++)
        {
            auto& pe = st.profileEntries[i];
            int ry = pY0 + (i - st.profileScroll) * pRowH;
            bool isCurrent = (pe.name == st.profile.name);
            bool hov = (st.profileHover == i);

            Color bg = isCurrent ? (Color){40, 60, 50, 255} : (Color){0, 0, 0, 0};
            if (hov && !isCurrent) bg = {30, 30, 42, 255};
            DrawRectangle(4, ry, leftW - 8, pRowH - 1, bg);

            Color tc = pe.isBuiltin ? (Color){200, 200, 100, 255} : LIGHTGRAY;
            if (isCurrent) tc = WHITE;
            DrawText(pe.name.c_str(), 10, ry + 2, 10, tc);

            if (pe.isBuiltin)
                DrawText("B", leftW - 16, ry + 2, 8, {150, 150, 80, 255});
        }

        // profile buttons
        {
            int pbY = topH + st.profileSectionH - 26;
            int mbx = 6;
            auto drawBtn = [&](int x, int bw, const char* label, bool hl) {
                Rectangle r = {(float)x, (float)pbY, (float)bw, 22};
                DrawRectangleRec(r, hl ? (Color){65, 65, 85, 255} : (Color){28, 28, 42, 255});
                DrawText(label, x + 4, pbY + 5, 10, hl ? WHITE : LIGHTGRAY);
            };
            drawBtn(mbx, 46, "Save", st.mouse.x >= mbx && st.mouse.x < mbx+46 && st.mouse.y >= pbY && st.mouse.y < pbY+22); mbx += 50;
            drawBtn(mbx, 46, "Refr", st.mouse.x >= mbx && st.mouse.x < mbx+46 && st.mouse.y >= pbY && st.mouse.y < pbY+22); mbx += 50;
            drawBtn(mbx, 46, "Rnm",  st.mouse.x >= mbx && st.mouse.x < mbx+46 && st.mouse.y >= pbY && st.mouse.y < pbY+22);
        }

        // separator
        DrawRectangle(4, topH + st.profileSectionH, leftW - 8, 1, {30, 30, 45, 255});

        // ── track section ────────────────────────────────────
        int trackSecY = topH + st.profileSectionH + 6;
        DrawText("TRACKS", 8, trackSecY - 2, 10, DARKGRAY);

        for (int t = 0; t < static_cast<int>(st.profile.tracks.size()); t++)
        {
            auto& tr = st.profile.tracks[t];
            int ry = trackPanelY + t * trackRowH;
            if (ry + cellH < topH + 10 || ry > h - bottomH) continue;

            bool sel = (t == st.selectedTrack);
            bool hov = (t == st.hoverTrack);

            // track row bg
            Color rowBg = sel ? (Color){40, 50, 60, 255} : (Color){0, 0, 0, 0};
            if (hov && !sel) rowBg = {25, 30, 35, 255};
            DrawRectangle(trackPanelX, ry, leftW - 4, cellH, rowBg);

            // track number
            DrawText(TextFormat("%d", t + 1), trackPanelX + 2, ry + 4, 10, sel ? WHITE : GRAY);

            // sound name (clickable)
            int soundIdx = tr.soundIdx;
            if (soundIdx >= 0 && soundIdx < static_cast<int>(sounds.size()))
            {
                Color scol = sounds[soundIdx].color;
                DrawRectangle(trackPanelX + 14, ry + 3, 6, 6, scol);
                const char* sn = sounds[soundIdx].name.c_str();
                DrawText(sn, trackPanelX + 22, ry + 3, 9, sel ? WHITE : LIGHTGRAY);
            }

            // mute indicator
            if (tr.muted)
            {
                DrawText("M", trackPanelX + leftW - 24, ry + 4, 10, RED);
            }
            else
            {
                DrawText(".", trackPanelX + leftW - 22, ry + 2, 12, DARKGRAY);
            }

            // step count markers on first track
            if (t == 0)
            {
                for (int s = 0; s < st.profile.steps; s++)
                {
                    if (s % 4 == 0)
                    {
                        int rx2 = gx0 + s * cellW;
                        DrawText(TextFormat("%d", s), rx2 + 2, gridY - 6, 8, DARKGRAY);
                    }
                }
            }
        }

        // ── grid ────────────────────────────────────────────────
        for (int t = 0; t < static_cast<int>(st.profile.tracks.size()); t++)
        {
            auto& tr = st.profile.tracks[t];
            int ry = gy0 + t * trackRowH;
            if (ry + cellH < gridY || ry > gridY + gridH) continue;

            for (int s = 0; s < st.profile.steps; s++)
            {
                int rx = gx0 + s * cellW;

                // check if this step has a note
                bool hasNote = false;
                int note = 60;
                float vol = 0.5F;
                for (auto& n : tr.notes)
                {
                    if (n.step == s) { hasNote = true; note = n.midiNote; vol = n.vol; break; }
                }

                bool hover = (st.hoverTrack == t && st.hoverStep == s);
                bool playHead = (s == st.curStep && st.playing);
                bool sel = (t == st.selectedTrack);

                // cell background
                Color bg = {15, 15, 20, 255};
                if (playHead) bg = {40, 70, 40, 255};
                else if (hover) bg = {35, 35, 45, 255};
                else if (s % 4 == 0) bg = {20, 20, 28, 255};

                if (sel && !hover && !playHead) bg = {20, 25, 35, 255};

                // bar line
                if (s % 4 == 0)
                    DrawRectangle(rx - 1, ry - 1, cellW + 2, cellH + 2, {30, 30, 45, 255});

                DrawRectangle(rx, ry, cellW, cellH, bg);

                if (hasNote)
                {
                    Color nc = noteColorForSound(tr.soundIdx);
                    // full cell color for active note
                    DrawRectangle(rx + 1, ry + 1, cellW - 2, cellH - 2, nc);
                    // note label
                    if (cellW >= 20)
                    {
                        Color tc = {255, 255, 255, 200};
                        DrawText(noteLabel(note), rx + 2, ry + 2, 9, tc);
                    }
                }
                else if (s % 4 == 0 && cellW >= 8)
                {
                    DrawText(".", rx + cellW / 2 - 2, ry + cellH / 2 - 4, 8, {30, 30, 40, 255});
                }
            }
        }

        // ── sound chooser panel (right) ─────────────────────────
        DrawRectangle(rightX, topH, rightW, h - topH - bottomH, {12, 12, 18, 255});
        if (st.soundChooserTrack >= 0 && st.soundChooserTrack < static_cast<int>(st.profile.tracks.size()))
        {
            DrawText("SOUNDS", rightX + 4, topH + 4, 12, DARKGRAY);
            int curSound = st.profile.tracks[st.soundChooserTrack].soundIdx;

            int sy = topH + 22;
            DrawText("WAVEFORMS", rightX + 4, sy, 10, DARKGRAY); sy += 16;
            for (int i = 0; i < 5; i++)
            {
                if (i >= static_cast<int>(sounds.size())) break;
                auto& sd = sounds[i];
                bool hover = (st.soundHover == i);
                bool cur = (i == curSound);
                Color rowBg = cur ? (Color){40, 60, 50, 255} : (Color){0, 0, 0, 0};
                if (hover && !cur) rowBg = {30, 30, 40, 255};
                DrawRectangle(rightX + 2, sy, rightW - 4, 16, rowBg);
                DrawRectangle(rightX + 4, sy + 4, 8, 8, sd.color);
                DrawText(sd.name.c_str(), rightX + 16, sy + 2, 10, cur ? WHITE : LIGHTGRAY);
                sy += 18;
            }

            sy += 4;
            DrawText("SFX", rightX + 4, sy, 10, DARKGRAY); sy += 16;
            for (int i = 5; i < static_cast<int>(sounds.size()); i++)
            {
                auto& sd = sounds[i];
                bool hover = (st.soundHover == i);
                bool cur = (i == curSound);
                Color rowBg = cur ? (Color){40, 60, 50, 255} : (Color){0, 0, 0, 0};
                if (hover && !cur) rowBg = {30, 30, 40, 255};
                DrawRectangle(rightX + 2, sy, rightW - 4, 16, rowBg);
                DrawRectangle(rightX + 4, sy + 4, 8, 8, sd.color);
                DrawText(sd.name.c_str(), rightX + 16, sy + 2, 10, cur ? WHITE : LIGHTGRAY);
                sy += 18;
            }
        }
        else
        {
            DrawText("Select a track", rightX + 4, topH + 10, 11, DARKGRAY);
            DrawText("then click its", rightX + 4, topH + 26, 11, DARKGRAY);
            DrawText("sound name", rightX + 4, topH + 42, 11, DARKGRAY);
            DrawText("to choose sound.", rightX + 4, topH + 58, 11, DARKGRAY);
        }

        // ── bottom bar bg ────────────────────────────────────────
        DrawRectangle(0, h - bottomH, w, bottomH, {15, 15, 22, 255});

        // ── keyboard hint ────────────────────────────────────────
        DrawText("Space=Play/Stop  Tab=NextTrack  M=Mute  Del=ClearStep   Esc=Quit",
                  w / 2 - 200, h - 12, 10, DARKGRAY);

        EndDrawing();
    }

    // cleanup
    for (int t = 0; t < maxTracks; t++)
        if (st.chPlaying[t]) UnloadSound(st.chSounds[t]);
    for (auto& sd : sounds)
        if (sd.idx >= 5) UnloadSound(sd.sound);

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
