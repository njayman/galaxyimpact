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

// Where background seal `index` shows in the world. The background repeats, so there are many
// copies: this is the one nearest `near`.
static auto banishedSealWorld(int32_t index, Vector2 ship, Vector2 near) -> Vector2
{
    const Vector2 tile{.x = 2.0F * punctumTrapHalfWidth * banishedSealParallax,
                       .y = 2.0F * punctumTrapHalfHeight * banishedSealParallax};
    const Vector2 at = Vector2Add(banishedSeals.at(static_cast<size_t>(index)).at, Vector2Scale(ship, 1.0F - banishedSealParallax));
    return Vector2{.x = at.x - tile.x * std::floor((at.x - near.x) / tile.x + 0.5F),
                   .y = at.y - tile.y * std::floor((at.y - near.y) / tile.y + 0.5F)};
}

// Tentacle stage: the eye rides its background ring (which drifts as the ship moves), and when
// the ship gets away from that ring it shuts and opens again in the ring nearest the ship. A
// body left over from the start or the finale dissolves the same way. Attacks go on throughout.
static void updateBanishedHop(Game& game, Boss& boss, Vector2 center, float deltaTime)
{
    const Vector2 ship = game.run.player.position;
    const auto place = [&](Vector2 at) { boss.position = Vector2Subtract(at, Vector2Scale(boss.size, 0.5F)); };
    if (boss.banishedHop > 0)
    {
        const float before = boss.banishedHop;
        boss.banishedHop += deltaTime;
        if (before <= banishedHopLeave && boss.banishedHop > banishedHopLeave)
        {
            // Open in the ring nearest the ship (another one than it just left).
            const int32_t left = boss.banishedInBody ? -1 : boss.banishedSeal;
            float best = 0;
            for (int32_t i = 0; i < Boss::banishedSealCount; i++)
            {
                const float dist = Vector2Distance(banishedSealWorld(i, ship, ship), ship);
                if (i != left && (boss.banishedSeal == left || dist < best))
                {
                    boss.banishedSeal = i;
                    best = dist;
                }
            }
            boss.banishedInBody = false;
            place(banishedSealWorld(boss.banishedSeal, ship, ship));
        }
        else if (!boss.banishedInBody)
        {
            place(banishedSealWorld(boss.banishedSeal, ship, center));
        }
        const float arriving = (boss.banishedHop - banishedHopLeave) / banishedHopArrive;
        boss.banishedReveal = boss.banishedHop < banishedHopLeave ? 1.0F - boss.banishedHop / banishedHopLeave
                                                                  : std::min(arriving, 1.0F);
        if (arriving >= 1.0F)
        {
            boss.banishedHop = 0;
        }
        return;
    }
    if (!boss.banishedInBody)
    {
        place(banishedSealWorld(boss.banishedSeal, ship, center));
    }
    const bool away = boss.banishedInBody || Vector2Distance(center, ship) > banishedHopAway;
    boss.banishedAway = away && boss.banishedGazePhase == 0 ? boss.banishedAway + deltaTime : 0;
    if (boss.banishedInBody || boss.banishedAway >= banishedHopDelay)
    {
        boss.banishedAway = 0;
        boss.banishedHop = 0.0001F;
    }
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
    boss.banishedGrabbing.at(static_cast<size_t>(idx)) = true;
    boss.banishedPhase.at(static_cast<size_t>(idx)) = 6;
    boss.banishedTimer.at(static_cast<size_t>(idx)) = UpdateConstants::banishedPlainGrabHold;
    boss.banishedTip.at(static_cast<size_t>(idx)) = contactPoint;
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
    boss.banishedGrabbing.at(static_cast<size_t>(idx)) = false;
    boss.banishedPhase.at(static_cast<size_t>(idx)) = 3;
    boss.banishedTimer.at(static_cast<size_t>(idx)) = banishedTentacleRetreat;
}

// --- Banished big attack: the grab, the toss and the bat (the Juggled QTE) -------
//
// On a timer one tentacle opens far off and takes its time aiming. When the ship holds still
// or comes near, it flashes and lunges: it cannot be dodged, only blocked (shield up, or a
// shield dash into it, as it lands). Caught, the ship is tossed: a tentacle leans back and
// throws it far out of view, another catches it and throws again, five times. After the last
// throw three tentacles twist into one, curl back and swing like a bat. A shield+dash prompt
// shows before it lands: answered, the three are paralysed; missed, the bat connects.
// Paralysing all twelve starts the finale.
// --------------------------------------------------------------------------------

static void restBanishedTentacles(Boss& boss)
{
    for (int32_t t = 0; t < Boss::banishedTentacleCount; t++)
    {
        const auto idx = static_cast<size_t>(t);
        boss.banishedPhase.at(idx) = 0;
        boss.banishedGrabbing.at(idx) = false;
        boss.banishedCooldown.at(idx) = static_cast<float>(GetRandomValue(8, 30)) / 10.0F;
    }
}

static auto randomBigGap() -> float
{
    return static_cast<float>(GetRandomValue(static_cast<int32_t>(banishedBigGapMin), static_cast<int32_t>(banishedBigGapMax)));
}

// A throw: the holder leans back, then lets the ship go along `angle`; a catcher waits at the far end.
static void startBanishedThrow(Boss& boss, Vector2 ship, float angle)
{
    const Vector2 dir{.x = std::cos(angle), .y = std::sin(angle)};
    boss.banishedTossAngle = angle;
    boss.banishedTossFrom = ship;
    boss.banishedTossTo = Vector2Add(ship, Vector2Scale(dir, static_cast<float>(GetRandomValue(
                                                                 static_cast<int32_t>(banishedTossDistMin),
                                                                 static_cast<int32_t>(banishedTossDistMax)))));
    const float side = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    boss.banishedTossNextAnchor = Vector2Add(boss.banishedTossTo, Vector2{.x = std::cos(side) * 200.0F, .y = std::sin(side) * 200.0F});
    boss.banishedTossPhase = 1;
    boss.banishedTossTimer = banishedTossLean;
}

// The big grab landed: the toss begins. (Also the sandbox's forced QTE.)
void beginBanishedJuggled(Game& game, Boss& boss, int32_t slot)
{
    bossQteBegin(game, boss.instanceId, QteKind::Juggled);
    game.run.player.grabbed = true;
    boss.banishedTossAnchor = boss.banishedIsFar.at(static_cast<size_t>(slot))
                                  ? boss.banishedAnchor.at(static_cast<size_t>(slot))
                                  : Vector2Add(game.run.player.position, Vector2{.x = 0, .y = -200.0F});
    restBanishedTentacles(boss);
    boss.banishedTossLeft = banishedTossThrows;
    startBanishedThrow(boss, game.run.player.position, static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD);
    playSFX(game, game.resources.sounds.slamBoom);
    triggerShake(game, 12, 0.4F);
}

void endBanishedJuggled(Game& game, Boss& boss, bool clean)
{
    (void)clean;
    bossQteEnd(game);
    game.run.player.grabbed = false;
    boss.banishedTossPhase = 0;
    boss.banishedBigTimer = randomBigGap();
}

static void updateBanishedFinaleQte(Game& game, Boss& boss);

void updateBanishedJuggled(Game& game, Boss& boss, float deltaTime)
{
    auto& run = game.run;
    if (boss.banishedStage == 1)
    {
        updateBanishedFinaleQte(game, boss);
        return;
    }
    const Vector2 dir{.x = std::cos(boss.banishedTossAngle), .y = std::sin(boss.banishedTossAngle)};
    const Vector2 leanedBack = Vector2Subtract(boss.banishedTossFrom, Vector2Scale(dir, 90.0F));
    boss.banishedTossTimer -= deltaTime;

    if (boss.banishedTossPhase == 1)
    {
        const float p = std::clamp(1.0F - boss.banishedTossTimer / banishedTossLean, 0.0F, 1.0F);
        run.player.position = Vector2Lerp(boss.banishedTossFrom, leanedBack, p * (2.0F - p));
        if (boss.banishedTossTimer <= 0)
        {
            boss.banishedTossPhase = 2;
            boss.banishedTossTimer = banishedTossFlight;
            playSFX(game, game.resources.sounds.bossWindUp);
            triggerShake(game, 6, 0.15F);
        }
        return;
    }
    if (boss.banishedTossPhase == 2)
    {
        const float p = std::clamp(1.0F - boss.banishedTossTimer / banishedTossFlight, 0.0F, 1.0F);
        // A caught throw eases to a stop in the catcher's grip; the last one is still travelling when it ends.
        run.player.position =
            Vector2Lerp(leanedBack, boss.banishedTossTo, boss.banishedTossLeft > 1 ? p * (2.0F - p) : p * (1.6F - 0.6F * p));
        if (boss.banishedTossTimer > 0)
        {
            return;
        }
        boss.banishedTossLeft--;
        if (boss.banishedTossLeft > 0)
        {
            // caught, and thrown again in a new direction
            boss.banishedTossAnchor = boss.banishedTossNextAnchor;
            const float turn = static_cast<float>(GetRandomValue(100, 260)) * DEG2RAD;
            startBanishedThrow(boss, run.player.position, boss.banishedTossAngle + turn);
            playSFX(game, game.resources.sounds.slamBoom);
            triggerShake(game, 8, 0.2F);
            return;
        }
        // The last throw is not caught: three tentacles twist into a bat ahead of the ship.
        int32_t found = 0;
        for (int32_t t = 0; t < Boss::banishedTentacleCount && found < 3; t++)
        {
            if (!boss.banishedStaggered.at(static_cast<size_t>(t)))
            {
                boss.banishedBatSlots.at(static_cast<size_t>(found++)) = t;
            }
        }
        boss.banishedTossAnchor = Vector2Add(boss.banishedTossTo, Vector2Scale(dir, banishedBatDist));
        boss.banishedTossPhase = 3;
        boss.banishedTossTimer = banishedBatWindup;
        run.player.grabbed = false; // free of the grip: time slows for the prompt
        return;
    }
    if (boss.banishedTossPhase == 3 || boss.banishedTossPhase == 4)
    {
        // Nothing caught it: the ship is still carried toward the bat by the throw, slowing, all
        // through the wind-up and the swing. The prompt comes while it drifts.
        const float elapsed = boss.banishedTossPhase == 3 ? banishedBatWindup - boss.banishedTossTimer
                                                          : banishedBatWindup + banishedBatSwing - boss.banishedTossTimer;
        const float q = std::clamp(elapsed / (banishedBatWindup + banishedBatSwing), 0.0F, 1.0F);
        run.player.position =
            Vector2Add(boss.banishedTossTo, Vector2Scale(dir, (banishedBatDist - banishedBatMiss) * q * (2.0F - q)));
    }
    if (boss.banishedTossPhase == 3)
    {
        run.qteThreatDir = Vector2Normalize(Vector2Subtract(run.player.position, boss.banishedTossAnchor));
        if (run.qtePrompt == QtePrompt::None && boss.banishedTossTimer <= banishedBatPrompt && run.qteActionTimer <= 0)
        {
            run.qtePrompt = QtePrompt::Parry;
            run.qteTimer = banishedBatPrompt;
            run.qteTimerMax = banishedBatPrompt;
            playSFX(game, game.resources.sounds.bossWindUp);
        }
        if (run.qtePrompt == QtePrompt::Parry && qteReadInput(game, QtePrompt::Parry) > 0)
        {
            // Shield-dashed into the bat: the three limbs are paralysed.
            qteStartAction(game, QtePrompt::Parry, run.qteThreatDir);
            run.qtePrompt = QtePrompt::None;
            for (const int32_t slot : boss.banishedBatSlots)
            {
                boss.banishedStaggered.at(static_cast<size_t>(slot)) = true;
                boss.banishedTentacleHitFlash.at(static_cast<size_t>(slot)) = UpdateConstants::hitFlashDuration;
            }
            spawnKillExplosion(game, boss.banishedTossAnchor, Palette::Crit, 30, 1.6F);
            playSFX(game, game.resources.sounds.critical);
            triggerBossShieldDashShake(game);
            // The ship goes clean through the bat, shield up, and out the far side.
            const Vector2 beyond = Vector2Subtract(boss.banishedTossAnchor, Vector2Scale(run.qteThreatDir, banishedBatThrough));
            run.qteActionTimer = banishedBatBreak;
            run.qteActionVelocity = Vector2Scale(Vector2Subtract(beyond, run.player.position), 1.0F / banishedBatBreak);
            boss.banishedTossPhase = 5; // let the dash play out, then end
            boss.banishedTossTimer = banishedBatBreak;
            return;
        }
        if (boss.banishedTossTimer <= 0)
        {
            run.qtePrompt = QtePrompt::None;
            boss.banishedTossPhase = 4;
            boss.banishedTossTimer = banishedBatSwing;
        }
        return;
    }
    if (boss.banishedTossPhase == 4 && boss.banishedTossTimer <= 0)
    {
        // The bat connects.
        damagePlayer(game, enemyDamage(game, banishedBatDamage));
        run.player.position = Vector2Add(run.player.position, Vector2Scale(run.qteThreatDir, 320.0F));
        playSFX(game, game.resources.sounds.slamBoom);
        triggerShake(game, 14, 0.4F);
        endBanishedJuggled(game, boss, false);
        return;
    }
    if (boss.banishedTossPhase == 5 && boss.banishedTossTimer <= 0)
    {
        endBanishedJuggled(game, boss, true);
    }
}

// --- Banished finale ------------------------------------------------------------
//
// With all twelve tentacles paralysed the body comes for the ship itself: ten to twelve moves,
// each a dash from a random side (a lane shows first) or a Doom Gaze beam (the eye tracks, the
// line freezes, it fires). Every third dash carries a dodge prompt. The last move is a dash
// with the eye open and a shield+dash prompt: answered, the ship rams the eye. Then the
// tentacles wake and the fight goes round again.
// --------------------------------------------------------------------------------

static void wakeBanished(Boss& boss)
{
    boss.banishedStage = 0;
    boss.banishedFinaleMove = 0;
    boss.banishedStaggered.fill(false);
    boss.banishedEyeChargeBurstUsed = false;
    boss.banishedBigTimer = 14.0F;
    restBanishedTentacles(boss);
}

// The prompts of the finale's QTE dashes.
static void updateBanishedFinaleQte(Game& game, Boss& boss)
{
    auto& run = game.run;
    if (run.qtePrompt != QtePrompt::None)
    {
        if (qteReadInput(game, run.qtePrompt) > 0)
        {
            const bool ram = run.qtePrompt == QtePrompt::Parry;
            qteStartAction(game, run.qtePrompt, run.qteThreatDir);
            run.qtePrompt = QtePrompt::None;
            boss.banishedFinaleHit = true; // this dash can no longer hurt
            playSFX(game, game.resources.sounds.critical);
            if (ram)
            {
                // Into the open eye.
                damageBanishedEyeBurst(game, boss, banishedFinaleEyeDivisor);
                spawnKillExplosion(game, Vector2Add(boss.position, Vector2Scale(boss.size, 0.5F)), Palette::Crit, 40, 2.0F);
                boss.banishedFinaleLeft = 0;
                boss.banishedFinaleMove = 0;
                boss.banishedEyeTimer = 0.8F;
            }
        }
        else if (run.qteTimer <= 0)
        {
            run.qtePrompt = QtePrompt::None; // too late: the dash lands
        }
        return;
    }
    if (run.qteActionTimer <= 0)
    {
        bossQteEnd(game); // prompt answered or missed: time runs again and the dash plays out
    }
}

// Doom Gaze burning along its frozen line (centre to boss.targetPosition); the shield stops it.
static void burnBanishedGaze(Game& game, Boss& boss, Vector2 center, float deltaTime)
{
    auto& run = game.run;
    if (run.player.shieldActive ||
        !CheckCollisionCircleLine(run.player.position, run.player.radius + 7.0F, center, boss.targetPosition))
    {
        return;
    }
    boss.beamDamageAccum += banishedGazeDps * deltaTime;
    if (boss.beamDamageAccum >= 1.0F)
    {
        const auto tick = static_cast<int32_t>(boss.beamDamageAccum);
        damagePlayer(game, tick);
        boss.beamDamageAccum -= static_cast<float>(tick);
    }
}

// Tentacle stage: every so often the eye opens, focuses on the ship (tracking, then the line
// freezes) and fires a Doom Gaze down that line.
static void updateBanishedGaze(Game& game, Boss& boss, Vector2 center, float deltaTime)
{
    boss.banishedGazeTimer -= deltaTime;
    if (boss.banishedGazePhase == 0)
    {
        if (boss.banishedGazeTimer <= 0)
        {
            boss.banishedGazePhase = 1;
            boss.banishedGazeTimer = banishedGazeCharge + banishedGazeLock;
            boss.targetPosition = game.run.player.position;
            playSFX(game, game.resources.sounds.bossWindUp);
        }
        return;
    }
    if (boss.banishedGazePhase == 1)
    {
        if (boss.banishedGazeTimer > banishedGazeLock)
        {
            boss.targetPosition = game.run.player.position; // still tracking
        }
        if (boss.banishedGazeTimer <= 0)
        {
            const Vector2 toward = Vector2Normalize(Vector2Subtract(boss.targetPosition, center));
            boss.targetPosition = Vector2Add(center, Vector2Scale(toward, 2400.0F));
            boss.banishedGazePhase = 2;
            boss.banishedGazeTimer = banishedGazeFire;
            playSFX(game, game.resources.sounds.slamBoom);
            triggerShake(game, 8, 0.3F);
        }
        return;
    }
    burnBanishedGaze(game, boss, center, deltaTime);
    if (boss.banishedGazeTimer <= 0)
    {
        boss.banishedGazePhase = 0;
        boss.banishedGazeTimer = static_cast<float>(GetRandomValue(static_cast<int32_t>(banishedGazeGapMin * 10),
                                                                   static_cast<int32_t>(banishedGazeGapMax * 10))) /
                                 10.0F;
    }
}

static void updateBanishedFinale(Game& game, Boss& boss, Vector2 center, float deltaTime)
{
    auto& run = game.run;
    const Vector2 ship = run.player.position;
    boss.banishedEyeTimer -= deltaTime;
    boss.banishedReveal = std::min(1.0F, boss.banishedReveal + deltaTime / 0.8F); // the body assembling

    if (boss.banishedFinaleMove == 0)
    {
        if (boss.banishedEyeTimer > 0)
        {
            return;
        }
        if (boss.banishedFinaleLeft <= 0)
        {
            wakeBanished(boss);
            return;
        }
        boss.banishedFinaleLeft--;
        const bool last = boss.banishedFinaleLeft == 0;
        const float a = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const Vector2 from{.x = std::cos(a), .y = std::sin(a)};
        if (last || GetRandomValue(0, 99) < banishedFinaleDashChance)
        {
            boss.banishedFinaleDashes++;
            boss.banishedFinaleEyeDash = last;
            boss.banishedFinaleHit = false;
            boss.position = Vector2Subtract(Vector2Add(ship, Vector2Scale(from, banishedDashStart)), Vector2Scale(boss.size, 0.5F));
            boss.targetPosition = ship;
            boss.chargeVelocity = Vector2Scale(from, -banishedDashSpeed);
            boss.banishedFinaleMove = 1;
            boss.banishedEyeTimer = banishedDashTell;
            playSFX(game, game.resources.sounds.bossWindUp);
            if ((last || boss.banishedFinaleDashes % banishedFinaleQteEvery == 0) && qteSlotFree(game))
            {
                bossQteBegin(game, boss.instanceId, QteKind::Juggled);
                run.qteThreatDir = Vector2Negate(from);
                run.qtePrompt = last ? QtePrompt::Parry : qtePromptForThreat(game, run.qteThreatDir, 0);
                run.qteTimer = banishedDashTell + banishedDashStart / banishedDashSpeed;
                run.qteTimerMax = run.qteTimer;
            }
        }
        else
        {
            boss.position = Vector2Subtract(Vector2Add(ship, Vector2Scale(from, banishedGazeDist)), Vector2Scale(boss.size, 0.5F));
            boss.targetPosition = ship;
            boss.banishedFinaleMove = 3;
            boss.banishedEyeTimer = banishedGazeCharge + banishedGazeLock;
            playSFX(game, game.resources.sounds.bossWindUp);
        }
        return;
    }
    if (boss.banishedFinaleMove == 1)
    {
        if (boss.banishedEyeTimer <= 0)
        {
            boss.banishedFinaleMove = 2;
            boss.banishedEyeTimer = banishedDashTravel / banishedDashSpeed;
        }
        return;
    }
    if (boss.banishedFinaleMove == 2)
    {
        boss.position = Vector2Add(boss.position, Vector2Scale(boss.chargeVelocity, deltaTime));
        if (!boss.banishedFinaleHit && Vector2Distance(center, ship) < banishedDiscRadius + run.player.radius)
        {
            boss.banishedFinaleHit = true;
            if (run.qteBossId == boss.instanceId)
            {
                run.qtePrompt = QtePrompt::None; // the prompt went unanswered
            }
            if (run.player.shieldActive)
            {
                triggerBossShieldDashShake(game);
            }
            else
            {
                damagePlayer(game, enemyDamage(game, banishedDashDamage));
                triggerShake(game, 12, 0.3F);
            }
        }
        if (boss.banishedEyeTimer <= 0)
        {
            boss.banishedFinaleMove = 0;
            boss.banishedEyeTimer = banishedFinaleGap;
        }
        return;
    }
    if (boss.banishedFinaleMove == 3)
    {
        if (boss.banishedEyeTimer > banishedGazeLock)
        {
            boss.targetPosition = ship; // still tracking
        }
        if (boss.banishedEyeTimer <= 0)
        {
            const Vector2 toward = Vector2Normalize(Vector2Subtract(boss.targetPosition, center));
            boss.targetPosition = Vector2Add(center, Vector2Scale(toward, 2400.0F));
            boss.banishedFinaleMove = 4;
            boss.banishedEyeTimer = banishedGazeFire;
            playSFX(game, game.resources.sounds.slamBoom);
            triggerShake(game, 8, 0.3F);
        }
        return;
    }
    // Doom Gaze firing down its frozen line.
    burnBanishedGaze(game, boss, center, deltaTime);
    if (boss.banishedEyeTimer <= 0)
    {
        boss.banishedFinaleMove = 0;
        boss.banishedEyeTimer = banishedFinaleGap;
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
        updateBanishedFinale(game, boss, bossCenter, deltaTime);
        return;
    }

    if (game.run.qteBossId == boss.instanceId)
    {
        return; // Juggled owns the tentacles
    }
    updateBanishedHop(game, boss, bossCenter, deltaTime);
    if (boss.banishedHop <= 0)
    {
        updateBanishedGaze(game, boss, bossCenter, deltaTime); // it also fires from the background
    }

    const bool bossVisible =
        std::abs(bossCenter.x - game.run.player.position.x) <=
            static_cast<float>(game.resources.screenWidth) / 2.0F + boss.size.x / 2.0F &&
        std::abs(bossCenter.y - game.run.player.position.y) <=
            static_cast<float>(game.resources.screenHeight) / 2.0F + boss.size.y / 2.0F;
    const float tentacleDeltaTime =
        bossVisible ? deltaTime : deltaTime * banishedOffscreenAggressionMult;

    // Attacks come one at a time from anywhere in view: a shared gate spaces the starts out, only
    // a few tentacles are out at once, and the slot order is shuffled so no tentacle goes first.
    boss.banishedAttackGate -= tentacleDeltaTime;
    int32_t active = 0;
    for (const int32_t phase : boss.banishedPhase)
    {
        active += phase != 0 ? 1 : 0;
    }
    const int32_t firstSlot = GetRandomValue(0, Boss::banishedTentacleCount - 1);

    // The big grab, on its own timer: everything else pulls back and one tentacle opens far off.
    bool bigOut = false;
    for (int32_t t = 0; t < Boss::banishedTentacleCount; t++)
    {
        bigOut = bigOut || (boss.banishedAttackKind.at(static_cast<size_t>(t)) == 4 && boss.banishedPhase.at(static_cast<size_t>(t)) != 0);
    }
    if (!bigOut)
    {
        boss.banishedBigTimer -= deltaTime;
    }
    if (!bigOut && boss.banishedBigTimer <= 0 && qteSlotFree(game))
    {
        int32_t slot = -1;
        for (int32_t order = 0; order < Boss::banishedTentacleCount && slot < 0; order++)
        {
            const int32_t t = (firstSlot + order) % Boss::banishedTentacleCount;
            slot = !boss.banishedStaggered.at(static_cast<size_t>(t)) ? t : -1;
        }
        if (slot >= 0)
        {
            for (int32_t t = 0; t < Boss::banishedTentacleCount; t++)
            {
                const auto tdx = static_cast<size_t>(t);
                if (boss.banishedPhase.at(tdx) == 1 || boss.banishedPhase.at(tdx) == 2)
                {
                    boss.banishedTip.at(tdx) = boss.banishedAnchor.at(tdx);
                    boss.banishedPhase.at(tdx) = 3;
                    boss.banishedTimer.at(tdx) = banishedTentacleRetreat;
                }
            }
            const auto sdx = static_cast<size_t>(slot);
            const Vector2 ship = game.run.player.position;
            const float a = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 at = Vector2Add(ship, Vector2{.x = std::cos(a) * banishedBigPortalDist, .y = std::sin(a) * banishedBigPortalDist});
            boss.banishedAttackKind.at(sdx) = 4;
            boss.banishedIsFar.at(sdx) = true;
            boss.banishedAnchor.at(sdx) = at;
            boss.banishedTip.at(sdx) = at;
            boss.banishedAimAngle.at(sdx) = std::atan2(ship.y - at.y, ship.x - at.x);
            boss.banishedPhase.at(sdx) = 1;
            boss.banishedTimer.at(sdx) = banishedBigAimMax;
            boss.banishedBigStill = 0;
            boss.banishedBigShipPrev = ship;
            bigOut = true;
            playSFX(game, game.resources.sounds.bossWindUp);
        }
    }

    bool allStaggered = true;
    for (int32_t order = 0; order < Boss::banishedTentacleCount; order++)
    {
        const int32_t i = (firstSlot + order) % Boss::banishedTentacleCount;
        const auto idx = static_cast<size_t>(i);
        if (boss.banishedTentacleHitFlash.at(idx) > 0)
        {
            boss.banishedTentacleHitFlash.at(idx) -= deltaTime;
        }
        if (!boss.banishedStaggered.at(idx))
        {
            allStaggered = false;
        }
        if (boss.banishedStaggered.at(idx))
        {
            continue;
        }

        if (boss.banishedPhase.at(idx) == 0)
        {
            boss.banishedCooldown.at(idx) -= tentacleDeltaTime;
            if (boss.banishedCooldown.at(idx) > 0)
            {
                continue;
            }

            if (boss.banishedAttackGate > 0 || active >= banishedMaxActive || bigOut)
            {
                continue;
            }
            boss.banishedAttackGate = static_cast<float>(GetRandomValue(static_cast<int32_t>(banishedAttackGapMin * 100),
                                                                        static_cast<int32_t>(banishedAttackGapMax * 100))) /
                                      100.0F;
            active++;

            const Vector2 ship = game.run.player.position;
            // Every attack comes through a portal torn open anywhere in view, never right on the ship.
            const int32_t pick = i;
            const bool fromBody = false;
            const auto pdx = static_cast<size_t>(pick);
            Vector2 at = ship;
            for (int32_t attempt = 0; attempt < 8 && Vector2Distance(at, ship) < banishedPortalClearance; attempt++)
            {
                at = Vector2Add(ship, Vector2{.x = static_cast<float>(GetRandomValue(-560, 560)),
                                              .y = static_cast<float>(GetRandomValue(-310, 310))});
            }

            // Pierce; sometimes a grab (when the ship is free) or a hurled enemy.
            const int32_t roll = GetRandomValue(0, 99);
            const bool canGrab = !game.run.player.grabbed && !game.run.player.launched;
            boss.banishedAttackKind.at(pdx) = [&]() -> int32_t
            {
                if (canGrab && roll < banishedGrabChance) { return 2; }
                if (roll < banishedGrabChance + banishedHurlChance) { return 3; }
                return 0;
            }();
            // What a hurl pulls through: an enemy (most often), a meteor, or a lava chunk.
            const int32_t pulled = GetRandomValue(0, 99);
            const auto enemyCount = static_cast<int32_t>(enemyKinds.size());
            boss.banishedHeldKind.at(pdx) = [&]() -> int32_t
            {
                if (pulled < 60) { return GetRandomValue(0, enemyCount - 1); }
                if (pulled < 85) { return enemyCount; }
                return enemyCount + 1;
            }();
            boss.banishedIsFar.at(pdx) = !fromBody;
            boss.banishedAnchor.at(pdx) = at;
            boss.banishedTip.at(pdx) = at;
            boss.banishedAimAngle.at(pdx) = std::atan2(ship.y - at.y, ship.x - at.x);
            boss.banishedAimReach.at(pdx) = 0;
            boss.banishedPhase.at(pdx) = 1;
            boss.banishedTimer.at(pdx) = banishedTentacleWindup;
            continue;
        }

        if (boss.banishedPhase.at(idx) == 1 && boss.banishedAttackKind.at(idx) == 4)
        {
            // The big grab aims for as long as the ship keeps moving and keeps away.
            const Vector2 ship = game.run.player.position;
            boss.banishedTimer.at(idx) -= deltaTime;
            if (boss.banishedTimer.at(idx) > banishedBigTell)
            {
                boss.banishedAimAngle.at(idx) = std::atan2(ship.y - boss.banishedAnchor.at(idx).y, ship.x - boss.banishedAnchor.at(idx).x);
                const float speed = deltaTime > 0 ? Vector2Distance(ship, boss.banishedBigShipPrev) / deltaTime : 0.0F;
                boss.banishedBigStill = speed < banishedBigStillSpeed ? boss.banishedBigStill + deltaTime : 0.0F;
                if (boss.banishedBigStill >= banishedBigStillTime || Vector2Distance(ship, boss.banishedAnchor.at(idx)) < banishedBigNear)
                {
                    boss.banishedTimer.at(idx) = banishedBigTell; // it flashes: block now
                    playSFX(game, game.resources.sounds.bossWindUp);
                }
            }
            boss.banishedBigShipPrev = ship;
            if (boss.banishedTimer.at(idx) <= 0)
            {
                boss.banishedPhase.at(idx) = 2;
                boss.banishedTimer.at(idx) = banishedBigLunge;
            }
            continue;
        }

        if (boss.banishedPhase.at(idx) == 1)
        {
            // The stance: it tracks the ship until the last moment, then the aim freezes.
            boss.banishedTimer.at(idx) -= tentacleDeltaTime;
            if (boss.banishedTimer.at(idx) > banishedAimLock)
            {
                const Vector2 toShip = Vector2Subtract(game.run.player.position, boss.banishedAnchor.at(idx));
                boss.banishedAimAngle.at(idx) = std::atan2(toShip.y, toShip.x);
                boss.banishedAimReach.at(idx) = std::min(Vector2Length(toShip) + banishedPierceOvershoot, banishedPierceReach);
            }
            if (boss.banishedTimer.at(idx) <= 0)
            {
                boss.banishedPhase.at(idx) = 2;
                boss.banishedTimer.at(idx) = boss.banishedAttackKind.at(idx) == 2 ? banishedTentacleStrike : banishedPierceStrike;
                if (boss.banishedAttackKind.at(idx) == 3)
                {
                    // Hurl: the enemy it pulled through leaves the tip down the frozen aim.
                    const Vector2 dir{.x = std::cos(boss.banishedAimAngle.at(idx)), .y = std::sin(boss.banishedAimAngle.at(idx))};
                    game.run.bossProjectiles.push_back(
                        BossProjectile{.position = Vector2Add(boss.banishedAnchor.at(idx), Vector2Scale(dir, banishedHurlRelease)),
                                       .velocity = Vector2Scale(dir, krakenLimbHurlSpeed),
                                       .radius = 22,
                                       .homing = false,
                                       .active = true,
                                       .fromPlayer = false,
                                       .damage = krakenLimbHurlDamage,
                                       .health = 0,
                                       .isMeteor = false,
                                       .visualEnemyKind = boss.banishedHeldKind.at(idx),
                                       .energized = true});
                }
            }
            continue;
        }

        if (boss.banishedPhase.at(idx) == 2 && boss.banishedAttackKind.at(idx) == 3)
        {
            boss.banishedTimer.at(idx) -= tentacleDeltaTime; // the throw's follow-through; the tip hits nothing
            if (boss.banishedTimer.at(idx) <= 0)
            {
                boss.banishedPhase.at(idx) = 3;
                boss.banishedTimer.at(idx) = banishedTentacleRetreat;
            }
            continue;
        }

        if (boss.banishedPhase.at(idx) == 2)
        {
            const bool big = boss.banishedAttackKind.at(idx) == 4;
            const bool grab = boss.banishedAttackKind.at(idx) == 2 || big;
            const float strike = [&]() -> float
            {
                if (big) { return banishedBigLunge; }
                if (grab) { return banishedTentacleStrike; }
                return banishedPierceStrike;
            }();
            const float progress = std::clamp(1.0F - boss.banishedTimer.at(idx) / strike, 0.0F, 1.0F);
            const Vector2 prevTip = boss.banishedTip.at(idx);
            if (grab)
            {
                // The grab lunges after the ship wherever it goes.
                const Vector2 toPlayer = Vector2Subtract(game.run.player.position, boss.banishedAnchor.at(idx));
                const float dist = big ? Vector2Length(toPlayer) : std::min(Vector2Length(toPlayer), banishedPierceReach);
                const Vector2 dir = Vector2Length(toPlayer) > 0.01F ? Vector2Normalize(toPlayer) : Vector2{.x = 0, .y = 1};
                boss.banishedTip.at(idx) =
                    Vector2Add(boss.banishedAnchor.at(idx), Vector2Scale(dir, dist * std::min(1.0F, progress / 0.7F))); // on the ship by 70%
            }
            else
            {
                // The pierce thrusts straight down its frozen aim: step off the line to dodge it.
                const float a = boss.banishedAimAngle.at(idx);
                const float out = boss.banishedAimReach.at(idx) * (1.0F - (1.0F - progress) * (1.0F - progress));
                boss.banishedTip.at(idx) =
                    Vector2Add(boss.banishedAnchor.at(idx), Vector2{.x = std::cos(a) * out, .y = std::sin(a) * out});
            }

            boss.banishedTimer.at(idx) -= tentacleDeltaTime;

            // The tip moves far in a frame, so test the stretch it just covered.
            if (CheckCollisionCircleLine(game.run.player.position, banishedTentacleHitRadius + game.run.player.radius,
                                         prevTip, boss.banishedTip.at(idx)))
            {
                if (game.run.player.shieldActive)
                {
                    // Blocked: it recoils. (Only the bat's QTE paralyses a tentacle now.)
                    boss.banishedPhase.at(idx) = 3;
                    boss.banishedTimer.at(idx) = banishedTentacleRetreat;
                    boss.banishedTentacleHitFlash.at(idx) = UpdateConstants::hitFlashDuration;
                    triggerBossShieldDashShake(game);
                    if (big)
                    {
                        boss.banishedBigTimer = banishedBigRetryGap;
                    }
                }
                else if (big)
                {
                    beginBanishedJuggled(game, boss, i);
                }
                else if (grab)
                {
                    beginBanishedCoil(boss, game.run.player, i, boss.banishedTip.at(idx));
                }
                else
                {
                    damagePlayer(game, enemyDamage(game, banishedTentacleDamage));
                    boss.banishedPhase.at(idx) = 3;
                    boss.banishedTimer.at(idx) = banishedTentacleRetreat;
                }
                continue;
            }

            if (boss.banishedTimer.at(idx) <= 0)
            {
                boss.banishedPhase.at(idx) = 3;
                boss.banishedTimer.at(idx) = banishedTentacleRetreat;
                if (big)
                {
                    boss.banishedBigTimer = banishedBigRetryGap;
                }
            }
            continue;
        }

        if (boss.banishedPhase.at(idx) == 3)
        {
            boss.banishedTimer.at(idx) -= tentacleDeltaTime;
            if (boss.banishedTimer.at(idx) <= 0)
            {
                boss.banishedPhase.at(idx) = 0;
                boss.banishedCooldown.at(idx) =
                    static_cast<float>(GetRandomValue(
                        static_cast<int32_t>(banishedTentacleCooldownMin * 10),
                        static_cast<int32_t>(banishedTentacleCooldownMax * 10))) /
                    10.0F;
            }
            continue;
        }

        if (boss.banishedPhase.at(idx) == 5)
        {
            const float easeLerp = std::clamp(3.0F * deltaTime, 0.0F, 1.0F);
            boss.banishedTip.at(idx) = Vector2Lerp(boss.banishedTip.at(idx), game.run.player.position, easeLerp);
            boss.banishedAnchor.at(idx) = boss.banishedTip.at(idx);
            continue;
        }

        if (boss.banishedPhase.at(idx) == 6)
        {
            game.run.player.position = boss.banishedTip.at(idx);
            game.run.player.dashing = false;
            boss.banishedTimer.at(idx) -= deltaTime;
            if (boss.banishedTimer.at(idx) <= 0)
            {
                resolveBanishedCoil(game, boss, i);
            }
        }
    }

    if (allStaggered)
    {
        boss.banishedStage = 1;
        boss.banishedInBody = true; // it comes out of the background, in its body
        boss.banishedReveal = 0;
        boss.banishedHop = 0;
        boss.banishedGazePhase = 0;
        boss.banishedGazeTimer = banishedGazeGapMin;
        boss.banishedFinaleLeft = GetRandomValue(banishedFinaleMovesMin, banishedFinaleMovesMax);
        boss.banishedFinaleMove = 0;
        boss.banishedFinaleDashes = 0;
        boss.banishedEyeTimer = 1.0F;
        boss.banishedEyeChargeBurstUsed = false;
        restBanishedTentacles(boss);
    }
}
