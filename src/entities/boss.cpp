#include "boss_internal.hpp"

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

// Turns `heading` toward `desired` by at most maxTurn radians; either may be zero length.
auto steerHeading(Vector2 heading, Vector2 desired, float maxTurn) -> Vector2
{
    if (Vector2LengthSqr(desired) < 0.0001F)
    {
        return heading;
    }
    desired = Vector2Normalize(desired);
    if (Vector2LengthSqr(heading) < 0.0001F)
    {
        return desired;
    }
    heading = Vector2Normalize(heading);
    const float turn = std::atan2(heading.x * desired.y - heading.y * desired.x, Vector2DotProduct(heading, desired));
    return Vector2Rotate(heading, std::clamp(turn, -maxTurn, maxTurn));
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

    if (boss.isSlagmaw)
    {
        return; // the lava mole never flies: it moves blob to blob under the magma (updateLavaMole)
    }
    if (false)
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
            // Keep the pattern's speed, but turn toward its direction instead of snapping to it.
            const float magnitude = Vector2Length(move);
            boss.moveHeading = steerHeading(boss.moveHeading, move, directorTurnRate * deltaTime);
            const float chaseSpeed = (game.run.player.speed * 1.15F + 1) * speedMult;
            boss.position = Vector2Add(boss.position,
                                       Vector2Scale(boss.moveHeading, magnitude * chaseSpeed * deltaTime * frameScale));
        }
        return;
    }
    if (boss.isWreckwormHead)
    {
        move = dirToPlayer;
    }
    else
    {
        // Sweep round the player at the engage distance, the orbit slowly reversing.
        const float radialAmount =
            std::clamp((dist - bossEngageDistance) / bossEngageDistance, -1.5F, 1.5F);
        const float orbit = std::sin(static_cast<float>(GetTime()) * 0.35F + boss.strafePhase);
        move = Vector2Add(Vector2Scale(dirToPlayer, radialAmount), Vector2Scale(perp, orbit * 1.4F));
    }

    if (Vector2Length(move) > 0)
    {
        const float chaseSpeed = (game.run.player.speed * 1.15F + 1) * speedMult;
        boss.moveHeading = steerHeading(boss.moveHeading, move,
                                        (boss.isWreckwormHead ? wreckwormTurnRate : bossTurnRate) * deltaTime);
        move = Vector2Scale(boss.moveHeading, chaseSpeed);
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
        // Cycles its attacks: Arm Lash, Portal Lash, Spear Rake, Feeding Strike (whose catch
        // starts Dragged Under).
        static int32_t next = 0;
        constexpr std::array<const char*, 4> names{"Arm Lash", "Portal Lash", "Spear Rake", "Feeding Strike"};
        boss.sandboxKrakenKind = next;
        boss.tentacleCheckTimer = 0;
        boss.qteStunTimer = 0;
        game.run.achievementToast = names.at(static_cast<size_t>(next));
        game.run.achievementToastTimer = 2.0F;
        next = (next + 1) % 4;
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
    if (boss.isSlagmaw && boss.moleUnder > 0.5F)
    {
        return; // ponytail: underground; shots still stop on the swell
    }
    if (boss.isSlagmaw && boss.slagmawBloomed)
    {
        amount = static_cast<int32_t>(std::ceil(static_cast<float>(amount) * slagmawBloomDamageMult));
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

// Boss state IDLE: one step.
static void updateBossIdle(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter, float rateMult)
{
    boss.attackTimer -= deltaTime;

    if (boss.attackTimer <= 0 && boss.health > 0 && boss.wreckwormPunishTimer <= 0 &&
        boss.slagmawPunishTimer <= 0 && boss.qteStunTimer <= 0 && bossOnScreen(game, bossCenter) &&
        !(boss.isSlagmaw && (boss.moleHidden || boss.moleUnder > 0.05F)))
    {
        boss.attack = boss.moveset.at(static_cast<size_t>(
            GetRandomValue(0, static_cast<int32_t>(boss.moveset.size()) - 1)));
        const bool pounceChained = boss.isSlagmaw && boss.rampageDashesLeft > 0;
        if (pounceChained)
        {
            boss.attack = BossAttack::ChargeDash;
            boss.rampageDashesLeft--;
        }
        else if (boss.isSlagmaw && boss.attack == BossAttack::ChargeDash)
        {
            boss.rampageDashesLeft = slagmawPounceCount - 1; // Pounce Frenzy
        }
        // A bare Beltbreaker has no plates for Ring Crush or Plate Hurl: it rampages
        // (chained dashes) or bursts a shard nova, at random.
        bool shardNova = false;
        if (boss.isBeltbreaker && beltbreakerBare(game, boss) &&
            (boss.attack == BossAttack::GravityWell || boss.attack == BossAttack::PlateHurl))
        {
            if (GetRandomValue(0, 1) == 0)
            {
                boss.attack = BossAttack::ChargeDash;
                boss.rampageDashesLeft = beltbreakerRampageDashes - 1;
            }
            else
            {
                boss.attack = BossAttack::Barrage;
                shardNova = true;
            }
        }
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
        else if (boss.isSlagmaw && boss.attack == BossAttack::Devour)
        {
            boss.stateTimer = eruptTell; // dives; the ring hunts the player
            boss.eruptAt = game.run.player.position;
        }
        else if (boss.isSlagmaw && boss.attack == BossAttack::BurrowCharge)
        {
            boss.stateTimer = tunnelTell;
        }
        else if (boss.isSlagmaw && boss.attack == BossAttack::Spread)
        {
            boss.stateTimer = petalTell; // the collar flares
        }
        else if (pounceChained)
        {
            boss.stateTimer = pounceChainTell;
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
            boss.barragePattern = boss.isBeltbreaker ? (shardNova ? 2 : 0) : GetRandomValue(0, 2);
            boss.barrageAngle = std::atan2(game.run.player.position.y - bossCenter.y,
                                           game.run.player.position.x - bossCenter.x);
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
            if (boss.isSlagmaw)
            {
                break; // dives where it is
            }

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
}

// Boss state WINDING_UP: one step.
static void updateBossWindup(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
{
    if (boss.windupTotal <= 0)
    {
        beginWindup(game, boss);
    }
    boss.stateTimer -= deltaTime;
    if (!isAimLockedAttack(boss.attack) || boss.stateTimer > aimLockTime)
    {
        boss.targetPosition = bossAimPoint(game, boss); // frozen for the last aimLockTime
    }
    if (boss.isSlagmaw && boss.attack == BossAttack::Devour)
    {
        if (boss.stateTimer > eruptLockTime)
        {
            boss.eruptAt = game.run.player.position;
        }
        if (boss.moleUnder > 0.9F) // the swell runs under toward the ring
        {
            const Vector2 to = Vector2Subtract(boss.eruptAt, bossCenter);
            const float len = Vector2Length(to);
            if (len > 1.0F)
            {
                boss.position = Vector2Add(boss.position,
                                           Vector2Scale(to, std::min(len, moleTunnelSpeed * deltaTime) / len));
            }
        }
    }

    if (boss.stateTimer <= 0)
    {
        startBossAttack(game, boss, bossCenter);
    }
}

// Slam: the firing phase, one step.
static void bossShootSlam(Game& game, float /*deltaTime*/, Boss& boss, Vector2 bossCenter)
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

// Spread: the firing phase, one step.
static void bossShootSpread(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
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

// MeteorHell: the firing phase, one step.
static void bossShootMeteorHell(Game& game, float deltaTime, Boss& boss, Vector2 /*bossCenter*/)
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
            static_cast<int32_t>(static_cast<float>(GetRandomValue(meteorHellSpawnPerBatchMin, meteorHellSpawnPerBatchMax)) *
                                 (boss.isSlagmaw ? slagmawSwarmMult : 1.0F));
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
                meteorHellProjSpeed * (boss.isSlagmaw ? slagmawShotSpeedMult : 1.0F));

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

// ChargeDash: the firing phase, one step.
static void bossShootChargeDash(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
{
    // The charge bends toward the player while they are still ahead of it.
    const Vector2 toPlayer = Vector2Subtract(game.run.player.position, bossCenter);
    if (!boss.isBeltbreakerPlate && !boss.isSlagmaw && Vector2DotProduct(toPlayer, boss.chargeVelocity) > 0)
    {
        const float speed = Vector2Length(boss.chargeVelocity);
        boss.chargeVelocity = Vector2Scale(
            steerHeading(boss.chargeVelocity, toPlayer, bossChargeCurveRate * deltaTime), speed);
    }
    const Vector2 step = Vector2Scale(boss.chargeVelocity,
                                      deltaTime * (boss.shockwaveSlowTimer > 0 ? boss.shockwaveSlowMult : 1.0F));
    boss.position = Vector2Add(boss.position, step);
    tryBeginRushQte(game, boss, bossCenter);
    dropMagmaTrail(game, boss, bossCenter, step);
}

// DashRush: the firing phase, one step.
static void bossShootDashRush(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
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

// Barrage: the firing phase, one step.
static void bossShootBarrage(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
{
    boss.barrageTimer -= deltaTime;
    if (boss.barrageTimer <= 0)
    {

        const float attackScale =
            boss.isBeltbreakerPlate ? beltbreakerPlateAttackScale : 1.0F;
        boss.barrageTimer = barrageFireInterval / attackScale / (boss.isSlagmaw ? slagmawSwarmMult : 1.0F);
        // Generic bosses: a turning spiral or gapped rings instead of the aimed stream.
        const bool generic = (!boss.isBeltbreaker && !boss.isBeltbreakerPlate && !boss.isWreckwormHead &&
                              !boss.isWreckwormSegment && !boss.isSlagmaw) ||
                             boss.isBeltbreaker; // a Beltbreaker's pattern is 0 unless shard nova
        if (generic && boss.barragePattern != 0)
        {
            constexpr float tau = 2.0F * std::numbers::pi_v<float>;
            const int32_t damage = enemyDamage(game, crossfireProjectileDamage);
            const auto shoot = [&](float angle)
            {
                const Vector2 dir{.x = std::cos(angle), .y = std::sin(angle)};
                game.run.bossProjectiles.push_back(BossProjectile{
                    .position = Vector2Add(bossCenter, Vector2Scale(dir, boss.size.x * 0.4F)),
                    .velocity = Vector2Scale(dir, barrageProjSpeed * 0.6F),
                    .radius = 6,
                    .homing = false,
                    .active = true,
                    .fromPlayer = false,
                    .damage = damage,
                    .health = 1,
                    .isShard = boss.isBeltbreaker});
            };
            if (boss.barragePattern == 1)
            {
                boss.barrageAngle += barrageSpiralStepDeg * DEG2RAD;
                for (int32_t arm = 0; arm < barrageSpiralArms; arm++)
                {
                    shoot(boss.barrageAngle + static_cast<float>(arm) * tau / barrageSpiralArms);
                }
            }
            else
            {
                spawnBlastRing(game, bossCenter, boss.size.x * 0.4F + 140.0F);
                boss.barrageTimer *= barrageRingIntervalMult;
                boss.barrageAngle += 23.0F * DEG2RAD;
                for (int32_t shot = barrageRingGap; shot < barrageRingCount; shot++)
                {
                    shoot(boss.barrageAngle + static_cast<float>(shot) * tau / barrageRingCount);
                }
            }
            playSFX(game, game.resources.sounds.spreadBurst);
        }
        else
        {
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
            .velocity = Vector2Scale(shotDir, barrageProjSpeed * attackScale *
                                             (boss.isSlagmaw ? slagmawShotSpeedMult
                                              : isSporeSpit  ? wreckwormShotSpeedMult
                                                             : 1.0F)),
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
}

// HomingBarrage: the firing phase, one step.
static void bossShootHomingBarrage(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
{
    boss.barrageTimer -= deltaTime;
    if (boss.barrageTimer <= 0)
    {
        boss.barrageTimer = homingBarrageFireInterval / (boss.isSlagmaw ? slagmawSwarmMult : 1.0F);
        const Vector2 direction =
            Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));
        game.run.bossProjectiles.push_back(
            BossProjectile{.position = bossCenter,
                           .velocity = Vector2Scale(direction, homingProjSpeed * (boss.isSlagmaw ? slagmawShotSpeedMult : 1.0F)),
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

// PlateHurl: the firing phase, one step.
static void bossShootPlateHurl(Game& game, float /*deltaTime*/, Boss& boss, Vector2 /*bossCenter*/)
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

// CoilClamp: the firing phase, one step.
static void bossShootCoilClamp(Game& game, float /*deltaTime*/, Boss& boss, Vector2 bossCenter)
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

// TailWrap: the firing phase, one step.
static void bossShootTailWrap(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
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

// Boss state SHOOTING: one step.
static void updateBossShooting(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter, float rateMult)
{
    boss.stateTimer -= deltaTime;

    if (boss.attack == BossAttack::Slam)
    {
        bossShootSlam(game, deltaTime, boss, bossCenter);
    }

    if (boss.attack == BossAttack::Spread && !boss.isSlagmaw)
    {
        bossShootSpread(game, deltaTime, boss, bossCenter);
    }

    if (boss.attack == BossAttack::MeteorHell)
    {
        bossShootMeteorHell(game, deltaTime, boss, bossCenter);
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
        bossShootChargeDash(game, deltaTime, boss, bossCenter);
    }


    if (boss.attack == BossAttack::DashRush)
    {
        bossShootDashRush(game, deltaTime, boss, bossCenter);
    }

    if (boss.attack == BossAttack::Barrage)
    {
        bossShootBarrage(game, deltaTime, boss, bossCenter);
    }

    if (boss.attack == BossAttack::HomingBarrage)
    {
        bossShootHomingBarrage(game, deltaTime, boss, bossCenter);
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
        bossShootPlateHurl(game, deltaTime, boss, bossCenter);
    }

    if (boss.attack == BossAttack::BurrowCharge)
    {
        const Vector2 step = Vector2Scale(boss.chargeVelocity,
                                          deltaTime * (boss.shockwaveSlowTimer > 0 ? boss.shockwaveSlowMult : 1.0F));
        boss.position = Vector2Add(boss.position, step);
        tryBeginRushQte(game, boss, bossCenter);
        dropMagmaTrail(game, boss, bossCenter, step); // Tunnel Run: the crust splits behind it
    }

    if (boss.attack == BossAttack::CoilClamp)
    {
        bossShootCoilClamp(game, deltaTime, boss, bossCenter);
    }

    if (boss.attack == BossAttack::TailWrap)
    {
        bossShootTailWrap(game, deltaTime, boss, bossCenter);
    }

    if (boss.stateTimer <= 0 && boss.isSlagmaw && boss.attack == BossAttack::BurrowCharge)
    {
        moleBreach(game, boss, bossCenter, tunnelBreachRadius);
    }
    if (boss.stateTimer <= 0 && boss.isBeltbreaker && boss.attack == BossAttack::ChargeDash &&
        boss.rampageDashesLeft > 0)
    {
        boss.rampageDashesLeft--; // rampage: straight into the next dash's tell
        boss.state = BossState::WINDING_UP;
        boss.stateTimer = beltbreakerRampageChainTell;
        return;
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
        if (boss.isSlagmaw)
        {
            boss.moleHidden = true; // dives back into the magma and moves to another blob
            boss.moleHideTimer = moleHideTime;
            boss.moleRumble = 0;
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
        // The maw tracks the player, held while it bites.
        if (boss.state != BossState::SHOOTING || boss.attack != BossAttack::Devour)
        {
            const Vector2 toPlayer = Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));
            boss.slagmawFacing = Vector2Normalize(
                Vector2Lerp(boss.slagmawFacing, toPlayer, std::min(1.0F, deltaTime * 2.5F)));
        }
        updateLavaMole(game, boss, bossCenter, deltaTime);
    }

    if (boss.state != BossState::WINDING_UP)
    {
        boss.windupTotal = 0;
    }
    if (boss.usesDirector && updateBossDirector(game, boss, bossCenter, deltaTime))
    {
        return;
    }

    const bool enraged =
        boss.health > 0 &&
        static_cast<float>(boss.health) <= static_cast<float>(boss.maxHealth) * enrageHealthFrac;
    float rateMult = enraged ? enrageSpeedMult : 1.0F;

    if (boss.isSlagmaw && boss.slagmawBloomed)
    {
        rateMult *= slagmawBloomRateMult;
    }
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
        updateBossIdle(game, deltaTime, boss, bossCenter, rateMult);
        break;
    case BossState::WINDING_UP:
        updateBossWindup(game, deltaTime, boss, bossCenter);
        break;
    case BossState::SHOOTING:
        updateBossShooting(game, deltaTime, boss, bossCenter, rateMult);
        break;
    }
}

auto isAimLockedAttack(BossAttack attack) -> bool
{
    return attack == BossAttack::Beam || attack == BossAttack::WormholeBeam || attack == BossAttack::ChargeDash ||
           attack == BossAttack::DashRush;
}

auto bossOnScreen(const Game& game, Vector2 bossCenter) -> bool
{
    return !isOutsideCameraView(game, bossCenter, -bossOnScreenInset);
}

// First frame of a wind-up, whoever started it: stretch beam and dash tells to their minimum and
// place the wormhole portal now, so its tell line can show where the beam will come from.
void beginWindup(Game& game, Boss& boss)
{
    if (boss.attack == BossAttack::Beam || boss.attack == BossAttack::WormholeBeam)
    {
        boss.stateTimer = std::max(boss.stateTimer, minBeamTell);
    }
    else if (boss.attack == BossAttack::ChargeDash || boss.attack == BossAttack::DashRush)
    {
        boss.stateTimer = std::max(boss.stateTimer, minDashTell);
    }
    boss.windupTotal = std::max(boss.stateTimer, 0.01F);
    if (boss.attack == BossAttack::WormholeBeam)
    {
        const float flankAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto flankDist =
            static_cast<float>(GetRandomValue(static_cast<int32_t>(wormholeBeamFlankMinDist),
                                              static_cast<int32_t>(wormholeBeamFlankMaxDist)));
        boss.wormholeBeamOrigin =
            Vector2Add(game.run.player.position, Vector2{.x = std::cos(flankAngle) * flankDist,
                                                         .y = std::sin(flankAngle) * flankDist});
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
        // Director bosses and forced attacks come through here: pick the pattern too.
        boss.barragePattern = GetRandomValue(0, 2);
        boss.barrageAngle = std::atan2(game.run.player.position.y - (boss.position.y + boss.size.y / 2),
                                       game.run.player.position.x - (boss.position.x + boss.size.x / 2));
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
        if (boss.isSlagmaw)
        {
            shedPetals(game, boss, bossCenter);
            boss.stateTimer = 0.5F;
            break;
        }
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
        boss.beamShieldLatched = false; // the portal was placed when the wind-up began
        playSFX(game, game.resources.sounds.beamFire);
        triggerShake(game, 6, 0.25F);
        break;
    }
    case BossAttack::ChargeDash:
        if (boss.isSlagmaw)
        {
            // Pounce: launch through the ship into the first blob past it.
            const float toShip = Vector2Length(direction);
            float travel = toShip + 600.0F;
            for (float d = toShip + 150.0F; d < toShip + 900.0F; d += 30.0F)
            {
                if (!isSolarForgeCaveOpen(Vector2Add(bossCenter, Vector2Scale(aimDirection, d))))
                {
                    travel = d;
                    break;
                }
            }
            boss.stateTimer = travel / pounceSpeed;
            boss.chargeVelocity = Vector2Scale(aimDirection, pounceSpeed);
            boss.dashHitPlayer = false;
            boss.magmaTrailDist = 0;
            spawnBlastRing(game, bossCenter, boss.size.x * 0.5F + 60.0F);
            playSFX(game, game.resources.sounds.bossWindUp);
            triggerShake(game, 6, 0.2F);
            break;
        }
        boss.stateTimer = chargeDashDuration;
        boss.chargeVelocity = Vector2Scale(
            aimDirection,
            chargeDashSpeed * (boss.isBeltbreakerPlate ? beltbreakerPlateAttackScale : boss.isSlagmaw ? 1.6F : 1.0F));
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
        if (boss.isSlagmaw)
        {
            // Tunnel Run: race under the locked line to just past the player.
            const float travel = Vector2Length(direction) + tunnelOvershoot;
            boss.stateTimer = travel / moleTunnelSpeed;
            boss.chargeVelocity = Vector2Scale(aimDirection, moleTunnelSpeed);
            boss.magmaTrailDist = 0;
            playSFX(game, game.resources.sounds.rockBreak);
            break;
        }
        boss.stateTimer = wreckwormBurrowChargeDuration;
        boss.chargeVelocity = Vector2Scale(aimDirection, chargeDashSpeed * 1.6F); // Wreckworm: fast
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
        for (int32_t i = 0; i < static_cast<int32_t>(static_cast<float>(meteorSwarmSpawnCount) * (boss.isSlagmaw ? slagmawSwarmMult : 1.0F)); i++)
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
        slagmawErupt(game, boss);
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
