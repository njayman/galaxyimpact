#include "boss_internal.hpp"

void spawnSlagmaw(Game& game, int32_t wave)
{
    game.run.bossSpawnCount++;

    const int32_t tier = wave / megaBossWaveInterval;

    std::vector<BossAttack> moveset{BossAttack::Barrage,    BossAttack::ChargeDash,
                                    BossAttack::MeteorHell, BossAttack::MeteorSwarm,
                                    BossAttack::Devour,     BossAttack::Spread,
                                    BossAttack::BurrowCharge};
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

    constexpr float slagmawSize = slagmawHeadSize;
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

    boss.moleHidden = true; // its first appearance is out of a magma blob near the ship
    boss.moleUnder = 1;
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
    const float halfSize = original.size.x * 0.75F;
    const int32_t childGeneration = original.slagmawBreakGeneration + 1;
    const float generationMult = static_cast<float>(1 << std::clamp(childGeneration, 0, 8));
    const auto halfHealth =
        std::max(1, static_cast<int32_t>(static_cast<float>(original.maxHealth) / 3.0F * generationMult));

    for (int32_t i = 0; i < 2; i++)
    {
        game.run.bossSpawnCount++;

        std::vector<BossAttack> moveset{BossAttack::Barrage,      BossAttack::ChargeDash,
                                        BossAttack::MeteorHell,   BossAttack::MeteorSwarm,
                                        BossAttack::HomingBarrage, BossAttack::MeteorHell,
                                        BossAttack::Spread,        BossAttack::BurrowCharge};
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

// Slag gobs thrown out in a ring, from a breach or the jaws coming up.
void throwSlagRing(Game& game, Vector2 at, int32_t count)
{
    const float start = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    for (int32_t i = 0; i < count; i++)
    {
        const float a = start + static_cast<float>(i) * 2.0F * PI / static_cast<float>(count);
        const Vector2 dir{.x = std::cos(a), .y = std::sin(a)};
        game.run.bossProjectiles.push_back(BossProjectile{.position = Vector2Add(at, Vector2Scale(dir, 40.0F)),
                                                          .velocity = Vector2Scale(dir, barrageProjSpeed * 0.7F),
                                                          .radius = 7,
                                                          .homing = false,
                                                          .active = true,
                                                          .fromPlayer = false,
                                                          .damage = enemyDamage(game, crossfireProjectileDamage),
                                                          .health = 1,
                                                          .isMeteor = true});
    }
}

// A point in the magma blobs (not a cave) round `near`, else the last try.
auto findMagmaBlob(Vector2 near, float minDist, float maxDist) -> Vector2
{
    Vector2 p = near;
    for (int32_t i = 0; i < 40; i++)
    {
        const float a = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const float d = Lerp(minDist, maxDist, static_cast<float>(GetRandomValue(0, 100)) / 100.0F);
        p = Vector2Add(near, Vector2{.x = std::cos(a) * d, .y = std::sin(a) * d});
        if (!isSolarForgeCaveOpen(p))
        {
            return p;
        }
    }
    return p;
}

// While any Slagmaw lives the magma blobs thicken; they ease back once it dies.
void updateMagmaDensity(Game& game, float deltaTime)
{
    const bool slagmaw = std::ranges::any_of(game.run.bosses, [](const Boss& b) { return b.isSlagmaw && b.health > 0; });
    const float target = slagmaw ? slagmawCaveWidthScale : 1.0F;
    const float now = solarForgeCaveWidthScale();
    const float step = magmaDensityRate * deltaTime;
    setSolarForgeCaveWidthScale(now < target ? std::min(target, now + step) : std::max(target, now - step));
}

// Lava mole, every frame: dive or surface, spin the drill, and the collar
// blooms open for good below the bloom HP.
void updateLavaMole(Game& game, Boss& boss, Vector2 bossCenter, float deltaTime)
{
    const bool under = (boss.state == BossState::WINDING_UP &&
                        (boss.attack == BossAttack::Devour || boss.attack == BossAttack::BurrowCharge)) ||
                       (boss.state == BossState::SHOOTING && boss.attack == BossAttack::BurrowCharge) ||
                       boss.moleHidden;
    if (boss.moleHidden && boss.state == BossState::IDLE)
    {
        if (boss.moleRumble > 0)
        {
            boss.moleRumble -= deltaTime;
            if (boss.moleRumble <= 0)
            {
                boss.moleHidden = false; // peeks out of the blob
                boss.attackTimer = boss.rampageDashesLeft > 0 ? 0.0F
                                                              : static_cast<float>(GetRandomValue(6, 12)) / 10.0F;
                spawnBlastRing(game, bossCenter, boss.size.x * 0.5F + 60.0F);
                playSFX(game, game.resources.sounds.rockBreak);
            }
        }
        else
        {
            boss.moleHideTimer -= deltaTime;
            if (boss.moleHideTimer <= 0 && boss.moleUnder >= 0.99F)
            {
                const Vector2 spot = findMagmaBlob(game.run.player.position, moleSpotMin, moleSpotMax);
                boss.position = Vector2Subtract(spot, Vector2{.x = boss.size.x / 2, .y = boss.size.y / 2});
                boss.moleRumble = moleRumbleTime;
            }
        }
    }
    boss.moleUnder = std::clamp(boss.moleUnder + (under ? 1.0F : -1.0F) * moleDiveRate * deltaTime, 0.0F, 1.0F);
    if (under)
    {
        boss.moleDrill += deltaTime * 6.0F; // the drill spins up underground
    }
    boss.moleDrill += deltaTime * 1.5F;

    if (!boss.slagmawBloomed && boss.health > 0 &&
        static_cast<float>(boss.health) <= static_cast<float>(boss.maxHealth) * slagmawBloomHealthFrac)
    {
        boss.slagmawBloomed = true;
        spawnBlastRing(game, bossCenter, 340.0F);
        throwSlagRing(game, bossCenter, 16);
        triggerShake(game, 10, 0.5F);
        playSFX(game, game.resources.sounds.slamBoom);
    }
}

// Surfacing with a bang: a blast ring, slag thrown out, and a hit on a ship too close.
void moleBreach(Game& game, Boss& boss, Vector2 at, float radius)
{
    boss.moleUnder = 0;
    spawnBlastRing(game, at, radius + 80.0F);
    throwSlagRing(game, at, eruptSlagCount);
    triggerShake(game, 9, 0.35F);
    playSFX(game, game.resources.sounds.slamBoom);
    auto& player = game.run.player;
    if (player.health > 0 && !player.dashing && player.immunityTimer <= 0 &&
        Vector2Distance(player.position, at) <= radius + player.radius)
    {
        damagePlayer(game, enemyDamage(game, devourBiteDamage));
    }
}

// Erupt: the jaws come up through the ring. A ship still inside is swallowed (the Boss QTE),
// or bitten and thrown clear while the QTE is on its gap. A dash through the moment escapes.
void slagmawErupt(Game& game, Boss& boss)
{
    auto& player = game.run.player;
    const Vector2 toPlayer = Vector2Subtract(player.position, boss.eruptAt);
    boss.slagmawFacing = Vector2Length(toPlayer) > 1.0F ? Vector2Normalize(toPlayer) : Vector2{.x = 0, .y = -1};
    boss.devourDir = boss.slagmawFacing;
    // The maw, not the head's centre, comes up on the ring.
    const Vector2 center = Vector2Subtract(boss.eruptAt, Vector2Scale(boss.slagmawFacing, boss.size.x / 2 * slagmawMawOffset));
    boss.position = Vector2Subtract(center, Vector2{.x = boss.size.x / 2, .y = boss.size.y / 2});
    boss.stateTimer = eruptHold;
    const bool caught = player.health > 0 && !player.dashing && !bossQteActive(game) &&
                        Vector2Length(toPlayer) <= eruptRadius + player.radius;
    if (caught && bossQteCanStart(game, QteKind::Swallowed))
    {
        boss.moleUnder = 0;
        beginSlagmawSwallowed(game, boss);
        return;
    }
    moleBreach(game, boss, boss.eruptAt, eruptRadius);
}

// Petal Volley: the collar sheds crystal petals in a ring; they slow to a hover, then dart at
// the ship (updateProjectiles).
void shedPetals(Game& game, Boss& boss, Vector2 bossCenter)
{
    const int32_t count = boss.slagmawBloomed ? petalBloomCount : petalCount;
    const float start = std::atan2(boss.slagmawFacing.y, boss.slagmawFacing.x);
    for (int32_t i = 0; i < count; i++)
    {
        const float a = start + static_cast<float>(i) * 2.0F * PI / static_cast<float>(count);
        const Vector2 dir{.x = std::cos(a), .y = std::sin(a)};
        game.run.bossProjectiles.push_back(
            BossProjectile{.life = petalLife,
                           .position = Vector2Add(bossCenter, Vector2Scale(dir, boss.size.x * 0.55F)),
                           .velocity = Vector2Scale(dir, petalOutSpeed),
                           .radius = 9,
                           .homing = false,
                           .active = true,
                           .fromPlayer = false,
                           .damage = enemyDamage(game, crossfireProjectileDamage),
                           .health = 1,
                           .isPetal = true});
    }
    spawnBlastRing(game, bossCenter, boss.size.x * 0.5F + 120.0F);
    playSFX(game, game.resources.sounds.spreadBurst);
}

// Magma pools along a charge or tunnel, evenly spaced however far a frame moves.
void dropMagmaTrail(Game& game, Boss& boss, Vector2 bossCenter, Vector2 step)
{
    if (!boss.isSlagmaw)
    {
        return;
    }
    const float stepLength = Vector2Length(step);
    boss.magmaTrailDist += stepLength;
    while (stepLength > 0 && boss.magmaTrailDist >= magmaPoolSpacing)
    {
        boss.magmaTrailDist -= magmaPoolSpacing;
        const Vector2 at =
            Vector2Subtract(Vector2Add(bossCenter, step), Vector2Scale(step, boss.magmaTrailDist / stepLength));
        game.run.magmaPools.push_back(MagmaPool{.position = at, .life = magmaPoolLife});
    }
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
