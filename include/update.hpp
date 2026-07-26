#pragma once

#include "entities/boss.hpp"
#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "game.hpp"
#include "raylib.h"
#include <optional>

namespace UpdateConstants
{
constexpr float nerveMax = 100;
constexpr float chargeRegenTime = 3.0;
constexpr int32_t maxCharges = 2;
constexpr float slamDuration = 1.6F;
constexpr float bossDeathShockwaveDuration = 3.0F;
constexpr float shockFlashDuration = 0.4F;
constexpr float waveEnemyScalePerWave = 0.035F;
constexpr float pickupExpiryWarning = 2.5F; // pickups blink in their last this-many seconds
constexpr float shockwaveStompRadius = 220; // shared with draw.cpp's telegraph/impact visual
constexpr float shockwaveStompDuration = 0.5F;
constexpr float shieldCooldownDuration = 2.0F; // shared with draw.cpp's cooldown-arc visual
constexpr int maxEnemies = 200; // hard population cap; also game.cpp's reserve() target
constexpr float enemyChargeDashDuration = 0.4F;
constexpr float enemyChargeDashSpeedMult = 6.0F; // Charger's dash velocity is kind.speed * this
constexpr float frameScale = 60.0F;              // see update.cpp for the full explanation
} // namespace UpdateConstants

auto nerveFrac(const Game& game) -> float;
auto chargeRegenDuration(const Game& game) -> float;
auto aimAtMouse(const Game& game) -> Vector2;
auto orbitRadius(int32_t level) -> float;
auto shockwaveRadius(int32_t level, bool evolved) -> float;
auto waveEnemyScale(const Game& game) -> float;
auto isFusedPassive(const Game& game, SkillType id) -> bool;
auto hasWeapon(const Game& game, WeaponType kind) -> bool;
auto weaponForGrantSkill(SkillType id) -> std::optional<WeaponType>;
auto bossWindupDuration(BossAttack attack) -> float;

// UpdateGame advances the game by one frame and reports whether the player
// chose to exit.
auto UpdateGame(Game& game, float deltaTime) -> bool;

void applyBGMState(Game& game);
void startLevelUp(Game& game);
void spawnBoss(Game& game);
void spawnMiniboss(Game& game);
void spawnSwarmBoss(Game& game);
void spawnEnemyAt(Game& game, int kindIndex, Vector2 pos);
void forceBossAttack(Game& game, Boss& boss, BossAttack attack);
void spawnBlackHole(Game& game);
void spawnWormholePair(Game& game);
void spawnEliteHazard(Game& game, EliteHazardRole role);
