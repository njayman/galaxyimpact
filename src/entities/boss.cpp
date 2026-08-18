#include "update.hpp"

#include "democonfig.hpp"
#include "entities/ship.hpp"
#include "raymath.h"
#include "update_constants.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

void seedWreckwormTrail(Boss& head, Vector2 headCenter, Vector2 trailDir);

auto isGenericMovesetAttack(BossAttack attack) -> bool
{
    switch (attack)
    {
    case BossAttack::ShockwaveStomp:
    case BossAttack::PlateHurl:
    case BossAttack::BurrowCharge:
    case BossAttack::CoilClamp:
    case BossAttack::MeteorHell:
    case BossAttack::MeteorSwarm:
    case BossAttack::DashRush:
    case BossAttack::CloudBreath:
    case BossAttack::TailWrap:
        return false;
    default:
        return true;
    }
}

auto sampleBossMoveset(int count) -> std::vector<BossAttack>
{
    std::vector<BossAttack> pool;
    pool.reserve(bossAttackCount);
    for (int i = 0; i < bossAttackCount; i++)
    {
        if (const auto attack = static_cast<BossAttack>(i);
            isGenericMovesetAttack(attack) && DemoConfig::isBossAttackAllowed(attack))
        {
            pool.push_back(attack);
        }
    }

    count = std::min(count, static_cast<int>(pool.size()));
    for (int i = 0; i < count; i++)
    {
        const int32_t j = i + GetRandomValue(0, static_cast<int32_t>(pool.size()) - i - 1);
        std::swap(pool.at(static_cast<size_t>(i)), pool.at(static_cast<size_t>(j)));
    }
    pool.resize(static_cast<size_t>(count));
    return pool;
}

void spawnBossInstance(Game& game, float healthMult, float sizeMult, bool isMega, bool isSwarm,
                       bool isFinal)
{
    const int32_t tier = game.run.waveNumber / megaBossWaveInterval;

    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(1200, 1500));
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

    auto typeIndex =
        static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(bossTypes.size()) - 1));
    if constexpr (DemoConfig::isDemoBuild)
    {
        typeIndex = DemoConfig::allowedBossTypeIndices.at(static_cast<size_t>(GetRandomValue(
            0, static_cast<int32_t>(DemoConfig::allowedBossTypeIndices.size()) - 1)));
    }
    if (isFinal)
    {
        typeIndex = bossTypes.size() - 1;
    }
    const auto& type = bossTypes.at(typeIndex);

    const float finalBossMult = isFinal ? 3.0F : 1.0F;
    const auto health =
        static_cast<int32_t>(static_cast<float>(500 + tier * bossHpPerTier) * healthMult *
                             type.healthMult * finalBossMult * enemyHealthMult *
                             enemyHealthScale(game));
    const float size = 100.0F * sizeMult * type.sizeMult * (isFinal ? 1.3F : 1.0F);

    auto moveset = sampleBossMoveset(bossMoveCount);
    const BossAttack firstAttack = moveset.front();

    game.run.bosses.push_back(
        Boss{.position = spawnPos,
             .size = Vector2{.x = size, .y = size},
             .color = type.color,
             .baseColor = type.color,
             .health = health,
             .maxHealth = health,
             .state = BossState::IDLE,
             .attack = firstAttack,
             .moveset = std::move(moveset),
             .attackTimer = static_cast<float>(GetRandomValue(15, 35)) / 10.0F,
             .stateTimer = 0,
             .targetPosition = Vector2{},
             .slamHit = false,
             .beamShieldLatched = false,
             .wormholeBeamOrigin = Vector2{},
             .chargeVelocity = Vector2{},
             .barrageTimer = 0,
             .hitByDash = false,
             .isMega = isMega,
             .isSwarm = isSwarm,
             .strafePhase = static_cast<float>(GetRandomValue(0, 628)) / 100.0F,
             .hitFlashTimer = 0,
             .shape = type.shape,
             .typeIndex = static_cast<int32_t>(typeIndex),
             .isFinalBoss = isFinal});
}

void spawnBossWave(Game& game, float healthMult, float sizeMult, bool isMega)
{
    game.run.bossSpawnCount++;
    const bool isSwarm = game.run.bossSpawnCount % bossSwarmInterval == 0;
    const int count = isSwarm ? 3 : 1;
    for (int i = 0; i < count; i++)
    {
        spawnBossInstance(game, healthMult, sizeMult, isMega, isSwarm);
    }
    playSFX(game, game.resources.sounds.bossWindUp);
}

void spawnFinalBossWave(Game& game)
{
    game.run.bossSpawnCount++;
    spawnBossInstance(game, megaBossHealthMult, megaBossSizeMult, true, false, true);
    playSFX(game, game.resources.sounds.bossWindUp);
}

void spawnBeltbreaker(Game& game, int32_t wave)
{
    game.run.bossSpawnCount++;

    const int32_t tier = wave / megaBossWaveInterval;
    int32_t plateCount = 4;
    float rotationMult = 1.0F;
    if (wave >= 25)
    {
        plateCount = 8;
        rotationMult = 1.6F;
    }
    else if (wave >= 20)
    {
        plateCount = 6;
        rotationMult = 1.25F;
    }
    const bool overloadBeam = wave >= 20;

    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(500, 650));
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

    const auto health = static_cast<int32_t>(static_cast<float>(500 + tier * bossHpPerTier) *
                                             megaBossHealthMult * enemyHealthMult *
                                             enemyHealthScale(game));

    std::vector<BossAttack> moveset{BossAttack::Barrage, BossAttack::GravityWell,
                                    BossAttack::PlateHurl};
    if (overloadBeam)
    {
        moveset.push_back(BossAttack::Beam);
    }
    const BossAttack firstAttack = moveset.front();

    const float size = 170.0F + static_cast<float>(plateCount - 4) * 8.0F;

    Boss boss{.position = spawnPos,
              .size = Vector2{.x = size, .y = size},
              .color = Palette::StructMid,
              .baseColor = Palette::StructMid,
              .health = health,
              .maxHealth = health,
              .state = BossState::IDLE,
              .attack = firstAttack,
              .moveset = std::move(moveset),
              .attackTimer = static_cast<float>(GetRandomValue(20, 40)) / 10.0F,
              .stateTimer = 0,
              .targetPosition = Vector2{},
              .slamHit = false,
              .beamShieldLatched = false,
              .wormholeBeamOrigin = Vector2{},
              .chargeVelocity = Vector2{},
              .barrageTimer = 0,
              .hitByDash = false,
              .isMega = true,
              .isSwarm = false,
              .strafePhase = 0,
              .hitFlashTimer = 0,
              .shape = BossShape::HexPlated,
              .isBeltbreaker = true,
              .plateCount = plateCount,
              .plateAngleDeg = 0,
              .plateRotationSpeed = beltbreakerRotationSpeed * rotationMult,
              .instanceId = game.run.bossSpawnCount};

    const auto plateHealth = static_cast<int32_t>(UpdateConstants::beltbreakerPlateHealthPerTier *
                                                  enemyHealthMult * enemyHealthScale(game));
    for (int32_t i = 0; i < plateCount; i++)
    {
        Boss plate{.position = beltbreakerPlateSlotPosition(boss, i),
                   .size = Vector2{.x = 44, .y = 44},
                   .color = Palette::Shield,
                   .baseColor = Palette::Shield,
                   .health = plateHealth,
                   .maxHealth = plateHealth,
                   .state = BossState::IDLE,
                   .attack = BossAttack::Barrage,
                   .moveset = {BossAttack::Barrage, BossAttack::ChargeDash},
                   .attackTimer = static_cast<float>(GetRandomValue(20, 40)) / 10.0F,
                   .isMega = false,
                   .isSwarm = false,
                   .shape = BossShape::HexPlated,
                   .isBeltbreakerPlate = true,
                   .plateOwnerId = boss.instanceId,
                   .plateSlotIndex = i,
                   .plateAttached = true};
        game.run.bosses.push_back(std::move(plate));
    }

    game.run.bosses.push_back(std::move(boss));
    playSFX(game, game.resources.sounds.bossWindUp);
}

void spawnWreckworm(Game& game, int32_t wave)
{
    game.run.bossSpawnCount++;

    const int32_t tier = wave / megaBossWaveInterval;
    int32_t segmentCount = 8 * 2;
    bool burrowChargeUnlocked = false;
    std::vector<float> moltThresholds{0.5F};
    if (wave >= 50)
    {
        segmentCount = static_cast<int32_t>(10 * 3);
        burrowChargeUnlocked = true;
        moltThresholds = {0.66F, 0.33F};
    }
    else if (wave >= 40)
    {
        segmentCount = static_cast<int32_t>(9 * 2.5F);
        burrowChargeUnlocked = true;
    }

    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(500, 650));
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

    const auto health = static_cast<int32_t>(static_cast<float>(500 + tier * bossHpPerTier) *
                                             megaBossHealthMult * enemyHealthMult *
                                             enemyHealthScale(game));

    std::vector<BossAttack> moveset{BossAttack::Barrage,   BossAttack::CoilClamp,
                                    BossAttack::DashRush,  BossAttack::CloudBreath,
                                    BossAttack::TailWrap};
    if (burrowChargeUnlocked)
    {
        moveset.push_back(BossAttack::BurrowCharge);
    }
    const BossAttack firstAttack = moveset.front();

    constexpr float headSize = 192.0F;

    Boss head{.position = spawnPos,
              .size = Vector2{.x = headSize, .y = headSize},
              .color = Palette::RustbloomAccent,
              .baseColor = Palette::RustbloomAccent,
              .health = health,
              .maxHealth = health,
              .state = BossState::IDLE,
              .attack = firstAttack,
              .moveset = std::move(moveset),
              .attackTimer = static_cast<float>(GetRandomValue(20, 40)) / 10.0F,
              .stateTimer = 0,
              .targetPosition = Vector2{},
              .slamHit = false,
              .beamShieldLatched = false,
              .wormholeBeamOrigin = Vector2{},
              .chargeVelocity = Vector2{},
              .barrageTimer = 0,
              .hitByDash = false,
              .isMega = true,
              .isSwarm = false,
              .strafePhase = 0,
              .hitFlashTimer = 0,
              .shape = BossShape::Segment,
              .isBeltbreaker = false,
              .instanceId = game.run.bossSpawnCount,
              .isWreckwormHead = true,
              .segmentCount = segmentCount,
              .wreckwormSpeedMult = 1.0F,
              .moltThresholds = std::move(moltThresholds)};

    const Vector2 trailDir = Vector2Normalize(Vector2Subtract(spawnPos, game.run.player.position));
    seedWreckwormTrail(
        head, Vector2{.x = head.position.x + head.size.x / 2, .y = head.position.y + head.size.y / 2},
        trailDir);
    const auto infectedHealth =
        static_cast<int32_t>(wreckwormSegmentHealthPerTier * static_cast<float>(tier) *
                             enemyHealthMult * enemyHealthScale(game)) +
        60;
    const auto armorHealth =
        static_cast<int32_t>(static_cast<float>(infectedHealth) * wreckwormArmorHealthMult);
    for (int32_t i = 0; i < segmentCount; i++)
    {
        const bool armor = i % 2 == 0;
        const int32_t segHealth = armor ? armorHealth : infectedHealth;
        const Vector2 segPos = Vector2Add(
            spawnPos, Vector2Scale(trailDir, wreckwormSegmentSpacing * static_cast<float>(i + 1)));
        const Color segColor = armor ? Palette::StructMid : Palette::RustbloomAccent;

        Boss segment{.position = segPos,
                     .size = Vector2{.x = 80, .y = 80},
                     .color = segColor,
                     .baseColor = segColor,
                     .health = segHealth,
                     .maxHealth = segHealth,
                     .state = BossState::IDLE,
                     .attack = BossAttack::Barrage,
                     .moveset = {},
                     .attackTimer = 0,
                     .isMega = false,
                     .isSwarm = false,
                     .shape = BossShape::Segment,
                     .isWreckwormSegment = true,
                     .segmentOwnerId = head.instanceId,
                     .segmentIndex = i,
                     .isArmorSegment = armor,
                     .segmentVolleyTimer =
                         static_cast<float>(GetRandomValue(20, 45)) / 10.0F};
        game.run.bosses.push_back(std::move(segment));
    }

    game.run.bosses.push_back(std::move(head));
    playSFX(game, game.resources.sounds.bossWindUp);
}

void spawnSlagmaw(Game& game, int32_t wave)
{
    game.run.bossSpawnCount++;

    const int32_t tier = wave / megaBossWaveInterval;

    std::vector<BossAttack> moveset{BossAttack::Barrage, BossAttack::ChargeDash,
                                    BossAttack::MeteorHell, BossAttack::MeteorSwarm};
    float attackTimerMin = 2.5F;
    float attackTimerMax = 4.5F;
    if (wave >= 70)
    {
        moveset.push_back(BossAttack::HomingBarrage);
        attackTimerMin = 2.0F;
        attackTimerMax = 3.5F;
    }
    if (wave >= 75)
    {
        moveset.push_back(BossAttack::MeteorHell);
        attackTimerMin = 1.6F;
        attackTimerMax = 3.0F;
    }

    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(500, 650));
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

    const auto health = static_cast<int32_t>(static_cast<float>(500 + tier * bossHpPerTier) *
                                             megaBossHealthMult * enemyHealthMult *
                                             enemyHealthScale(game));

    constexpr float slagmawSize = 450.0F;
    const BossAttack firstAttack = moveset.front();

    Boss boss{.position = spawnPos,
              .size = Vector2{.x = slagmawSize, .y = slagmawSize},
              .color = Palette::SolarForgeAccent,
              .baseColor = Palette::SolarForgeAccent,
              .health = health,
              .maxHealth = health,
              .state = BossState::IDLE,
              .attack = firstAttack,
              .moveset = std::move(moveset),
              .attackTimer = static_cast<float>(GetRandomValue(
                                static_cast<int32_t>(attackTimerMin * 10),
                                static_cast<int32_t>(attackTimerMax * 10))) /
                            10.0F,
              .stateTimer = 0,
              .targetPosition = Vector2{},
              .slamHit = false,
              .beamShieldLatched = false,
              .wormholeBeamOrigin = Vector2{},
              .chargeVelocity = Vector2{},
              .barrageTimer = 0,
              .hitByDash = false,
              .isMega = true,
              .isSwarm = false,
              .strafePhase = 0,
              .hitFlashTimer = 0,
              .shape = BossShape::SpikedRing,
              .isBeltbreaker = false,
              .instanceId = game.run.bossSpawnCount,
              .isSlagmaw = true};

    if (wave == 75)
    {
        boss.slagmawBreaksIntoHalves = true;
    }
    else if (wave == 70)
    {
        boss.slagmawBreaksIntoDrifters = true;
    }

    game.run.bosses.push_back(std::move(boss));
    playSFX(game, game.resources.sounds.bossWindUp);
}

void spawnKraken(Game& game, int32_t wave)
{
    game.run.bossSpawnCount++;

    const int32_t tier = wave / megaBossWaveInterval;

    const std::vector<BossAttack> moveset{BossAttack::Barrage, BossAttack::Beam,
                                          BossAttack::GravityWell, BossAttack::HomingBarrage};

    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(500, 650));
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

    const auto health = static_cast<int32_t>(static_cast<float>(650 + tier * bossHpPerTier) *
                                             megaBossHealthMult * enemyHealthMult *
                                             enemyHealthScale(game));

    Boss boss{.position = spawnPos,
              .size = Vector2{.x = krakenSize, .y = krakenSize},
              .color = Palette::PunctumHaze,
              .baseColor = Palette::PunctumHaze,
              .health = health,
              .maxHealth = health,
              .state = BossState::IDLE,
              .attack = moveset.front(),
              .moveset = moveset,
              .attackTimer = static_cast<float>(GetRandomValue(20, 40)) / 10.0F,
              .stateTimer = 0,
              .targetPosition = Vector2{},
              .slamHit = false,
              .beamShieldLatched = false,
              .wormholeBeamOrigin = Vector2{},
              .chargeVelocity = Vector2{},
              .barrageTimer = 0,
              .hitByDash = false,
              .isMega = true,
              .isSwarm = false,
              .strafePhase = 0,
              .hitFlashTimer = 0,
              .shape = BossShape::HexPlated,
              .isBeltbreaker = false,
              .instanceId = game.run.bossSpawnCount,
              .isKraken = true};

    boss.krakenEncounter = wave >= 100 ? 3 : (wave >= 90 ? 2 : 1);
    if (wave >= 100)
    {
        boss.isFinalBoss = true;
    }

    game.run.bosses.push_back(std::move(boss));
    playSFX(game, game.resources.sounds.bossWindUp);
}

void spawnKrakenSnake(Game& game, Vector2 nearPos)
{
    game.run.bossSpawnCount++;

    const int32_t segmentCount = krakenSnakeSegmentCount;
    const std::vector<BossAttack> moveset{BossAttack::Barrage, BossAttack::CoilClamp,
                                          BossAttack::DashRush, BossAttack::CloudBreath,
                                          BossAttack::TailWrap};

    const auto health = static_cast<int32_t>(500.0F * megaBossHealthMult * enemyHealthMult *
                                             enemyHealthScale(game) * krakenSnakeHealthMult);

    constexpr float headSize = 140.0F;

    Boss head{.position = nearPos,
              .size = Vector2{.x = headSize, .y = headSize},
              .color = Palette::SolarForgeAccent,
              .baseColor = Palette::SolarForgeAccent,
              .health = health,
              .maxHealth = health,
              .state = BossState::IDLE,
              .attack = moveset.front(),
              .moveset = moveset,
              .attackTimer = static_cast<float>(GetRandomValue(20, 40)) / 10.0F,
              .stateTimer = 0,
              .targetPosition = Vector2{},
              .slamHit = false,
              .beamShieldLatched = false,
              .wormholeBeamOrigin = Vector2{},
              .chargeVelocity = Vector2{},
              .barrageTimer = 0,
              .hitByDash = false,
              .isMega = false,
              .isSwarm = false,
              .strafePhase = 0,
              .hitFlashTimer = 0,
              .shape = BossShape::Segment,
              .isBeltbreaker = false,
              .instanceId = game.run.bossSpawnCount,
              .isWreckwormHead = true,
              .segmentCount = segmentCount,
              .wreckwormSpeedMult = 1.2F,
              .krakenSnakeVariant = true};

    const Vector2 trailDir = Vector2Normalize(Vector2Subtract(nearPos, game.run.player.position));
    seedWreckwormTrail(
        head, Vector2{.x = head.position.x + head.size.x / 2, .y = head.position.y + head.size.y / 2},
        trailDir);

    const auto segHealth =
        static_cast<int32_t>(static_cast<float>(health) / static_cast<float>(segmentCount) * 0.6F);
    for (int32_t i = 0; i < segmentCount; i++)
    {
        const Vector2 segPos = Vector2Add(
            nearPos, Vector2Scale(trailDir, wreckwormSegmentSpacing * static_cast<float>(i + 1)));

        Boss segment{.position = segPos,
                     .size = Vector2{.x = 70, .y = 70},
                     .color = Palette::StructMid,
                     .baseColor = Palette::StructMid,
                     .health = segHealth,
                     .maxHealth = segHealth,
                     .state = BossState::IDLE,
                     .attack = BossAttack::Barrage,
                     .moveset = {},
                     .attackTimer = 0,
                     .isMega = false,
                     .isSwarm = false,
                     .shape = BossShape::Segment,
                     .isWreckwormSegment = true,
                     .segmentOwnerId = head.instanceId,
                     .segmentIndex = i,
                     .isArmorSegment = false,
                     .segmentVolleyTimer = static_cast<float>(GetRandomValue(20, 45)) / 10.0F,
                     .krakenSnakeVariant = true};
        game.run.bosses.push_back(std::move(segment));
    }

    game.run.bosses.push_back(std::move(head));
    playSFX(game, game.resources.sounds.bossWindUp);
}

void updateKrakenSummon(Game& game, Boss& boss, float deltaTime)
{
    if (!boss.isKraken || boss.health <= 0)
    {
        return;
    }

    if (boss.krakenEncounter >= 3)
    {
        if (boss.krakenSnakeSummoned)
        {
            return;
        }
        boss.krakenSummonTimer += deltaTime;
        if (boss.krakenSummonTimer >= krakenSnakeSpawnDelay)
        {
            boss.krakenSnakeSummoned = true;
            const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const auto dist = static_cast<float>(GetRandomValue(300, 420));
            const Vector2 pos = Vector2Add(
                game.run.player.position,
                Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
            spawnKrakenSnake(game, pos);
        }
        return;
    }

    boss.krakenSummonTimer -= deltaTime;
    if (boss.krakenSummonTimer > 0)
    {
        return;
    }

    if (boss.krakenEncounter == 1)
    {
        boss.krakenSummonTimer = krakenSummonIntervalE1;
        for (int32_t i = 0; i < krakenSummonCountE1; i++)
        {
            spawnEnemy(game);
        }
        return;
    }

    boss.krakenSummonTimer = krakenSummonIntervalE2;
    for (int32_t i = 0; i < krakenSummonCountE2; i++)
    {
        spawnEnemy(game);
        if (!game.run.enemies.empty())
        {
            game.run.enemies.back().health = static_cast<int32_t>(
                static_cast<float>(game.run.enemies.back().health) * krakenSummonHealthMultE2);
        }
    }
    spawnEliteHazard(game,
                     GetRandomValue(0, 1) == 0 ? EliteHazardRole::Warlord : EliteHazardRole::Suppressor);
}

void spawnBanished(Game& game)
{
    game.run.bossSpawnCount++;

    Boss boss{.position = Vector2{.x = -banishedSize / 2, .y = -banishedSize / 2},
              .size = Vector2{.x = banishedSize, .y = banishedSize},
              .color = Palette::Void,
              .baseColor = Palette::Void,
              .health = 1,
              .maxHealth = 1,
              .state = BossState::IDLE,
              .attack = BossAttack::Barrage,
              .moveset = {},
              .attackTimer = 999999.0F,
              .stateTimer = 0,
              .targetPosition = Vector2{},
              .slamHit = false,
              .beamShieldLatched = false,
              .wormholeBeamOrigin = Vector2{},
              .chargeVelocity = Vector2{},
              .barrageTimer = 0,
              .hitByDash = false,
              .isMega = true,
              .isSwarm = false,
              .strafePhase = 0,
              .hitFlashTimer = 0,
              .shape = BossShape::HexPlated,
              .isBeltbreaker = false,
              .instanceId = game.run.bossSpawnCount,
              .isBanished = true};

    game.run.bosses.push_back(std::move(boss));
}

void damageBanishedEyeBurst(Game& game, Boss& boss, int32_t divisor)
{
    const auto amount = std::max(1, boss.maxHealth / divisor);
    boss.health -= amount;
    boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
    boss.banishedEyeChargeBurstUsed = true;
    triggerBossShieldDashShake(game);
    playSFX(game, game.resources.sounds.critical);
}

void beginBanishedCoil(Boss& boss, Player& player, int32_t idx, Vector2 contactPoint)
{
    boss.banishedGrabbing[static_cast<size_t>(idx)] = true;
    boss.banishedPhase[static_cast<size_t>(idx)] = 6;
    boss.banishedTimer[static_cast<size_t>(idx)] = banishedCoilDuration;
    boss.banishedTip[static_cast<size_t>(idx)] = contactPoint;
    player.grabbed = true;
    player.grabbingTentacle = idx;
    player.launched = false;
    player.launchTargetTentacle = -1;
    player.position = contactPoint;
}

void resolveBanishedCoil(Game& game, Boss& boss, int32_t idx)
{
    Player& player = game.run.player;
    const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                             .y = boss.position.y + boss.size.y / 2};

    player.grabbed = false;
    player.grabbingTentacle = -1;

    if (player.launchRound >= banishedJuggleMaxRounds)
    {
        damagePlayer(game, enemyDamage(game, banishedThrashDamage));
        player.launchRound = 0;
        boss.banishedGrabbing[static_cast<size_t>(idx)] = false;
        boss.banishedPhase[static_cast<size_t>(idx)] = 3;
        boss.banishedTimer[static_cast<size_t>(idx)] = banishedTentacleRetreat;
        return;
    }

    int32_t catcher = -1;
    for (int32_t attempt = 0; attempt < Boss::banishedTentacleCount; attempt++)
    {
        const int32_t candidate = GetRandomValue(0, Boss::banishedTentacleCount - 1);
        if (candidate == idx || boss.banishedStaggered[static_cast<size_t>(candidate)] ||
            boss.banishedGrabbing[static_cast<size_t>(candidate)])
        {
            continue;
        }
        catcher = candidate;
        break;
    }

    if (catcher < 0)
    {
        damagePlayer(game, enemyDamage(game, banishedThrashDamage));
        player.launchRound = 0;
        boss.banishedGrabbing[static_cast<size_t>(idx)] = false;
        boss.banishedPhase[static_cast<size_t>(idx)] = 3;
        boss.banishedTimer[static_cast<size_t>(idx)] = banishedTentacleRetreat;
        return;
    }

    player.launchRound++;

    const float away = std::atan2(player.position.y - bossCenter.y, player.position.x - bossCenter.x);
    const float jitter = (static_cast<float>(GetRandomValue(-60, 60))) * DEG2RAD;
    const float angle = away + jitter;
    const float distance = static_cast<float>(
        GetRandomValue(static_cast<int32_t>(banishedLaunchDistanceMin),
                       static_cast<int32_t>(banishedLaunchDistanceMax)));
    const Vector2 dir{.x = std::cos(angle), .y = std::sin(angle)};
    player.launched = true;
    player.launchVelocity = Vector2Scale(dir, distance / banishedLaunchDuration);
    player.launchTargetTentacle = catcher;

    boss.banishedGrabbing[static_cast<size_t>(catcher)] = true;
    boss.banishedPhase[static_cast<size_t>(catcher)] = 5;
    boss.banishedTip[static_cast<size_t>(catcher)] = boss.banishedTip[static_cast<size_t>(idx)];
    boss.banishedAnchor[static_cast<size_t>(catcher)] = boss.banishedTip[static_cast<size_t>(catcher)];

    boss.banishedGrabbing[static_cast<size_t>(idx)] = false;
    boss.banishedPhase[static_cast<size_t>(idx)] = 3;
    boss.banishedTimer[static_cast<size_t>(idx)] = banishedTentacleRetreat;
}

void updateBanishedLaunch(Game& game, Boss& boss, float deltaTime)
{
    Player& player = game.run.player;
    const int32_t idx = player.launchTargetTentacle;

    if (idx < 0 || idx >= Boss::banishedTentacleCount || boss.banishedStaggered[static_cast<size_t>(idx)])
    {
        player.launched = false;
        player.launchTargetTentacle = -1;
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        const Vector2 aim = aimAtMouse(game);
        const Vector2 launchDir = Vector2Normalize(player.launchVelocity);
        const float dot = aim.x * -launchDir.x + aim.y * -launchDir.y;
        if (dot <= banishedJuggleCancelDot)
        {
            player.launched = false;
            player.dashing = true;
            player.dashTimer = dashChunkDuration(game);
            player.dashVelocity = Vector2Scale(aim, dashSpeed * currentShip(game).dashDistanceMult);
            player.immunityTimer = std::max(player.immunityTimer, 0.5F);

            boss.banishedGrabbing[static_cast<size_t>(idx)] = false;
            boss.banishedPhase[static_cast<size_t>(idx)] = 3;
            boss.banishedTimer[static_cast<size_t>(idx)] = banishedTentacleRetreat;
            player.launchTargetTentacle = -1;
            return;
        }
    }

    player.position = Vector2Add(player.position, Vector2Scale(player.launchVelocity, deltaTime));

    if (Vector2Distance(player.position, boss.banishedTip[static_cast<size_t>(idx)]) <=
        banishedCatchRadius)
    {
        beginBanishedCoil(boss, player, idx, player.position);
    }
}

void updateBanished(Game& game, Boss& boss, float deltaTime)
{
    if (!boss.isBanished)
    {
        return;
    }

    const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                             .y = boss.position.y + boss.size.y / 2};

    if (boss.banishedDefeated)
    {
        const Vector2 away = Vector2Subtract(bossCenter, game.run.player.position);
        const float dist = Vector2Length(away);
        const Vector2 dir = dist > 1.0F ? Vector2Scale(away, 1.0F / dist) : Vector2{.x = 0, .y = -1};
        const float fleeSpeed = std::min(banishedFleeSpeed, game.run.player.speed * 0.85F);
        boss.position = Vector2Add(boss.position, Vector2Scale(dir, fleeSpeed * deltaTime * frameScale));
        return;
    }

    if (boss.health <= 0)
    {
        boss.banishedDefeated = true;
        boss.banishedStage = 3;
        game.run.player.grabbed = false;
        game.run.player.grabbingTentacle = -1;
        game.run.player.launched = false;
        game.run.player.launchTargetTentacle = -1;
        game.run.player.launchRound = 0;
        return;
    }

    if (boss.banishedStage == 1)
    {
        const float chaseLerp = std::clamp(1.4F * deltaTime, 0.0F, 1.0F);
        boss.banishedEyePos = Vector2Lerp(boss.banishedEyePos, game.run.player.position, chaseLerp);
        boss.position =
            Vector2Subtract(boss.banishedEyePos, Vector2{.x = boss.size.x / 2, .y = boss.size.y / 2});
        boss.banishedEyeTimer += deltaTime;
        if (boss.banishedEyeTimer >= banishedEyeChargeDuration)
        {
            boss.banishedStage = 2;
            boss.banishedEyeTimer = 0;
            boss.targetPosition = game.run.player.position;
        }
        return;
    }

    if (boss.banishedStage == 2)
    {
        boss.banishedEyeTimer += deltaTime;
        const Vector2 eyePos{.x = boss.position.x + boss.size.x / 2,
                             .y = boss.position.y + boss.size.y / 2};
        const bool onBeam = CheckCollisionCircleLine(game.run.player.position, game.run.player.radius,
                                                     eyePos, boss.targetPosition);
        if (onBeam && game.run.player.dashing && game.run.player.shieldActive &&
            !boss.banishedEyeChargeBurstUsed)
        {
            damageBanishedEyeBurst(game, boss, banishedEyeShieldDashDamageDivisor);
        }
        else if (onBeam && !game.run.player.shieldActive)
        {
            boss.beamDamageAccum += banishedBeamDps * deltaTime;
            if (boss.beamDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(boss.beamDamageAccum);
                damagePlayer(game, tick);
                boss.beamDamageAccum -= static_cast<float>(tick);
            }
        }

        if (boss.banishedEyeTimer >= banishedEyeFireDuration)
        {
            boss.banishedStage = 0;
            boss.banishedEyeCharging = false;
            boss.banishedEyeChargeBurstUsed = false;
            game.run.player.grabbed = false;
            game.run.player.grabbingTentacle = -1;
            game.run.player.launched = false;
            game.run.player.launchTargetTentacle = -1;
            game.run.player.launchRound = 0;
            for (int32_t t = 0; t < Boss::banishedTentacleCount; t++)
            {
                boss.banishedStaggered[static_cast<size_t>(t)] = false;
                boss.banishedPhase[static_cast<size_t>(t)] = 0;
                boss.banishedGrabbing[static_cast<size_t>(t)] = false;
                boss.banishedCooldown[static_cast<size_t>(t)] =
                    static_cast<float>(GetRandomValue(
                        static_cast<int32_t>(banishedTentacleCooldownMin * 10),
                        static_cast<int32_t>(banishedTentacleCooldownMax * 10))) /
                    10.0F;
            }
        }
        return;
    }

    if (game.run.player.launched)
    {
        updateBanishedLaunch(game, boss, deltaTime);
    }

    const bool bossVisible =
        std::abs(bossCenter.x - game.run.player.position.x) <=
            static_cast<float>(game.resources.screenWidth) / 2.0F + boss.size.x / 2.0F &&
        std::abs(bossCenter.y - game.run.player.position.y) <=
            static_cast<float>(game.resources.screenHeight) / 2.0F + boss.size.y / 2.0F;
    const float tentacleDeltaTime =
        bossVisible ? deltaTime : deltaTime * banishedOffscreenAggressionMult;

    bool allStaggered = true;
    for (int32_t i = 0; i < Boss::banishedTentacleCount; i++)
    {
        const auto idx = static_cast<size_t>(i);
        if (boss.banishedTentacleHitFlash[idx] > 0)
        {
            boss.banishedTentacleHitFlash[idx] -= deltaTime;
        }
        if (!boss.banishedStaggered[idx])
        {
            allStaggered = false;
        }
        if (boss.banishedStaggered[idx])
        {
            continue;
        }

        if (boss.banishedPhase[idx] == 0)
        {
            boss.banishedCooldown[idx] -= tentacleDeltaTime;
            if (boss.banishedCooldown[idx] > 0)
            {
                continue;
            }

            const float distToPlayer = Vector2Distance(bossCenter, game.run.player.position);
            boss.banishedIsFar[idx] = distToPlayer > banishedNearReach;

            int32_t kind = GetRandomValue(0, 1);
            if (!boss.banishedIsFar[idx] && !game.run.player.grabbed && !game.run.player.launched &&
                GetRandomValue(0, 99) < static_cast<int32_t>(banishedGrabAttackChance * 100))
            {
                kind = 2;
            }
            boss.banishedAttackKind[idx] = kind;

            const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            boss.banishedAnchor[idx] =
                boss.banishedIsFar[idx]
                    ? Vector2Add(game.run.player.position,
                                Vector2{.x = std::cos(angle) * banishedFarPortalDist,
                                        .y = std::sin(angle) * banishedFarPortalDist})
                    : Vector2Add(bossCenter, Vector2{.x = std::cos(angle) * boss.size.x / 2 * 0.9F,
                                                     .y = std::sin(angle) * boss.size.y / 2 * 0.9F});
            boss.banishedTip[idx] = boss.banishedAnchor[idx];

            if (kind == 1)
            {
                const float startAngle =
                    std::atan2(game.run.player.position.y - boss.banishedAnchor[idx].y,
                              game.run.player.position.x - boss.banishedAnchor[idx].x);
                const float sweepDir = GetRandomValue(0, 1) == 0 ? 1.0F : -1.0F;
                boss.banishedThrashStartAngle[idx] = startAngle;
                boss.banishedThrashEndAngle[idx] =
                    startAngle + sweepDir * banishedThrashArcDegrees * DEG2RAD;
            }

            boss.banishedPhase[idx] = 1;
            boss.banishedTimer[idx] = banishedTentacleWindup;
            continue;
        }

        if (boss.banishedPhase[idx] == 1)
        {
            boss.banishedTimer[idx] -= tentacleDeltaTime;
            if (boss.banishedTimer[idx] <= 0)
            {
                boss.banishedPhase[idx] = 2;
                boss.banishedTimer[idx] = banishedTentacleStrike;
            }
            continue;
        }

        if (boss.banishedPhase[idx] == 2)
        {
            const float progress =
                std::clamp(1.0F - boss.banishedTimer[idx] / banishedTentacleStrike, 0.0F, 1.0F);

            if (boss.banishedAttackKind[idx] == 1)
            {
                const float radius = std::min(
                    Vector2Distance(boss.banishedAnchor[idx], game.run.player.position),
                    banishedNearReach * 1.3F);
                const float angle =
                    boss.banishedThrashStartAngle[idx] +
                    (boss.banishedThrashEndAngle[idx] - boss.banishedThrashStartAngle[idx]) * progress;
                const Vector2 dir{.x = std::cos(angle), .y = std::sin(angle)};
                boss.banishedTip[idx] = Vector2Add(boss.banishedAnchor[idx], Vector2Scale(dir, radius));
            }
            else
            {
                const Vector2 toPlayer =
                    Vector2Subtract(game.run.player.position, boss.banishedAnchor[idx]);
                const float dist = std::min(Vector2Length(toPlayer), banishedNearReach * 1.3F);
                const Vector2 dir = Vector2Length(toPlayer) > 0.01F
                                        ? Vector2Scale(toPlayer, 1.0F / Vector2Length(toPlayer))
                                        : Vector2{.x = 0, .y = 1};
                boss.banishedTip[idx] =
                    Vector2Add(boss.banishedAnchor[idx], Vector2Scale(dir, dist * progress));
            }

            boss.banishedTimer[idx] -= tentacleDeltaTime;

            if (Vector2Distance(boss.banishedTip[idx], game.run.player.position) <=
                banishedTentacleHitRadius + game.run.player.radius)
            {
                if (game.run.player.shieldActive)
                {
                    boss.banishedStaggered[idx] = true;
                    boss.banishedPhase[idx] = 0;
                    boss.banishedTentacleHitFlash[idx] = UpdateConstants::hitFlashDuration;
                    triggerBossShieldDashShake(game);
                }
                else if (boss.banishedAttackKind[idx] == 2)
                {
                    beginBanishedCoil(boss, game.run.player, i, boss.banishedTip[idx]);
                }
                else
                {
                    const int32_t dmg =
                        boss.banishedAttackKind[idx] == 1 ? banishedThrashDamage : banishedTentacleDamage;
                    damagePlayer(game, enemyDamage(game, dmg));
                    boss.banishedPhase[idx] = 3;
                    boss.banishedTimer[idx] = banishedTentacleRetreat;
                }
                continue;
            }

            if (boss.banishedTimer[idx] <= 0)
            {
                boss.banishedPhase[idx] = 3;
                boss.banishedTimer[idx] = banishedTentacleRetreat;
            }
            continue;
        }

        if (boss.banishedPhase[idx] == 3)
        {
            boss.banishedTimer[idx] -= tentacleDeltaTime;
            if (boss.banishedTimer[idx] <= 0)
            {
                boss.banishedPhase[idx] = 0;
                boss.banishedCooldown[idx] =
                    static_cast<float>(GetRandomValue(
                        static_cast<int32_t>(banishedTentacleCooldownMin * 10),
                        static_cast<int32_t>(banishedTentacleCooldownMax * 10))) /
                    10.0F;
            }
            continue;
        }

        if (boss.banishedPhase[idx] == 5)
        {
            const float easeLerp = std::clamp(3.0F * deltaTime, 0.0F, 1.0F);
            boss.banishedTip[idx] = Vector2Lerp(boss.banishedTip[idx], game.run.player.position, easeLerp);
            boss.banishedAnchor[idx] = boss.banishedTip[idx];
            continue;
        }

        if (boss.banishedPhase[idx] == 6)
        {
            game.run.player.position = boss.banishedTip[idx];
            game.run.player.dashing = false;
            boss.banishedTimer[idx] -= deltaTime;
            if (boss.banishedTimer[idx] <= 0)
            {
                resolveBanishedCoil(game, boss, i);
            }
        }
    }

    if (allStaggered)
    {
        boss.banishedStage = 1;
        boss.banishedEyeTimer = 0;
        boss.banishedEyePos = bossCenter;
        boss.banishedEyeChargeBurstUsed = false;
    }
}

void spawnSlagmawDrifterBreak(Game& game, Vector2 position)
{
    for (int32_t i = 0; i < slagmawBreakDrifterCount; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto dist = static_cast<float>(GetRandomValue(40, 140));
        const Vector2 pos = Vector2Add(
            position, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        spawnEnemyAt(game, enemyKindMeteorChunk, pos);
        auto& drifter = game.run.enemies.back();
        drifter.health =
            static_cast<int32_t>(static_cast<float>(drifter.health) * slagmawBrokenDrifterHealthMult);
    }
}

void spawnSlagmawHalfBreak(Game& game, const Boss& original, Vector2 bossCenter,
                           std::vector<Boss>& pendingHalves)
{
    const float halfSize = original.size.x * 0.5F;
    const auto halfHealth = std::max(1, original.maxHealth / 3);

    for (int32_t i = 0; i < 2; i++)
    {
        game.run.bossSpawnCount++;

        std::vector<BossAttack> moveset{BossAttack::Barrage,      BossAttack::ChargeDash,
                                        BossAttack::MeteorHell,   BossAttack::MeteorSwarm,
                                        BossAttack::HomingBarrage, BossAttack::MeteorHell};
        const BossAttack firstAttack = moveset.front();

        const float offsetAngle = i == 0 ? 0.0F : 180.0F;
        const Vector2 spawnCenter = Vector2Add(
            bossCenter, Vector2{.x = std::cos(offsetAngle * DEG2RAD) * 40.0F,
                               .y = std::sin(offsetAngle * DEG2RAD) * 40.0F});

        Boss half{.position = Vector2Subtract(spawnCenter,
                                              Vector2{.x = halfSize / 2, .y = halfSize / 2}),
                 .size = Vector2{.x = halfSize, .y = halfSize},
                 .color = Palette::SolarForgeAccent,
                 .baseColor = Palette::SolarForgeAccent,
                 .health = halfHealth,
                 .maxHealth = halfHealth,
                 .state = BossState::IDLE,
                 .attack = firstAttack,
                 .moveset = std::move(moveset),
                 .attackTimer = 2.0F,
                 .stateTimer = 0,
                 .targetPosition = Vector2{},
                 .slamHit = false,
                 .beamShieldLatched = false,
                 .wormholeBeamOrigin = Vector2{},
                 .chargeVelocity = Vector2{},
                 .barrageTimer = 0,
                 .hitByDash = false,
                 .isMega = true,
                 .isSwarm = false,
                 .strafePhase = 0,
                 .hitFlashTimer = 0,
                 .shape = BossShape::SpikedRing,
                 .isBeltbreaker = false,
                 .instanceId = game.run.bossSpawnCount,
                 .isSlagmaw = true,
                 .slagmawBreaksIntoDrifters = true};
        pendingHalves.push_back(std::move(half));
    }
}

void spawnBoss(Game& game) { spawnBossWave(game, megaBossHealthMult, megaBossSizeMult, true); }

void spawnMiniboss(Game& game) { spawnBossWave(game, miniBossHealthMult, miniBossSizeMult, false); }

void spawnSwarmBoss(Game& game)
{
    for (int i = 0; i < 3; i++)
    {
        spawnBossInstance(game, megaBossHealthMult, megaBossSizeMult, true, true);
    }
    playSFX(game, game.resources.sounds.bossWindUp);
}

void updateBossMovement(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
{
    if (boss.hitFlashTimer > 0)
    {
        boss.hitFlashTimer -= deltaTime;
    }

    if (boss.recoveryTimer > 0)
    {
        boss.recoveryTimer -= deltaTime;
    }

    if (!boss.isWreckwormSegment && !(boss.isBeltbreakerPlate && boss.plateAttached) &&
        boss.health > 0 && boss.state == BossState::IDLE &&
        Vector2Distance(game.run.player.position, bossCenter) <= UpdateConstants::shockwaveStompRadius)
    {
        boss.meleeStompTimer += deltaTime;
        if (boss.meleeStompTimer >= bossMeleeStompTriggerDuration)
        {
            boss.meleeStompTimer = 0;
            forceBossAttack(game, boss, BossAttack::ShockwaveStomp);
        }
    }
    else
    {
        boss.meleeStompTimer = 0;
    }

    if (boss.isWreckwormSegment)
    {
        return;
    }

    if (boss.isBeltbreakerPlate)
    {
        if (boss.plateAttached)
        {

            if (Boss* core = findBeltbreakerById(game, boss.plateOwnerId); core != nullptr)
            {
                const Vector2 slotCenter = beltbreakerPlateSlotPosition(*core, boss.plateSlotIndex);
                boss.position = Vector2Subtract(
                    slotCenter, Vector2{.x = boss.size.x / 2, .y = boss.size.y / 2});
            }

            if (boss.health > 0 && boss.health < boss.maxHealth)
            {
                const float healPerSecond =
                    static_cast<float>(boss.maxHealth) / beltbreakerPlateRegenDuration;
                boss.health =
                    std::min(boss.maxHealth,
                             boss.health + static_cast<int32_t>(healPerSecond * deltaTime) + 1);
            }
            return;
        }
        if (boss.plateReturning)
        {
            if (Boss* core = findBeltbreakerById(game, boss.plateOwnerId); core != nullptr)
            {
                const Vector2 targetCenter =
                    beltbreakerPlateSlotPosition(*core, boss.plateSlotIndex);
                const Vector2 dir = Vector2Subtract(targetCenter, bossCenter);
                const float dist = Vector2Length(dir);
                if (dist > beltbreakerReturnArriveDist)
                {
                    boss.position =
                        Vector2Add(boss.position,
                                   Vector2Scale(Vector2Normalize(dir), beltbreakerPlateReturnSpeed *
                                                                           deltaTime * frameScale));
                }
                else
                {

                    boss.plateAttached = true;
                    boss.plateReturning = false;
                    boss.plateExcursionTimer = 0;
                }
            }
            else
            {

                boss.plateReturning = false;
            }
            return;
        }

        if (boss.plateExcursionTimer > 0)
        {
            boss.plateExcursionTimer -= deltaTime;
            if (boss.plateExcursionTimer <= 0)
            {
                boss.plateExcursionTimer = 0;
                boss.plateReturning = true;
                return;
            }
        }

    }

    if (boss.isSlagmaw && boss.health > 0)
    {
        const float offscreenDist =
            std::max(static_cast<float>(game.resources.screenWidth),
                     static_cast<float>(game.resources.screenHeight)) /
                2 +
            ghostTeleportMargin;
        if (boss.ghostFadeAlpha >= 1.0F &&
            Vector2Distance(bossCenter, game.run.player.position) > offscreenDist)
        {
            const float teleportAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 spawnCenter = Vector2Add(
                game.run.player.position,
                Vector2{.x = std::cos(teleportAngle) * offscreenDist * 0.85F,
                        .y = std::sin(teleportAngle) * offscreenDist * 0.85F});
            boss.position =
                Vector2Subtract(spawnCenter, Vector2{.x = boss.size.x / 2, .y = boss.size.y / 2});
            boss.ghostFadeAlpha = 0;
        }
        if (boss.ghostFadeAlpha < 1.0F)
        {
            boss.ghostFadeAlpha = std::min(1.0F, boss.ghostFadeAlpha + deltaTime / ghostFadeDuration);
        }
    }

    if (boss.state == BossState::SHOOTING || boss.recoveryTimer > 0)
    {
        return;
    }

    if (boss.debuffStatic)
    {
        return;
    }

    const Vector2 toPlayer = Vector2Subtract(game.run.player.position, bossCenter);
    const float dist = Vector2Length(toPlayer);
    if (dist <= 1.0F)
    {
        return;
    }

    const Vector2 dirToPlayer = Vector2Scale(toPlayer, 1.0F / dist);
    const Vector2 perp{.x = -dirToPlayer.y, .y = dirToPlayer.x};

    const bool enraged =
        boss.health > 0 &&
        static_cast<float>(boss.health) <= static_cast<float>(boss.maxHealth) * enrageHealthFrac;

    const float speedMult = (enraged ? 1.25F : 1.0F) * (boss.debuffFreeze ? freezeSlowMult : 1.0F) *
                            (boss.isBeltbreakerPlate ? beltbreakerPlateMoveSpeedMult : 1.0F) *
                            (boss.isWreckwormHead ? boss.wreckwormSpeedMult * wreckwormChaseSpeedMult
                                                  : 1.0F) *
                            (boss.isSlagmaw ? slagmawMoveSpeedMult : 1.0F);

    Vector2 move;
    if (boss.isWreckwormHead)
    {
        move = dirToPlayer;
    }
    else
    {
        const float radialAmount =
            std::clamp((dist - bossEngageDistance) / bossEngageDistance, -1.5F, 1.5F);
        const float strafe = std::sin(static_cast<float>(GetTime()) * 0.9F + boss.strafePhase);
        move = Vector2Add(Vector2Scale(dirToPlayer, radialAmount), Vector2Scale(perp, strafe));
    }

    if (Vector2Length(move) > 0)
    {
        const float chaseSpeed = (game.run.player.speed * 1.15F + 1) * speedMult;
        move = Vector2Scale(Vector2Normalize(move), chaseSpeed);
        boss.position = Vector2Add(boss.position, Vector2Scale(move, deltaTime * frameScale));

        if (boss.isWreckwormHead && GetRandomValue(0, 3) == 0)
        {
            game.run.flameParticles.push_back(
                Particle{.position = bossCenter,
                        .velocity = Vector2Scale(move, -0.3F),
                        .radius = boss.size.x * 0.05F,
                        .life = 1.2F,
                        .maxLife = 1.2F,
                        .color = Palette::RustbloomAccent});
        }
    }
}

auto beltbreakerCoreCenter(const Boss& core) -> Vector2
{
    return Vector2{.x = core.position.x + core.size.x / 2, .y = core.position.y + core.size.y / 2};
}

auto beltbreakerPlateSlotPosition(const Boss& core, int32_t slotIndex) -> Vector2
{
    const float slotWidth = 360.0F / static_cast<float>(core.plateCount);
    const float deg = core.plateAngleDeg + static_cast<float>(slotIndex) * slotWidth;
    const float radius = core.size.x * 0.75F;
    return Vector2Add(beltbreakerCoreCenter(core), Vector2{.x = std::cos(deg * DEG2RAD) * radius,
                                                           .y = std::sin(deg * DEG2RAD) * radius});
}

auto findBeltbreakerById(Game& game, int32_t instanceId) -> Boss*
{
    for (auto& boss : game.run.bosses)
    {
        if (boss.isBeltbreaker && boss.health > 0 && boss.instanceId == instanceId)
        {
            return &boss;
        }
    }
    return nullptr;
}

auto countAttachedAlivePlates(Game& game, const Boss& core) -> int32_t
{
    int32_t count = 0;
    for (const auto& boss : game.run.bosses)
    {
        if (boss.isBeltbreakerPlate && boss.plateOwnerId == core.instanceId && boss.plateAttached &&
            boss.health > 0)
        {
            count++;
        }
    }
    return count;
}

auto countContributingPlates(Game& game, const Boss& core) -> int32_t
{
    int32_t count = 0;
    for (const auto& boss : game.run.bosses)
    {
        if (boss.isBeltbreakerPlate && boss.plateOwnerId == core.instanceId && boss.plateAttached &&
            boss.health >= boss.maxHealth)
        {
            count++;
        }
    }
    return count;
}

void detachBeltbreakerPlates(Game& game, const Boss& core)
{
    for (auto& boss : game.run.bosses)
    {
        if (boss.isBeltbreakerPlate && boss.plateOwnerId == core.instanceId && boss.plateAttached &&
            boss.health > 0)
        {
            boss.plateAttached = false;
            boss.state = BossState::IDLE;
            boss.attackTimer = static_cast<float>(GetRandomValue(5, 20)) / 10.0F;
        }
    }
}

void triggerBeltbreakerReturn(Game& game, const Boss& core)
{
    for (auto& boss : game.run.bosses)
    {
        if (boss.isBeltbreakerPlate && boss.plateOwnerId == core.instanceId &&
            !boss.plateAttached && boss.health > 0)
        {
            boss.plateReturning = true;
        }
    }
}

void sendPlateOnExcursion(Boss& plate, float durationSeconds)
{
    plate.plateAttached = false;
    plate.plateExcursionTimer = durationSeconds;
}

void updateBeltbreakerCore(Game& game, Boss& core, float deltaTime)
{
    if (!core.isBeltbreaker || core.health <= 0)
    {
        return;
    }

    core.plateAngleDeg =
        std::fmod(core.plateAngleDeg + core.plateRotationSpeed * deltaTime, 360.0F);

    if (!core.beltbreakerShielded)
    {
        if (core.plateCount > 0)
        {
            const float contributingFrac = static_cast<float>(countContributingPlates(game, core)) /
                                           static_cast<float>(core.plateCount);
            core.shieldGenProgress += deltaTime * beltbreakerShieldGenRate * contributingFrac;
        }
        if (core.shieldGenProgress >= 1.0F)
        {
            core.shieldGenProgress = 0;
            core.beltbreakerShielded = true;
            core.beltbreakerReturnTriggered = false;
            core.beltbreakerShieldTimer =
                UpdateConstants::beltbreakerShieldedDuration(core.plateCount);
            detachBeltbreakerPlates(game, core);
            core.harassTimer = 0;
            return;
        }

        core.harassTimer -= deltaTime;
        if (core.harassTimer <= 0)
        {
            core.harassTimer = static_cast<float>(GetRandomValue(40, 70)) / 10.0F;

            std::vector<Boss*> candidates;
            for (auto& boss : game.run.bosses)
            {
                if (boss.isBeltbreakerPlate && boss.plateOwnerId == core.instanceId &&
                    boss.plateAttached && boss.health >= boss.maxHealth)
                {
                    candidates.push_back(&boss);
                }
            }
            if (!candidates.empty())
            {
                Boss* chosen = candidates.at(static_cast<size_t>(
                    GetRandomValue(0, static_cast<int32_t>(candidates.size()) - 1)));
                sendPlateOnExcursion(*chosen, beltbreakerHarassExcursionDuration);
                chosen->state = BossState::IDLE;
                chosen->attackTimer = 0.3F;
            }
        }
        return;
    }

    core.beltbreakerShieldTimer -= deltaTime;
    if (!core.beltbreakerReturnTriggered &&
        core.beltbreakerShieldTimer <= beltbreakerReturnLeadTime)
    {
        core.beltbreakerReturnTriggered = true;
        triggerBeltbreakerReturn(game, core);
    }

    bool anyStillOut = false;
    for (const auto& boss : game.run.bosses)
    {
        if (boss.isBeltbreakerPlate && boss.plateOwnerId == core.instanceId &&
            !boss.plateAttached && boss.health > 0)
        {
            anyStillOut = true;
            break;
        }
    }
    if (!anyStillOut)
    {
        core.beltbreakerShielded = false;
    }
}

auto wreckwormDashRushOffDist(const Game& game) -> float
{
    return std::max(static_cast<float>(game.resources.screenWidth),
                    static_cast<float>(game.resources.screenHeight)) /
               2 +
           wreckwormDashRushOffscreenMargin;
}

auto wreckwormDashRushTravelDuration(const Game& game, int32_t segmentCount) -> float
{
    const float travelDist = 2.0F * wreckwormDashRushOffDist(game) +
                             static_cast<float>(segmentCount) * wreckwormSegmentSpacing;
    return travelDist / (chargeDashSpeed * wreckwormDashSpeedMult);
}

auto findWreckwormSegment(Game& game, int32_t headId, int32_t segmentIndex) -> Boss*
{
    for (auto& boss : game.run.bosses)
    {
        if (boss.isWreckwormSegment && boss.health > 0 && boss.segmentOwnerId == headId &&
            boss.segmentIndex == segmentIndex)
        {
            return &boss;
        }
    }
    return nullptr;
}

void seedWreckwormTrail(Boss& head, Vector2 headCenter, Vector2 trailDir)
{
    head.wreckwormTrail.clear();
    for (int32_t i = 1; i <= head.segmentCount + 2; i++)
    {
        head.wreckwormTrail.push_back(
            Vector2Add(headCenter, Vector2Scale(trailDir, wreckwormSegmentSpacing * static_cast<float>(i))));
    }
}

auto pointAlongWreckwormTrail(Vector2 headCenter, const std::vector<Vector2>& trail, float dist)
    -> Vector2
{
    Vector2 prev = headCenter;
    float remaining = dist;
    for (const auto& pt : trail)
    {
        const float segDist = Vector2Distance(prev, pt);
        if (segDist >= remaining)
        {
            return segDist < 0.0001F ? pt : Vector2Lerp(prev, pt, remaining / segDist);
        }
        remaining -= segDist;
        prev = pt;
    }

    if (trail.size() >= 2)
    {
        const Vector2 dir =
            Vector2Normalize(Vector2Subtract(trail.back(), trail.at(trail.size() - 2)));
        return Vector2Add(trail.back(), Vector2Scale(dir, remaining));
    }
    if (!trail.empty())
    {
        const Vector2 dir = Vector2Normalize(Vector2Subtract(trail.back(), headCenter));
        return Vector2Length(dir) > 0 ? Vector2Add(trail.back(), Vector2Scale(dir, remaining))
                                       : trail.back();
    }
    return headCenter;
}

void moveWreckwormSegmentToward(Boss& seg, Vector2 targetCenter, float speed, float deltaTime)
{
    const Vector2 segCenter{.x = seg.position.x + seg.size.x / 2, .y = seg.position.y + seg.size.y / 2};
    const Vector2 toTarget = Vector2Subtract(targetCenter, segCenter);
    const float dist = Vector2Length(toTarget);
    const Vector2 newCenter =
        dist > 1.0F
            ? Vector2Add(segCenter,
                         Vector2Scale(toTarget, std::min(dist, speed * deltaTime * frameScale) / dist))
            : targetCenter;
    seg.position = Vector2Subtract(newCenter, Vector2{.x = seg.size.x / 2, .y = seg.size.y / 2});
}

void updateWreckwormChain(Game& game, Boss& head, float deltaTime)
{
    if (!head.isWreckwormHead)
    {
        return;
    }
    if (head.state == BossState::SHOOTING &&
        (head.attack == BossAttack::CoilClamp || head.attack == BossAttack::TailWrap))
    {
        return;
    }
    (void)deltaTime;

    const Vector2 headCenter{.x = head.position.x + head.size.x / 2,
                             .y = head.position.y + head.size.y / 2};

    if (head.wreckwormTrail.empty() ||
        Vector2Distance(head.wreckwormTrail.front(), headCenter) > wreckwormTrailSampleDist)
    {
        head.wreckwormTrail.insert(head.wreckwormTrail.begin(), headCenter);
    }

    const float maxTrailLength =
        static_cast<float>(head.segmentCount + 2) * wreckwormSegmentSpacing + 200.0F;
    float accum = 0;
    size_t keep = head.wreckwormTrail.size();
    for (size_t i = 1; i < head.wreckwormTrail.size(); i++)
    {
        accum += Vector2Distance(head.wreckwormTrail.at(i - 1), head.wreckwormTrail.at(i));
        if (accum > maxTrailLength)
        {
            keep = i + 1;
            break;
        }
    }
    if (keep < head.wreckwormTrail.size())
    {
        head.wreckwormTrail.resize(keep);
    }

    for (int32_t i = 0; i < head.segmentCount; i++)
    {
        Boss* seg = findWreckwormSegment(game, head.instanceId, i);
        if (seg == nullptr)
        {
            continue;
        }
        const float targetDist = static_cast<float>(i + 1) * wreckwormSegmentSpacing;
        const Vector2 point = pointAlongWreckwormTrail(headCenter, head.wreckwormTrail, targetDist);
        seg->position =
            Vector2Subtract(point, Vector2{.x = seg->size.x / 2, .y = seg->size.y / 2});
    }
}

void updateKrakenTentacle(Game& game, Boss& boss, float deltaTime)
{
    if (!boss.isKraken || boss.health <= 0)
    {
        return;
    }

    if (boss.tentacleCooldownTimer > 0)
    {
        boss.tentacleCooldownTimer -= deltaTime;
    }

    if (boss.tentaclePhase == 0)
    {
        boss.tentacleCheckTimer -= deltaTime;
        if (boss.tentacleCheckTimer > 0)
        {
            return;
        }
        boss.tentacleCheckTimer = krakenTentacleCheckInterval;

        if (boss.tentacleCooldownTimer > 0 ||
            static_cast<float>(GetRandomValue(0, 99)) / 100.0F >= krakenTentacleTriggerChance)
        {
            return;
        }

        const float portalAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto portalDist = static_cast<float>(GetRandomValue(90, 160));
        boss.tentaclePortalPos = Vector2Add(
            game.run.player.position,
            Vector2{.x = std::cos(portalAngle) * portalDist, .y = std::sin(portalAngle) * portalDist});

        const float aimAngle = std::atan2(game.run.player.position.y - boss.tentaclePortalPos.y,
                                          game.run.player.position.x - boss.tentaclePortalPos.x);
        const float halfArc = krakenTentacleSwipeArcDeg * DEG2RAD / 2.0F;
        boss.tentacleSwipeStartAngle = aimAngle - halfArc;
        boss.tentacleSwipeEndAngle = aimAngle + halfArc;

        boss.tentaclePhase = 1;
        boss.tentacleStateTimer = krakenTentacleEmergeDuration;
        boss.tentacleHitPlayerThisSwipe = false;
        playSFX(game, game.resources.sounds.bossWindUp);
        return;
    }

    boss.tentacleStateTimer -= deltaTime;

    if (boss.tentaclePhase == 1)
    {
        if (boss.tentacleStateTimer <= 0)
        {
            boss.tentaclePhase = 2;
            boss.tentacleStateTimer = krakenTentacleSwipeDuration;
        }
        return;
    }

    if (boss.tentaclePhase == 2)
    {
        const float progress =
            std::clamp(1.0F - boss.tentacleStateTimer / krakenTentacleSwipeDuration, 0.0F, 1.0F);
        const float angle =
            boss.tentacleSwipeStartAngle +
            (boss.tentacleSwipeEndAngle - boss.tentacleSwipeStartAngle) * progress;
        const Vector2 tip =
            Vector2Add(boss.tentaclePortalPos,
                      Vector2{.x = std::cos(angle) * krakenTentacleReach,
                              .y = std::sin(angle) * krakenTentacleReach});

        if (!boss.tentacleHitPlayerThisSwipe && game.run.player.health > 0 &&
            Vector2Distance(tip, game.run.player.position) <=
                krakenTentacleHitRadius + game.run.player.radius)
        {
            const bool blocking =
                game.run.player.shieldActive || (game.run.player.dashing && game.run.player.shieldActive);
            if (blocking)
            {
                boss.tentacleHitPlayerThisSwipe = true;
                boss.tentacleCooldownTimer = krakenTentacleCooldownDuration;
                boss.tentaclePhase = 4;
                boss.tentacleStateTimer = krakenTentacleStaggerRetreatDuration;
                damageBoss(game, boss, krakenTentacleParryDamage, false);
                triggerBossShieldDashShake(game);
                return;
            }
            boss.tentacleHitPlayerThisSwipe = true;
            damagePlayer(game, enemyDamage(game, krakenTentaclePlayerDamage));
        }

        if (boss.tentacleStateTimer <= 0)
        {
            boss.tentaclePhase = 3;
            boss.tentacleStateTimer = krakenTentacleRetreatDuration;
        }
        return;
    }

    if (boss.tentaclePhase == 3 || boss.tentaclePhase == 4)
    {
        if (boss.tentacleStateTimer <= 0)
        {
            boss.tentaclePhase = 0;
        }
    }
}

auto krakenPodPosition(const Boss& boss, int32_t podIndex) -> Vector2
{
    const Vector2 center{.x = boss.position.x + boss.size.x / 2, .y = boss.position.y + boss.size.y / 2};
    const float seed = static_cast<float>(boss.instanceId);
    const float angle = static_cast<float>(podIndex) * 90.0F * DEG2RAD + seed;
    return Vector2Add(center, Vector2{.x = std::cos(angle) * boss.size.x / 2 * 0.78F,
                                      .y = std::sin(angle) * boss.size.y / 2 * 0.78F});
}

void updateKrakenLimbs(Game& game, Boss& boss, float deltaTime)
{
    if (!boss.isKraken || boss.health <= 0 || boss.krakenEncounter < 2)
    {
        return;
    }

    const int32_t activeSlots = boss.krakenEncounter >= 3 ? 2 : 1;

    for (int32_t i = 0; i < Boss::krakenLimbSlots; i++)
    {
        if (i >= activeSlots)
        {
            boss.limbPhase[i] = 0;
            boss.limbPod[i] = -1;
            continue;
        }

        if (boss.limbCooldown[i] > 0)
        {
            boss.limbCooldown[i] -= deltaTime;
        }

        if (boss.limbPhase[i] == 0)
        {
            if (boss.limbCooldown[i] > 0)
            {
                continue;
            }
            const int32_t otherPod = (i == 0) ? boss.limbPod[1] : boss.limbPod[0];
            int32_t pod = GetRandomValue(0, 3);
            if (pod == otherPod)
            {
                pod = (pod + 1) % 4;
            }
            boss.limbPod[i] = pod;
            boss.limbPhase[i] = 1;
            boss.limbTimer[i] = krakenLimbOpenDuration;
            boss.limbGrabTimer[i] = krakenLimbGrabInterval * 0.4F;
            continue;
        }

        if (boss.limbPhase[i] == 1 || boss.limbPhase[i] == 2)
        {
            const Vector2 podPos = krakenPodPosition(boss, boss.limbPod[i]);
            const Vector2 toPlayer = Vector2Subtract(game.run.player.position, podPos);
            const float distToPlayer = Vector2Length(toPlayer);
            const Vector2 aimDir =
                distToPlayer > 1.0F ? Vector2Scale(toPlayer, 1.0F / distToPlayer) : Vector2{.x = 1, .y = 0};
            boss.limbGrabTarget[i] = Vector2Add(podPos, Vector2Scale(aimDir, krakenLimbNearReach));
        }

        if (boss.limbPhase[i] == 1)
        {
            boss.limbTimer[i] -= deltaTime;
            if (boss.limbTimer[i] <= 0)
            {
                boss.limbPhase[i] = 2;
                boss.limbTimer[i] = krakenLimbActiveDuration;
            }
            continue;
        }

        if (boss.limbPhase[i] == 2)
        {
            boss.limbTimer[i] -= deltaTime;

            if (boss.limbThrowPhase[i] != 0)
            {
                boss.limbThrowTimer[i] -= deltaTime;
                if (boss.limbThrowPhase[i] == 1 && boss.limbThrowTimer[i] <= 0)
                {
                    boss.limbThrowPhase[i] = 2;
                    boss.limbThrowTimer[i] = krakenLimbThrowDuration;
                }
                else if (boss.limbThrowPhase[i] == 2 && boss.limbThrowTimer[i] <= 0)
                {
                    const Vector2 podPos = krakenPodPosition(boss, boss.limbPod[i]);
                    const Vector2 toPlayer = Vector2Subtract(game.run.player.position, podPos);
                    const float dist = Vector2Length(toPlayer);
                    const Vector2 dir = dist > 1.0F ? Vector2Scale(toPlayer, 1.0F / dist)
                                                    : Vector2{.x = 1, .y = 0};
                    const Vector2 launchPos =
                        Vector2Add(podPos, Vector2Scale(dir, krakenLimbNearReach));
                    game.run.bossProjectiles.push_back(
                        BossProjectile{.position = launchPos,
                                      .velocity = Vector2Scale(dir, krakenLimbHurlSpeed),
                                      .radius = 22,
                                      .homing = false,
                                      .active = true,
                                      .fromPlayer = false,
                                      .damage = krakenLimbHurlDamage,
                                      .health = 0,
                                      .isMeteor = false,
                                      .visualEnemyKind = boss.limbGrabbedKind[i]});
                    boss.limbThrowPhase[i] = 0;
                    boss.limbGrabbedKind[i] = -1;
                }
            }
            else if (boss.limbGrabAnimTimer[i] > 0)
            {
                boss.limbGrabAnimTimer[i] -= deltaTime;
                if (boss.limbGrabAnimTimer[i] <= 0)
                {
                    boss.limbThrowPhase[i] = 1;
                    boss.limbThrowTimer[i] = krakenLimbLeanBackDuration;
                }
            }
            else
            {
                boss.limbGrabTimer[i] -= deltaTime;
                if (boss.limbGrabTimer[i] <= 0)
                {
                    boss.limbGrabTimer[i] = krakenLimbGrabInterval;
                    const Vector2 podPos = krakenPodPosition(boss, boss.limbPod[i]);
                    const Vector2 outward =
                        Vector2Normalize(Vector2Subtract(podPos, Vector2{.x = boss.position.x + boss.size.x / 2,
                                                                        .y = boss.position.y + boss.size.y / 2}));
                    boss.limbPortalPos[i] = Vector2Add(podPos, Vector2Scale(outward, krakenLimbPortalOffset));
                    boss.limbGrabAnimTimer[i] = krakenLimbGrabAnimDuration;
                    boss.limbGrabbedKind[i] = GetRandomValue(0, static_cast<int32_t>(enemyKinds.size()) - 1);
                }
            }

            if (boss.limbTimer[i] <= 0 && boss.limbThrowPhase[i] == 0)
            {
                boss.limbPhase[i] = 3;
                boss.limbTimer[i] = krakenLimbRetreatDuration;
            }
            continue;
        }

        if (boss.limbPhase[i] == 3)
        {
            boss.limbTimer[i] -= deltaTime;
            if (boss.limbTimer[i] <= 0)
            {
                boss.limbPhase[i] = 0;
                boss.limbPod[i] = -1;
                boss.limbCooldown[i] = krakenLimbCooldownDuration;
            }
        }
    }
}

void updateWreckwormSegmentVolley(Game& game, Boss& segment, float deltaTime)
{
    if (!segment.isWreckwormSegment || segment.isArmorSegment || segment.health <= 0)
    {
        return;
    }

    segment.segmentVolleyTimer -= deltaTime;
    if (segment.segmentVolleyTimer > 0)
    {
        return;
    }
    segment.segmentVolleyTimer =
        static_cast<float>(GetRandomValue(
            static_cast<int32_t>(wreckwormSegmentVolleyIntervalMin * 10),
            static_cast<int32_t>(wreckwormSegmentVolleyIntervalMax * 10))) /
        10.0F;

    const Vector2 segCenter{.x = segment.position.x + segment.size.x / 2,
                            .y = segment.position.y + segment.size.y / 2};
    const Vector2 direction =
        Vector2Normalize(Vector2Subtract(game.run.player.position, segCenter));
    game.run.bossProjectiles.push_back(BossProjectile{
        .position = segCenter,
        .velocity = Vector2Scale(direction, wreckwormSegmentVolleyProjSpeed),
        .radius = 5,
        .homing = false,
        .active = true,
        .fromPlayer = false,
        .damage = std::max(1, static_cast<int32_t>(static_cast<float>(enemyDamage(
                                   game, crossfireProjectileDamage)) *
                                   wreckwormSegmentVolleyDamageMult)),
        .health = 1});
    playSFX(game, game.resources.sounds.spreadBurst);
}

void triggerWreckwormMolt(Game& game, Boss& head)
{
    constexpr int32_t moltShedCount = 2;
    int32_t shed = 0;
    for (int32_t i = head.segmentCount - 1; i >= 0 && shed < moltShedCount; i--)
    {
        Boss* seg = findWreckwormSegment(game, head.instanceId, i);
        if (seg != nullptr)
        {
            const Vector2 segCenter{.x = seg->position.x + seg->size.x / 2,
                                    .y = seg->position.y + seg->size.y / 2};
            game.run.asteroids.push_back(Asteroid{.position = segCenter,
                                                  .velocity = Vector2{},
                                                  .radius = asteroidRadius(AsteroidTier::Medium),
                                                  .tier = AsteroidTier::Medium,
                                                  .active = true});
            seg->health = 0;
            shed++;
        }
    }

    head.wreckwormSpeedMult += wreckwormMoltSpeedBoost;
    head.recoveryTimer = wreckwormExposedLullDuration;
    head.attackTimer = wreckwormExposedLullDuration;
    head.state = BossState::IDLE;
}

void applyElementDebuff(Boss& boss, ElementType element, float burnDps)
{
    switch (element)
    {
    case ElementType::Static:
    case ElementType::Freeze:
        break;
    case ElementType::Burn:
        if (boss.burnDps <= 0)
        {
            boss.burnDps = burnDps;
        }
        break;
    case ElementType::Count:
        break;
    }
}

void applyActiveElementalDebuffs(Game& game, Boss& boss)
{
    const float burnDps = currentBurnDps(game);
    for (size_t e = 0; e < static_cast<size_t>(ElementType::Count); e++)
    {
        if (game.run.player.elementalBuffTimer.at(e) > 0)
        {
            applyElementDebuff(boss, static_cast<ElementType>(e), burnDps);
        }
    }
}

void damageBoss(Game& game, Boss& boss, int32_t amount, bool applyShake, bool piercesTailWrap)
{
    if (boss.isBanished)
    {
        // ponytail: the eye only takes damage from the dedicated shield-dash/nerve burst
        // (damageBanishedEyeBurst) so it can only be hit once per exposure; incidental
        // weapon fire just flashes without draining health.
        if (boss.banishedStage == 1)
        {
            boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
        }
        return;
    }

    applyActiveElementalDebuffs(game, boss);

    if (boss.isBeltbreaker &&
        (boss.beltbreakerShielded || countAttachedAlivePlates(game, boss) > 0))
    {
        boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
        return;
    }

    if (boss.isWreckwormHead && boss.state == BossState::SHOOTING &&
        boss.attack == BossAttack::TailWrap && !piercesTailWrap && boss.segmentCount > 0 &&
        boss.plateCount * 2 >= boss.segmentCount)
    {
        boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
        return;
    }

    if (boss.isWreckwormSegment && boss.isArmorSegment)
    {
        amount = std::max(
            1, static_cast<int32_t>(static_cast<float>(amount) * wreckwormArmorDamageMult));
    }

    boss.health -= amount;
    boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
    spawnDamageNumber(game, boss.position, amount);

    if (applyShake)
    {
        triggerShake(game, damageShakeIntensity(amount), damageShakeDuration);
    }

    constexpr float heavyHitFraction = 0.05F;
    constexpr float heavyHitPause = 0.1F;
    if (boss.maxHealth > 0 &&
        static_cast<float>(amount) / static_cast<float>(boss.maxHealth) >= heavyHitFraction)
    {
        triggerHitPause(game, heavyHitPause);
    }

    if (boss.isWreckwormHead && !boss.moltThresholds.empty() && boss.maxHealth > 0)
    {
        const float frac = static_cast<float>(boss.health) / static_cast<float>(boss.maxHealth);
        if (frac <= boss.moltThresholds.back())
        {
            boss.moltThresholds.pop_back();
            triggerWreckwormMolt(game, boss);
        }
    }
}

void updateBoss(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
{

    if (boss.debuffStatic)
    {
        return;
    }

    if (boss.isBeltbreakerPlate && boss.plateAttached)
    {
        return;
    }

    if (boss.isWreckwormSegment)
    {
        return;
    }

    const bool enraged =
        boss.health > 0 &&
        static_cast<float>(boss.health) <= static_cast<float>(boss.maxHealth) * enrageHealthFrac;
    const float rateMult = enraged ? enrageSpeedMult : 1.0F;

    switch (boss.state)
    {
    case BossState::IDLE:
        boss.attackTimer -= deltaTime;

        if (boss.attackTimer <= 0 && boss.health > 0)
        {
            boss.attack = boss.moveset.at(static_cast<size_t>(
                GetRandomValue(0, static_cast<int32_t>(boss.moveset.size()) - 1)));
            boss.state = BossState::WINDING_UP;
            boss.stateTimer = bossWindupDuration(boss.attack) * rateMult;
            boss.beamShieldLatched = false;

            switch (boss.attack)
            {
            case BossAttack::Spread:
                boss.color = Palette::BossSpread;
                boss.spreadWindupShots = 0;
                break;
            case BossAttack::Slam:
                boss.color = Palette::Accent;
                break;
            case BossAttack::Beam:
                boss.color = Palette::BossBeam;
                break;
            case BossAttack::WormholeBeam:
                boss.color = Palette::Shield;
                break;
            case BossAttack::ChargeDash:
                boss.color = Palette::Crit;
                break;
            case BossAttack::SummonAdds:
                boss.color = Palette::Charge;
                break;
            case BossAttack::ShockwaveStomp:
                boss.color = Palette::Haze;
                break;
            case BossAttack::Barrage:
                boss.color = Palette::BossSpread;
                break;
            case BossAttack::GravityWell:
                boss.color = Palette::StructMid;
                break;
            case BossAttack::HomingBarrage:
                boss.color = Palette::BossHoming;
                break;
            case BossAttack::PlateHurl:
                boss.color = Palette::Shield;
                break;
            case BossAttack::BurrowCharge:
            {
                boss.color = Palette::RustbloomHaze;

                const float burrowAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
                const auto burrowDist = static_cast<float>(GetRandomValue(550, 700));
                const Vector2 entry = Vector2Add(game.run.player.position,
                                                 Vector2{.x = std::cos(burrowAngle) * burrowDist,
                                                         .y = std::sin(burrowAngle) * burrowDist});
                boss.position =
                    Vector2Subtract(entry, Vector2{.x = boss.size.x / 2, .y = boss.size.y / 2});
                if (boss.isWreckwormHead)
                {
                    const Vector2 entryTrailDir = Vector2Normalize(
                        Vector2Subtract(entry, game.run.player.position));
                    seedWreckwormTrail(boss, entry, entryTrailDir);
                }
                break;
            }
            case BossAttack::CoilClamp:
                boss.color = Palette::RustbloomAccent;
                break;
            case BossAttack::MeteorHell:
            case BossAttack::MeteorSwarm:
                boss.color = Palette::SolarForgeAccent;
                break;
            case BossAttack::DashRush:
            case BossAttack::TailWrap:
                boss.color = Palette::Crit;
                break;
            case BossAttack::CloudBreath:
                boss.color = Palette::RustbloomAccent;
                break;
            case BossAttack::Count:
                break;
            }

            playSFX(game, game.resources.sounds.bossWindUp);
        }
        break;
    case BossState::WINDING_UP:
        boss.stateTimer -= deltaTime;
        boss.targetPosition = game.run.player.position;

        if (boss.stateTimer <= 0)
        {
            startBossAttack(game, boss, bossCenter);
        }
        break;
    case BossState::SHOOTING:
        boss.stateTimer -= deltaTime;

        if (boss.attack == BossAttack::Slam)
        {
            const float progress =
                std::clamp(1 - boss.stateTimer / UpdateConstants::slamDuration, 0.0F, 1.0F);
            const float radius = bossConstants::maxSlamRadius * progress;

            for (auto& asteroid : game.run.asteroids)
            {
                if (asteroid.active &&
                    Vector2Distance(bossCenter, asteroid.position) <= radius + asteroid.radius)
                {
                    asteroid.active = false;
                    breakAsteroid(game, asteroid);
                }
            }

            for (size_t i = 0; i < game.run.enemies.size(); i++)
            {
                const auto& e = game.run.enemies.at(i);
                if (e.active && !e.phased &&
                    Vector2Distance(bossCenter, e.position) <=
                        radius + enemyCollisionRadius(e))
                {
                    killEnemyForBossAttack(game, i, false);
                }
            }

            if (!boss.slamHit)
            {
                if (const auto result = resolveExpandingWaveHit(game, bossCenter, radius, true);
                    result.resolved)
                {
                    boss.slamHit = true;
                    if (result.hit)
                    {
                        damagePlayer(game, enemyDamage(game, 1));
                    }
                }
            }
        }

        if (boss.attack == BossAttack::Spread)
        {
            boss.barrageTimer -= deltaTime;
            if (boss.barrageTimer <= 0 && boss.spreadWindupShots < spreadRounds)
            {
                boss.barrageTimer = spreadRoundInterval;
                boss.spreadWindupShots++;
                playSFX(game, game.resources.sounds.spreadBurst);
                triggerShake(game, 5, 0.2F);

                const Vector2 aimDir =
                    Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));
                const float baseAngle = std::atan2(aimDir.y, aimDir.x);
                const auto roundProjectileHealth = static_cast<int32_t>(
                    std::max(1.0F, static_cast<float>(baseProjectileHealth) * enemyHealthMult *
                                       enemyHealthScale(game)));
                constexpr int32_t half = spreadBulletsPerRound / 2;
                for (int32_t i = -half; i < spreadBulletsPerRound - half; i++)
                {
                    const float angle = baseAngle + static_cast<float>(i) * 15 * DEG2RAD;
                    const Vector2 vel{.x = std::cos(angle) * spreadProjSpeed,
                                      .y = std::sin(angle) * spreadProjSpeed};

                    game.run.bossProjectiles.push_back(
                        BossProjectile{.position = bossCenter,
                                       .velocity = vel,
                                       .radius = 7,
                                       .homing = false,
                                       .active = true,
                                       .fromPlayer = false,
                                       .damage = enemyDamage(game, crossfireProjectileDamage),
                                       .health = roundProjectileHealth});
                }
            }
        }

        if (boss.attack == BossAttack::MeteorHell)
        {
            boss.barrageTimer -= deltaTime;
            if (boss.barrageTimer <= 0)
            {
                boss.barrageTimer = meteorHellSpawnInterval;
                playSFX(game, game.resources.sounds.spreadBurst);
                triggerShake(game, 5, 0.2F);

                const auto roundProjectileHealth = static_cast<int32_t>(
                    std::max(1.0F, static_cast<float>(baseProjectileHealth) * meteorHellHealthMult *
                                       enemyHealthMult * enemyHealthScale(game)));
                const int32_t spawnCount =
                    GetRandomValue(meteorHellSpawnPerBatchMin, meteorHellSpawnPerBatchMax);
                for (int32_t i = 0; i < spawnCount; i++)
                {

                    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
                    const auto dist = static_cast<float>(
                        GetRandomValue(static_cast<int32_t>(meteorHellSpawnDistMin),
                                       static_cast<int32_t>(meteorHellSpawnDistMax)));
                    const Vector2 spawnPos = Vector2Add(
                        game.run.player.position,
                        Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
                    const Vector2 vel = Vector2Scale(
                        Vector2Normalize(Vector2Subtract(game.run.player.position, spawnPos)),
                        meteorHellProjSpeed);

                    game.run.bossProjectiles.push_back(
                        BossProjectile{.position = spawnPos,
                                       .velocity = vel,
                                       .radius = 10,
                                       .homing = false,
                                       .active = true,
                                       .fromPlayer = false,
                                       .damage = enemyDamage(game, crossfireProjectileDamage),
                                       .health = roundProjectileHealth,
                                       .isMeteor = true});
                }
            }
        }

        if (boss.attack == BossAttack::Beam)
        {
            const Vector2 direction =
                Vector2Normalize(Vector2Subtract(boss.targetPosition, bossCenter));
            const Vector2 beamEnd = Vector2Add(bossCenter, Vector2Scale(direction, 2000));
            for (const auto& [segStart, segEnd] :
                 wormholeBentBeamSegments(game, bossCenter, beamEnd))
            {
                processBeamAttack(game, boss, segStart, segEnd);
            }
        }

        if (boss.attack == BossAttack::WormholeBeam)
        {
            const Vector2 direction =
                Vector2Normalize(Vector2Subtract(boss.targetPosition, boss.wormholeBeamOrigin));
            const Vector2 beamEnd =
                Vector2Add(boss.wormholeBeamOrigin, Vector2Scale(direction, 2000));
            for (const auto& [segStart, segEnd] :
                 wormholeBentBeamSegments(game, boss.wormholeBeamOrigin, beamEnd))
            {
                processBeamAttack(game, boss, segStart, segEnd);
            }
        }

        if (boss.attack == BossAttack::ChargeDash)
        {
            boss.position = Vector2Add(boss.position, Vector2Scale(boss.chargeVelocity, deltaTime));
        }

        if (boss.attack == BossAttack::DashRush)
        {
            boss.barrageTimer -= deltaTime;
            if (boss.slamHit)
            {
                boss.position =
                    Vector2Add(boss.position, Vector2Scale(boss.chargeVelocity, deltaTime));
                if (boss.barrageTimer <= 0)
                {
                    boss.slamHit = false;
                    boss.chargeVelocity = Vector2{};
                    boss.barrageTimer = wreckwormDashRushGapDuration;
                }
            }
            else if (boss.wreckwormRepositioning)
            {
                boss.position =
                    Vector2Add(boss.position, Vector2Scale(boss.chargeVelocity, deltaTime));
                if (boss.barrageTimer <= 0)
                {
                    boss.wreckwormRepositioning = false;
                    const Vector2 launchCenter{.x = boss.position.x + boss.size.x / 2,
                                               .y = boss.position.y + boss.size.y / 2};
                    const Vector2 aimDir = Vector2Normalize(
                        Vector2Subtract(game.run.player.position, launchCenter));
                    boss.chargeVelocity =
                        Vector2Scale(aimDir, chargeDashSpeed * wreckwormDashSpeedMult);
                    boss.slamHit = true;
                    boss.spreadWindupShots++;
                    boss.barrageTimer = wreckwormDashRushTravelDuration(game, boss.segmentCount);
                    playSFX(game, game.resources.sounds.bossWindUp);
                    triggerShake(game, 6, 0.2F);
                }
            }
            else if (boss.barrageTimer <= 0 &&
                     boss.spreadWindupShots < wreckwormDashRushCount(game.run.waveNumber))
            {
                const float offDist = wreckwormDashRushOffDist(game);
                const float targetAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
                boss.targetPosition = Vector2Add(
                    game.run.player.position,
                    Vector2{.x = std::cos(targetAngle) * offDist,
                            .y = std::sin(targetAngle) * offDist});

                const Vector2 headCenter{.x = boss.position.x + boss.size.x / 2,
                                         .y = boss.position.y + boss.size.y / 2};
                const Vector2 toTarget = Vector2Subtract(boss.targetPosition, headCenter);
                const float travelDist = Vector2Length(toTarget);
                boss.chargeVelocity = travelDist > 0
                                          ? Vector2Scale(toTarget, wreckwormDashRepositionSpeed / travelDist)
                                          : Vector2{};
                boss.barrageTimer = travelDist / wreckwormDashRepositionSpeed;
                boss.wreckwormRepositioning = true;
            }
        }

        if (boss.attack == BossAttack::Barrage)
        {
            boss.barrageTimer -= deltaTime;
            if (boss.barrageTimer <= 0)
            {

                const float attackScale =
                    boss.isBeltbreakerPlate ? beltbreakerPlateAttackScale : 1.0F;
                boss.barrageTimer = barrageFireInterval / attackScale;
                const Vector2 direction =
                    Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));

                const bool isWreckworm = boss.isWreckwormHead || boss.isWreckwormSegment;
                constexpr std::array<Color, 3> fluidColors{Palette::RustbloomAccent, Palette::Charge,
                                                            Palette::Accent};
                game.run.bossProjectiles.push_back(BossProjectile{
                    .position = bossCenter,
                    .velocity = Vector2Scale(direction, barrageProjSpeed * attackScale),
                    .radius = 6,
                    .homing = false,
                    .active = true,
                    .fromPlayer = false,
                    .damage = std::max(
                        1, static_cast<int32_t>(
                               static_cast<float>(enemyDamage(game, crossfireProjectileDamage)) *
                               attackScale)),
                    .health = 1,
                    .isMeteor = boss.isSlagmaw,
                    .isFluid = isWreckworm,
                    .fluidColor = fluidColors.at(static_cast<size_t>(GetRandomValue(0, 2)))});
                playSFX(game, game.resources.sounds.spreadBurst);
            }
        }

        if (boss.attack == BossAttack::HomingBarrage)
        {
            boss.barrageTimer -= deltaTime;
            if (boss.barrageTimer <= 0)
            {
                boss.barrageTimer = homingBarrageFireInterval;
                const Vector2 direction =
                    Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));
                game.run.bossProjectiles.push_back(
                    BossProjectile{.position = bossCenter,
                                   .velocity = Vector2Scale(direction, homingProjSpeed),
                                   .radius = 6,
                                   .homing = true,
                                   .active = true,
                                   .fromPlayer = false,
                                   .damage = enemyDamage(game, crossfireProjectileDamage),
                                   .health = 1});
                playSFX(game, game.resources.sounds.spreadBurst);
            }
        }

        if (boss.attack == BossAttack::GravityWell)
        {
            const Vector2 toBoss = Vector2Subtract(bossCenter, game.run.player.position);
            if (Vector2Length(toBoss) > 0)
            {

                const Vector2 pull = Vector2Scale(Vector2Normalize(toBoss),
                                                  game.run.player.speed * gravityWellPullFraction);
                game.run.player.position = Vector2Add(game.run.player.position,
                                                      Vector2Scale(pull, deltaTime * frameScale));
            }

            if (boss.isBeltbreaker)
            {
                boss.plateAngleDeg =
                    std::fmod(boss.plateAngleDeg +
                                  beltbreakerRotationSpeed * gravityWellFanSpinMult * deltaTime,
                              360.0F);
            }
        }

        if (boss.attack == BossAttack::PlateHurl)
        {

            for (auto& plate : game.run.bosses)
            {
                if (!plate.isBeltbreakerPlate || plate.plateOwnerId != boss.instanceId ||
                    !plate.plateAttached || plate.health < plate.maxHealth)
                {
                    continue;
                }
                const Vector2 plateCenter{.x = plate.position.x + plate.size.x / 2,
                                          .y = plate.position.y + plate.size.y / 2};
                const Vector2 dir =
                    Vector2Normalize(Vector2Subtract(game.run.player.position, plateCenter));
                sendPlateOnExcursion(plate, beltbreakerPlateHurlDuration);
                plate.state = BossState::SHOOTING;
                plate.attack = BossAttack::ChargeDash;
                plate.stateTimer = chargeDashDuration;
                plate.chargeVelocity = Vector2Scale(dir, chargeDashSpeed);
            }
        }

        if (boss.attack == BossAttack::BurrowCharge)
        {
            boss.position = Vector2Add(boss.position, Vector2Scale(boss.chargeVelocity, deltaTime));
        }

        if (boss.attack == BossAttack::CoilClamp)
        {
            const float elapsed = wreckwormCoilClampTotalDuration - boss.stateTimer;
            const float holdElapsed = std::clamp(elapsed - wreckwormCoilClampAssembleDuration, 0.0F,
                                                 wreckwormCoilClampDuration);
            const float progress = holdElapsed / wreckwormCoilClampDuration;
            const float radius =
                wreckwormCoilClampStartRadius +
                (wreckwormCoilClampEndRadius - wreckwormCoilClampStartRadius) * progress;
            const float arcDeg = 360.0F - wreckwormCoilClampGapDeg;
            const float releaseStart = wreckwormCoilClampAssembleDuration + wreckwormCoilClampDuration;

            for (int32_t i = 0; i < boss.segmentCount; i++)
            {
                Boss* seg = findWreckwormSegment(game, boss.instanceId, i);
                if (seg == nullptr)
                {
                    continue;
                }

                const int32_t group = i * 3 / boss.segmentCount;
                const float assembleTime =
                    static_cast<float>(group) * (wreckwormCoilClampAssembleDuration / 3.0F);
                const float releaseTime = releaseStart + static_cast<float>(group) *
                                                              (wreckwormCoilClampReleaseDuration / 3.0F);
                const bool shouldBeAssembled = elapsed >= assembleTime && elapsed < releaseTime;

                if (shouldBeAssembled != seg->wreckwormDetached)
                {
                    seg->hitFlashTimer = UpdateConstants::hitFlashDuration;
                    seg->wreckwormDetached = shouldBeAssembled;
                }

                if (shouldBeAssembled)
                {
                    const float t = boss.segmentCount > 1
                                        ? static_cast<float>(i) /
                                              static_cast<float>(boss.segmentCount - 1)
                                        : 0.0F;
                    const float deg =
                        boss.coilClampGapDeg + wreckwormCoilClampGapDeg / 2.0F + t * arcDeg;
                    const Vector2 ringCenter = Vector2Add(
                        boss.targetPosition, Vector2{.x = std::cos(deg * DEG2RAD) * radius,
                                                     .y = std::sin(deg * DEG2RAD) * radius});
                    seg->position = Vector2Subtract(
                        ringCenter, Vector2{.x = seg->size.x / 2, .y = seg->size.y / 2});
                }
                else
                {
                    const Vector2 chainTarget = pointAlongWreckwormTrail(
                        bossCenter, boss.wreckwormTrail,
                        static_cast<float>(seg->segmentIndex + 1) * wreckwormSegmentSpacing);
                    seg->position = Vector2Subtract(
                        chainTarget, Vector2{.x = seg->size.x / 2, .y = seg->size.y / 2});
                }
            }
        }

        if (boss.attack == BossAttack::TailWrap)
        {
            const float elapsed = wreckwormTailWrapDuration - boss.stateTimer;
            const bool returningIn = boss.stateTimer < wreckwormTailWrapReleaseDuration;
            const bool wrapInPhase = !returningIn && elapsed < wreckwormTailWrapCoilInDuration;
            const bool chasePhase = !returningIn && !wrapInPhase;

            if (chasePhase)
            {
                const Vector2 toPlayer = Vector2Subtract(game.run.player.position, bossCenter);
                if (Vector2Length(toPlayer) > 1)
                {
                    boss.position = Vector2Add(
                        boss.position, Vector2Scale(Vector2Normalize(toPlayer),
                                                    wreckwormTailWrapMoveSpeed * deltaTime));
                }
            }

            const float spinPhaseElapsed = std::max(0.0F, elapsed - wreckwormTailWrapCoilInDuration);
            const float spinAngle =
                std::fmod(spinPhaseElapsed * wreckwormTailWrapSpinSpeedDeg, 360.0F);

            int32_t aliveCount = 0;
            for (int32_t i = 0; i < boss.segmentCount; i++)
            {
                Boss* seg = findWreckwormSegment(game, boss.instanceId, i);
                if (seg == nullptr)
                {
                    continue;
                }
                aliveCount++;

                if (returningIn)
                {
                    const Vector2 chainTarget = pointAlongWreckwormTrail(
                        bossCenter, boss.wreckwormTrail,
                        static_cast<float>(seg->segmentIndex + 1) * wreckwormSegmentSpacing);
                    moveWreckwormSegmentToward(*seg, chainTarget, wreckwormDetachSpeed, deltaTime);
                }
                else
                {
                    seg->wreckwormDetached = true;

                    const float targetDeg = std::fmod(
                        spinAngle + static_cast<float>(i) * (360.0F / static_cast<float>(boss.segmentCount)),
                        360.0F);
                    float angleDiff = std::fmod(targetDeg - seg->plateAngleDeg, 360.0F);
                    if (angleDiff < 0)
                    {
                        angleDiff += 360.0F;
                    }
                    const float step = std::min(angleDiff, wreckwormTailWrapCoilSpeedDeg * deltaTime);
                    seg->plateAngleDeg = std::fmod(seg->plateAngleDeg + step, 360.0F);

                    const Vector2 ringCenter = Vector2Add(
                        bossCenter,
                        Vector2{.x = std::cos(seg->plateAngleDeg * DEG2RAD) * wreckwormTailWrapRadius,
                               .y = std::sin(seg->plateAngleDeg * DEG2RAD) * wreckwormTailWrapRadius});
                    seg->position = Vector2Subtract(
                        ringCenter, Vector2{.x = seg->size.x / 2, .y = seg->size.y / 2});
                }
            }
            boss.plateCount = aliveCount;
        }

        if (boss.stateTimer <= 0)
        {
            if (boss.isWreckwormHead &&
                (boss.attack == BossAttack::CoilClamp || boss.attack == BossAttack::TailWrap))
            {
                for (auto& seg : game.run.bosses)
                {
                    if (seg.isWreckwormSegment && seg.segmentOwnerId == boss.instanceId)
                    {
                        seg.wreckwormDetached = false;
                    }
                }
            }
            boss.state = BossState::IDLE;
            boss.attackTimer = static_cast<float>(GetRandomValue(15, 40)) / 10.0F * rateMult;
            boss.color = boss.baseColor;
            boss.recoveryTimer = bossRecoveryDuration;
        }
        break;
    }
}

auto bossWindupDuration(BossAttack attack) -> float
{
    switch (attack)
    {
    case BossAttack::Slam:
        return 1.3F;
    case BossAttack::WormholeBeam:
        return 1.4F;
    case BossAttack::ChargeDash:
        return 1.2F;
    case BossAttack::MeteorHell:
        return 1.4F;
    case BossAttack::DashRush:
        return 1.6F;
    case BossAttack::TailWrap:
        return 1.3F;
    default:
        return 1.0F;
    }
}

void processBeamAttack(Game& game, Boss& boss, Vector2 beamStart, Vector2 beamEnd)
{
    for (auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active &&
            CheckCollisionCircleLine(asteroid.position, asteroid.radius, beamStart, beamEnd))
        {
            asteroid.active = false;
            breakAsteroid(game, asteroid);
        }
    }

    for (size_t i = 0; i < game.run.enemies.size(); i++)
    {
        const auto& e = game.run.enemies.at(i);
        if (e.active && !e.phased &&
            CheckCollisionCircleLine(e.position, enemyCollisionRadius(e),
                                     beamStart, beamEnd))
        {
            killEnemyForBossAttack(game, i, false);
        }
    }

    if (game.run.player.health <= 0 || game.run.player.dashing)
    {
        return;
    }

    if (!CheckCollisionCircleLine(game.run.player.position, game.run.player.radius, beamStart,
                                  beamEnd))
    {
        return;
    }

    if (game.run.player.shieldActive)
    {
        boss.beamShieldLatched = true;
        if (boss.parryStunTimer <= 0)
        {
            boss.parryStunTimer = parryStunDuration;
            tryComboRefund(game);
        }
        return;
    }

    if (boss.beamShieldLatched)
    {
        boss.beamShieldLatched = false;
        damagePlayer(game, instakillDamage);
        return;
    }

    if (game.run.player.immunityTimer <= 0)
    {
        damagePlayer(game, enemyDamage(game, 1));
    }
}

void forceBossAttack(Game& game, Boss& boss, BossAttack attack)
{
    boss.attack = attack;
    boss.state = BossState::WINDING_UP;
    boss.stateTimer = bossWindupDuration(attack);
    boss.beamShieldLatched = false;

    switch (attack)
    {
    case BossAttack::Spread:
        boss.color = Palette::BossSpread;
        boss.spreadWindupShots = 0;
        break;
    case BossAttack::Slam:
        boss.color = Palette::Accent;
        break;
    case BossAttack::Beam:
        boss.color = Palette::BossBeam;
        break;
    case BossAttack::WormholeBeam:
        boss.color = Palette::Shield;
        break;
    case BossAttack::ChargeDash:
        boss.color = Palette::Crit;
        break;
    case BossAttack::SummonAdds:
        boss.color = Palette::Charge;
        break;
    case BossAttack::ShockwaveStomp:
        boss.color = Palette::Haze;
        break;
    case BossAttack::Barrage:
        boss.color = Palette::BossSpread;
        break;
    case BossAttack::GravityWell:
        boss.color = Palette::StructMid;
        break;
    case BossAttack::HomingBarrage:
        boss.color = Palette::BossHoming;
        break;
    case BossAttack::PlateHurl:
        boss.color = Palette::Shield;
        break;
    case BossAttack::BurrowCharge:
        boss.color = Palette::RustbloomHaze;
        break;
    case BossAttack::CoilClamp:
        boss.color = Palette::RustbloomAccent;
        break;
    case BossAttack::MeteorHell:
    case BossAttack::MeteorSwarm:
        boss.color = Palette::SolarForgeAccent;
        break;
    case BossAttack::DashRush:
    case BossAttack::TailWrap:
        boss.color = Palette::Crit;
        break;
    case BossAttack::CloudBreath:
        boss.color = Palette::RustbloomAccent;
        break;
    case BossAttack::Count:
        break;
    }

    playSFX(game, game.resources.sounds.bossWindUp);
}

void startBossAttack(Game& game, Boss& boss, Vector2 bossCenter)
{
    const Vector2 direction = Vector2Subtract(boss.targetPosition, bossCenter);
    const Vector2 aimDirection = Vector2Normalize(direction);

    boss.state = BossState::SHOOTING;

    switch (boss.attack)
    {
    case BossAttack::Beam:
        boss.stateTimer = beamAttackDuration;
        playSFX(game, game.resources.sounds.beamFire);
        triggerShake(game, 5, 0.2F);
        break;
    case BossAttack::Spread:
    {
        boss.stateTimer = static_cast<float>(spreadRounds) * spreadRoundInterval + 0.2F;
        boss.barrageTimer = 0;
        boss.spreadWindupShots = 0;
        break;
    }
    case BossAttack::Slam:
        boss.stateTimer = UpdateConstants::slamDuration;
        boss.slamHit = false;
        playSFX(game, game.resources.sounds.slamBoom);
        duckBGM(game);
        triggerShake(game, 10, 0.4F);
        break;
    case BossAttack::WormholeBeam:
    {
        boss.stateTimer = wormholeBeamDuration;
        boss.beamShieldLatched = false;
        const float flankAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto flankDist =
            static_cast<float>(GetRandomValue(static_cast<int32_t>(wormholeBeamFlankMinDist),
                                              static_cast<int32_t>(wormholeBeamFlankMaxDist)));
        boss.wormholeBeamOrigin =
            Vector2Add(game.run.player.position, Vector2{.x = std::cos(flankAngle) * flankDist,
                                                         .y = std::sin(flankAngle) * flankDist});
        playSFX(game, game.resources.sounds.beamFire);
        triggerShake(game, 6, 0.25F);
        break;
    }
    case BossAttack::ChargeDash:
        boss.stateTimer = chargeDashDuration;
        boss.chargeVelocity = Vector2Scale(
            aimDirection,
            chargeDashSpeed * (boss.isBeltbreakerPlate ? beltbreakerPlateAttackScale : 1.0F));
        playSFX(game, game.resources.sounds.bossWindUp);
        triggerShake(game, 8, 0.3F);
        break;
    case BossAttack::SummonAdds:
    {
        boss.stateTimer = 0.6F;
        playSFX(game, game.resources.sounds.spreadBurst);
        for (int i = 0; i < summonAddsCount; i++)
        {
            spawnEnemy(game);
        }
        break;
    }
    case BossAttack::ShockwaveStomp:
        boss.stateTimer = UpdateConstants::shockwaveStompDuration;
        playSFX(game, game.resources.sounds.slamBoom);
        duckBGM(game);
        triggerShake(game, 10, 0.35F);
        for (auto& asteroid : game.run.asteroids)
        {
            if (asteroid.active && Vector2Distance(bossCenter, asteroid.position) <=
                                       UpdateConstants::shockwaveStompRadius + asteroid.radius)
            {
                asteroid.active = false;
                breakAsteroid(game, asteroid);
            }
        }
        for (size_t i = 0; i < game.run.enemies.size(); i++)
        {
            const auto& e = game.run.enemies.at(i);
            if (e.active && !e.phased &&
                Vector2Distance(bossCenter, e.position) <=
                    UpdateConstants::shockwaveStompRadius +
                        enemyCollisionRadius(e))
            {
                killEnemyForBossAttack(game, i, false);
            }
        }
        if (game.run.player.health > 0 && !game.run.player.shieldActive &&
            !game.run.player.dashing &&
            Vector2Distance(bossCenter, game.run.player.position) <=
                UpdateConstants::shockwaveStompRadius + game.run.player.radius)
        {
            damagePlayer(game, enemyDamage(game, 2));
        }
        break;
    case BossAttack::Barrage:
        boss.stateTimer = barrageDuration;
        boss.barrageTimer = 0;
        playSFX(game, game.resources.sounds.spreadBurst);
        break;
    case BossAttack::GravityWell:
        boss.stateTimer = gravityWellDuration;
        playSFX(game, game.resources.sounds.bossWindUp);
        break;
    case BossAttack::HomingBarrage:
        boss.stateTimer = barrageDuration;
        boss.barrageTimer = 0;
        playSFX(game, game.resources.sounds.homingLaunch);
        break;
    case BossAttack::PlateHurl:
        boss.stateTimer = beltbreakerPlateHurlDuration;
        playSFX(game, game.resources.sounds.bossWindUp);
        triggerShake(game, 6, 0.25F);
        break;
    case BossAttack::BurrowCharge:
        boss.stateTimer = wreckwormBurrowChargeDuration;
        boss.chargeVelocity = Vector2Scale(aimDirection, chargeDashSpeed);
        playSFX(game, game.resources.sounds.bossWindUp);
        triggerShake(game, 8, 0.3F);
        break;
    case BossAttack::CoilClamp:
    {
        boss.stateTimer = wreckwormCoilClampTotalDuration;
        boss.coilClampGapDeg = static_cast<float>(GetRandomValue(0, 359));
        for (int32_t i = 0; i < boss.segmentCount; i++)
        {
            Boss* seg = findWreckwormSegment(game, boss.instanceId, i);
            if (seg != nullptr)
            {
                seg->wreckwormDetached = false;
            }
        }
        playSFX(game, game.resources.sounds.bossWindUp);
        break;
    }
    case BossAttack::MeteorHell:
        boss.stateTimer = meteorHellDuration(game.run.waveNumber);
        boss.barrageTimer = 0;
        playSFX(game, game.resources.sounds.spreadBurst);
        triggerShake(game, 8, 0.3F);
        break;
    case BossAttack::MeteorSwarm:
        boss.stateTimer = 0.6F;
        playSFX(game, game.resources.sounds.spreadBurst);
        triggerShake(game, 6, 0.25F);
        for (int32_t i = 0; i < meteorSwarmSpawnCount; i++)
        {
            Vector2 pos = spawnRingPosition(game);
            for (int32_t attempt = 0; attempt < 8 && !isSolarForgeCaveOpen(pos); attempt++)
            {
                pos = spawnRingPosition(game);
            }
            spawnEnemyAt(game, enemyKindMeteorChunk, pos);
        }
        break;
    case BossAttack::DashRush:
    {
        const int32_t dashCount = wreckwormDashRushCount(game.run.waveNumber);
        const float repositionEstimate =
            3.0F * wreckwormDashRushOffDist(game) / wreckwormDashRepositionSpeed;
        boss.stateTimer =
            static_cast<float>(dashCount) *
                (wreckwormDashRushTravelDuration(game, boss.segmentCount) + wreckwormDashRushGapDuration +
                 repositionEstimate) +
            0.5F;
        boss.spreadWindupShots = 0;
        boss.barrageTimer = 0;
        boss.slamHit = false;
        boss.wreckwormRepositioning = false;
        boss.chargeVelocity = Vector2{};
        break;
    }
    case BossAttack::CloudBreath:
    {
        boss.stateTimer = 1.0F;
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        for (int32_t i = 0; i < wreckwormCloudBreathCount; i++)
        {
            const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const auto dist = static_cast<float>(GetRandomValue(
                0, static_cast<int32_t>(wreckwormCloudSpawnSpread)));
            const auto radius = static_cast<float>(GetRandomValue(
                static_cast<int32_t>(wreckwormCloudRadiusMin),
                static_cast<int32_t>(wreckwormCloudRadiusMax)));
            game.run.gasHazards.push_back(GasHazard{
                .position = Vector2Add(
                    bossCenter, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist}),
                .radius = radius,
                .seed = static_cast<float>(GetRandomValue(0, 10000)),
                .life = wreckwormCloudLifespan,
                .isFire = boss.krakenSnakeVariant});
        }
        playSFX(game, game.resources.sounds.spreadBurst);
        break;
    }
    case BossAttack::TailWrap:
    {
        boss.stateTimer = wreckwormTailWrapDuration;
        boss.plateCount = 0;
        for (int32_t i = 0; i < boss.segmentCount; i++)
        {
            Boss* seg = findWreckwormSegment(game, boss.instanceId, i);
            if (seg == nullptr)
            {
                continue;
            }
            const Vector2 segCenter{.x = seg->position.x + seg->size.x / 2,
                                    .y = seg->position.y + seg->size.y / 2};
            const Vector2 rel = Vector2Subtract(segCenter, bossCenter);
            seg->plateAngleDeg = Vector2Length(rel) > 0.01F
                                     ? std::atan2(rel.y, rel.x) * RAD2DEG
                                     : 0.0F;
        }
        playSFX(game, game.resources.sounds.bossWindUp);
        triggerShake(game, 6, 0.25F);
        break;
    }
    case BossAttack::Count:
        break;
    }
}
