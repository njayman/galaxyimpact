#include "update.hpp"

#include "democonfig.hpp"
#include "entities/ship.hpp"
#include "raymath.h"
#include "update_constants.hpp"
#include <algorithm>
#include <cmath>
#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

void seedWreckwormTrail(Boss& head, Vector2 headCenter, Vector2 trailDir);
void beginWreckwormConstrict(Game& game, Boss& head);
void updateSlagmawDevour(Game& game, Boss& slagmaw, float deltaTime);
void endKrakenDraggedUnder(Game& game, Boss& kraken, bool clean);
void beginKrakenDraggedUnder(Game& game, Boss& kraken);
auto wreckwormCoilSegmentDeg(const Boss& head, int32_t segmentIndex) -> float;

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
    case BossAttack::Devour:
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

void spawnBossInstance(Game& game, float healthMult, float sizeMult, bool isMega, bool isSwarm)
{
    const int32_t tier = game.run.waveNumber / megaBossWaveInterval;

    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(1200, 1500));
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

    auto typeIndex =
        static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(bossTypes.size()) - 1));
    // Wave Bosses fighting in a biome draw only from that biome's pool.
    constexpr std::array<std::string_view, 5> beltPool{
        "Shatter Prime", "Fracture Queen", "Ashen Sentinel", "Barrier Colossus", "Warbringer"};
    constexpr std::array<std::string_view, 4> rustPool{
        "The Last Signal", "Hollow Idol", "Dim Reaver", "Static Wraith"};
    constexpr auto isBossType = [](std::string_view n)
    { return std::ranges::find(bossTypes, n, &BossType::name) != bossTypes.end(); };
    static_assert(std::ranges::all_of(beltPool, isBossType));
    static_assert(std::ranges::all_of(rustPool, isBossType));
    const Biome biome = currentBiome(game.run.waveNumber);
    std::span<const std::string_view> pool;
    if (biome == Biome::ShatteredBelt)
    {
        pool = beltPool;
    }
    else if (biome == Biome::Rustbloom)
    {
        pool = rustPool;
    }
    if (!pool.empty())
    {
        const auto name =
            pool[static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(pool.size()) - 1))];
        typeIndex = static_cast<size_t>(
            std::ranges::find(bossTypes, name, &BossType::name) - bossTypes.begin());
    }
    if constexpr (DemoConfig::isDemoBuild)
    {
        typeIndex = DemoConfig::allowedBossTypeIndices.at(static_cast<size_t>(GetRandomValue(
            0, static_cast<int32_t>(DemoConfig::allowedBossTypeIndices.size()) - 1)));
    }
    const auto& type = bossTypes.at(typeIndex);
    Color color = type.color;
    if (biome == Biome::ShatteredBelt)
    {
        color = ColorLerp(type.color, Palette::ShatteredBeltAccent, beltDustTint);
    }
    else if (biome == Biome::Rustbloom)
    {
        color = ColorLerp(type.color, Palette::RustbloomAccent, rustbloomTint);
    }

    const auto health =
        static_cast<int32_t>(static_cast<float>(500 + tier * bossHpPerTier) * healthMult *
                             type.healthMult * enemyHealthMult *
                             enemyHealthScale(game));
    const float size = 100.0F * sizeMult * type.sizeMult;

    auto moveset = sampleBossMoveset(bossMoveCount);
    const BossAttack firstAttack = moveset.front();

    game.run.bosses.push_back(
        Boss{.position = spawnPos,
             .size = Vector2{.x = size, .y = size},
             .color = color,
             .baseColor = color,
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
             // ponytail: demo builds keep the plain loop; the director needs ChargeDash, which
             // the demo's attack allowlist excludes.
             .usesDirector = !DemoConfig::isDemoBuild,
             .patternTimer = 5.0F}); // open with a hover so the player sees it before it dashes
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
        moltThresholds = {0.33F, 0.66F}; // popped from the back: 66% fires first
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

    std::vector<BossAttack> moveset{BossAttack::Barrage,    BossAttack::ChargeDash,
                                    BossAttack::MeteorHell, BossAttack::MeteorSwarm,
                                    BossAttack::Devour};
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
              .health = banishedHealth,
              .maxHealth = banishedHealth,
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
    boss.banishedTimer[static_cast<size_t>(idx)] = UpdateConstants::banishedPlainGrabHold;
    boss.banishedTip[static_cast<size_t>(idx)] = contactPoint;
    player.grabbed = true;
    player.grabbingTentacle = idx;
    player.launched = false;
    player.launchTargetTentacle = -1;
    player.position = contactPoint;
}

// A plain grab (Bind inside the QTE gap): the short hold ends with the ship thrown clear.
void resolveBanishedCoil(Game& game, Boss& boss, int32_t idx)
{
    Player& player = game.run.player;
    player.grabbed = false;
    player.grabbingTentacle = -1;
    damagePlayer(game, enemyDamage(game, banishedThrashDamage));
    boss.banishedGrabbing[static_cast<size_t>(idx)] = false;
    boss.banishedPhase[static_cast<size_t>(idx)] = 3;
    boss.banishedTimer[static_cast<size_t>(idx)] = banishedTentacleRetreat;
}

// --- Banished Juggled QTE (cutscene) -------------------------------------------
//
// Bind's catch: a Mash to break the coil, then prompts across three throws, each throw
// handing the ship to another tentacle. Clean clear: the eye snaps open and the finisher
// beam is one eye burst. Fail: thrown clear with one Chain Lash hit.
// --------------------------------------------------------------------------

void gripBanishedTentacle(Boss& boss, int32_t slot, Vector2 tip)
{
    boss.snatchSlot = slot;
    boss.banishedGrabbing[static_cast<size_t>(slot)] = true;
    boss.banishedPhase[static_cast<size_t>(slot)] = 6;
    boss.banishedTip[static_cast<size_t>(slot)] = tip;
}

void releaseBanishedTentacle(Boss& boss)
{
    if (boss.snatchSlot < 0)
    {
        return;
    }
    const auto slot = static_cast<size_t>(boss.snatchSlot);
    boss.banishedGrabbing[slot] = false;
    boss.banishedPhase[slot] = 3;
    boss.banishedTimer[slot] = banishedTentacleRetreat;
    boss.snatchSlot = -1;
}

void beginBanishedJuggled(Game& game, Boss& boss, int32_t slot)
{
    bossQteBegin(game, boss.instanceId, QteKind::Juggled);
    game.run.qtePrompt = QtePrompt::Mash;
    game.run.qteTimer = UpdateConstants::banishedJuggleMashDuration;
    game.run.qteTimerMax = UpdateConstants::banishedJuggleMashDuration;
    game.run.qteMashTarget = UpdateConstants::banishedJuggleMashPresses;
    game.run.qteThreatDir = Vector2{};
    gripBanishedTentacle(boss, slot, game.run.player.position);
    boss.grabPromptsLeft = UpdateConstants::banishedJugglePrompts;
    boss.grabDragTarget = game.run.player.position;
    playSFX(game, game.resources.sounds.slamBoom);
    triggerShake(game, 12, 0.4F);
}

void endBanishedJuggled(Game& game, Boss& boss, bool clean)
{
    bossQteEnd(game);
    releaseBanishedTentacle(boss);
    boss.grabPromptsLeft = 0;
    const Vector2 center{.x = boss.position.x + boss.size.x / 2, .y = boss.position.y + boss.size.y / 2};
    const Vector2 away = Vector2Normalize(Vector2Subtract(game.run.player.position, center));
    game.run.player.position = Vector2Add(game.run.player.position, Vector2Scale(away, 160.0F));
    triggerShake(game, 12, 0.35F);
    if (clean)
    {
        playSFX(game, game.resources.sounds.critical);
        return; // the finisher already landed the eye burst
    }
    damagePlayer(game, enemyDamage(game, banishedThrashDamage));
    playSFX(game, game.resources.sounds.nerveFizzle);
}

void updateBanishedJuggled(Game& game, Boss& boss, float deltaTime)
{
    auto& run = game.run;
    const Vector2 center{.x = boss.position.x + boss.size.x / 2, .y = boss.position.y + boss.size.y / 2};
    if (run.qteActionTimer <= 0)
    {
        run.player.position =
            Vector2Lerp(run.player.position, boss.grabDragTarget, std::min(1.0F, deltaTime * 2.5F));
    }
    if (boss.snatchSlot >= 0)
    {
        boss.banishedTip[static_cast<size_t>(boss.snatchSlot)] = run.player.position;
    }

    if (run.qtePrompt == QtePrompt::Mash)
    {
        if (qteReadInput(game, QtePrompt::Mash) > 0)
        {
            run.qteMashCount++;
            triggerShake(game, 3, 0.08F);
        }
        if (run.qteMashCount >= run.qteMashTarget)
        {
            run.qteFinisherCharge += UpdateConstants::qteFinisherChargeMash;
            run.qtePrompt = QtePrompt::None;
            run.qteTimer = UpdateConstants::banishedJuggleMashBeat;
            run.qteTimerMax = 0;
            playSFX(game, game.resources.sounds.critical);
        }
        else if (run.qteTimer <= 0)
        {
            endBanishedJuggled(game, boss, false);
        }
        return;
    }

    if (run.qtePrompt != QtePrompt::None)
    {
        const int32_t answer = qteReadInput(game, run.qtePrompt);
        if (answer > 0)
        {
            qteStartAction(game, run.qtePrompt, run.qteThreatDir);
            playSFX(game, game.resources.sounds.critical);
            run.qtePrompt = QtePrompt::None;
            run.qteTimer = UpdateConstants::banishedJuggleBeat;
            run.qteTimerMax = 0;
        }
        else if (answer < 0 || run.qteTimer <= 0)
        {
            endBanishedJuggled(game, boss, false);
        }
        return;
    }

    if (run.qteTimer > 0)
    {
        return; // beat between prompts
    }
    if (boss.grabPromptsLeft > 0)
    {
        const int32_t left = boss.grabPromptsLeft--;
        const Vector2 fromBody = Vector2Normalize(Vector2Subtract(run.player.position, center));
        if (left > 1 && (left - 1) % UpdateConstants::banishedJugglePromptsPerThrow == 0)
        {
            // A throw: another tentacle takes the ship and hurls it round the body.
            std::array<int32_t, Boss::banishedTentacleCount> free{};
            int32_t freeCount = 0;
            for (int32_t t = 0; t < Boss::banishedTentacleCount; t++)
            {
                if (t != boss.snatchSlot && !boss.banishedStaggered[static_cast<size_t>(t)])
                {
                    free[static_cast<size_t>(freeCount++)] = t;
                }
            }
            if (freeCount > 0)
            {
                releaseBanishedTentacle(boss);
                gripBanishedTentacle(boss, free[static_cast<size_t>(GetRandomValue(0, freeCount - 1))],
                                     run.player.position);
            }
            const float turn = static_cast<float>(GetRandomValue(90, 150)) * DEG2RAD *
                               (GetRandomValue(0, 1) == 0 ? 1.0F : -1.0F);
            boss.grabDragTarget = Vector2Add(center, Vector2Scale(Vector2Rotate(fromBody, turn),
                                                                  boss.size.x / 2 + banishedNearReach));
            playSFX(game, game.resources.sounds.slamBoom);
            triggerShake(game, 8, 0.25F);
        }
        // Tentacles lash in from around the body.
        const float lashTurn = static_cast<float>(GetRandomValue(-50, 50)) * DEG2RAD;
        run.qteThreatDir = Vector2Rotate(fromBody, lashTurn);
        run.qtePrompt = qtePromptForThreat(game, run.qteThreatDir, UpdateConstants::banishedJuggleParryChance);
        run.qteTimer = UpdateConstants::banishedJugglePromptWindow;
        run.qteTimerMax = UpdateConstants::banishedJugglePromptWindow;
        playSFX(game, game.resources.sounds.bossWindUp);
        return;
    }
    qteBeginFinisher(game); // every prompt answered
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

    if (game.run.qteBossId == boss.instanceId)
    {
        return; // Juggled owns the tentacles
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
                const float maxReach = std::max(banishedNearReach * 1.3F, banishedFarPortalDist);
                const float radius = std::min(
                    Vector2Distance(boss.banishedAnchor[idx], game.run.player.position), maxReach);
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
                const float maxReach = std::max(banishedNearReach * 1.3F, banishedFarPortalDist);
                const float dist = std::min(Vector2Length(toPlayer), maxReach);
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
                else if (boss.banishedAttackKind[idx] == 2 && bossQteCanStart(game, QteKind::Juggled))
                {
                    beginBanishedJuggled(game, boss, i);
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

void spawnSlagmawDrifterBreak(Game& game, Vector2 position, int32_t generation)
{
    const float generationMult = static_cast<float>(1 << std::clamp(generation, 0, 8));
    for (int32_t i = 0; i < slagmawBreakDrifterCount; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto dist = static_cast<float>(GetRandomValue(40, 140));
        const Vector2 pos = Vector2Add(
            position, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        spawnEnemyAt(game, enemyKindMeteorChunk, pos);
        auto& drifter = game.run.enemies.back();
        drifter.health = static_cast<int32_t>(static_cast<float>(drifter.health) *
                                              slagmawBrokenDrifterHealthMult * generationMult);
    }
}

void spawnSlagmawHalfBreak(Game& game, const Boss& original, Vector2 bossCenter,
                           std::vector<Boss>& pendingHalves)
{
    const float halfSize = original.size.x * 0.5F;
    const int32_t childGeneration = original.slagmawBreakGeneration + 1;
    const float generationMult = static_cast<float>(1 << std::clamp(childGeneration, 0, 8));
    const auto halfHealth =
        std::max(1, static_cast<int32_t>(static_cast<float>(original.maxHealth) / 3.0F * generationMult));

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
                 .slagmawBreaksIntoDrifters = true,
                 .slagmawBreakGeneration = childGeneration};
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

// Rush is over for this plate: fly back to its slot with no leftover dash state.
void sendRushPlateHome(Boss& plate)
{
    plate.plateRushPhase = 0;
    plate.plateRushDelay = 0;
    plate.plateReturning = true;
    plate.dashHitPlayer = false;
    plate.state = BossState::IDLE;
    plate.chargeVelocity = Vector2{};
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

    if ((boss.isWreckwormHead && boss.wreckwormPunishTimer > 0) ||
        (boss.isSlagmaw && boss.slagmawPunishTimer > 0) || boss.qteStunTimer > 0)
    {
        return; // stunned by a clean Constrict, Rush or Swallowed
    }

    if (!boss.isWreckwormSegment && !(boss.isBeltbreakerPlate && boss.plateAttached) &&
        boss.health > 0 && boss.state == BossState::IDLE && boss.dazedTimer <= 0 &&
        boss.phaseInvulnTimer <= 0 &&
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
        if (boss.dazedTimer > 0)
        {
            boss.dazedTimer -= deltaTime;
        }
        if (boss.plateRushPhase != 0 && boss.plateRushPhase != 4)
        {
            // Owned by the plate-rush cutscene state machine.
            return;
        }
        if (boss.plateRushPhase == 4)
        {
            // Broken / timed-out plate: a live charge dash. Contact hurts.
            boss.position = Vector2Add(
                boss.position,
                Vector2Scale(boss.chargeVelocity, deltaTime));
            const Vector2 plateCenter{.x = boss.position.x + boss.size.x / 2,
                                      .y = boss.position.y + boss.size.y / 2};
            if (Vector2Distance(plateCenter, game.run.player.position) <=
                UpdateConstants::beltbreakerPlateRushHitRadius)
            {
                if (!boss.dashHitPlayer && game.run.player.health > 0 &&
                    !game.run.player.shieldActive && !game.run.player.dashing &&
                    game.run.player.immunityTimer <= 0)
                {
                    boss.dashHitPlayer = true;
                    damagePlayer(game, enemyDamage(game, 2));
                }
                // One way or another the pass is over: send it home.
                sendRushPlateHome(boss);
                return;
            }
            boss.plateRushDelay -= deltaTime;
            if (boss.plateRushDelay <= -UpdateConstants::beltbreakerRushLooseLingerTime)
            {
                sendRushPlateHome(boss);
            }
            return;
        }
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
        !boss.usesDirector && boss.health > 0 &&
        static_cast<float>(boss.health) <= static_cast<float>(boss.maxHealth) * enrageHealthFrac;

    const float speedMult = (enraged ? 1.25F : 1.0F) * (boss.debuffFreeze ? freezeSlowMult : 1.0F) *
                            (boss.shockwaveSlowTimer > 0 ? boss.shockwaveSlowMult : 1.0F) *
                            (boss.isBeltbreakerPlate ? beltbreakerPlateMoveSpeedMult : 1.0F) *
                            (boss.isWreckwormHead ? boss.wreckwormSpeedMult * wreckwormChaseSpeedMult
                                                  : 1.0F) *
                            (boss.isSlagmaw ? slagmawMoveSpeedMult : 1.0F);

    Vector2 move;
    if (boss.usesDirector)
    {
        // Director bosses scale their own speed per pattern, so keep the magnitude.
        move = bossDirectorMove(game, boss, bossCenter);
        if (Vector2Length(move) > 0)
        {
            const float chaseSpeed = (game.run.player.speed * 1.15F + 1) * speedMult;
            boss.position =
                Vector2Add(boss.position, Vector2Scale(move, chaseSpeed * deltaTime * frameScale));
        }
        return;
    }
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
    const float radius = core.size.x * 0.75F * core.plateRadiusMult;
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

auto ringCrushDuration(const Boss& core) -> float
{
    const float baseRadius = core.size.x * 0.75F;
    const float speed = ringCrushSpeed * frameScale;
    return (ringCrushMaxRadius - baseRadius * ringCrushTightMult) / speed +
           (ringCrushMaxRadius - baseRadius) / speed;
}

// Ring Crush: the tell pulls the plate ring tight, then it flies out to the max radius and comes
// back. While flying out, each attached plate's arc hurts; a missing plate leaves a gap.
void updateRingCrush(Game& game, Boss& core)
{
    const float baseRadius = core.size.x * 0.75F;
    const float tightRadius = baseRadius * ringCrushTightMult;
    core.plateRadiusMult = 1.0F;
    if (core.attack != BossAttack::GravityWell)
    {
        return;
    }
    if (core.state == BossState::WINDING_UP)
    {
        const float progress = std::clamp(1.0F - core.stateTimer / ringCrushTell, 0.0F, 1.0F);
        core.plateRadiusMult = Lerp(1.0F, ringCrushTightMult, progress);
        return;
    }
    if (core.state != BossState::SHOOTING)
    {
        return;
    }

    const float speed = ringCrushSpeed * frameScale;
    const float elapsed = ringCrushDuration(core) - core.stateTimer;
    const float outTime = (ringCrushMaxRadius - tightRadius) / speed;
    const bool flyingOut = elapsed < outTime;
    const float radius = flyingOut ? tightRadius + speed * elapsed
                                   : std::max(baseRadius, ringCrushMaxRadius - speed * (elapsed - outTime));
    core.plateRadiusMult = radius / baseRadius;

    if (!flyingOut || radius < core.size.x / 2 + ringCrushHugMargin || game.run.player.health <= 0 ||
        game.run.player.immunityTimer > 0)
    {
        return;
    }
    const Vector2 toPlayer = Vector2Subtract(game.run.player.position, beltbreakerCoreCenter(core));
    if (std::abs(Vector2Length(toPlayer) - radius) > 22.0F + game.run.player.radius)
    {
        return;
    }
    const float playerDeg = std::atan2(toPlayer.y, toPlayer.x) * RAD2DEG;
    const float slotWidth = 360.0F / static_cast<float>(core.plateCount);
    for (const auto& plate : game.run.bosses)
    {
        if (!plate.isBeltbreakerPlate || plate.plateOwnerId != core.instanceId || !plate.plateAttached ||
            plate.health <= 0)
        {
            continue;
        }
        const float plateDeg = core.plateAngleDeg + static_cast<float>(plate.plateSlotIndex) * slotWidth;
        const float diff = std::abs(std::remainder(playerDeg - plateDeg, 360.0F));
        if (diff < slotWidth * ringCrushArcFrac / 2)
        {
            damagePlayer(game, enemyDamage(game, ringCrushDamage));
            return;
        }
    }
}

auto isRushPlateOf(const Boss& plate, const Boss& core) -> bool;

void updateBeltbreakerCore(Game& game, Boss& core, float deltaTime)
{
    if (!core.isBeltbreaker || core.health <= 0)
    {
        return;
    }

    updateRingCrush(game, core);

    if (core.beltbreakerPunishTimer > 0)
    {
        core.beltbreakerPunishTimer -= deltaTime;
        if (core.beltbreakerPunishTimer <= 0)
        {
            for (auto& plate : game.run.bosses)
            {
                if (isRushPlateOf(plate, core) && plate.plateRushPhase == 2)
                {
                    sendRushPlateHome(plate);
                }
            }
        }
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
            // First biome-boss QTE: the plates suddenly burst away and the
            // plate-rush cutscene (reaction prompts) takes over.
            if (bossQteCanStart(game, QteKind::PlateRush))
            {
                beginBeltbreakerPlateRush(game, core);
            }
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

// --- Beltbreaker plate-rush QTE (cutscene) --------------------------------
//
// When the core shields up, its plates burst off-screen and the boss QTE
// cutscene (qte.cpp) starts. A barrage follows: one plate at a time comes in
// from a new side, posing a prompt from its approach. Answering it plays the
// ship's reaction (dodge / block / parry / burst) and the plate flies back
// off-screen for another pass. A wrong or late answer ends the cutscene: the
// plate hits for real and every plate heads home.
// --------------------------------------------------------------------------

auto beltbreakerRushOffscreenDist(const Game& game) -> float
{
    return std::max(static_cast<float>(game.resources.screenWidth),
                    static_cast<float>(game.resources.screenHeight)) /
               2 +
           UpdateConstants::beltbreakerPlateRushOffscreenMargin;
}

auto isRushPlateOf(const Boss& plate, const Boss& core) -> bool
{
    return plate.isBeltbreakerPlate && plate.plateOwnerId == core.instanceId && plate.health > 0;
}

void beginBeltbreakerPlateRush(Game& game, Boss& core)
{
    if (std::ranges::none_of(game.run.bosses,
                             [&](const Boss& plate) { return isRushPlateOf(plate, core); }))
    {
        return;
    }

    bossQteBegin(game, core.instanceId, QteKind::PlateRush);
    game.run.qteTimer = UpdateConstants::beltbreakerPlateRushCueDuration;
    core.plateRushAttacksLeft = UpdateConstants::beltbreakerRushAttacks;
    core.plateRushLastAngle = static_cast<float>(GetRandomValue(0, 359));

    const Vector2 coreCenter = beltbreakerCoreCenter(core);
    for (auto& plate : game.run.bosses)
    {
        if (!isRushPlateOf(plate, core))
        {
            continue;
        }
        const float angleDeg = static_cast<float>(GetRandomValue(0, 359));
        const Vector2 dir{.x = std::cos(angleDeg * DEG2RAD), .y = std::sin(angleDeg * DEG2RAD)};
        plate.plateAttached = false;
        plate.plateReturning = false;
        plate.plateExcursionTimer = 0;
        plate.plateRushPhase = 1;
        plate.plateRushDelay = 0;
        plate.position =
            Vector2Subtract(coreCenter, Vector2{.x = plate.size.x / 2, .y = plate.size.y / 2});
        plate.state = BossState::IDLE;
        plate.dashHitPlayer = false;
        plate.chargeVelocity = Vector2Scale(dir, UpdateConstants::beltbreakerPlateRushOffscreenSpeed);
    }
    playSFX(game, game.resources.sounds.bossWindUp);
    triggerShake(game, 8, 0.3F);
}

// Clean clear: the shield shatters into the Punish Window; the plates stay parked off screen
// until it ends (updateBeltbreakerCore sends them home). Fail: the shield stays up its full
// duration while the plates go back to harassing.
void endBeltbreakerPlateRush(Game& game, Boss& core, bool clean)
{
    bossQteEnd(game);
    if (clean)
    {
        core.beltbreakerShielded = false;
        core.shieldGenProgress = 0;
        core.beltbreakerPunishTimer = UpdateConstants::beltbreakerPunishWindow;
        playSFX(game, game.resources.sounds.critical);
        triggerShake(game, 10, 0.35F);
        return;
    }
    for (auto& plate : game.run.bosses)
    {
        if (isRushPlateOf(plate, core) && plate.plateRushPhase != 0 && plate.plateRushPhase != 4)
        {
            plate.plateRushPhase = 0;
            plate.state = BossState::IDLE;
            plate.chargeVelocity = Vector2{};
            sendPlateOnExcursion(plate, std::max(0.1F, core.beltbreakerShieldTimer - beltbreakerReturnLeadTime));
        }
    }
}

// Send a parked plate in from a new side, aimed at the player.
void launchBeltbreakerRushAttack(Game& game, Boss& core, Boss& plate)
{
    const float offDist = beltbreakerRushOffscreenDist(game);
    const float turn = static_cast<float>(
        GetRandomValue(static_cast<int32_t>(UpdateConstants::beltbreakerRushMinAngleChange),
                       360 - static_cast<int32_t>(UpdateConstants::beltbreakerRushMinAngleChange)));
    core.plateRushLastAngle = std::fmod(core.plateRushLastAngle + turn, 360.0F);
    const Vector2 fromDir{.x = std::cos(core.plateRushLastAngle * DEG2RAD),
                          .y = std::sin(core.plateRushLastAngle * DEG2RAD)};
    const Vector2 start = Vector2Add(game.run.player.position, Vector2Scale(fromDir, offDist));

    plate.position = Vector2Subtract(start, Vector2{.x = plate.size.x / 2, .y = plate.size.y / 2});
    plate.plateRushPhase = 3;
    plate.plateRushPromptDirection = Vector2Negate(fromDir);
    // Arrives on the player exactly when the prompt times out.
    plate.chargeVelocity = Vector2Scale(plate.plateRushPromptDirection,
                                        offDist / UpdateConstants::beltbreakerPlateRushPromptDuration);
    core.plateRushAttacksLeft--;

    game.run.qtePrompt = qtePromptForThreat(game, plate.plateRushPromptDirection);
    game.run.qteThreatDir = plate.plateRushPromptDirection;
    game.run.qteTimer = UpdateConstants::beltbreakerPlateRushPromptDuration;
    game.run.qteTimerMax = UpdateConstants::beltbreakerPlateRushPromptDuration;
    playSFX(game, game.resources.sounds.bossWindUp);
}

// The prompt was answered: the plate reacts to what the ship did.
void resolveBeltbreakerRushHit(Game& game, Boss& plate)
{
    const QtePrompt prompt = game.run.qtePrompt;
    qteStartAction(game, prompt, plate.plateRushPromptDirection);
    game.run.qtePrompt = QtePrompt::None;
    game.run.qteTimer = UpdateConstants::beltbreakerRushBeat;

    float damageFrac = 0;
    switch (prompt)
    {
    case QtePrompt::Shield:
        damageFrac = UpdateConstants::beltbreakerRushShieldDamageFrac;
        break;
    case QtePrompt::Parry:
        damageFrac = UpdateConstants::beltbreakerRushParryDamageFrac;
        break;
    case QtePrompt::Nerve:
        damageFrac = UpdateConstants::beltbreakerRushNerveDamageFrac;
        break;
    default:
        break; // dodged: the plate just flies past
    }

    plate.plateRushPhase = 5;
    if (damageFrac > 0)
    {
        // Blocked / parried / burst: knocked back and staggered.
        plate.chargeVelocity = Vector2Negate(plate.chargeVelocity);
        plate.dazedTimer = UpdateConstants::beltbreakerRushStagger;
        const auto amount =
            std::max(1, static_cast<int32_t>(static_cast<float>(plate.maxHealth) * damageFrac));
        damageBoss(game, plate, amount);
        playSFX(game, game.resources.sounds.critical);
        triggerBossShieldDashShake(game);
    }
    else
    {
        playSFX(game, game.resources.sounds.menuMove);
    }
}

void updateBeltbreakerPlateRush(Game& game, Boss& core, float deltaTime)
{
    const float offDist = beltbreakerRushOffscreenDist(game);
    Boss* incoming = nullptr;
    bool anyInFlight = false;
    std::vector<Boss*> parked;
    for (auto& plate : game.run.bosses)
    {
        if (!isRushPlateOf(plate, core))
        {
            continue;
        }
        switch (plate.plateRushPhase)
        {
        case 1: // bursting out
        case 5: // answered, leaving
            plate.position =
                Vector2Add(plate.position, Vector2Scale(plate.chargeVelocity, deltaTime));
            if (Vector2Distance(Vector2Add(plate.position, Vector2Scale(plate.size, 0.5F)),
                                game.run.player.position) > offDist)
            {
                plate.plateRushPhase = 2;
                plate.chargeVelocity = Vector2{};
                parked.push_back(&plate);
            }
            else
            {
                anyInFlight = true;
            }
            break;
        case 2:
            parked.push_back(&plate);
            break;
        case 3:
            plate.position =
                Vector2Add(plate.position, Vector2Scale(plate.chargeVelocity, deltaTime));
            incoming = &plate;
            break;
        default:
            break;
        }
    }

    if (incoming != nullptr)
    {
        const int32_t answer = qteReadInput(game, game.run.qtePrompt);
        if (answer > 0)
        {
            resolveBeltbreakerRushHit(game, *incoming);
        }
        else if (answer < 0 || game.run.qteTimer <= 0)
        {
            // Wrong or late: the cutscene breaks and this plate hits for real.
            endBeltbreakerPlateRush(game, core, false);
            incoming->plateRushPhase = 4;
            incoming->plateRushDelay = 0;
            incoming->dashHitPlayer = false;
            playSFX(game, game.resources.sounds.nerveFizzle);
        }
        return;
    }

    // The prompted plate was destroyed mid-flight.
    game.run.qtePrompt = QtePrompt::None;
    if (game.run.qteTimer > 0)
    {
        return; // cue / beat between attacks
    }
    if (core.plateRushAttacksLeft > 0 && !parked.empty())
    {
        Boss* plate = parked.at(static_cast<size_t>(
            GetRandomValue(0, static_cast<int32_t>(parked.size()) - 1)));
        launchBeltbreakerRushAttack(game, core, *plate);
        return;
    }
    if (!anyInFlight)
    {
        qteBeginFinisher(game); // every prompt answered
    }
}

// --- Wreckworm constrict QTE (cutscene) ------------------------------------
//
// A closing Coil Clamp that reaches the player outside its gap starts the boss
// QTE cutscene (qte.cpp). The coil keeps tightening around the ship while a
// Mash prompt pries it open; then segments rear out from different points on
// the coil and lunge in, one prompt each. Clean clear: the coil springs open
// and the head is stunned for the Punish Window. A failed mash or a wrong or
// late answer: the coil crushes once and lets go.
// --------------------------------------------------------------------------

auto findWreckwormHeadById(Game& game, int32_t instanceId) -> Boss*
{
    for (auto& boss : game.run.bosses)
    {
        if (boss.isWreckwormHead && boss.health > 0 && boss.instanceId == instanceId)
        {
            return &boss;
        }
    }
    return nullptr;
}

auto wreckwormCoilSegmentDeg(const Boss& head, int32_t segmentIndex) -> float
{
    const float t = head.segmentCount > 1 ? static_cast<float>(segmentIndex) /
                                                static_cast<float>(head.segmentCount - 1)
                                          : 0.0F;
    return head.coilClampGapDeg + wreckwormCoilClampGapDeg / 2.0F +
           t * (360.0F - wreckwormCoilClampGapDeg);
}

void beginWreckwormConstrict(Game& game, Boss& head)
{
    bossQteBegin(game, head.instanceId, QteKind::Constrict);
    game.run.qtePrompt = QtePrompt::Mash;
    game.run.qteTimer = UpdateConstants::wreckwormConstrictMashDuration;
    game.run.qteTimerMax = UpdateConstants::wreckwormConstrictMashDuration;
    game.run.qteMashTarget = UpdateConstants::wreckwormConstrictMashPresses;
    game.run.qteThreatDir = Vector2{};
    head.constrictStrikesLeft = UpdateConstants::wreckwormConstrictStrikes;
    head.constrictLungeSegment = -1;
    head.constrictLungeFrac = 0;
    head.constrictStruckMask = 0;
    playSFX(game, game.resources.sounds.bossWindUp);
    triggerShake(game, 8, 0.3F);
}

// Either way the coil lets go: the clamp skips to its release, segments rejoin the chain.
void endWreckwormConstrict(Game& game, Boss& head, bool clean)
{
    bossQteEnd(game);
    head.constrictLungeSegment = -1;
    head.stateTimer = std::min(head.stateTimer, wreckwormCoilClampReleaseDuration);
    if (clean)
    {
        head.wreckwormPunishTimer = UpdateConstants::wreckwormPunishWindow;
        playSFX(game, game.resources.sounds.critical);
        triggerShake(game, 10, 0.35F);
        return;
    }
    damagePlayer(game, enemyDamage(game, UpdateConstants::wreckwormConstrictCrushDamage));
    playSFX(game, game.resources.sounds.nerveFizzle);
    triggerShake(game, 12, 0.35F);
}

// A segment from a new point on the coil rears out and lunges at the player. Segments that
// already struck are skipped while fresh ones remain; never the same one twice in a row.
void launchWreckwormConstrictStrike(Game& game, Boss& head)
{
    std::vector<int32_t> fresh;
    std::vector<int32_t> choices;
    for (int32_t i = 0; i < head.segmentCount; i++)
    {
        if (i != head.constrictLungeSegment && findWreckwormSegment(game, head.instanceId, i) != nullptr)
        {
            choices.push_back(i);
            if (i >= 64 || (head.constrictStruckMask & (uint64_t{1} << i)) == 0)
            {
                fresh.push_back(i);
            }
        }
    }
    if (!fresh.empty())
    {
        choices = std::move(fresh);
    }
    if (choices.empty())
    {
        qteBeginFinisher(game); // no segments left to strike with
        return;
    }
    const int32_t index =
        choices.at(static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(choices.size()) - 1)));
    const float deg = wreckwormCoilSegmentDeg(head, index);
    const Vector2 travelDir{.x = -std::cos(deg * DEG2RAD), .y = -std::sin(deg * DEG2RAD)};

    head.constrictLungeSegment = index;
    head.constrictLungeFrac = 0;
    if (index < 64)
    {
        head.constrictStruckMask |= uint64_t{1} << index;
    }
    head.constrictStrikesLeft--;
    game.run.qtePrompt =
        qtePromptForThreat(game, travelDir, UpdateConstants::wreckwormConstrictParryChance);
    game.run.qteThreatDir = travelDir;
    game.run.qteTimer = UpdateConstants::wreckwormConstrictStrikeWindow;
    game.run.qteTimerMax = UpdateConstants::wreckwormConstrictStrikeWindow;
    playSFX(game, game.resources.sounds.bossWindUp);
}

void updateWreckwormConstrict(Game& game, Boss& head, float deltaTime)
{
    auto& run = game.run;
    if (head.state != BossState::SHOOTING || head.attack != BossAttack::CoilClamp)
    {
        endWreckwormConstrict(game, head, false); // the clamp was cut short
        return;
    }
    // The coil rides with the ship and keeps tightening on real time, short of letting go.
    head.targetPosition = run.player.position;
    head.stateTimer = std::max(wreckwormCoilClampReleaseDuration + UpdateConstants::wreckwormConstrictHoldMargin,
                               head.stateTimer - deltaTime);

    if (run.qtePrompt == QtePrompt::Mash)
    {
        if (qteReadInput(game, QtePrompt::Mash) > 0)
        {
            run.qteMashCount++;
            triggerShake(game, 3, 0.08F);
        }
        if (run.qteMashCount >= run.qteMashTarget)
        {
            run.qteFinisherCharge += UpdateConstants::qteFinisherChargeMash;
            run.qtePrompt = QtePrompt::None;
            run.qteTimer = UpdateConstants::wreckwormConstrictMashBeat;
            run.qteTimerMax = 0;
            playSFX(game, game.resources.sounds.critical);
        }
        else if (run.qteTimer <= 0)
        {
            endWreckwormConstrict(game, head, false);
        }
        return;
    }

    if (run.qtePrompt != QtePrompt::None)
    {
        Boss* seg = findWreckwormSegment(game, head.instanceId, head.constrictLungeSegment);
        head.constrictLungeFrac =
            1.0F - std::clamp(run.qteTimer / UpdateConstants::wreckwormConstrictStrikeWindow, 0.0F, 1.0F);
        const int32_t answer = qteReadInput(game, run.qtePrompt);
        if (seg == nullptr || answer > 0)
        {
            // Answered (or the striking segment died): it falls back into the coil.
            if (seg != nullptr)
            {
                qteStartAction(game, run.qtePrompt, run.qteThreatDir);
                seg->hitFlashTimer = UpdateConstants::hitFlashDuration;
                playSFX(game, game.resources.sounds.menuMove);
            }
            head.constrictLungeFrac = 0; // kept as the last striker, so the next comes from elsewhere
            run.qtePrompt = QtePrompt::None;
            run.qteTimer = UpdateConstants::wreckwormConstrictBeat;
            run.qteTimerMax = 0;
        }
        else if (answer < 0 || run.qteTimer <= 0)
        {
            endWreckwormConstrict(game, head, false);
        }
        return;
    }

    if (run.qteTimer > 0)
    {
        return; // beat between strikes
    }
    if (head.constrictStrikesLeft > 0)
    {
        launchWreckwormConstrictStrike(game, head);
        return;
    }
    qteBeginFinisher(game); // every prompt answered
}

// --- Wreckworm rush QTE (cutscene) -----------------------------------------
//
// A Burrow Charge or Dash Rush head about to reach the player starts the boss QTE
// cutscene. The head keeps crawling on real time so it, then the body, pass the
// ship while it answers a short chain of prompts; every answer strikes the head
// (harder for a parry or nerve burst). Clean clear: the head stalls for the Punish
// Window. A wrong or late answer ends the cutscene and the rush lands live, still
// dodgeable.
// --------------------------------------------------------------------------

auto findBossById(Game& game, int32_t instanceId) -> Boss*
{
    for (auto& boss : game.run.bosses)
    {
        if (boss.health > 0 && boss.instanceId == instanceId)
        {
            return &boss;
        }
    }
    return nullptr;
}

auto bossQteStunned(const Boss& boss) -> bool
{
    return boss.wreckwormPunishTimer > 0 || boss.slagmawPunishTimer > 0 || boss.qteStunTimer > 0;
}

void tryBeginRushQte(Game& game, Boss& boss, Vector2 bossCenter)
{
    // The Wreckworm's rushes always qualify; any other boss's dash only once it is enraged. The
    // Storm Serpent shares the Wreckworm's moves but Boss QTEs belong to Biome Bosses only.
    const bool wreckworm = boss.isWreckwormHead && !boss.krakenSnakeVariant;
    const bool enragedDash = boss.attack == BossAttack::ChargeDash && !boss.isWreckwormHead &&
                             !boss.isWreckwormSegment && !boss.isBeltbreakerPlate && !boss.isKraken &&
                             !boss.isBanished &&
                             (boss.sandboxForcedRush ||
                              static_cast<float>(boss.health) <= static_cast<float>(boss.maxHealth) * enrageHealthFrac);
    if (!(wreckworm || enragedDash) || bossQteStunned(boss) || !bossQteCanStart(game, QteKind::Rush))
    {
        return;
    }
    const Vector2 toPlayer = Vector2Subtract(game.run.player.position, bossCenter);
    if (Vector2Length(toPlayer) > UpdateConstants::rushQteTriggerDist + boss.size.x / 2 ||
        Vector2DotProduct(toPlayer, boss.chargeVelocity) <= 0)
    {
        return;
    }
    bossQteBegin(game, boss.instanceId, QteKind::Rush);
    boss.sandboxForcedRush = false;
    game.run.qteThreatDir = Vector2Normalize(boss.chargeVelocity);
    boss.rushPromptsLeft = UpdateConstants::rushQtePrompts;
    playSFX(game, game.resources.sounds.bossWindUp);
    triggerShake(game, 8, 0.3F);
}

void endRushQte(Game& game, Boss& boss, bool clean)
{
    bossQteEnd(game);
    boss.rushPromptsLeft = 0;
    if (!clean)
    {
        playSFX(game, game.resources.sounds.nerveFizzle);
        return; // the rush carries on live
    }
    // It ploughs to a stop, stunned: the rush is over.
    float& stun = boss.isWreckwormHead ? boss.wreckwormPunishTimer
                  : boss.isSlagmaw     ? boss.slagmawPunishTimer
                                       : boss.qteStunTimer;
    stun = UpdateConstants::qteStunWindow;
    boss.chargeVelocity = Vector2{};
    boss.slamHit = false;
    boss.wreckwormRepositioning = false;
    boss.state = BossState::IDLE;
    boss.attackTimer = UpdateConstants::qteStunWindow;
    playSFX(game, game.resources.sounds.critical);
}

void updateRushQte(Game& game, Boss& boss, float deltaTime)
{
    auto& run = game.run;
    const bool rushing = boss.state == BossState::SHOOTING &&
                         (boss.attack == BossAttack::BurrowCharge || boss.attack == BossAttack::DashRush ||
                          boss.attack == BossAttack::ChargeDash);
    if (!rushing)
    {
        endRushQte(game, boss, false);
        return;
    }
    // It keeps coming on real time at a crawl, so it passes the ship during the chain.
    boss.position = Vector2Add(boss.position, Vector2Scale(run.qteThreatDir,
                                                           UpdateConstants::rushQteCrawlSpeed * deltaTime));

    if (run.qtePrompt != QtePrompt::None)
    {
        const int32_t answer = qteReadInput(game, run.qtePrompt);
        if (answer > 0)
        {
            qteStartAction(game, run.qtePrompt, run.qteThreatDir);
            playSFX(game, game.resources.sounds.critical);
            run.qtePrompt = QtePrompt::None;
            run.qteTimer = UpdateConstants::rushQteBeat;
            run.qteTimerMax = 0;
        }
        else if (answer < 0 || run.qteTimer <= 0)
        {
            endRushQte(game, boss, false);
        }
        return;
    }

    if (run.qteTimer > 0)
    {
        return; // beat between prompts
    }
    if (boss.rushPromptsLeft > 0)
    {
        boss.rushPromptsLeft--;
        run.qtePrompt = qtePromptForThreat(game, run.qteThreatDir, UpdateConstants::rushQteParryChance);
        run.qteTimer = UpdateConstants::rushQtePromptWindow;
        run.qteTimerMax = UpdateConstants::rushQtePromptWindow;
        playSFX(game, game.resources.sounds.bossWindUp);
        return;
    }
    qteBeginFinisher(game); // every prompt answered
}

// --- Slagmaw Devour and Swallowed QTE (cutscene) -------------------------------
//
// Devour: after a gold tell the maw inhales along a locked cone, dragging the player
// toward it (a dash rides through the pull). Reaching the maw starts the Swallowed QTE,
// or takes a bite when a QTE can't start. Swallowed: a Mash to brace the jaws, then
// prompts as Slagmaw chews and whips, slag gobs coming from shifting angles; every
// answer strikes it from inside. Clean clear: the ship bursts out and Slagmaw chokes
// for the Punish Window. Fail: spat out with one burn hit.
// --------------------------------------------------------------------------

auto slagmawMawCenter(const Boss& slagmaw) -> Vector2
{
    const Vector2 center{.x = slagmaw.position.x + slagmaw.size.x / 2,
                         .y = slagmaw.position.y + slagmaw.size.y / 2};
    return Vector2Add(center, Vector2Scale(slagmaw.slagmawFacing, slagmaw.size.x / 2 * slagmawMawOffset));
}

auto findSlagmawById(Game& game, int32_t instanceId) -> Boss*
{
    for (auto& boss : game.run.bosses)
    {
        if (boss.isSlagmaw && boss.health > 0 && boss.instanceId == instanceId)
        {
            return &boss;
        }
    }
    return nullptr;
}

void beginSlagmawSwallowed(Game& game, Boss& slagmaw)
{
    bossQteBegin(game, slagmaw.instanceId, QteKind::Swallowed);
    game.run.qtePrompt = QtePrompt::Mash;
    game.run.qteTimer = UpdateConstants::slagmawSwallowMashDuration;
    game.run.qteTimerMax = UpdateConstants::slagmawSwallowMashDuration;
    game.run.qteMashTarget = UpdateConstants::slagmawSwallowMashPresses;
    game.run.qteThreatDir = Vector2{};
    slagmaw.swallowPromptsLeft = UpdateConstants::slagmawSwallowPrompts;
    playSFX(game, game.resources.sounds.slamBoom);
    triggerShake(game, 12, 0.4F);
}

// Either way the ship leaves the maw and Devour is over.
void endSlagmawSwallowed(Game& game, Boss& slagmaw, bool clean)
{
    bossQteEnd(game);
    slagmaw.swallowPromptsLeft = 0;
    const Vector2 maw = slagmawMawCenter(slagmaw);
    game.run.player.position = Vector2Add(maw, Vector2Scale(slagmaw.slagmawFacing, slagmaw.size.x * 0.4F));
    slagmaw.state = BossState::IDLE;
    slagmaw.stateTimer = 0;
    triggerShake(game, 12, 0.35F);
    if (clean)
    {
        slagmaw.slagmawPunishTimer = UpdateConstants::slagmawPunishWindow;
        slagmaw.attackTimer = UpdateConstants::slagmawPunishWindow;
        spawnKillExplosion(game, maw, Palette::Charge, 30, 1.6F);
        playSFX(game, game.resources.sounds.critical);
        return;
    }
    damagePlayer(game, enemyDamage(game, UpdateConstants::slagmawSwallowBurnDamage));
    playSFX(game, game.resources.sounds.nerveFizzle);
}

void updateSlagmawDevour(Game& game, Boss& slagmaw, float deltaTime)
{
    auto& player = game.run.player;
    if (bossQteActive(game) || player.health <= 0)
    {
        return;
    }
    const Vector2 maw = slagmawMawCenter(slagmaw);
    const Vector2 toPlayer = Vector2Subtract(player.position, maw);
    const float dist = Vector2Length(toPlayer);
    if (dist <= slagmaw.size.x * 0.18F + player.radius)
    {
        if (bossQteCanStart(game, QteKind::Swallowed))
        {
            beginSlagmawSwallowed(game, slagmaw);
            return;
        }
        damagePlayer(game, enemyDamage(game, devourBiteDamage)); // a bite, then thrown clear
        player.position = Vector2Add(maw, Vector2Scale(slagmaw.devourDir, slagmaw.size.x * 0.4F));
        slagmaw.stateTimer = 0;
        return;
    }
    if (dist > devourRange || dist < 0.01F || player.dashing ||
        Vector2DotProduct(Vector2Scale(toPlayer, 1.0F / dist), slagmaw.devourDir) <
            std::cos(devourConeHalfDeg * DEG2RAD))
    {
        return;
    }
    player.position = Vector2Subtract(player.position, Vector2Scale(toPlayer, devourPull * deltaTime / dist));
}

void updateSlagmawSwallowed(Game& game, Boss& slagmaw, float deltaTime)
{
    auto& run = game.run;
    // It chews and whips about; between moves the ship is hauled back into the maw.
    slagmaw.slagmawFacing =
        Vector2Rotate(slagmaw.devourDir, std::sin(static_cast<float>(GetTime()) * 7.0F) * 0.35F);
    if (run.qteActionTimer <= 0)
    {
        run.player.position =
            Vector2Lerp(run.player.position, slagmawMawCenter(slagmaw), std::min(1.0F, deltaTime * 10.0F));
    }

    if (run.qtePrompt == QtePrompt::Mash)
    {
        if (qteReadInput(game, QtePrompt::Mash) > 0)
        {
            run.qteMashCount++;
            triggerShake(game, 3, 0.08F);
        }
        if (run.qteMashCount >= run.qteMashTarget)
        {
            run.qteFinisherCharge += UpdateConstants::qteFinisherChargeMash;
            run.qtePrompt = QtePrompt::None;
            run.qteTimer = UpdateConstants::slagmawSwallowMashBeat;
            run.qteTimerMax = 0;
            playSFX(game, game.resources.sounds.critical);
        }
        else if (run.qteTimer <= 0)
        {
            endSlagmawSwallowed(game, slagmaw, false);
        }
        return;
    }

    if (run.qtePrompt != QtePrompt::None)
    {
        const int32_t answer = qteReadInput(game, run.qtePrompt);
        if (answer > 0)
        {
            qteStartAction(game, run.qtePrompt, run.qteThreatDir);
            playSFX(game, game.resources.sounds.critical);
            run.qtePrompt = QtePrompt::None;
            run.qteTimer = UpdateConstants::slagmawSwallowBeat;
            run.qteTimerMax = 0;
        }
        else if (answer < 0 || run.qteTimer <= 0)
        {
            endSlagmawSwallowed(game, slagmaw, false);
        }
        return;
    }

    if (run.qteTimer > 0)
    {
        return; // beat between prompts
    }
    if (slagmaw.swallowPromptsLeft > 0)
    {
        // A slag gob from a new angle inside the jaws.
        slagmaw.swallowPromptsLeft--;
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        run.qteThreatDir = Vector2{.x = std::cos(angle), .y = std::sin(angle)};
        run.qtePrompt = qtePromptForThreat(game, run.qteThreatDir, UpdateConstants::slagmawSwallowParryChance);
        run.qteTimer = UpdateConstants::slagmawSwallowPromptWindow;
        run.qteTimerMax = UpdateConstants::slagmawSwallowPromptWindow;
        playSFX(game, game.resources.sounds.spreadBurst);
        return;
    }
    qteBeginFinisher(game); // every prompt answered
}

// Sandbox: forcing a boss move starts that move's QTE at once, ignoring the 20 s gap. Grab
// moves skip their catch; rush moves still run, and their Rush QTE starts as they pass (even
// before enrage). Beltbreaker, Kraken and Banished open their own QTE on any forced move.
void sandboxForceBossQte(Game& game, Boss& boss, BossAttack attack)
{
    if (!qteSlotFree(game))
    {
        return;
    }
    game.run.qteCooldowns.fill(0);
    const Vector2 player = game.run.player.position;
    if (boss.isBeltbreaker)
    {
        beginBeltbreakerPlateRush(game, boss);
        detachBeltbreakerPlates(game, boss);
    }
    else if (boss.isKraken)
    {
        boss.snatchSlot = 0;
        boss.snatchPhase = 0;
        boss.tentaclePhase[0] = 5;
        boss.tentaclePortalPos[0] = Vector2Add(player, Vector2{.x = 320.0F, .y = 0});
        boss.snatchPos = player;
        beginKrakenDraggedUnder(game, boss);
    }
    else if (boss.isBanished)
    {
        beginBanishedJuggled(game, boss, 0);
    }
    else if (boss.isWreckwormHead && !boss.krakenSnakeVariant && attack == BossAttack::CoilClamp)
    {
        boss.attack = BossAttack::CoilClamp;
        boss.state = BossState::SHOOTING;
        boss.stateTimer = 3.0F;
        boss.targetPosition = player;
        beginWreckwormConstrict(game, boss);
    }
    else if (boss.isSlagmaw && attack == BossAttack::Devour)
    {
        boss.devourDir = Vector2Normalize(Vector2Subtract(player, slagmawMawCenter(boss)));
        beginSlagmawSwallowed(game, boss);
    }
    else if (attack == BossAttack::BurrowCharge || attack == BossAttack::DashRush || attack == BossAttack::ChargeDash)
    {
        boss.sandboxForcedRush = true; // tryBeginRushQte skips the enrage check
    }
}

// After the finisher lands: each Boss QTE's own clean-clear payoff.
void endBossQteClean(Game& game, Boss& boss)
{
    switch (static_cast<QteKind>(game.run.qteKind))
    {
    case QteKind::PlateRush:
        endBeltbreakerPlateRush(game, boss, true);
        return;
    case QteKind::Constrict:
        endWreckwormConstrict(game, boss, true);
        return;
    case QteKind::Rush:
        endRushQte(game, boss, true);
        return;
    case QteKind::Swallowed:
        endSlagmawSwallowed(game, boss, true);
        return;
    case QteKind::DraggedUnder:
        endKrakenDraggedUnder(game, boss, true);
        return;
    case QteKind::Juggled:
        endBanishedJuggled(game, boss, true);
        return;
    }
    bossQteEnd(game);
}

// Magma Ram pools burn the player inside one every tick, ignoring hit immunity.
void updateMagmaPools(Game& game, float deltaTime)
{
    auto& run = game.run;
    bool inPool = false;
    for (auto& pool : run.magmaPools)
    {
        pool.life -= deltaTime;
        inPool = inPool || (pool.life > 0 && Vector2Distance(pool.position, run.player.position) <
                                                 magmaPoolRadius + run.player.radius);
    }
    std::erase_if(run.magmaPools, [](const MagmaPool& pool) { return pool.life <= 0; });
    if (!inPool || run.player.health <= 0 || run.player.dashing)
    {
        run.magmaPoolTick = 0;
        return;
    }
    run.magmaPoolTick -= deltaTime;
    if (run.magmaPoolTick <= 0)
    {
        run.magmaPoolTick = magmaPoolTickInterval;
        damagePlayer(game, enemyDamage(game, magmaPoolDamage));
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

auto krakenPodPosition(const Boss& boss, int32_t podIndex) -> Vector2;

auto krakenActiveTentacleCount(int32_t encounter) -> int32_t
{
    if (encounter <= 1)
    {
        return 1;
    }
    if (encounter == 2)
    {
        return 2;
    }
    return Boss::krakenLimbSlots;
}

auto krakenStaggerDuration(int32_t encounter) -> float
{
    if (encounter <= 1)
    {
        return krakenStaggerDurationE1;
    }
    if (encounter == 2)
    {
        return krakenStaggerDurationE2;
    }
    return krakenStaggerDurationE3;
}

// A tentacle slot is free to start a new swipe or grab-throw attack only when it isn't
// already mid-attack in either system and isn't hanging staggered.
auto krakenTentacleSlotIdle(const Boss& boss, int32_t slot) -> bool
{
    return boss.tentaclePhase[static_cast<size_t>(slot)] == 0 &&
           boss.limbPhase[static_cast<size_t>(slot)] == 0 &&
           !boss.tentacleStaggered[static_cast<size_t>(slot)];
}

// Lets the boss's ordinary body-contact shield parry also stagger a mid-swing tentacle,
// since that's the parry players already know from every other boss.
void krakenStaggerSwipingTentacle(Boss& boss)
{
    const int32_t activeCount = krakenActiveTentacleCount(boss.krakenEncounter);
    for (int32_t s = 0; s < activeCount; s++)
    {
        const auto slot = static_cast<size_t>(s);
        if (boss.tentaclePhase[slot] == 1 || boss.tentaclePhase[slot] == 2)
        {
            float angle =
                (boss.tentacleSwipeStartAngle[slot] + boss.tentacleSwipeEndAngle[slot]) / 2.0F;
            if (boss.tentaclePhase[slot] == 2)
            {
                const float progress = std::clamp(
                    1.0F - boss.tentacleStateTimer[slot] / krakenTentacleSwipeDuration, 0.0F, 1.0F);
                angle += (boss.tentacleSwipeEndAngle[slot] - boss.tentacleSwipeStartAngle[slot]) *
                         (progress - 0.5F);
            }
            boss.tentacleSwipeEndAngle[slot] = angle;
            boss.tentacleHitPlayerThisSwipe[slot] = true;
            boss.tentaclePhase[slot] = 4;
            boss.tentacleStateTimer[slot] = krakenTentacleStaggerRetreatDuration;
            return;
        }
    }
}

void updateKrakenTentacle(Game& game, Boss& boss, float deltaTime)
{
    if (!boss.isKraken || boss.health <= 0)
    {
        return;
    }

    const int32_t activeCount = krakenActiveTentacleCount(boss.krakenEncounter);
    const bool allStaggered = std::all_of(
        boss.tentacleStaggered.begin(),
        boss.tentacleStaggered.begin() + activeCount,
        [](bool staggered) { return staggered; });
    if (allStaggered)
    {
        if (boss.krakenCoreExposedTimer > 0)
        {
            boss.krakenCoreExposedTimer -= deltaTime;
            if (boss.krakenCoreExposedTimer <= 0)
            {
                boss.krakenCoreExposedTimer = 0;
                for (int32_t slot = 0; slot < activeCount; slot++)
                {
                    boss.tentacleStaggered[static_cast<size_t>(slot)] = false;
                    boss.tentacleCooldownTimer[static_cast<size_t>(slot)] =
                        krakenTentacleCooldownDuration;
                }
                boss.krakenCoreExposureUsed = true;
            }
        }
        else if (!boss.krakenCoreExposureUsed)
        {
            boss.krakenCoreExposedTimer = krakenCoreExposureDuration;
            boss.krakenCoreExposureUsed = true;
            for (int32_t slot = 0; slot < Boss::krakenLimbSlots; slot++)
            {
                boss.limbPhase[static_cast<size_t>(slot)] = 0;
                boss.limbPod[static_cast<size_t>(slot)] = -1;
                boss.limbThrowPhase[static_cast<size_t>(slot)] = 0;
            }
        }
        return;
    }

    updateKrakenSnatch(game, boss, deltaTime);

    for (int32_t s = 0; s < activeCount; s++)
    {
        const auto slot = static_cast<size_t>(s);

        if (boss.tentacleStaggered[slot])
        {
            continue;
        }

        if (boss.tentacleCooldownTimer[slot] > 0)
        {
            boss.tentacleCooldownTimer[slot] -= deltaTime;
        }

        if (boss.tentaclePhase[slot] == 0)
        {
            continue;
        }

        boss.tentacleStateTimer[slot] -= deltaTime;

        if (boss.tentaclePhase[slot] == 1)
        {
            if (boss.tentacleStateTimer[slot] <= 0)
            {
                boss.tentaclePhase[slot] = 2;
                boss.tentacleStateTimer[slot] = krakenTentacleSwipeDuration;
            }
            continue;
        }

        if (boss.tentaclePhase[slot] == 2)
        {
            const float progress =
                std::clamp(1.0F - boss.tentacleStateTimer[slot] / krakenTentacleSwipeDuration, 0.0F, 1.0F);
            const float angle =
                boss.tentacleSwipeStartAngle[slot] +
                (boss.tentacleSwipeEndAngle[slot] - boss.tentacleSwipeStartAngle[slot]) * progress;
            const Vector2 tip =
                Vector2Add(boss.tentaclePortalPos[slot],
                          Vector2{.x = std::cos(angle) * krakenTentacleReach,
                                  .y = std::sin(angle) * krakenTentacleReach});

            if (!boss.tentacleHitPlayerThisSwipe[slot] && game.run.player.health > 0 &&
                CheckCollisionCircleLine(game.run.player.position,
                                         game.run.player.radius + krakenTentacleLineHalfWidth,
                                         boss.tentaclePortalPos[slot], tip))
            {
                const bool blocking = game.run.player.shieldActive;
                if (blocking)
                {
                    boss.tentacleSwipeEndAngle[slot] = angle;
                    boss.tentacleHitPlayerThisSwipe[slot] = true;
                    boss.tentaclePhase[slot] = 4;
                    boss.tentacleStateTimer[slot] = krakenTentacleStaggerRetreatDuration;
                    triggerBossShieldDashShake(game);
                    continue;
                }
                boss.tentacleHitPlayerThisSwipe[slot] = true;
                damagePlayer(game, enemyDamage(game, krakenTentaclePlayerDamage));
            }

            if (boss.tentacleStateTimer[slot] <= 0)
            {
                boss.tentaclePhase[slot] = 3;
                boss.tentacleStateTimer[slot] = krakenTentacleRetreatDuration;
            }
            continue;
        }

        if (boss.tentaclePhase[slot] == 3)
        {
            if (boss.tentacleStateTimer[slot] <= 0)
            {
                boss.tentaclePhase[slot] = 0;
                boss.tentacleCooldownTimer[slot] = krakenTentacleCooldownDuration;
            }
            continue;
        }

        if (boss.tentaclePhase[slot] == 4)
        {
            if (boss.tentacleStateTimer[slot] <= 0)
            {
                boss.tentaclePhase[slot] = 0;
                boss.tentacleCooldownTimer[slot] = krakenTentacleCooldownDuration;
                boss.tentacleStaggered[slot] = true;
                boss.tentacleStaggerTimer[slot] = krakenStaggerDuration(boss.krakenEncounter);
                boss.tentacleStaggerPos[slot] = Vector2Add(
                    krakenPodPosition(boss, s % 4),
                    Vector2Scale(Vector2Normalize(Vector2Subtract(
                                     krakenPodPosition(boss, s % 4),
                                     Vector2{.x = boss.position.x + boss.size.x / 2,
                                             .y = boss.position.y + boss.size.y / 2})),
                                 krakenStaggerGateOffset));
            }
        }
    }

    boss.tentacleCheckTimer -= deltaTime;
    if (boss.tentacleCheckTimer > 0 || boss.qteStunTimer > 0)
    {
        return;
    }
    boss.tentacleCheckTimer = krakenTentacleCheckInterval;

    // Every idle, off-cooldown tentacle gets its own roll, so encounters with more than one
    // active tentacle actually use them instead of leaving spares parked.
    for (int32_t s = 0; s < activeCount; s++)
    {
        const auto slot = static_cast<size_t>(s);
        if (!krakenTentacleSlotIdle(boss, s) || boss.tentacleCooldownTimer[slot] > 0)
        {
            continue;
        }
        if (static_cast<float>(GetRandomValue(0, 99)) / 100.0F >= krakenTentacleTriggerChance)
        {
            continue;
        }

        const float portalAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto portalDist = static_cast<float>(GetRandomValue(260, 380));
        boss.tentaclePortalPos[slot] = Vector2Add(
            game.run.player.position,
            Vector2{.x = std::cos(portalAngle) * portalDist, .y = std::sin(portalAngle) * portalDist});

        const float aimAngle = std::atan2(game.run.player.position.y - boss.tentaclePortalPos[slot].y,
                                          game.run.player.position.x - boss.tentaclePortalPos[slot].x);
        const float halfArc = krakenTentacleSwipeArcDeg * DEG2RAD / 2.0F;
        boss.tentacleSwipeStartAngle[slot] = aimAngle - halfArc;
        boss.tentacleSwipeEndAngle[slot] = aimAngle + halfArc;

        boss.tentaclePhase[slot] = 1;
        boss.tentacleStateTimer[slot] = krakenTentacleEmergeDuration;
        boss.tentacleHitPlayerThisSwipe[slot] = false;
        playSFX(game, game.resources.sounds.bossWindUp);
    }
}

// --- Kraken Snatch and Dragged Under QTE (cutscene) ----------------------------
//
// Snatch: a cyan ring opens under the player; still inside it after the tell, an idle
// tentacle grabs the ship from a portal. That starts Dragged Under, or a short plain hold
// inside the QTE gap. Dragged Under: a Mash to break the grip, then prompts as the
// tentacle hauls the ship portal to portal while others lash in. Clean clear: the grip is
// severed, the finisher hits the core, then a Punish Window with the core open
// (qteStunTimer). Fail: flung out of a portal nearby with one tentacle hit.
// --------------------------------------------------------------------------

void releaseKrakenGrip(Boss& kraken)
{
    if (kraken.snatchSlot < 0)
    {
        return;
    }
    const auto slot = static_cast<size_t>(kraken.snatchSlot);
    const Vector2 toTip = Vector2Subtract(kraken.snatchPos, kraken.tentaclePortalPos[slot]);
    kraken.tentacleSwipeEndAngle[slot] = std::atan2(toTip.y, toTip.x);
    kraken.tentacleSwipeStartAngle[slot] = kraken.tentacleSwipeEndAngle[slot];
    kraken.tentaclePhase[slot] = 3;
    kraken.tentacleStateTimer[slot] = krakenTentacleRetreatDuration;
    kraken.snatchSlot = -1;
    kraken.snatchPhase = 0;
}

void flingFromKrakenGrip(Game& game, Boss& kraken)
{
    releaseKrakenGrip(kraken);
    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(150, 250));
    auto& player = game.run.player;
    spawnKillExplosion(game, player.position, Palette::ElementFreeze, 12, 1.0F);
    player.position = Vector2Add(player.position, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
    spawnKillExplosion(game, player.position, Palette::ElementFreeze, 12, 1.0F);
    damagePlayer(game, enemyDamage(game, krakenTentaclePlayerDamage));
    playSFX(game, game.resources.sounds.nerveFizzle);
}

void beginKrakenDraggedUnder(Game& game, Boss& kraken)
{
    bossQteBegin(game, kraken.instanceId, QteKind::DraggedUnder);
    game.run.qtePrompt = QtePrompt::Mash;
    game.run.qteTimer = UpdateConstants::krakenDragMashDuration;
    game.run.qteTimerMax = UpdateConstants::krakenDragMashDuration;
    game.run.qteMashTarget = UpdateConstants::krakenDragMashPresses;
    game.run.qteThreatDir = Vector2{};
    kraken.grabPromptsLeft = UpdateConstants::krakenDragPrompts;
    kraken.grabDragTarget = kraken.tentaclePortalPos[static_cast<size_t>(kraken.snatchSlot)];
    playSFX(game, game.resources.sounds.slamBoom);
    triggerShake(game, 12, 0.4F);
}

void endKrakenDraggedUnder(Game& game, Boss& kraken, bool clean)
{
    bossQteEnd(game);
    kraken.grabPromptsLeft = 0;
    if (!clean)
    {
        flingFromKrakenGrip(game, kraken);
        return;
    }
    // The ship cuts itself free; the core stays open for the Punish Window.
    spawnKillExplosion(game, kraken.snatchPos, Palette::ElementFreeze, 24, 1.4F);
    releaseKrakenGrip(kraken);
    kraken.qteStunTimer = UpdateConstants::qteStunWindow;
    kraken.state = BossState::IDLE;
    kraken.attackTimer = UpdateConstants::qteStunWindow;
    playSFX(game, game.resources.sounds.critical);
}

void updateKrakenSnatch(Game& game, Boss& kraken, float deltaTime)
{
    auto& player = game.run.player;
    if (kraken.snatchPhase == 0)
    {
        kraken.snatchTimer -= deltaTime;
        if (kraken.snatchTimer > 0 || kraken.snatchSlot >= 0 || kraken.qteStunTimer > 0 || !qteSlotFree(game))
        {
            return;
        }
        for (int32_t s = 0; s < krakenActiveTentacleCount(kraken.krakenEncounter); s++)
        {
            if (krakenTentacleSlotIdle(kraken, s))
            {
                kraken.snatchSlot = s;
                kraken.tentaclePhase[static_cast<size_t>(s)] = 5; // reserved, drawn once it grabs
                kraken.snatchPhase = 1;
                kraken.snatchStateTimer = UpdateConstants::krakenSnatchTell;
                kraken.snatchPos = player.position;
                kraken.snatchTimer = static_cast<float>(GetRandomValue(UpdateConstants::krakenSnatchCooldownMin,
                                                                       UpdateConstants::krakenSnatchCooldownMax));
                playSFX(game, game.resources.sounds.bossWindUp);
                return;
            }
        }
        return;
    }

    kraken.snatchStateTimer -= deltaTime;
    if (kraken.snatchPhase == 1)
    {
        if (kraken.snatchStateTimer > 0)
        {
            return;
        }
        if (Vector2Distance(player.position, kraken.snatchPos) > UpdateConstants::krakenSnatchRadius + player.radius ||
            !qteSlotFree(game))
        {
            kraken.tentaclePhase[static_cast<size_t>(kraken.snatchSlot)] = 0;
            kraken.snatchSlot = -1;
            kraken.snatchPhase = 0;
            return; // left the ring: no grab
        }
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        kraken.tentaclePortalPos[static_cast<size_t>(kraken.snatchSlot)] =
            Vector2Add(player.position, Vector2{.x = std::cos(angle) * 320.0F, .y = std::sin(angle) * 320.0F});
        kraken.snatchPos = player.position;
        triggerShake(game, 8, 0.25F);
        if (bossQteCanStart(game, QteKind::DraggedUnder))
        {
            kraken.snatchPhase = 0;
            beginKrakenDraggedUnder(game, kraken);
            return;
        }
        kraken.snatchPhase = 2;
        kraken.snatchStateTimer = UpdateConstants::krakenSnatchHold;
        playSFX(game, game.resources.sounds.slamBoom);
        return;
    }
    // Plain grab: held fast, then the fail outcome.
    player.position = kraken.snatchPos;
    player.velocity = Vector2{};
    if (kraken.snatchStateTimer <= 0)
    {
        flingFromKrakenGrip(game, kraken);
    }
}

void updateKrakenDraggedUnder(Game& game, Boss& kraken, float deltaTime)
{
    auto& run = game.run;
    if (kraken.snatchSlot < 0)
    {
        endKrakenDraggedUnder(game, kraken, false);
        return;
    }
    const auto slot = static_cast<size_t>(kraken.snatchSlot);
    if (run.qteActionTimer <= 0)
    {
        run.player.position =
            Vector2Lerp(run.player.position, kraken.grabDragTarget, std::min(1.0F, deltaTime * 0.6F));
    }
    kraken.snatchPos = run.player.position;

    if (run.qtePrompt == QtePrompt::Mash)
    {
        if (qteReadInput(game, QtePrompt::Mash) > 0)
        {
            run.qteMashCount++;
            triggerShake(game, 3, 0.08F);
        }
        if (run.qteMashCount >= run.qteMashTarget)
        {
            run.qteFinisherCharge += UpdateConstants::qteFinisherChargeMash;
            run.qtePrompt = QtePrompt::None;
            run.qteTimer = UpdateConstants::krakenDragMashBeat;
            run.qteTimerMax = 0;
            playSFX(game, game.resources.sounds.critical);
        }
        else if (run.qteTimer <= 0)
        {
            endKrakenDraggedUnder(game, kraken, false);
        }
        return;
    }

    if (run.qtePrompt != QtePrompt::None)
    {
        const int32_t answer = qteReadInput(game, run.qtePrompt);
        if (answer > 0)
        {
            qteStartAction(game, run.qtePrompt, run.qteThreatDir);
            playSFX(game, game.resources.sounds.critical);
            run.qtePrompt = QtePrompt::None;
            run.qteTimer = UpdateConstants::krakenDragBeat;
            run.qteTimerMax = 0;
        }
        else if (answer < 0 || run.qteTimer <= 0)
        {
            endKrakenDraggedUnder(game, kraken, false);
        }
        return;
    }

    if (run.qteTimer > 0)
    {
        return; // beat between prompts
    }
    if (kraken.grabPromptsLeft > 0)
    {
        const int32_t done = UpdateConstants::krakenDragPrompts - kraken.grabPromptsLeft--;
        if (done > 0 && done % UpdateConstants::krakenDragPromptsPerPortal == 0)
        {
            // Hauled through the tear: out of a portal by the Kraken, toward the next.
            const Vector2 center{.x = kraken.position.x + kraken.size.x / 2,
                                 .y = kraken.position.y + kraken.size.y / 2};
            const float exitAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 exitDir{.x = std::cos(exitAngle), .y = std::sin(exitAngle)};
            spawnKillExplosion(game, run.player.position, Palette::ElementFreeze, 14, 1.0F);
            run.player.position = Vector2Add(center, Vector2Scale(exitDir, kraken.size.x / 2 + 260.0F));
            kraken.snatchPos = run.player.position;
            spawnKillExplosion(game, run.player.position, Palette::ElementFreeze, 14, 1.0F);
            const Vector2 pullDir = Vector2Rotate(exitDir, static_cast<float>(GetRandomValue(60, 120)) * DEG2RAD);
            kraken.tentaclePortalPos[slot] = Vector2Add(run.player.position, Vector2Scale(pullDir, 320.0F));
            kraken.grabDragTarget = kraken.tentaclePortalPos[slot];
            game.run.punctumThunderFlashTimer = punctumThunderFlashDuration; // visual only
            playSFX(game, game.resources.sounds.slamBoom);
            triggerShake(game, 10, 0.3F);
        }
        // Other tentacles lash in from any side.
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        run.qteThreatDir = Vector2{.x = std::cos(angle), .y = std::sin(angle)};
        run.qtePrompt = qtePromptForThreat(game, run.qteThreatDir, UpdateConstants::krakenDragParryChance);
        run.qteTimer = UpdateConstants::krakenDragPromptWindow;
        run.qteTimerMax = UpdateConstants::krakenDragPromptWindow;
        playSFX(game, game.resources.sounds.bossWindUp);
        return;
    }
    qteBeginFinisher(game); // every prompt answered
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
    if (!boss.isKraken || boss.health <= 0 || boss.krakenEncounter < 2 ||
        boss.krakenCoreExposedTimer > 0 ||
        std::all_of(boss.tentacleStaggered.begin(),
                    boss.tentacleStaggered.begin() + krakenActiveTentacleCount(boss.krakenEncounter),
                    [](bool staggered) { return staggered; }))
    {
        return;
    }

    const int32_t activeSlots = krakenActiveTentacleCount(boss.krakenEncounter);

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
            if (boss.limbCooldown[i] > 0 || !krakenTentacleSlotIdle(boss, i))
            {
                continue;
            }
            int32_t pod = GetRandomValue(0, 3);
            bool podTaken = true;
            while (podTaken)
            {
                podTaken = false;
                for (int32_t other = 0; other < Boss::krakenLimbSlots; other++)
                {
                    if (other != i && boss.limbPod[other] == pod)
                    {
                        podTaken = true;
                        pod = (pod + 1) % 4;
                        break;
                    }
                }
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

    segment.gunPopTimer = std::max(0.0F, segment.gunPopTimer - deltaTime);
    segment.segmentVolleyTimer -= deltaTime;
    if (segment.segmentVolleyTimer > 0)
    {
        return;
    }
    if (const Boss* head = findWreckwormHeadById(game, segment.segmentOwnerId);
        head != nullptr && head->wreckwormPunishTimer > 0)
    {
        return; // the stunned worm holds fire; the volley goes out once the window closes
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
    segment.gunPopTimer = wreckwormGunRetractDuration;
    game.run.bossProjectiles.push_back(BossProjectile{
        .position = Vector2Add(segCenter, Vector2Scale(direction, segment.size.x / 2 *
                                                                      wreckwormGunLength)),
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

void damageBoss(Game& game, Boss& boss, int32_t amount, bool applyShake, bool piercesTailWrap,
                bool hitsStaggeredKrakenTentacle)
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

    if (boss.isKraken && !hitsStaggeredKrakenTentacle && boss.krakenCoreExposedTimer <= 0 &&
        boss.qteStunTimer <= 0)
    {
        // The core only takes weapon damage during the coordinated tentacle-down exposure.
        boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
        return;
    }

    applyActiveElementalDebuffs(game, boss);

    if (boss.isBeltbreaker &&
        (boss.beltbreakerShielded || countAttachedAlivePlates(game, boss) > 0))
    {
        boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
        return;
    }
    if (boss.isBeltbreaker && boss.beltbreakerPunishTimer > 0)
    {
        amount = static_cast<int32_t>(std::ceil(static_cast<float>(amount) * UpdateConstants::beltbreakerPunishDamageMult));
    }
    if (boss.isWreckwormHead && boss.wreckwormPunishTimer > 0)
    {
        amount = static_cast<int32_t>(std::ceil(static_cast<float>(amount) * UpdateConstants::wreckwormPunishDamageMult));
    }
    if (boss.isSlagmaw && boss.slagmawPunishTimer > 0)
    {
        amount = static_cast<int32_t>(std::ceil(static_cast<float>(amount) * UpdateConstants::slagmawPunishDamageMult));
    }
    if (boss.qteStunTimer > 0)
    {
        amount = static_cast<int32_t>(std::ceil(static_cast<float>(amount) * UpdateConstants::qteStunDamageMult));
    }
    if (bossQteActive(game))
    {
        amount = static_cast<int32_t>(std::ceil(static_cast<float>(amount) * UpdateConstants::qteDamageMult));
    }

    if (boss.isWreckwormHead && boss.state == BossState::SHOOTING &&
        boss.attack == BossAttack::TailWrap && !piercesTailWrap && boss.segmentCount > 0 &&
        boss.plateCount * 2 >= boss.segmentCount)
    {
        boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
        return;
    }

    if (const float mult = bossDirectorDamageMult(boss); mult != 1.0F)
    {
        if (mult <= 0)
        {
            boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
            return;
        }
        amount = std::max(1, static_cast<int32_t>(static_cast<float>(amount) * mult));
    }

    // During a Constrict Punish Window the armour takes full damage.
    if (boss.isWreckwormSegment && boss.isArmorSegment)
    {
        const Boss* head = findWreckwormHeadById(game, boss.segmentOwnerId);
        if (head == nullptr || head->wreckwormPunishTimer <= 0)
        {
            amount = std::max(
                1, static_cast<int32_t>(static_cast<float>(amount) * wreckwormArmorDamageMult));
        }
    }

    boss.health -= amount;
    if (game.sandbox && game.sandboxBossNoDeath)
    {
        boss.health = std::max(1, boss.health);
    }
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
    if (boss.shockwaveSlowTimer > 0)
    {
        boss.shockwaveSlowTimer -= deltaTime;
    }

    if (boss.debuffStatic)
    {
        return;
    }

    if (boss.isBeltbreakerPlate &&
        (boss.plateAttached || boss.plateRushPhase != 0))
    {
        return;
    }

    if (boss.isWreckwormSegment)
    {
        return;
    }

    if (boss.isWreckwormHead && boss.wreckwormPunishTimer > 0)
    {
        boss.wreckwormPunishTimer -= deltaTime; // stunned: starts no attack (IDLE below)
    }
    if (boss.qteStunTimer > 0)
    {
        boss.qteStunTimer -= deltaTime; // stunned after a clean Rush: starts no attack (IDLE below)
    }
    if (boss.isSlagmaw)
    {
        if (boss.slagmawPunishTimer > 0)
        {
            boss.slagmawPunishTimer -= deltaTime; // choking: starts no attack (IDLE below)
        }
        // The maw tracks the player, held on the cone while Devour inhales.
        if (boss.state != BossState::SHOOTING || boss.attack != BossAttack::Devour)
        {
            const Vector2 toPlayer = Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));
            boss.slagmawFacing = Vector2Normalize(
                Vector2Lerp(boss.slagmawFacing, toPlayer, std::min(1.0F, deltaTime * 2.5F)));
        }
    }

    if (boss.usesDirector && updateBossDirector(game, boss, bossCenter, deltaTime))
    {
        return;
    }

    const bool enraged =
        boss.health > 0 &&
        static_cast<float>(boss.health) <= static_cast<float>(boss.maxHealth) * enrageHealthFrac;
    float rateMult = enraged ? enrageSpeedMult : 1.0F;

    if (boss.isBeltbreaker && boss.plateCount > 0)
    {
        // No minions left to hide behind: the core attacks faster the fewer plates remain.
        const float plateFrac =
            static_cast<float>(countAttachedAlivePlates(game, boss)) / static_cast<float>(boss.plateCount);
        rateMult = std::min(rateMult, Lerp(beltbreakerNoPlatesSpeedMult, 1.0F, plateFrac));
    }

    switch (boss.state)
    {
    case BossState::IDLE:
        boss.attackTimer -= deltaTime;

        if (boss.attackTimer <= 0 && boss.health > 0 && boss.wreckwormPunishTimer <= 0 &&
            boss.slagmawPunishTimer <= 0 && boss.qteStunTimer <= 0)
        {
            boss.attack = boss.moveset.at(static_cast<size_t>(
                GetRandomValue(0, static_cast<int32_t>(boss.moveset.size()) - 1)));
            boss.state = BossState::WINDING_UP;
            boss.stateTimer = bossWindupDuration(boss.attack) * rateMult;
            boss.beamShieldLatched = false;
            if (boss.isBeltbreaker && boss.attack == BossAttack::Barrage)
            {
                boss.stateTimer = rubbleBarrageTell; // core flashes amber
            }
            else if (boss.isWreckwormHead && !boss.krakenSnakeVariant &&
                     boss.attack == BossAttack::Barrage)
            {
                boss.stateTimer = sporeSpitTell; // head flares green
            }
            else if (boss.isBeltbreaker && boss.attack == BossAttack::GravityWell)
            {
                boss.stateTimer = ringCrushTell; // plates contract round the core
            }
            else if (boss.isSlagmaw && boss.attack == BossAttack::Barrage)
            {
                boss.stateTimer = slagSprayTell; // body flares gold
            }

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
                boss.color = boss.isBeltbreaker ? Palette::ShatteredBeltAccent
                             : boss.isWreckwormHead && !boss.krakenSnakeVariant
                                 ? Palette::RustbloomAccent
                                 : Palette::BossSpread;
                break;
            case BossAttack::GravityWell:
                boss.color = boss.isBeltbreaker ? Palette::ShatteredBeltAccent : Palette::StructMid;
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
            case BossAttack::Devour:
                boss.color = Palette::Charge;
                break;
            case BossAttack::Count:
                break;
            }

            playSFX(game, game.resources.sounds.bossWindUp);
        }
        break;
    case BossState::WINDING_UP:
        boss.stateTimer -= deltaTime;
        boss.targetPosition = bossAimPoint(game, boss);

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
            const Vector2 step = Vector2Scale(boss.chargeVelocity,
                                              deltaTime * (boss.shockwaveSlowTimer > 0 ? boss.shockwaveSlowMult : 1.0F));
            boss.position = Vector2Add(boss.position, step);
            tryBeginRushQte(game, boss, bossCenter);
            if (boss.isSlagmaw)
            {
                // Magma Ram: burning pools along the charge, evenly spaced however far a frame moves.
                const float stepLength = Vector2Length(step);
                boss.magmaTrailDist += stepLength;
                while (stepLength > 0 && boss.magmaTrailDist >= magmaPoolSpacing)
                {
                    boss.magmaTrailDist -= magmaPoolSpacing;
                    const Vector2 at = Vector2Subtract(
                        Vector2Add(bossCenter, step), Vector2Scale(step, boss.magmaTrailDist / stepLength));
                    game.run.magmaPools.push_back(MagmaPool{.position = at, .life = magmaPoolLife});
                }
            }
        }

        if (boss.attack == BossAttack::Devour)
        {
            updateSlagmawDevour(game, boss, deltaTime);
        }

        if (boss.attack == BossAttack::DashRush)
        {
            boss.barrageTimer -= deltaTime;
            if (boss.slamHit)
            {
                tryBeginRushQte(game, boss, bossCenter);
                boss.position =
                    Vector2Add(boss.position, Vector2Scale(boss.chargeVelocity,
                                            deltaTime * (boss.shockwaveSlowTimer > 0 ? boss.shockwaveSlowMult : 1.0F)));
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
                    Vector2Add(boss.position, Vector2Scale(boss.chargeVelocity,
                                            deltaTime * (boss.shockwaveSlowTimer > 0 ? boss.shockwaveSlowMult : 1.0F)));
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

                const bool isSporeSpit = boss.isWreckwormHead && !boss.krakenSnakeVariant;
                const bool isWreckworm =
                    !isSporeSpit && (boss.isWreckwormHead || boss.isWreckwormSegment);
                constexpr std::array<Color, 3> fluidColors{Palette::RustbloomAccent, Palette::Charge,
                                                            Palette::Accent};
                // Spores leave the slime head's mouth, which turns to face the player; Slag
                // Spray sprays from Slagmaw's maw in a loose fan.
                const Vector2 muzzle =
                    isSporeSpit    ? Vector2Add(bossCenter, Vector2Scale(direction, boss.size.x * 0.3F))
                    : boss.isSlagmaw ? slagmawMawCenter(boss)
                                     : bossCenter;
                const Vector2 shotDir =
                    boss.isSlagmaw
                        ? Vector2Rotate(direction, static_cast<float>(GetRandomValue(-100, 100)) / 100.0F *
                                                       slagSpraySpreadDeg * DEG2RAD)
                        : direction;
                game.run.bossProjectiles.push_back(BossProjectile{
                    .position = muzzle,
                    .velocity = Vector2Scale(shotDir, barrageProjSpeed * attackScale),
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
                    .isShard = boss.isBeltbreaker,
                    .isSpore = isSporeSpit,
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
                game.run.bossProjectiles.back().isEmber = boss.isSlagmaw;
                playSFX(game, game.resources.sounds.spreadBurst);
            }
        }

        if (boss.attack == BossAttack::GravityWell && !boss.isBeltbreaker)
        {
            const Vector2 toBoss = Vector2Subtract(bossCenter, game.run.player.position);
            if (Vector2Length(toBoss) > 0)
            {

                const Vector2 pull = Vector2Scale(Vector2Normalize(toBoss),
                                                  game.run.player.speed * gravityWellPullFraction);
                game.run.player.position = Vector2Add(game.run.player.position,
                                                      Vector2Scale(pull, deltaTime * frameScale));
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
                plate.chargeVelocity =
                    Vector2Scale(dir, chargeDashSpeed * beltbreakerPlateAttackScale);
                plate.dashHitPlayer = false;
            }
        }

        if (boss.attack == BossAttack::BurrowCharge)
        {
            boss.position = Vector2Add(boss.position, Vector2Scale(boss.chargeVelocity,
                                            deltaTime * (boss.shockwaveSlowTimer > 0 ? boss.shockwaveSlowMult : 1.0F)));
            tryBeginRushQte(game, boss, bossCenter);
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
            const float releaseStart = wreckwormCoilClampAssembleDuration + wreckwormCoilClampDuration;

            // Closing on a player who did not slip out through the gap starts Constrict.
            if (!boss.krakenSnakeVariant && elapsed >= wreckwormCoilClampAssembleDuration &&
                elapsed < releaseStart && bossQteCanStart(game, QteKind::Constrict))
            {
                const Vector2 fromCenter = Vector2Subtract(game.run.player.position, boss.targetPosition);
                const float dist = Vector2Length(fromCenter);
                const float playerDeg = std::atan2(fromCenter.y, fromCenter.x) * RAD2DEG;
                // Dead centre there is no side to slip out of.
                const bool inGap = dist > wreckwormCoilClampEndRadius / 2.0F &&
                                   std::abs(std::remainder(playerDeg - boss.coilClampGapDeg, 360.0F)) <
                                       wreckwormCoilClampGapDeg / 2.0F;
                if (dist <= radius && radius - dist <= UpdateConstants::wreckwormConstrictCatchBand &&
                    !inGap)
                {
                    beginWreckwormConstrict(game, boss);
                }
            }

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
                    const float deg = wreckwormCoilSegmentDeg(boss, i);
                    float reach = radius;
                    if (i == boss.constrictLungeSegment)
                    {
                        // Constrict strike: rear out, then lunge onto the player at the timeout.
                        const float frac = boss.constrictLungeFrac;
                        const float rear = radius + UpdateConstants::wreckwormConstrictLungeRear;
                        reach = frac < UpdateConstants::wreckwormConstrictLungeRearFrac
                                    ? Lerp(radius, rear, frac / UpdateConstants::wreckwormConstrictLungeRearFrac)
                                    : Lerp(rear, 0.0F,
                                           (frac - UpdateConstants::wreckwormConstrictLungeRearFrac) /
                                               (1.0F - UpdateConstants::wreckwormConstrictLungeRearFrac));
                    }
                    const Vector2 ringCenter = Vector2Add(
                        boss.targetPosition, Vector2{.x = std::cos(deg * DEG2RAD) * reach,
                                                     .y = std::sin(deg * DEG2RAD) * reach});
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
        boss.color = boss.isBeltbreaker ? Palette::ShatteredBeltAccent : Palette::BossSpread;
        break;
    case BossAttack::GravityWell:
        boss.color = boss.isBeltbreaker ? Palette::ShatteredBeltAccent : Palette::StructMid;
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
    case BossAttack::Devour:
        boss.color = Palette::Charge;
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
        boss.dashHitPlayer = false;
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
        boss.stateTimer = boss.isBeltbreaker ? ringCrushDuration(boss) : gravityWellDuration;
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
    case BossAttack::Devour:
        boss.stateTimer = devourDuration;
        boss.devourDir = boss.slagmawFacing;
        playSFX(game, game.resources.sounds.nerveCharge);
        triggerShake(game, 6, 0.4F);
        break;
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
