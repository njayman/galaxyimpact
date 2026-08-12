#include "update.hpp"

#include "draw.hpp"
#include "entities/ship.hpp"
#include "raymath.h"
#include "update_constants.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

auto playerConstant(const Game& game) -> float
{
    return 1 + static_cast<float>(game.run.level - 1) * UpdateConstants::playerConstantPerLevel;
}

auto nerveFrac(const Game& game) -> float
{
    return game.run.player.nerve / UpdateConstants::nerveMax;
}

void gainNerve(Game& game)
{
    game.run.player.nerve += nerveKillGain;
    if (game.run.player.nerve > UpdateConstants::nerveMax)
    {
        game.run.player.nerve = UpdateConstants::nerveMax;
    }
}

auto isNerveChargeFeeding(const Game& game) -> bool
{
    return game.run.player.charges < UpdateConstants::maxCharges && game.run.player.nerve > 0 &&
           game.run.player.nerveChargeFeedTimer >= chargeFeedDelay;
}

void updateNerve(Game& game, float deltaTime)
{
    auto& player = game.run.player;

    if (game.run.nerveBurstFlashTimer > 0)
    {
        game.run.nerveBurstFlashTimer -= deltaTime;
    }

    if (player.charges >= UpdateConstants::maxCharges)
    {
        player.nerveChargeFeedTimer = 0;
    }
    else
    {
        player.nerveChargeFeedTimer += deltaTime;
        if (isNerveChargeFeeding(game))
        {
            const float drain = std::min(player.nerve, chargeFeedDrainRate * deltaTime);
            player.nerve -= drain;
            player.chargeRegenTimer -= deltaTime * (chargeFeedRegenMult - 1.0F);
        }
    }

    if (!player.nerveCharging)
    {
        return;
    }

    if (!IsKeyDown(KEY_SPACE))
    {
        player.nerveCharging = false;
        return;
    }

    player.nerveChargeTimer -= deltaTime;
    if (player.nerveChargeTimer <= 0)
    {
        player.nerveCharging = false;
        player.nerve = 0;
        fireNerveBurst(game);
    }
}

void updateNerveBurstInput(Game& game)
{
    auto& player = game.run.player;
    if (game.run.inBanishedRealm)
    {
        player.nerve = UpdateConstants::nerveMax;
    }
    if (!player.nerveCharging && IsKeyPressed(KEY_SPACE) &&
        player.nerve >= UpdateConstants::nerveMax)
    {
        player.nerveCharging = true;
        player.nerveChargeTimer = UpdateConstants::nerveBurstWindup;
        playSFX(game, game.resources.sounds.nerveCharge);
    }
}

void damagePlayer(Game& game, int32_t amount)
{
    if (game.sandbox && !game.sandboxDeathEnabled)
    {
        return;
    }

    if (game.run.player.invulnerable)
    {
        return;
    }

    if (game.run.player.nerveCharging)
    {
        game.run.player.nerveCharging = false;
        game.run.player.nerve = 0;
        playSFX(game, game.resources.sounds.nerveFizzle);
    }

    if (game.run.player.shieldStacks > 0)
    {
        game.run.player.shieldStacks--;
        game.run.player.immunityTimer = 1.0F;
        playSFX(game, game.resources.sounds.hit);
        triggerShake(game, 3, 0.15F);
        return;
    }

    const float reduced = std::max(1.0F, static_cast<float>(amount) - currentShip(game).armor);
    game.run.player.health -= reduced;
    game.run.player.immunityTimer = 1.0F;

    if (game.run.player.health <= 0 && game.run.player.secondWindReady)
    {
        game.run.player.secondWindReady = false;
        game.run.player.health = 1.0F;
        playSFX(game, game.resources.sounds.victory);
        triggerShake(game, 8, 0.3F);
        return;
    }

    if (game.run.player.health <= 0)
    {
        game.state = GameState::GAME_OVER;
        game.menuIndex = 0;
        playSFX(game, game.resources.sounds.defeat);
        triggerShake(game, 12, 0.5F);
        triggerHitPause(game, 0.15F);
        spawnDeathExplosion(game);
    }
    else
    {
        playSFX(game, game.resources.sounds.hit);
        triggerShake(game, 4, 0.2F);
    }
}

void spawnDeathExplosion(Game& game)
{
    const std::array<Color, 3> debrisColors{Palette::Accent, Palette::Crit, Palette::Haze};

    for (int i = 0; i < 28; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const float speed = static_cast<float>(GetRandomValue(20, 80)) / 10.0F;
        const Vector2 velocity{.x = std::cos(angle) * speed, .y = std::sin(angle) * speed};
        const float life = static_cast<float>(GetRandomValue(60, 100)) / 100.0F;

        game.run.deathParticles.push_back(
            Particle{.position = game.run.player.position,
                     .velocity = velocity,
                     .radius = static_cast<float>(GetRandomValue(2, 5)),
                     .life = life,
                     .maxLife = life,
                     .color = debrisColors.at(static_cast<size_t>(
                         GetRandomValue(0, static_cast<int32_t>(debrisColors.size() - 1))))});
    }
}

void updateDeathParticles(Game& game, float deltaTime)
{
    for (auto& p : game.run.deathParticles)
    {
        p.position = Vector2Add(p.position, p.velocity);
        p.life -= deltaTime;
    }

    std::erase_if(game.run.deathParticles, [](const Particle& p) { return p.life <= 0; });
}

void updateBossDeathShockwave(Game& game, float deltaTime)
{
    for (auto& wave : game.run.bossDeathShockwaves)
    {
        wave.timer -= deltaTime;

        const float progress =
            std::clamp(1 - wave.timer / UpdateConstants::bossDeathShockwaveDuration, 0.0F, 1.0F);
        const float radius = bossConstants::maxSlamRadius * progress;

        for (size_t i = 0; i < game.run.enemies.size(); i++)
        {
            if (game.run.enemies.at(i).active &&
                Vector2Distance(wave.position, game.run.enemies.at(i).position) <= radius)
            {
                killEnemyForBossAttack(game, i, true);
            }
        }

        for (auto& asteroid : game.run.asteroids)
        {
            if (asteroid.active && Vector2Distance(wave.position, asteroid.position) <= radius)
            {
                asteroid.active = false;
            }
        }

        const float prevProgress = std::clamp(
            1 - (wave.timer + deltaTime) / UpdateConstants::bossDeathShockwaveDuration, 0.0F, 1.0F);
        const float prevRadius = bossConstants::maxSlamRadius * prevProgress;
        for (auto& other : game.run.bosses)
        {
            const Vector2 otherCenter{.x = other.position.x + other.size.x / 2,
                                      .y = other.position.y + other.size.y / 2};
            const float distToBoss = Vector2Distance(wave.position, otherCenter);
            if (distToBoss <= radius && distToBoss > prevRadius)
            {
                other.health -= std::max(1, other.maxHealth / 50);
            }
        }

        if (!wave.hit)
        {
            if (const auto result = resolveExpandingWaveHit(game, wave.position, radius, false);
                result.resolved)
            {
                wave.hit = true;
                if (result.hit)
                {
                    damagePlayer(game, enemyDamage(game, 1));
                }
            }
        }
    }

    std::erase_if(game.run.bossDeathShockwaves,
                  [](const BossDeathShockwave& wave) { return wave.timer <= 0; });
}

void updatePlayerMovement(Game& game, float deltaTime)
{
    const Vector2 startPosition = game.run.player.position;

    const bool inBlackHole =
        game.run.blackhole.active &&
        Vector2Distance(game.run.player.position, game.run.blackhole.position) <=
            game.run.blackhole.influenceRadius;

    if (game.run.player.slowTimer > 0)
    {
        game.run.player.slowTimer -= deltaTime;
    }
    if (game.run.player.dashing)
    {
        game.run.player.position =
            Vector2Add(game.run.player.position,
                       Vector2Scale(game.run.player.dashVelocity, deltaTime * frameScale));
    }
    else
    {
        float effectiveSpeed = game.run.player.speed;
        if (inBlackHole)
        {
            effectiveSpeed -= blackHoleSlow;
        }
        if (game.run.player.slowTimer > 0)
        {
            effectiveSpeed *= 0.5F;
        }
        if (effectiveSpeed < 1)
        {
            effectiveSpeed = 1;
        }

        Vector2 moveDelta{};

        if (IsKeyDown(KEY_D))
        {
            moveDelta.x += effectiveSpeed;
        }
        if (IsKeyDown(KEY_A))
        {
            moveDelta.x -= effectiveSpeed;
        }
        if (IsKeyDown(KEY_W))
        {
            moveDelta.y -= effectiveSpeed;
        }
        if (IsKeyDown(KEY_S))
        {
            moveDelta.y += effectiveSpeed;
        }

        if (game.run.punctumTrapActive && !game.run.inBanishedRealm && Vector2Length(moveDelta) == 0)
        {
            const Vector2 toCenter =
                Vector2Subtract(game.run.blackhole.position, game.run.player.position);
            if (Vector2Length(toCenter) > 1.0F)
            {
                moveDelta = Vector2Scale(Vector2Normalize(toCenter), punctumTrapIdlePullSpeed);
            }
        }

        game.run.player.position =
            Vector2Add(game.run.player.position, Vector2Scale(moveDelta, deltaTime * frameScale));
    }

    if (inBlackHole)
    {
        const Vector2 toHole =
            Vector2Subtract(game.run.blackhole.position, game.run.player.position);
        if (Vector2Length(toHole) > 0)
        {
            const Vector2 pull = Vector2Scale(Vector2Normalize(toHole), blackHolePull);
            game.run.player.position =
                Vector2Add(game.run.player.position, Vector2Scale(pull, deltaTime * frameScale));
        }
    }

    if (game.run.player.dashing)
    {
        applyWormholeTransit(game, game.run.player.position, game.run.player.dashVelocity,
                             game.run.player.radius);
    }
    else
    {
        Vector2 noVelocity{};
        applyWormholeTransit(game, game.run.player.position, noVelocity, game.run.player.radius);
    }

    game.run.player.velocity = Vector2Subtract(game.run.player.position, startPosition);

    if (game.run.inBanishedRealm || game.run.punctumTrapActive)
    {
        if (game.run.player.position.x > punctumTrapHalfWidth)
        {
            game.run.player.position.x = -punctumTrapHalfWidth;
        }
        else if (game.run.player.position.x < -punctumTrapHalfWidth)
        {
            game.run.player.position.x = punctumTrapHalfWidth;
        }
        if (game.run.player.position.y > punctumTrapHalfHeight)
        {
            game.run.player.position.y = -punctumTrapHalfHeight;
        }
        else if (game.run.player.position.y < -punctumTrapHalfHeight)
        {
            game.run.player.position.y = punctumTrapHalfHeight;
        }
    }
    else if (const float dist = Vector2Length(game.run.player.position);
            dist > GameConstants::arenaHalf)
    {
        game.run.player.position =
            Vector2Scale(Vector2Normalize(game.run.player.position), GameConstants::arenaHalf);
    }

    bool touchingAnyBoss = false;
    for (auto& boss : game.run.bosses)
    {
        const Rectangle bossRect{.x = boss.position.x,
                                 .y = boss.position.y,
                                 .width = boss.size.x,
                                 .height = boss.size.y};
        if ((boss.health <= 0 && !boss.isBanished) ||
            !CheckCollisionCircleRec(game.run.player.position, game.run.player.radius, bossRect))
        {
            continue;
        }

        touchingAnyBoss = true;

        if (boss.isBanished)
        {
            if (game.run.player.dashing && !boss.hitByDash)
            {
                boss.hitByDash = true;
                if (boss.banishedDefeated)
                {
                    if (game.run.player.shieldActive)
                    {
                        boss.health = -999999;
                        triggerBossShieldDashShake(game);
                        game.resources.achievements.infiniteModeUnlocked = true;
                        saveAchievements(game.resources.achievements);
                        game.state = GameState::ENDING;
                        game.run.player.position =
                            Vector2{.x = static_cast<float>(game.resources.windowWidth) * 0.75F,
                                    .y = static_cast<float>(game.resources.windowHeight) * 0.6F};
                    }
                }
                else if (boss.banishedStage == 1 && !boss.banishedEyeChargeBurstUsed)
                {
                    damageBanishedEyeBurst(game, boss, banishedEyeBurstDamageDivisor);
                }
            }
            continue;
        }

        if (!game.run.player.dashing && game.run.player.shieldActive && boss.parryStunTimer <= 0)
        {
            boss.parryStunTimer = parryStunDuration;
            tryComboRefund(game);
            triggerBossShieldDashShake(game);
        }

        if (game.run.player.dashing && !boss.hitByDash)
        {
            boss.hitByDash = true;
            const DashQuirk quirk = currentShip(game).dashQuirk;

            if (quirk != DashQuirk::Push)
            {
                const float quirkMult = quirk == DashQuirk::Hybrid ? 0.5F : 1.0F;

                const bool coreVulnerable = boss.isBeltbreaker && !boss.beltbreakerShielded &&
                                            countAttachedAlivePlates(game, boss) == 0;
                const float coreBonusMult =
                    coreVulnerable ? beltbreakerVulnerableDashDamageMult : 1.0F;
                const auto ramDamage = static_cast<int32_t>(
                    static_cast<float>(game.run.player.shieldActive ? bossRamDamage * 2
                                                                    : bossRamDamage) *
                    quirkMult * currentShip(game).damageMult * coreBonusMult);
                damageBoss(game, boss, ramDamage, true, true);
                if (game.run.player.shieldActive)
                {
                    game.run.player.chargeRegenTimer =
                        std::max(0.0F, game.run.player.chargeRegenTimer - shieldKillChargeRefund);
                }
            }

            const bool plateShieldDash = boss.isBeltbreakerPlate && game.run.player.shieldActive;
            const bool segmentShieldDash =
                boss.isWreckwormSegment && boss.wreckwormDetached && game.run.player.shieldActive;
            const bool bonusShieldDash = plateShieldDash || segmentShieldDash;

            if (quirk != DashQuirk::Damage)
            {
                const Vector2 pushDir = Vector2Normalize(game.run.player.dashVelocity);
                const float pushDist =
                    bonusShieldDash ? beltbreakerPlateShieldDashPushDistance : dashPushDistance;
                boss.position = Vector2Add(boss.position, Vector2Scale(pushDir, pushDist));
            }

            if (plateShieldDash)
            {
                boss.plateAttached = false;
                boss.plateReturning = false;
                boss.plateExcursionTimer = 0;
            }

            if (bonusShieldDash)
            {
                game.run.player.charges =
                    std::min(UpdateConstants::maxCharges, game.run.player.charges + 1);
                game.run.player.shieldActive = true;
                game.run.player.shieldTimer =
                    std::max(game.run.player.shieldTimer, beltbreakerPlateShieldDashShieldDuration);
            }

            if (game.run.player.shieldActive)
            {
                triggerBossShieldDashShake(game);
                boss.parryStunTimer = parryStunDuration;
                tryComboRefund(game);
            }
        }
    }

    for (size_t hi = 0; hi < game.run.eliteHazards.size(); hi++)
    {
        auto& hazard = game.run.eliteHazards.at(hi);
        if (!hazard.active ||
            !CheckCollisionCircles(game.run.player.position, game.run.player.radius,
                                   hazard.position, EliteHazardConstants::radius))
        {
            continue;
        }

        if (game.run.player.dashing && !hazard.hitByDash)
        {
            hazard.hitByDash = true;
            const DashQuirk quirk = currentShip(game).dashQuirk;

            if (quirk != DashQuirk::Push)
            {
                const auto ramDamage = static_cast<int32_t>(
                    static_cast<float>(game.run.player.shieldActive ? bossRamDamage * 2
                                                                    : bossRamDamage) *
                    (quirk == DashQuirk::Hybrid ? 0.5F : 1.0F) * currentShip(game).damageMult);
                damageEliteHazard(game, hi, ramDamage);
                if (game.run.player.shieldActive)
                {
                    game.run.player.chargeRegenTimer =
                        std::max(0.0F, game.run.player.chargeRegenTimer - shieldKillChargeRefund);
                }
            }

            if (quirk != DashQuirk::Damage)
            {
                const Vector2 pushDir = Vector2Normalize(game.run.player.dashVelocity);
                hazard.position = Vector2Add(hazard.position, Vector2Scale(pushDir, dashPushDistance));
            }

            if (game.run.player.shieldActive)
            {
                game.run.player.charges =
                    std::min(UpdateConstants::maxCharges, game.run.player.charges + 1);
                game.run.player.shieldTimer =
                    std::max(game.run.player.shieldTimer, beltbreakerPlateShieldDashShieldDuration);
            }
        }
    }

    if (touchingAnyBoss && !game.run.player.dashing && !game.run.player.shieldActive)
    {
        game.run.player.bossBodyTimer += deltaTime;
        if (game.run.player.bossBodyTimer >= bossBodyLingerLimit)
        {
            game.run.player.bossBodyTimer = 0;
            damagePlayer(game, instakillDamage);
        }
    }
    else
    {
        game.run.player.bossBodyTimer = 0;
    }

    if (game.run.player.immunityTimer > 0)
    {
        game.run.player.immunityTimer -= deltaTime;
    }
}

void updateShieldAndBarrier(Game& game, float deltaTime)
{
    if (game.run.player.shieldCooldownTimer > 0)
    {
        game.run.player.shieldCooldownTimer -= deltaTime;
    }
}

void tryComboRefund(Game& game)
{
    if (game.run.player.comboActive && game.run.player.comboRefundEligible)
    {
        game.run.player.comboRefundEligible = false;
        game.run.player.charges = std::min(UpdateConstants::maxCharges, game.run.player.charges + 1);
    }
}

auto dashChunkDuration(const Game& game) -> float
{
    return std::min(dashDuration + static_cast<float>(game.run.level - 1) * holdChunkPerLevel,
                    dashChunkCap);
}

auto chunkDuration(const Game& game) -> float
{
    return holdChunkBaseDuration + static_cast<float>(game.run.level - 1) * holdChunkPerLevel;
}

auto chargeRegenDuration(const Game& game) -> float
{
    const float duration =
        UpdateConstants::chargeRegenTime -
        static_cast<float>(game.run.skillLevels.at(static_cast<size_t>(SkillType::Barrier))) * 0.3F;
    return duration < 1 ? 1 : duration;
}

void updateAbilityCharges(Game& game, float deltaTime)
{
    if (game.run.player.charges < UpdateConstants::maxCharges)
    {
        game.run.player.chargeRegenTimer -= deltaTime;
        if (game.run.player.chargeRegenTimer <= 0)
        {
            game.run.player.charges++;
            game.run.player.chargeRegenTimer = chargeRegenDuration(game);
        }
    }

    auto& player = game.run.player;
    const float chunk = chunkDuration(game);
    const float dashChunk = dashChunkDuration(game);
    const bool infiniteCharges = game.run.inBanishedRealm;

    const auto resetDashHitFlags = [&]()
    {
        for (auto& enemy : game.run.enemies)
        {
            enemy.hitByDash = false;
        }
        for (auto& boss : game.run.bosses)
        {
            boss.hitByDash = false;
        }
        for (auto& hazard : game.run.eliteHazards)
        {
            hazard.hitByDash = false;
        }
    };

    const bool comboTrigger =
        !player.dashing && !player.shieldActive &&
        (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) ||
         (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
          (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
           static_cast<float>(GetTime()) - player.lastShieldClickTime <= comboClickWindow)) ||
         (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
          (IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
           static_cast<float>(GetTime()) - player.lastDashClickTime <= comboClickWindow)));

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        player.lastDashClickTime = static_cast<float>(GetTime());
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        player.lastShieldClickTime = static_cast<float>(GetTime());
    }

    if (comboTrigger &&
        (infiniteCharges || player.charges >= comboBaseCost || player.overdriveTimer > 0))
    {
        if (!infiniteCharges && player.overdriveTimer <= 0)
        {
            player.charges = std::max(0, player.charges - comboBaseCost);
        }
        player.comboActive = true;
        player.comboRefundEligible = true;
        player.dashing = true;
        player.shieldActive = true;
        player.invulnerable = true;
        player.dashTimer = dashChunk;
        player.shieldTimer = dashChunk;
        player.dashVelocity =
            Vector2Scale(aimAtMouse(game), dashSpeed * currentShip(game).dashDistanceMult);
        resetDashHitFlags();
    }
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !player.dashing && !player.comboActive)
    {
        if (infiniteCharges || player.charges > 0 || player.overdriveTimer > 0)
        {
            if (!infiniteCharges && player.overdriveTimer <= 0)
            {
                player.charges--;
            }
            player.dashing = true;
            player.dashTimer = dashChunk;
            player.dashVelocity =
                Vector2Scale(aimAtMouse(game), dashSpeed * currentShip(game).dashDistanceMult);
            resetDashHitFlags();
        }
    }
    else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !player.shieldActive && !player.comboActive)
    {
        if (infiniteCharges || player.charges > 0 || player.overdriveTimer > 0)
        {
            if (!infiniteCharges && player.overdriveTimer <= 0)
            {
                player.charges--;
            }
            player.shieldActive = true;
            player.shieldTimer = chunk;
        }
    }

    if (player.comboActive)
    {
        const bool stillHeld = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
                               IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
                               IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        if (!stillHeld)
        {
            player.comboActive = false;
            player.dashing = false;
            player.shieldActive = false;
            player.invulnerable = false;
            player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
        }
        else
        {
            player.dashVelocity =
                Vector2Scale(aimAtMouse(game), dashSpeed * currentShip(game).dashDistanceMult);
            player.dashTimer -= deltaTime;
            if (player.dashTimer <= 0)
            {
                if (infiniteCharges || player.charges > 0 || player.overdriveTimer > 0)
                {
                    if (!infiniteCharges && player.overdriveTimer <= 0)
                    {
                        player.charges--;
                    }
                    player.dashTimer = dashChunk;
                    player.shieldTimer = dashChunk;
                    resetDashHitFlags();
                }
                else
                {
                    player.comboActive = false;
                    player.dashing = false;
                    player.shieldActive = false;
                    player.invulnerable = false;
                    player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
                }
            }
            else
            {
                player.shieldTimer = player.dashTimer;
            }
        }
    }
    else
    {
        const bool dashWasActive = player.dashing;

        if (player.dashing)
        {
            if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                player.dashing = false;
            }
            else
            {
                player.dashVelocity = Vector2Scale(aimAtMouse(game),
                                                   dashSpeed * currentShip(game).dashDistanceMult);
                player.dashTimer -= deltaTime;
                if (player.dashTimer <= 0)
                {
                    if (infiniteCharges || player.charges > 0 || player.overdriveTimer > 0)
                    {
                        if (!infiniteCharges && player.overdriveTimer <= 0)
                        {
                            player.charges--;
                        }
                        player.dashTimer = dashChunk;
                        resetDashHitFlags();
                    }
                    else
                    {
                        player.dashing = false;
                    }
                }
            }
        }

        if (dashWasActive && !player.dashing && player.shieldDashKill)
        {
            player.shieldDashKill = false;
            player.shieldDashGraceTimer = shieldDashKillGraceDuration;
            player.shieldActive = true;
        }

        if (player.shieldDashGraceTimer > 0)
        {
            player.shieldActive = true;
            player.shieldDashGraceTimer -= deltaTime;
            if (player.shieldDashGraceTimer <= 0)
            {
                player.shieldDashGraceTimer = 0;
                player.shieldActive = false;
                player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
            }
        }
        else if (player.shieldActive)
        {
            player.shieldTimer -= deltaTime;
            if (player.shieldTimer <= 0)
            {
                const bool stillHeld = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
                if (stillHeld && (infiniteCharges || player.charges > 0 || player.overdriveTimer > 0))
                {
                    if (!infiniteCharges && player.overdriveTimer <= 0)
                    {
                        player.charges--;
                    }
                    player.shieldTimer = chunk;
                }
                else
                {
                    player.shieldActive = false;
                    player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
                }
            }
        }
    }

    player.shieldDashing = player.dashing && player.shieldActive;

    updateNerveBurstInput(game);
}

auto aimAtMouse(const Game& game) -> Vector2
{
    const Vector2 mouse = GetMousePosition();
    const Rectangle box = letterBoxRect(game);
    const Vector2 center{.x = box.x + box.width / 2, .y = box.y + box.height / 2};
    const Vector2 dir = Vector2Subtract(mouse, center);
    if (Vector2Length(dir) == 0)
    {
        return Vector2{.x = 0, .y = -1};
    }
    return Vector2Normalize(dir);
}

auto mouseWorldPos(const Game& game) -> Vector2
{
    const Vector2 mouse = GetMousePosition();
    const Rectangle box = letterBoxRect(game);
    const Vector2 center{.x = box.x + box.width / 2, .y = box.y + box.height / 2};
    const float scale = game.resources.renderScale;
    if (scale <= 0)
    {
        return game.run.player.position;
    }
    return Vector2Add(game.run.player.position,
                      Vector2Scale(Vector2Subtract(mouse, center), 1.0F / scale));
}

void updatePlayerBuffs(Game& game, float deltaTime)
{
    auto& player = game.run.player;

    for (auto& timer : player.elementalBuffTimer)
    {
        if (timer > 0)
        {
            timer = std::max(0.0F, timer - deltaTime);
        }
    }

    if (player.regenTimer > 0)
    {
        player.regenTimer = std::max(0.0F, player.regenTimer - deltaTime);
        player.health = std::min(player.maxHealth, player.health + player.regenRate * deltaTime);
    }

    if (player.burnRefreshTimer > 0)
    {
        player.burnRefreshTimer -= deltaTime;
        if (player.burnRefreshTimer <= 0)
        {
            player.burnDps = 0;
        }
    }
    if (player.burnDps > 0 && player.health > 0)
    {
        player.burnDamageAccum += player.burnDps * deltaTime;
        if (player.burnDamageAccum >= 1.0F)
        {
            const auto tick = static_cast<int32_t>(player.burnDamageAccum);
            damagePlayer(game, tick);
            player.burnDamageAccum -= static_cast<float>(tick);
        }
    }

    if (player.overchargeTimer > 0)
    {
        player.overchargeTimer = std::max(0.0F, player.overchargeTimer - deltaTime);
    }

    if (player.overdriveTimer > 0)
    {
        player.overdriveTimer = std::max(0.0F, player.overdriveTimer - deltaTime);
    }

    if (player.dashTrailTimer > 0)
    {
        player.dashTrailTimer = std::max(0.0F, player.dashTrailTimer - deltaTime);
    }

    if (player.dashing && player.dashTrailTimer > 0)
    {
        const float jitterAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const float jitterSpeed = static_cast<float>(GetRandomValue(2, 8)) / 10.0F;
        game.run.dashTrailParticles.push_back(
            Particle{.position = player.position,
                     .velocity = Vector2{.x = std::cos(jitterAngle) * jitterSpeed,
                                         .y = std::sin(jitterAngle) * jitterSpeed},
                     .radius = dashTrailRadius,
                     .life = dashTrailParticleLife,
                     .maxLife = dashTrailParticleLife,
                     .color = Palette::Accent});

        for (size_t i = 0; i < game.run.enemies.size(); i++)
        {
            auto& enemy = game.run.enemies.at(i);
            if (enemy.active && !enemy.phased &&
                CheckCollisionCircles(player.position, dashTrailRadius, enemy.position,
                                      enemyCollisionRadius(enemy)))
            {
                damageEnemy(game, i, dashTrailDamage);
                recordDamage(game, DamageSource::Dash, dashTrailDamage);
            }
        }
    }

    for (auto& particle : game.run.dashTrailParticles)
    {
        particle.position = Vector2Add(particle.position, particle.velocity);
        particle.life -= deltaTime;
    }
    std::erase_if(game.run.dashTrailParticles, [](const Particle& p) { return p.life <= 0; });
}

void collectLifeOrb(Game& game)
{
    game.run.player.health = std::min(game.run.player.health + 0.5F, game.run.player.maxHealth);
}
