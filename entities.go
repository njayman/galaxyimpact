package main

import (
	"math"

	rl "github.com/gen2brain/raylib-go/raylib"
)

type BossState int

const (
	IDLE BossState = iota
	WINDING_UP
	SHOOTING
)

type BossAttack int

const (
	AttackBeam BossAttack = iota
	AttackHoming
	AttackSpread
	AttackSlam
)

const maxSlamRadius = float32(1000)

func CheckCollisionCircleLine(center rl.Vector2, radius float32, startPos rl.Vector2, endPos rl.Vector2) bool {
	return rl.CheckCollisionPointLine(center, startPos, endPos, int32(radius))
}

type Star struct {
	Position rl.Vector2
	Radius   float32
}

// BgParticle is a soft, slow-drifting background mote (distant nebula dust) —
// purely decorative, distinct from the crisp foreground Stars.
type BgParticle struct {
	Position rl.Vector2
	Velocity rl.Vector2
	Radius   float32
	Color    rl.Color
}

type Player struct {
	Position            rl.Vector2
	Radius              float32
	Color               rl.Color
	Speed               float32
	Health              int32
	ShieldActive        bool
	ShieldTimer         float32
	ShieldCooldownTimer float32
	ImmunityTimer       float32
	Charging            bool
	ChargeTimer         float32
	HoldTimer           float32
	BlackHoleCoreTimer  float32
}

type Boss struct {
	Position       rl.Vector2
	Size           rl.Vector2
	Color          rl.Color
	Health         int32
	MaxHealth      int32
	State          BossState
	Attack         BossAttack
	AttackTimer    float32
	StateTimer     float32
	TargetPosition rl.Vector2
	BeamRotation   float32
	SlamHit        bool
}

type Bullet struct {
	Position rl.Vector2
	Velocity rl.Vector2
	Radius   float32
	Color    rl.Color
	Active   bool
	Damage   int32
}

type BossProjectile struct {
	Position rl.Vector2
	Velocity rl.Vector2
	Radius   float32
	Homing   bool
	Active   bool
}

type BlackHole struct {
	Position        rl.Vector2
	Radius          float32
	InfluenceRadius float32
	Active          bool
	Timer           float32
}

type AsteroidTier int

const (
	TierLarge AsteroidTier = iota
	TierMedium
	TierSmall
)

type Asteroid struct {
	Position rl.Vector2
	Velocity rl.Vector2
	Radius   float32
	Tier     AsteroidTier
	Active   bool
}

const maxAsteroids = 40

func asteroidRadius(tier AsteroidTier) float32 {
	switch tier {
	case TierLarge:
		return 32
	case TierMedium:
		return 20
	default:
		return 12
	}
}

func asteroidScore(tier AsteroidTier) int32 {
	switch tier {
	case TierLarge:
		return 10
	case TierMedium:
		return 20
	default:
		return 30
	}
}

// breakAsteroid shatters a non-small asteroid into 3 smaller ones flying outward.
func breakAsteroid(asteroids []Asteroid, a Asteroid) []Asteroid {
	if a.Tier == TierSmall || len(asteroids) >= maxAsteroids {
		return asteroids
	}

	childTier := a.Tier + 1
	speed := float32(rl.GetRandomValue(30, 50)) / 10.0

	for i := 0; i < 3 && len(asteroids) < maxAsteroids; i++ {
		angle := float64(rl.GetRandomValue(0, 359)) * rl.Deg2rad
		velocity := rl.NewVector2(float32(math.Cos(angle))*speed, float32(math.Sin(angle))*speed)

		asteroids = append(asteroids, Asteroid{
			Position: a.Position,
			Velocity: velocity,
			Radius:   asteroidRadius(childTier),
			Tier:     childTier,
			Active:   true,
		})
	}

	return asteroids
}
