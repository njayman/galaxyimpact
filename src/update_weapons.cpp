#include "update_internal.hpp"

auto resolveExpandingWaveHit(Game& game, Vector2 from, float radius,
                             bool allowDash) -> WaveHitResult
{
    const float distToPlayer = Vector2Distance(from, game.run.player.position);
    const bool inDanger = distToPlayer <= bossConstants::maxSlamRadius + game.run.player.radius;
    const bool dashProtected = allowDash && game.run.player.dashing;

    if (inDanger &&
        (game.run.player.shieldActive || dashProtected || game.run.player.shieldStacks > 0))
    {
        if (!game.run.player.shieldActive && !dashProtected && game.run.player.shieldStacks > 0)
        {
            game.run.player.shieldStacks--;
        }
        return WaveHitResult{.resolved = true, .hit = false};
    }

    if (distToPlayer <= radius + game.run.player.radius)
    {
        return WaveHitResult{.resolved = true, .hit = true};
    }

    return WaveHitResult{.resolved = false, .hit = false};
}

auto nearestEnemy(const Game& game, Vector2 from) -> std::optional<Vector2>
{
    float best = -1;
    Vector2 target{};
    bool found = false;

    for (const auto& enemy : game.run.enemies)
    {
        if (!enemy.active)
        {
            continue;
        }
        const float d = Vector2Distance(from, enemy.position);
        if (best < 0 || d < best)
        {
            best = d;
            target = enemy.position;
            found = true;
        }
    }

    for (const auto& hazard : game.run.eliteHazards)
    {
        if (!hazard.active)
        {
            continue;
        }
        const float d = Vector2Distance(from, hazard.position);
        if (best < 0 || d < best)
        {
            best = d;
            target = hazard.position;
            found = true;
        }
    }

    for (const auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        const float d = Vector2Distance(from, bossCenter);
        if (best < 0 || d < best)
        {
            best = d;
            target = bossCenter;
            found = true;
        }
    }

    if (!found)
    {
        return std::nullopt;
    }
    return target;
}

auto farthestEnemy(const Game& game, Vector2 from) -> std::optional<Vector2>
{
    float best = -1;
    Vector2 target{};
    bool found = false;

    for (const auto& enemy : game.run.enemies)
    {
        if (!enemy.active)
        {
            continue;
        }
        const float d = Vector2Distance(from, enemy.position);
        if (d > best)
        {
            best = d;
            target = enemy.position;
            found = true;
        }
    }

    for (const auto& hazard : game.run.eliteHazards)
    {
        if (!hazard.active)
        {
            continue;
        }
        const float d = Vector2Distance(from, hazard.position);
        if (d > best)
        {
            best = d;
            target = hazard.position;
            found = true;
        }
    }

    for (const auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        const float d = Vector2Distance(from, bossCenter);
        if (d > best)
        {
            best = d;
            target = bossCenter;
            found = true;
        }
    }

    if (!found)
    {
        return std::nullopt;
    }
    return target;
}

auto nearestEnemyExcluding(const Game& game, Vector2 from,
                           Vector2 exclude) -> std::optional<Vector2>
{
    constexpr float excludeEpsilon = 0.5F;
    float best = -1;
    Vector2 target{};
    bool found = false;

    for (const auto& enemy : game.run.enemies)
    {
        if (!enemy.active || enemy.phased ||
            Vector2Distance(enemy.position, exclude) <= excludeEpsilon)
        {
            continue;
        }
        const float d = Vector2Distance(from, enemy.position);
        if (best < 0 || d < best)
        {
            best = d;
            target = enemy.position;
            found = true;
        }
    }

    for (const auto& hazard : game.run.eliteHazards)
    {
        if (!hazard.active || Vector2Distance(hazard.position, exclude) <= excludeEpsilon)
        {
            continue;
        }
        const float d = Vector2Distance(from, hazard.position);
        if (best < 0 || d < best)
        {
            best = d;
            target = hazard.position;
            found = true;
        }
    }

    for (const auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        if (Vector2Distance(bossCenter, exclude) <= excludeEpsilon)
        {
            continue;
        }
        const float d = Vector2Distance(from, bossCenter);
        if (best < 0 || d < best)
        {
            best = d;
            target = bossCenter;
            found = true;
        }
    }

    if (!found)
    {
        return std::nullopt;
    }
    return target;
}

auto orbitRadius(int32_t level) -> float { return 55 + static_cast<float>(level) * 4; }

auto orbitBladeCount(int32_t level) -> int32_t { return 2 + level / 2; }

auto orbitBladePosition(const Game& game, Vector2 center, float radius, int32_t index,
                        int32_t count) -> Vector2
{
    const float angle =
        static_cast<float>(GetTime()) * 2 * currentShip(game).orbitSpinMult +
        static_cast<float>(index) * 2 * std::numbers::pi_v<float> / static_cast<float>(count);
    return Vector2Add(center,
                      Vector2{.x = std::cos(angle) * radius, .y = std::sin(angle) * radius});
}

auto beamAimDirection(const Game& game, bool evolved) -> Vector2
{
    const Vector2 dir = aimAtMouse(game);
    if (!evolved)
    {
        return dir;
    }

    constexpr float sweepArcDegrees = 40.0F;
    constexpr float sweepSpeed = 2.0F;
    const float sweepAngle =
        std::sin(static_cast<float>(GetTime()) * sweepSpeed) * (sweepArcDegrees / 2) * DEG2RAD;
    const float cosA = std::cos(sweepAngle);
    const float sinA = std::sin(sweepAngle);
    return Vector2{.x = dir.x * cosA - dir.y * sinA, .y = dir.x * sinA + dir.y * cosA};
}

auto beamLength(const Game& game, int32_t level, bool evolved) -> float
{
    float length = (150 + static_cast<float>(level) * 20) * currentShip(game).beamLengthMult;
    if (evolved)
    {
        length *= 1.3F;
    }
    return length;
}

auto shockwaveRadius(int32_t level, bool evolved) -> float
{
    float radius = 90 + static_cast<float>(level) * 8;
    if (evolved)
    {
        radius *= 1.4F;
    }
    return radius;
}

auto weaponCooldown(const Game& game, WeaponType kind, int32_t level, bool evolved) -> float
{
    float base = weaponBaseCooldown.at(static_cast<size_t>(kind));
    base -= static_cast<float>(level) * 0.02F;
    if (base < 0.12F)
    {
        base = 0.12F;
    }
    if (kind == WeaponType::Forward)
    {
        base /= currentShip(game).forwardFireRateMult;
    }
    if (kind == WeaponType::Shock && evolved)
    {
        base /= 2.0F; // evolution doubles Shockwave's attack speed
    }
    base *= 1 - 0.1F * static_cast<float>(
                           game.run.skillLevels.at(static_cast<size_t>(SkillType::Cooldown)));

    for (const auto& hazard : game.run.eliteHazards)
    {
        if (hazard.active && hazard.role == EliteHazardRole::Suppressor)
        {
            base *= suppressorCooldownPenalty;
            break;
        }
    }

    if (base < 0.08F)
    {
        base = 0.08F;
    }
    return base;
}

auto weaponDamage(const Game& game, int32_t level) -> int32_t
{
    auto dmg = static_cast<float>(baseBulletDamage + level * 2);
    dmg *= 1 + 0.15F * static_cast<float>(
                           game.run.skillLevels.at(static_cast<size_t>(SkillType::Damage)));
    dmg *= 1 + postCapDamageBonusPerLevel * static_cast<float>(game.run.postCapDamageLevels);
    dmg *= playerConstant(game);
    dmg *= currentShip(game).damageMult;
    if (game.run.player.overchargeTimer > 0)
    {
        dmg *= overchargeDamageMult;
    }
    return static_cast<int32_t>(dmg);
}

auto ricochetLevel(const Game& game) -> int32_t
{
    for (const auto& w : game.run.weapons)
    {
        if (w.type == WeaponType::Ricochet && !w.disabled)
        {
            return w.level;
        }
    }
    return 0;
}

auto estimatePlayerDps(const Game& game) -> float
{
    float dps = 0.0F;
    for (const auto& w : game.run.weapons)
    {
        if (w.disabled)
        {
            continue;
        }
        const float dmg = static_cast<float>(weaponDamage(game, w.level));
        const float cooldown = std::max(weaponCooldown(game, w.type, w.level, w.evolved), 0.05F);

        float hitsPerCooldown = 1.0F;
        switch (w.type)
        {
        case WeaponType::Forward:
            hitsPerCooldown = 1.0F + static_cast<float>(ricochetLevel(game)) + (w.evolved ? 2.0F : 0.0F);
            break;
        case WeaponType::Orbit:
            hitsPerCooldown = static_cast<float>(orbitBladeCount(w.level)) * continuousDpsMultiplier;
            break;
        case WeaponType::FlakCannon:
        case WeaponType::ChainLightning:
            hitsPerCooldown = 3.0F;
            break;
        case WeaponType::LaserShots:
        {
            // dmg here is the raw weaponDamage; mirror the fire code's per-shot multipliers.
            hitsPerCooldown = static_cast<float>((3 + w.level / 4) * (w.evolved ? laserEvoCountMult : 1)) *
                              laserShotDamageMult * (w.evolved ? laserEvoDamageMult : 1.0F);
            break;
        }
        default:
            break;
        }
        if (w.evolved)
        {
            hitsPerCooldown *= 1.3F;
        }
        dps += dmg * hitsPerCooldown / cooldown;
    }
    return dps;
}

auto dpsHealthScale(const Game& game) -> float
{
    const float dps = estimatePlayerDps(game);
    const float ratio = dps / dpsHealthScaleReference;
    return std::clamp(std::sqrt(std::max(ratio, 1.0F)), 1.0F, dpsHealthScaleCap);
}

auto enemyHealthScale(const Game& game) -> float
{
    return waveEnemyScale(game) * dpsHealthScale(game);
}

auto followerDroneDamageFraction(int32_t level) -> float
{
    constexpr std::array<float, 4> fractionByLevel{0.4F, 0.6F, 0.6F, 0.75F};
    return fractionByLevel.at(static_cast<size_t>(std::clamp(level, 1, 4) - 1));
}

auto laserDroneDamageFraction(int32_t level) -> float
{
    constexpr std::array<float, 4> fractionByLevel{0.55F, 0.76F, 0.76F, 0.98F};
    return fractionByLevel.at(static_cast<size_t>(std::clamp(level, 1, 4) - 1));
}

auto mineCount(int32_t level, bool evolved) -> int
{
    int count = 2 + static_cast<int>(level) / 3;
    if (evolved)
    {
        count++;
    }
    return count;
}

auto mineRadius(bool evolved) -> float { return evolved ? 42.0F : 32.0F; }

auto mineDamage(const Game& game, int32_t level, bool evolved) -> int32_t
{
    auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, level)) * mineDamageMult);
    if (evolved)
    {
        dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
    }
    return dmg;
}

void spawnMines(Game& game, const Weapon& weapon)
{
    const int count = mineCount(weapon.level, weapon.evolved);
    const float radius = mineRadius(weapon.evolved);
    const int32_t dmg = mineDamage(game, weapon.level, weapon.evolved);

    for (int i = 0; i < count; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto dist = static_cast<float>(GetRandomValue(20, 50));
        const Vector2 pos =
            Vector2Add(game.run.player.position,
                       Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

        game.run.mines.push_back(Mine{.position = pos,
                                      .velocity = Vector2{},
                                      .fuse = mineLifetime,
                                      .radius = radius,
                                      .damage = dmg,
                                      .active = true,
                                      .evolved = weapon.evolved});
    }
}

auto nearestEnemyWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>
{
    if (const auto target = nearestEnemy(game, from);
        target.has_value() && Vector2Distance(from, *target) <= maxDist)
    {
        return target;
    }
    return std::nullopt;
}

auto nearestAsteroidWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>
{
    float best = -1;
    Vector2 target{};
    bool found = false;

    for (const auto& asteroid : game.run.asteroids)
    {
        if (!asteroid.active)
        {
            continue;
        }
        const float d = Vector2Distance(from, asteroid.position);
        if (d <= maxDist && (best < 0 || d < best))
        {
            best = d;
            target = asteroid.position;
            found = true;
        }
    }

    if (!found)
    {
        return std::nullopt;
    }
    return target;
}

void detonateMine(Game& game, size_t index)
{
    Mine& mine = game.run.mines.at(index);
    if (!mine.active)
    {
        return;
    }
    mine.active = false;
    aoePulse(game, mine.position, mine.radius, mine.damage, DamageSource::Mine);

    if (!mine.evolved)
    {
        return;
    }
    constexpr float chainRadius = 80.0F;
    const Vector2 origin = mine.position;
    for (size_t j = 0; j < game.run.mines.size(); j++)
    {
        if (j == index)
        {
            continue;
        }
        const Mine& other = game.run.mines.at(j);
        if (other.active && other.evolved && Vector2Distance(origin, other.position) <= chainRadius)
        {
            detonateMine(game, j);
        }
    }
}

void updateMines(Game& game, float deltaTime)
{
    for (size_t i = 0; i < game.run.mines.size(); i++)
    {
        Mine& mine = game.run.mines.at(i);
        if (!mine.active)
        {
            continue;
        }

        bool detonate = false;

        if (mine.evolved)
        {
            auto target = nearestEnemyWithin(game, mine.position, mineSeekRadius);
            if (!target.has_value())
            {
                target = nearestAsteroidWithin(game, mine.position, mineSeekRadius);
            }

            if (target.has_value())
            {
                const Vector2 dir = Vector2Subtract(*target, mine.position);
                if (Vector2Length(dir) <= mine.radius)
                {
                    detonate = true;
                }
                else
                {
                    mine.velocity = Vector2Scale(Vector2Normalize(dir), mineHomeSpeed);
                }
            }
            else
            {
                mine.velocity = Vector2{};
            }

            mine.position =
                Vector2Add(mine.position, Vector2Scale(mine.velocity, deltaTime * frameScale));
        }
        else if (nearestEnemyWithin(game, mine.position, mine.radius).has_value())
        {

            detonate = true;
        }

        mine.fuse -= deltaTime;
        if (mine.fuse <= 0)
        {
            detonate = true;
        }

        if (detonate)
        {
            detonateMine(game, i);
        }
    }
}

void updateWeapons(Game& game, float deltaTime)
{
    for (auto& w : game.run.weapons)
    {
        if (w.flashTimer > 0)
        {
            w.flashTimer -= deltaTime;
        }

        if (w.disabled)
        {
            continue;
        }

        if (w.type == WeaponType::Orbit || w.type == WeaponType::Beam)
        {

            continue;
        }

        w.timer -= deltaTime;
        if (w.timer > 0)
        {
            continue;
        }
        w.timer = weaponCooldown(game, w.type, w.level, w.evolved);

        switch (w.type)
        {
        case WeaponType::Forward:
        {
            const Vector2 dir = aimAtMouse(game);

            const int32_t shots = std::min(2, 1 + w.level / 4);
            constexpr float spread = 10;
            auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                            forwardDamageMult);
            Color color = weaponColor(w.type);
            if (w.evolved)
            {
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
                color = evolvedWeaponColor(w.type);
            }
            for (int32_t s = 0; s < shots; s++)
            {
                const float angleOffset =
                    (static_cast<float>(s) - static_cast<float>(shots - 1) / 2) * spread * DEG2RAD;
                const float cosA = std::cos(angleOffset);
                const float sinA = std::sin(angleOffset);
                const Vector2 rotated{.x = dir.x * cosA - dir.y * sinA,
                                      .y = dir.x * sinA + dir.y * cosA};
                game.run.bullets.push_back(Bullet{
                    .position = game.run.player.position,
                    .velocity =
                        Vector2Scale(rotated, projectileSpeed * currentShip(game).bulletSpeedMult),
                    .radius = projectileSize,
                    .color = color,
                    .active = true,
                    .damage = dmg,
                    .pierceRemaining = 0,
                    .ricochetRemaining = ricochetLevel(game),
                    .source = DamageSource::Forward,
                    .ricochetThenPhase = w.evolved,
                    .evolved = w.evolved});
            }
            playSFX(game, game.resources.sounds.shoot);
            break;
        }
        case WeaponType::Homing:
        {
            const int missiles = w.evolved ? 2 : 1;
            int32_t dmg = weaponDamage(game, w.level);
            if (w.evolved)
            {
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
            }
            bool fired = false;
            std::optional<Vector2> firstTarget;
            for (int m = 0; m < missiles; m++)
            {
                const auto target =
                    (w.evolved && m > 0 && firstTarget.has_value())
                        ? nearestEnemyExcluding(game, game.run.player.position, *firstTarget)
                        : nearestEnemy(game, game.run.player.position);
                bool huntingNewTarget = false;
                Vector2 dir{};
                if (target.has_value())
                {
                    dir = Vector2Normalize(Vector2Subtract(*target, game.run.player.position));
                    if (m == 0)
                    {
                        firstTarget = target;
                    }
                }
                else if (w.evolved && firstTarget.has_value())
                {

                    dir = Vector2Normalize(Vector2Subtract(*firstTarget, game.run.player.position));
                    huntingNewTarget = true;
                }
                else
                {
                    continue;
                }
                game.run.bossProjectiles.push_back(
                    BossProjectile{.position = game.run.player.position,
                                   .velocity = Vector2Scale(dir, homingProjSpeed * 1.5F),
                                   .radius = 6,
                                   .homing = true,
                                   .active = true,
                                   .fromPlayer = true,
                                   .damage = dmg,
                                   .huntingNewTarget = huntingNewTarget,
                                   .ricochetRemaining = ricochetLevel(game)});
                fired = true;
            }
            if (fired)
            {
                playSFX(game, game.resources.sounds.homingLaunch);
            }
            break;
        }
        case WeaponType::Shock:
        {
            const float radius = shockwaveRadius(w.level, w.evolved);
            auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                            shockDamageMult);
            if (w.evolved)
            {
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
                if (game.run.player.health < game.run.player.maxHealth)
                {
                    game.run.player.health++;
                }
            }
            {
                float knockback = shockKnockbackBase + static_cast<float>(w.level) * shockKnockbackPerLevel;
                if (w.evolved)
                {
                    knockback += shockEvolvedKnockbackBonus;
                }
                const float bossSlowMult = std::clamp(
                    shockBossSlowMultBase - static_cast<float>(w.level) * shockBossSlowMultPerLevel,
                    shockBossSlowMultMin, 1.0F);
                aoePulse(game, game.run.player.position, radius, dmg, DamageSource::Shock, knockback,
                         bossSlowMult, shockBossSlowDuration, shockProjectileRepelForce);
            }
            w.flashTimer = UpdateConstants::shockFlashDuration;
            break;
        }
        case WeaponType::Mine:
            spawnMines(game, w);
            break;
        case WeaponType::FlakCannon:
        {
            const Vector2 dir = aimAtMouse(game);
            const int32_t pellets = 3 + (w.level >= 2 ? 2 : 0);
            const auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                                  flakCannonDamageMult);
            const bool pierces = w.level >= 3;
            for (int32_t s = 0; s < pellets; s++)
            {
                const float angleOffset =
                    (static_cast<float>(s) - static_cast<float>(pellets - 1) / 2) * 8.0F * DEG2RAD;
                const float cosA = std::cos(angleOffset);
                const float sinA = std::sin(angleOffset);
                const Vector2 rotated{.x = dir.x * cosA - dir.y * sinA,
                                      .y = dir.x * sinA + dir.y * cosA};
                game.run.bullets.push_back(Bullet{
                    .position = game.run.player.position,
                    .velocity =
                        Vector2Scale(rotated, projectileSpeed * currentShip(game).bulletSpeedMult),
                    .radius = projectileSize,
                    .color = Palette::Accent,
                    .active = true,
                    .damage = dmg,
                    .pierceRemaining = pierces ? 1 : 0,
                    .ricochetRemaining = pierces ? 0 : ricochetLevel(game),
                    .splashRadius = flakSplashRadius,
                    .source = DamageSource::FlakCannon});
            }
            playSFX(game, game.resources.sounds.shoot);
            break;
        }
        case WeaponType::Railgun:
        {
            const Vector2 dir = aimAtMouse(game);
            auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                            railgunBaseDamageMult);
            if (w.level >= 3)
            {
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.3F);
            }
            game.run.bullets.push_back(Bullet{
                .position = game.run.player.position,
                .velocity =
                    Vector2Scale(dir, projectileSpeed * 1.6F * currentShip(game).bulletSpeedMult),
                .radius = projectileSize * 1.5F,
                .color = Palette::Crit,
                .active = true,
                .damage = dmg,
                .pierceRemaining = 999,
                .source = DamageSource::Railgun});
            playSFX(game, game.resources.sounds.shoot);
            break;
        }
        case WeaponType::LaserShots:
        {
            const Vector2 dir = aimAtMouse(game);
            const float lengthMult = currentShip(game).beamLengthMult;
            const Vector2 nose = Vector2Add(game.run.player.position,
                                            Vector2Scale(dir, game.run.player.radius * 1.05F));
            const auto jitter = [](float range)
            { return static_cast<float>(GetRandomValue(-100, 100)) / 100.0F * range; };
            const auto baseDamage = static_cast<float>(weaponDamage(game, w.level));
            // A loose barrage: a few thin streaks scattered across a wide field in angle,
            // side offset, start position and length, so it reads as a volley, not one line.
            // Evolved: twice the streaks, longer, pink and harder hitting.
            const int32_t streaks = (3 + w.level / 4) * (w.evolved ? laserEvoCountMult : 1);
            const auto dmg = std::max(
                1, static_cast<int32_t>(baseDamage * laserShotDamageMult * (w.evolved ? laserEvoDamageMult : 1.0F)));
            const Vector2 side{.x = -dir.y, .y = dir.x};
            // Each level widens the field a bit: +8%/level, so 1.64x at level 8.
            const float field = 1.0F + 0.08F * static_cast<float>(w.level);
            for (int32_t s = 0; s < streaks; s++)
            {
                const Vector2 d =
                    Vector2Rotate(dir, jitter(laserShotSpreadDeg * field) * DEG2RAD);
                const auto ahead = static_cast<float>(GetRandomValue(0, 45));
                const Vector2 start = Vector2Add(
                    Vector2Add(nose,
                               Vector2Scale(side, jitter(laserShotFieldHalfWidth * field))),
                    Vector2Scale(d, ahead));
                game.run.bullets.push_back(Bullet{
                    .position = start,
                    .velocity = Vector2Scale(d, projectileSpeed * laserShotSpeedMult * (1.0F + jitter(0.06F))),
                    .radius = laserShotHalfWidth,
                    .color = w.evolved ? evolvedWeaponColor(w.type) : weaponColor(w.type),
                    .active = true,
                    .damage = dmg,
                    .source = DamageSource::LaserShots,
                    .streak = laserShotStreak * lengthMult * (w.evolved ? laserEvoLengthMult : 1.0F) * (1.0F + jitter(0.3F)),
                    .halfWidth = laserShotHalfWidth,
                    .traveled = ahead,
                    .evolved = w.evolved});
            }
            playSFX(game, game.resources.sounds.shoot);
            break;
        }
        case WeaponType::Sniper:
        {
            if (const auto target = farthestEnemy(game, game.run.player.position);
                target.has_value())
            {
                const Vector2 dir =
                    Vector2Normalize(Vector2Subtract(*target, game.run.player.position));
                auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                                sniperDamageMult);
                if (w.level >= 3)
                {
                    dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.3F);
                }
                const Vector2 start = game.run.player.position;
                const Vector2 end = Vector2Add(start, Vector2Scale(dir, sniperLineLength));
                sniperLineHit(game, start, end, dmg);
                game.run.sniperShots.push_back(
                    ChainLightningBolt{.from = start, .to = end, .timer = sniperLineFlashDuration});
                playSFX(game, game.resources.sounds.shoot);
            }
            break;
        }
        case WeaponType::ChainLightning:
            fireChainLightning(game, w);
            break;
        case WeaponType::TurretDeploy:
        {
            const int32_t count = w.level >= 3 ? 2 : 1;
            for (int32_t s = 0; s < count; s++)
            {
                game.run.turrets.push_back(Turret{.position = game.run.player.position,
                                                  .life = turretLife + (w.level >= 2 ? 4.0F : 0.0F),
                                                  .fireTimer = turretFireInterval});
            }
            break;
        }
        case WeaponType::Ricochet:
        case WeaponType::FollowerDrone:
        case WeaponType::LaserDrone:
        case WeaponType::Flamethrower:
        case WeaponType::Orbit:
        case WeaponType::Beam:
        case WeaponType::Count:
            break;
        }
    }
}

void recordDamage(Game& game, DamageSource source, int32_t amount)
{
    game.run.damageMeter.bySource.at(static_cast<size_t>(source)) += amount;
    game.run.damageMeter.total += amount;
}

void aoePulse(Game& game, Vector2 center, float radius, int32_t dmg, DamageSource source,
              float knockback, float bossSlowMult, float bossSlowDuration,
              float projectileRepelForce)
{
    bool hitAny = false;

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        auto& enemy = game.run.enemies.at(j);
        if (enemy.active && !enemy.phased &&
            Vector2Distance(center, enemy.position) <=
                radius + enemyCollisionRadius(enemy))
        {
            damageEnemy(game, j, dmg);
            recordDamage(game, source, dmg);
            if (!enemy.active && (source == DamageSource::Mine || source == DamageSource::Shock))
            {
                recordWeaponKill(game, source == DamageSource::Mine ? WeaponType::Mine
                                                                    : WeaponType::Shock);
            }
            if (knockback > 0 && Vector2Distance(center, enemy.position) > 0.01F)
            {
                const Vector2 pushDir = Vector2Normalize(Vector2Subtract(enemy.position, center));
                enemy.position = Vector2Add(enemy.position, Vector2Scale(pushDir, knockback));
            }
            hitAny = true;
        }
    }

    for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
    {
        const auto& hazard = game.run.eliteHazards.at(j);
        if (hazard.active &&
            Vector2Distance(center, hazard.position) <= radius + EliteHazardConstants::radius)
        {
            damageEliteHazard(game, j, dmg);
            recordDamage(game, source, dmg);
            hitAny = true;
        }
    }

    for (auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        const float bossRadius = std::max(boss.size.x, boss.size.y) / 2;
        if (Vector2Distance(center, bossCenter) <= radius + bossRadius)
        {
            damageBoss(game, boss, dmg);
            recordDamage(game, source, dmg);
            if (bossSlowDuration > 0)
            {
                // Bosses are too heavy to knock back, so the same hit slows them (and any
                // in-progress dash) instead of shoving their position.
                boss.shockwaveSlowTimer = std::max(boss.shockwaveSlowTimer, bossSlowDuration);
                boss.shockwaveSlowMult = std::min(boss.shockwaveSlowMult, bossSlowMult);
            }
            hitAny = true;
        }
    }

    if (projectileRepelForce > 0)
    {
        for (auto& proj : game.run.bossProjectiles)
        {
            if (!proj.active || proj.fromPlayer ||
                Vector2Distance(center, proj.position) > radius + proj.radius)
            {
                continue;
            }
            const Vector2 pushDir = Vector2Distance(center, proj.position) > 0.01F
                                        ? Vector2Normalize(Vector2Subtract(proj.position, center))
                                        : Vector2{.x = 0, .y = -1};
            const float speed = Vector2Length(proj.velocity);
            proj.velocity = Vector2Scale(pushDir, speed > 0.01F ? speed : projectileRepelForce);
            proj.position = Vector2Add(proj.position, Vector2Scale(pushDir, projectileRepelForce));
            proj.homing = false; // otherwise it just curves straight back at the player
            hitAny = true;
        }
    }

    for (auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active &&
            Vector2Distance(center, asteroid.position) <= radius + asteroid.radius)
        {
            asteroid.active = false;
            game.run.score += asteroidScore(asteroid.tier);
            breakAsteroid(game, asteroid);
            hitAny = true;
        }
    }

    if (hitAny && source != DamageSource::Mine)
    {
        playSFX(game, game.resources.sounds.explosion);
    }
}

void updateOrbitBladeContact(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::Orbit; });

    if (it == game.run.weapons.end() || it->disabled)
    {
        for (auto& enemy : game.run.enemies)
        {
            enemy.orbitContact = false;
            enemy.orbitDamageAccum = 0;
        }
        for (auto& hazard : game.run.eliteHazards)
        {
            hazard.orbitContact = false;
            hazard.orbitDamageAccum = 0;
        }
        for (auto& boss : game.run.bosses)
        {
            boss.orbitContact = false;
            boss.orbitDamageAccum = 0;
        }
        return;
    }

    const Weapon& w = *it;
    float radius = orbitRadius(w.level);
    auto firstHit = static_cast<float>(weaponDamage(game, w.level));
    if (w.evolved)
    {
        radius *= 1.4F;
        firstHit *= 1.5F;
    }
    const float dps = firstHit * continuousDpsMultiplier;
    const int32_t count = orbitBladeCount(w.level);

    std::vector<Vector2> bladePositions;
    bladePositions.reserve(static_cast<size_t>(count));
    for (int32_t s = 0; s < count; s++)
    {
        bladePositions.push_back(
            orbitBladePosition(game, game.run.player.position, radius, s, count));
    }

    const auto touchesAnyBlade = [&](Vector2 pos, float targetRadius)
    {
        return std::ranges::any_of(
            bladePositions, [&](const Vector2& bp)
            { return CheckCollisionCircles(bp, orbitBladeHitRadius, pos, targetRadius); });
    };

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        auto& enemy = game.run.enemies.at(j);
        if (!enemy.active || enemy.phased ||
            !touchesAnyBlade(enemy.position, enemyCollisionRadius(enemy)))
        {
            enemy.orbitContact = false;
            enemy.orbitDamageAccum = 0;
            continue;
        }

        if (!enemy.orbitContact)
        {
            enemy.orbitContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageEnemy(game, j, hit);
            recordDamage(game, DamageSource::Orbit, hit);
            if (!enemy.active)
            {
                recordWeaponKill(game, WeaponType::Orbit);
            }
        }
        else
        {
            enemy.orbitDamageAccum += dps * deltaTime;
            if (enemy.orbitDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(enemy.orbitDamageAccum);
                damageEnemy(game, j, tick);
                recordDamage(game, DamageSource::Orbit, tick);
                if (!enemy.active)
                {
                    recordWeaponKill(game, WeaponType::Orbit);
                }
                enemy.orbitDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
    {
        auto& hazard = game.run.eliteHazards.at(j);
        if (!hazard.active || !touchesAnyBlade(hazard.position, EliteHazardConstants::radius))
        {
            hazard.orbitContact = false;
            hazard.orbitDamageAccum = 0;
            continue;
        }

        if (!hazard.orbitContact)
        {
            hazard.orbitContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageEliteHazard(game, j, hit);
            recordDamage(game, DamageSource::Orbit, hit);
        }
        else
        {
            hazard.orbitDamageAccum += dps * deltaTime;
            if (hazard.orbitDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(hazard.orbitDamageAccum);
                damageEliteHazard(game, j, tick, false);
                recordDamage(game, DamageSource::Orbit, tick);
                hazard.orbitDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Rectangle bossRect{.x = boss.position.x,
                                 .y = boss.position.y,
                                 .width = boss.size.x,
                                 .height = boss.size.y};
        const bool touching = std::ranges::any_of(
            bladePositions, [&](const Vector2& bp)
            { return CheckCollisionCircleRec(bp, orbitBladeHitRadius, bossRect); });

        if (!touching)
        {
            boss.orbitContact = false;
            boss.orbitDamageAccum = 0;
            continue;
        }

        if (!boss.orbitContact)
        {
            boss.orbitContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageBoss(game, boss, hit, true, true);
            recordDamage(game, DamageSource::Orbit, hit);
        }
        else
        {
            boss.orbitDamageAccum += dps * deltaTime;
            if (boss.orbitDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(boss.orbitDamageAccum);
                damageBoss(game, boss, tick, false, true);
                recordDamage(game, DamageSource::Orbit, tick);
                boss.orbitDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active && touchesAnyBlade(asteroid.position, asteroid.radius))
        {
            asteroid.active = false;
            game.run.score += asteroidScore(asteroid.tier);
            breakAsteroid(game, asteroid);
        }
    }

    if (w.evolved)
    {
        for (auto& projectile : game.run.bossProjectiles)
        {
            if (projectile.active && !projectile.fromPlayer &&
                touchesAnyBlade(projectile.position, projectile.radius))
            {
                projectile.active = false;
            }
        }
    }
}

void updateOrbitBladeLaunch(Game& game, float deltaTime)
{
    for (auto& w : game.run.weapons)
    {
        if (w.type != WeaponType::Orbit || w.disabled)
        {
            continue;
        }

        if (w.flashTimer > 0)
        {
            w.flashTimer -= deltaTime;
        }

        const int32_t count = orbitBladeCount(w.level);
        auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) * 0.7F);
        if (w.evolved)
        {
            dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
        }

        const Vector2 dir = aimAtMouse(game);
        const float noseAngle = std::atan2(dir.y, dir.x);
        constexpr float twoPi = 2 * std::numbers::pi_v<float>;
        const float angularStep = 2 * currentShip(game).orbitSpinMult * deltaTime;

        float ringRadius = orbitRadius(w.level);
        if (w.evolved)
        {
            ringRadius *= 1.4F;
        }

        for (int32_t s = 0; s < count; s++)
        {
            const float bladeAngle = static_cast<float>(GetTime()) * 2 * currentShip(game).orbitSpinMult +
                                     static_cast<float>(s) * twoPi / static_cast<float>(count);
            float rel = std::fmod(bladeAngle - noseAngle, twoPi);
            if (rel < 0)
            {
                rel += twoPi;
            }
            if (rel >= angularStep)
            {
                continue;
            }

            const Vector2 shotDir{.x = std::cos(bladeAngle), .y = std::sin(bladeAngle)};
            const Vector2 bladePos =
                Vector2Add(game.run.player.position, Vector2Scale(shotDir, ringRadius));

            game.run.orbitBladeProjectiles.push_back(
                OrbitBladeProjectile{.position = bladePos,
                                     .velocity = Vector2Scale(shotDir, orbitProjectileSpeed),
                                     .radius = orbitProjectileRadius,
                                     .damage = dmg,
                                     .active = true,
                                     .evolved = w.evolved});

            w.flashTimer = orbitRegrowDuration;
            playSFX(game, game.resources.sounds.homingLaunch);
        }
    }
}

auto isOutsideCameraView(const Game& game, Vector2 pos, float margin) -> bool
{
    return std::abs(pos.x - game.run.player.position.x) >
               static_cast<float>(game.resources.screenWidth) / 2 + margin ||
           std::abs(pos.y - game.run.player.position.y) >
               static_cast<float>(game.resources.screenHeight) / 2 + margin;
}

void updatePiercingProjectiles(Game& game, float deltaTime,
                               std::vector<OrbitBladeProjectile>& projectiles, DamageSource source,
                               bool despawnOnCameraExit)
{
    for (auto& proj : projectiles)
    {
        if (!proj.active)
        {
            continue;
        }

        proj.position =
            Vector2Add(proj.position, Vector2Scale(proj.velocity, deltaTime * frameScale));

        const bool offscreen =
            despawnOnCameraExit
                ? isOutsideCameraView(game, proj.position, cameraDespawnMargin)
                : Vector2Distance(proj.position, game.run.player.position) > entityDespawnRadius;
        if (offscreen)
        {
            proj.active = false;
            continue;
        }

        for (size_t j = 0; j < game.run.enemies.size(); j++)
        {
            auto& enemy = game.run.enemies.at(j);
            if (enemy.active && !enemy.phased &&
                CheckCollisionCircles(proj.position, proj.radius, enemy.position,
                                      enemyCollisionRadius(enemy)))
            {
                damageEnemy(game, j, proj.damage);
                recordDamage(game, source, proj.damage);
            }
        }

        for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
        {
            auto& hazard = game.run.eliteHazards.at(j);
            if (hazard.active && CheckCollisionCircles(proj.position, proj.radius, hazard.position,
                                                       EliteHazardConstants::radius))
            {
                damageEliteHazard(game, j, proj.damage);
                recordDamage(game, source, proj.damage);
            }
        }

        for (auto& boss : game.run.bosses)
        {
            if (boss.health <= 0)
            {
                continue;
            }
            const Rectangle bossRect{.x = boss.position.x,
                                     .y = boss.position.y,
                                     .width = boss.size.x,
                                     .height = boss.size.y};
            if (CheckCollisionCircleRec(proj.position, proj.radius, bossRect))
            {
                damageBoss(game, boss, proj.damage, true, source == DamageSource::Orbit);
                recordDamage(game, source, proj.damage);
            }
        }

        for (auto& asteroid : game.run.asteroids)
        {
            if (asteroid.active && CheckCollisionCircles(proj.position, proj.radius,
                                                         asteroid.position, asteroid.radius))
            {
                asteroid.active = false;
                game.run.score += asteroidScore(asteroid.tier);
                breakAsteroid(game, asteroid);
            }
        }
    }

    std::erase_if(projectiles, [](const OrbitBladeProjectile& p) { return !p.active; });
}

void updateOrbitBladeProjectiles(Game& game, float deltaTime)
{
    updatePiercingProjectiles(game, deltaTime, game.run.orbitBladeProjectiles, DamageSource::Orbit,
                              false);
}

void updateNerveBallProjectiles(Game& game, float deltaTime)
{
    updatePiercingProjectiles(game, deltaTime, game.run.nerveBallProjectiles, DamageSource::Nerve,
                              true);
}

void updateNerveSpiralProjectiles(Game& game, float deltaTime)
{
    for (auto& spiral : game.run.nerveSpiralProjectiles)
    {
        if (!spiral.active)
        {
            continue;
        }

        spiral.age += deltaTime;
        spiral.origin = Vector2Add(
            spiral.origin, Vector2Scale(spiral.direction, spiral.speed * deltaTime * frameScale));
        const Vector2& center = spiral.origin;

        if (spiral.age >= spiral.life || isOutsideCameraView(game, center, cameraDespawnMargin))
        {
            spiral.active = false;
            continue;
        }

        for (int32_t s = 0; s < spiral.bladeCount; s++)
        {
            const float angle = spiral.age * nerveSpiralSpinSpeed +
                                static_cast<float>(s) * 2 * std::numbers::pi_v<float> /
                                    static_cast<float>(spiral.bladeCount);
            const Vector2 bladePos =
                Vector2Add(center, Vector2{.x = std::cos(angle) * spiral.spinRadius,
                                           .y = std::sin(angle) * spiral.spinRadius});

            for (size_t j = 0; j < game.run.enemies.size(); j++)
            {
                auto& enemy = game.run.enemies.at(j);
                if (enemy.active && !enemy.phased &&
                    CheckCollisionCircles(bladePos, orbitBladeHitRadius * 2.0F, enemy.position,
                                          enemyCollisionRadius(enemy)))
                {
                    damageEnemy(game, j, spiral.damagePerBlade);
                    recordDamage(game, DamageSource::Nerve, spiral.damagePerBlade);
                    if (!enemy.active)
                    {
                        recordDashOrNerveKill(game);
                    }
                }
            }

            for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
            {
                auto& hazard = game.run.eliteHazards.at(j);
                if (hazard.active &&
                    CheckCollisionCircles(bladePos, orbitBladeHitRadius * 2.0F, hazard.position,
                                          EliteHazardConstants::radius))
                {
                    damageEliteHazard(game, j, spiral.damagePerBlade);
                    recordDamage(game, DamageSource::Nerve, spiral.damagePerBlade);
                }
            }

            for (auto& boss : game.run.bosses)
            {
                if (boss.health <= 0)
                {
                    continue;
                }
                if (boss.isKraken && boss.krakenCoreExposedTimer <= 0 && boss.qteStunTimer <= 0)
                {
                    continue;
                }
                const Rectangle bossRect{.x = boss.position.x,
                                         .y = boss.position.y,
                                         .width = boss.size.x,
                                         .height = boss.size.y};
                if (CheckCollisionCircleRec(bladePos, orbitBladeHitRadius * 2.0F, bossRect))
                {
                    damageBoss(game, boss, spiral.damagePerBlade);
                    recordDamage(game, DamageSource::Nerve, spiral.damagePerBlade);
                }
            }

            for (auto& asteroid : game.run.asteroids)
            {
                if (asteroid.active && CheckCollisionCircles(bladePos, orbitBladeHitRadius * 2.0F,
                                                             asteroid.position, asteroid.radius))
                {
                    asteroid.active = false;
                    game.run.score += asteroidScore(asteroid.tier);
                    breakAsteroid(game, asteroid);
                }
            }
        }
    }

    std::erase_if(game.run.nerveSpiralProjectiles,
                  [](const NerveSpiralProjectile& s) { return !s.active; });
}

void updateBeamContact(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::Beam; });

    if (it == game.run.weapons.end() || it->disabled)
    {
        for (auto& enemy : game.run.enemies)
        {
            enemy.beamContact = false;
            enemy.beamDamageAccum = 0;
        }
        for (auto& hazard : game.run.eliteHazards)
        {
            hazard.beamContact = false;
            hazard.beamDamageAccum = 0;
        }
        for (auto& boss : game.run.bosses)
        {
            boss.beamContact = false;
            boss.beamDamageAccum = 0;
        }
        return;
    }

    const Weapon& w = *it;
    const Vector2 dir = beamAimDirection(game, w.evolved);
    const float length = beamLength(game, w.level, w.evolved);
    auto firstHit = static_cast<float>(weaponDamage(game, w.level)) * beamDamageMult;
    if (w.evolved)
    {
        firstHit *= 1.5F;
    }
    const float dps = firstHit * continuousDpsMultiplier;
    const Vector2 start = game.run.player.position;
    const Vector2 end = Vector2Add(start, Vector2Scale(dir, length));

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        auto& enemy = game.run.enemies.at(j);
        if (!enemy.active || enemy.phased ||
            !CheckCollisionCircleLine(
                enemy.position, enemyCollisionRadius(enemy), start, end))
        {
            enemy.beamContact = false;
            enemy.beamDamageAccum = 0;
            continue;
        }

        if (!enemy.beamContact)
        {
            enemy.beamContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageEnemy(game, j, hit);
            recordDamage(game, DamageSource::Beam, hit);
            if (!enemy.active)
            {
                recordWeaponKill(game, WeaponType::Beam);
            }
        }
        else
        {
            enemy.beamDamageAccum += dps * deltaTime;
            if (enemy.beamDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(enemy.beamDamageAccum);
                damageEnemy(game, j, tick);
                recordDamage(game, DamageSource::Beam, tick);
                if (!enemy.active)
                {
                    recordWeaponKill(game, WeaponType::Beam);
                }
                enemy.beamDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
    {
        auto& hazard = game.run.eliteHazards.at(j);
        if (!hazard.active ||
            !CheckCollisionCircleLine(hazard.position, EliteHazardConstants::radius, start, end))
        {
            hazard.beamContact = false;
            hazard.beamDamageAccum = 0;
            continue;
        }

        if (!hazard.beamContact)
        {
            hazard.beamContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageEliteHazard(game, j, hit);
            recordDamage(game, DamageSource::Beam, hit);
        }
        else
        {
            hazard.beamDamageAccum += dps * deltaTime;
            if (hazard.beamDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(hazard.beamDamageAccum);
                damageEliteHazard(game, j, tick, false);
                recordDamage(game, DamageSource::Beam, tick);
                hazard.beamDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        if (!CheckCollisionCircleLine(bossCenter, boss.size.x / 2, start, end))
        {
            boss.beamContact = false;
            boss.beamDamageAccum = 0;
            continue;
        }

        if (!boss.beamContact)
        {
            boss.beamContact = true;
            const auto hit = static_cast<int32_t>(firstHit);
            damageBoss(game, boss, hit, true, true);
            recordDamage(game, DamageSource::Beam, hit);
        }
        else
        {
            boss.beamDamageAccum += dps * deltaTime;
            if (boss.beamDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(boss.beamDamageAccum);
                damageBoss(game, boss, tick, false, true);
                recordDamage(game, DamageSource::Beam, tick);
                boss.beamDamageAccum -= static_cast<float>(tick);
            }
        }
    }

    for (auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active &&
            CheckCollisionCircleLine(asteroid.position, asteroid.radius, start, end))
        {
            asteroid.active = false;
            game.run.score += asteroidScore(asteroid.tier);
            breakAsteroid(game, asteroid);
        }
    }
}

void sniperLineHit(Game& game, Vector2 start, Vector2 end, int32_t dmg)
{
    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        const auto& enemy = game.run.enemies.at(j);
        if (enemy.active && !enemy.phased &&
            CheckCollisionCircleLine(enemy.position, enemyCollisionRadius(enemy), start, end))
        {
            damageEnemy(game, j, dmg);
            recordDamage(game, DamageSource::Sniper, dmg);
            if (!enemy.active)
            {
                recordWeaponKill(game, WeaponType::Sniper);
            }
        }
    }

    for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
    {
        const auto& hazard = game.run.eliteHazards.at(j);
        if (hazard.active &&
            CheckCollisionCircleLine(hazard.position, EliteHazardConstants::radius, start, end))
        {
            damageEliteHazard(game, j, dmg);
            recordDamage(game, DamageSource::Sniper, dmg);
        }
    }

    for (auto& boss : game.run.bosses)
    {
        if (boss.health <= 0)
        {
            continue;
        }
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        if (CheckCollisionCircleLine(bossCenter, boss.size.x / 2, start, end))
        {
            damageBoss(game, boss, dmg, true, true);
            recordDamage(game, DamageSource::Sniper, dmg);
        }
    }
}

void updateSniperShots(Game& game, float deltaTime)
{
    for (auto& bolt : game.run.sniperShots)
    {
        bolt.timer -= deltaTime;
    }
    std::erase_if(game.run.sniperShots, [](const ChainLightningBolt& bolt)
                  { return bolt.timer <= 0; });

}

// Whether the current ship's signature weapon (its default, whose Nerve burst it fires) is evolved.
auto signatureEvolved(const Game& game) -> bool
{
    const WeaponType signature = currentShip(game).defaultWeapon;
    return std::ranges::any_of(game.run.weapons, [&](const Weapon& w) { return w.type == signature && w.evolved; });
}

// Laser Storm, the Interceptor's Nerve burst: a 2.5 s barrage of Laser Shots streaks, sprayed
// wherever the ship aims while it lasts (updateNerveLaserStorm).
void fireNerveLaserStorm(Game& game)
{
    game.run.nerveStormTimer = nerveStormDuration;
    game.run.nerveStormEmit = 0;
    playSFX(game, game.resources.sounds.nerveRelease);
    duckBGM(game);
    triggerShake(game, 10, 0.3F);
    triggerHitPause(game, 0.08F);
}

void updateNerveLaserStorm(Game& game, float deltaTime)
{
    if (game.run.nerveStormTimer <= 0)
    {
        return;
    }
    game.run.nerveStormTimer -= deltaTime;
    game.run.nerveStormEmit += static_cast<float>(nerveStormStreaks) / nerveStormDuration * deltaTime;
    const bool evolved = signatureEvolved(game);
    const Color color = evolved ? evolvedWeaponColor(WeaponType::LaserShots) : weaponColor(WeaponType::LaserShots);
    const Vector2 dir = aimAtMouse(game);
    const Vector2 nose =
        Vector2Add(game.run.player.position, Vector2Scale(dir, game.run.player.radius * 1.05F));
    const Vector2 side{.x = -dir.y, .y = dir.x};
    const auto jitter = [](float range)
    { return static_cast<float>(GetRandomValue(-100, 100)) / 100.0F * range; };
    for (; game.run.nerveStormEmit >= 1.0F; game.run.nerveStormEmit -= 1.0F)
    {
        const Vector2 d = Vector2Rotate(dir, jitter(nerveStormSpreadDeg) * DEG2RAD);
        const auto ahead = static_cast<float>(GetRandomValue(0, 140));
        const Vector2 start =
            Vector2Add(Vector2Add(nose, Vector2Scale(side, jitter(30.0F))), Vector2Scale(d, ahead));
        game.run.bullets.push_back(Bullet{
            .position = start,
            .velocity =
                Vector2Scale(d, projectileSpeed * laserShotSpeedMult * 1.5F * (1.0F + jitter(0.1F))),
            .radius = 2.0F,
            .color = color,
            .active = true,
            .damage = nerveBurstDamage / nerveStormDamageDivisor,
            .source = DamageSource::Nerve,
            .streak = laserShotStreak * 1.8F * (1.0F + jitter(0.3F)),
            .halfWidth = 2.0F,
            .traveled = ahead,
            .evolved = evolved});
    }
}

void fireNerveBladeTornado(Game& game)
{
    const Vector2 dir = aimAtMouse(game);

    int32_t count = 4;
    if (const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                             { return w.type == WeaponType::Orbit; });
        it != game.run.weapons.end())
    {
        count = orbitBladeCount(it->level);
    }

    const int32_t dmgPerBlade = std::max(1, nerveBurstDamage / (count * 3));

    game.run.nerveSpiralProjectiles.push_back(
        NerveSpiralProjectile{.origin = game.run.player.position,
                              .direction = dir,
                              .age = 0,
                              .speed = 9.0F,
                              .life = 8.0F,
                              .spinRadius = 200.0F,
                              .bladeCount = count,
                              .damagePerBlade = dmgPerBlade,
                              .active = true,
                              .evolved = signatureEvolved(game)});

    playSFX(game, game.resources.sounds.nerveRelease);
    duckBGM(game);
    triggerShake(game, 10, 0.3F);
    triggerHitPause(game, 0.08F);
}

void fireNerveBall(Game& game)
{
    const Vector2 dir = aimAtMouse(game);
    const int32_t dmg = std::max(1, nerveBurstDamage / 4);

    game.run.nerveBallProjectiles.push_back(
        OrbitBladeProjectile{.position = game.run.player.position,
                             .velocity = Vector2Scale(dir, 8.0F),
                             .radius = 110.0F,
                             .damage = dmg,
                             .active = true,
                             .evolved = signatureEvolved(game)});

    playSFX(game, game.resources.sounds.nerveRelease);
    duckBGM(game);
    triggerShake(game, 10, 0.3F);
    triggerHitPause(game, 0.08F);
}

void fireNerveBurst(Game& game)
{
    switch (static_cast<ShipClass>(game.resources.settings.shipIndex))
    {
    case ShipClass::Bastion:
        fireNerveBladeTornado(game);
        break;
    case ShipClass::Interceptor:
        fireNerveLaserStorm(game);
        break;
    case ShipClass::Ranger:
    case ShipClass::Count:
    default:
        fireNerveBall(game);
        break;
    }
}

namespace
{
// Laser bolts hit along their whole drawn shape; every other bullet at its head.
auto bulletTouchesCircle(const Bullet& bullet, Vector2 center, float radius) -> bool
{
    if (bullet.streak <= 0)
    {
        return CheckCollisionCircles(bullet.position, bullet.radius, center, radius);
    }
    const LaserShape shape = laserShape(bullet);
    for (int k = 0; k + 1 < shape.count; k++)
    {
        if (CheckCollisionCircleLine(center, radius + bullet.halfWidth,
                                     shape.points.at(static_cast<size_t>(k)),
                                     shape.points.at(static_cast<size_t>(k) + 1)))
        {
            return true;
        }
    }
    return false;
}

auto bulletTouchesRect(const Bullet& bullet, Rectangle rect) -> bool
{
    if (bullet.streak <= 0)
    {
        return CheckCollisionCircleRec(bullet.position, bullet.radius, rect);
    }
    // ponytail: samples the bolt's points, not its segments; fine while points sit closer
    // together than a boss is wide (an arc spaces 21 points over its length).
    const LaserShape shape = laserShape(bullet);
    for (int k = 0; k < shape.count; k++)
    {
        if (CheckCollisionCircleRec(shape.points.at(static_cast<size_t>(k)), bullet.halfWidth, rect))
        {
            return true;
        }
    }
    return false;
}

// Laser Storm streaks pile onto anything big that absorbs them, so a whole fan landing on a boss,
// tentacle or hazard deals one Nerve burst, not one per streak.
auto bulletDamageVsLarge(const Bullet& bullet) -> int32_t
{
    return bullet.source == DamageSource::Nerve ? nerveBurstDamage / nerveStormStreaks
                                                : bullet.damage;
}
} // namespace

void updateBullets(Game& game, float deltaTime)
{
    for (auto& bullet : game.run.bullets)
    {
        if (!bullet.active)
        {
            continue;
        }

        bullet.position =
            Vector2Add(bullet.position, Vector2Scale(bullet.velocity, deltaTime * frameScale));
        bullet.traveled += Vector2Length(bullet.velocity) * deltaTime * frameScale;
        applyWormholeTransit(game, bullet.position, bullet.velocity, bullet.radius);

        for (auto& boss : game.run.bosses)
        {
            if (!bullet.active || boss.health <= 0)
            {
                continue;
            }

            if (boss.isKraken)
            {
                for (int32_t slot = 0; slot < Boss::krakenLimbSlots; slot++)
                {
                    if (bullet.active && boss.tentacleStaggered.at(static_cast<size_t>(slot)) &&
                        bulletTouchesCircle(bullet,
                                            boss.tentacleStaggerPos.at(static_cast<size_t>(slot)),
                                            krakenTentacleHitRadius))
                    {
                        bullet.active = false;
                        const int32_t dmg = bulletDamageVsLarge(bullet);
                        damageBoss(game, boss, dmg, true, false, true);
                        recordDamage(game, bullet.source, dmg);
                    }
                }
                continue;
            }

            const Rectangle bossRect{.x = boss.position.x,
                                     .y = boss.position.y,
                                     .width = boss.size.x,
                                     .height = boss.size.y};
            if (bullet.active && bulletTouchesRect(bullet, bossRect))
            {
                bullet.active = false;
                if (bullet.source == DamageSource::Nerve && boss.isBanished &&
                    boss.banishedStage == 1 && !boss.banishedEyeChargeBurstUsed)
                {
                    // Laser Storm opens the Banished's eye, as the Nerve cone it replaced did.
                    damageBanishedEyeBurst(game, boss, banishedEyeBurstDamageDivisor);
                    continue;
                }
                const int32_t bossDamage = bulletDamageVsLarge(bullet);
                damageBoss(game, boss, bossDamage, true, bullet.source == DamageSource::Railgun);
                recordDamage(game, bullet.source, bossDamage);
            }
        }

        for (auto& asteroid : game.run.asteroids)
        {
            if (bullet.active && asteroid.active &&
                bulletTouchesCircle(bullet, asteroid.position, asteroid.radius))
            {
                // Laser bolts shatter rocks and fly on, as they phase through enemies; a wide arc
                // would otherwise vanish whenever any part of an arm brushed a rock.
                bullet.active = bullet.streak > 0;
                asteroid.active = false;
                game.run.score += asteroidScore(asteroid.tier);
                breakAsteroid(game, asteroid);
            }
        }

        for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
        {
            auto& hazard = game.run.eliteHazards.at(j);
            if (bullet.active && hazard.active &&
                bulletTouchesCircle(bullet, hazard.position, EliteHazardConstants::radius))
            {
                bullet.active = false;
                const int32_t dmg = bulletDamageVsLarge(bullet);
                damageEliteHazard(game, j, dmg);
                recordDamage(game, bullet.source, dmg);
            }
        }

        for (size_t j = 0; j < game.run.enemies.size(); j++)
        {
            auto& enemy = game.run.enemies.at(j);
            if (!bullet.active || !enemy.active || enemy.phased ||
                (bullet.ricochetLastHit != 0 && enemy.id == bullet.ricochetLastHit) ||
                std::ranges::find(bullet.phasedThrough, enemy.id) != bullet.phasedThrough.end())
            {
                continue;
            }
            if (bulletTouchesCircle(bullet, enemy.position, enemyCollisionRadius(enemy)))
            {
                damageEnemy(game, j, bullet.damage);
                recordDamage(game, bullet.source, bullet.damage);
                // damageEnemy can spawn enemies (Hive Node), invalidating `enemy`.
                const Enemy& hit = game.run.enemies.at(j);
                if (!hit.active)
                {
                    if (bullet.source == DamageSource::Forward)
                    {
                        recordWeaponKill(game, WeaponType::Forward);
                    }
                    else if (bullet.source == DamageSource::FlakCannon)
                    {
                        recordWeaponKill(game, WeaponType::FlakCannon);
                    }
                    else if (bullet.source == DamageSource::Railgun)
                    {
                        recordWeaponKill(game, WeaponType::Railgun);
                    }
                    else if (bullet.source == DamageSource::TurretDeploy)
                    {
                        recordWeaponKill(game, WeaponType::TurretDeploy);
                    }
                    else if (bullet.source == DamageSource::LaserShots)
                    {
                        recordWeaponKill(game, WeaponType::LaserShots);
                    }
                    else if (bullet.source == DamageSource::Nerve)
                    {
                        recordDashOrNerveKill(game);
                    }
                }

                if (bullet.splashRadius > 0)
                {
                    aoePulse(game, bullet.position, bullet.splashRadius, bullet.damage / 2,
                             bullet.source);
                }

                if (bullet.streak > 0)
                {
                    // Laser bolt: phases on through, never hitting this enemy again.
                    bullet.phasedThrough.at(bullet.phasedCount % bullet.phasedThrough.size()) =
                        hit.id;
                    bullet.phasedCount++;
                }
                else if (bullet.pierceRemaining > 0)
                {

                    bullet.pierceRemaining--;
                }
                else if (bullet.ricochetRemaining > 0)
                {

                    if (const auto next =
                            nearestEnemyExcluding(game, bullet.position, hit.position);
                        next.has_value())
                    {
                        bullet.velocity =
                            Vector2Scale(Vector2Normalize(Vector2Subtract(*next, bullet.position)),
                                         Vector2Length(bullet.velocity));
                        bullet.ricochetRemaining--;
                    }
                    // No bounce target: keep flying straight, past the enemy just hit.
                    bullet.ricochetLastHit = hit.id;
                    break;
                }
                else if (bullet.phasing)
                {
                    // already phasing: keeps flying and keeps damaging everything it touches
                }
                else if (bullet.ricochetThenPhase)
                {
                    bullet.phasing = true;
                }
                else if (bullet.ricochetLastHit != 0)
                {
                    // Bounces spent: the shot goes on through, hitting each enemy it passes once.
                    bullet.ricochetLastHit = hit.id;
                }
                else
                {
                    bullet.active = false;
                }
            }
        }

        for (auto& projectile : game.run.bossProjectiles)
        {
            if (bullet.active && projectile.active && !projectile.fromPlayer &&
                CheckCollisionCircles(bullet.position, bullet.radius, projectile.position,
                                      projectile.radius))
            {
                bullet.active = false;
                projectile.health -= 1;
                if (projectile.health <= 0)
                {
                    projectile.active = false;
                    game.run.score += 5;
                }
            }
        }

        if (Vector2Distance(bullet.position, game.run.player.position) > entityDespawnRadius)
        {
            bullet.active = false;
        }
    }
}

void fireChainLightning(Game& game, const Weapon& weapon)
{
    const auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, weapon.level)) *
                                          chainLightningDamageMult);
    // Jumps 2 + level times, never back to an enemy it already struck.
    std::vector<size_t> struck;
    Vector2 from = game.run.player.position;
    float reach = chainLightningRange;
    for (int32_t hit = 0; hit < 2 + weapon.level; hit++)
    {
        std::optional<size_t> best;
        float bestDist = reach;
        for (size_t j = 0; j < game.run.enemies.size(); j++)
        {
            const auto& enemy = game.run.enemies.at(j);
            const float dist = Vector2Distance(from, enemy.position);
            if (enemy.active && !enemy.phased && dist <= bestDist && std::ranges::find(struck, j) == struck.end())
            {
                best = j;
                bestDist = dist;
            }
        }
        if (!best.has_value())
        {
            break;
        }
        struck.push_back(*best);
        const Vector2 to = game.run.enemies.at(*best).position;
        damageEnemy(game, *best, dmg);
        recordDamage(game, DamageSource::ChainLightning, dmg);
        if (!game.run.enemies.at(*best).active)
        {
            recordWeaponKill(game, WeaponType::ChainLightning);
        }
        game.run.chainLightningBolts.push_back(ChainLightningBolt{.from = from, .to = to, .timer = chainLightningBoltLife});
        from = to;
        reach = chainLightningJumpRange;
    }

    // The chain ends in the nearest boss within reach (the first strike, when no enemy is near).
    for (auto& boss : game.run.bosses)
    {
        const Vector2 center{.x = boss.position.x + boss.size.x / 2, .y = boss.position.y + boss.size.y / 2};
        if (boss.health > 0 && Vector2Distance(from, center) <= reach + boss.size.x / 2)
        {
            damageBoss(game, boss, dmg);
            recordDamage(game, DamageSource::ChainLightning, dmg);
            game.run.chainLightningBolts.push_back(
                ChainLightningBolt{.from = from, .to = center, .timer = chainLightningBoltLife});
            break;
        }
    }
    if (!game.run.chainLightningBolts.empty())
    {
        playSFX(game, game.resources.sounds.shoot);
    }
}

void updateChainLightningBolts(Game& game, float deltaTime)
{
    for (auto& bolt : game.run.chainLightningBolts)
    {
        bolt.timer -= deltaTime;
    }
    std::erase_if(game.run.chainLightningBolts,
                  [](const ChainLightningBolt& bolt) { return bolt.timer <= 0; });
}

auto droneCountForLevel(int32_t level) -> size_t
{
    if (level >= 3)
    {
        return 2;
    }
    return level >= 1 ? 1 : 0;
}

auto dronePosition(Game& game, size_t index) -> Vector2
{
    const Vector2 dir = aimAtMouse(game);
    const Vector2 perp{.x = -dir.y, .y = dir.x};
    const auto side = index % 2 == 0 ? 1.0F : -1.0F;
    const size_t rankInt = index / 2;
    const auto rank = static_cast<float>(rankInt);
    const float lateral = side * (droneOrbitRadius + rank * 30.0F);
    return Vector2Add(game.run.player.position, Vector2Scale(perp, lateral));
}

void updateDroneContactDamage(Game& game, float deltaTime, int32_t followerDmg, int32_t laserDmg)
{
    const auto touchesDrones = [&](Vector2 enemyPos, float enemyRadius, DamageSource& sourceOut,
                                   int32_t& dmgOut)
    {
        for (const auto& drone : game.run.followerDrones)
        {
            if (Vector2Distance(enemyPos, drone.position) <= droneContactRadius + enemyRadius)
            {
                sourceOut = DamageSource::FollowerDrone;
                dmgOut = followerDmg;
                return true;
            }
        }
        for (const auto& drone : game.run.laserDrones)
        {
            if (Vector2Distance(enemyPos, drone.position) <= droneContactRadius + enemyRadius)
            {
                sourceOut = DamageSource::LaserDrone;
                dmgOut = laserDmg;
                return true;
            }
        }
        return false;
    };

    for (size_t j = 0; j < game.run.enemies.size(); j++)
    {
        auto& enemy = game.run.enemies.at(j);
        DamageSource source = DamageSource::FollowerDrone;
        int32_t baseDmg = 0;
        if (!enemy.active || enemy.phased ||
            !touchesDrones(enemy.position, enemyCollisionRadius(enemy), source, baseDmg))
        {
            enemy.droneContact = false;
            enemy.droneDamageAccum = 0;
            continue;
        }

        const float dps = static_cast<float>(baseDmg) * droneContactDpsMult;
        if (!enemy.droneContact)
        {
            enemy.droneContact = true;
            const auto hit = std::max(1, static_cast<int32_t>(dps));
            damageEnemy(game, j, hit);
            recordDamage(game, source, hit);
        }
        else
        {
            enemy.droneDamageAccum += dps * deltaTime;
            if (enemy.droneDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(enemy.droneDamageAccum);
                damageEnemy(game, j, tick);
                recordDamage(game, source, tick);
                enemy.droneDamageAccum -= static_cast<float>(tick);
            }
        }
        if (!enemy.active)
        {
            recordWeaponKill(game, source == DamageSource::FollowerDrone ? WeaponType::FollowerDrone
                                                                         : WeaponType::LaserDrone);
        }
    }
}

void updateFollowerDrones(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::FollowerDrone; });
    if (it == game.run.weapons.end() || it->disabled)
    {
        game.run.followerDrones.clear();
        return;
    }

    const Weapon& w = *it;
    const size_t desired = droneCountForLevel(w.level);
    while (game.run.followerDrones.size() < desired)
    {
        game.run.followerDrones.push_back(
            FollowerDrone{.position = game.run.player.position, .attackTimer = 0, .shotTimer = 0});
    }
    game.run.followerDrones.resize(desired);

    const auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                          followerDroneDamageFraction(w.level));
    const float interval = droneAttackIntervalBase / (w.level >= 2 ? 1.3F : 1.0F);
    const auto shotDmg = static_cast<int32_t>(static_cast<float>(dmg) * droneShotDamageMult);
    const Vector2 aimDir = aimAtMouse(game);

    for (size_t i = 0; i < game.run.followerDrones.size(); i++)
    {
        auto& drone = game.run.followerDrones.at(i);
        drone.position = dronePosition(game, i);

        drone.attackTimer -= deltaTime;
        if (drone.attackTimer <= 0)
        {
            if (const auto target = nearestEnemyWithin(game, drone.position, droneMeleeRange);
                target.has_value())
            {
                for (size_t j = 0; j < game.run.enemies.size(); j++)
                {
                    auto& enemy = game.run.enemies.at(j);
                    if (enemy.active && !enemy.phased &&
                        Vector2Distance(enemy.position, *target) <= 0.01F)
                    {
                        damageEnemy(game, j, dmg);
                        recordDamage(game, DamageSource::FollowerDrone, dmg);
                        if (!enemy.active)
                        {
                            recordWeaponKill(game, WeaponType::FollowerDrone);
                        }
                        break;
                    }
                }
                drone.attackTimer = interval;
            }
        }

        drone.shotTimer -= deltaTime;
        if (drone.shotTimer <= 0 && nearestEnemyWithin(game, drone.position, laserDroneRangeBase))
        {
            game.run.bullets.push_back(Bullet{.position = drone.position,
                                              .velocity = Vector2Scale(aimDir, droneShotSpeed),
                                              .radius = projectileSize,
                                              .color = Palette::Shield,
                                              .active = true,
                                              .damage = shotDmg,
                                              .source = DamageSource::FollowerDrone});
            drone.shotTimer = droneShotInterval;
        }
    }
}

void updateLaserDrones(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::LaserDrone; });
    if (it == game.run.weapons.end() || it->disabled)
    {
        game.run.laserDrones.clear();
        return;
    }

    const Weapon& w = *it;
    const size_t desired = droneCountForLevel(w.level);
    while (game.run.laserDrones.size() < desired)
    {
        game.run.laserDrones.push_back(LaserDrone{.position = game.run.player.position,
                                                  .attackTimer = 0,
                                                  .beamFlashTimer = 0,
                                                  .beamTarget = Vector2{}});
    }
    game.run.laserDrones.resize(desired);

    const auto dmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, w.level)) *
                                          laserDroneDamageFraction(w.level));
    const float interval = droneAttackIntervalBase;
    const float range =
        laserDroneRangeBase + (w.level >= 2 ? 60.0F : 0.0F) + (w.level >= 4 ? 60.0F : 0.0F);

    for (size_t i = 0; i < game.run.laserDrones.size(); i++)
    {
        auto& drone = game.run.laserDrones.at(i);
        drone.position = dronePosition(game, i);

        if (drone.beamFlashTimer > 0)
        {
            drone.beamFlashTimer -= deltaTime;
        }

        drone.attackTimer -= deltaTime;
        if (drone.attackTimer > 0)
        {
            continue;
        }
        if (const auto target = nearestEnemyWithin(game, drone.position, range); target.has_value())
        {
            for (size_t j = 0; j < game.run.enemies.size(); j++)
            {
                auto& enemy = game.run.enemies.at(j);
                if (enemy.active && !enemy.phased &&
                    Vector2Distance(enemy.position, *target) <= 0.01F)
                {
                    damageEnemy(game, j, dmg);
                    recordDamage(game, DamageSource::LaserDrone, dmg);
                    if (!enemy.active)
                    {
                        recordWeaponKill(game, WeaponType::LaserDrone);
                    }
                    break;
                }
            }
            drone.beamTarget = *target;
            drone.beamFlashTimer = 0.15F;
            drone.attackTimer = interval;
        }
    }
}

void updateDroneContact(Game& game, float deltaTime)
{
    if (game.run.followerDrones.empty() && game.run.laserDrones.empty())
    {
        return;
    }

    int32_t followerDmg = 0;
    if (const auto it = std::ranges::find_if(
            game.run.weapons,
            [](const Weapon& w) { return w.type == WeaponType::FollowerDrone; });
        it != game.run.weapons.end())
    {
        followerDmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, it->level)) *
                                           followerDroneDamageFraction(it->level));
    }

    int32_t laserDmg = 0;
    if (const auto it = std::ranges::find_if(
            game.run.weapons, [](const Weapon& w) { return w.type == WeaponType::LaserDrone; });
        it != game.run.weapons.end())
    {
        laserDmg = static_cast<int32_t>(static_cast<float>(weaponDamage(game, it->level)) *
                                        laserDroneDamageFraction(it->level));
    }

    updateDroneContactDamage(game, deltaTime, followerDmg, laserDmg);
}

void updateTurrets(Game& game, float deltaTime)
{
    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::TurretDeploy; });
    const int32_t dmg = it != game.run.weapons.end() ? weaponDamage(game, it->level) : 0;

    for (auto& turret : game.run.turrets)
    {
        turret.life -= deltaTime;
        turret.fireTimer -= deltaTime;
        if (turret.fireTimer > 0)
        {
            continue;
        }
        turret.fireTimer = turretFireInterval;
        if (const auto target = nearestEnemy(game, turret.position); target.has_value())
        {
            const Vector2 dir = Vector2Normalize(Vector2Subtract(*target, turret.position));
            game.run.bullets.push_back(Bullet{.position = turret.position,
                                              .velocity = Vector2Scale(dir, projectileSpeed),
                                              .radius = projectileSize,
                                              .color = Palette::Accent,
                                              .active = true,
                                              .damage = dmg,
                                              .ricochetRemaining = ricochetLevel(game),
                                              .source = DamageSource::TurretDeploy});
        }
    }
    std::erase_if(game.run.turrets, [](const Turret& turret) { return turret.life <= 0; });
}

// Flamethrower: two long heat rays from the nose that scissor slowly around the aim, burning
// everything they cross.
auto flamethrowerRayDirs(const Game& game) -> std::array<Vector2, 2>
{
    const Vector2 aim = aimAtMouse(game);
    const float splay = (flamethrowerRaySplayDeg + flamethrowerRaySwayDeg * std::sin(static_cast<float>(GetTime()) * 3.0F)) *
                        DEG2RAD;
    return {Vector2Rotate(aim, splay), Vector2Rotate(aim, -splay)};
}

auto flamethrowerFiring(const Game& game) -> bool
{
    return game.run.flamethrowerCycle < flamethrowerBurstDuration;
}

auto flamethrowerRayHalfWidth(int32_t level) -> float
{
    return flamethrowerRayHalfWidthBase + flamethrowerRayHalfWidthPerLevel * static_cast<float>(level);
}

void updateFlamethrower(Game& game, float deltaTime)
{
    for (auto& particle : game.run.flameParticles)
    {
        particle.position = Vector2Add(particle.position, particle.velocity);
        particle.life -= deltaTime;
    }
    std::erase_if(game.run.flameParticles, [](const Particle& p) { return p.life <= 0; });

    const auto it = std::ranges::find_if(game.run.weapons, [](const Weapon& w)
                                         { return w.type == WeaponType::Flamethrower; });
    if (it == game.run.weapons.end() || it->disabled)
    {
        return;
    }
    const Weapon& w = *it;
    const auto dps = static_cast<float>(weaponDamage(game, w.level)) * flamethrowerDamageMult;
    const float halfWidth = flamethrowerRayHalfWidth(w.level);
    const Vector2 start = game.run.player.position;

    // Burst, then reheat: only the burst part of the cycle burns.
    game.run.flamethrowerCycle =
        std::fmod(game.run.flamethrowerCycle + deltaTime, flamethrowerBurstDuration + flamethrowerChargeDuration);
    if (!flamethrowerFiring(game))
    {
        game.run.flamethrowerEmit = 0;
        return;
    }
    game.run.flamethrowerEmit += flamethrowerJetParticleRate * deltaTime;
    const auto emit = static_cast<int32_t>(game.run.flamethrowerEmit);
    game.run.flamethrowerEmit -= static_cast<float>(emit);

    for (const Vector2& dir : flamethrowerRayDirs(game))
    {
        const Vector2 end = Vector2Add(start, Vector2Scale(dir, flamethrowerRayLength));
        // The jet: flame particles pour out of the nozzle and race the length of the jet,
        // spreading and cooling from white-gold to red as they go (velocity is per frame).
        const float perFrame = flamethrowerRayLength / flamethrowerJetLife * deltaTime;
        for (int32_t i = 0; i < emit; i++)
        {
            const float spread = static_cast<float>(GetRandomValue(-100, 100)) / 100.0F * 2.5F * DEG2RAD;
            const float pace = static_cast<float>(GetRandomValue(80, 105)) / 100.0F;
            const int32_t heat = GetRandomValue(0, 2);
            game.run.flameParticles.push_back(Particle{
                .position = Vector2Add(start, Vector2Scale(dir, game.run.player.radius)),
                .velocity = Vector2Add(Vector2Scale(Vector2Rotate(dir, spread), perFrame * pace),
                                       Vector2Scale(game.run.player.velocity, deltaTime)),
                .radius = halfWidth * (1.6F + static_cast<float>(GetRandomValue(0, 100)) / 100.0F * 1.4F),
                .life = flamethrowerJetLife,
                .maxLife = flamethrowerJetLife,
                .color = heat == 0 ? Palette::Crit : heat == 1 ? Palette::Charge : Palette::Accent});
        }

        for (size_t j = 0; j < game.run.enemies.size(); j++)
        {
            auto& enemy = game.run.enemies.at(j);
            if (!enemy.active || enemy.phased ||
                !CheckCollisionCircleLine(enemy.position, enemyCollisionRadius(enemy) + halfWidth, start, end))
            {
                continue;
            }
            // ponytail: int ticks lose fractions at high frame rates; accumulate if low levels read weak.
            const auto tick = std::max(1, static_cast<int32_t>(dps * deltaTime));
            damageEnemy(game, j, tick);
            recordDamage(game, DamageSource::Flamethrower, tick);
            if (!enemy.active)
            {
                recordWeaponKill(game, WeaponType::Flamethrower);
            }
        }

        for (auto& boss : game.run.bosses)
        {
            const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                     .y = boss.position.y + boss.size.y / 2};
            if (boss.health <= 0 || !CheckCollisionCircleLine(bossCenter, boss.size.x / 2 + halfWidth, start, end))
            {
                continue;
            }
            const auto tick = std::max(1, static_cast<int32_t>(dps * deltaTime));
            damageBoss(game, boss, tick, false);
            recordDamage(game, DamageSource::Flamethrower, tick);
        }
    }
}
