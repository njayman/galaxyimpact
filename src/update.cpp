#include "update_internal.hpp"

auto damageShakeIntensity(int32_t amount) -> float
{
    constexpr float perDamageUnit = 0.4F;
    constexpr float maxIntensity = 9.0F;
    return std::min(maxIntensity, static_cast<float>(amount) * perDamageUnit);
}
void triggerShake(Game& /*game*/, float /*intensity*/, float /*duration*/)
{
    // ponytail: screen shake is disabled everywhere except triggerBossShieldDashShake and
    // triggerPlayerHitShake below, per explicit request that shake should only fire on a
    // shield-dash into a boss or on the player taking damage.
}

void triggerPlayerHitShake(Game& game, float intensity, float duration)
{
    game.run.shakeIntensity = intensity;
    game.run.shakeTimer = duration;
    game.run.shakeDuration = duration;
}

void triggerBossShieldDashShake(Game& game)
{
    if (game.run.shieldDashShakeCooldownTimer > 0)
    {
        return;
    }
    game.run.shieldDashShakeCooldownTimer = shieldDashBossShakeCooldown;

    game.run.shakeIntensity = 14.0F;
    game.run.shakeTimer = 0.4F;
    game.run.shakeDuration = 0.4F;
    game.run.hitPauseTimer = 0.12F;
    playSFX(game, game.resources.sounds.critical);
}

void spawnDamageNumber(Game& game, Vector2 position, int32_t amount)
{
    constexpr float damageNumberLife = 0.6F;
    constexpr float jitterRange = 10.0F;
    const Vector2 jittered{
        .x = position.x + (static_cast<float>(GetRandomValue(-100, 100)) / 100.0F) * jitterRange,
        .y = position.y};
    game.run.damageNumbers.push_back(DamageNumber{.position = jittered,
                                                  .amount = amount,
                                                  .timer = damageNumberLife,
                                                  .maxTimer = damageNumberLife});
}

void updateDamageNumbers(Game& game, float deltaTime)
{
    constexpr float riseSpeed = 45.0F;
    for (auto& number : game.run.damageNumbers)
    {
        number.position.y -= riseSpeed * deltaTime;
        number.timer -= deltaTime;
    }
    std::erase_if(game.run.damageNumbers, [](const DamageNumber& n) { return n.timer <= 0; });
}

void updateWeaponDowngrade(Game& game, float deltaTime)
{
    if (!game.run.weaponDowngrade.has_value())
    {
        return;
    }

    auto& downgrade = *game.run.weaponDowngrade;
    downgrade.timer -= deltaTime;
    if (downgrade.timer > 0)
    {
        return;
    }

    for (auto& weapon : game.run.weapons)
    {
        if (weapon.type == downgrade.type)
        {
            weapon.disabled = false;
            game.run.achievementToast = std::string(effectiveWeaponName(weapon)) + " restored!";
            game.run.achievementToastTimer = 3.0F;
            break;
        }
    }
    game.run.weaponDowngrade = std::nullopt;
}

void triggerHitPause(Game& /*game*/, float /*duration*/)
{
    // ponytail: see triggerShake above — hit-pause is likewise reserved for
    // triggerBossShieldDashShake only.
}

void duckBGM(Game& game) { game.resources.bgm.duckTimer = bgmDuckDuration; }

void spawnKillExplosion(Game& game, Vector2 position, Color color, int32_t particleCount,
                        float speedScale)
{
    for (int i = 0; i < particleCount; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const float speed = static_cast<float>(GetRandomValue(15, 60)) / 10.0F * speedScale;
        const Vector2 velocity{.x = std::cos(angle) * speed, .y = std::sin(angle) * speed};
        const float life = static_cast<float>(GetRandomValue(30, 60)) / 100.0F;

        game.run.deathParticles.push_back(
            Particle{.position = position,
                     .velocity = velocity,
                     .radius = static_cast<float>(GetRandomValue(2, 4)) * speedScale,
                     .life = life,
                     .maxLife = life,
                     .color = color});
    }
}

auto updateBanishedEntryCutscene(Game& game, float deltaTime) -> bool
{
    game.run.bossCutsceneTimer -= deltaTime;

    if (game.run.bossCutscenePhase == 10)
    {
        const Vector2 toCenter =
            Vector2Subtract(game.run.blackhole.position, game.run.player.position);
        if (Vector2Length(toCenter) > 1.0F)
        {
            game.run.player.position = Vector2Add(
                game.run.player.position, Vector2Scale(toCenter, std::min(1.0F, 3.0F * deltaTime)));
        }

        if (game.run.bossCutsceneTimer <= 0)
        {
            game.run.inBanishedRealm = true;
            game.run.punctumTrapActive = false;
            game.run.blackhole.active = false;
            game.run.enemies.clear();
            game.run.eliteHazards.clear();
            game.run.asteroids.clear();
            game.run.bossProjectiles.clear();
            game.run.gasHazards.clear();
            game.run.mines.clear();
            game.run.bgParticles.clear();
            game.run.stars.clear();
            game.run.player.position = Vector2{};
            game.run.achievementToast = "I am the banished one. I am your doom.";
            game.run.achievementToastTimer = banishedRemainsDuration + banishedBreakApartDuration;
            game.run.bossCutscenePhase = 11;
            game.run.bossCutsceneTimer = banishedRemainsDuration;
        }
        return true;
    }

    if (game.run.bossCutscenePhase == 11)
    {
        if (game.run.bossCutsceneTimer <= 0)
        {
            game.run.bossCutscenePhase = 12;
            game.run.bossCutsceneTimer = banishedBreakApartDuration;
        }
        return true;
    }

    if (game.run.bossCutscenePhase == 12)
    {
        if (game.run.bossCutsceneTimer <= 0)
        {
            game.run.bossCutscenePhase = 0;
            spawnBanished(game);
        }
        return true;
    }

    game.run.bossCutscenePhase = 0;
    return false;
}

auto updateBossCutscene(Game& game, float deltaTime) -> bool
{
    if (game.run.bossCutscenePhase == 0)
    {
        return false;
    }

    if (game.run.bossCutscenePhase >= 10)
    {
        return updateBanishedEntryCutscene(game, deltaTime);
    }

    Boss* kraken = nullptr;
    for (auto& candidate : game.run.bosses)
    {
        if (candidate.isKraken && candidate.krakenEncounter >= 3)
        {
            kraken = &candidate;
            break;
        }
    }
    if (kraken == nullptr)
    {
        game.run.bossCutscenePhase = 0;
        return false;
    }

    game.run.bossCutsceneTimer -= deltaTime;

    if (game.run.bossCutscenePhase <= 3)
    {
        kraken->deathAnimTimer -= deltaTime;
        if (kraken->deathAnimTimer <= 0)
        {
            kraken->deathAnimTimer = bossDeathAnimDuration;
        }
    }

    const Vector2 bossCenter{.x = kraken->position.x + kraken->size.x / 2,
                             .y = kraken->position.y + kraken->size.y / 2};

    if (game.run.bossCutscenePhase == 1)
    {
        const Vector2 toPlayer = Vector2Subtract(game.run.player.position, bossCenter);
        const float dist = Vector2Length(toPlayer);
        if (dist > krakenOutroApproachStopDistance)
        {
            kraken->position = Vector2Add(
                kraken->position,
                Vector2Scale(Vector2Normalize(toPlayer), krakenOutroApproachSpeed * deltaTime));
        }
        if (game.run.bossCutsceneTimer <= 0 || dist <= krakenOutroApproachStopDistance)
        {
            game.run.bossCutscenePhase = 2;
            game.run.bossCutsceneTimer = krakenOutroSummonDuration;
            const Vector2 sideDir =
                Vector2Normalize(Vector2Subtract(bossCenter, game.run.player.position));
            game.run.krakenFleePos =
                Vector2Add(game.run.player.position, Vector2Scale(sideDir, krakenOutroSideOffset));
        }
        return true;
    }

    if (game.run.bossCutscenePhase == 2)
    {
        const Vector2 toSpot = Vector2Subtract(game.run.krakenFleePos, bossCenter);
        if (Vector2Length(toSpot) > 4.0F)
        {
            kraken->position = Vector2Add(
                kraken->position,
                Vector2Scale(Vector2Normalize(toSpot), krakenOutroApproachSpeed * deltaTime));
        }
        if (game.run.bossCutsceneTimer <= 0)
        {
            game.run.bossCutscenePhase = 3;
            game.run.bossCutsceneTimer = krakenOutroHoldDuration;
            // The trap wraps the ship in a strip round the origin, so recentre the scene on the portal.
            const Vector2 shift = Vector2Negate(game.run.krakenFleePos);
            game.run.player.position = Vector2Add(game.run.player.position, shift);
            kraken->position = Vector2Add(kraken->position, shift);
            game.run.krakenFleePos = Vector2{};
            game.run.blackhole.position = Vector2{};
            game.run.blackhole.radius = 60;
            game.run.blackhole.influenceRadius = 260;
            game.run.blackhole.active = true;
            game.run.blackhole.isKrakenPortal = true;
            for (auto& dust : game.run.blackHoleDust)
            {
                dust.color = spawnBlackHoleDustParticle(game).color;
            }
            game.run.punctumTrapActive = true;
            game.run.achievementToast = "Follow me, if you dare.";
            game.run.achievementToastTimer = krakenOutroHoldDuration;
            game.run.enemies.clear();
            game.run.eliteHazards.clear();
            game.run.asteroids.clear();
            game.run.bossProjectiles.clear();
        }
        return true;
    }

    if (game.run.bossCutscenePhase == 3)
    {
        if (game.run.bossCutsceneTimer <= 0)
        {
            game.run.bossCutscenePhase = 4;
            game.run.bossCutsceneTimer = krakenOutroTwirlDuration;
        }
        return true;
    }

    if (game.run.bossCutscenePhase == 4)
    {
        const Vector2 toCenter = Vector2Subtract(game.run.blackhole.position, bossCenter);
        if (Vector2Length(toCenter) > 2.0F)
        {
            kraken->position = Vector2Add(
                kraken->position,
                Vector2Scale(Vector2Normalize(toCenter),
                            krakenOutroApproachSpeed * krakenOutroTwirlSpeedMult * deltaTime));
        }

        if (game.run.bossCutsceneTimer <= 0)
        {
            const Vector2 finalCenter{.x = kraken->position.x + kraken->size.x / 2,
                                      .y = kraken->position.y + kraken->size.y / 2};
            game.run.score += 1000;
            recordBossKilled(game, *kraken);
            game.run.xp += static_cast<int>(static_cast<float>(game.run.xpToNext) * megaBossXpMult);
            spawnRareBonusPickup(game, finalCenter);
            spawnRareDangerPickup(game, finalCenter);
            std::erase_if(game.run.bosses, [](const Boss& b) { return b.isKraken; });
            game.run.bossCutscenePhase = 0;
        }
        return true;
    }

    return true;
}

// Rustbloom's near bloom pods (parallax 0.8) wobble when the ship moves through them, then ring
// down. Same pod placement as drawRustbloomBackground.
void updatePodJiggle(Game& game, float deltaTime)
{
    auto& jiggle = game.run.podJiggle;
    std::erase_if(jiggle, [&](auto& entry) { return (entry.second *= std::exp(-2.2F * deltaTime)) < 0.01F; });
    if (currentBiome(game.run.waveNumber) != Biome::Rustbloom)
    {
        jiggle.clear();
        game.run.tendrilPull.clear();
        return;
    }
    constexpr float cell = rustbloomPodCellSize;
    const Player& player = game.run.player;
    const Vector2 shift = Vector2Scale(player.position, 0.2F);
    const Vector2 layer = Vector2Subtract(player.position, shift);
    const float moving = std::clamp(Vector2Length(player.velocity) / std::max(player.speed, 1.0F), 0.0F, 1.0F);
    const bool stirring = moving >= 0.05F || player.dashing; // resting on a pod doesn't stir it
    for (int32_t dy = -1; dy <= 1 && stirring; dy++)
    {
        for (int32_t dx = -1; dx <= 1; dx++)
        {
            const float x = (std::floor(layer.x / cell) + static_cast<float>(dx)) * cell;
            const float y = (std::floor(layer.y / cell) + static_cast<float>(dy)) * cell;
            if (hashNoise(x * 0.021F + y * 0.037F) > rustbloomPodChance)
            {
                continue;
            }
            const float jx = (hashNoise(x * 0.05F + y * 0.02F + 3.0F) - 0.5F) * cell * 0.5F;
            const float jy = (hashNoise(x * 0.02F + y * 0.05F + 7.0F) - 0.5F) * cell * 0.5F;
            const Vector2 center{.x = x + cell / 2 + jx + shift.x, .y = y + cell / 2 + jy + shift.y};
            const float radius = cell * (0.14F + hashNoise(x * 0.043F + y * 0.017F + 9.0F) * 0.06F);
            if (Vector2Distance(center, player.position) < radius + player.radius)
            {
                float& energy = jiggle[rustbloomPodKey(x, y, cell)];
                energy = std::max(energy, player.dashing ? 1.0F : 0.4F + 0.6F * moving);
            }

            // Tendrils to the right and down neighbours (same ends as drawRustbloomBackground):
            // brushing one hooks it onto the ship.
            for (int32_t dir = 0; dir < 2; dir++)
            {
                const float nx = dir == 0 ? x + cell : x;
                const float ny = dir == 0 ? y : y + cell;
                if (hashNoise(nx * 0.021F + ny * 0.037F) > rustbloomPodChance)
                {
                    continue;
                }
                const Vector2 end{.x = nx + cell / 2 + shift.x, .y = ny + cell / 2 + shift.y};
                const Vector2 span = Vector2Subtract(end, center);
                const float along = std::clamp(Vector2DotProduct(Vector2Subtract(player.position, center), span) /
                                                   Vector2LengthSqr(span),
                                               0.1F, 0.9F);
                const Vector2 onLine = Vector2Add(center, Vector2Scale(span, along));
                const int64_t key = rustbloomPodKey(x, y, cell) * 2 + dir;
                auto found = game.run.tendrilPull.find(key);
                if (Vector2Distance(onLine, player.position) < player.radius && (found == game.run.tendrilPull.end() || !found->second.hooked))
                {
                    TendrilPull& pull = game.run.tendrilPull[key];
                    pull.hooked = true;
                    pull.at = along;
                    pull.cellX = x;
                    pull.cellY = y;
                }
            }
        }
    }

    // Hooked tendrils follow the ship until stretched too far, then snap back on a spring, ringing
    // their pods as they go.
    constexpr float maxStretch = 55.0F;
    constexpr float stiffness = 170.0F;
    constexpr float damping = 5.0F;
    std::erase_if(game.run.tendrilPull,
                  [&](auto& entry)
                  {
                      TendrilPull& pull = entry.second;
                      const int64_t podKey = entry.first / 2;
                      if (pull.hooked)
                      {
                          // Where the hooked point would rest, from its pod's key.
                          const float cx = pull.cellX;
                          const float cy = pull.cellY;
                          const bool right = entry.first % 2 == 0;
                          const float jx = (hashNoise(cx * 0.05F + cy * 0.02F + 3.0F) - 0.5F) * cell * 0.5F;
                          const float jy = (hashNoise(cx * 0.02F + cy * 0.05F + 7.0F) - 0.5F) * cell * 0.5F;
                          const Vector2 a{.x = cx + cell / 2 + jx + shift.x, .y = cy + cell / 2 + jy + shift.y};
                          const Vector2 b{.x = cx + (right ? cell * 1.5F : cell / 2) + shift.x,
                                          .y = cy + (right ? cell / 2 : cell * 1.5F) + shift.y};
                          const Vector2 rest = Vector2Lerp(a, b, pull.at);
                          const Vector2 target = Vector2Subtract(player.position, rest);
                          if (Vector2Length(target) > maxStretch)
                          {
                              pull.hooked = false; // snaps: the spring takes it from here
                              jiggle[podKey] = std::max(jiggle[podKey], 0.7F);
                          }
                          else
                          {
                              pull.vel = Vector2Scale(Vector2Subtract(target, pull.offset), 1.0F / std::max(deltaTime, 0.001F));
                              pull.offset = target;
                              return false;
                          }
                      }
                      const Vector2 accel = Vector2Subtract(Vector2Scale(pull.offset, -stiffness), Vector2Scale(pull.vel, damping));
                      pull.vel = Vector2Add(pull.vel, Vector2Scale(accel, deltaTime));
                      pull.offset = Vector2Add(pull.offset, Vector2Scale(pull.vel, deltaTime));
                      return Vector2Length(pull.offset) < 0.3F && Vector2Length(pull.vel) < 2.0F;
                  });
}

// A radial burst's shock ring: races out from where the shots left, fading.
void spawnBlastRing(Game& game, Vector2 at, float reach)
{
    constexpr float life = 0.55F;
    game.run.blastRings.push_back(
        Particle{.position = at, .velocity = {}, .radius = reach, .life = life, .maxLife = life, .color = Palette::Accent});
}

// Minimal HUD bookkeeping: calm timer, hit flash, biome name, boss damage trails.
void updateHud(Game& game, float deltaTime)
{
    HudState& hud = game.run.hud;
    const Player& player = game.run.player;
    const bool nerveReady = nerveFrac(game) >= 1.0F;
    hud.calmTimer += deltaTime;
    hud.hitFlashTimer = std::max(0.0F, hud.hitFlashTimer - deltaTime);
    hud.biomeTimer = std::max(0.0F, hud.biomeTimer - deltaTime);
    if (hud.lastHealth >= 0 && player.health < hud.lastHealth)
    {
        hud.hitFrom = hud.hitFlashTimer > 0 ? std::max(hud.hitFrom, hud.lastHealth) : hud.lastHealth;
        hud.hitFlashTimer = 0.6F;
    }
    if (player.health != hud.lastHealth || player.charges != hud.lastCharges || nerveReady != hud.lastNerveReady ||
        player.nerveCharging)
    {
        hud.calmTimer = 0;
    }
    hud.lastHealth = player.health;
    hud.lastCharges = player.charges;
    hud.lastNerveReady = nerveReady;

    const auto biome = static_cast<int32_t>(currentBiome(game.run.waveNumber));
    if (biome != hud.biome)
    {
        hud.biome = biome;
        hud.biomeTimer = 4.0F;
    }

    for (auto& boss : game.run.bosses)
    {
        const auto health = static_cast<float>(boss.health);
        boss.trailHealth = boss.trailHealth < health ? health
                                                     : std::max(health, boss.trailHealth - static_cast<float>(boss.maxHealth) * 0.3F * deltaTime);
    }
}

void updateGameplay(Game& game, float deltaTime)
{

    game.run.damageMeter = DamageMeter{};

    if (IsKeyPressed(KEY_ESCAPE) || pausePressed(game))
    {
        game.state = GameState::PAUSED;
        game.menuIndex = 0;
        return;
    }

    updateBossQte(game, deltaTime);
    deltaTime *= bossQteSlowdown(game);

    game.run.runTime += deltaTime;
    updateHud(game, deltaTime);
    for (auto& ring : game.run.blastRings)
    {
        ring.life -= deltaTime;
    }
    std::erase_if(game.run.blastRings, [](const Particle& p) { return p.life <= 0; });
    updatePunctumInfall(game, deltaTime);
    updatePodJiggle(game, deltaTime);
    updateNerveLaserStorm(game, deltaTime);

    if (game.sandbox)
    {
        updateSandboxInput(game);
    }

    if (game.sandboxKillBoxArmed)
    {
        game.sandboxKillBoxTimer -= deltaTime;
        if (game.sandboxKillBoxTimer <= 0)
        {
            game.sandboxKillBoxArmed = false;
            for (auto& boss : game.run.bosses)
            {
                if (!boss.isBeltbreakerPlate && !boss.isWreckwormSegment)
                {
                    boss.health = 0;
                }
            }
        }
    }

    const bool cutsceneLocked = updateBossCutscene(game, deltaTime);

    updateNerve(game, deltaTime);
    if (!cutsceneLocked)
    {
        updateAbilityCharges(game, deltaTime);
        updatePlayerMovement(game, deltaTime);
    }
    updateShieldAndBarrier(game, deltaTime);
    if (!cutsceneLocked)
    {
        updateWeapons(game, deltaTime);
    }
    updateOrbitBladeContact(game, deltaTime);
    updateOrbitBladeLaunch(game, deltaTime);
    updateOrbitBladeProjectiles(game, deltaTime);
    updateNerveBallProjectiles(game, deltaTime);
    updateNerveSpiralProjectiles(game, deltaTime);
    updateBeamContact(game, deltaTime);
    updateFollowerDrones(game, deltaTime);
    updateLaserDrones(game, deltaTime);
    updateDroneContact(game, deltaTime);
    updateTurrets(game, deltaTime);
    updateFlamethrower(game, deltaTime);
    updateSolarForgeEnemyFireParticles(game, deltaTime);
    updateChainLightningBolts(game, deltaTime);
    updateSniperShots(game, deltaTime);
    updatePlayerBuffs(game, deltaTime);
    updateWaveSpawner(game, deltaTime);
    updateBlackHole(game, deltaTime);
    updateBlackHoleDust(game, deltaTime);
    updateWormhole(game, deltaTime);
    updateEliteHazards(game, deltaTime);
    updateBeltbreakerRubble(game, deltaTime);
    updateMagmaDensity(game, deltaTime);

    if (game.run.player.health > 0 && game.run.blackhole.active &&
        Vector2Distance(game.run.player.position, game.run.blackhole.position) <=
            game.run.blackhole.radius)
    {
        if (game.run.punctumTrapActive && !game.run.inBanishedRealm)
        {
            if (game.run.bossCutscenePhase == 0)
            {
                game.run.bossCutscenePhase = 10;
                game.run.bossCutsceneTimer = banishedEntryTwirlDuration;
            }
        }
        else
        {
            game.run.player.blackHoleCoreTimer += deltaTime;
            if (game.run.player.blackHoleCoreTimer >= 1.0F)
            {
                game.run.player.blackHoleCoreTimer = 0;
                damagePlayer(game, instakillDamage);
            }
        }
    }
    else
    {
        game.run.player.blackHoleCoreTimer = 0;
    }

    for (auto& boss : game.run.bosses)
    {
        if (boss.retreating)
        {
            const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                     .y = boss.position.y + boss.size.y / 2};
            const Vector2 away = Vector2Subtract(bossCenter, game.run.player.position);
            const float dist = Vector2Length(away);
            const Vector2 dir =
                dist > 1.0F ? Vector2Scale(away, 1.0F / dist) : Vector2{.x = 1, .y = 0};
            boss.position =
                Vector2Add(boss.position, Vector2Scale(dir, krakenRetreatSpeed * deltaTime));
            continue;
        }

        if (boss.deathAnimTimer >= 0)
        {
            continue;
        }

        if (boss.isBanished)
        {
            updateBanished(game, boss, deltaTime);
            continue;
        }

        if (boss.health > 0 && boss.burnDps > 0)
        {
            boss.burnDamageAccum += boss.burnDps * deltaTime;
            if (boss.burnDamageAccum >= 1.0F)
            {
                const auto tick = static_cast<int32_t>(boss.burnDamageAccum);
                damageBoss(game, boss, tick, false, true);
                recordDamage(game, DamageSource::Elemental, tick);
                boss.burnDamageAccum -= static_cast<float>(tick);
            }
        }

        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};

        if (boss.parryStunTimer > 0)
        {
            boss.parryStunTimer -= deltaTime;
        }
        updateBossMovement(game, deltaTime, boss, bossCenter);
        updateBoss(game, deltaTime, boss, bossCenter);
        updateBeltbreakerCore(game, boss, deltaTime);
        updateWreckwormChain(game, boss, deltaTime);
        updateWreckwormSegmentVolley(game, boss, deltaTime);
        updateKrakenTentacle(game, boss, deltaTime);
        updateKrakenLimbs(game, boss, deltaTime);
        updateKrakenSummon(game, boss, deltaTime);

        if (boss.isWreckwormHead && deltaTime > 0)
        {
            boss.wreckwormVelocity =
                Vector2Scale(Vector2Subtract(boss.position, boss.wreckwormPrevPosition), 1.0F / deltaTime);
            boss.wreckwormPrevPosition = boss.position;
        }
    }

    updateBullets(game, deltaTime);
    updateAsteroids(game, deltaTime);
    updateSolarForgeHeat(game, deltaTime);
    updateRicochetRelay(game, deltaTime);
    updateMagmaPools(game, deltaTime);
    updatePunctumThunder(game, deltaTime);
    updateDebrisSquall(game, deltaTime);
    updateRustPatches(game, deltaTime);
    updateFluidHazardContact(game);
    updateTacticTracker(game, deltaTime);
    updateGasHazards(game, deltaTime);
    updateOrganicMerges(game, deltaTime);
    updateEnemies(game, deltaTime);
    updateProjectiles(game, deltaTime);
    updateMines(game, deltaTime);
    updatePickups(game, deltaTime);

    filterDeadEntities(game);

    bool anyBossKilledThisFrame = false;
    std::vector<Boss> pendingSlagmawHalves;
    for (auto& boss : game.run.bosses)
    {
        if (boss.health > 0)
        {
            continue;
        }

        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};

        if (boss.isKraken && boss.krakenEncounter < 3)
        {
            boss.retreating = true;
            boss.health = 1;
            continue;
        }

        if (boss.isKraken && !game.run.inBanishedRealm)
        {
            if (game.run.bossCutscenePhase == 0)
            {
                boss.health = 1;
                boss.deathAnimTimer = bossDeathAnimDuration;
                boss.color = Palette::StructDark;
                game.run.bossCutscenePhase = 1;
                game.run.bossCutsceneTimer = krakenOutroApproachDuration;
            }
            continue;
        }

        if (boss.isBeltbreakerPlate || boss.isWreckwormSegment)
        {
            boss.health = 0;
            game.run.score += 100;
            spawnKillExplosion(game, bossCenter, boss.baseColor, 14, 1.2F);
            playSFX(game, game.resources.sounds.explosion);
            if (boss.isWreckwormSegment)
            {
                bloomRustPatch(game, bossCenter, true);
            }
            continue;
        }

        boss.health = 0;

        if (boss.deathAnimTimer < 0)
        {
            boss.deathAnimTimer = bossDeathAnimDuration;
            boss.color = Palette::StructDark;

            if (boss.isWreckwormHead)
            {
                for (auto& other : game.run.bosses)
                {
                    if (other.isWreckwormSegment && other.segmentOwnerId == boss.instanceId &&
                        other.health > 0)
                    {
                        other.health = 0;
                    }
                }
            }
        }

        boss.deathAnimTimer -= deltaTime;
        if (boss.deathAnimTimer > 0)
        {
            continue;
        }

        game.run.score += 1000;
        anyBossKilledThisFrame = true;
        recordBossKilled(game, boss);

        if (!boss.slagmawBreaksIntoDrifters && !boss.slagmawBreaksIntoHalves)
        {
            game.run.xp += static_cast<int>(static_cast<float>(game.run.xpToNext) *
                                            (boss.isMega ? megaBossXpMult : miniBossXpMult));
        }
        if (boss.isSwarm)
        {
            spawnPickup(game, bossCenter, 0, PickupType::Shield);
            spawnPickup(game, bossCenter, 0, PickupType::LifeOrb);
        }
        spawnRareBonusPickup(game, bossCenter);
        spawnRareDangerPickup(game, bossCenter);

        game.run.bossDeathShockwaves.push_back(
            BossDeathShockwave{.timer = UpdateConstants::bossDeathShockwaveDuration,
                               .position = bossCenter,
                               .hit = false});
        spawnKillExplosion(game, bossCenter, boss.baseColor, 40, 2.2F);

        if (game.run.player.health > 0)
        {
            playSFX(game, game.resources.sounds.victory);
            triggerShake(game, 14, 0.6F);
            triggerHitPause(game, 0.2F);
            gainNerve(game);
        }

        if (boss.isFinalBoss && !boss.isKraken && !game.resources.achievements.infiniteModeUnlocked)
        {
            game.resources.achievements.infiniteModeUnlocked = true;
            saveAchievements(game.resources.achievements);
            game.run.achievementToast = "Infinite mode unlocked!";
            game.run.achievementToastTimer = 4.0F;
        }

        if (boss.slagmawBreaksIntoDrifters)
        {
            spawnSlagmawDrifterBreak(game, bossCenter, boss.slagmawBreakGeneration + 1);
        }
        else if (boss.slagmawBreaksIntoHalves)
        {
            spawnSlagmawHalfBreak(game, boss, bossCenter, pendingSlagmawHalves);
        }
    }
    for (auto& half : pendingSlagmawHalves)
    {
        game.run.bosses.push_back(std::move(half));
    }
    if (anyBossKilledThisFrame)
    {
        game.resources.bgm.calmTimer = bossKillCalmDuration;
    }
    const size_t bossCountBeforeErase = game.run.bosses.size();
    std::erase_if(game.run.bosses,
                  [&game](const Boss& boss)
                  {
                      if (boss.retreating)
                      {
                          const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                                   .y = boss.position.y + boss.size.y / 2};
                          return Vector2Distance(bossCenter, game.run.player.position) >
                                 krakenRetreatDespawnDistance;
                      }
                      return boss.health <= 0 && boss.deathAnimTimer <= 0;
                  });

    if (game.run.bosses.size() < bossCountBeforeErase && game.run.bosses.empty())
    {
        game.run.waveTimer = 0;
    }

    updateBossDeathShockwave(game, deltaTime);

    if (game.run.player.health > 0 && game.run.xp >= game.run.xpToNext)
    {
        startLevelUp(game);
    }

    if (game.run.damageMeter.total > 0)
    {
        game.run.damageMeterDisplay = game.run.damageMeter;
        game.run.damageMeterHoldTimer = damageMeterHoldDuration;
    }
    else if (game.run.damageMeterHoldTimer > 0)
    {
        game.run.damageMeterHoldTimer -= deltaTime;
        if (game.run.damageMeterHoldTimer <= 0)
        {
            game.run.damageMeterDisplay = DamageMeter{};
        }
    }
}

void filterDeadEntities(Game& game)
{
    std::erase_if(game.run.asteroids, [](const Asteroid& a) { return !a.active; });
    std::erase_if(game.run.bullets, [](const Bullet& b) { return !b.active; });
    std::erase_if(game.run.bossProjectiles, [](const BossProjectile& p) { return !p.active; });
    std::erase_if(game.run.enemies, [](const Enemy& e) { return !e.active; });
    std::erase_if(game.run.eliteHazards, [](const EliteHazard& h) { return !h.active; });
    std::erase_if(game.run.pickups, [](const Pickup& p) { return !p.active; });
    std::erase_if(game.run.mines, [](const Mine& m) { return !m.active; });
}

void updateBgParticles(Game& game)
{
    const auto tileW = static_cast<float>(game.resources.screenWidth);
    const auto tileH = static_cast<float>(game.resources.screenHeight);

    const bool inPunctum = currentBiome(game.run.waveNumber) == Biome::Punctum;
    const bool spiraling = inPunctum && game.run.punctumTrapActive;
    // A particle's stored .position is a LOCAL tile coordinate in [0,tileW)x[0,tileH);
    // its actual on-screen position each frame comes from tiledWorldPos(player.position, pos, ...),
    // which wraps (pos - player.position) into the nearest copy of the tile. To pull particles
    // toward a specific WORLD point (the black hole), we need to compare/adjust that same wrapped
    // delta-from-player, not the raw local coordinate directly.
    const auto localDeltaFromPlayer = [&](Vector2 pos) -> Vector2
    {
        float dx = std::fmod(pos.x - game.run.player.position.x, tileW);
        if (dx > tileW / 2)
        {
            dx -= tileW;
        }
        else if (dx < -tileW / 2)
        {
            dx += tileW;
        }
        float dy = std::fmod(pos.y - game.run.player.position.y, tileH);
        if (dy > tileH / 2)
        {
            dy -= tileH;
        }
        else if (dy < -tileH / 2)
        {
            dy += tileH;
        }
        return Vector2{.x = dx, .y = dy};
    };

    const Vector2 desiredDelta = spiraling && game.run.blackhole.active
                                    ? Vector2Subtract(game.run.blackhole.position,
                                                      game.run.player.position)
                                    : Vector2{};

    // Punctum: everything streams toward the map centre, from wherever it is on the map.
    const auto worldOf = [&](Vector2 pos) { return Vector2Add(game.run.player.position, localDeltaFromPlayer(pos)); };
    const auto driftFor = [&](Vector2 pos) -> Vector2
    {
        if (!spiraling)
        {
            if (!inPunctum)
            {
                return Vector2{};
            }
            const Vector2 world = worldOf(pos);
            const float d = std::max(Vector2Length(world), 1.0F);
            return Vector2Scale(world, -punctumPullSpeed / d);
        }
        const Vector2 toCenter = Vector2Subtract(desiredDelta, localDeltaFromPlayer(pos));
        if (Vector2Length(toCenter) < 1.0F)
        {
            return Vector2{};
        }
        const Vector2 radial = Vector2Normalize(toCenter);
        const Vector2 tangent{.x = -radial.y, .y = radial.x};
        return Vector2Add(Vector2Scale(radial, punctumTrapSpiralInwardSpeed),
                          Vector2Scale(tangent, punctumTrapSpiralSwirlSpeed));
    };

    const auto respawnAtEdge = [&]() -> Vector2
    {
        return Vector2{.x = static_cast<float>(GetRandomValue(0, static_cast<int32_t>(tileW))),
                       .y = static_cast<float>(GetRandomValue(0, static_cast<int32_t>(tileH)))};
    };

    // Swallowed at the centre, or (rarely) reborn anywhere, so the flow never bunches into a line.
    const float rebirthChance = GetFrameTime() / 5.0F;
    const auto swallowed = [&](Vector2 pos)
    {
        return inPunctum && !spiraling &&
               (Vector2Length(worldOf(pos)) < punctumTrapConsumeRadius ||
                static_cast<float>(GetRandomValue(0, 99999)) < rebirthChance * 100000.0F);
    };

    for (auto& p : game.run.bgParticles)
    {
        if (swallowed(p.position))
        {
            p.position = respawnAtEdge();
            continue;
        }
        if (spiraling &&
            Vector2Length(Vector2Subtract(desiredDelta, localDeltaFromPlayer(p.position))) <=
                punctumTrapConsumeRadius)
        {
            p.position = respawnAtEdge();
            continue;
        }

        p.position = Vector2Add(p.position, Vector2Add(p.velocity, driftFor(p.position)));

        if (p.position.x < 0)
        {
            p.position.x += tileW;
        }
        if (p.position.x > tileW)
        {
            p.position.x -= tileW;
        }
        if (p.position.y < 0)
        {
            p.position.y += tileH;
        }
        if (p.position.y > tileH)
        {
            p.position.y -= tileH;
        }
    }

    if (inPunctum)
    {
        for (auto& s : game.run.stars)
        {
            if (swallowed(s.position))
            {
                s.position = respawnAtEdge();
                continue;
            }
            if (spiraling &&
                Vector2Length(Vector2Subtract(desiredDelta, localDeltaFromPlayer(s.position))) <=
                    punctumTrapConsumeRadius)
            {
                s.position = respawnAtEdge();
                continue;
            }

            s.position = Vector2Add(s.position, driftFor(s.position));

            if (s.position.x < 0)
            {
                s.position.x += tileW;
            }
            if (s.position.x > tileW)
            {
                s.position.x -= tileW;
            }
            if (s.position.y < 0)
            {
                s.position.y += tileH;
            }
            if (s.position.y > tileH)
            {
                s.position.y -= tileH;
            }
        }
    }
}

auto UpdateGame(Game& game, float deltaTime) -> bool
{
    if (IsKeyPressed(KEY_F11))
    {
        toggleFullscreen(game);
    }
    syncScreenSize(game);
    updateTouchControls(game);
    if (game.autoplay && updateAutoplayBot(game, deltaTime))
    {
        return true;
    }
    updatePointerHold(game);

    updateBgParticles(game);
    updateDeathParticles(game, deltaTime);
    updateDamageNumbers(game, deltaTime);
    updateWeaponDowngrade(game, deltaTime);
    updateBgmLayers(game, deltaTime);
    UpdateMusicStream(game.resources.bgm.base);
    UpdateMusicStream(game.resources.bgm.intensity);
    UpdateMusicStream(game.resources.bgm.miniboss);
    UpdateMusicStream(game.resources.bgm.megaboss);
    UpdateMusicStream(game.resources.bgm.swarmBoss);

    if (game.run.shakeTimer > 0)
    {
        game.run.shakeTimer -= deltaTime;
        if (game.run.shakeTimer <= 0)
        {
            game.run.shakeTimer = 0;
            game.run.shakeIntensity = 0;
        }
    }

    if (game.run.shieldDashShakeCooldownTimer > 0)
    {
        game.run.shieldDashShakeCooldownTimer -= deltaTime;
    }

    if (game.run.hitPauseTimer > 0)
    {
        game.run.hitPauseTimer -= deltaTime;
        return false;
    }

    if (game.run.achievementToastTimer > 0)
    {
        game.run.achievementToastTimer -= deltaTime;
    }

    switch (game.state)
    {
    case GameState::TITLE:
        return updateTitle(game);
    case GameState::SHIP_SELECT:
        return updateShipSelect(game);
    case GameState::GAMEPLAY:
        updateGameplay(game, deltaTime);
        break;
    case GameState::PAUSED:
        return updatePaused(game);
    case GameState::LEVEL_UP:
        return updateLevelUp(game);
    case GameState::GAME_OVER:
        return updateGameOver(game);
    case GameState::SETTINGS:
        return updateSettings(game);
    case GameState::ACHIEVEMENTS:
        return updateAchievements(game);
    case GameState::BESTIARY:
        return updateBestiary(game);
    case GameState::SKILL_TREE:
        return updateSkillTree(game);
    case GameState::WEAPON_GALLERY:
        if (IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                                         CheckCollisionPointRec(GetMousePosition(), backButtonRect(game))))
        {
            game.state = GameState::SANDBOX_MENU;
        }
        return false;
    case GameState::SANDBOX_MENU:
        updateSandboxMenuInput(game);
        break;
    case GameState::ENDING:
        return updateEnding(game, deltaTime);
    }

    return false;
}
