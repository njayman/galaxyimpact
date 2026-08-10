#include "update.hpp"

#include "entities/ship.hpp"
#include "raymath.h"
#include "update_constants.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

auto waveEnemyScale(const Game& game) -> float
{
    return 1 +
           static_cast<float>(game.run.waveNumber - 1) * UpdateConstants::waveEnemyScalePerWave +
           static_cast<float>(game.run.level - 1) * UpdateConstants::waveEnemyScalePerLevel;
}

auto enemyDamage(const Game& game, int32_t base) -> int32_t
{
    return static_cast<int32_t>(static_cast<float>(base) * enemyDamageMult * waveEnemyScale(game));
}

void spawnEnemy(Game& game)
{

    constexpr int biomeExclusiveWeight = 4;

    std::vector<int> eligible;
    eligible.reserve(enemyKinds.size() * static_cast<size_t>(biomeExclusiveWeight));
    for (size_t i = 0; i < enemyKinds.size(); i++)
    {
        const auto& candidate = enemyKinds.at(i);
        const bool biomeOk =
            !candidate.biomeExclusive || candidate.biome == currentBiome(game.run.waveNumber);
        if (candidate.minWave <= game.run.waveNumber && biomeOk)
        {
            const int weight = candidate.biomeExclusive ? biomeExclusiveWeight : 1;
            for (int w = 0; w < weight; w++)
            {
                eligible.push_back(static_cast<int>(i));
            }
        }
    }
    if (eligible.empty())
    {
        return;
    }

    const int kindIndex =
        eligible.at(static_cast<size_t>(GetRandomValue(0, static_cast<int>(eligible.size()) - 1)));
    spawnEnemyAt(game, kindIndex, spawnRingPosition(game));
}

auto spawnRingPosition(const Game& game) -> Vector2
{
    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(420, 520));
    return Vector2Add(game.run.player.position,
                      Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
}

void spawnEnemyAt(Game& game, int kindIndex, Vector2 pos)
{
    if (static_cast<int>(game.run.enemies.size()) >= UpdateConstants::maxEnemies)
    {
        return;
    }

    const auto& kind = enemyKinds.at(static_cast<size_t>(kindIndex));
    const bool elite = GetRandomValue(0, 999) < static_cast<int32_t>(eliteChance * 1000);

    auto health = static_cast<int32_t>(static_cast<float>(kind.health) * enemyHealthMult *
                                       waveEnemyScale(game));
    if (elite)
    {
        health *= 2;
    }

    game.run.enemies.push_back(Enemy{.kind = kindIndex,
                                     .position = pos,
                                     .velocity = Vector2{},
                                     .health = health,
                                     .active = true,
                                     .stateTimer = kind.fireInterval + kind.spawnInterval + 1,
                                     .charging = false,
                                     .telegraphing = false,
                                     .phased = false,
                                     .orbitAngle = 0,
                                     .orbitDist = 200,
                                     .orbitCenterCurrent = pos,
                                     .isElite = elite,
                                     .hitByDash = false,
                                     .hitFlashTimer = 0,
                                     .organicSeed =
                                         static_cast<float>(GetRandomValue(0, 6283)) / 1000.0F});
}

auto organicVertexRadius(float baseRadius, int32_t vertexIndex, float seed) -> float
{
    const float t = static_cast<float>(GetTime()) * organicWobbleSpeed + seed;
    const float phase = static_cast<float>(vertexIndex) * 2.399F;
    return baseRadius * (1.0F + organicWobbleAmplitude * std::sin(t + phase));
}

auto enemyCollisionRadius(const Enemy& enemy) -> float
{
    const auto& kind = enemyKinds.at(static_cast<size_t>(enemy.kind));
    if (kind.shape != EnemyShape::Organic)
    {
        return kind.radius;
    }
    const float baseRadius = kind.radius * enemy.organicRadiusMult;
    float sum = 0;
    for (int32_t i = 0; i < organicVertexCount; i++)
    {
        sum += organicVertexRadius(baseRadius, i, enemy.organicSeed);
    }
    return sum / static_cast<float>(organicVertexCount);
}

void updateOrganicMerges(Game& game, float deltaTime)
{
    game.run.organicMergeTimer -= deltaTime;
    if (game.run.organicMergeTimer > 0)
    {
        return;
    }
    game.run.organicMergeTimer = organicMergeCheckInterval;

    std::vector<size_t> candidates;
    for (size_t i = 0; i < game.run.enemies.size(); i++)
    {
        const auto& e = game.run.enemies.at(i);
        if (e.active && !e.phased &&
            enemyKinds.at(static_cast<size_t>(e.kind)).shape == EnemyShape::Organic &&
            e.mergeCount < organicMergeMaxCount)
        {
            candidates.push_back(i);
        }
    }
    if (candidates.size() < 2)
    {
        return;
    }

    const size_t pickIdx = candidates.at(
        static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(candidates.size()) - 1)));

    std::optional<size_t> partner;
    float bestDist = organicMergeRange;
    for (size_t j : candidates)
    {
        if (j == pickIdx)
        {
            continue;
        }
        const float dist = Vector2Distance(game.run.enemies.at(pickIdx).position,
                                           game.run.enemies.at(j).position);
        if (dist < bestDist)
        {
            bestDist = dist;
            partner = j;
        }
    }
    if (!partner.has_value() || GetRandomValue(0, 99) >= organicMergeChancePercent)
    {
        return;
    }

    Enemy& a = game.run.enemies.at(pickIdx);
    Enemy& b = game.run.enemies.at(*partner);
    Enemy& survivor = a.health >= b.health ? a : b;
    Enemy& absorbed = a.health >= b.health ? b : a;

    survivor.health += absorbed.health;
    survivor.mergeCount++;
    survivor.organicRadiusMult += organicMergeRadiusGrowth;
    absorbed.active = false;

    spawnKillExplosion(game, absorbed.position,
                       enemyKinds.at(static_cast<size_t>(absorbed.kind)).color, 10, 1.1F);
}

void applyElementDebuff(Enemy& enemy, ElementType element, float burnDps)
{
    switch (element)
    {
    case ElementType::Static:
        enemy.debuffStatic = true;
        break;
    case ElementType::Freeze:
        enemy.debuffFreeze = true;
        break;
    case ElementType::Confuse:
        enemy.debuffConfuse = true;
        break;
    case ElementType::Burn:
        if (enemy.burnDps <= 0 && !enemyKinds.at(static_cast<size_t>(enemy.kind)).burnImmune)
        {
            enemy.burnDps = burnDps;
        }
        break;
    case ElementType::Count:
        break;
    }
}

void applyActiveElementalDebuffs(Game& game, Enemy& enemy)
{
    const float burnDps = currentBurnDps(game);
    for (size_t e = 0; e < static_cast<size_t>(ElementType::Count); e++)
    {
        if (game.run.player.elementalBuffTimer.at(e) > 0)
        {
            applyElementDebuff(enemy, static_cast<ElementType>(e), burnDps);
        }
    }
}

void spawnOrganicHitSplash(Game& game, Vector2 position, Color color)
{

    constexpr int32_t splashCount = 6;
    for (int32_t i = 0; i < splashCount; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const float speed = static_cast<float>(GetRandomValue(15, 45)) / 10.0F;
        const float life = static_cast<float>(GetRandomValue(15, 30)) / 100.0F;
        game.run.deathParticles.push_back(
            Particle{.position = position,
                     .velocity = Vector2{.x = std::cos(angle) * speed, .y = std::sin(angle) * speed},
                     .radius = static_cast<float>(GetRandomValue(2, 4)),
                     .life = life,
                     .maxLife = life,
                     .color = color});
    }
}

void damageEnemy(Game& game, size_t index, int32_t amount)
{
    const auto kind = enemyKinds.at(static_cast<size_t>(game.run.enemies.at(index).kind));
    const int32_t healthBefore = game.run.enemies.at(index).health;
    game.run.enemies.at(index).health -= amount;
    game.run.enemies.at(index).hitFlashTimer = UpdateConstants::hitFlashDuration;
    applyActiveElementalDebuffs(game, game.run.enemies.at(index));

    if (kind.shape == EnemyShape::Organic)
    {
        spawnOrganicHitSplash(game, game.run.enemies.at(index).position, kind.color);
        playSFX(game, game.resources.sounds.squishHit);
    }

    spawnDamageNumber(game, game.run.enemies.at(index).position, std::min(amount, healthBefore));

    if (game.run.enemies.at(index).health > 0)
    {
        return;
    }

    game.run.enemies.at(index).active = false;

    if (game.run.enemies.at(index).kind == enemyKindHiveNode)
    {
        game.run.asteroids.push_back(Asteroid{.position = game.run.enemies.at(index).position,
                                              .velocity = Vector2{},
                                              .radius = asteroidRadius(AsteroidTier::Medium),
                                              .tier = AsteroidTier::Medium,
                                              .active = true});
    }

    int32_t score = kind.score;
    if (game.run.enemies.at(index).isElite)
    {
        score *= 2;
        triggerHitPause(game, UpdateConstants::hitFlashDuration);
        spawnKillExplosion(game, game.run.enemies.at(index).position, kind.color, 14, 1.4F);
    }
    else
    {
        spawnKillExplosion(game, game.run.enemies.at(index).position, kind.color, 8, 1.0F);
    }
    game.run.score += score;
    playSFX(game, game.resources.sounds.explosion);
    gainNerve(game);

    spawnPickup(game, game.run.enemies.at(index).position, score, PickupType::XP);

    const Vector2 bonusPos =
        Vector2Add(game.run.enemies.at(index).position, Vector2{.x = 8, .y = 8});
    const int32_t roll = GetRandomValue(0, 999);
    if (roll < shieldDropChance)
    {
        spawnPickup(game, bonusPos, 0, PickupType::Shield);
    }
    else if (roll < shieldDropChance + lifeOrbDropChance)
    {
        spawnPickup(game, bonusPos, 0, PickupType::LifeOrb);
    }
    spawnRareBonusPickup(game, bonusPos);
    spawnRareDangerPickup(game, bonusPos);

    if (kind.splitsOnDeath)
    {
        for (int i = 0; i < kind.splitCount; i++)
        {
            const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 offset{.x = std::cos(angle) * 10, .y = std::sin(angle) * 10};
            spawnEnemyAt(game, kind.splitKind,
                         Vector2Add(game.run.enemies.at(index).position, offset));
        }
    }

    if (kind.explodesOnDeath &&
        Vector2Distance(game.run.player.position, game.run.enemies.at(index).position) <=
            kind.explodeRadius + game.run.player.radius)
    {
        damagePlayer(game, enemyDamage(game, kind.explodeDamage));
    }
}

void killEnemyForBossAttack(Game& game, size_t index, bool alwaysLoot)
{
    if (alwaysLoot)
    {
        damageEnemy(game, index, 999);
    }
    else
    {
        game.run.enemies.at(index).active = false;
    }
}

void updateEnemies(Game& game, float deltaTime)
{

    game.run.enemies.reserve(static_cast<size_t>(UpdateConstants::maxEnemies));

    for (size_t i = 0; i < game.run.enemies.size(); i++)
    {
        auto& enemy = game.run.enemies.at(i);
        if (!enemy.active)
        {
            continue;
        }

        const auto& kind = enemyKinds.at(static_cast<size_t>(enemy.kind));
        float speedMod = enemy.isElite ? 1.2F : 1.0F;

        if (enemy.hitFlashTimer > 0)
        {
            enemy.hitFlashTimer -= deltaTime;
        }

        if (enemy.burnDps > 0)
        {
            enemy.burnDamageAccum += enemy.burnDps * deltaTime;
            if (enemy.burnDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(enemy.burnDamageAccum);
                damageEnemy(game, i, tick);
                recordDamage(game, DamageSource::Elemental, tick);
                enemy.burnDamageAccum -= static_cast<float>(tick);
            }
            if (!enemy.active)
            {
                continue;
            }
        }

        for (const auto& hazard : game.run.eliteHazards)
        {
            if (hazard.active && hazard.role == EliteHazardRole::Warlord)
            {
                speedMod *= warlordSpeedBuff;
                break;
            }
        }

        if (enemy.debuffFreeze)
        {
            speedMod *= freezeSlowMult;
        }

        if (enemy.debuffStatic)
        {

        }
        else if (enemy.debuffConfuse)
        {
            enemy.confuseWanderTimer -= deltaTime;
            if (enemy.confuseWanderTimer <= 0)
            {
                const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
                enemy.confuseWanderDir = Vector2{.x = std::cos(angle), .y = std::sin(angle)};
                enemy.confuseWanderTimer = confuseWanderInterval;
            }
            if (kind.pattern != EnemyPattern::Stationary)
            {
                enemy.position = Vector2Add(
                    enemy.position, Vector2Scale(enemy.confuseWanderDir,
                                                 kind.speed * speedMod * deltaTime * frameScale));
            }

            if (kind.pattern == EnemyPattern::Turret)
            {
                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0 &&
                    Vector2Distance(game.run.player.position, enemy.position) < turretFireRange)
                {
                    enemy.stateTimer = kind.fireInterval / speedMod;

                    std::vector<Vector2> otherTargets;
                    for (const auto& other : game.run.enemies)
                    {
                        if (other.active && !other.phased && &other != &enemy)
                        {
                            otherTargets.push_back(other.position);
                        }
                    }

                    if (!otherTargets.empty())
                    {
                        const Vector2 targetPos = otherTargets.at(static_cast<size_t>(
                            GetRandomValue(0, static_cast<int32_t>(otherTargets.size() - 1))));
                        const Vector2 dir =
                            Vector2Normalize(Vector2Subtract(targetPos, enemy.position));
                        const auto turretProjectileHealth = static_cast<int32_t>(
                            std::max(1.0F, static_cast<float>(baseProjectileHealth) *
                                               enemyHealthMult * waveEnemyScale(game)));
                        game.run.bossProjectiles.push_back(
                            BossProjectile{.position = enemy.position,
                                           .velocity = Vector2Scale(dir, kind.projectileSpeed),
                                           .radius = 6,
                                           .homing = false,
                                           .active = true,
                                           .fromPlayer = false,
                                           .damage = crossfireProjectileDamage,
                                           .health = turretProjectileHealth});
                    }
                }
            }
        }
        else
        {
            switch (kind.pattern)
            {
            case EnemyPattern::Chase:
            {
                const Vector2 dir = Vector2Subtract(game.run.player.position, enemy.position);
                if (Vector2Length(dir) > 0)
                {
                    enemy.position =
                        Vector2Add(enemy.position,
                                   Vector2Scale(Vector2Normalize(dir),
                                                kind.speed * speedMod * deltaTime * frameScale));
                }
                break;
            }
            case EnemyPattern::Zigzag:
            {
                Vector2 dir = Vector2Subtract(game.run.player.position, enemy.position);
                if (Vector2Length(dir) > 0)
                {
                    dir = Vector2Normalize(dir);
                    const Vector2 perp{.x = -dir.y, .y = dir.x};
                    const float wobble =
                        std::sin(static_cast<float>(GetTime()) * 5 + static_cast<float>(i)) * 1.5F;
                    const Vector2 move = Vector2Add(Vector2Scale(dir, kind.speed * speedMod),
                                                    Vector2Scale(perp, wobble));
                    enemy.position =
                        Vector2Add(enemy.position, Vector2Scale(move, deltaTime * frameScale));
                }
                break;
            }
            case EnemyPattern::Charge:
                enemy.stateTimer -= deltaTime;
                if (enemy.telegraphing)
                {

                    if (enemy.stateTimer <= 0)
                    {
                        enemy.telegraphing = false;
                        enemy.charging = true;
                        enemy.stateTimer = enemyChargeDashDuration;
                    }
                }
                else if (!enemy.charging)
                {
                    if (enemy.stateTimer <= 0)
                    {
                        enemy.telegraphing = true;
                        enemy.stateTimer = chargeTelegraphDuration;
                        const Vector2 dir = Vector2Normalize(
                            Vector2Subtract(game.run.player.position, enemy.position));
                        enemy.velocity = Vector2Scale(
                            dir, kind.speed * speedMod * UpdateConstants::enemyChargeDashSpeedMult);
                    }
                }
                else
                {
                    enemy.position = Vector2Add(
                        enemy.position, Vector2Scale(enemy.velocity, deltaTime * frameScale));
                    if (enemy.stateTimer <= 0)
                    {
                        enemy.charging = false;
                        enemy.stateTimer = 1.2F;
                        enemy.velocity = Vector2{};
                    }
                }
                break;
            case EnemyPattern::Orbit:
            {
                enemy.orbitAngle += 1.5F * speedMod * deltaTime;

                if (enemy.orbitDist > kind.radius + 6)
                {
                    enemy.orbitDist -= 9 * speedMod * deltaTime;
                }

                Vector2 orbitCenter = game.run.player.position;
                if (kind.orbitsNearestAsteroid)
                {
                    Vector2 nearestAsteroid = enemy.orbitCenterCurrent;
                    float bestDist = -1;
                    for (const auto& asteroid : game.run.asteroids)
                    {
                        if (!asteroid.active)
                        {
                            continue;
                        }
                        const float dist =
                            Vector2Distance(enemy.orbitCenterCurrent, asteroid.position);
                        if (bestDist < 0 || dist < bestDist)
                        {
                            bestDist = dist;
                            nearestAsteroid = asteroid.position;
                        }
                    }

                    const Vector2 toTarget =
                        Vector2Subtract(nearestAsteroid, enemy.orbitCenterCurrent);
                    const float retargetDist = Vector2Length(toTarget);
                    if (retargetDist > 1.0F)
                    {
                        const float step =
                            std::min(retargetDist, orbitAsteroidRetargetSpeed * deltaTime);
                        enemy.orbitCenterCurrent = Vector2Add(
                            enemy.orbitCenterCurrent,
                            Vector2Scale(toTarget, step / retargetDist));
                    }
                    else
                    {
                        enemy.orbitCenterCurrent = nearestAsteroid;
                    }
                    orbitCenter = enemy.orbitCenterCurrent;
                }

                enemy.position = Vector2Add(
                    orbitCenter, Vector2{.x = std::cos(enemy.orbitAngle) * enemy.orbitDist,
                                         .y = std::sin(enemy.orbitAngle) * enemy.orbitDist});
                break;
            }
            case EnemyPattern::Turret:
            {
                const Vector2 toPlayer = Vector2Subtract(game.run.player.position, enemy.position);
                const float playerDist = Vector2Length(toPlayer);
                constexpr float kiteDistance = turretFireRange * 0.55F;
                if (playerDist > 1.0F)
                {
                    const Vector2 dirToPlayer = Vector2Scale(toPlayer, 1.0F / playerDist);
                    const Vector2 perp{.x = -dirToPlayer.y, .y = dirToPlayer.x};
                    const float strafe =
                        std::sin(static_cast<float>(GetTime()) * 0.8F + static_cast<float>(i)) *
                        0.6F;
                    const float radial = playerDist < kiteDistance
                                             ? -1.0F
                                             : (playerDist > turretFireRange ? 1.0F : 0.0F);
                    Vector2 move =
                        Vector2Add(Vector2Scale(dirToPlayer, radial), Vector2Scale(perp, strafe));
                    if (Vector2Length(move) > 0)
                    {
                        move = Vector2Scale(Vector2Normalize(move), kind.speed * speedMod);
                        enemy.position =
                            Vector2Add(enemy.position, Vector2Scale(move, deltaTime * frameScale));
                    }
                }

                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0 && playerDist < turretFireRange)
                {
                    enemy.stateTimer = kind.fireInterval / speedMod;
                    const Vector2 dir =
                        Vector2Normalize(Vector2Subtract(game.run.player.position, enemy.position));
                    const auto turretProjectileHealth = static_cast<int32_t>(
                        std::max(1.0F, static_cast<float>(baseProjectileHealth) * enemyHealthMult *
                                           waveEnemyScale(game)));
                    game.run.bossProjectiles.push_back(BossProjectile{
                        .position = enemy.position,
                        .velocity = Vector2Scale(dir, kind.projectileSpeed),
                        .radius = 6,
                        .homing = false,
                        .active = true,
                        .fromPlayer = false,
                        .damage = crossfireProjectileDamage,
                        .health = turretProjectileHealth,
                        .ricochetRemaining = kind.projectileBouncesOffAsteroids ? 1 : 0});
                }
                break;
            }
            case EnemyPattern::Spawner:
                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0)
                {
                    enemy.stateTimer = kind.spawnInterval / speedMod;

                    const bool spawnFromPod = kind.biomeExclusive && kind.biome == Biome::Rustbloom;
                    const Vector2 spawnOrigin =
                        spawnFromPod ? rustbloomNearestPodCenter(enemy.position) : enemy.position;
                    for (int s = 0; s < kind.spawnCount; s++)
                    {
                        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
                        const Vector2 offset{.x = std::cos(angle) * 30, .y = std::sin(angle) * 30};
                        spawnEnemyAt(game, kind.spawnKind, Vector2Add(spawnOrigin, offset));
                    }
                }
                break;
            case EnemyPattern::Stationary:
                if (kind.pulseInsteadOfSpawn)
                {
                    enemy.stateTimer -= deltaTime;
                    if (enemy.stateTimer <= 0)
                    {
                        enemy.stateTimer = kind.spawnInterval / speedMod;
                        if (Vector2Distance(enemy.position, game.run.player.position) <=
                            kind.pulseRadius)
                        {
                            damagePlayer(game, enemyDamage(game, kind.pulseDamage));
                        }
                    }
                }
                break;
            }
        }

        if (kind.pattern != EnemyPattern::Stationary)
        {
            applyWormholeTransit(game, enemy.position, enemy.velocity, kind.radius);
        }

        if (kind.phaseCycle)
        {
            enemy.stateTimer -= deltaTime;
            if (enemy.stateTimer <= 0)
            {
                enemy.phased = !enemy.phased;
                enemy.stateTimer = 1.5F;
            }
        }

        if (enemy.kind == enemyKindMeteorChunk)
        {

            if (isOutsideCameraView(game, enemy.position, cameraDespawnMargin))
            {
                enemy.position = spawnRingPosition(game);
                enemy.fadeAlpha = 0;
            }
            enemy.fadeAlpha = std::min(1.0F, enemy.fadeAlpha + deltaTime / ghostFadeDuration);
        }
        else if (Vector2Distance(enemy.position, game.run.player.position) > entityDespawnRadius)
        {
            enemy.active = false;
            continue;
        }

        if (enemy.phased)
        {
            continue;
        }

        const bool collides =
            game.run.player.health > 0 && enemy.fadeAlpha >= 1.0F &&
            CheckCollisionCircles(game.run.player.position, game.run.player.radius, enemy.position,
                                  kind.radius);

        if (collides && game.run.player.dashing && !enemy.hitByDash)
        {
            enemy.hitByDash = true;
            const DashQuirk quirk = currentShip(game).dashQuirk;

            if (game.run.player.shieldActive)
            {
                damageEnemy(game, i, 999);
                recordDamage(game, DamageSource::Dash, 999);
            }
            else
            {
                if (quirk != DashQuirk::Push)
                {
                    const auto dmg = static_cast<int32_t>(
                        static_cast<float>(dashDamage) *
                        (quirk == DashQuirk::Hybrid ? 0.5F : 1.0F) * currentShip(game).damageMult);
                    damageEnemy(game, i, dmg);
                    recordDamage(game, DamageSource::Dash, dmg);
                }
                if (quirk != DashQuirk::Damage)
                {
                    const Vector2 pushDir = Vector2Normalize(game.run.player.dashVelocity);
                    enemy.position =
                        Vector2Add(enemy.position, Vector2Scale(pushDir, dashPushDistance));
                }
                damagePlayer(game, enemyDamage(game, kind.contactDamage));
            }

            if (!enemy.active)
            {
                game.run.player.chargeRegenTimer =
                    std::max(0.0F, game.run.player.chargeRegenTimer - dashKillChargeRefund);
                recordDashKill(game, enemy.kind);
                recordDashOrNerveKill(game);
            }
            continue;
        }

        if (collides && !game.run.player.dashing && game.run.player.immunityTimer <= 0 &&
            !enemy.debuffStatic)
        {
            if (kind.isLeech)
            {
                game.run.player.slowTimer = 2.0F;
                game.run.player.immunityTimer = 1.0F;
                damageEnemy(game, i, 999);
            }
            else if (game.run.player.shieldActive)
            {
                game.run.player.shieldActive = false;
                game.run.player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
                damageEnemy(game, i, 999);

                if (!enemy.active)
                {
                    game.run.player.chargeRegenTimer =
                        std::max(0.0F, game.run.player.chargeRegenTimer - shieldKillChargeRefund);
                }
            }
            else
            {
                damagePlayer(game, enemyDamage(game, kind.contactDamage));
                if (kind.contactAppliesConfuse && confusePulseActive(kind.confuseTelegraphDuration))
                {
                    game.run.player.confusedTimer = 1.5F;
                }
            }
        }
    }
}
