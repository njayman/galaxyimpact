#include "boss_internal.hpp"

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

// --- Kraken arm attacks --------------------------------------------------------
//
// Every active tentacle slot runs one attack at a time: Arm Lash (an arm from the face rears
// back and whips through an arc across the player), Portal Lash (the same whip out of a portal
// beside the player) or Spear Rake (3-5 straight arms snapping out from the face in a fan).
// Whipping arms are rope sims, so they lag, curl and crack instead of turning as rigid lines;
// the whole length hits. Shielding a strike staggers the slot, feeding the core-exposure loop.
// --------------------------------------------------------------------------

namespace
{
constexpr int32_t krakenKindLash = 0;
constexpr int32_t krakenKindPortal = 1;
constexpr int32_t krakenKindRake = 2;
} // namespace

auto krakenBodyCenter(const Boss& boss) -> Vector2
{
    return Vector2{.x = boss.position.x + boss.size.x / 2, .y = boss.position.y + boss.size.y / 2};
}

// Lays a rope out from root along dir, `length` long.
void seedKrakenRope(Boss& boss, int32_t rope, Vector2 root, Vector2 dir, float length)
{
    auto& pos = boss.ropePos.at(static_cast<size_t>(rope));
    auto& old = boss.ropeOld.at(static_cast<size_t>(rope));
    for (int32_t i = 0; i < Boss::krakenRopePoints; i++)
    {
        const float f = static_cast<float>(i) / static_cast<float>(Boss::krakenRopePoints - 1);
        pos.at(static_cast<size_t>(i)) = Vector2Add(root, Vector2Scale(dir, length * f));
        old.at(static_cast<size_t>(i)) = pos.at(static_cast<size_t>(i));
    }
}

// Verlet rope: pinned at root, tip pulled toward target at `pull` per second, segments held to
// length / (points - 1). A fast-moving target makes the rope lag into an S and crack.
void stepKrakenRope(Boss& boss, int32_t rope, Vector2 root, Vector2 target, float length, float pull, float deltaTime)
{
    auto& pos = boss.ropePos.at(static_cast<size_t>(rope));
    auto& old = boss.ropeOld.at(static_cast<size_t>(rope));
    constexpr int32_t n = Boss::krakenRopePoints;
    for (int32_t i = 1; i < n; i++)
    {
        const Vector2 velocity = Vector2Scale(Vector2Subtract(pos.at(static_cast<size_t>(i)), old.at(static_cast<size_t>(i))), 0.9F);
        old.at(static_cast<size_t>(i)) = pos.at(static_cast<size_t>(i));
        pos.at(static_cast<size_t>(i)) = Vector2Add(pos.at(static_cast<size_t>(i)), velocity);
    }
    pos.at(n - 1) = Vector2Lerp(pos.at(n - 1), target, std::min(1.0F, pull * deltaTime));
    const float segment = std::max(1.0F, length) / static_cast<float>(n - 1);
    for (int32_t iteration = 0; iteration < 8; iteration++)
    {
        pos.at(0) = root;
        for (int32_t i = 0; i + 1 < n; i++)
        {
            Vector2& a = pos.at(static_cast<size_t>(i));
            Vector2& b = pos.at(static_cast<size_t>(i + 1));
            const Vector2 d = Vector2Subtract(b, a);
            const float dist = std::max(0.001F, Vector2Length(d));
            const Vector2 fix = Vector2Scale(d, (dist - segment) / dist);
            if (i == 0)
            {
                b = Vector2Subtract(b, fix);
            }
            else
            {
                a = Vector2Add(a, Vector2Scale(fix, 0.5F));
                b = Vector2Subtract(b, Vector2Scale(fix, 0.5F));
            }
        }
    }
    pos.at(0) = root;
}

auto krakenRopeHits(const Boss& boss, int32_t rope, Vector2 point, float radius) -> bool
{
    const auto& pos = boss.ropePos.at(static_cast<size_t>(rope));
    for (int32_t i = 0; i + 1 < Boss::krakenRopePoints; i++)
    {
        if (CheckCollisionCircleLine(point, radius, pos.at(static_cast<size_t>(i)), pos.at(static_cast<size_t>(i + 1))))
        {
            return true;
        }
    }
    return false;
}

// Spear Rake: spear i's direction, fanned around the locked aim.
auto krakenRakeDir(const Boss& boss, int32_t slot, int32_t spear) -> Vector2
{
    const auto s = static_cast<size_t>(slot);
    const Vector2 aim = Vector2Normalize(Vector2Subtract(boss.tentacleAim.at(s), boss.krakenMouth));
    const float offset = (static_cast<float>(spear) - static_cast<float>(boss.rakeCount.at(s) - 1) / 2.0F) * krakenRakeSpreadDeg;
    return Vector2Rotate(aim, offset * DEG2RAD);
}

// How far spear i reaches at time `elapsed` into the strike (0..krakenTentacleReach).
auto krakenRakeExtent(const Boss& boss, int32_t slot, int32_t spear, float elapsed) -> float
{
    const auto count = static_cast<float>(boss.rakeCount.at(static_cast<size_t>(slot)));
    const float start = krakenRakeSpearGap * static_cast<float>(spear);
    const float holdEnd = krakenRakeSpearGap * (count - 1.0F) + krakenRakeExtend + krakenRakeHold;
    if (elapsed < start)
    {
        return 0;
    }
    if (elapsed < holdEnd)
    {
        return krakenTentacleReach * std::min(1.0F, (elapsed - start) / krakenRakeExtend);
    }
    return krakenTentacleReach * std::max(0.0F, 1.0F - (elapsed - holdEnd) / krakenRakeRetract);
}

auto krakenRakeStrikeDuration(const Boss& boss, int32_t slot) -> float
{
    return krakenRakeSpearGap * static_cast<float>(boss.rakeCount.at(static_cast<size_t>(slot)) - 1) + krakenRakeExtend +
           krakenRakeHold + krakenRakeRetract;
}

// Arm Lash and Portal Lash: the root of the whipping arm.
auto krakenLashRoot(const Boss& boss, int32_t slot) -> Vector2
{
    const auto s = static_cast<size_t>(slot);
    return boss.tentacleKind.at(s) == krakenKindPortal ? boss.tentaclePortalPos.at(s) : boss.krakenMouth;
}

// The whip sweeps from swipeStart to swipeEnd (radians, world) around the root.
auto krakenLashAngle(const Boss& boss, int32_t slot, float progress) -> float
{
    const auto s = static_cast<size_t>(slot);
    const float eased = progress * progress * (3.0F - 2.0F * progress);
    return boss.tentacleSwipeStartAngle.at(s) + (boss.tentacleSwipeEndAngle.at(s) - boss.tentacleSwipeStartAngle.at(s)) * eased;
}

void staggerKrakenSlot(Boss& boss, int32_t slot)
{
    const auto s = static_cast<size_t>(slot);
    boss.tentacleHitPlayerThisSwipe.at(s) = true;
    boss.tentaclePhase.at(s) = 4;
    boss.tentacleStateTimer.at(s) = krakenTentacleStaggerRetreatDuration;
}

// Lets the boss's ordinary body-contact shield parry also stagger a striking arm.
void krakenStaggerSwipingTentacle(Boss& boss)
{
    for (int32_t s = 0; s < krakenActiveTentacleCount(boss.krakenEncounter); s++)
    {
        const int32_t phase = boss.tentaclePhase.at(static_cast<size_t>(s));
        if (phase == 1 || phase == 2)
        {
            staggerKrakenSlot(boss, s);
            return;
        }
    }
}

void startKrakenArmAttack(Game& game, Boss& boss, int32_t slot, int32_t kind)
{
    const auto s = static_cast<size_t>(slot);
    const Vector2 player = game.run.player.position;
    boss.tentacleKind.at(s) = kind;
    boss.tentacleAim.at(s) = player;
    boss.tentaclePhase.at(s) = 1;
    boss.tentacleStateTimer.at(s) = kind == krakenKindRake ? krakenRakeTell : krakenArmTell;
    boss.tentacleHitPlayerThisSwipe.at(s) = false;
    if (kind == krakenKindRake)
    {
        boss.rakeCount.at(s) = 2 + boss.krakenEncounter;
    }
    else
    {
        if (kind == krakenKindPortal)
        {
            const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const auto dist = static_cast<float>(GetRandomValue(260, 380));
            boss.tentaclePortalPos.at(s) = Vector2Add(player, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
            boss.tentacleStateTimer.at(s) += krakenPortalOpen;
        }
        const Vector2 root = krakenLashRoot(boss, slot);
        const float toPlayer = std::atan2(player.y - root.y, player.x - root.x);
        const float side = GetRandomValue(0, 1) == 0 ? 1.0F : -1.0F;
        // Rear back well to one side, then crack across to past the other.
        boss.tentacleSwipeStartAngle.at(s) = toPlayer + side * 100.0F * DEG2RAD;
        boss.tentacleSwipeEndAngle.at(s) = toPlayer - side * 60.0F * DEG2RAD;
        seedKrakenRope(boss, slot, root, Vector2{.x = std::cos(toPlayer), .y = std::sin(toPlayer)}, krakenTentacleReach * 0.15F);
    }
    playSFX(game, game.resources.sounds.bossWindUp);
}

void updateKrakenSlot(Game& game, Boss& boss, int32_t slot, float deltaTime)
{
    const auto s = static_cast<size_t>(slot);
    const int32_t kind = boss.tentacleKind.at(s);
    auto& player = game.run.player;
    boss.tentacleStateTimer.at(s) -= deltaTime;
    const float timer = boss.tentacleStateTimer.at(s);
    const int32_t phase = boss.tentaclePhase.at(s);

    if (kind == krakenKindRake)
    {
        if (phase == 1 && timer <= 0)
        {
            boss.tentaclePhase.at(s) = 2;
            boss.tentacleStateTimer.at(s) = krakenRakeStrikeDuration(boss, slot);
            playSFX(game, game.resources.sounds.slamBoom);
        }
        else if (phase == 2)
        {
            const float elapsed = krakenRakeStrikeDuration(boss, slot) - timer;
            for (int32_t spear = 0; spear < boss.rakeCount.at(s) && !boss.tentacleHitPlayerThisSwipe.at(s); spear++)
            {
                const Vector2 tip = Vector2Add(boss.krakenMouth, Vector2Scale(krakenRakeDir(boss, slot, spear),
                                                                              krakenRakeExtent(boss, slot, spear, elapsed)));
                if (player.health > 0 &&
                    CheckCollisionCircleLine(player.position, player.radius + krakenTentacleLineHalfWidth, boss.krakenMouth, tip))
                {
                    if (player.shieldActive)
                    {
                        staggerKrakenSlot(boss, slot);
                        triggerBossShieldDashShake(game);
                        return;
                    }
                    boss.tentacleHitPlayerThisSwipe.at(s) = true;
                    damagePlayer(game, enemyDamage(game, krakenTentaclePlayerDamage));
                }
            }
            if (timer <= 0)
            {
                boss.tentaclePhase.at(s) = 0;
                boss.tentacleCooldownTimer.at(s) = krakenTentacleCooldownDuration;
            }
        }
        else if (phase == 4 && timer <= 0)
        {
            boss.tentaclePhase.at(s) = 5; // falls through to the stagger set-up below
        }
        if (boss.tentaclePhase.at(s) != 5)
        {
            return;
        }
    }
    else
    {
        const Vector2 root = krakenLashRoot(boss, slot);
        const float reach = krakenTentacleReach;
        if (phase == 1)
        {
            // Portal Lash: the portal opens first, then the arm uncoils out of it; both rear back.
            const float tell = kind == krakenKindPortal ? krakenArmTell : krakenArmTell;
            const float uncoil = std::clamp(1.0F - timer / tell, 0.0F, 1.0F);
            const float back = boss.tentacleSwipeStartAngle.at(s);
            const Vector2 target = Vector2Add(root, Vector2Scale(Vector2{.x = std::cos(back), .y = std::sin(back)}, reach * 0.55F * uncoil));
            stepKrakenRope(boss, slot, root, target, reach * (0.15F + 0.55F * uncoil), 6.0F, deltaTime);
            if (timer <= 0)
            {
                boss.tentaclePhase.at(s) = 2;
                boss.tentacleStateTimer.at(s) = krakenArmStrike;
            }
        }
        else if (phase == 2)
        {
            const float progress = std::clamp(1.0F - timer / krakenArmStrike, 0.0F, 1.0F);
            const float angle = krakenLashAngle(boss, slot, progress);
            const Vector2 target = Vector2Add(root, Vector2Scale(Vector2{.x = std::cos(angle), .y = std::sin(angle)}, reach * 1.05F));
            stepKrakenRope(boss, slot, root, target, reach, 30.0F, deltaTime);
            if (!boss.tentacleHitPlayerThisSwipe.at(s) && player.health > 0 &&
                krakenRopeHits(boss, slot, player.position, player.radius + krakenTentacleLineHalfWidth))
            {
                if (player.shieldActive)
                {
                    staggerKrakenSlot(boss, slot);
                    triggerBossShieldDashShake(game);
                    return;
                }
                boss.tentacleHitPlayerThisSwipe.at(s) = true;
                damagePlayer(game, enemyDamage(game, krakenTentaclePlayerDamage));
            }
            if (timer <= 0)
            {
                boss.tentaclePhase.at(s) = 3;
                boss.tentacleStateTimer.at(s) = krakenArmRecover;
            }
        }
        else if (phase == 3 || phase == 4)
        {
            // Follow-through, then the arm draws back in (into its portal, for Portal Lash).
            const float duration = phase == 3 ? krakenArmRecover : krakenTentacleStaggerRetreatDuration;
            const float left = std::clamp(timer / duration, 0.0F, 1.0F);
            const float end = boss.tentacleSwipeEndAngle.at(s) + (phase == 4 ? std::sin(timer * 60.0F) * 0.1F : 0.0F);
            const Vector2 target = Vector2Add(root, Vector2Scale(Vector2{.x = std::cos(end), .y = std::sin(end)}, reach * 0.5F * left));
            stepKrakenRope(boss, slot, root, target, reach * std::max(0.05F, left), 8.0F, deltaTime);
            if (timer <= 0)
            {
                boss.tentaclePhase.at(s) = phase == 4 ? 5 : 0;
                boss.tentacleCooldownTimer.at(s) = krakenTentacleCooldownDuration;
            }
        }
        if (boss.tentaclePhase.at(s) != 5)
        {
            return;
        }
    }

    // A staggered arm hangs limp out of the body's gate, open to fire.
    boss.tentaclePhase.at(s) = 0;
    boss.tentacleCooldownTimer.at(s) = krakenTentacleCooldownDuration;
    boss.tentacleStaggered.at(s) = true;
    boss.tentacleStaggerTimer.at(s) = krakenStaggerDuration(boss.krakenEncounter);
    const Vector2 gate = krakenPodPosition(boss, slot % 4);
    boss.tentacleStaggerPos.at(s) =
        Vector2Add(gate, Vector2Scale(Vector2Normalize(Vector2Subtract(gate, krakenBodyCenter(boss))), krakenStaggerGateOffset));
}

void updateKrakenTentacle(Game& game, Boss& boss, float deltaTime)
{
    if (!boss.isKraken || boss.health <= 0)
    {
        return;
    }
    const Vector2 center = krakenBodyCenter(boss);
    const Vector2 facing = Vector2Normalize(Vector2Subtract(game.run.player.position, center));
    boss.krakenMouth = Vector2Add(center, Vector2Scale(facing, boss.size.x / 2 * 0.95F));

    const int32_t activeCount = krakenActiveTentacleCount(boss.krakenEncounter);
    const bool allStaggered = std::all_of(boss.tentacleStaggered.begin(), boss.tentacleStaggered.begin() + activeCount,
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
                    boss.tentacleCooldownTimer[static_cast<size_t>(slot)] = krakenTentacleCooldownDuration;
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
        if (boss.tentaclePhase[slot] != 0)
        {
            updateKrakenSlot(game, boss, s, deltaTime);
        }
    }

    boss.tentacleCheckTimer -= deltaTime;
    if (boss.tentacleCheckTimer > 0 || boss.qteStunTimer > 0)
    {
        return;
    }
    boss.tentacleCheckTimer = krakenTentacleCheckInterval;

    // Every idle, off-cooldown slot gets its own roll for an attack.
    for (int32_t s = 0; s < activeCount; s++)
    {
        const auto slot = static_cast<size_t>(s);
        const bool forced = boss.sandboxKrakenKind >= 0 && boss.sandboxKrakenKind <= krakenKindRake;
        if (!krakenTentacleSlotIdle(boss, s) || (boss.tentacleCooldownTimer[slot] > 0 && !forced))
        {
            continue;
        }
        if (!forced && static_cast<float>(GetRandomValue(0, 99)) / 100.0F >= krakenTentacleTriggerChance)
        {
            continue;
        }
        const int32_t roll = GetRandomValue(0, 99);
        const int32_t kind = forced       ? boss.sandboxKrakenKind
                             : roll < 40  ? krakenKindLash
                             : roll < 75  ? krakenKindPortal
                                          : krakenKindRake;
        boss.sandboxKrakenKind = -1;
        startKrakenArmAttack(game, boss, s, kind);
    }
}

// --- Kraken Feeding Strike and Dragged Under QTE (cutscene) --------------------
//
// Feeding Strike: the two long clubbed feeding tentacles draw back, then shoot at where the
// player stood. A club touching the ship catches it: that starts Dragged Under, or a short plain
// hold inside the QTE gap. Dragged Under: a Mash to break the grip, then prompts as the
// tentacle hauls the ship toward the beak and then portal to portal while other arms lash in.
// Clean clear: the grip is severed, the finisher hits the core, then a Punish Window with the
// core open (qteStunTimer). Fail: flung out of a portal nearby with one tentacle hit.
// --------------------------------------------------------------------------

void releaseKrakenGrip(Boss& kraken)
{
    kraken.gripping = false;
    kraken.gripFromMouth = false;
    kraken.snatchPhase = 3; // the feeding tentacles reel back in
    kraken.snatchStateTimer = krakenFeedRecover;
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
    kraken.gripping = true;
    kraken.gripFromMouth = true;
    kraken.snatchPhase = 0;
    kraken.grabPromptsLeft = UpdateConstants::krakenDragPrompts;
    kraken.grabDragTarget = kraken.krakenMouth; // hauled toward the beak first
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

// The feeding tentacles' rest pose, tucked back along the mantle.
auto krakenFeedRest(const Boss& kraken, float sign) -> Vector2
{
    const Vector2 center = krakenBodyCenter(kraken);
    const Vector2 back = Vector2Normalize(Vector2Subtract(center, kraken.krakenMouth));
    const Vector2 side{.x = -back.y, .y = back.x};
    return Vector2Add(center, Vector2Add(Vector2Scale(back, kraken.size.x * 0.9F), Vector2Scale(side, sign * kraken.size.x * 0.5F)));
}

void updateKrakenSnatch(Game& game, Boss& kraken, float deltaTime)
{
    auto& player = game.run.player;
    const float feedLength = kraken.size.x / 2 * 6.5F;
    if (kraken.gripping)
    {
        return; // Dragged Under or the plain hold below owns the grip
    }
    if (kraken.snatchPhase == 0)
    {
        kraken.snatchTimer -= deltaTime;
        const bool forced = kraken.sandboxKrakenKind == 3;
        if ((kraken.snatchTimer > 0 && !forced) || kraken.qteStunTimer > 0 || !qteSlotFree(game))
        {
            return;
        }
        kraken.sandboxKrakenKind = forced ? -1 : kraken.sandboxKrakenKind;
        kraken.snatchPhase = 1;
        kraken.snatchStateTimer = krakenFeedTell;
        kraken.snatchTimer = static_cast<float>(GetRandomValue(UpdateConstants::krakenSnatchCooldownMin,
                                                               UpdateConstants::krakenSnatchCooldownMax));
        for (int32_t k = 0; k < 2; k++)
        {
            seedKrakenRope(kraken, 4 + k, kraken.krakenMouth,
                           Vector2Normalize(Vector2Subtract(krakenFeedRest(kraken, k == 0 ? 1.0F : -1.0F), kraken.krakenMouth)),
                           feedLength * 0.3F);
        }
        playSFX(game, game.resources.sounds.bossWindUp);
        return;
    }

    kraken.snatchStateTimer -= deltaTime;
    for (int32_t k = 0; k < 2; k++)
    {
        const float sign = k == 0 ? 1.0F : -1.0F;
        Vector2 target = krakenFeedRest(kraken, sign);
        float pull = 6.0F;
        float length = feedLength * 0.45F;
        if (kraken.snatchPhase == 2)
        {
            const Vector2 aimDir = Vector2Normalize(Vector2Subtract(kraken.feedAim, kraken.krakenMouth));
            target = Vector2Add(Vector2Add(kraken.feedAim, Vector2Scale(aimDir, 60.0F)),
                                Vector2Scale(Vector2{.x = -aimDir.y, .y = aimDir.x}, sign * 22.0F));
            pull = 40.0F;
            length = Vector2Distance(kraken.krakenMouth, kraken.feedAim) + 80.0F;
        }
        else if (kraken.snatchPhase == 3 || kraken.snatchPhase == 4)
        {
            length = feedLength * 0.45F * std::clamp(kraken.snatchStateTimer / krakenFeedRecover, 0.1F, 1.0F);
            if (kraken.snatchPhase == 4)
            {
                target = player.position;
                length = Vector2Distance(kraken.krakenMouth, player.position) * 1.05F;
                pull = 40.0F;
            }
        }
        stepKrakenRope(kraken, 4 + k, kraken.krakenMouth, target, length, pull, deltaTime);
    }

    if (kraken.snatchPhase == 1)
    {
        kraken.feedAim = player.position; // tracks through the tell, locks as it shoots
        if (kraken.snatchStateTimer <= 0)
        {
            kraken.snatchPhase = 2;
            kraken.snatchStateTimer = krakenFeedShoot;
            playSFX(game, game.resources.sounds.slamBoom);
        }
        return;
    }
    if (kraken.snatchPhase == 2)
    {
        for (int32_t k = 0; k < 2; k++)
        {
            const Vector2 club = kraken.ropePos.at(static_cast<size_t>(4 + k)).back();
            if (player.health > 0 && Vector2Distance(club, player.position) <= krakenFeedCatchRadius + player.radius &&
                qteSlotFree(game))
            {
                kraken.snatchPos = player.position;
                triggerShake(game, 8, 0.25F);
                if (bossQteCanStart(game, QteKind::DraggedUnder))
                {
                    beginKrakenDraggedUnder(game, kraken);
                    return;
                }
                kraken.snatchPhase = 4;
                kraken.snatchStateTimer = UpdateConstants::krakenSnatchHold;
                playSFX(game, game.resources.sounds.slamBoom);
                return;
            }
        }
        if (kraken.snatchStateTimer <= 0)
        {
            kraken.snatchPhase = 3;
            kraken.snatchStateTimer = krakenFeedRecover;
        }
        return;
    }
    if (kraken.snatchPhase == 4)
    {
        // Plain grab: held fast, then the fail outcome.
        player.position = kraken.snatchPos;
        player.velocity = Vector2{};
        if (kraken.snatchStateTimer <= 0)
        {
            flingFromKrakenGrip(game, kraken);
        }
        return;
    }
    if (kraken.snatchStateTimer <= 0)
    {
        kraken.snatchPhase = 0;
    }
}

void updateKrakenDraggedUnder(Game& game, Boss& kraken, float deltaTime)
{
    auto& run = game.run;
    if (!kraken.gripping)
    {
        endKrakenDraggedUnder(game, kraken, false);
        return;
    }
    if (kraken.gripFromMouth)
    {
        kraken.gripRoot = kraken.krakenMouth;
        kraken.grabDragTarget = Vector2Add(kraken.krakenMouth, Vector2Scale(Vector2Normalize(
                                                                   Vector2Subtract(run.player.position, kraken.krakenMouth)),
                                                               kraken.size.x * 0.6F));
    }
    if (run.qteActionTimer <= 0)
    {
        run.player.position =
            Vector2Lerp(run.player.position, kraken.grabDragTarget, std::min(1.0F, deltaTime * 0.6F));
    }
    kraken.snatchPos = run.player.position;
    stepKrakenRope(kraken, 4, kraken.gripRoot, run.player.position, Vector2Distance(kraken.gripRoot, run.player.position) * 1.05F,
                   40.0F, deltaTime);

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
            const Vector2 center = krakenBodyCenter(kraken);
            const float exitAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 exitDir{.x = std::cos(exitAngle), .y = std::sin(exitAngle)};
            spawnKillExplosion(game, run.player.position, Palette::ElementFreeze, 14, 1.0F);
            run.player.position = Vector2Add(center, Vector2Scale(exitDir, kraken.size.x / 2 + 260.0F));
            kraken.snatchPos = run.player.position;
            spawnKillExplosion(game, run.player.position, Palette::ElementFreeze, 14, 1.0F);
            const Vector2 pullDir = Vector2Rotate(exitDir, static_cast<float>(GetRandomValue(60, 120)) * DEG2RAD);
            kraken.gripFromMouth = false;
            kraken.gripRoot = Vector2Add(run.player.position, Vector2Scale(pullDir, 320.0F));
            kraken.grabDragTarget = kraken.gripRoot;
            seedKrakenRope(kraken, 4, kraken.gripRoot, Vector2Scale(pullDir, -1.0F), 320.0F);
            game.run.punctumThunderFlashTimer = punctumThunderFlashDuration; // visual only
            playSFX(game, game.resources.sounds.slamBoom);
            triggerShake(game, 10, 0.3F);
        }
        // Other arms lash in from any side.
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
