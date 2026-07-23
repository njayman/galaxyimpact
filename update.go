package main

import (
	"math"

	rl "github.com/gen2brain/raylib-go/raylib"

	"github.com/njayman/galaxyimpact/highscore"
)

const (
	projectileSpeed       = float32(10)
	projectileSize        = float32(7)
	bossRamDamage         = int32(50)
	blackHolePull         = float32(2)
	blackHoleSlow         = float32(2.5)
	blackHoleAsteroidPull = float32(1.5)
	blackHoleChaseSpeed   = float32(0.3)
	slamDuration          = float32(1.6)
	homingProjSpeed       = float32(2.5)
	spreadProjSpeed       = float32(4)
	baseBulletDamage      = int32(5)
	pickupMagnetSpeed     = float32(7)
	maxEnemies            = 200
	maxCharges            = int32(2)
	chargeRegenTime       = float32(3.0)
	dashSpeed             = float32(22)
	dashDuration          = float32(0.18)
	dashDamage            = int32(15)
	shieldBaseDuration    = float32(1.2)

	// entityDespawnRadius is how far a bullet/enemy/projectile can drift from
	// the player before it's culled - shared by updateBullets, updateEnemies,
	// and updateProjectiles so the three stay in sync.
	entityDespawnRadius = float32(1400)
	// asteroidDespawnRadius is asteroids' equivalent (they use a shorter
	// leash since they drift in from a closer spawn ring).
	asteroidDespawnRadius = float32(900)
	// turretFireRange is how close the player must be before a Turret/Sniper
	// enemy will fire at them.
	turretFireRange = float32(700)
	// crossfireProjectileDamage is what a boss/enemy-fired projectile deals
	// if it incidentally hits a regular enemy (not its intended target) -
	// preserved at its historical flat value for that rare crossfire case.
	crossfireProjectileDamage = int32(15)
	// shieldDropChance/lifeOrbDropChance are out of 1000, rolled once per
	// enemy kill in damageEnemy (mutually exclusive with each other).
	shieldDropChance  = int32(15) // 1.5%
	lifeOrbDropChance = int32(40) // 4%, i.e. rolls 15..54
)

// UpdateGame advances the game by one frame and reports whether the player chose to exit.
func UpdateGame(g *Game, deltaTime float32) bool {
	if rl.IsKeyPressed(rl.KeyF11) {
		toggleFullscreen(g)
	}
	syncScreenSize(g)

	updateBgParticles(g)
	updateDeathParticles(g, deltaTime)
	rl.UpdateMusicStream(g.BGM)

	if g.ShakeTimer > 0 {
		g.ShakeTimer -= deltaTime
		if g.ShakeTimer <= 0 {
			g.ShakeTimer = 0
			g.ShakeIntensity = 0
		}
	}

	if g.HitPauseTimer > 0 {
		g.HitPauseTimer -= deltaTime
		return false
	}

	switch g.State {
	case TITLE:
		return updateTitle(g)
	case GAMEPLAY:
		updateGameplay(g, deltaTime)
	case PAUSED:
		return updatePaused(g)
	case LEVEL_UP:
		return updateLevelUp(g)
	case GAME_OVER:
		return updateGameOver(g)
	case SETTINGS:
		return updateSettings(g)
	}

	return false
}

// triggerShake raises the screen-shake amplitude/duration to at least the given
// values (multiple simultaneous triggers don't stack, the strongest wins).
func triggerShake(g *Game, intensity, duration float32) {
	if intensity > g.ShakeIntensity {
		g.ShakeIntensity = intensity
	}
	if duration > g.ShakeTimer {
		g.ShakeTimer = duration
		g.ShakeDuration = duration
	}
}

// triggerHitPause freezes gameplay for a brief moment to sell an impactful hit.
func triggerHitPause(g *Game, duration float32) {
	if duration > g.HitPauseTimer {
		g.HitPauseTimer = duration
	}
}

func updateTitle(g *Game) bool {
	index, confirmed := updateMenuSelection(g, g.MenuIndex, 3, titleMenuY, menuLineHeight)
	playMenuSounds(g, index, confirmed)
	g.MenuIndex = index

	if confirmed {
		switch index {
		case 0:
			resetRun(g)
		case 1:
			g.SettingsReturnState = TITLE
			g.MenuIndex = 0
			g.State = SETTINGS
		case 2:
			return true
		}
	}

	return false
}

func updatePaused(g *Game) bool {
	if rl.IsKeyPressed(rl.KeyEscape) {
		g.State = GAMEPLAY
		return false
	}

	index, confirmed := updateMenuSelection(g, g.MenuIndex, 4, pausedMenuY, menuLineHeight)
	playMenuSounds(g, index, confirmed)
	g.MenuIndex = index

	if confirmed {
		switch index {
		case 0:
			g.State = GAMEPLAY
		case 1:
			g.SettingsReturnState = PAUSED
			g.MenuIndex = 0
			g.State = SETTINGS
		case 2:
			resetRun(g)
		case 3:
			return true
		}
	}

	return false
}

func updateGameOver(g *Game) bool {
	if !g.ScoreRecorded {
		g.HighScores = highscore.Record(g.HighScoreRepo, g.HighScores, g.Score)
		g.ScoreRecorded = true
	}

	index, confirmed := updateMenuSelection(g, g.MenuIndex, 2, gameOverMenuY, menuLineHeight)
	playMenuSounds(g, index, confirmed)
	g.MenuIndex = index

	if confirmed {
		switch index {
		case 0:
			resetRun(g)
		case 1:
			return true
		}
	}

	return false
}

// updateLevelUp handles the non-cancelable skill/evolve picker that pauses
// the world when the player levels up.
func updateLevelUp(g *Game) bool {
	index, confirmed := updateMenuSelection(g, g.MenuIndex, int32(len(g.PendingChoices)), levelUpMenuY, levelUpLineHeight)
	playMenuSounds(g, index, confirmed)
	g.MenuIndex = index

	if confirmed {
		choice := g.PendingChoices[index]
		switch choice.Kind {
		case ChoiceEvolve:
			applyEvolution(g, choice.Weapon)
		case ChoiceLifeOrbs:
			for i := int32(0); i < choice.Count; i++ {
				collectLifeOrb(g)
			}
		case ChoiceShields:
			for i := int32(0); i < choice.Count; i++ {
				if g.Player.ShieldStacks < maxShieldStacks {
					g.Player.ShieldStacks++
				}
			}
		default:
			applySkill(g, choice.Skill)
		}
		g.State = GAMEPLAY
	}

	return false
}

func playMenuSounds(g *Game, newIndex int32, confirmed bool) {
	if confirmed {
		playSFX(g, g.Sounds.MenuConfirm)
	} else if newIndex != g.MenuIndex {
		playSFX(g, g.Sounds.MenuMove)
	}
}

// playSFX is the single path all sound-effect playback flows through, so the
// Sound on/off setting has one place to take effect.
func playSFX(g *Game, sound rl.Sound) {
	if g.Settings.SoundOn {
		rl.PlaySound(sound)
	}
}

const settingsRowCount = 6 // Resolution, Difficulty, BGM, Sound, Display Mode, Back

// updateSettings handles the settings screen: Up/Down selects a row,
// Left/Right (or Enter/Space) changes that row's value.
func updateSettings(g *Game) bool {
	if rl.IsKeyPressed(rl.KeyW) || rl.IsKeyPressed(rl.KeyUp) {
		g.MenuIndex--
	}
	if rl.IsKeyPressed(rl.KeyS) || rl.IsKeyPressed(rl.KeyDown) {
		g.MenuIndex++
	}
	if g.MenuIndex < 0 {
		g.MenuIndex = settingsRowCount - 1
	}
	if g.MenuIndex >= settingsRowCount {
		g.MenuIndex = 0
	}

	left := rl.IsKeyPressed(rl.KeyA) || rl.IsKeyPressed(rl.KeyLeft)
	right := rl.IsKeyPressed(rl.KeyD) || rl.IsKeyPressed(rl.KeyRight)
	confirm := rl.IsKeyPressed(rl.KeyEnter) || rl.IsKeyPressed(rl.KeySpace)

	if row, hovering := hoveredRow(g, settingsRowCount, settingsMenuY, settingsLineHeight); hovering {
		g.MenuIndex = row
		if rl.IsMouseButtonPressed(rl.MouseButtonLeft) {
			confirm = true
		}
	}

	if left || right || confirm {
		playSFX(g, g.Sounds.MenuMove)
	}

	switch g.MenuIndex {
	case 0: // Resolution
		if left {
			cycleResolution(g, -1)
		}
		if right || confirm {
			cycleResolution(g, 1)
		}
	case 1: // Difficulty
		if left {
			cycleDifficulty(g, -1)
		}
		if right || confirm {
			cycleDifficulty(g, 1)
		}
	case 2: // BGM
		if left || right || confirm {
			g.Settings.BGMOn = !g.Settings.BGMOn
			applyBGMState(g)
		}
	case 3: // Sound
		if left || right || confirm {
			g.Settings.SoundOn = !g.Settings.SoundOn
		}
	case 4: // Display mode
		if left || right || confirm {
			toggleFullscreen(g)
		}
	case 5: // Back
		if confirm {
			g.State = g.SettingsReturnState
			g.MenuIndex = 0
		}
	}

	if (left || right || confirm) && g.MenuIndex <= 3 {
		saveSettings(g.Settings)
	}

	return false
}

func cycleResolution(g *Game, dir int32) {
	n := int32(len(resolutionOptions))
	g.Settings.ResolutionIndex = (g.Settings.ResolutionIndex + dir + n) % n
	opt := resolutionOptions[g.Settings.ResolutionIndex]
	rl.SetWindowSize(int(opt.Width), int(opt.Height))
	syncScreenSize(g)
}

func cycleDifficulty(g *Game, dir int32) {
	n := int32(difficultyCount)
	g.Settings.Difficulty = Difficulty((int32(g.Settings.Difficulty) + dir + n) % n)
}

func applyBGMState(g *Game) {
	if g.Settings.BGMOn {
		rl.ResumeMusicStream(g.BGM)
	} else {
		rl.PauseMusicStream(g.BGM)
	}
}

// nerveMax/nerveKillGain/nerveDecayPerSec drive the Nerve meter: it climbs
// with kills, bleeds away on its own so idling can't bank it forever, and
// snaps to zero on any hit - "hesitation is defeat" means a mistake should
// cost the aggression bonus you built up, not just a chunk of HP.
const (
	nerveMax            = float32(100)
	nerveKillGain       = float32(6)
	nerveDecayPerSec    = float32(4)
	nerveDamageBonusMax = float32(0.5)
	nerveSpeedBonusMax  = float32(0.2)
)

func nerveFrac(g *Game) float32 {
	return g.Player.Nerve / nerveMax
}

func gainNerve(g *Game) {
	g.Player.Nerve += nerveKillGain
	if g.Player.Nerve > nerveMax {
		g.Player.Nerve = nerveMax
	}

	// Landing a kill also chips away at the ability-charge regen timer, so
	// staying aggressive earns dash/shield charges back faster than turtling
	// out the clock does.
	if g.Player.Charges < maxCharges {
		g.Player.ChargeRegenTimer -= 0.4
	}
}

func updateNerve(g *Game, deltaTime float32) {
	if g.Player.Nerve > 0 {
		g.Player.Nerve -= nerveDecayPerSec * deltaTime
		if g.Player.Nerve < 0 {
			g.Player.Nerve = 0
		}
	}
}

// damagePlayer is the single path all player damage flows through: it applies
// the hit and transitions to GAME_OVER if health runs out.
func damagePlayer(g *Game, amount int32) {
	g.Player.Nerve = 0

	if g.Player.ShieldStacks > 0 {
		g.Player.ShieldStacks--
		g.Player.ImmunityTimer = 1.0
		playSFX(g, g.Sounds.Hit)
		triggerShake(g, 3, 0.15)
		return
	}

	g.Player.Health -= amount
	g.Player.ImmunityTimer = 1.0

	if g.Player.Health <= 0 {
		g.State = GAME_OVER
		g.MenuIndex = 0
		playSFX(g, g.Sounds.Defeat)
		triggerShake(g, 12, 0.5)
		triggerHitPause(g, 0.15)
		spawnDeathExplosion(g)
	} else {
		playSFX(g, g.Sounds.Hit)
		triggerShake(g, 4, 0.2)
	}
}

// spawnDeathExplosion bursts debris outward from the player on death -
// purely cosmetic, kept animating independent of g.State (see
// updateDeathParticles's call site in UpdateGame) so it survives into the
// Game Over screen instead of freezing.
func spawnDeathExplosion(g *Game) {
	debrisColors := []rl.Color{colorAccent, colorCrit, colorHaze}

	for i := 0; i < 28; i++ {
		angle := float64(rl.GetRandomValue(0, 359)) * rl.Deg2rad
		speed := float32(rl.GetRandomValue(20, 80)) / 10.0
		velocity := rl.NewVector2(float32(math.Cos(angle))*speed, float32(math.Sin(angle))*speed)
		life := float32(rl.GetRandomValue(60, 100)) / 100.0

		g.DeathParticles = append(g.DeathParticles, Particle{
			Position: g.Player.Position,
			Velocity: velocity,
			Radius:   float32(rl.GetRandomValue(2, 5)),
			Life:     life,
			MaxLife:  life,
			Color:    debrisColors[rl.GetRandomValue(0, int32(len(debrisColors)-1))],
		})
	}
}

func updateDeathParticles(g *Game, deltaTime float32) {
	active := g.DeathParticles[:0]
	for _, p := range g.DeathParticles {
		p.Position = rl.Vector2Add(p.Position, p.Velocity)
		p.Life -= deltaTime
		if p.Life > 0 {
			active = append(active, p)
		}
	}
	g.DeathParticles = active
}

func updateGameplay(g *Game, deltaTime float32) {
	if rl.IsKeyPressed(rl.KeyEscape) {
		g.State = PAUSED
		g.MenuIndex = 0
		return
	}

	g.RunTime += deltaTime

	if g.Sandbox {
		updateSandboxInput(g)
	}

	updateNerve(g, deltaTime)
	updateAbilityCharges(g, deltaTime)
	updatePlayerMovement(g, deltaTime)
	updateShieldAndBarrier(g, deltaTime)
	updateWeapons(g, deltaTime)
	updateWaveSpawner(g, deltaTime)
	updateBlackHole(g, deltaTime)

	if g.Player.Health > 0 && g.BlackHole.Active && rl.Vector2Distance(g.Player.Position, g.BlackHole.Position) <= g.BlackHole.Radius {
		g.Player.BlackHoleCoreTimer += deltaTime
		if g.Player.BlackHoleCoreTimer >= 1.0 {
			damagePlayer(g, g.Player.Health)
		}
	} else {
		g.Player.BlackHoleCoreTimer = 0
	}

	if g.BossActive {
		bossCenter := rl.NewVector2(g.Boss.Position.X+g.Boss.Size.X/2, g.Boss.Position.Y+g.Boss.Size.Y/2)
		updateBossMovement(g, deltaTime, bossCenter)
		updateBoss(g, deltaTime, bossCenter)
	}

	updateBullets(g)
	updateAsteroids(g)
	updateEnemies(g, deltaTime)
	updateProjectiles(g)
	updateMines(g, deltaTime)
	updatePickups(g, deltaTime)

	filterDeadEntities(g)

	if g.BossActive && g.Boss.Health <= 0 {
		bossCenter := rl.NewVector2(g.Boss.Position.X+g.Boss.Size.X/2, g.Boss.Position.Y+g.Boss.Size.Y/2)

		g.Boss.Health = 0
		g.Boss.Color = colorStructDark
		g.BossActive = false
		g.Score += 1000

		g.BossDeathShockwave = true
		g.BossDeathShockwaveTimer = bossDeathShockwaveDuration
		g.BossDeathShockwavePos = bossCenter
		g.BossDeathShockwaveHit = false

		if g.Player.Health > 0 {
			playSFX(g, g.Sounds.Victory)
			triggerShake(g, 14, 0.6)
			triggerHitPause(g, 0.2)
			gainNerve(g)
		}
	}

	updateBossDeathShockwave(g, deltaTime)

	if g.XP >= g.XPToNext {
		startLevelUp(g)
	}
}

// bossDeathShockwaveDuration is deliberately slow (vs. the boss's own
// slamDuration) so it reads as a lingering aftermath rather than another
// snap attack - it still reaches maxSlamRadius (screen-covering) by the end.
const bossDeathShockwaveDuration = float32(3.0)

// resolveExpandingWaveHit checks a slow-expanding hazard (boss Slam / death
// shockwave) against the player once per frame: if they're within the
// hazard's eventual max radius and have a shield bubble, a shield stack, or
// (if allowDash) an active dash up at any point before the growing radius
// physically reaches them, that counts as dodging it - consuming a shield
// stack if that's specifically what protected them. Only once the radius
// itself crosses the player unprotected does it report a hit. A transient
// post-hit ImmunityTimer deliberately does NOT count as protection here -
// it has nothing to do with dodging this specific hazard, and treating it
// as a free pass let an unrelated graze on the way in cancel the hazard
// entirely. Returns (resolved, hit): resolved means the caller should latch
// its own one-shot hit flag; hit means damagePlayer should be called.
func resolveExpandingWaveHit(g *Game, from rl.Vector2, radius float32, allowDash bool) (resolved bool, hit bool) {
	distToPlayer := rl.Vector2Distance(from, g.Player.Position)
	inDanger := distToPlayer <= maxSlamRadius+g.Player.Radius
	dashProtected := allowDash && g.Player.Dashing

	if inDanger && (g.Player.ShieldActive || dashProtected || g.Player.ShieldStacks > 0) {
		if !g.Player.ShieldActive && !dashProtected && g.Player.ShieldStacks > 0 {
			g.Player.ShieldStacks--
		}
		return true, false
	}

	if distToPlayer <= radius+g.Player.Radius {
		return true, true
	}

	return false, false
}

// updateBossDeathShockwave expands a one-time ring from the boss's death
// position, destroying every enemy/asteroid it passes over (no score/XP -
// it's the boss's own death throes, not a player kill) and damaging the
// player once if it reaches them and they're not shielded.
func updateBossDeathShockwave(g *Game, deltaTime float32) {
	if !g.BossDeathShockwave {
		return
	}

	g.BossDeathShockwaveTimer -= deltaTime

	progress := 1 - g.BossDeathShockwaveTimer/bossDeathShockwaveDuration
	if progress < 0 {
		progress = 0
	}
	if progress > 1 {
		progress = 1
	}
	radius := maxSlamRadius * progress

	for i := range g.Enemies {
		if g.Enemies[i].Active && rl.Vector2Distance(g.BossDeathShockwavePos, g.Enemies[i].Position) <= radius {
			g.Enemies[i].Active = false
		}
	}

	for i := range g.Asteroids {
		if g.Asteroids[i].Active && rl.Vector2Distance(g.BossDeathShockwavePos, g.Asteroids[i].Position) <= radius {
			g.Asteroids[i].Active = false
		}
	}

	if !g.BossDeathShockwaveHit {
		if resolved, hit := resolveExpandingWaveHit(g, g.BossDeathShockwavePos, radius, false); resolved {
			g.BossDeathShockwaveHit = true
			if hit {
				damagePlayer(g, enemyDamage(g, 1))
			}
		}
	}

	if g.BossDeathShockwaveTimer <= 0 {
		g.BossDeathShockwave = false
	}
}

func updatePlayerMovement(g *Game, deltaTime float32) {
	inBlackHole := g.BlackHole.Active && rl.Vector2Distance(g.Player.Position, g.BlackHole.Position) <= g.BlackHole.InfluenceRadius

	if g.Player.SlowTimer > 0 {
		g.Player.SlowTimer -= deltaTime
	}

	if g.Player.Dashing {
		g.Player.DashTimer -= deltaTime
		if g.Player.DashTimer <= 0 {
			g.Player.Dashing = false
		}
		g.Player.Position = rl.Vector2Add(g.Player.Position, g.Player.DashVelocity)
	} else {
		effectiveSpeed := g.Player.Speed * (1 + nerveSpeedBonusMax*nerveFrac(g))
		if inBlackHole {
			effectiveSpeed -= blackHoleSlow
		}
		if g.Player.SlowTimer > 0 {
			effectiveSpeed *= 0.5
		}
		if effectiveSpeed < 1 {
			effectiveSpeed = 1
		}

		moveDelta := rl.Vector2{}

		if rl.IsKeyDown(rl.KeyD) {
			moveDelta.X += effectiveSpeed
		}
		if rl.IsKeyDown(rl.KeyA) {
			moveDelta.X -= effectiveSpeed
		}
		if rl.IsKeyDown(rl.KeyW) {
			moveDelta.Y -= effectiveSpeed
		}
		if rl.IsKeyDown(rl.KeyS) {
			moveDelta.Y += effectiveSpeed
		}

		g.Player.Position = rl.Vector2Add(g.Player.Position, moveDelta)
	}

	if inBlackHole {
		toHole := rl.Vector2Subtract(g.BlackHole.Position, g.Player.Position)
		if rl.Vector2Length(toHole) > 0 {
			pull := rl.Vector2Scale(rl.Vector2Normalize(toHole), blackHolePull)
			g.Player.Position = rl.Vector2Add(g.Player.Position, pull)
		}
	}

	if dist := rl.Vector2Length(g.Player.Position); dist > arenaHalf {
		g.Player.Position = rl.Vector2Scale(rl.Vector2Normalize(g.Player.Position), arenaHalf)
	}

	if g.BossActive {
		bossRect := rl.Rectangle{X: g.Boss.Position.X, Y: g.Boss.Position.Y, Width: g.Boss.Size.X, Height: g.Boss.Size.Y}
		if g.Boss.Health > 0 && rl.CheckCollisionCircleRec(g.Player.Position, g.Player.Radius, bossRect) {
			g.Boss.Health -= bossRamDamage
			if g.Boss.Health <= 0 {
				g.Score += 1000
			}
			triggerShake(g, 14, 0.4)
			triggerHitPause(g, 0.12)
			damagePlayer(g, g.Player.Health)
		}
	}

	if g.Player.ImmunityTimer > 0 {
		g.Player.ImmunityTimer -= deltaTime
	}
}

// updateShieldAndBarrier counts down the shield's active/cooldown timers.
// Activation itself is manual (right-click, see updateAbilityCharges).
func updateShieldAndBarrier(g *Game, deltaTime float32) {
	if g.Player.ShieldCooldownTimer > 0 {
		g.Player.ShieldCooldownTimer -= deltaTime
	}

	if g.Player.ShieldActive {
		g.Player.ShieldTimer -= deltaTime

		if g.Player.ShieldTimer <= 0 {
			g.Player.ShieldActive = false
			g.Player.ShieldCooldownTimer = 2.0
		}
	}
}

// chargeRegenDuration is how long one charge takes to regenerate, sped up by
// the Barrier Mastery skill.
func chargeRegenDuration(g *Game) float32 {
	d := chargeRegenTime - float32(g.SkillLevels[SkillBarrier])*0.3
	if d < 1 {
		d = 1
	}
	return d
}

// updateAbilityCharges regenerates the shared 2-charge pool and handles the
// left-click dash / right-click shield triggers, each spending one charge.
func updateAbilityCharges(g *Game, deltaTime float32) {
	if g.Player.Charges < maxCharges {
		g.Player.ChargeRegenTimer -= deltaTime
		if g.Player.ChargeRegenTimer <= 0 {
			g.Player.Charges++
			g.Player.ChargeRegenTimer = chargeRegenDuration(g)
		}
	}

	if rl.IsMouseButtonPressed(rl.MouseButtonLeft) && g.Player.Charges > 0 && !g.Player.Dashing {
		g.Player.Charges--
		g.Player.Dashing = true
		g.Player.DashTimer = dashDuration
		g.Player.DashVelocity = rl.Vector2Scale(aimAtMouse(g), dashSpeed)

		for i := range g.Enemies {
			g.Enemies[i].HitByDash = false
		}
	}

	if rl.IsMouseButtonPressed(rl.MouseButtonRight) && g.Player.Charges > 0 && !g.Player.ShieldActive {
		g.Player.Charges--
		g.Player.ShieldActive = true
		g.Player.ShieldTimer = shieldBaseDuration + float32(g.SkillLevels[SkillBarrier])*0.4
	}
}

// aimAtMouse returns the normalized direction from screen-center (where the
// player always renders) to the current mouse position.
func aimAtMouse(g *Game) rl.Vector2 {
	mouse := rl.GetMousePosition()
	center := rl.Vector2{X: float32(g.WindowWidth) / 2, Y: float32(g.WindowHeight) / 2}
	dir := rl.Vector2Subtract(mouse, center)
	if rl.Vector2Length(dir) == 0 {
		return rl.Vector2{X: 0, Y: -1}
	}
	return rl.Vector2Normalize(dir)
}

func nearestEnemy(g *Game, from rl.Vector2) (rl.Vector2, bool) {
	best := float32(-1)
	var target rl.Vector2
	found := false

	for _, e := range g.Enemies {
		if !e.Active {
			continue
		}
		d := rl.Vector2Distance(from, e.Position)
		if best < 0 || d < best {
			best = d
			target = e.Position
			found = true
		}
	}

	return target, found
}

// weaponCooldown returns the fire interval for a weapon kind at a given
// level, scaled by the Overclock passive.
var weaponBaseCooldown = map[WeaponKind]float32{
	WeaponForward: 0.45,
	WeaponOrbit:   1.0,
	WeaponHoming:  1.2,
	WeaponMine:    0.8,
	WeaponBeam:    0.7,
	WeaponShock:   1.8,
}

func weaponCooldown(g *Game, kind WeaponKind, level int32, evolved bool) float32 {
	base := weaponBaseCooldown[kind]
	base -= float32(level) * 0.02
	if base < 0.12 {
		base = 0.12
	}
	base *= 1 - 0.1*float32(g.SkillLevels[SkillCooldown])
	if evolved {
		base *= 0.8
	}
	if base < 0.08 {
		base = 0.08
	}
	return base
}

// postCapDamageBonusPerLevel is the small compounding damage bump granted
// each level-up once every ability slot is full and maxed (see
// rollLevelUpChoices) - keeps the player scaling to match waveEnemyScale's
// indefinite escalation instead of flatlining once the build is "complete".
const postCapDamageBonusPerLevel = float32(0.05)

func weaponDamage(g *Game, level int32) int32 {
	dmg := baseBulletDamage + level*2
	dmg = int32(float32(dmg) * (1 + 0.15*float32(g.SkillLevels[SkillDamage])))
	dmg = int32(float32(dmg) * (1 + nerveDamageBonusMax*nerveFrac(g)))
	dmg = int32(float32(dmg) * (1 + postCapDamageBonusPerLevel*float32(g.PostCapDamageLevels)))
	return dmg
}

// orbitRadius is also used by draw.go to place the decorative blade dots.
func orbitRadius(level int32) float32 {
	return 55 + float32(level)*4
}

// shockwaveRadius is also used by draw.go for the on-fire pulse visual.
func shockwaveRadius(level int32, evolved bool) float32 {
	radius := 90 + float32(level)*8
	if evolved {
		radius *= 1.4
	}
	return radius
}

const shockFlashDuration = float32(0.4)

// mineCount/mineRadius/mineDamage are also used by draw.go.
func mineCount(level int32, evolved bool) int {
	count := 2 + int(level)/3
	if evolved {
		count++
	}
	return count
}

func mineRadius(evolved bool) float32 {
	if evolved {
		return 42
	}
	return 32
}

func mineDamage(g *Game, level int32, evolved bool) int32 {
	dmg := weaponDamage(g, level)
	if evolved {
		dmg = int32(float32(dmg) * 1.5)
	}
	return dmg
}

// spawnMines scatters mineCount mines at random offsets around the player;
// they home toward the nearest enemy/asteroid (see updateMines) rather than
// detonating immediately.
func spawnMines(g *Game, w *Weapon) {
	count := mineCount(w.Level, w.Evolved)
	radius := mineRadius(w.Evolved)
	dmg := mineDamage(g, w.Level, w.Evolved)

	for i := 0; i < count; i++ {
		angle := float64(rl.GetRandomValue(0, 359)) * rl.Deg2rad
		dist := float32(rl.GetRandomValue(20, 50))
		pos := rl.Vector2Add(g.Player.Position, rl.NewVector2(float32(math.Cos(angle))*dist, float32(math.Sin(angle))*dist))

		g.Mines = append(g.Mines, Mine{
			Position: pos,
			Fuse:     mineLifetime,
			Radius:   radius,
			Damage:   dmg,
			Active:   true,
		})
	}
}

const (
	mineLifetime   = float32(5.0)
	mineHomeSpeed  = float32(3.5)
	mineSeekRadius = float32(320)
)

// updateMines drifts each mine toward the nearest enemy/asteroid within
// mineSeekRadius and detonates it (an aoePulse at the mine's own position) on
// contact or once its fuse runs out.
func updateMines(g *Game, deltaTime float32) {
	for i := range g.Mines {
		if !g.Mines[i].Active {
			continue
		}

		target, foundEnemy := nearestEnemyWithin(g, g.Mines[i].Position, mineSeekRadius)
		if !foundEnemy {
			target, foundEnemy = nearestAsteroidWithin(g, g.Mines[i].Position, mineSeekRadius)
		}

		detonate := false

		if foundEnemy {
			dir := rl.Vector2Subtract(target, g.Mines[i].Position)
			if rl.Vector2Length(dir) <= g.Mines[i].Radius {
				detonate = true
			} else {
				g.Mines[i].Velocity = rl.Vector2Scale(rl.Vector2Normalize(dir), mineHomeSpeed)
			}
		}

		g.Mines[i].Position = rl.Vector2Add(g.Mines[i].Position, g.Mines[i].Velocity)

		g.Mines[i].Fuse -= deltaTime
		if g.Mines[i].Fuse <= 0 {
			detonate = true
		}

		if detonate {
			g.Mines[i].Active = false
			aoePulse(g, g.Mines[i].Position, g.Mines[i].Radius, g.Mines[i].Damage)
		}
	}
}

func nearestEnemyWithin(g *Game, from rl.Vector2, maxDist float32) (rl.Vector2, bool) {
	target, ok := nearestEnemy(g, from)
	if ok && rl.Vector2Distance(from, target) <= maxDist {
		return target, true
	}
	return rl.Vector2{}, false
}

func nearestAsteroidWithin(g *Game, from rl.Vector2, maxDist float32) (rl.Vector2, bool) {
	best := float32(-1)
	var target rl.Vector2
	found := false

	for _, a := range g.Asteroids {
		if !a.Active {
			continue
		}
		d := rl.Vector2Distance(from, a.Position)
		if d <= maxDist && (best < 0 || d < best) {
			best = d
			target = a.Position
			found = true
		}
	}

	return target, found
}

// updateWeapons auto-fires every equipped weapon on its own cooldown - there
// is no manual fire key, matching the bullet-heaven auto-combat feel.
func updateWeapons(g *Game, deltaTime float32) {
	for i := range g.Weapons {
		w := &g.Weapons[i]

		if w.FlashTimer > 0 {
			w.FlashTimer -= deltaTime
		}

		w.Timer -= deltaTime
		if w.Timer > 0 {
			continue
		}
		w.Timer = weaponCooldown(g, w.Kind, w.Level, w.Evolved)

		switch w.Kind {
		case WeaponForward:
			dir := aimAtMouse(g)
			shots := 1 + w.Level/3
			if w.Evolved {
				shots++
			}
			spread := float32(10)
			dmg := weaponDamage(g, w.Level)
			color := colorAccent
			if w.Evolved {
				dmg = int32(float32(dmg) * 1.5)
				color = colorCrit
			}
			for s := int32(0); s < shots; s++ {
				angleOffset := (float64(s) - float64(shots-1)/2) * float64(spread) * float64(rl.Deg2rad)
				cos, sin := math.Cos(angleOffset), math.Sin(angleOffset)
				rotated := rl.NewVector2(
					dir.X*float32(cos)-dir.Y*float32(sin),
					dir.X*float32(sin)+dir.Y*float32(cos),
				)
				g.Bullets = append(g.Bullets, Bullet{
					Position: g.Player.Position,
					Velocity: rl.Vector2Scale(rotated, projectileSpeed),
					Radius:   projectileSize,
					Color:    color,
					Active:   true,
					Damage:   dmg,
				})
			}
			playSFX(g, g.Sounds.Shoot)
		case WeaponHoming:
			missiles := 1
			if w.Evolved {
				missiles = 2
			}
			dmg := weaponDamage(g, w.Level)
			if w.Evolved {
				dmg = int32(float32(dmg) * 1.5)
			}
			fired := false
			for m := 0; m < missiles; m++ {
				if target, ok := nearestEnemy(g, g.Player.Position); ok {
					dir := rl.Vector2Normalize(rl.Vector2Subtract(target, g.Player.Position))
					g.Projectiles = append(g.Projectiles, BossProjectile{
						Position:   g.Player.Position,
						Velocity:   rl.Vector2Scale(dir, homingProjSpeed*1.5),
						Radius:     6,
						Homing:     true,
						Active:     true,
						FromPlayer: true,
						Damage:     dmg,
					})
					fired = true
				}
			}
			if fired {
				playSFX(g, g.Sounds.HomingLaunch)
			}
		case WeaponOrbit:
			radius := orbitRadius(w.Level)
			dmg := weaponDamage(g, w.Level) / 2
			if w.Evolved {
				radius *= 1.4
				dmg = int32(float32(dmg) * 1.5)
			}
			aoePulse(g, g.Player.Position, radius, dmg)
		case WeaponShock:
			radius := shockwaveRadius(w.Level, w.Evolved)
			dmg := weaponDamage(g, w.Level)
			if w.Evolved {
				dmg = int32(float32(dmg) * 1.5)
				if g.Player.Health < g.Player.MaxHealth {
					g.Player.Health++
				}
			}
			aoePulse(g, g.Player.Position, radius, dmg)
			w.FlashTimer = shockFlashDuration
		case WeaponMine:
			spawnMines(g, w)
		case WeaponBeam:
			dir := aimAtMouse(g)
			length := float32(300 + w.Level*15)
			dmg := weaponDamage(g, w.Level)
			if w.Evolved {
				length *= 1.3
				dmg = int32(float32(dmg) * 1.5)
			}
			beamPulse(g, dir, length, dmg)
		}
	}
}

// aoePulse damages/destroys everything within radius of the player in one
// instantaneous pulse - shared by Orbit, Shockwave, and Mine Layer, which
// differ only in radius/damage/cooldown numbers and visuals.
func aoePulse(g *Game, center rl.Vector2, radius float32, dmg int32) {
	hitAny := false

	for j := range g.Enemies {
		if g.Enemies[j].Active && !g.Enemies[j].Phased && rl.Vector2Distance(center, g.Enemies[j].Position) <= radius+enemyKinds[g.Enemies[j].Kind].Radius {
			damageEnemy(g, j, dmg)
			hitAny = true
		}
	}

	for j := range g.Asteroids {
		if g.Asteroids[j].Active && rl.Vector2Distance(center, g.Asteroids[j].Position) <= radius+g.Asteroids[j].Radius {
			g.Asteroids[j].Active = false
			g.Score += asteroidScore(g.Asteroids[j].Tier)
			g.Asteroids = breakAsteroid(g.Asteroids, g.Asteroids[j])
			hitAny = true
		}
	}

	if hitAny {
		playSFX(g, g.Sounds.Explosion)
	}
}

// beamPulse damages everything along a line from the player toward dir, out
// to length - Beam Sweep's attack.
func beamPulse(g *Game, dir rl.Vector2, length float32, dmg int32) {
	start := g.Player.Position
	end := rl.Vector2Add(start, rl.Vector2Scale(dir, length))
	hitAny := false

	for j := range g.Enemies {
		if g.Enemies[j].Active && !g.Enemies[j].Phased && CheckCollisionCircleLine(g.Enemies[j].Position, enemyKinds[g.Enemies[j].Kind].Radius, start, end) {
			damageEnemy(g, j, dmg)
			hitAny = true
		}
	}

	for j := range g.Asteroids {
		if g.Asteroids[j].Active && CheckCollisionCircleLine(g.Asteroids[j].Position, g.Asteroids[j].Radius, start, end) {
			g.Asteroids[j].Active = false
			g.Score += asteroidScore(g.Asteroids[j].Tier)
			g.Asteroids = breakAsteroid(g.Asteroids, g.Asteroids[j])
			hitAny = true
		}
	}

	if hitAny {
		playSFX(g, g.Sounds.Explosion)
	}
}

// updateWaveSpawner escalates wave number/spawn rate over time and spawns
// enemies from the roster on a ring just outside the screen around the
// player; every 5th wave also brings the boss in.
func updateWaveSpawner(g *Game, deltaTime float32) {
	if g.Sandbox {
		return
	}

	g.WaveTimer -= deltaTime
	if g.WaveTimer <= 0 {
		g.WaveNumber++
		g.WaveTimer = waveDuration

		if g.WaveNumber%5 == 0 {
			spawnBoss(g)
		}
	}

	g.EnemySpawnTimer -= deltaTime
	if g.EnemySpawnTimer <= 0 && len(g.Enemies) < maxEnemies {
		interval := 1.2 - float32(g.WaveNumber)*0.05
		if interval < 0.15 {
			interval = 0.15
		}
		g.EnemySpawnTimer = interval * difficultyDefs[g.Settings.Difficulty].SpawnRateMult

		spawnCount := 1 + int(g.WaveNumber)/4
		for i := 0; i < spawnCount && len(g.Enemies) < maxEnemies; i++ {
			spawnEnemy(g)
		}
	}

	asteroidsUnlocked := g.Settings.Difficulty != DifficultyEasy || g.WaveNumber >= 10

	g.AsteroidSpawnTimer -= deltaTime
	if asteroidsUnlocked && g.AsteroidSpawnTimer <= 0 && len(g.Asteroids) < asteroidCap(g) {
		g.AsteroidSpawnTimer = float32(rl.GetRandomValue(8, 16)) / 10.0 * asteroidIntervalMultiplier(g)

		tier := TierLarge
		if rl.GetRandomValue(0, 1) == 1 {
			tier = TierMedium
		}

		angle := float64(rl.GetRandomValue(0, 359)) * rl.Deg2rad
		spawnPos := rl.Vector2Add(g.Player.Position, rl.NewVector2(float32(math.Cos(angle))*500, float32(math.Sin(angle))*500))
		aimPoint := rl.Vector2Add(g.Player.Position, rl.NewVector2(float32(rl.GetRandomValue(-180, 180)), float32(rl.GetRandomValue(-180, 180))))
		direction := rl.Vector2Normalize(rl.Vector2Subtract(aimPoint, spawnPos))
		speed := float32(rl.GetRandomValue(25, 45)) / 10.0

		g.Asteroids = append(g.Asteroids, Asteroid{
			Position: spawnPos,
			Velocity: rl.Vector2Scale(direction, speed),
			Radius:   asteroidRadius(tier),
			Tier:     tier,
			Active:   true,
		})
	}
}

// asteroidIntervalMultiplier/asteroidCap tune how annoying the asteroid
// field is per difficulty: Hard keeps today's baseline pace, Normal/Easy
// spawn slower and cap lower; Easy is additionally locked out entirely
// before wave 10 (see asteroidsUnlocked above).
func asteroidIntervalMultiplier(g *Game) float32 {
	if g.Settings.Difficulty == DifficultyHard {
		return 1.0
	}
	return 1.6
}

func asteroidCap(g *Game) int {
	switch g.Settings.Difficulty {
	case DifficultyHard:
		return maxAsteroids
	case DifficultyEasy:
		return maxAsteroids / 3
	default:
		return maxAsteroids * 2 / 3
	}
}

// enemyDamage scales a base enemy damage amount by the difficulty's damage
// multiplier (Easy hits softer, Hard hits harder).
// waveEnemyScalePerWave gives enemies a small, steady stat climb every wave
// (health here, contact damage via enemyDamage) so the run keeps escalating
// indefinitely - the player is expected to keep pace by leveling up weapons/
// passives, and by the flat postCapDamageBonus once every slot is full and
// maxed (see rollLevelUpChoices).
const waveEnemyScalePerWave = float32(0.035)

func waveEnemyScale(g *Game) float32 {
	return 1 + float32(g.WaveNumber-1)*waveEnemyScalePerWave
}

func enemyDamage(g *Game, base int32) int32 {
	return int32(float32(base) * difficultyDefs[g.Settings.Difficulty].EnemyDamageMult * waveEnemyScale(g))
}

// spawnEnemy picks a weighted-random kind (eligible for the current wave)
// and places it just outside the screen around the player.
func spawnEnemy(g *Game) {
	eligible := make([]int, 0, len(enemyKinds))
	for i, k := range enemyKinds {
		if k.MinWave <= g.WaveNumber {
			eligible = append(eligible, i)
		}
	}
	if len(eligible) == 0 {
		return
	}

	kindIndex := eligible[rl.GetRandomValue(0, int32(len(eligible)-1))]
	spawnEnemyAt(g, kindIndex, spawnRingPosition(g))
}

func spawnRingPosition(g *Game) rl.Vector2 {
	angle := float64(rl.GetRandomValue(0, 359)) * rl.Deg2rad
	dist := float32(rl.GetRandomValue(420, 520))
	return rl.Vector2Add(g.Player.Position, rl.NewVector2(float32(math.Cos(angle))*dist, float32(math.Sin(angle))*dist))
}

func spawnEnemyAt(g *Game, kindIndex int, pos rl.Vector2) {
	if len(g.Enemies) >= maxEnemies {
		return
	}

	kind := enemyKinds[kindIndex]
	elite := rl.GetRandomValue(0, 999) < int32(eliteChance*1000)

	health := int32(float32(kind.Health) * difficultyDefs[g.Settings.Difficulty].EnemyHealthMult * waveEnemyScale(g))
	if elite {
		health *= 2
	}

	g.Enemies = append(g.Enemies, Enemy{
		Kind:       kindIndex,
		Position:   pos,
		Health:     health,
		Active:     true,
		StateTimer: kind.FireInterval + kind.SpawnInterval + 1,
		IsElite:    elite,
		OrbitDist:  200,
	})
}

// bossEngageDistance is how close the boss settles once it arrives - it
// approaches from off-screen but holds here rather than closing all the way
// onto the player (its attacks all reach well past this range anyway).
const bossEngageDistance = float32(380)

func spawnBoss(g *Game) {
	tier := g.WaveNumber / 5

	angle := float64(rl.GetRandomValue(0, 359)) * rl.Deg2rad
	dist := float32(rl.GetRandomValue(1200, 1500))
	spawnPos := rl.Vector2Add(g.Player.Position, rl.NewVector2(float32(math.Cos(angle))*dist, float32(math.Sin(angle))*dist))

	health := int32(float32(500+(tier-1)*250) * difficultyDefs[g.Settings.Difficulty].EnemyHealthMult)

	g.Boss = Boss{
		Position:    spawnPos,
		Size:        rl.NewVector2(100, 100),
		Color:       colorBossIdle,
		Health:      health,
		MaxHealth:   health,
		State:       IDLE,
		Attack:      AttackBeam,
		AttackTimer: float32(rl.GetRandomValue(15, 35)) / 10.0,
	}
	g.BossActive = true
	playSFX(g, g.Sounds.BossWindup)
}

// updateBossMovement has the boss approach from wherever it spawned (usually
// far off-camera) toward the player - a bit faster than the player's own
// (possibly skill-boosted) move speed, so it can't be outrun forever - but it
// settles at bossEngageDistance rather than closing in all the way to melee
// range; if the player runs further off, it resumes closing.
func updateBossMovement(g *Game, deltaTime float32, bossCenter rl.Vector2) {
	toPlayer := rl.Vector2Subtract(g.Player.Position, bossCenter)
	if dist := rl.Vector2Length(toPlayer); dist > bossEngageDistance {
		chaseSpeed := g.Player.Speed*1.15 + 1
		drift := rl.Vector2Scale(rl.Vector2Normalize(toPlayer), chaseSpeed)
		g.Boss.Position = rl.Vector2Add(g.Boss.Position, drift)
	}
}

// updatePickups magnet-pulls XP gems within pickup radius toward the player
// and collects any that reach them.
func updatePickups(g *Game, deltaTime float32) {
	magnetRadius := 70 + 70*float32(g.SkillLevels[SkillPickupRadius])*0.2

	for i := range g.Pickups {
		p := &g.Pickups[i]
		if !p.Active {
			continue
		}

		toPlayer := rl.Vector2Subtract(g.Player.Position, p.Position)
		dist := rl.Vector2Length(toPlayer)

		if dist <= magnetRadius && dist > 0 {
			pull := rl.Vector2Scale(rl.Vector2Normalize(toPlayer), pickupMagnetSpeed)
			p.Position = rl.Vector2Add(p.Position, pull)
		}

		if dist <= g.Player.Radius+6 {
			p.Active = false

			switch p.Kind {
			case PickupXP:
				g.XP += p.Value
			case PickupLifeOrb:
				collectLifeOrb(g)
			case PickupShield:
				if g.Player.ShieldStacks < maxShieldStacks {
					g.Player.ShieldStacks++
				}
				playSFX(g, g.Sounds.MenuConfirm)
			}
		}
	}
}

// collectLifeOrb: two orbs make one extra heart; a single orb alone doesn't
// count toward health yet (see Player.HalfLifeOrb).
func collectLifeOrb(g *Game) {
	if !g.Player.HalfLifeOrb {
		g.Player.HalfLifeOrb = true
		return
	}

	g.Player.HalfLifeOrb = false
	if g.Player.Health < g.Player.MaxHealth {
		g.Player.Health++
	}
}

func startLevelUp(g *Game) {
	g.XP -= g.XPToNext
	g.Level++
	g.XPToNext = 100 + (g.Level-1)*60

	g.PendingChoices = rollLevelUpChoices(g)
	g.MenuIndex = 0
	g.State = LEVEL_UP
	playSFX(g, g.Sounds.MenuConfirm)
}

// isFusedPassive reports whether id is the linked passive of an already
// -evolved weapon: once fused, that passive's level/effect is still exactly
// what weaponDamage/chargeRegenDuration/etc. already read from
// g.SkillLevels (nothing is reset), but it no longer counts as its own
// ability slot or gets offered for further leveling - it's now embedded in
// the evolved weapon's single slot.
func isFusedPassive(g *Game, id SkillID) bool {
	for _, w := range g.Weapons {
		if w.Evolved && skillLinkedPassive[weaponGrantSkill[w.Kind]] == id {
			return true
		}
	}
	return false
}

// equippedSlotCount is how many of the 6 ability slots are currently filled
// (any skill - weapon-grant or passive - with a level above 0, excluding
// passives already fused into an evolved weapon).
func equippedSlotCount(g *Game) int {
	count := 0
	for id := SkillID(0); id < skillCount; id++ {
		if g.SkillLevels[id] > 0 && !isFusedPassive(g, id) {
			count++
		}
	}
	return count
}

func hasWeapon(g *Game, kind WeaponKind) bool {
	for _, w := range g.Weapons {
		if w.Kind == kind {
			return true
		}
	}
	return false
}

// sampleDistinct returns up to count random, non-repeating entries from ids
// (fewer if the pool is smaller) via a partial Fisher-Yates shuffle - this is
// what stops the level-up picker from ever offering the same skill twice.
func sampleDistinct(ids []SkillID, count int) []SkillID {
	pool := append([]SkillID{}, ids...)
	if count > len(pool) {
		count = len(pool)
	}
	for i := 0; i < count; i++ {
		j := i + int(rl.GetRandomValue(0, int32(len(pool)-i-1)))
		pool[i], pool[j] = pool[j], pool[i]
	}
	return pool[:count]
}

// rollLevelUpChoices rolls up to 3 distinct skill picks (new skills only
// offered if there's a free ability slot; owned-but-unmaxed skills are
// always offerable) plus, if any weapon+linked-passive pair has both reached
// level 3, one guaranteed Evolve choice as a 4th option. Once literally
// nothing is left to offer (every slot full and every one of those skills
// maxed), falls back to a choice of direct Life Orb / Shield rewards instead
// of repeating an already-maxed pick.
func rollLevelUpChoices(g *Game) []LevelUpChoice {
	slotsFull := equippedSlotCount(g) >= maxAbilitySlots

	eligible := make([]SkillID, 0, skillCount)
	for id := SkillID(0); id < skillCount; id++ {
		if isFusedPassive(g, id) || g.SkillLevels[id] >= skillDefs[id].MaxLevel {
			continue
		}
		if g.SkillLevels[id] > 0 || !slotsFull {
			eligible = append(eligible, id)
		}
	}

	var choices []LevelUpChoice
	if len(eligible) == 0 {
		choices = rollRewardChoices()
		// Nothing left to level - waves keep escalating regardless (see
		// waveEnemyScale), so grant a small permanent damage bump each time
		// instead of leaving the player stuck at a fixed power level forever.
		g.PostCapDamageLevels++
	} else {
		for _, id := range sampleDistinct(eligible, 3) {
			choices = append(choices, LevelUpChoice{Kind: ChoiceSkill, Skill: id})
		}
	}

	var evolvable []WeaponKind
	for kind, grantSkill := range weaponGrantSkill {
		passive := skillLinkedPassive[grantSkill]
		if hasWeapon(g, kind) && !weaponEvolved(g, kind) && g.SkillLevels[grantSkill] >= 3 && g.SkillLevels[passive] >= 3 {
			evolvable = append(evolvable, kind)
		}
	}
	if len(evolvable) > 0 {
		pick := evolvable[rl.GetRandomValue(0, int32(len(evolvable)-1))]
		choices = append(choices, LevelUpChoice{Kind: ChoiceEvolve, Weapon: pick})
	}

	return choices
}

// rollRewardChoices offers 3 of the 6 direct Life Orb/Shield rewards
// (1/2/3 of each) - the fallback once every ability slot is full and maxed.
func rollRewardChoices() []LevelUpChoice {
	pool := []LevelUpChoice{
		{Kind: ChoiceLifeOrbs, Count: 1},
		{Kind: ChoiceLifeOrbs, Count: 2},
		{Kind: ChoiceLifeOrbs, Count: 3},
		{Kind: ChoiceShields, Count: 1},
		{Kind: ChoiceShields, Count: 2},
		{Kind: ChoiceShields, Count: 3},
	}

	n := len(pool)
	for i := 0; i < 3; i++ {
		j := i + int(rl.GetRandomValue(0, int32(n-i-1)))
		pool[i], pool[j] = pool[j], pool[i]
	}
	return pool[:3]
}

func weaponEvolved(g *Game, kind WeaponKind) bool {
	for _, w := range g.Weapons {
		if w.Kind == kind {
			return w.Evolved
		}
	}
	return false
}

func applySkill(g *Game, id SkillID) {
	g.SkillLevels[id]++

	if kind, ok := weaponForGrantSkill(id); ok {
		grantOrLevelWeapon(g, kind)
		return
	}

	switch id {
	case SkillMoveSpeed:
		g.Player.Speed *= 1.1
	case SkillMaxHP:
		g.Player.MaxHealth++
		g.Player.Health = g.Player.MaxHealth
	}
	// SkillDamage, SkillPickupRadius, SkillCooldown, SkillBarrier are read
	// directly from g.SkillLevels where needed (weaponDamage, magnetRadius,
	// weaponCooldown, updateShieldAndBarrier) - no extra state to update here.
}

func weaponForGrantSkill(id SkillID) (WeaponKind, bool) {
	for kind, grantSkill := range weaponGrantSkill {
		if grantSkill == id {
			return kind, true
		}
	}
	return 0, false
}

func grantOrLevelWeapon(g *Game, kind WeaponKind) {
	for i := range g.Weapons {
		if g.Weapons[i].Kind == kind {
			g.Weapons[i].Level++
			return
		}
	}
	g.Weapons = append(g.Weapons, Weapon{Kind: kind, Level: 1})
}

// applyEvolution fuses a weapon with its linked passive into a super weapon:
// same slot, but a flat power/behavior bonus applied at fire time (see
// updateWeapons) plus a distinct look (see draw.go).
func applyEvolution(g *Game, kind WeaponKind) {
	for i := range g.Weapons {
		if g.Weapons[i].Kind == kind {
			g.Weapons[i].Evolved = true
			triggerShake(g, 10, 0.3)
			playSFX(g, g.Sounds.Critical)
			return
		}
	}
}

func updateBullets(g *Game) {
	bossRect := rl.Rectangle{X: g.Boss.Position.X, Y: g.Boss.Position.Y, Width: g.Boss.Size.X, Height: g.Boss.Size.Y}

	for i := range g.Bullets {
		if !g.Bullets[i].Active {
			continue
		}

		g.Bullets[i].Position = rl.Vector2Add(g.Bullets[i].Position, g.Bullets[i].Velocity)

		if g.BossActive && g.Boss.Health > 0 && rl.CheckCollisionCircleRec(g.Bullets[i].Position, g.Bullets[i].Radius, bossRect) {
			g.Bullets[i].Active = false
			g.Boss.Health -= g.Bullets[i].Damage

			if g.Boss.Health <= 0 {
				g.Score += 1000
			}
		}

		for j := range g.Asteroids {
			if g.Bullets[i].Active && g.Asteroids[j].Active && rl.CheckCollisionCircles(g.Bullets[i].Position, g.Bullets[i].Radius, g.Asteroids[j].Position, g.Asteroids[j].Radius) {
				g.Bullets[i].Active = false
				g.Asteroids[j].Active = false
				g.Score += asteroidScore(g.Asteroids[j].Tier)
				g.Asteroids = breakAsteroid(g.Asteroids, g.Asteroids[j])
				playSFX(g, g.Sounds.Explosion)
			}
		}

		for j := range g.Enemies {
			if !g.Bullets[i].Active || !g.Enemies[j].Active || g.Enemies[j].Phased {
				continue
			}
			kind := enemyKinds[g.Enemies[j].Kind]
			if rl.CheckCollisionCircles(g.Bullets[i].Position, g.Bullets[i].Radius, g.Enemies[j].Position, kind.Radius) {
				g.Bullets[i].Active = false
				damageEnemy(g, j, g.Bullets[i].Damage)
			}
		}

		for j := range g.Projectiles {
			if g.Bullets[i].Active && g.Projectiles[j].Active && rl.CheckCollisionCircles(g.Bullets[i].Position, g.Bullets[i].Radius, g.Projectiles[j].Position, g.Projectiles[j].Radius) {
				g.Bullets[i].Active = false
				g.Projectiles[j].Active = false
				g.Score += 5
			}
		}

		if rl.Vector2Distance(g.Bullets[i].Position, g.Player.Position) > entityDespawnRadius {
			g.Bullets[i].Active = false
		}
	}
}

// damageEnemy applies damage, handles death (score/XP/split/explode), and
// plays feedback - the single path all enemy damage flows through.
func damageEnemy(g *Game, index int, amount int32) {
	kind := enemyKinds[g.Enemies[index].Kind]
	g.Enemies[index].Health -= amount

	if g.Enemies[index].Health > 0 {
		return
	}

	g.Enemies[index].Active = false
	score := kind.Score
	if g.Enemies[index].IsElite {
		score *= 2
	}
	g.Score += score
	playSFX(g, g.Sounds.Explosion)
	gainNerve(g)

	g.Pickups = append(g.Pickups, Pickup{Position: g.Enemies[index].Position, Value: score, Kind: PickupXP, Active: true})

	bonusPos := rl.Vector2Add(g.Enemies[index].Position, rl.NewVector2(8, 8))
	switch roll := rl.GetRandomValue(0, 999); {
	case roll < shieldDropChance:
		g.Pickups = append(g.Pickups, Pickup{Position: bonusPos, Kind: PickupShield, Active: true})
	case roll < shieldDropChance+lifeOrbDropChance:
		g.Pickups = append(g.Pickups, Pickup{Position: bonusPos, Kind: PickupLifeOrb, Active: true})
	}

	if kind.SplitsOnDeath {
		for i := 0; i < kind.SplitCount; i++ {
			angle := float64(rl.GetRandomValue(0, 359)) * rl.Deg2rad
			offset := rl.NewVector2(float32(math.Cos(angle))*10, float32(math.Sin(angle))*10)
			spawnEnemyAt(g, kind.SplitKind, rl.Vector2Add(g.Enemies[index].Position, offset))
		}
	}

	if kind.ExplodesOnDeath && rl.Vector2Distance(g.Player.Position, g.Enemies[index].Position) <= kind.ExplodeRadius+g.Player.Radius {
		damagePlayer(g, enemyDamage(g, kind.ExplodeDamage))
	}
}

func updateAsteroids(g *Game) {
	for i := range g.Asteroids {
		if !g.Asteroids[i].Active {
			continue
		}

		g.Asteroids[i].Position = rl.Vector2Add(g.Asteroids[i].Position, g.Asteroids[i].Velocity)

		if rl.Vector2Distance(g.Asteroids[i].Position, g.Player.Position) > asteroidDespawnRadius {
			g.Asteroids[i].Active = false
			continue
		}

		if g.BlackHole.Active {
			toHole := rl.Vector2Subtract(g.BlackHole.Position, g.Asteroids[i].Position)
			dist := rl.Vector2Length(toHole)

			if dist <= g.BlackHole.InfluenceRadius && dist > 0 {
				pull := rl.Vector2Scale(rl.Vector2Normalize(toHole), blackHoleAsteroidPull)
				g.Asteroids[i].Position = rl.Vector2Add(g.Asteroids[i].Position, pull)
			}

			if dist <= g.BlackHole.Radius {
				g.Asteroids[i].Active = false
				g.Asteroids = breakAsteroid(g.Asteroids, g.Asteroids[i])
				continue
			}
		}

		if g.Player.Health > 0 && g.Player.ImmunityTimer <= 0 && rl.CheckCollisionCircles(g.Player.Position, g.Player.Radius, g.Asteroids[i].Position, g.Asteroids[i].Radius) {
			if g.Player.ShieldActive {
				g.Player.ShieldActive = false
				g.Player.ShieldCooldownTimer = 2.0
			} else {
				damagePlayer(g, enemyDamage(g, 1))
			}

			g.Asteroids[i].Active = false
			g.Asteroids = breakAsteroid(g.Asteroids, g.Asteroids[i])
		}
	}
}

// updateEnemies dispatches movement/attack behavior per EnemyKind.Pattern and
// resolves player contact damage.
func updateEnemies(g *Game, deltaTime float32) {
	for i := range g.Enemies {
		if !g.Enemies[i].Active {
			continue
		}

		kind := enemyKinds[g.Enemies[i].Kind]
		speedMod := float32(1)
		if g.Enemies[i].IsElite {
			speedMod = 1.2
		}

		switch kind.Pattern {
		case PatternChase:
			dir := rl.Vector2Subtract(g.Player.Position, g.Enemies[i].Position)
			if rl.Vector2Length(dir) > 0 {
				g.Enemies[i].Position = rl.Vector2Add(g.Enemies[i].Position, rl.Vector2Scale(rl.Vector2Normalize(dir), kind.Speed*speedMod))
			}
		case PatternZigzag:
			dir := rl.Vector2Subtract(g.Player.Position, g.Enemies[i].Position)
			if rl.Vector2Length(dir) > 0 {
				dir = rl.Vector2Normalize(dir)
				perp := rl.NewVector2(-dir.Y, dir.X)
				wobble := float32(math.Sin(float64(rl.GetTime())*5+float64(i))) * 1.5
				move := rl.Vector2Add(rl.Vector2Scale(dir, kind.Speed*speedMod), rl.Vector2Scale(perp, wobble))
				g.Enemies[i].Position = rl.Vector2Add(g.Enemies[i].Position, move)
			}
		case PatternCharge:
			g.Enemies[i].StateTimer -= deltaTime
			if !g.Enemies[i].Charging {
				if g.Enemies[i].StateTimer <= 0 {
					g.Enemies[i].Charging = true
					g.Enemies[i].StateTimer = 0.4
					dir := rl.Vector2Normalize(rl.Vector2Subtract(g.Player.Position, g.Enemies[i].Position))
					g.Enemies[i].Velocity = rl.Vector2Scale(dir, kind.Speed*speedMod*6)
				}
			} else {
				g.Enemies[i].Position = rl.Vector2Add(g.Enemies[i].Position, g.Enemies[i].Velocity)
				if g.Enemies[i].StateTimer <= 0 {
					g.Enemies[i].Charging = false
					g.Enemies[i].StateTimer = 1.2
					g.Enemies[i].Velocity = rl.Vector2{}
				}
			}
		case PatternOrbit:
			g.Enemies[i].OrbitAngle += float64(deltaTime) * 1.5 * float64(speedMod)
			if g.Enemies[i].OrbitDist > kind.Radius+20 {
				// 9/sec, matching the frame-based rate this replaced (was -0.15/frame
				// at the assumed 60fps) so the spiral-in speed stays in sync with
				// OrbitAngle's already-deltaTime-scaled rotation at any frame rate.
				g.Enemies[i].OrbitDist -= 9 * speedMod * deltaTime
			}
			g.Enemies[i].Position = rl.Vector2Add(g.Player.Position, rl.NewVector2(
				float32(math.Cos(g.Enemies[i].OrbitAngle))*g.Enemies[i].OrbitDist,
				float32(math.Sin(g.Enemies[i].OrbitAngle))*g.Enemies[i].OrbitDist,
			))
		case PatternTurret:
			g.Enemies[i].StateTimer -= deltaTime
			if g.Enemies[i].StateTimer <= 0 && rl.Vector2Distance(g.Player.Position, g.Enemies[i].Position) < turretFireRange {
				g.Enemies[i].StateTimer = kind.FireInterval / speedMod
				dir := rl.Vector2Normalize(rl.Vector2Subtract(g.Player.Position, g.Enemies[i].Position))
				g.Projectiles = append(g.Projectiles, BossProjectile{
					Position: g.Enemies[i].Position,
					Velocity: rl.Vector2Scale(dir, kind.ProjectileSpeed),
					Radius:   6,
					Active:   true,
					Damage:   crossfireProjectileDamage,
				})
			}
		case PatternSpawner:
			g.Enemies[i].StateTimer -= deltaTime
			if g.Enemies[i].StateTimer <= 0 {
				g.Enemies[i].StateTimer = kind.SpawnInterval / speedMod
				for s := 0; s < kind.SpawnCount; s++ {
					angle := float64(rl.GetRandomValue(0, 359)) * rl.Deg2rad
					offset := rl.NewVector2(float32(math.Cos(angle))*30, float32(math.Sin(angle))*30)
					spawnEnemyAt(g, kind.SpawnKind, rl.Vector2Add(g.Enemies[i].Position, offset))
				}
			}
		case PatternStationary:
			// doesn't move; pure contact hazard.
		}

		if kind.PhaseCycle {
			g.Enemies[i].StateTimer -= deltaTime
			if g.Enemies[i].StateTimer <= 0 {
				g.Enemies[i].Phased = !g.Enemies[i].Phased
				g.Enemies[i].StateTimer = 1.5
			}
		}

		if rl.Vector2Distance(g.Enemies[i].Position, g.Player.Position) > entityDespawnRadius {
			g.Enemies[i].Active = false
			continue
		}

		if g.Enemies[i].Phased {
			continue
		}

		collides := g.Player.Health > 0 && rl.CheckCollisionCircles(g.Player.Position, g.Player.Radius, g.Enemies[i].Position, kind.Radius)

		// Dashing takes priority over normal contact rules: shield+dash plows
		// through and destroys everything in the path with no self-damage;
		// dashing alone still damages the enemy but costs the player health
		// per enemy hit (ignores the usual post-hit immunity window, since
		// it's meant to sting for every enemy plowed through in one dash).
		if collides && g.Player.Dashing && !g.Enemies[i].HitByDash {
			g.Enemies[i].HitByDash = true
			if g.Player.ShieldActive {
				damageEnemy(g, i, 999)
			} else {
				damageEnemy(g, i, dashDamage)
				damagePlayer(g, enemyDamage(g, kind.ContactDamage))
			}
			continue
		}

		if collides && !g.Player.Dashing && g.Player.ImmunityTimer <= 0 {
			switch {
			case kind.IsLeech:
				g.Player.SlowTimer = 2.0
				g.Player.ImmunityTimer = 1.0
				damageEnemy(g, i, 999)
			case g.Player.ShieldActive:
				g.Player.ShieldActive = false
				g.Player.ShieldCooldownTimer = 2.0
				damageEnemy(g, i, 999)
			default:
				damagePlayer(g, enemyDamage(g, kind.ContactDamage))
			}
		}
	}
}

func updateProjectiles(g *Game) {
	for i := range g.Projectiles {
		if !g.Projectiles[i].Active {
			continue
		}

		if g.Projectiles[i].Homing {
			if g.Projectiles[i].FromPlayer {
				// Player-fired: keep chasing the nearest enemy; if none, keep
				// flying straight (never retarget the player who fired it).
				if target, ok := nearestEnemy(g, g.Projectiles[i].Position); ok {
					direction := rl.Vector2Normalize(rl.Vector2Subtract(target, g.Projectiles[i].Position))
					g.Projectiles[i].Velocity = rl.Vector2Scale(direction, homingProjSpeed*1.5)
				}
			} else {
				direction := rl.Vector2Normalize(rl.Vector2Subtract(g.Player.Position, g.Projectiles[i].Position))
				g.Projectiles[i].Velocity = rl.Vector2Scale(direction, homingProjSpeed)
			}
		}

		g.Projectiles[i].Position = rl.Vector2Add(g.Projectiles[i].Position, g.Projectiles[i].Velocity)

		if rl.Vector2Distance(g.Projectiles[i].Position, g.Player.Position) > entityDespawnRadius {
			g.Projectiles[i].Active = false
			continue
		}

		for j := range g.Asteroids {
			if g.Asteroids[j].Active && rl.CheckCollisionCircles(g.Projectiles[i].Position, g.Projectiles[i].Radius, g.Asteroids[j].Position, g.Asteroids[j].Radius) {
				g.Projectiles[i].Active = false
				g.Asteroids[j].Active = false
				g.Asteroids = breakAsteroid(g.Asteroids, g.Asteroids[j])
				break
			}
		}

		for j := range g.Enemies {
			if g.Projectiles[i].Active && g.Enemies[j].Active && !g.Enemies[j].Phased && rl.CheckCollisionCircles(g.Projectiles[i].Position, g.Projectiles[i].Radius, g.Enemies[j].Position, enemyKinds[g.Enemies[j].Kind].Radius) {
				g.Projectiles[i].Active = false
				damageEnemy(g, j, g.Projectiles[i].Damage)
				break
			}
		}

		if g.Projectiles[i].Active && !g.Projectiles[i].FromPlayer && g.Player.Health > 0 && !g.Player.ShieldActive && !g.Player.Dashing && g.Player.ImmunityTimer <= 0 && rl.CheckCollisionCircles(g.Player.Position, g.Player.Radius, g.Projectiles[i].Position, g.Projectiles[i].Radius) {
			g.Projectiles[i].Active = false
			damagePlayer(g, enemyDamage(g, 1))
		}
	}
}

func filterDeadEntities(g *Game) {
	activeAsteroids := []Asteroid{}
	for _, a := range g.Asteroids {
		if a.Active {
			activeAsteroids = append(activeAsteroids, a)
		}
	}
	g.Asteroids = activeAsteroids

	activeBullets := []Bullet{}
	for _, b := range g.Bullets {
		if b.Active {
			activeBullets = append(activeBullets, b)
		}
	}
	g.Bullets = activeBullets

	activeProjectiles := []BossProjectile{}
	for _, p := range g.Projectiles {
		if p.Active {
			activeProjectiles = append(activeProjectiles, p)
		}
	}
	g.Projectiles = activeProjectiles

	activeEnemies := []Enemy{}
	for _, e := range g.Enemies {
		if e.Active {
			activeEnemies = append(activeEnemies, e)
		}
	}
	g.Enemies = activeEnemies

	activePickups := []Pickup{}
	for _, p := range g.Pickups {
		if p.Active {
			activePickups = append(activePickups, p)
		}
	}
	g.Pickups = activePickups

	activeMines := []Mine{}
	for _, m := range g.Mines {
		if m.Active {
			activeMines = append(activeMines, m)
		}
	}
	g.Mines = activeMines
}

// updateBgParticles drifts the decorative background motes and, if a black
// hole is active, warps nearby ones toward it (visually distorting that patch
// of background) before recycling them once they reach its core. Positions
// are tile-space offsets (see tiledWorldPos in draw.go), not world positions.
func updateBgParticles(g *Game) {
	tileW, tileH := float32(g.ScreenWidth), float32(g.ScreenHeight)

	for i := range g.BgParticles {
		p := &g.BgParticles[i]
		p.Position = rl.Vector2Add(p.Position, p.Velocity)

		if p.Position.X < 0 {
			p.Position.X += tileW
		}
		if p.Position.X > tileW {
			p.Position.X -= tileW
		}
		if p.Position.Y < 0 {
			p.Position.Y += tileH
		}
		if p.Position.Y > tileH {
			p.Position.Y -= tileH
		}
	}
}

func updateBlackHole(g *Game, deltaTime float32) {
	g.BlackHole.Timer -= deltaTime

	if !g.BlackHole.Active && g.BlackHole.Timer <= 0 {
		angle := float64(rl.GetRandomValue(0, 359)) * rl.Deg2rad
		dist := float32(rl.GetRandomValue(150, 350))
		g.BlackHole.Position = rl.Vector2Add(g.Player.Position, rl.NewVector2(float32(math.Cos(angle))*dist, float32(math.Sin(angle))*dist))
		g.BlackHole.Radius = 25
		g.BlackHole.InfluenceRadius = 140
		g.BlackHole.Active = true
		g.BlackHole.Timer = float32(rl.GetRandomValue(60, 100)) / 10.0
	} else if g.BlackHole.Active && g.BlackHole.Timer <= 0 {
		g.BlackHole.Active = false
		g.BlackHole.Timer = float32(rl.GetRandomValue(50, 90)) / 10.0
	}

	if g.BlackHole.Active {
		toPlayer := rl.Vector2Subtract(g.Player.Position, g.BlackHole.Position)
		dist := rl.Vector2Length(toPlayer)

		if dist > g.BlackHole.InfluenceRadius {
			drift := rl.Vector2Scale(rl.Vector2Normalize(toPlayer), blackHoleChaseSpeed)
			g.BlackHole.Position = rl.Vector2Add(g.BlackHole.Position, drift)
		}
	}
}

func updateBoss(g *Game, deltaTime float32, bossCenter rl.Vector2) {
	switch g.Boss.State {
	case IDLE:
		g.Boss.AttackTimer -= deltaTime

		if g.Boss.AttackTimer <= 0 && g.Boss.Health > 0 {
			g.Boss.Attack = BossAttack(rl.GetRandomValue(0, 3))
			g.Boss.State = WINDING_UP
			g.Boss.StateTimer = bossWindupDuration(g.Boss.Attack)

			switch g.Boss.Attack {
			case AttackHoming:
				g.Boss.Color = colorBossHoming
			case AttackSpread:
				g.Boss.Color = colorBossSpread
				g.SpreadWindupShots = 0
			case AttackSlam:
				g.Boss.Color = colorAccent
			case AttackBeam:
				g.Boss.Color = colorBossBeam
			default:
				g.Boss.Color = colorAccentDim
			}

			playSFX(g, g.Sounds.BossWindup)
		}
	case WINDING_UP:
		g.Boss.StateTimer -= deltaTime
		g.Boss.TargetPosition = g.Player.Position

		if g.Boss.StateTimer <= 0 {
			startBossAttack(g, bossCenter)
		}
	case SHOOTING:
		g.Boss.StateTimer -= deltaTime

		if g.Boss.Attack == AttackSlam {
			progress := 1 - g.Boss.StateTimer/slamDuration
			if progress < 0 {
				progress = 0
			}
			if progress > 1 {
				progress = 1
			}
			radius := maxSlamRadius * progress

			for i := range g.Asteroids {
				if g.Asteroids[i].Active && rl.Vector2Distance(bossCenter, g.Asteroids[i].Position) <= radius+g.Asteroids[i].Radius {
					g.Asteroids[i].Active = false
					g.Asteroids = breakAsteroid(g.Asteroids, g.Asteroids[i])
				}
			}

			if !g.Boss.SlamHit {
				if resolved, hit := resolveExpandingWaveHit(g, bossCenter, radius, true); resolved {
					g.Boss.SlamHit = true
					if hit {
						damagePlayer(g, enemyDamage(g, 1))
					}
				}
			}
		}

		if g.Boss.Attack == AttackBeam {
			beamStart := bossCenter
			direction := rl.Vector2Normalize(rl.Vector2Subtract(g.Boss.TargetPosition, bossCenter))
			beamEnd := rl.Vector2Add(beamStart, rl.Vector2Scale(direction, 2000))

			blocked := false
			playerDist := rl.Vector2Distance(bossCenter, g.Player.Position)

			for i := range g.Asteroids {
				if !g.Asteroids[i].Active || !CheckCollisionCircleLine(g.Asteroids[i].Position, g.Asteroids[i].Radius, beamStart, beamEnd) {
					continue
				}

				if rl.Vector2Distance(bossCenter, g.Asteroids[i].Position) < playerDist {
					blocked = true
				}

				g.Asteroids[i].Active = false
				g.Asteroids = breakAsteroid(g.Asteroids, g.Asteroids[i])
			}

			if !blocked && g.Player.Health > 0 && !g.Player.ShieldActive && !g.Player.Dashing && g.Player.ImmunityTimer <= 0 && CheckCollisionCircleLine(g.Player.Position, g.Player.Radius, beamStart, beamEnd) {
				damagePlayer(g, enemyDamage(g, 1))
			}
		}

		if g.Boss.StateTimer <= 0 {
			g.Boss.State = IDLE
			g.Boss.AttackTimer = float32(rl.GetRandomValue(15, 40)) / 10.0
			g.Boss.Color = colorBossIdle
		}
	}
}

func bossWindupDuration(attack BossAttack) float32 {
	if attack == AttackSlam {
		return 1.3
	}
	return 1.0
}

func startBossAttack(g *Game, bossCenter rl.Vector2) {
	direction := rl.Vector2Subtract(g.Boss.TargetPosition, bossCenter)
	g.Boss.BeamRotation = float32(math.Atan2(float64(direction.Y), float64(direction.X))) * rl.Rad2deg
	aimDirection := rl.Vector2Normalize(direction)

	g.Boss.State = SHOOTING

	switch g.Boss.Attack {
	case AttackBeam:
		g.Boss.StateTimer = 2.0
		playSFX(g, g.Sounds.BeamFire)
		triggerShake(g, 5, 0.2)
	case AttackHoming:
		g.Boss.StateTimer = 3.0
		playSFX(g, g.Sounds.HomingLaunch)
		for i := 0; i < 3; i++ {
			angleOffset := float64(i-1) * 12 * rl.Deg2rad
			angle := math.Atan2(float64(aimDirection.Y), float64(aimDirection.X)) + angleOffset
			vel := rl.NewVector2(float32(math.Cos(angle))*homingProjSpeed, float32(math.Sin(angle))*homingProjSpeed)

			g.Projectiles = append(g.Projectiles, BossProjectile{
				Position: bossCenter,
				Velocity: vel,
				Radius:   8,
				Homing:   true,
				Active:   true,
				Damage:   crossfireProjectileDamage,
			})
		}
	case AttackSpread:
		g.Boss.StateTimer = 0.4
		playSFX(g, g.Sounds.SpreadBurst)
		triggerShake(g, 5, 0.2)
		baseAngle := math.Atan2(float64(aimDirection.Y), float64(aimDirection.X))

		bonus := g.SpreadWindupShots
		if bonus > 12 {
			bonus = 12
		}
		half := (7 + bonus) / 2
		g.SpreadWindupShots = 0

		for i := -half; i <= half; i++ {
			angle := baseAngle + float64(i)*15*rl.Deg2rad
			vel := rl.NewVector2(float32(math.Cos(angle))*spreadProjSpeed, float32(math.Sin(angle))*spreadProjSpeed)

			g.Projectiles = append(g.Projectiles, BossProjectile{
				Position: bossCenter,
				Velocity: vel,
				Radius:   7,
				Homing:   false,
				Active:   true,
				Damage:   crossfireProjectileDamage,
			})
		}
	case AttackSlam:
		g.Boss.StateTimer = slamDuration
		g.Boss.SlamHit = false
		playSFX(g, g.Sounds.SlamBoom)
		triggerShake(g, 10, 0.4)
	}
}
