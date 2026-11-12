#include "boss_internal.hpp"

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
