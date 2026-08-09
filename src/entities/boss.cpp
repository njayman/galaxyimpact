#include "update.hpp"

#include "democonfig.hpp"
#include "raymath.h"
#include "update_constants.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

auto sampleBossMoveset(int count) -> std::vector<BossAttack>
{
    std::vector<BossAttack> pool;
    pool.reserve(bossAttackCount);
    for (int i = 0; i < bossAttackCount; i++)
    {
        if (const auto attack = static_cast<BossAttack>(i); DemoConfig::isBossAttackAllowed(attack))
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

    // M11: the wave-100 placeholder final boss is a deliberately-picked, stat-boosted BossType
    // rather than a random one — a bespoke final-boss shape/identity is a later pass.
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
                             type.healthMult * finalBossMult * enemyHealthMult);
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

// M11: wave 100 (and every 100th wave after, once infinite mode is unlocked) spawns this single
// stat-boosted placeholder boss instead of a normal mega-boss.
void spawnFinalBossWave(Game& game)
{
    game.run.bossSpawnCount++;
    spawnBossInstance(game, megaBossHealthMult, megaBossSizeMult, true, false, true);
    playSFX(game, game.resources.sounds.bossWindUp);
}

// The Beltbreaker: Shattered Belt's signature boss, replaces the generic random-BossType pick at
// waves 10/20/25. Wave 25 (the biome's last wave) is the hardest appearance and a transition beat
// into Rustbloom - see work/galaxy-impact/biomes/bosses.md in the brain.md vault for full design.
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

    // Regular megabosses spawn far out (1200-1500) since they close the gap fast on their own.
    // The Beltbreaker detaches plates that used to despawn if they ended up too far from the
    // player, so it spawns closer to keep the whole fight on-screen from the start.
    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(500, 650));
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

    const auto health = static_cast<int32_t>(static_cast<float>(500 + tier * bossHpPerTier) *
                                             megaBossHealthMult * enemyHealthMult);

    // Barrage is the real damage every appearance has from wave 1 - GravityWell alone (its old
    // sole wave-10 move) just drags the player in without ever threatening them, which read as
    // "the boss doesn't attack." Beam joins at wave 20+ as the harder-to-dodge follow-up.
    // PlateHurl is a deliberate attack on top of the "at whim" harassment excursions - the core
    // can launch its whole ring at once, not just pick one plate off.
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

    // Plates are real Boss entries (see boss.hpp) so they get full weapon collision for free and
    // an explicit owner id instead of a synthetic hit-test. Spawned attached, orbiting the core,
    // from the moment the core exists - they detach on their own once the shield finishes
    // charging (see updateBeltbreakerCore) rather than needing a separate spawn step later.
    const auto plateHealth =
        static_cast<int32_t>(UpdateConstants::beltbreakerPlateHealthPerTier * enemyHealthMult);
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

// The Wreckworm: Rustbloom's signature boss, replaces the generic random-BossType pick at waves
// 30/40/50. Wave 50 (the biome's last wave) is the hardest appearance and a transition beat into
// Solar Forge - see work/galaxy-impact/biomes/bosses.md in the brain.md vault for full design.
void spawnWreckworm(Game& game, int32_t wave)
{
    game.run.bossSpawnCount++;

    const int32_t tier = wave / megaBossWaveInterval;
    int32_t segmentCount = 8;
    bool burrowChargeUnlocked = false;
    std::vector<float> moltThresholds{0.5F};
    if (wave >= 50)
    {
        segmentCount = 10;
        burrowChargeUnlocked = true;
        moltThresholds = {0.66F, 0.33F};
    }
    else if (wave >= 40)
    {
        segmentCount = 9;
        burrowChargeUnlocked = true;
    }
    // Molt pops thresholds off the back as health falls, so they're listed descending above.

    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(500, 650));
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

    const auto health = static_cast<int32_t>(static_cast<float>(500 + tier * bossHpPerTier) *
                                             megaBossHealthMult * enemyHealthMult);

    std::vector<BossAttack> moveset{BossAttack::Barrage, BossAttack::CoilClamp};
    if (burrowChargeUnlocked)
    {
        moveset.push_back(BossAttack::BurrowCharge);
    }
    const BossAttack firstAttack = moveset.front();

    constexpr float headSize = 96.0F;

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

    // Segments spawn already trailing the head in a line, alternating armor (dull, tanky, low
    // priority) and infected (bright, softer, the real target) - the same alternation the design
    // calls for, starting with an armor segment right behind the head.
    const Vector2 trailDir = Vector2Normalize(Vector2Subtract(spawnPos, game.run.player.position));
    const auto infectedHealth = static_cast<int32_t>(wreckwormSegmentHealthPerTier *
                                                     static_cast<float>(tier) * enemyHealthMult) +
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
                     .size = Vector2{.x = 40, .y = 40},
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
                     .isArmorSegment = armor};
        game.run.bosses.push_back(std::move(segment));
    }

    game.run.bosses.push_back(std::move(head));
    playSFX(game, game.resources.sounds.bossWindUp);
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

    // A segment never runs its own movement - it's entirely driven by updateWreckwormChain (or,
    // during CoilClamp, by the head's own attack code in updateBoss).
    if (boss.isWreckwormSegment)
    {
        return;
    }

    if (boss.isBeltbreakerPlate)
    {
        if (boss.plateAttached)
        {
            // Always surrounds and revolves with the core - recomputed every frame from the
            // core's live position/rotation, not just while visually "in the ring."
            if (Boss* core = findBeltbreakerById(game, boss.plateOwnerId); core != nullptr)
            {
                const Vector2 slotCenter = beltbreakerPlateSlotPosition(*core, boss.plateSlotIndex);
                boss.position = Vector2Subtract(
                    slotCenter, Vector2{.x = boss.size.x / 2, .y = boss.size.y / 2});
            }

            // Regen isn't instant, and a plate can't recover health and contribute to shield
            // generation at once (see countContributingPlates) - it just sits at whatever health
            // it came home with and climbs back to full over beltbreakerPlateRegenDuration.
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
                    // Reforms its OWN slot (not just any open one). Health is whatever it came
                    // back with - regen happens gradually while attached, not instantly here.
                    boss.plateAttached = true;
                    boss.plateReturning = false;
                    boss.plateExcursionTimer = 0;
                }
            }
            else
            {
                // Core is gone - nothing to go home to; keep fighting independently.
                boss.plateReturning = false;
            }
            return;
        }

        // Detached and not returning: an excursion (whim harassment or PlateHurl) forces its own
        // early return once the timer runs out, regardless of the core's shield state. A plate
        // detached via the full shield-generation cycle has no timer and just waits on the
        // core's own recall (see triggerBeltbreakerReturn).
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
        // Falls through to the same chase/strafe every other boss uses below - it's just an
        // independent boss for now.
    }

    // M12 dodge-window fix: EVERY attack now holds the boss still while actually firing (was only
    // 4 of 11 — Spread/MineDrop/SummonAdds/ShockwaveStomp/Barrage/GravityWell/HomingBarrage used
    // to strafe and chase at full speed *while* attacking, denying any weapon that needs proximity
    // or sustained aim a reliable window). The restless strafe/chase this function does for every
    // other state (IDLE, WINDING_UP) is deliberately untouched — bosses should still feel
    // aggressive and hard to pin down between attacks, just not *during* one.
    if (boss.state == BossState::SHOOTING || boss.recoveryTimer > 0)
    {
        return;
    }

    // Static: same as an enemy's - no movement, no attacks, no state-machine progress at all.
    if (boss.debuffStatic)
    {
        return;
    }

    // A frozen, off-screen-but-still-attacking Beltbreaker core read as broken (unavoidable
    // damage from a target the player can't even see) - it now moves and chases exactly like
    // every other boss (never camera-relative) in every phase, Shielded included.

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
    // M12: was 1.6F — sped the boss up exactly when players are trying to land the killing blow.
    // Kept modest rather than removed outright so enrage still reads as "more dangerous," just not
    // actively fighting the player's ability to finish the fight.
    const float speedMult = (enraged ? 1.25F : 1.0F) * (boss.debuffFreeze ? freezeSlowMult : 1.0F) *
                            (boss.isBeltbreakerPlate ? beltbreakerPlateMoveSpeedMult : 1.0F) *
                            (boss.isWreckwormHead ? boss.wreckwormSpeedMult : 1.0F);

    const float radialAmount =
        std::clamp((dist - bossEngageDistance) / bossEngageDistance, -1.5F, 1.5F);
    const float strafe = std::sin(static_cast<float>(GetTime()) * 0.9F + boss.strafePhase);

    Vector2 move = Vector2Add(Vector2Scale(dirToPlayer, radialAmount), Vector2Scale(perp, strafe));
    if (Vector2Length(move) > 0)
    {
        const float chaseSpeed = (game.run.player.speed * 1.15F + 1) * speedMult;
        move = Vector2Scale(Vector2Normalize(move), chaseSpeed);
        boss.position = Vector2Add(boss.position, Vector2Scale(move, deltaTime * frameScale));
    }
}

// boss.position is the top-left corner (as in drawBoss's bossCenter) - offset by size/2 so this
// matches where the core is actually drawn, not its hull's corner.
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

// "Protecting": any attached, alive plate blocks core damage regardless of its health - this is
// what damageBoss checks for core invulnerability.
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

// "Contributing": attached AND fully healed. A plate can't recover its own health and help
// charge the shield at the same time (see the regen tick in updateBossMovement) - it still
// physically protects the core while regenerating, it just doesn't speed up the shield.
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

// Generation complete: every attached plate cuts loose and becomes its own independent boss -
// same generic chase/attack code as any other boss (see updateBossMovement/updateBoss), just
// with a small stagger on attackTimer so all plates don't all open fire in the same instant.
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

// Only recalls THIS core's own plates (matched by Boss::instanceId) - with two Beltbreakers
// alive at once, a global "recall every plate in the game" would yank the other core's plates
// home early too.
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

// Sends a single attached plate out on a timed job (whim harassment or a PlateHurl launch) -
// unlike the full-cycle detach above, this plate forces its own return once durationSeconds
// elapses, regardless of whether the core's shield ever goes up.
void sendPlateOnExcursion(Boss& plate, float durationSeconds)
{
    plate.plateAttached = false;
    plate.plateExcursionTimer = durationSeconds;
}

// The core's shield-generation / detach / recall cycle, plus the "at whim" harassment excursion
// (not every plate cycle is the same to-and-fro - the core can peel one plate off to attack
// independent of the main generate/detach/return timing). Plate orbiting, homing, regen, and
// excursion countdowns are handled per-plate in updateBossMovement (every plate is its own Boss
// update).
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

        // "At whim": while still generating, the core can peel a single healthy attached plate
        // off to harass the player independently, without waiting for the whole ring to detach.
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

// Snake-follow: each segment chases the entity ahead of it (the head, or the previous alive
// segment), closing to wreckwormSegmentSpacing rather than snapping onto it - that's what gives
// the chain its lag. Skipped entirely during CoilClamp, which drives segment positions itself
// (see the CoilClamp handling in updateBoss) - a dead segment (molted) is simply skipped, the
// next one in line just follows the one before it instead.
void updateWreckwormChain(Game& game, Boss& head, float deltaTime)
{
    if (!head.isWreckwormHead)
    {
        return;
    }
    if (head.state == BossState::SHOOTING && head.attack == BossAttack::CoilClamp)
    {
        return;
    }

    Vector2 leaderCenter{.x = head.position.x + head.size.x / 2,
                         .y = head.position.y + head.size.y / 2};
    for (int32_t i = 0; i < head.segmentCount; i++)
    {
        Boss* seg = findWreckwormSegment(game, head.instanceId, i);
        if (seg == nullptr)
        {
            continue;
        }

        Vector2 segCenter{.x = seg->position.x + seg->size.x / 2,
                          .y = seg->position.y + seg->size.y / 2};
        const Vector2 toLeader = Vector2Subtract(leaderCenter, segCenter);
        const float dist = Vector2Length(toLeader);
        if (dist > wreckwormSegmentSpacing)
        {
            const Vector2 dir = Vector2Scale(toLeader, 1.0F / dist);
            const float step = std::min(dist - wreckwormSegmentSpacing,
                                        wreckwormChainFollowSpeed * head.wreckwormSpeedMult *
                                            deltaTime * frameScale);
            segCenter = Vector2Add(segCenter, Vector2Scale(dir, step));
            seg->position =
                Vector2Subtract(segCenter, Vector2{.x = seg->size.x / 2, .y = seg->size.y / 2});
        }
        leaderCenter = segCenter;
    }
}

// Molt: sheds the tail-most alive segments as the head's health crosses each scheduled threshold
// (see Boss::moltThresholds), then opens the "exposed lull" crit window - reuses recoveryTimer
// (holds movement) and a matching attackTimer reset (holds attacks) rather than adding new state.
void triggerWreckwormMolt(Game& game, Boss& head)
{
    constexpr int32_t moltShedCount = 2;
    int32_t shed = 0;
    for (int32_t i = head.segmentCount - 1; i >= 0 && shed < moltShedCount; i--)
    {
        Boss* seg = findWreckwormSegment(game, head.instanceId, i);
        if (seg != nullptr)
        {
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
        boss.debuffStatic = true;
        break;
    case ElementType::Freeze:
        boss.debuffFreeze = true;
        break;
    case ElementType::Confuse:
        boss.debuffConfuse = true;
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

// M18: centralizes every boss.health -= site so damage numbers and proportional hit-stop only
// need to live in one place instead of being duplicated at each of the ~10 call sites.
void damageBoss(Game& game, Boss& boss, int32_t amount, bool applyShake)
{
    applyActiveElementalDebuffs(game, boss);

    // The core is invulnerable while its shield is up, or while it still has any attached plate
    // protecting it - killing every attached plate (or waiting for the shield to finish charging
    // and detach them) is the only way through. See updateBeltbreakerCore.
    if (boss.isBeltbreaker &&
        (boss.beltbreakerShielded || countAttachedAlivePlates(game, boss) > 0))
    {
        boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
        return;
    }

    // Armor segments are dull, tanky, low-priority filler - "block shots" reads here as taking a
    // fraction of the hit rather than a full binary block, so they're still worth chipping in a
    // pinch, just clearly not the efficient target.
    if (boss.isWreckwormSegment && boss.isArmorSegment)
    {
        amount = std::max(
            1, static_cast<int32_t>(static_cast<float>(amount) * wreckwormArmorDamageMult));
    }

    boss.health -= amount;
    boss.hitFlashTimer = UpdateConstants::hitFlashDuration;
    spawnDamageNumber(game, boss.position, amount);
    // Continuous-contact ticks (beam/orbit-blade/burn accumulators calling this every frame) used
    // to keep resetting the shake timer back to full before it could ever decay, reading as a
    // shake that never stops - those callers pass applyShake=false and rely on the initial
    // first-contact hit (which still shakes) instead.
    if (applyShake)
    {
        triggerShake(game, damageShakeIntensity(amount), damageShakeDuration);
    }

    // Hit-stop scales with how big a bite this hit took out of the boss's max health, so a
    // single heavy attack (e.g. a nerve burst or evolved weapon) reads as a "heavy" hit without
    // needing every source of boss damage to hand-tune its own hitstop duration.
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
    // Static: no state-machine progress at all, mirroring updateBossMovement's early return.
    if (boss.debuffStatic)
    {
        return;
    }

    // An attached plate is protecting the core, not attacking - it only starts running its own
    // attack moveset once detached (see updateBeltbreakerCore / detachBeltbreakerPlates).
    if (boss.isBeltbreakerPlate && boss.plateAttached)
    {
        return;
    }

    // A segment never attacks on its own - only the Wreckworm head runs an attack state machine
    // (see the Architecture pattern note: reuse existing systems instead of per-segment timers).
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

        // Confuse: too disoriented to commit to a new attack - it'll keep winding down its
        // timer but never actually pull the trigger until the debuff clears.
        if (boss.attackTimer <= 0 && boss.health > 0 && !boss.debuffConfuse)
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
            case BossAttack::MineDrop:
                boss.color = Palette::AccentDim;
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
                // Teleports now, during the wind-up, rather than at the moment it fires - the
                // generic WINDING_UP charge-particle draw (drawBoss) already centers on the
                // boss's live position, so this alone doubles as the "ground-crack" telegraph at
                // the entry point without any bespoke draw code.
                const float burrowAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
                const auto burrowDist = static_cast<float>(GetRandomValue(550, 700));
                const Vector2 entry = Vector2Add(game.run.player.position,
                                                 Vector2{.x = std::cos(burrowAngle) * burrowDist,
                                                         .y = std::sin(burrowAngle) * burrowDist});
                boss.position =
                    Vector2Subtract(entry, Vector2{.x = boss.size.x / 2, .y = boss.size.y / 2});
                break;
            }
            case BossAttack::CoilClamp:
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
                    breakAsteroid(game.run.asteroids, asteroid);
                }
            }

            for (size_t i = 0; i < game.run.enemies.size(); i++)
            {
                const auto& e = game.run.enemies.at(i);
                if (e.active && !e.phased &&
                    Vector2Distance(bossCenter, e.position) <=
                        radius + enemyKinds.at(static_cast<size_t>(e.kind)).radius)
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
                                       waveEnemyScale(game)));
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
                                       .damage = crossfireProjectileDamage,
                                       .health = roundProjectileHealth});
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

        if (boss.attack == BossAttack::Barrage)
        {
            boss.barrageTimer -= deltaTime;
            if (boss.barrageTimer <= 0)
            {
                // A plate's Barrage is a mini version of the core's - slower shots, fired less
                // often, for less damage, not a full-strength boss attack from something meant
                // to be an individually-manageable threat.
                const float attackScale =
                    boss.isBeltbreakerPlate ? beltbreakerPlateAttackScale : 1.0F;
                boss.barrageTimer = barrageFireInterval / attackScale;
                const Vector2 direction =
                    Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));

                game.run.bossProjectiles.push_back(BossProjectile{
                    .position = bossCenter,
                    .velocity = Vector2Scale(direction, barrageProjSpeed * attackScale),
                    .radius = 6,
                    .homing = false,
                    .active = true,
                    .fromPlayer = false,
                    .damage = std::max(
                        1, static_cast<int32_t>(static_cast<float>(crossfireProjectileDamage) *
                                                attackScale)),
                    .health = 1});
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
                                   .damage = crossfireProjectileDamage,
                                   .health = 1});
                playSFX(game, game.resources.sounds.spreadBurst);
            }
        }

        if (boss.attack == BossAttack::GravityWell)
        {
            const Vector2 toBoss = Vector2Subtract(bossCenter, game.run.player.position);
            if (Vector2Length(toBoss) > 0)
            {
                // Pull now scales with the player's own speed instead of a flat constant - only
                // holding the exact opposite direction lets them crawl free; any other input
                // still nets them pulled in, which a flat pull well below player speed never
                // threatened.
                const Vector2 pull = Vector2Scale(Vector2Normalize(toBoss),
                                                  game.run.player.speed * gravityWellPullFraction);
                game.run.player.position = Vector2Add(game.run.player.position,
                                                      Vector2Scale(pull, deltaTime * frameScale));
            }

            // Getting dragged toward the core is far more dangerous if it still has its ring -
            // plates spin up into a fan (even while the core is shielded) on top of their normal
            // rotation in updateBeltbreakerCore, so running the wrong way (toward it) risks being
            // chewed up by the ring, not just the pull itself.
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
            // One-shot in effect: after the first SHOOTING frame every attached plate has
            // detached, so later frames of this same attack find nothing left to launch.
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
            const float progress =
                std::clamp(1 - boss.stateTimer / wreckwormCoilClampDuration, 0.0F, 1.0F);
            const float radius =
                wreckwormCoilClampStartRadius +
                (wreckwormCoilClampEndRadius - wreckwormCoilClampStartRadius) * progress;
            const float arcDeg = 360.0F - wreckwormCoilClampGapDeg;

            std::vector<Boss*> alive;
            for (auto& seg : game.run.bosses)
            {
                if (seg.isWreckwormSegment && seg.segmentOwnerId == boss.instanceId &&
                    seg.health > 0)
                {
                    alive.push_back(&seg);
                }
            }
            for (size_t i = 0; i < alive.size(); i++)
            {
                const float t = alive.size() > 1
                                    ? static_cast<float>(i) / static_cast<float>(alive.size() - 1)
                                    : 0.0F;
                const float deg =
                    boss.coilClampGapDeg + wreckwormCoilClampGapDeg / 2.0F + t * arcDeg;
                const Vector2 segCenter =
                    Vector2Add(boss.targetPosition, Vector2{.x = std::cos(deg * DEG2RAD) * radius,
                                                            .y = std::sin(deg * DEG2RAD) * radius});
                alive.at(i)->position = Vector2Subtract(
                    segCenter, Vector2{.x = alive.at(i)->size.x / 2, .y = alive.at(i)->size.y / 2});
            }
        }

        if (boss.stateTimer <= 0)
        {
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
            breakAsteroid(game.run.asteroids, asteroid);
        }
    }

    for (size_t i = 0; i < game.run.enemies.size(); i++)
    {
        const auto& e = game.run.enemies.at(i);
        if (e.active && !e.phased &&
            CheckCollisionCircleLine(e.position, enemyKinds.at(static_cast<size_t>(e.kind)).radius,
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
    case BossAttack::MineDrop:
        boss.color = Palette::AccentDim;
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

    const auto projectileHealth = static_cast<int32_t>(std::max(
        1.0F, static_cast<float>(baseProjectileHealth) * enemyHealthMult * waveEnemyScale(game)));

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
    case BossAttack::MineDrop:
        boss.stateTimer = 0.5F;
        playSFX(game, game.resources.sounds.slamBoom);
        duckBGM(game);
        for (int i = 0; i < mineDropCount; i++)
        {
            const float angle = static_cast<float>(i) * (360.0F / mineDropCount) * DEG2RAD;
            const auto dist = static_cast<float>(GetRandomValue(60, 140));
            const Vector2 pos = Vector2Add(
                bossCenter, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

            game.run.bossProjectiles.push_back(BossProjectile{.position = pos,
                                                              .velocity = Vector2{},
                                                              .radius = mineDropRadius,
                                                              .homing = false,
                                                              .active = true,
                                                              .fromPlayer = false,
                                                              .damage = mineDropDamage,
                                                              .health = projectileHealth});
        }
        break;
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
                breakAsteroid(game.run.asteroids, asteroid);
            }
        }
        for (size_t i = 0; i < game.run.enemies.size(); i++)
        {
            const auto& e = game.run.enemies.at(i);
            if (e.active && !e.phased &&
                Vector2Distance(bossCenter, e.position) <=
                    UpdateConstants::shockwaveStompRadius +
                        enemyKinds.at(static_cast<size_t>(e.kind)).radius)
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
        // Center is whatever boss.targetPosition last got set to during WINDING_UP (see
        // updateBoss) - a near-live read of the player's position at the moment the ring starts
        // closing, reusing the field instead of adding a dedicated one.
        boss.stateTimer = wreckwormCoilClampDuration;
        boss.coilClampGapDeg = static_cast<float>(GetRandomValue(0, 359));
        playSFX(game, game.resources.sounds.bossWindUp);
        break;
    case BossAttack::Count:
        break;
    }
}

