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
	chargeWindup          = float32(1.5)
	chargeFullTime        = float32(3.0)
	chargeOverload        = float32(5.0)
	chargeMaxBonus        = int32(45)
	baseBulletDamage      = int32(5)
)

// UpdateGame advances the game by one frame and reports whether the player chose to exit.
func UpdateGame(g *Game, deltaTime float32) bool {
	if rl.IsKeyPressed(rl.KeyF11) {
		toggleFullscreen(g)
	}
	syncScreenSize(g)

	updateBgParticles(g)

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
	case GAME_OVER:
		return updateGameOver(g)
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
	index, confirmed := updateMenuSelection(g.MenuIndex, 2)
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

func updatePaused(g *Game) bool {
	if rl.IsKeyPressed(rl.KeyEscape) {
		g.State = GAMEPLAY
		return false
	}

	index, confirmed := updateMenuSelection(g.MenuIndex, 3)
	playMenuSounds(g, index, confirmed)
	g.MenuIndex = index

	if confirmed {
		switch index {
		case 0:
			g.State = GAMEPLAY
		case 1:
			resetRun(g)
		case 2:
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

	index, confirmed := updateMenuSelection(g.MenuIndex, 2)
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

func playMenuSounds(g *Game, newIndex int32, confirmed bool) {
	if confirmed {
		rl.PlaySound(g.Sounds.MenuConfirm)
	} else if newIndex != g.MenuIndex {
		rl.PlaySound(g.Sounds.MenuMove)
	}
}

// damagePlayer is the single path all player damage flows through: it applies
// the hit, cancels any in-progress charge, and transitions to GAME_OVER if
// health runs out.
func damagePlayer(g *Game, amount int32) {
	g.Player.Health -= amount
	g.Player.ImmunityTimer = 1.0
	g.Player.Charging = false
	g.Player.ChargeTimer = 0

	if g.Player.Health <= 0 {
		g.State = GAME_OVER
		g.MenuIndex = 0
		rl.PlaySound(g.Sounds.Defeat)
		triggerShake(g, 12, 0.5)
		triggerHitPause(g, 0.15)
	} else {
		rl.PlaySound(g.Sounds.Hit)
		triggerShake(g, 4, 0.2)
	}
}

func updateGameplay(g *Game, deltaTime float32) {
	bossRect := rl.Rectangle{X: g.Boss.Position.X, Y: g.Boss.Position.Y, Width: g.Boss.Size.X, Height: g.Boss.Size.Y}
	bossCenter := rl.NewVector2(g.Boss.Position.X+g.Boss.Size.X/2, g.Boss.Position.Y+g.Boss.Size.Y/2)

	if rl.IsKeyPressed(rl.KeyEscape) {
		g.State = PAUSED
		g.MenuIndex = 0
		return
	}

	inBlackHole := g.BlackHole.Active && rl.Vector2Distance(g.Player.Position, g.BlackHole.Position) <= g.BlackHole.InfluenceRadius

	effectiveSpeed := g.Player.Speed
	if inBlackHole {
		effectiveSpeed -= blackHoleSlow
		if effectiveSpeed < 1 {
			effectiveSpeed = 1
		}
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

	if g.Player.Charging {
		moveDelta = rl.Vector2Scale(moveDelta, 0.5)
	}

	g.Player.Position = rl.Vector2Add(g.Player.Position, moveDelta)

	if inBlackHole {
		toHole := rl.Vector2Subtract(g.BlackHole.Position, g.Player.Position)
		if rl.Vector2Length(toHole) > 0 {
			pull := rl.Vector2Scale(rl.Vector2Normalize(toHole), blackHolePull)
			g.Player.Position = rl.Vector2Add(g.Player.Position, pull)
		}
	}

	if g.Player.Position.X < float32(0+g.Player.Radius) {
		g.Player.Position.X = 0 + g.Player.Radius
	}

	if g.Player.Position.X > float32(g.ScreenWidth-int32(g.Player.Radius)) {
		g.Player.Position.X = float32(g.ScreenWidth - int32(g.Player.Radius))
	}

	if g.Player.Position.Y < float32(0+g.Player.Radius) {
		g.Player.Position.Y = 0 + g.Player.Radius
	}

	if g.Player.Position.Y > float32(g.ScreenHeight-int32(g.Player.Radius)) {
		g.Player.Position.Y = float32(g.ScreenHeight - int32(g.Player.Radius))
	}

	if g.Boss.Health > 0 && rl.CheckCollisionCircleRec(g.Player.Position, g.Player.Radius, bossRect) {
		g.Boss.Health -= bossRamDamage
		if g.Boss.Health <= 0 {
			g.Score += 1000
		}
		triggerShake(g, 14, 0.4)
		triggerHitPause(g, 0.12)
		damagePlayer(g, g.Player.Health)
	}

	if g.Player.ImmunityTimer > 0 {
		g.Player.ImmunityTimer -= deltaTime
	}

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

	if rl.IsKeyPressed(rl.KeyJ) && !g.Player.ShieldActive && g.Player.ShieldCooldownTimer <= 0 && !g.Player.Charging {
		g.Player.ShieldActive = true
		g.Player.ShieldTimer = 3.0
	}

	if rl.IsKeyDown(rl.KeySpace) {
		if !g.Player.Charging {
			g.Player.HoldTimer += deltaTime

			if g.Player.HoldTimer >= chargeWindup {
				g.Player.Charging = true
				g.Player.ChargeTimer = 0
			}
		} else {
			g.Player.ChargeTimer += deltaTime

			if g.Player.ChargeTimer >= chargeOverload {
				g.Player.Charging = false
				g.Player.ChargeTimer = 0
				g.Player.HoldTimer = 0
				damagePlayer(g, 1)
			}
		}
	}

	if rl.IsKeyReleased(rl.KeySpace) {
		fraction := float32(0)
		if g.Player.Charging {
			fraction = g.Player.ChargeTimer / chargeFullTime
			if fraction > 1 {
				fraction = 1
			}
		}

		bulletVelocity := rl.NewVector2(0, -projectileSpeed)

		bulletColor := colorAccent
		if fraction >= 1 {
			bulletColor = colorCrit
		}

		newBullet := Bullet{
			Position: g.Player.Position,
			Velocity: bulletVelocity,
			Radius:   projectileSize + fraction*8,
			Color:    bulletColor,
			Active:   true,
			Damage:   baseBulletDamage + int32(fraction*float32(chargeMaxBonus)),
		}

		g.Bullets = append(g.Bullets, newBullet)
		rl.PlaySound(g.Sounds.Shoot)

		if g.Boss.State == WINDING_UP && g.Boss.Attack == AttackSpread {
			g.SpreadWindupShots++
		}

		g.Player.Charging = false
		g.Player.ChargeTimer = 0
		g.Player.HoldTimer = 0
	}

	g.AsteroidSpawnTimer -= deltaTime

	if g.AsteroidSpawnTimer <= 0 && len(g.Asteroids) < maxAsteroids {
		g.AsteroidSpawnTimer = float32(rl.GetRandomValue(8, 16)) / 10.0

		tier := TierLarge
		if rl.GetRandomValue(0, 1) == 1 {
			tier = TierMedium
		}

		spawnPos := rl.NewVector2(float32(rl.GetRandomValue(0, g.ScreenWidth)), -40)
		aimPoint := rl.Vector2Add(g.Player.Position, rl.NewVector2(float32(rl.GetRandomValue(-180, 180)), float32(rl.GetRandomValue(-80, 80))))
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

	updateBlackHole(g, deltaTime)

	if g.Player.Health > 0 && g.BlackHole.Active && rl.Vector2Distance(g.Player.Position, g.BlackHole.Position) <= g.BlackHole.Radius {
		g.Player.BlackHoleCoreTimer += deltaTime
		if g.Player.BlackHoleCoreTimer >= 1.0 {
			damagePlayer(g, g.Player.Health)
		}
	} else {
		g.Player.BlackHoleCoreTimer = 0
	}

	updateBoss(g, deltaTime, bossCenter)

	for i := range g.Bullets {
		if !g.Bullets[i].Active {
			continue
		}

		g.Bullets[i].Position = rl.Vector2Add(g.Bullets[i].Position, g.Bullets[i].Velocity)

		if g.Boss.Health > 0 && rl.CheckCollisionCircleRec(g.Bullets[i].Position, g.Bullets[i].Radius, bossRect) {
			g.Bullets[i].Active = false
			g.Boss.Health -= g.Bullets[i].Damage

			if g.Bullets[i].Damage >= baseBulletDamage+chargeMaxBonus {
				rl.PlaySound(g.Sounds.Critical)
				triggerShake(g, 8, 0.25)
				triggerHitPause(g, 0.08)
			}

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
				rl.PlaySound(g.Sounds.Explosion)
			}
		}

		for j := range g.Projectiles {
			if g.Bullets[i].Active && g.Projectiles[j].Active && rl.CheckCollisionCircles(g.Bullets[i].Position, g.Bullets[i].Radius, g.Projectiles[j].Position, g.Projectiles[j].Radius) {
				g.Bullets[i].Active = false
				g.Projectiles[j].Active = false
				g.Score += 5
			}
		}

		if g.Bullets[i].Position.X < 0 || g.Bullets[i].Position.X > float32(g.ScreenWidth) || g.Bullets[i].Position.Y < 0 || g.Bullets[i].Position.Y > float32(g.ScreenHeight) {
			g.Bullets[i].Active = false
		}
	}

	for i := range g.Asteroids {
		if !g.Asteroids[i].Active {
			continue
		}

		g.Asteroids[i].Position = rl.Vector2Add(g.Asteroids[i].Position, g.Asteroids[i].Velocity)

		margin := g.Asteroids[i].Radius + 40
		if g.Asteroids[i].Position.X < -margin || g.Asteroids[i].Position.X > float32(g.ScreenWidth)+margin || g.Asteroids[i].Position.Y > float32(g.ScreenHeight)+margin {
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
				damagePlayer(g, 1)
			}

			g.Asteroids[i].Active = false
			g.Asteroids = breakAsteroid(g.Asteroids, g.Asteroids[i])
		}
	}

	for i := range g.Projectiles {
		if !g.Projectiles[i].Active {
			continue
		}

		if g.Projectiles[i].Homing {
			direction := rl.Vector2Normalize(rl.Vector2Subtract(g.Player.Position, g.Projectiles[i].Position))
			g.Projectiles[i].Velocity = rl.Vector2Scale(direction, homingProjSpeed)
		}

		g.Projectiles[i].Position = rl.Vector2Add(g.Projectiles[i].Position, g.Projectiles[i].Velocity)

		margin := g.Projectiles[i].Radius + 40
		if g.Projectiles[i].Position.X < -margin || g.Projectiles[i].Position.X > float32(g.ScreenWidth)+margin || g.Projectiles[i].Position.Y < -margin || g.Projectiles[i].Position.Y > float32(g.ScreenHeight)+margin {
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

		if g.Projectiles[i].Active && g.Player.Health > 0 && !g.Player.ShieldActive && g.Player.ImmunityTimer <= 0 && rl.CheckCollisionCircles(g.Player.Position, g.Player.Radius, g.Projectiles[i].Position, g.Projectiles[i].Radius) {
			g.Projectiles[i].Active = false
			damagePlayer(g, 1)
		}
	}

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

	if g.Boss.Health <= 0 {
		g.Boss.Health = 0
		g.Boss.Color = colorStructDark
		g.State = GAME_OVER
		g.MenuIndex = 0
		if g.Player.Health > 0 {
			rl.PlaySound(g.Sounds.Victory)
			triggerShake(g, 14, 0.6)
			triggerHitPause(g, 0.2)
		}
	}
}

// updateBgParticles drifts the decorative background motes and, if a black
// hole is active, warps nearby ones toward it (visually distorting that patch
// of background) before recycling them once they reach its core.
func updateBgParticles(g *Game) {
	for i := range g.BgParticles {
		p := &g.BgParticles[i]
		p.Position = rl.Vector2Add(p.Position, p.Velocity)

		if p.Position.X < -10 {
			p.Position.X = float32(g.ScreenWidth) + 10
		}
		if p.Position.X > float32(g.ScreenWidth)+10 {
			p.Position.X = -10
		}
		if p.Position.Y < -10 {
			p.Position.Y = float32(g.ScreenHeight) + 10
		}
		if p.Position.Y > float32(g.ScreenHeight)+10 {
			p.Position.Y = -10
		}

		if g.BlackHole.Active {
			toHole := rl.Vector2Subtract(g.BlackHole.Position, p.Position)
			dist := rl.Vector2Length(toHole)

			if dist <= g.BlackHole.InfluenceRadius*1.5 && dist > 1 {
				pull := rl.Vector2Scale(rl.Vector2Normalize(toHole), 0.4)
				p.Position = rl.Vector2Add(p.Position, pull)
			}

			if dist <= g.BlackHole.Radius {
				p.Position = rl.NewVector2(float32(rl.GetRandomValue(0, g.ScreenWidth)), float32(rl.GetRandomValue(0, g.ScreenHeight)))
			}
		}
	}
}

func updateBlackHole(g *Game, deltaTime float32) {
	g.BlackHole.Timer -= deltaTime

	if !g.BlackHole.Active && g.BlackHole.Timer <= 0 {
		angle := float64(rl.GetRandomValue(0, 359)) * rl.Deg2rad
		dist := float32(rl.GetRandomValue(120, 260))
		spawnPos := rl.Vector2Add(g.Player.Position, rl.NewVector2(float32(math.Cos(angle))*dist, float32(math.Sin(angle))*dist))

		if spawnPos.X < 60 {
			spawnPos.X = 60
		}
		if spawnPos.X > float32(g.ScreenWidth-60) {
			spawnPos.X = float32(g.ScreenWidth - 60)
		}
		if spawnPos.Y < 200 {
			spawnPos.Y = 200
		}
		if spawnPos.Y > float32(g.ScreenHeight-50) {
			spawnPos.Y = float32(g.ScreenHeight - 50)
		}

		g.BlackHole.Position = spawnPos
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

			if g.BlackHole.Position.X < 60 {
				g.BlackHole.Position.X = 60
			}
			if g.BlackHole.Position.X > float32(g.ScreenWidth-60) {
				g.BlackHole.Position.X = float32(g.ScreenWidth - 60)
			}
			if g.BlackHole.Position.Y < 200 {
				g.BlackHole.Position.Y = 200
			}
			if g.BlackHole.Position.Y > float32(g.ScreenHeight-50) {
				g.BlackHole.Position.Y = float32(g.ScreenHeight - 50)
			}
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
			default:
				g.Boss.Color = colorAccentDim
			}

			rl.PlaySound(g.Sounds.BossWindup)
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

			if !g.Boss.SlamHit && !g.Player.ShieldActive && g.Player.ImmunityTimer <= 0 &&
				rl.Vector2Distance(g.Player.Position, bossCenter) <= radius+g.Player.Radius {
				g.Boss.SlamHit = true
				damagePlayer(g, 1)
			}
		}

		if g.Boss.Attack == AttackBeam {
			beamStart := bossCenter
			direction := rl.Vector2Normalize(rl.Vector2Subtract(g.Boss.TargetPosition, bossCenter))
			beamEnd := rl.Vector2Add(beamStart, rl.Vector2Scale(direction, float32(g.ScreenWidth)+1.5))

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

			if !blocked && g.Player.Health > 0 && !g.Player.ShieldActive && g.Player.ImmunityTimer <= 0 && CheckCollisionCircleLine(g.Player.Position, g.Player.Radius, beamStart, beamEnd) {
				damagePlayer(g, 1)
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
		rl.PlaySound(g.Sounds.BeamFire)
		triggerShake(g, 5, 0.2)
	case AttackHoming:
		g.Boss.StateTimer = 3.0
		rl.PlaySound(g.Sounds.HomingLaunch)
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
			})
		}
	case AttackSpread:
		g.Boss.StateTimer = 0.4
		rl.PlaySound(g.Sounds.SpreadBurst)
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
			})
		}
	case AttackSlam:
		g.Boss.StateTimer = slamDuration
		g.Boss.SlamHit = false
		rl.PlaySound(g.Sounds.SlamBoom)
		triggerShake(g, 10, 0.4)
	}
}
