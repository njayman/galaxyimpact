// M12 adaptive boss director for generic bosses. Instead of a fixed attack loop, the boss mixes
// movement patterns at runtime, weighted toward whichever pattern counters the player's current
// habit (TacticTracker). Whiffed dashes leave it dazed (the punish window), and it escalates at
// 50% and 20% health. Biome bosses keep their own logic and never set usesDirector.
#include "update.hpp"

#include "raymath.h"
#include "update_constants.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
// Tactic tracker.
constexpr float tacticWindow = 8.0F;          // seconds, time constant of the rolling scores
constexpr float kiteDistance = 650;           // farther than this from the boss counts as kiting
constexpr float campDistance = 260;           // closer than this counts as hugging
constexpr float campSpeedFrac = 0.25F;        // slower than this fraction of top speed: sitting
constexpr float circleAngularSpeed = 0.9F;    // rad/s around the boss that scores a full Circle
constexpr float dashImpulse = 0.17F;          // ~1 dash per 1.5s saturates DashSpam
constexpr float dominanceStart = 0.45F;       // score where a habit starts drawing counters
constexpr float dominanceRange = 0.4F;        // ...and how far above that it is fully dominant

// Pattern weights.
constexpr std::array<float, bossPatternCount> baseWeight{3.0F, 2.0F, 1.5F, 1.5F, 1.0F, 1.5F, 3.0F,
                                                                  3.0F, 2.5F};
constexpr float counterWeight = 4.0F;
constexpr float readTellDuration = 0.7F;

struct Counter
{
    Tactic habit;
    BossPattern pattern;
};
constexpr std::array<Counter, 7> counters{{{Tactic::Kite, BossPattern::Ambush},
                                           {Tactic::Kite, BossPattern::DashChain},
                                           {Tactic::Circle, BossPattern::Zone},
                                           {Tactic::Camp, BossPattern::Pressure},
                                           {Tactic::DashSpam, BossPattern::Feint},
                                           {Tactic::Turtle, BossPattern::Feint},
                                           {Tactic::Turtle, BossPattern::Pressure}}};

// Pattern timings (seconds); all scaled by phaseTimeMult.
constexpr std::array<float, 3> phaseTimeMult{1.0F, 0.85F, 0.7F};
constexpr float hoverDuration = 6.0F;
constexpr float pressureDuration = 4.5F;
constexpr float zoneDuration = 4.0F;
constexpr float chainDashWindup = 0.55F;
constexpr float chainDashGap = 0.25F;
constexpr float ambushExitDuration = 1.4F;
constexpr float ambushWindup = 0.8F;
constexpr float feintWindup = 1.0F;
constexpr float feintHold = 0.35F;
constexpr float feintMaxWait = 1.2F; // stop waiting out a dash after this long
constexpr float feintStrikeWindup = 0.3F;
constexpr float dazedDuration = 2.2F;
constexpr float phaseInvulnDuration = 1.2F;
constexpr float leadTime = 0.35F; // dashes aim where the player will be this far ahead

// Movement.
constexpr float hoverDistance = 300;
constexpr float pressureDistance = 170;
constexpr float pressureSpeedMult = 1.3F;
constexpr float ambushExitSpeedMult = 1.8F;
constexpr float gapDriftSpeedMult = 0.5F;

// Damage taken.
constexpr float dazedDamageMult = 1.5F;
constexpr float desperationDamageMult = 1.35F;

// Zone walls.
constexpr int32_t zoneClouds = 5;
constexpr float zoneAhead = 260;
constexpr float zoneSpacing = 75;
constexpr float zoneCloudLife = 6.0F;

// StrafeRun (Saucer Hunter): fly past the player, dropping a line of bullets that drifts inward.
constexpr float strafeLead = 520;      // run starts this far before the player along the line
constexpr float strafeOffset = 230;    // ...and passes this far to the player's side
constexpr float strafeSpeed = 1500;    // units/s during the run
constexpr float strafeLength = 1300;
constexpr float strafeDropInterval = 0.05F;
constexpr float strafeBulletDrift = 55; // u/s; bullets creep toward the run's centerline
constexpr float strafeBulletLife = 3.5F;
constexpr float strafeWindup = 0.45F;
constexpr float strafeSetupMax = 1.6F;  // give up flying to the start point after this long
constexpr float strafeSetupSpeedMult = 2.2F;
constexpr float strafeRecovery = 0.9F;  // hangs still after the last run: the punish window

// BladeToss / BladeCage (SpikedRing Grinder).
constexpr int32_t tossVolleys = 3;
constexpr float tossVolleyGap = 1.1F;
constexpr float tossTell = 0.3F;        // boss flashes this long before each volley
constexpr float tossFanDeg = 22;
constexpr float bladeSpeed = 900;       // u/s outward
constexpr float bladeOutTime = 0.75F;   // seconds until a blade turns back
constexpr float bladeCurve = 3.7F;      // sideways pull, so the return arc misses the outbound line
constexpr float bladeRadius = 14;
constexpr int32_t bladeHealth = 40;
constexpr int32_t cageSlots = 18;
constexpr int32_t cageGapSlots = 2;
constexpr float cageStartRadius = 400;
constexpr float cageEndRadius = 140;
constexpr float cageCloseTime = 3.0F;
constexpr float cageSpin = 0.8F;        // rad/s
constexpr float cageFlingSpeed = 720;   // u/s once the cage bursts
constexpr float cageDuration = 3.6F;

// Signature patterns only enter the pool for their archetype (sandbox can still force them).
auto signatureAllowed(const Boss& boss, BossPattern p) -> bool
{
    switch (p)
    {
    case BossPattern::StrafeRun:
        return boss.shape == BossShape::Saucer;
    case BossPattern::BladeToss:
        return boss.shape == BossShape::SpikedRing;
    case BossPattern::BladeCage:
        return boss.shape == BossShape::SpikedRing && boss.phase >= 1;
    default:
        return true;
    }
}

constexpr float twoPi = 2 * std::numbers::pi_v<float>;

auto randomUnit() -> float { return static_cast<float>(GetRandomValue(0, 10000)) / 10000.0F; }

auto dominance(const Game& game, Tactic habit) -> float
{
    const float s = game.run.tactics.score.at(static_cast<size_t>(habit));
    return std::clamp((s - dominanceStart) / dominanceRange, 0.0F, 1.0F);
}

auto timeMult(const Boss& boss) -> float { return phaseTimeMult.at(static_cast<size_t>(boss.phase)); }

auto isDashAttack(const Boss& boss) -> bool { return boss.attack == BossAttack::ChargeDash; }

// Where the player is heading, or a sideways guess if they're standing still.
auto playerHeading(const Game& game, Vector2 bossCenter) -> Vector2
{
    const Vector2 v = game.run.tactics.playerVelocity;
    if (Vector2Length(v) > 30)
    {
        return Vector2Normalize(v);
    }
    const Vector2 away = Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));
    return Vector2{.x = -away.y, .y = away.x};
}

void fireNova(Game& game, Vector2 center, int32_t count)
{
    const float offset = randomUnit() * twoPi;
    for (int32_t i = 0; i < count; i++)
    {
        const float a = offset + static_cast<float>(i) * twoPi / static_cast<float>(count);
        game.run.bossProjectiles.push_back(BossProjectile{
            .position = center,
            .velocity = Vector2{.x = std::cos(a) * spreadProjSpeed, .y = std::sin(a) * spreadProjSpeed},
            .radius = 7,
            .homing = false,
            .active = true,
            .fromPlayer = false,
            .damage = enemyDamage(game, crossfireProjectileDamage),
            .health = 1});
    }
}

// A line of anchored gas clouds across the player's path.
void layZoneWall(Game& game, Vector2 bossCenter)
{
    const Vector2 heading = playerHeading(game, bossCenter);
    const Vector2 across{.x = -heading.y, .y = heading.x};
    const Vector2 mid = Vector2Add(game.run.player.position, Vector2Scale(heading, zoneAhead));
    for (int32_t i = 0; i < zoneClouds; i++)
    {
        const float off = (static_cast<float>(i) - static_cast<float>(zoneClouds - 1) / 2) * zoneSpacing;
        game.run.gasHazards.push_back(GasHazard{.position = Vector2Add(mid, Vector2Scale(across, off)),
                                                .radius = static_cast<float>(GetRandomValue(45, 55)),
                                                .seed = static_cast<float>(GetRandomValue(0, 10000)),
                                                .life = zoneCloudLife,
                                                .anchored = true});
    }
    playSFX(game, game.resources.sounds.spreadBurst);
}

void issueAttack(Game& game, Boss& boss, BossAttack attack, float windup)
{
    forceBossAttack(game, boss, attack);
    boss.stateTimer = windup * timeMult(boss);
    boss.awaitingAttack = true;
}

void choosePattern(Game& game, Boss& boss, Vector2 bossCenter);

auto makeBlade(const Game& game, Vector2 at) -> BossProjectile
{
    return BossProjectile{.isBlade = true,
                          .position = at,
                          .velocity = Vector2{},
                          .radius = bladeRadius,
                          .homing = false,
                          .active = true,
                          .fromPlayer = false,
                          .damage = enemyDamage(game, crossfireProjectileDamage),
                          .health = bladeHealth};
}

// A fan of boomerang blades: out toward the player, then curving back past the throw point.
void throwBladeVolley(Game& game, const Boss& boss, Vector2 bossCenter)
{
    const int32_t count = 2 + boss.phase;
    const Vector2 aim = Vector2Normalize(Vector2Subtract(game.run.player.position, bossCenter));
    const float base = std::atan2(aim.y, aim.x);
    const float v0 = bladeSpeed / frameScale; // projectile velocity is per frame
    for (int32_t i = 0; i < count; i++)
    {
        const float off = (static_cast<float>(i) - static_cast<float>(count - 1) / 2) * tossFanDeg * DEG2RAD;
        const Vector2 dir{.x = std::cos(base + off), .y = std::sin(base + off)};
        const Vector2 perp{.x = -dir.y, .y = dir.x};
        const float side = i % 2 == 0 ? 1.0F : -1.0F;
        BossProjectile blade = makeBlade(game, bossCenter);
        blade.velocity = Vector2Scale(dir, v0);
        blade.accel = Vector2Add(Vector2Scale(dir, -v0 / bladeOutTime), Vector2Scale(perp, bladeCurve * side));
        blade.life = 2 * bladeOutTime + 0.2F;
        game.run.bossProjectiles.push_back(blade);
    }
    playSFX(game, game.resources.sounds.homingLaunch);
}

// A ring of blades around the player with one gap; it spins, tightens, then bursts outward.
void spawnBladeCage(Game& game)
{
    const Vector2 center = game.run.player.position;
    const int32_t gap = GetRandomValue(0, cageSlots - 1);
    const float spin = GetRandomValue(0, 1) == 0 ? cageSpin : -cageSpin;
    for (int32_t i = 0; i < cageSlots; i++)
    {
        if ((i - gap + cageSlots) % cageSlots < cageGapSlots)
        {
            continue;
        }
        const float angle = static_cast<float>(i) * twoPi / static_cast<float>(cageSlots);
        BossProjectile blade = makeBlade(game, Vector2Add(center, Vector2{.x = std::cos(angle) * cageStartRadius,
                                                                          .y = std::sin(angle) * cageStartRadius}));
        blade.orbitCenter = center;
        blade.orbitRadius = cageStartRadius;
        blade.orbitAngle = angle;
        blade.orbitSpin = spin;
        blade.orbitShrink = (cageStartRadius - cageEndRadius) / cageCloseTime;
        blade.orbitTime = cageCloseTime;
        blade.orbitFling = cageFlingSpeed / frameScale;
        blade.life = cageCloseTime + 2.0F;
        game.run.bossProjectiles.push_back(blade);
    }
    playSFX(game, game.resources.sounds.slamBoom);
    triggerShake(game, 6, 0.3F);
}

// Lay out the next run alongside the player, on boss.runSide, in a random direction along it.
void planStrafeRun(const Game& game, Boss& boss)
{
    const float sign = GetRandomValue(0, 1) == 0 ? 1.0F : -1.0F;
    boss.runDir = Vector2{.x = -boss.runSide.y * sign, .y = boss.runSide.x * sign};
    boss.runStart = Vector2Add(Vector2Add(game.run.player.position, Vector2Scale(boss.runSide, strafeOffset)),
                               Vector2Scale(boss.runDir, -strafeLead));
    boss.runEnd = Vector2Add(boss.runStart, Vector2Scale(boss.runDir, strafeLength));
    boss.runMirrorShift = Vector2Scale(boss.runSide, -2 * strafeOffset);
    boss.patternStep = 1;
    boss.patternTimer = strafeSetupMax;
}

void dropStrafeBullet(Game& game, Vector2 at, Vector2 drift)
{
    game.run.bossProjectiles.push_back(BossProjectile{.life = strafeBulletLife,
                                                      .position = at,
                                                      .velocity = drift,
                                                      .radius = 7,
                                                      .homing = false,
                                                      .active = true,
                                                      .fromPlayer = false,
                                                      .damage = enemyDamage(game, crossfireProjectileDamage),
                                                      .health = 1});
}

void runStrafe(Game& game, Boss& boss, Vector2 bossCenter, float deltaTime)
{
    switch (boss.patternStep)
    {
    case 1: // fly to the start point
        if (Vector2Distance(bossCenter, boss.runStart) < 80 || boss.patternTimer <= 0)
        {
            boss.position = Vector2Subtract(boss.runStart, Vector2Scale(boss.size, 0.5F));
            boss.patternStep = 2;
            boss.attackTimer = strafeWindup * timeMult(boss);
            boss.color = Palette::Crit;
            playSFX(game, game.resources.sounds.bossWindUp);
        }
        break;
    case 2: // telegraph, holding still
        if (boss.attackTimer <= 0)
        {
            boss.patternStep = 3;
            boss.runTravel = 0;
            boss.barrageTimer = 0;
            boss.decoyActive = boss.phase >= 1;
        }
        break;
    case 3: // the run itself
    {
        boss.runTravel += strafeSpeed * deltaTime;
        const Vector2 center = Vector2Add(boss.runStart, Vector2Scale(boss.runDir, boss.runTravel));
        boss.position = Vector2Subtract(center, Vector2Scale(boss.size, 0.5F));
        // The decoy mirrors the run on the player's other side, so the two walls close in.
        boss.decoyCenter = Vector2Add(center, boss.runMirrorShift);

        boss.barrageTimer -= deltaTime;
        if (boss.barrageTimer <= 0)
        {
            boss.barrageTimer = strafeDropInterval;
            const float drift = strafeBulletDrift / frameScale; // projectile velocity is per frame
            dropStrafeBullet(game, center, Vector2Scale(boss.runSide, -drift));
            if (boss.decoyActive)
            {
                dropStrafeBullet(game, boss.decoyCenter, Vector2Scale(boss.runSide, drift));
            }
        }

        if (boss.runTravel >= strafeLength)
        {
            boss.color = boss.baseColor;
            boss.decoyActive = false;
            if (--boss.runsLeft > 0)
            {
                boss.runSide = Vector2Negate(boss.runSide); // next pass from the other side
                planStrafeRun(game, boss);
            }
            else
            {
                boss.recoveryTimer = strafeRecovery;
                choosePattern(game, boss, center);
            }
        }
        break;
    }
    default: // step 0: interrupted (phase event); re-plan from where we are
        boss.decoyActive = false;
        planStrafeRun(game, boss);
        break;
    }
}

void startPattern(Game& game, Boss& boss, Vector2 bossCenter, BossPattern pattern)
{
    boss.pattern = pattern;
    boss.patternStep = 0;
    boss.decoyActive = false;
    boss.awaitingAttack = false;
    boss.chainConnected = false;
    boss.patternLead = pattern == BossPattern::Ambush || dominance(game, Tactic::Kite) > 0;

    const float tm = timeMult(boss);
    switch (boss.pattern)
    {
    case BossPattern::HoverShoot:
        boss.patternTimer = hoverDuration;
        boss.attackTimer = 0.6F * tm;
        break;
    case BossPattern::DashChain:
        boss.patternStep = GetRandomValue(2, 3) + boss.phase;
        boss.attackTimer = chainDashGap * tm;
        break;
    case BossPattern::Ambush:
        boss.patternTimer = ambushExitDuration;
        break;
    case BossPattern::Pressure:
        boss.patternTimer = pressureDuration;
        boss.attackTimer = 0.8F * tm;
        fireNova(game, bossCenter, 12);
        break;
    case BossPattern::Zone:
        boss.patternTimer = zoneDuration;
        boss.attackTimer = 1.0F * tm;
        layZoneWall(game, bossCenter);
        break;
    case BossPattern::Feint:
        boss.attackTimer = 0.4F * tm;
        break;
    case BossPattern::BladeToss:
        boss.runsLeft = tossVolleys;
        boss.attackTimer = 0.6F * tm;
        break;
    case BossPattern::BladeCage:
        boss.patternTimer = cageDuration;
        spawnBladeCage(game);
        break;
    case BossPattern::StrafeRun:
        boss.runsLeft = 2 + boss.phase;
        boss.runSide = Vector2Normalize(Vector2Subtract(bossCenter, game.run.player.position));
        planStrafeRun(game, boss);
        break;
    case BossPattern::Count:
        break;
    }
}

void choosePattern(Game& game, Boss& boss, Vector2 bossCenter)
{
    std::array<float, bossPatternCount> counterBonus{};
    for (const auto& c : counters)
    {
        counterBonus.at(static_cast<size_t>(c.pattern)) += counterWeight * dominance(game, c.habit);
    }

    // Ambush and Feint only join the pool from phase 2, so the opening plays straighter.
    std::array<float, bossPatternCount> weight{};
    float total = 0;
    for (int i = 0; i < bossPatternCount; i++)
    {
        const auto p = static_cast<BossPattern>(i);
        const bool locked = (boss.phase == 0 && (p == BossPattern::Ambush || p == BossPattern::Feint)) ||
                            !signatureAllowed(boss, p);
        if (p == boss.pattern || locked)
        {
            continue;
        }
        weight.at(static_cast<size_t>(i)) = baseWeight.at(static_cast<size_t>(i)) +
                                            counterBonus.at(static_cast<size_t>(i));
        total += weight.at(static_cast<size_t>(i));
    }

    float pick = randomUnit() * total;
    int chosen = 0;
    for (int i = 0; i < bossPatternCount; i++)
    {
        pick -= weight.at(static_cast<size_t>(i));
        if (weight.at(static_cast<size_t>(i)) > 0 && pick <= 0)
        {
            chosen = i;
            break;
        }
    }

    // Fairness: when the pick was mostly a counter, show the boss "reading" the player.
    if (counterBonus.at(static_cast<size_t>(chosen)) > baseWeight.at(static_cast<size_t>(chosen)))
    {
        boss.readTellTimer = readTellDuration;
        playSFX(game, game.resources.sounds.critical);
    }
    startPattern(game, boss, bossCenter, static_cast<BossPattern>(chosen));
}

// A dash pattern finished: a clean miss leaves the boss dazed and open.
void endDashPattern(Game& game, Boss& boss, Vector2 bossCenter)
{
    if (!boss.chainConnected)
    {
        boss.dazedTimer = dazedDuration;
        playSFX(game, game.resources.sounds.nerveFizzle);
        return; // choose the next pattern once the daze wears off
    }
    choosePattern(game, boss, bossCenter);
}

void triggerPhaseEvent(Game& game, Boss& boss, Vector2 bossCenter)
{
    boss.phaseInvulnTimer = phaseInvulnDuration;
    boss.dazedTimer = 0;
    boss.state = BossState::IDLE;
    boss.color = boss.baseColor;
    boss.awaitingAttack = false;
    boss.patternTimer = 0;
    boss.patternStep = 0;
    boss.decoyActive = false;
    fireNova(game, bossCenter, 16);
    triggerShake(game, 10, 0.4F);
    playSFX(game, game.resources.sounds.slamBoom);
    if (boss.typeIndex >= 0)
    {
        game.run.achievementToast =
            std::string(bossTypes.at(static_cast<size_t>(boss.typeIndex)).name) +
            (boss.phase == 2 ? ": DESPERATE" : ": ENRAGED");
        game.run.achievementToastTimer = 2.5F;
    }
}

auto patternOver(const Boss& boss) -> bool
{
    switch (boss.pattern)
    {
    case BossPattern::HoverShoot:
    case BossPattern::Pressure:
    case BossPattern::Zone:
    case BossPattern::BladeCage:
        return boss.patternTimer <= 0;
    default:
        return false; // dash patterns end themselves
    }
}

void runIdle(Game& game, Boss& boss, Vector2 bossCenter, float deltaTime)
{
    const float tm = timeMult(boss);
    boss.attackTimer -= deltaTime;

    if (boss.awaitingAttack)
    {
        boss.awaitingAttack = false;
        if (isDashAttack(boss))
        {
            boss.chainConnected = boss.chainConnected || boss.dashHitPlayer;
        }
        switch (boss.pattern)
        {
        case BossPattern::DashChain:
            boss.attackTimer = chainDashGap * tm;
            if (--boss.patternStep <= 0)
            {
                endDashPattern(game, boss, bossCenter);
                return;
            }
            break;
        case BossPattern::Ambush:
        case BossPattern::Feint:
            endDashPattern(game, boss, bossCenter);
            return;
        default:
            break;
        }
    }

    if (patternOver(boss))
    {
        choosePattern(game, boss, bossCenter);
        return;
    }

    switch (boss.pattern)
    {
    case BossPattern::HoverShoot:
    case BossPattern::Zone:
        if (boss.attackTimer <= 0)
        {
            // Dashes belong to the dash patterns; hover fire comes from the rest of the moveset.
            std::vector<BossAttack> pool;
            std::ranges::copy_if(boss.moveset, std::back_inserter(pool),
                                 [](BossAttack a) { return a != BossAttack::ChargeDash; });
            const BossAttack attack =
                pool.empty() ? BossAttack::Spread
                             : pool.at(static_cast<size_t>(
                                   GetRandomValue(0, static_cast<int32_t>(pool.size()) - 1)));
            issueAttack(game, boss, attack, bossWindupDuration(attack));
            boss.attackTimer = static_cast<float>(GetRandomValue(12, 20)) / 10.0F * tm;
        }
        break;
    case BossPattern::Pressure:
        if (boss.attackTimer <= 0)
        {
            issueAttack(game, boss, GetRandomValue(0, 1) == 0 ? BossAttack::Spread : BossAttack::Barrage,
                        0.5F);
            boss.attackTimer = static_cast<float>(GetRandomValue(6, 10)) / 10.0F * tm;
        }
        break;
    case BossPattern::DashChain:
        if (boss.attackTimer <= 0)
        {
            issueAttack(game, boss, BossAttack::ChargeDash, chainDashWindup);
        }
        break;
    case BossPattern::Ambush:
        if (boss.patternStep == 0 &&
            (boss.patternTimer <= 0 ||
             Vector2Distance(bossCenter, game.run.player.position) > kiteDistance * 1.3F))
        {
            // Reappear at the screen edge the player is heading toward, then dive in.
            const float edge = static_cast<float>(std::min(game.resources.screenWidth,
                                                           game.resources.screenHeight)) *
                               0.45F;
            const Vector2 entry = Vector2Add(game.run.player.position,
                                             Vector2Scale(playerHeading(game, bossCenter), edge));
            boss.position = Vector2Subtract(entry, Vector2Scale(boss.size, 0.5F));
            boss.patternStep = 1;
            issueAttack(game, boss, BossAttack::ChargeDash, ambushWindup);
        }
        break;
    case BossPattern::Feint:
        if (boss.patternStep == 0 && boss.attackTimer <= 0)
        {
            boss.patternStep = 1;
            boss.patternTimer = feintMaxWait;
            issueAttack(game, boss, BossAttack::ChargeDash, feintWindup);
            boss.awaitingAttack = false; // the feint is cancelled in WINDING_UP, not awaited
        }
        else if (boss.patternStep == 1)
        {
            boss.patternStep = 2; // feint windup was overridden (e.g. melee stomp): skip to the hold
        }
        else if (boss.patternStep == 2)
        {
            // Wait out a dodge, then strike where the player lands.
            if (game.run.player.dashing && boss.patternTimer > 0)
            {
                boss.attackTimer = std::max(boss.attackTimer, 0.1F);
            }
            if (boss.attackTimer <= 0)
            {
                boss.patternStep = 3;
                issueAttack(game, boss, BossAttack::ChargeDash, feintStrikeWindup);
            }
        }
        break;
    case BossPattern::StrafeRun:
        runStrafe(game, boss, bossCenter, deltaTime);
        break;
    case BossPattern::BladeToss:
        if (boss.runsLeft > 0)
        {
            boss.color = boss.attackTimer < tossTell ? Palette::Crit : boss.baseColor;
            if (boss.attackTimer <= 0)
            {
                throwBladeVolley(game, boss, bossCenter);
                boss.color = boss.baseColor;
                // After the last volley, wait for the blades to come home before moving on.
                boss.attackTimer = --boss.runsLeft > 0 ? tossVolleyGap * tm : 2 * bladeOutTime;
            }
        }
        else if (boss.attackTimer <= 0)
        {
            choosePattern(game, boss, bossCenter);
        }
        break;
    case BossPattern::BladeCage: // the cage runs itself; the boss just hovers until it ends
    case BossPattern::Count:
        break;
    }
}
} // namespace

void updateTacticTracker(Game& game, float deltaTime)
{
    auto& t = game.run.tactics;
    if (deltaTime <= 0)
    {
        return;
    }
    const float decay = std::exp(-deltaTime / tacticWindow);
    const auto feed = [&](Tactic k, float signal)
    {
        float& s = t.score.at(static_cast<size_t>(k));
        s = s * decay + signal * (1 - decay);
    };

    const Player& player = game.run.player;
    const Boss* nearest = nullptr;
    float nearestDist = 0;
    for (const auto& boss : game.run.bosses)
    {
        const Vector2 c = Vector2Add(boss.position, Vector2Scale(boss.size, 0.5F));
        const float d = Vector2Distance(c, player.position);
        if (boss.usesDirector && boss.health > 0 && (nearest == nullptr || d < nearestDist))
        {
            nearest = &boss;
            nearestDist = d;
        }
    }

    if (nearest == nullptr)
    {
        for (int i = 0; i < tacticCount; i++)
        {
            feed(static_cast<Tactic>(i), 0);
        }
        t.angularEma *= decay;
        t.primed = false;
        return;
    }

    const Vector2 rel = Vector2Subtract(
        player.position, Vector2Add(nearest->position, Vector2Scale(nearest->size, 0.5F)));
    const float angle = std::atan2(rel.y, rel.x);
    if (t.primed)
    {
        t.playerVelocity = Vector2Scale(Vector2Subtract(player.position, t.prevPosition), 1 / deltaTime);
        const float dAngle = std::remainder(angle - t.prevAngle, twoPi);
        t.angularEma = t.angularEma * decay + (dAngle / deltaTime) * (1 - decay);
    }
    const float speed = Vector2Length(t.playerVelocity);

    feed(Tactic::Kite, nearestDist > kiteDistance ? 1.0F : 0.0F);
    t.score.at(static_cast<size_t>(Tactic::Circle)) =
        std::clamp(std::abs(t.angularEma) / circleAngularSpeed, 0.0F, 1.0F);
    feed(Tactic::Camp,
         nearestDist < campDistance || speed < player.speed * frameScale * campSpeedFrac ? 1.0F : 0.0F);
    float& dashSpam = t.score.at(static_cast<size_t>(Tactic::DashSpam));
    dashSpam *= decay;
    if (player.dashing && !t.prevDashing)
    {
        dashSpam = std::min(1.0F, dashSpam + dashImpulse);
    }
    feed(Tactic::Turtle, player.shieldActive && !player.dashing ? 1.0F : 0.0F);

    t.prevPosition = player.position;
    t.prevAngle = angle;
    t.prevDashing = player.dashing;
    t.primed = true;
}

auto updateBossDirector(Game& game, Boss& boss, Vector2 bossCenter, float deltaTime) -> bool
{
    if (boss.health <= 0)
    {
        return false;
    }

    boss.readTellTimer = std::max(0.0F, boss.readTellTimer - deltaTime);
    boss.patternTimer -= deltaTime;

    const float frac = static_cast<float>(boss.health) / static_cast<float>(boss.maxHealth);
    const int32_t targetPhase = static_cast<int32_t>(frac <= 0.5F) + static_cast<int32_t>(frac <= 0.2F);
    if (targetPhase > boss.phase)
    {
        boss.phase = targetPhase;
        triggerPhaseEvent(game, boss, bossCenter);
        return true;
    }

    if (boss.phaseInvulnTimer > 0)
    {
        boss.phaseInvulnTimer -= deltaTime;
        return true;
    }

    if (boss.dazedTimer > 0)
    {
        boss.dazedTimer -= deltaTime;
        if (boss.dazedTimer <= 0)
        {
            choosePattern(game, boss, bossCenter);
        }
        return true;
    }

    switch (boss.state)
    {
    case BossState::IDLE:
        runIdle(game, boss, bossCenter, deltaTime);
        return true;
    case BossState::WINDING_UP:
        if (boss.pattern == BossPattern::Feint && boss.patternStep == 1 &&
            boss.stateTimer <= deltaTime)
        {
            // The telegraphed dash never comes: hold, then strike where the player went.
            boss.state = BossState::IDLE;
            boss.color = boss.baseColor;
            boss.patternStep = 2;
            boss.attackTimer = feintHold * timeMult(boss);
            playSFX(game, game.resources.sounds.nerveFizzle);
            return true;
        }
        return false;
    case BossState::SHOOTING:
        return false;
    }
    return false;
}

auto bossDirectorMove(const Game& game, const Boss& boss, Vector2 bossCenter) -> Vector2
{
    if (boss.dazedTimer > 0 || boss.phaseInvulnTimer > 0 ||
        (boss.state == BossState::WINDING_UP && isDashAttack(boss)))
    {
        return Vector2{}; // hold still: dazed, phasing, or telegraphing a dash
    }

    const Vector2 toPlayer = Vector2Subtract(game.run.player.position, bossCenter);
    const float dist = Vector2Length(toPlayer);
    if (dist <= 1.0F)
    {
        return Vector2{};
    }
    const Vector2 dir = Vector2Scale(toPlayer, 1.0F / dist);
    const Vector2 perp{.x = -dir.y, .y = dir.x};

    float keepDist = hoverDistance;
    float speedMult = 1.0F;
    float strafeAmount = 1.0F;
    switch (boss.pattern)
    {
    case BossPattern::Ambush:
        if (boss.patternStep == 0)
        {
            return Vector2Scale(dir, -ambushExitSpeedMult);
        }
        break;
    case BossPattern::StrafeRun:
    {
        // Fly to the run's start; during the telegraph and the run the director places it.
        const Vector2 toStart = Vector2Subtract(boss.runStart, bossCenter);
        return boss.patternStep == 1 && Vector2Length(toStart) > 1
                   ? Vector2Scale(Vector2Normalize(toStart), strafeSetupSpeedMult)
                   : Vector2{};
    }
    case BossPattern::Pressure:
        keepDist = pressureDistance;
        speedMult = pressureSpeedMult;
        strafeAmount = 0.4F;
        break;
    case BossPattern::DashChain:
    case BossPattern::Feint:
        speedMult = gapDriftSpeedMult;
        break;
    default:
        break;
    }

    const float radial = std::clamp((dist - keepDist) / keepDist, -1.5F, 1.5F);
    const float strafe = std::sin(static_cast<float>(GetTime()) * 0.9F + boss.strafePhase) * strafeAmount;
    const Vector2 move = Vector2Add(Vector2Scale(dir, radial), Vector2Scale(perp, strafe));
    return Vector2Length(move) > 0 ? Vector2Scale(Vector2Normalize(move), speedMult) : Vector2{};
}

auto bossAimPoint(const Game& game, const Boss& boss) -> Vector2
{
    if (boss.usesDirector && boss.patternLead)
    {
        return Vector2Add(game.run.player.position,
                          Vector2Scale(game.run.tactics.playerVelocity, leadTime));
    }
    return game.run.player.position;
}

auto bossDirectorDamageMult(const Boss& boss) -> float
{
    if (!boss.usesDirector)
    {
        return 1.0F;
    }
    if (boss.phaseInvulnTimer > 0)
    {
        return 0.0F;
    }
    return (boss.dazedTimer > 0 ? dazedDamageMult : 1.0F) *
           (boss.phase == 2 ? desperationDamageMult : 1.0F);
}

void forceBossPattern(Game& game, Boss& boss, BossPattern pattern)
{
    if (!boss.usesDirector || boss.health <= 0)
    {
        return;
    }
    boss.state = BossState::IDLE;
    boss.color = boss.baseColor;
    boss.dazedTimer = 0;
    boss.phaseInvulnTimer = 0;
    startPattern(game, boss, Vector2Add(boss.position, Vector2Scale(boss.size, 0.5F)), pattern);
}
