package main

import (
	"bytes"
	"encoding/binary"
	"math"
	"math/rand"

	rl "github.com/gen2brain/raylib-go/raylib"
)

const audioSampleRate = 22050

type Sounds struct {
	Shoot        rl.Sound
	Hit          rl.Sound
	Explosion    rl.Sound
	MenuMove     rl.Sound
	MenuConfirm  rl.Sound
	Victory      rl.Sound
	Defeat       rl.Sound
	Critical     rl.Sound
	BossWindup   rl.Sound
	BeamFire     rl.Sound
	HomingLaunch rl.Sound
	SpreadBurst  rl.Sound
	SlamBoom     rl.Sound
}

// LoadSounds generates all game sound effects procedurally (simple tones/noise
// bursts baked into in-memory WAV data), so no external audio assets are needed.
func LoadSounds() Sounds {
	return Sounds{
		Shoot:        toneSound(900, 0.08),
		Hit:          toneSound(220, 0.18),
		Explosion:    noiseSound(0.25),
		MenuMove:     toneSound(500, 0.05),
		MenuConfirm:  sweepSound(500, 900, 0.15),
		Victory:      sweepSound(400, 1000, 0.6),
		Defeat:       sweepSound(400, 120, 0.6),
		Critical:     sweepSound(1000, 1400, 0.15),
		BossWindup:   sweepSound(150, 420, 0.9),
		BeamFire:     sweepSound(800, 200, 0.5),
		HomingLaunch: sweepSound(600, 900, 0.12),
		SpreadBurst:  noiseSound(0.15),
		SlamBoom:     noiseSound(0.45),
	}
}

func toneSound(freq, duration float64) rl.Sound {
	n := int(audioSampleRate * duration)
	samples := make([]int16, n)

	for i := range samples {
		t := float64(i) / audioSampleRate
		envelope := 1.0 - float64(i)/float64(n)
		samples[i] = int16(math.Sin(2*math.Pi*freq*t) * 0.3 * envelope * 32767)
	}

	return soundFromSamples(samples)
}

func sweepSound(startFreq, endFreq, duration float64) rl.Sound {
	n := int(audioSampleRate * duration)
	samples := make([]int16, n)
	phase := 0.0

	for i := range samples {
		frac := float64(i) / float64(n)
		freq := startFreq + (endFreq-startFreq)*frac
		phase += 2 * math.Pi * freq / audioSampleRate
		envelope := 1.0 - frac
		samples[i] = int16(math.Sin(phase) * 0.3 * envelope * 32767)
	}

	return soundFromSamples(samples)
}

func noiseSound(duration float64) rl.Sound {
	n := int(audioSampleRate * duration)
	samples := make([]int16, n)

	for i := range samples {
		envelope := 1.0 - float64(i)/float64(n)
		samples[i] = int16((rand.Float64()*2 - 1) * 0.35 * envelope * 32767)
	}

	return soundFromSamples(samples)
}

// loadBGM procedurally generates a soft, seamlessly-looping ambient chord
// pad (no external audio assets needed) for the background music toggle.
func loadBGM() rl.Music {
	const duration = 6.0
	n := int(audioSampleRate * duration)
	samples := make([]int16, n)

	freqs := []float64{110.00, 130.81, 164.81} // A2-C3-E3 minor chord
	fadeDuration := 0.05
	fadeSamples := int(fadeDuration * float64(audioSampleRate))

	for i := range samples {
		t := float64(i) / audioSampleRate

		v := 0.0
		for _, f := range freqs {
			v += math.Sin(2*math.Pi*f*t) / float64(len(freqs))
		}

		breathe := 0.6 + 0.4*math.Sin(2*math.Pi*t/duration)
		amp := 0.12 * breathe

		if i < fadeSamples {
			amp *= float64(i) / float64(fadeSamples)
		} else if i > n-fadeSamples {
			amp *= float64(n-i) / float64(fadeSamples)
		}

		samples[i] = int16(v * amp * 32767)
	}

	data := encodeWAV(samples)
	return rl.LoadMusicStreamFromMemory(".wav", data, int32(len(data)))
}

func soundFromSamples(samples []int16) rl.Sound {
	data := encodeWAV(samples)
	wave := rl.LoadWaveFromMemory(".wav", data, int32(len(data)))
	sound := rl.LoadSoundFromWave(wave)
	rl.UnloadWave(wave)
	return sound
}

func encodeWAV(samples []int16) []byte {
	var buf bytes.Buffer
	dataSize := int32(len(samples) * 2)

	buf.WriteString("RIFF")
	binary.Write(&buf, binary.LittleEndian, int32(36)+dataSize)
	buf.WriteString("WAVE")

	buf.WriteString("fmt ")
	binary.Write(&buf, binary.LittleEndian, int32(16))
	binary.Write(&buf, binary.LittleEndian, int16(1))                 // PCM
	binary.Write(&buf, binary.LittleEndian, int16(1))                 // mono
	binary.Write(&buf, binary.LittleEndian, int32(audioSampleRate))   // sample rate
	binary.Write(&buf, binary.LittleEndian, int32(audioSampleRate*2)) // byte rate
	binary.Write(&buf, binary.LittleEndian, int16(2))                 // block align
	binary.Write(&buf, binary.LittleEndian, int16(16))                // bits per sample

	buf.WriteString("data")
	binary.Write(&buf, binary.LittleEndian, dataSize)
	binary.Write(&buf, binary.LittleEndian, samples)

	return buf.Bytes()
}
