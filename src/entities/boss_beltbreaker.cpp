#include "boss_internal.hpp"

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

auto beltbreakerBare(const Game& game, const Boss& core) -> bool
{
    return core.isBeltbreaker && core.health > 0 &&
           std::ranges::none_of(game.run.bosses, [&](const Boss& plate) { return isRushPlateOf(plate, core); });
}

// The bare core calls Belt rubble in from off screen. Each rock flies to a free slot
// and attaches as a weak plate, restoring the armour and shield loop; shoot them down in flight.
// Runs outside the boss loop since it adds bosses.
void updateBeltbreakerRubble(Game& game, float deltaTime)
{
    for (size_t c = 0; c < game.run.bosses.size(); c++)
    {
        Boss& core = game.run.bosses.at(c);
        if (!beltbreakerBare(game, core) || core.deathAnimTimer >= 0 || bossQteActive(game))
        {
            continue;
        }
        core.rubbleRegrowTimer += deltaTime;
        if (core.rubbleRegrowTimer < beltbreakerRubbleInterval || !bossOnScreen(game, beltbreakerCoreCenter(core)))
        {
            continue;
        }
        core.rubbleRegrowTimer = 0;
        const int32_t ownerId = core.instanceId;
        const int32_t slots = core.plateCount;
        const auto plateHealth = std::max(1, static_cast<int32_t>(UpdateConstants::beltbreakerPlateHealthPerTier *
                                                                  beltbreakerRubbleHealthFrac * enemyHealthMult *
                                                                  enemyHealthScale(game)));
        const float offDist = std::max(static_cast<float>(game.resources.screenWidth),
                                       static_cast<float>(game.resources.screenHeight)) /
                              2;
        const int32_t start = GetRandomValue(0, slots - 1);
        for (int32_t i = 0; i < std::min(beltbreakerRubbleCount, slots); i++)
        {
            const int32_t slot = (start + i * slots / beltbreakerRubbleCount) % slots;
            const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 from = Vector2Add(game.run.player.position,
                                            Vector2{.x = std::cos(angle) * offDist, .y = std::sin(angle) * offDist});
            game.run.bosses.push_back(Boss{.position = Vector2Subtract(from, Vector2{.x = 22, .y = 22}),
                                           .size = Vector2{.x = 44, .y = 44},
                                           .color = Palette::StructLight,
                                           .baseColor = Palette::StructLight,
                                           .health = plateHealth,
                                           .maxHealth = plateHealth,
                                           .state = BossState::IDLE,
                                           .attack = BossAttack::Barrage,
                                           .moveset = {BossAttack::Barrage, BossAttack::ChargeDash},
                                           .attackTimer = 3.0F,
                                           .isMega = false,
                                           .isSwarm = false,
                                           .shape = BossShape::HexPlated,
                                           .isBeltbreakerPlate = true,
                                           .plateOwnerId = ownerId,
                                           .plateSlotIndex = slot,
                                           .plateAttached = false,
                                           .plateReturning = true});
        }
        playSFX(game, game.resources.sounds.bossWindUp);
        return; // the vector moved; one core per frame is plenty
    }
}

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
