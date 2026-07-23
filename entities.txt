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
	MaxHealth           int32
	ShieldActive        bool
	ShieldTimer         float32
	ShieldCooldownTimer float32
	ImmunityTimer       float32
	BlackHoleCoreTimer  float32
	SlowTimer           float32
	Charges             int32
	ChargeRegenTimer    float32
	Dashing             bool
	DashTimer           float32
	DashVelocity        rl.Vector2
	ShieldStacks        int32 // rare-drop damage shields, absorb one hit each - separate from the dash-ability ShieldActive bubble
	HalfLifeOrb         bool  // true once one life orb has been collected; a second converts to +1 Health
}

const maxShieldStacks = int32(3)

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
	Position   rl.Vector2
	Velocity   rl.Vector2
	Radius     float32
	Homing     bool
	Active     bool
	FromPlayer bool  // true for the player's own homing missiles: targets enemies and can't hurt the player who fired it
	Damage     int32 // damage dealt on hitting an enemy (player-vs-boss/enemy damage still goes through damagePlayer's flat amounts)
}

// Mine is a Mine Layer weapon projectile: scatters near the player, drifts
// toward the nearest enemy/asteroid, and detonates in an AoE pulse on
// contact or once its fuse runs out.
type Mine struct {
	Position rl.Vector2
	Velocity rl.Vector2
	Fuse     float32
	Radius   float32
	Damage   int32
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

// --- Enemies (data-driven roster) ---
//
// Rather than 20 bespoke Go types, one EnemyKind table drives a handful of
// movement-pattern behaviors (updateEnemies in update.go). The full roster
// counted in the "~20 types" is: the 17 EnemyKind rows below, plus the
// existing tiered Asteroid and BlackHole hazards (already their own
// systems, reused as-is), plus the Elite modifier (a stat-doubling reroll
// applicable to any kind at spawn time, not a row of its own).

type EnemyPattern int

const (
	PatternChase EnemyPattern = iota
	PatternZigzag
	PatternCharge
	PatternOrbit
	PatternTurret
	PatternSpawner
	PatternStationary
)

type EnemyKind struct {
	Name            string
	Radius          float32
	Health          int32
	Speed           float32
	ContactDamage   int32
	Score           int32
	Pattern         EnemyPattern
	Color           rl.Color
	MinWave         int32
	SplitsOnDeath   bool
	SplitKind       int
	SplitCount      int
	ExplodesOnDeath bool
	ExplodeDamage   int32
	ExplodeRadius   float32
	IsLeech         bool
	PhaseCycle      bool
	FireInterval    float32 // PatternTurret only
	ProjectileSpeed float32 // PatternTurret only
	SpawnKind       int     // PatternSpawner only
	SpawnCount      int     // PatternSpawner only
	SpawnInterval   float32 // PatternSpawner only
}

// Indices into enemyKinds, referenced by SplitKind/SpawnKind below.
const (
	enemyKindSwarmling = 1
)

var enemyKinds = []EnemyKind{
	{Name: "Drifter", Radius: 14, Health: 10, Speed: 1.0, ContactDamage: 1, Score: 5, Pattern: PatternChase, Color: colorBossIdle, MinWave: 1},
	{Name: "Swarmling", Radius: 8, Health: 4, Speed: 2.2, ContactDamage: 1, Score: 3, Pattern: PatternChase, Color: colorHaze, MinWave: 1},
	{Name: "Brute", Radius: 22, Health: 40, Speed: 0.6, ContactDamage: 3, Score: 15, Pattern: PatternChase, Color: colorStructDark, MinWave: 2},
	{Name: "Zigzagger", Radius: 12, Health: 12, Speed: 1.4, ContactDamage: 1, Score: 8, Pattern: PatternZigzag, Color: colorBossSpread, MinWave: 2},
	{Name: "Charger", Radius: 14, Health: 14, Speed: 0.8, ContactDamage: 2, Score: 10, Pattern: PatternCharge, Color: colorAccentDim, MinWave: 3},
	{Name: "Orbiter", Radius: 12, Health: 10, Speed: 1.2, ContactDamage: 1, Score: 8, Pattern: PatternOrbit, Color: colorShield, MinWave: 3},
	{Name: "Splitter", Radius: 16, Health: 16, Speed: 1.0, ContactDamage: 1, Score: 10, Pattern: PatternChase, Color: colorStructMid, MinWave: 2, SplitsOnDeath: true, SplitKind: enemyKindSwarmling, SplitCount: 2},
	{Name: "Turret", Radius: 16, Health: 20, Speed: 0, ContactDamage: 1, Score: 12, Pattern: PatternTurret, Color: colorBossHoming, MinWave: 3, FireInterval: 2.5, ProjectileSpeed: 3},
	{Name: "Sniper", Radius: 14, Health: 10, Speed: 0, ContactDamage: 1, Score: 14, Pattern: PatternTurret, Color: colorCrit, MinWave: 5, FireInterval: 3.5, ProjectileSpeed: 6},
	{Name: "Shielded Drone", Radius: 16, Health: 35, Speed: 0.9, ContactDamage: 2, Score: 16, Pattern: PatternChase, Color: colorHaze, MinWave: 4},
	{Name: "Bomber", Radius: 14, Health: 8, Speed: 1.1, ContactDamage: 0, Score: 12, Pattern: PatternChase, Color: colorAccent, MinWave: 4, ExplodesOnDeath: true, ExplodeDamage: 2, ExplodeRadius: 50},
	{Name: "Leech", Radius: 12, Health: 10, Speed: 1.3, ContactDamage: 0, Score: 8, Pattern: PatternChase, Color: colorCharge, MinWave: 3, IsLeech: true},
	{Name: "Swarm Mother", Radius: 20, Health: 30, Speed: 0.4, ContactDamage: 1, Score: 20, Pattern: PatternSpawner, Color: colorBossSpread, MinWave: 5, SpawnKind: enemyKindSwarmling, SpawnCount: 2, SpawnInterval: 3.0},
	{Name: "Phase Wraith", Radius: 14, Health: 18, Speed: 1.3, ContactDamage: 2, Score: 18, Pattern: PatternChase, Color: colorStructLight, MinWave: 6, PhaseCycle: true},
	{Name: "Mine", Radius: 18, Health: 5, Speed: 0, ContactDamage: 4, Score: 10, Pattern: PatternStationary, Color: colorAccentDim, MinWave: 3},
	{Name: "Laser Fence", Radius: 26, Health: 30, Speed: 0, ContactDamage: 3, Score: 15, Pattern: PatternStationary, Color: colorAccent, MinWave: 6},
	{Name: "Void Rift", Radius: 22, Health: 25, Speed: 0, ContactDamage: 1, Score: 20, Pattern: PatternSpawner, Color: colorBossIdle, MinWave: 7, SpawnKind: enemyKindSwarmling, SpawnCount: 3, SpawnInterval: 4.0},
}

const eliteChance = float32(0.06)

type Enemy struct {
	Kind       int
	Position   rl.Vector2
	Velocity   rl.Vector2
	Health     int32
	Active     bool
	StateTimer float32
	Charging   bool // PatternCharge sub-state: telegraphing vs dashing
	Phased     bool // PhaseCycle: currently intangible
	OrbitAngle float64
	OrbitDist  float32
	IsElite    bool
	HitByDash  bool // guards against one player dash hitting the same enemy multiple frames in a row
}

// --- XP pickups ---

type PickupKind int

const (
	PickupXP PickupKind = iota
	PickupLifeOrb
	PickupShield
)

type Pickup struct {
	Position rl.Vector2
	Value    int32
	Kind     PickupKind
	Active   bool
}

// --- Weapons (auto-firing, granted/leveled by skill picks) ---

type WeaponKind int

const (
	WeaponForward WeaponKind = iota
	WeaponOrbit
	WeaponHoming
	WeaponMine
	WeaponBeam
	WeaponShock
)

type Weapon struct {
	Kind       WeaponKind
	Level      int32
	Timer      float32
	Evolved    bool
	FlashTimer float32 // counts down from flashDuration on each fire, drives the on-fire pulse visual
}

// --- Skills (level-up picks: grant/level a weapon, or a passive stat) ---
//
// 6 weapon-grant skills, each linked 1:1 to one of the 6 passive skills.
// Only 6 skills total may be "equipped" (level > 0) at once - the ability
// slots. Once both halves of a link reach level 3, an Evolve choice can
// appear in the level-up picker (see rollLevelUpChoices in update.go).

type SkillID int

const (
	SkillForwardShot SkillID = iota
	SkillOrbitBlades
	SkillHomingMissiles
	SkillMineLayer
	SkillBeamSweep
	SkillShockwave
	SkillDamage
	SkillBarrier
	SkillCooldown
	SkillPickupRadius
	SkillMoveSpeed
	SkillMaxHP
	skillCount
)

const maxAbilitySlots = 6

type SkillDef struct {
	Name        string
	Description string
	MaxLevel    int32
}

var skillDefs = [skillCount]SkillDef{
	SkillForwardShot:    {"Forward Shot", "Fires toward your cursor. Levels add damage.", 8},
	SkillOrbitBlades:    {"Orbit Blades", "Pulses damage to anything circling you.", 8},
	SkillHomingMissiles: {"Homing Missiles", "Auto-fires at the nearest enemy.", 8},
	SkillMineLayer:      {"Mine Layer", "Drops a scattering blast pulse.", 8},
	SkillBeamSweep:      {"Beam Sweep", "Fires a piercing beam toward your cursor.", 8},
	SkillShockwave:      {"Shockwave", "A slow, heavy-hitting pulse around you.", 8},
	SkillDamage:         {"Warhead Tuning", "+15% damage on all weapons.", 5},
	SkillBarrier:        {"Barrier Mastery", "+shield duration, faster charge regen.", 3},
	SkillCooldown:       {"Overclock", "-10% weapon cooldowns.", 5},
	SkillPickupRadius:   {"Tractor Beam", "+20% pickup magnet radius.", 5},
	SkillMoveSpeed:      {"Thrusters", "+10% move speed.", 5},
	SkillMaxHP:          {"Hull Plating", "+1 max health, fully healed.", 5},
}

// weaponGrantSkill maps a WeaponKind back to the skill that grants/levels it.
var weaponGrantSkill = map[WeaponKind]SkillID{
	WeaponForward: SkillForwardShot,
	WeaponOrbit:   SkillOrbitBlades,
	WeaponHoming:  SkillHomingMissiles,
	WeaponMine:    SkillMineLayer,
	WeaponBeam:    SkillBeamSweep,
	WeaponShock:   SkillShockwave,
}

// skillLinkedPassive maps each weapon-grant skill to its linked passive -
// both must reach level 3 for that weapon's Evolve choice to appear.
var skillLinkedPassive = map[SkillID]SkillID{
	SkillForwardShot:    SkillDamage,
	SkillOrbitBlades:    SkillBarrier,
	SkillHomingMissiles: SkillCooldown,
	SkillMineLayer:      SkillPickupRadius,
	SkillBeamSweep:      SkillMoveSpeed,
	SkillShockwave:      SkillMaxHP,
}

var evolvedWeaponName = map[WeaponKind]string{
	WeaponForward: "Photon Cannon",
	WeaponOrbit:   "Aegis Ring",
	WeaponHoming:  "Seeker Swarm",
	WeaponMine:    "Cluster Charges",
	WeaponBeam:    "Lance Sweep",
	WeaponShock:   "Bulwark Pulse",
}

// LevelUpChoice is one option offered on the level-up picker: either level up
// a skill (new or owned) or, if eligible, evolve a weapon into its super form.
type ChoiceKind int

const (
	ChoiceSkill ChoiceKind = iota
	ChoiceEvolve
	ChoiceLifeOrbs // fallback reward once every slot is full and maxed
	ChoiceShields  // fallback reward once every slot is full and maxed
)

type LevelUpChoice struct {
	Kind   ChoiceKind
	Skill  SkillID
	Weapon WeaponKind
	Count  int32 // for ChoiceLifeOrbs/ChoiceShields: how many
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
