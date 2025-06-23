package main

import (
	"fmt"
	"math"

	rl "github.com/gen2brain/raylib-go/raylib"
)

type GameState int
type BossState int

const (
	TITLE GameState = iota
	GAMEPLAY
	PAUSED
	GAME_OVER
)

const (
	IDLE BossState = iota
	WINDING_UP
	SHOOTING
)

func CheckCollisionCircleLine(center rl.Vector2, radius float32, startPos rl.Vector2, endPos rl.Vector2) bool {
	return rl.CheckCollisionPointLine(center, startPos, endPos, int32(radius))
}

type Game struct {
	ScreenWidth  int32
	ScreenHeight int32
	State        GameState
	Player       Player
	Boss         Boss
	Bullets      []Bullet
	GameOverMsg  string
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
}

type Boss struct {
	Position       rl.Vector2
	Size           rl.Vector2
	Color          rl.Color
	Health         int32
	MaxHealth      int32
	State          BossState
	AttackTimer    float32
	StateTimer     float32
	TargetPosition rl.Vector2
	BeamRotation   float32
}

type Bullet struct {
	Position rl.Vector2
	Velocity rl.Vector2
	Radius   float32
	Color    rl.Color
	Active   bool
}

func InitGame() *Game {
	g := &Game{}

	g.ScreenWidth = 800
	g.ScreenHeight = 600
	g.State = TITLE

	g.Player = Player{
		Position: rl.NewVector2(float32(g.ScreenWidth)/2, float32(g.ScreenHeight)/2),
		Radius:   15,
		Color:    rl.Red,
		Speed:    5,
		Health:   5,
	}

	g.Boss = Boss{
		Position:    rl.NewVector2(float32(g.ScreenWidth)/2-50, 50),
		Size:        rl.NewVector2(100, 100),
		Color:       rl.Blue,
		Health:      500,
		MaxHealth:   500,
		State:       IDLE,
		AttackTimer: 5.0,
	}

	g.Bullets = []Bullet{}

	return g
}

func main() {
	game := InitGame()

	rl.InitWindow(game.ScreenWidth, game.ScreenHeight, "Raylib-Go - Boss fight")
	defer rl.CloseWindow()
	rl.SetTargetFPS(60)

	projectileSpeed := float32(10)
	projectileSize := float32(7)
	var gameOver bool = false

	for !rl.WindowShouldClose() {
		deltaTime := rl.GetFrameTime()
		bossRect := rl.Rectangle{X: game.Boss.Position.X, Y: game.Boss.Position.Y, Width: game.Boss.Size.X, Height: game.Boss.Size.Y}

		if !gameOver {
			if rl.IsKeyDown(rl.KeyD) {
				game.Player.Position.X += game.Player.Speed
			}

			if rl.IsKeyDown(rl.KeyA) {
				game.Player.Position.X -= game.Player.Speed
			}

			if rl.IsKeyDown(rl.KeyW) {
				game.Player.Position.Y -= game.Player.Speed
			}

			if rl.IsKeyDown(rl.KeyS) {
				game.Player.Position.Y += game.Player.Speed
			}

			if game.Player.Position.X < float32(0+game.Player.Radius) {
				game.Player.Position.X = 0 + game.Player.Radius
			}

			if game.Player.Position.X > float32(game.ScreenWidth-int32(game.Player.Radius)) {
				game.Player.Position.X = float32(game.ScreenWidth - int32(game.Player.Radius))
			}

			if game.Player.Position.Y < float32(0+game.Player.Radius) {
				game.Player.Position.Y = 0 + game.Player.Radius
			}

			if game.Player.Position.Y > float32(game.ScreenHeight-int32(game.Player.Radius)) {
				game.Player.Position.Y = float32(game.ScreenHeight - int32(game.Player.Radius))
			}

			if game.Player.ImmunityTimer > 0 {
				game.Player.ImmunityTimer -= deltaTime
			}

			if game.Player.ShieldCooldownTimer > 0 {
				game.Player.ShieldCooldownTimer -= deltaTime
			}

			if game.Player.ShieldActive {
				game.Player.ShieldTimer -= deltaTime

				if game.Player.ShieldTimer <= 0 {
					game.Player.ShieldActive = false
					game.Player.ShieldCooldownTimer = 2.0
				}
			}

			if rl.IsKeyPressed(rl.KeyJ) && !game.Player.ShieldActive && game.Player.ShieldCooldownTimer <= 0 {
				game.Player.ShieldActive = true
				game.Player.ShieldTimer = 3.0
			}

			if rl.IsKeyPressed(rl.KeySpace) {
				bossCenter := rl.NewVector2(game.Boss.Position.X+game.Boss.Size.X/2, game.Boss.Position.Y+game.Boss.Size.Y/2)
				bossDirection := rl.Vector2Subtract(bossCenter, game.Player.Position)

				normalizedDirection := rl.Vector2Normalize(bossDirection)

				bulletVelocity := rl.Vector2Scale(normalizedDirection, float32(projectileSpeed))

				newBullet := Bullet{
					Position: game.Player.Position,
					Velocity: bulletVelocity,
					Radius:   float32(projectileSize),
					Color:    rl.Violet,
					Active:   true,
				}

				game.Bullets = append(game.Bullets, newBullet)
			}

			switch game.Boss.State {
			case IDLE:
				game.Boss.AttackTimer -= deltaTime

				if game.Boss.AttackTimer <= 0 && game.Boss.Health > 0 {
					game.Boss.State = WINDING_UP
					game.Boss.StateTimer = 1.0
					game.Boss.Color = rl.Purple
				}
			case WINDING_UP:
				game.Boss.StateTimer -= deltaTime
				game.Boss.TargetPosition = game.Player.Position

				if game.Boss.StateTimer <= 0 {
					game.Boss.State = SHOOTING
					game.Boss.StateTimer = 2.0
					bossCenter := rl.NewVector2(game.Boss.Position.X+game.Boss.Size.X/2, game.Boss.Position.Y+game.Boss.Size.Y/2)
					direction := rl.Vector2Subtract(game.Boss.TargetPosition, bossCenter)

					game.Boss.BeamRotation = float32(math.Atan2(float64(direction.Y), float64(direction.X))) * rl.Rad2deg
				}
			case SHOOTING:
				game.Boss.StateTimer -= deltaTime

				if game.Boss.StateTimer <= 0 {
					game.Boss.State = IDLE
					game.Boss.AttackTimer = 10.0
					game.Boss.Color = rl.Blue
				}
			}
		}

		for i := range game.Bullets {
			if game.Bullets[i].Active {
				game.Bullets[i].Position = rl.Vector2Add(game.Bullets[i].Position, game.Bullets[i].Velocity)

				if game.Boss.Health > 0 && rl.CheckCollisionCircleRec(game.Bullets[i].Position, game.Bullets[i].Radius, bossRect) {
					game.Bullets[i].Active = false
					game.Boss.Health -= 5
				}

				if game.Bullets[i].Position.X < 0 || game.Bullets[i].Position.X > float32(game.ScreenWidth) || game.Bullets[i].Position.Y < 0 || game.Bullets[i].Position.Y > float32(game.ScreenHeight) {
					game.Bullets[i].Active = false
				}
			}
		}

		if !gameOver && (game.Boss.Health <= 0 || game.Player.Health <= 0) {
			gameOver = true

			if game.Boss.Health < 0 {
				game.Boss.Health = 0
				game.Boss.Color = rl.Gray
			}
		}

		activeBullets := []Bullet{}

		for _, b := range game.Bullets {
			if b.Active {
				activeBullets = append(activeBullets, b)
			}
		}

		game.Bullets = activeBullets

		rl.BeginDrawing()
		rl.ClearBackground(rl.RayWhite)

		for i := range int(game.Player.Health) {
			posX := float32(game.ScreenWidth - int32(30) - (int32(i) * int32(25)))
			v1 := rl.NewVector2(posX, 10)
			v2 := rl.NewVector2(posX-10, 30)
			v3 := rl.NewVector2(posX+10, 30)

			rl.DrawTriangle(v1, v2, v3, rl.Red)
		}

		if game.Boss.Health > 0 {
			rl.DrawRectangleV(game.Boss.Position, game.Boss.Size, game.Boss.Color)
			healthPercentage := float32(game.Boss.Health) / float32(game.Boss.MaxHealth)
			healthBarWidth := float32(game.Boss.Size.X) * healthPercentage
			rl.DrawRectangle(int32(game.Boss.Position.X), int32(game.Boss.Position.Y)-20, int32(game.Boss.Size.X), 15, rl.Fade(rl.Green, 0.3))
			rl.DrawRectangle(int32(game.Boss.Position.X), int32(game.Boss.Position.Y)-20, int32(healthBarWidth), 15, rl.Green)
			rl.DrawRectangleLines(int32(game.Boss.Position.X), int32(game.Boss.Position.Y)-20, int32(game.Boss.Size.X), 15, rl.DarkGray)
		} else {
			rl.DrawRectangleV(game.Boss.Position, game.Boss.Size, rl.Gray)
		}

		if game.Boss.State == SHOOTING {
			bossCenter := rl.NewVector2(game.Boss.Position.X+game.Boss.Size.X/2, game.Boss.Position.Y+game.Boss.Size.Y/2)
			beamRec := rl.Rectangle{X: bossCenter.X, Y: bossCenter.Y, Width: float32(game.ScreenWidth) * 1.5, Height: 20}
			beamOrigin := rl.NewVector2(0, beamRec.Height/2)

			rl.DrawRectanglePro(beamRec, beamOrigin, game.Boss.BeamRotation, rl.Fade(rl.Yellow, 0.7))

			beamStart := bossCenter
			direction := rl.Vector2Normalize(rl.Vector2Subtract(game.Boss.TargetPosition, bossCenter))
			beamEnd := rl.Vector2Add(beamStart, rl.Vector2Scale(direction, float32(game.ScreenWidth)+1.5))

			if game.Player.Health > 0 && !game.Player.ShieldActive && game.Player.ImmunityTimer <= 0 && CheckCollisionCircleLine(game.Player.Position, game.Player.Radius, beamStart, beamEnd) {
				game.Player.Health--
				game.Player.ImmunityTimer = 1.0
			}
		}
		if game.Player.ImmunityTimer > 0 {
			if int(rl.GetTime()*10)%2 == 0 {
				rl.DrawCircleV(game.Player.Position, game.Player.Radius, game.Player.Color)
			}
		} else {
			rl.DrawCircleV(game.Player.Position, float32(game.Player.Radius), game.Player.Color)
		}

		if game.Player.ShieldActive {
			shieldRadius := game.Player.Radius + 5
			rl.DrawCircleV(game.Player.Position, shieldRadius, rl.Fade(rl.SkyBlue, 0.5))
			rl.DrawCircleLines(int32(game.Player.Position.X), int32(game.Player.Position.Y), shieldRadius, rl.SkyBlue)
		}

		for _, b := range game.Bullets {
			rl.DrawCircleV(b.Position, b.Radius, b.Color)
		}

		rl.DrawText(fmt.Sprintf("Boss Health: %.0f%%", (float32(game.Boss.Health)/float32(game.Boss.MaxHealth))*100), 10, 10, 20, rl.Black)
		rl.DrawText("Move: WASD | Shoot: Space | Shield: J", 10, 40, 20, rl.DarkGray)

		if gameOver {
			var message string
			if game.Player.Health <= 0 {
				message = "YOU WERE DEFEATED!"
			} else {
				message = "BOSS DEFEATED!"
			}
			rl.DrawText(message, game.ScreenWidth/2-rl.MeasureText(message, 40)/2, game.ScreenHeight/2-20, 40, rl.Gold)
		}

		rl.EndDrawing()
	}
}
