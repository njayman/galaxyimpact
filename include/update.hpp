#pragma once

#include "entities/boss.hpp"
#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "game.hpp"
#include "raylib.h"
#include <optional>
#include <utility>
#include <vector>

namespace UpdateConstants
{
constexpr float nerveMax = 100;
constexpr float chargeRegenTime = 3.0;
constexpr int32_t maxCharges = 2;
constexpr float slamDuration = 1.6F;
constexpr float bossDeathShockwaveDuration = 3.0F;
constexpr float shockFlashDuration = 0.4F;
constexpr float waveEnemyScalePerWave = 0.035F;
constexpr float pickupExpiryWarning = 2.5F;
constexpr float shockwaveStompRadius = 220;
constexpr float shockwaveStompDuration = 0.5F;
constexpr float shieldCooldownDuration = 2.0F;
constexpr int maxEnemies = 200;
constexpr float enemyChargeDashDuration = 0.4F;
constexpr float enemyChargeDashSpeedMult = 6.0F;
constexpr float frameScale = 60.0F;
constexpr float hitFlashDuration = 0.12F;
constexpr float nerveBurstWindup = 0.35F;
}

auto nerveFrac(const Game& game) -> float;
auto isNerveChargeFeeding(const Game& game) -> bool;
auto chargeRegenDuration(const Game& game) -> float;
auto aimAtMouse(const Game& game) -> Vector2;
auto mouseWorldPos(const Game& game) -> Vector2;
auto orbitRadius(int32_t level) -> float;
auto orbitBladeCount(int32_t level) -> int32_t;
auto orbitBladePosition(const Game& game, Vector2 center, float radius, int32_t index,
                        int32_t count) -> Vector2;
auto shockwaveRadius(int32_t level, bool evolved) -> float;
auto beamAimDirection(const Game& game, bool evolved) -> Vector2;
auto beamLength(const Game& game, int32_t level, bool evolved) -> float;
auto waveEnemyScale(const Game& game) -> float;
auto isFusedPassive(const Game& game, SkillType id) -> bool;
auto hasWeapon(const Game& game, WeaponType kind) -> bool;
auto weaponForGrantSkill(SkillType id) -> std::optional<WeaponType>;
auto weaponEvolved(const Game& game, WeaponType kind) -> bool;
auto bossWindupDuration(BossAttack attack) -> float;
auto wormholeBentBeamSegments(const Game& game, Vector2 start,
                              Vector2 end) -> std::vector<std::pair<Vector2, Vector2>>;

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
void spawnPickup(Game& game, Vector2 position, int value, PickupType type,
                 ElementType element = ElementType::Static,
                 ElementMechanism mechanism = ElementMechanism::Infusion);

auto isWeaponTypeUnlocked(const Game& game, WeaponType weapon) -> bool;
void recordWeaponKill(Game& game, WeaponType weapon);
void recordSkillMaxed(Game& game, SkillType id);
void recordDashKill(Game& game, int32_t enemyKindIndex);
void recordDashOrNerveKill(Game& game);
void recordWaveReached(Game& game, int32_t wave);
