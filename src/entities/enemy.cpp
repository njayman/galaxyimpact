#include "update.hpp"

#include "entities/ship.hpp"
#include "raymath.h"
#include "update_constants.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
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

auto isDasher(const EnemyKind& kind) -> bool
{
    return kind.pattern == EnemyPattern::Charge || kind.rollCharge;
}

auto activeDasherCount(const Game& game) -> int
{
    return static_cast<int>(std::ranges::count_if(
        game.run.enemies, [](const Enemy& enemy)
        { return enemy.active && isDasher(enemyKinds.at(static_cast<size_t>(enemy.kind))); }));
}

auto activeRangedEnemyCount(const Game& game) -> int
{
    int count = 0;
    for (const auto& enemy : game.run.enemies)
    {
        if (enemy.active && enemyKinds.at(static_cast<size_t>(enemy.kind)).pattern == EnemyPattern::Turret)
        {
            count++;
        }
    }
    return count;
}

void spawnEnemy(Game& game)
{

    constexpr int biomeExclusiveWeight = 4;
    const Biome biome = currentBiome(game.run.waveNumber);
    const bool rangedSlotAvailable = activeRangedEnemyCount(game) < maxActiveRangedEnemies;
    const bool dasherSlotAvailable =
        game.run.dasherSpawnCooldown <= 0 && activeDasherCount(game) < maxActiveDashers;

    std::vector<int> eligible;
    eligible.reserve(enemyKinds.size() * static_cast<size_t>(biomeExclusiveWeight));
    for (size_t i = 0; i < enemyKinds.size(); i++)
    {
        if ((enemyKinds.at(i).pattern == EnemyPattern::Turret && !rangedSlotAvailable) ||
            (isDasher(enemyKinds.at(i)) && !dasherSlotAvailable) || i == enemyKindRicochetStrafer)
        {
            continue; // Ricochet Strafers come from updateRicochetRelay
        }
        const auto& candidate = enemyKinds.at(i);
        if (candidate.minWave > game.run.waveNumber)
        {
            continue;
        }
        if (!candidate.biomeExclusive)
        {
            eligible.push_back(static_cast<int>(i));
            continue;
        }
        if (candidate.biome == biome)
        {
            for (int w = 0; w < biomeExclusiveWeight; w++)
            {
                eligible.push_back(static_cast<int>(i));
            }
        }
        else if (biome == Biome::Punctum)
        {
            eligible.push_back(static_cast<int>(i));
        }
    }
    if (eligible.empty())
    {
        return;
    }

    const int kindIndex =
        eligible.at(static_cast<size_t>(GetRandomValue(0, static_cast<int>(eligible.size()) - 1)));
    if (isDasher(enemyKinds.at(static_cast<size_t>(kindIndex))))
    {
        game.run.dasherSpawnCooldown = dasherSpawnCooldown;
    }
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
                                       enemyHealthScale(game));
    if (currentBiome(game.run.waveNumber) == Biome::Punctum)
    {
        health = static_cast<int32_t>(static_cast<float>(health) * punctumEnemyHealthMult);
    }
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
                                         static_cast<float>(GetRandomValue(0, 6283)) / 1000.0F,
                                     .id = ++game.run.nextEnemyId});
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
    if (bossQteActive(game))
    {
        amount = static_cast<int32_t>(std::ceil(static_cast<float>(amount) * UpdateConstants::qteDamageMult));
    }
    const int32_t healthBefore = game.run.enemies.at(index).health;
    game.run.enemies.at(index).health -= amount;
    game.run.enemies.at(index).hitFlashTimer = UpdateConstants::hitFlashDuration;
    applyActiveElementalDebuffs(game, game.run.enemies.at(index));

    if (kind.shape == EnemyShape::Organic)
    {
        spawnOrganicHitSplash(game, game.run.enemies.at(index).position, kind.color);
    }

    spawnDamageNumber(game, game.run.enemies.at(index).position, std::min(amount, healthBefore));

    if (game.run.enemies.at(index).health > 0)
    {
        return;
    }

    game.run.enemies.at(index).active = false;
    tryComboRefund(game);
    recordEnemyKilled(game, game.run.enemies.at(index).kind);
    bloomRustPatch(game, game.run.enemies.at(index).position, false);

    if (game.run.enemies.at(index).kind == enemyKindHiveNode)
    {
        const Vector2 deathPos = game.run.enemies.at(index).position;
        for (int i = 0; i < hiveNodeDeathSpawnCount; i++)
        {
            const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 offset{.x = std::cos(angle) * 20, .y = std::sin(angle) * 20};
            spawnEnemyAt(game, enemyKindSporeSwarmling, Vector2Add(deathPos, offset));
        }
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

// A lit fuse runs out: the blast hurts the player inside it; no score or loot.
void detonateFuseEnemy(Game& game, size_t index)
{
    Enemy& enemy = game.run.enemies.at(index);
    const auto& kind = enemyKinds.at(static_cast<size_t>(enemy.kind));
    if (Vector2Distance(game.run.player.position, enemy.position) <=
        kind.explodeRadius + game.run.player.radius)
    {
        damagePlayer(game, enemyDamage(game, kind.explodeDamage));
    }
    spawnKillExplosion(game, enemy.position, kind.color, 22, 1.4F);
    playSFX(game, game.resources.sounds.rockBreak);
    triggerShake(game, 4, 0.2F);
    enemy.active = false;
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

// --- Sniper shot QTE (cutscene) ---------------------------------------------
//
// A Sniper lining up its shot starts a short QTE: one prompt against the line.
// Answered, the ship slips the line, turns on the Sniper, charges and shoots it dead.
// Wrong or late, the line goes live after a short beat, still dodgeable.
// --------------------------------------------------------------------------

void sniperQteBegin(Game& game, int32_t enemyId, Vector2 aim)
{
    bossQteBegin(game, -1, QteKind::Rush); // not a Boss QTE: sets no gap (see bossQteEnd)
    game.run.qteEnemyId = enemyId;
    game.run.qteThreatDir = aim;
    game.run.qtePrompt = qtePromptForThreat(game, aim, sniperQteParryChance);
    game.run.qteTimer = sniperQtePromptWindow;
    game.run.qteTimerMax = sniperQtePromptWindow;
    playSFX(game, game.resources.sounds.bossWindUp);
}

void updateSniperQte(Game& game, float deltaTime)
{
    auto& run = game.run;
    const auto sniper = std::ranges::find_if(run.enemies, [&](const Enemy& enemy)
                                             { return enemy.active && static_cast<int32_t>(enemy.id) == run.qteEnemyId; });
    if (sniper == run.enemies.end())
    {
        bossQteEnd(game);
        return;
    }
    if (run.qteFinisherPhase > 0)
    {
        if (qteUpdateFinisher(game, sniper->position, deltaTime))
        {
            spawnKillExplosion(game, sniper->position, enemyKinds.at(static_cast<size_t>(sniper->kind)).color, 30,
                               1.6F);
            playSFX(game, game.resources.sounds.explosion);
            damageEnemy(game, static_cast<size_t>(std::distance(run.enemies.begin(), sniper)), sniper->health);
            bossQteEnd(game);
        }
        return;
    }

    const int32_t answer = qteReadInput(game, run.qtePrompt);
    if (answer > 0)
    {
        qteStartAction(game, run.qtePrompt, run.qteThreatDir);
        // The shot never fires: its tell fades, and the counter-shot flashes back.
        for (auto& shot : run.sniperShots)
        {
            if (Vector2Equals(shot.from, run.sniperEventStart) != 0)
            {
                shot.timer = 0;
            }
        }
        run.sniperEventPhase = 0;
        run.sniperEventEnemyId = -1;
        run.sniperEventCooldown = sniperQteGap;
        sniper->sniperPhase = 0;
        sniper->sniperLineHitPlayer = false;
        sniper->stateTimer = std::max(enemyKinds.at(static_cast<size_t>(sniper->kind)).fireInterval, sniperQteGap);
        playSFX(game, game.resources.sounds.critical);
        run.qtePrompt = QtePrompt::None;
        run.qteTimerMax = 0;
        qteBeginFinisher(game);
    }
    else if (answer < 0 || run.qteTimer <= 0)
    {
        bossQteEnd(game);
        run.sniperEventTimer = std::min(run.sniperEventTimer, sniperQteFailTell);
    }
}

// --- Ricochet Strafers --------------------------------------------------------
//
// Strafers spawn one at a time on a ring round the player, up to ricochetStraferMax.
// While two or more live, one bullet relays between them across the screen, each
// arrival sending it on to another strafer, until they are down to one. It passes
// through enemies; hitting the player or being lost relaunches it after a beat.
// --------------------------------------------------------------------------

void updateRicochetRelay(Game& game, float deltaTime)
{
    auto& run = game.run;
    const auto& kind = enemyKinds.at(enemyKindRicochetStrafer);
    std::vector<size_t> strafers;
    for (size_t i = 0; i < run.enemies.size(); i++)
    {
        if (run.enemies.at(i).active && run.enemies.at(i).kind == enemyKindRicochetStrafer)
        {
            strafers.push_back(i);
        }
    }

    if ((!game.sandbox || game.sandboxNaturalSpawnEnabled) && currentBiome(run.waveNumber) == kind.biome &&
        run.waveNumber >= kind.minWave &&
        std::ssize(strafers) < ricochetStraferMax(run.waveNumber))
    {
        run.ricochetSpawnTimer -= deltaTime;
        if (run.ricochetSpawnTimer <= 0)
        {
            run.ricochetSpawnTimer = ricochetStraferSpawnInterval;
            const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const auto dist = static_cast<float>(GetRandomValue(static_cast<int32_t>(ricochetStraferSpawnDistMin),
                                                                static_cast<int32_t>(ricochetStraferSpawnDistMax)));
            spawnEnemyAt(game, enemyKindRicochetStrafer,
                         Vector2Add(run.player.position,
                                    Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist}));
            return; // spawning may have moved the enemy list
        }
    }

    const auto pickOther = [&](uint32_t exceptId) -> const Enemy*
    {
        std::vector<const Enemy*> others;
        for (const size_t i : strafers)
        {
            if (run.enemies.at(i).id != exceptId)
            {
                others.push_back(&run.enemies.at(i));
            }
        }
        return others.empty() ? nullptr
                              : others.at(static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(others.size()) - 1)));
    };
    const auto aim = [&](BossProjectile& bullet, const Enemy& target)
    {
        bullet.relayTargetId = target.id;
        bullet.velocity =
            Vector2Scale(Vector2Normalize(Vector2Subtract(target.position, bullet.position)), ricochetRelaySpeed);
    };

    const auto relay = std::ranges::find_if(run.bossProjectiles, [](const BossProjectile& p)
                                            { return p.active && p.relayTargetId != 0; });
    if (relay == run.bossProjectiles.end())
    {
        run.ricochetRelayDelay -= deltaTime;
        if (strafers.size() < 2 || run.ricochetRelayDelay > 0)
        {
            return;
        }
        run.ricochetRelayDelay = ricochetRelayRestart;
        const Enemy& from = run.enemies.at(strafers.at(static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(strafers.size()) - 1))));
        const Enemy* to = pickOther(from.id);
        BossProjectile bullet{.position = from.position,
                              .velocity = Vector2{},
                              .radius = 7,
                              .homing = false,
                              .active = true,
                              .fromPlayer = false,
                              .damage = crossfireProjectileDamage,
                              .health = 1};
        aim(bullet, *to);
        bullet.position = Vector2Add(from.position, Vector2Scale(Vector2Normalize(bullet.velocity), kind.radius + 8.0F));
        run.bossProjectiles.push_back(bullet);
        playSFX(game, game.resources.sounds.spreadBurst);
        return;
    }

    const auto target = std::ranges::find_if(strafers, [&](size_t i) { return run.enemies.at(i).id == relay->relayTargetId; });
    if (target == strafers.end())
    {
        // Its target died: on to another, or the relay ends with fewer than one left to reach.
        if (const Enemy* next = pickOther(0); next != nullptr && strafers.size() >= 2)
        {
            aim(*relay, *next);
        }
        else
        {
            relay->active = false;
        }
        return;
    }
    const Enemy& at = run.enemies.at(*target);
    if (Vector2Distance(relay->position, at.position) > kind.radius)
    {
        aim(*relay, at); // keep homing on the (static) target
        return;
    }
    if (const Enemy* next = pickOther(at.id); next != nullptr)
    {
        aim(*relay, *next);
        playSFX(game, game.resources.sounds.menuMove);
    }
    else
    {
        relay->active = false;
    }
}

void updateEnemies(Game& game, float deltaTime)
{
    game.run.dasherSpawnCooldown = std::max(0.0F, game.run.dasherSpawnCooldown - deltaTime);
    game.run.sniperEventCooldown = std::max(0.0F, game.run.sniperEventCooldown - deltaTime);
    if (game.run.sniperEventPhase != 0 &&
        std::ranges::none_of(game.run.enemies, [&](const Enemy& enemy)
                             { return enemy.active && enemy.id == static_cast<uint32_t>(game.run.sniperEventEnemyId); }))
    {
        game.run.sniperEventPhase = 0;
        game.run.sniperEventEnemyId = -1;
    }
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
        else
        {
            switch (kind.pattern)
            {
            case EnemyPattern::Chase:
            {
                if (kind.fuseRadius > 0 && enemy.telegraphing)
                {
                    break; // fuse lit: it stops and waits to blow
                }
                // Roll charge: stateTimer counts down the chase gap, then the tell, then (while
                // charging) the distance left to roll.
                if (kind.rollCharge)
                {
                    if (enemy.charging)
                    {
                        const Vector2 step = Vector2Scale(enemy.velocity, deltaTime * frameScale);
                        enemy.position = Vector2Add(enemy.position, step);
                        enemy.stateTimer -= Vector2Length(step);
                        if (enemy.stateTimer <= 0)
                        {
                            enemy.charging = false;
                            enemy.stateTimer = rockhideRollInterval;
                        }
                        break;
                    }
                    enemy.stateTimer -= deltaTime;
                    if (enemy.telegraphing)
                    {
                        // Like the Charger: the aim tracks the player, then it rolls through them.
                        const Vector2 toPlayer = Vector2Subtract(game.run.player.position, enemy.position);
                        enemy.velocity = Vector2Scale(Vector2Normalize(toPlayer),
                                                      kind.speed * speedMod * rockhideRollSpeedMult);
                        if (enemy.stateTimer <= 0)
                        {
                            enemy.telegraphing = false;
                            enemy.charging = true;
                            enemy.stateTimer = Vector2Length(toPlayer) + UpdateConstants::enemyChargeOvershoot;
                        }
                        break;
                    }
                    if (enemy.stateTimer <= 0)
                    {
                        enemy.telegraphing = true;
                        enemy.stateTimer = rockhideRollTell;
                        enemy.velocity = Vector2Scale(
                            Vector2Normalize(Vector2Subtract(game.run.player.position, enemy.position)),
                            kind.speed * speedMod * rockhideRollSpeedMult);
                        break;
                    }
                }
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
                    // Triangle wave: hard, wide cuts side to side.
                    const float wobble =
                        std::asin(std::sin(static_cast<float>(GetTime()) * 4.0F + static_cast<float>(i))) *
                        (2.0F / std::numbers::pi_v<float>) * 4.5F;
                    const Vector2 move = Vector2Add(Vector2Scale(dir, kind.speed * speedMod),
                                                    Vector2Scale(perp, wobble));
                    enemy.position =
                        Vector2Add(enemy.position, Vector2Scale(move, deltaTime * frameScale));
                }
                break;
            }
            case EnemyPattern::Charge:
            {
                // Creep in, aim at the player like a sniper (the line tracks them), lock, then
                // charge through where they stood. While charging stateTimer is distance left.
                const Vector2 toPlayer = Vector2Subtract(game.run.player.position, enemy.position);
                const float chargeSpeed =
                    kind.speed * speedMod * UpdateConstants::enemyChargeDashSpeedMult;
                if (enemy.charging)
                {
                    const Vector2 step = Vector2Scale(enemy.velocity, deltaTime * frameScale);
                    enemy.position = Vector2Add(enemy.position, step);
                    enemy.stateTimer -= Vector2Length(step);
                    if (enemy.stateTimer <= 0)
                    {
                        enemy.charging = false;
                        enemy.stateTimer = 1.2F;
                        enemy.velocity = Vector2{};
                    }
                    break;
                }
                enemy.stateTimer -= deltaTime;
                if (enemy.telegraphing)
                {
                    enemy.velocity = Vector2Scale(Vector2Normalize(toPlayer), chargeSpeed);
                    if (enemy.stateTimer <= 0)
                    {
                        enemy.telegraphing = false;
                        enemy.charging = true;
                        enemy.stateTimer = Vector2Length(toPlayer) + UpdateConstants::enemyChargeOvershoot;
                        spawnKillExplosion(game, enemy.position, kind.color, 8, 0.6F);
                    }
                    break;
                }
                if (Vector2Length(toPlayer) > 0)
                {
                    enemy.position = Vector2Add(
                        enemy.position, Vector2Scale(Vector2Normalize(toPlayer),
                                                     kind.speed * speedMod * deltaTime * frameScale));
                }
                if (enemy.stateTimer <= 0)
                {
                    enemy.telegraphing = true;
                    enemy.stateTimer = chargeTelegraphDuration;
                    enemy.velocity = Vector2Scale(Vector2Normalize(toPlayer), chargeSpeed);
                }
                break;
            }
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
                // Every turret-type enemy fires the shot QTE (see sniperQteBegin).
                if (game.run.sniperEventEnemyId != static_cast<int32_t>(enemy.id))
                {
                    enemy.stateTimer -= deltaTime;
                    if (enemy.stateTimer <= 0 && game.run.sniperEventPhase == 0 &&
                        game.run.sniperEventCooldown <= 0 && qteSlotFree(game) &&
                        Vector2Distance(enemy.position, game.run.player.position) < turretFireRange)
                    {
                        const Vector2 predictedPlayer =
                            Vector2Add(game.run.player.position,
                                       Vector2Scale(game.run.player.velocity, 12.0F));
                        const Vector2 aim =
                            Vector2Normalize(Vector2Subtract(predictedPlayer, enemy.position));
                        game.run.sniperEventStart = enemy.position;
                        game.run.sniperEventEnd =
                            Vector2Add(enemy.position, Vector2Scale(aim, sniperLineLength));
                        game.run.sniperEventPlayerStart = game.run.player.position;
                        game.run.sniperEventEnemyId = static_cast<int32_t>(enemy.id);
                        game.run.sniperEventPhase = 1;
                        game.run.sniperEventTimer = sniperDodgeTellDuration;
                        game.run.sniperEventStart = enemy.position;
                        game.run.sniperEventEnd = Vector2Add(
                            enemy.position, Vector2Scale(aim, sniperLineLength));
                        game.run.sniperShots.push_back(ChainLightningBolt{
                            .from = game.run.sniperEventStart,
                            .to = game.run.sniperEventEnd,
                            .timer = sniperDodgeTellDuration + sniperLiveLineDuration});
                        enemy.sniperLineHitPlayer = false;
                        enemy.sniperPhase = 1;
                        sniperQteBegin(game, static_cast<int32_t>(enemy.id), aim);
                    }
                    break;
                }

                game.run.sniperEventTimer -= deltaTime;
                enemy.sniperPhase = game.run.sniperEventPhase;
                if (game.run.sniperEventPhase == 1 && game.run.sniperEventTimer <= 0)
                {
                    game.run.sniperEventPhase = 2;
                    game.run.sniperEventTimer = sniperLiveLineDuration;
                    enemy.sniperPhase = 2;
                    if (!enemy.sniperLineHitPlayer && game.run.player.health > 0 &&
                        CheckCollisionCircleLine(game.run.player.position,
                                                 game.run.player.radius + sniperLineWidth / 2,
                                                 game.run.sniperEventStart,
                                                 game.run.sniperEventEnd))
                    {
                        enemy.sniperLineHitPlayer = true;
                        damagePlayer(game, enemyDamage(game, sniperLineDamage));
                    }
                }
                else if (game.run.sniperEventPhase == 2 && !enemy.sniperLineHitPlayer &&
                         game.run.player.health > 0 && game.run.player.immunityTimer <= 0 &&
                         CheckCollisionCircleLine(game.run.player.position,
                                                  game.run.player.radius + sniperLineWidth / 2,
                                                  game.run.sniperEventStart,
                                                  game.run.sniperEventEnd))
                {
                    enemy.sniperLineHitPlayer = true;
                    damagePlayer(game, enemyDamage(game, sniperLineDamage));
                }
                else if (game.run.sniperEventPhase == 2 && game.run.sniperEventTimer <= 0)
                {
                    game.run.sniperEventPhase = 0;
                    game.run.sniperEventEnemyId = -1;
                    game.run.sniperEventCooldown = std::max(kind.fireInterval, sniperQteGap);
                    enemy.sniperPhase = 0;
                    enemy.sniperLineHitPlayer = false;
                    enemy.stateTimer = kind.fireInterval;
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

        if (kind.fuseRadius > 0)
        {
            if (!enemy.telegraphing)
            {
                if (Vector2Distance(enemy.position, game.run.player.position) < kind.fuseRadius)
                {
                    enemy.telegraphing = true;
                    enemy.stateTimer = kind.fuseTime;
                    playSFX(game, game.resources.sounds.bossWindUp);
                }
            }
            else
            {
                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0)
                {
                    detonateFuseEnemy(game, i);
                    continue;
                }
            }
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

            if (game.run.player.shieldActive)
            {
                damageEnemy(game, i, 999);
                recordDamage(game, DamageSource::Dash, 999);
            }
            else
            {
                const auto dmg =
                    static_cast<int32_t>(static_cast<float>(dashDamage) * currentShip(game).damageMult);
                damageEnemy(game, i, dmg);
                recordDamage(game, DamageSource::Dash, dmg);
                damagePlayer(game, enemyDamage(game, kind.contactDamage));
            }

            if (!enemy.active)
            {
                game.run.player.chargeRegenTimer =
                    std::max(0.0F, game.run.player.chargeRegenTimer - dashKillChargeRefund);
                recordDashKill(game, enemy.kind);
                recordDashOrNerveKill(game);
                if (game.run.player.shieldDashing)
                {
                    game.run.player.shieldDashKill = true;
                }
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
            }
        }
    }
}
