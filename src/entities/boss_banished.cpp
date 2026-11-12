#include "boss_internal.hpp"

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
        earnBossAchievement(game, BossAchievement::Banished);
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
