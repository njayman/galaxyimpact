#include "update_internal.hpp"

void spawnRockCluster(Game& game, Vector2 center)
{
    constexpr int32_t chunkCount = 5;
    constexpr float orbitRadius = 70.0F;
    const float rotationSpeed =
        static_cast<float>(GetRandomValue(0, 1) == 0 ? 1 : -1) * rockClusterRotationSpeed;

    for (int32_t i = 0; i < chunkCount; i++)
    {
        const auto tier = GetRandomValue(0, 1) == 0 ? AsteroidTier::Large : AsteroidTier::Medium;
        const float startAngle = static_cast<float>(i) * (360.0F / chunkCount);

        game.run.asteroids.push_back(Asteroid{
            .position =
                Vector2Add(center, Vector2{.x = std::cos(startAngle * DEG2RAD) * orbitRadius,
                                           .y = std::sin(startAngle * DEG2RAD) * orbitRadius}),
            .velocity = Vector2{},
            .radius = asteroidRadius(tier),
            .tier = tier,
            .active = true,
            .inCluster = true,
            .clusterCenter = center,
            .clusterOrbitRadius = orbitRadius,
            .clusterAngleDeg = startAngle,
            .clusterRotationSpeed = rotationSpeed});
    }
}

void updateSolarForgeHeat(Game& game, float deltaTime)
{
    if (game.run.player.health <= 0)
    {
        return;
    }

    const bool inSolarForge = currentBiome(game.run.waveNumber) == Biome::SolarForge;
    const bool onSafePath = !inSolarForge || isSolarForgeCaveOpen(game.run.player.position);

    // Gel wobble: a spring dragged along by the ship while inside, kicked on entry.
    auto& run = game.run;
    const bool inGel = !onSafePath;
    if (inGel && !run.solarForgeInGel)
    {
        run.solarForgeGelWobbleVel = Vector2Add(
            run.solarForgeGelWobbleVel,
            Vector2Scale(Vector2Normalize(run.player.velocity), solarForgeGelSplash));
    }
    run.solarForgeInGel = inGel;
    const Vector2 target = inGel ? Vector2Scale(run.player.velocity, solarForgeGelDrag) : Vector2{};
    const Vector2 accel = Vector2Subtract(
        Vector2Scale(Vector2Subtract(target, run.solarForgeGelWobble), solarForgeGelStiffness),
        Vector2Scale(run.solarForgeGelWobbleVel, solarForgeGelDamping));
    run.solarForgeGelWobbleVel = Vector2Add(run.solarForgeGelWobbleVel, Vector2Scale(accel, deltaTime));
    run.solarForgeGelWobble = Vector2ClampValue(
        Vector2Add(run.solarForgeGelWobble, Vector2Scale(run.solarForgeGelWobbleVel, deltaTime)), 0.0F,
        solarForgeGelMaxWobble);
    run.solarForgeGelPocket =
        std::clamp(run.solarForgeGelPocket + (inGel ? 4.0F : -4.0F) * deltaTime, 0.0F, 1.0F);

    if (onSafePath)
    {
        game.run.solarForgeHeatMeter =
            std::max(0.0F, game.run.solarForgeHeatMeter - solarForgeHeatDecayRate * deltaTime);
    }
    else
    {
        game.run.solarForgeHeatMeter += deltaTime;
    }

    if (!inSolarForge || game.run.solarForgeHeatMeter <= solarForgeMeltThreshold)
    {
        game.run.solarForgeHeatTickTimer = 0;
        return;
    }

    game.run.solarForgeHeatTickTimer -= deltaTime;
    if (game.run.solarForgeHeatTickTimer <= 0)
    {
        game.run.solarForgeHeatTickTimer = solarForgeMeltTickInterval;
        damagePlayer(game, enemyDamage(game, solarForgeMeltDamage));
    }
}

void updatePunctumThunder(Game& game, float deltaTime)
{
    if (game.run.punctumThunderFlashTimer > 0)
    {
        game.run.punctumThunderFlashTimer -= deltaTime;
    }

    if (currentBiome(game.run.waveNumber) != Biome::Punctum)
    {
        return;
    }

    game.run.punctumThunderTimer -= deltaTime;
    if (game.run.punctumThunderTimer > 0)
    {
        return;
    }

    float intervalReduction = 0;
    if (game.run.waveNumber >= punctumThunderSpeedupWaveA)
    {
        intervalReduction += 1.0F;
    }
    if (game.run.waveNumber >= punctumThunderSpeedupWaveB)
    {
        intervalReduction += 1.0F;
    }
    const float intervalMin = std::max(1.0F, punctumThunderIntervalMin - intervalReduction);
    const float intervalMax = std::max(intervalMin + 1.0F, punctumThunderIntervalMax - intervalReduction);
    game.run.punctumThunderTimer =
        static_cast<float>(GetRandomValue(static_cast<int32_t>(intervalMin * 10),
                                          static_cast<int32_t>(intervalMax * 10))) /
        10.0F;
    game.run.punctumThunderFlashTimer = punctumThunderFlashDuration;
    const float boltAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto boltDist = static_cast<float>(GetRandomValue(300, 900));
    game.run.punctumThunderBoltOrigin =
        Vector2Add(game.run.player.position,
                  Vector2{.x = std::cos(boltAngle) * boltDist, .y = std::sin(boltAngle) * boltDist});
    game.run.punctumThunderBoltSeed = static_cast<float>(GetRandomValue(0, 10000));
    triggerShake(game, 6, 0.3F);
    playSFX(game, game.resources.sounds.thunder);
}

auto debrisSquallRock(const Game& game, int32_t index) -> SquallRock
{
    const float angle = game.run.debrisSquallAngle;
    const Vector2 along{.x = std::cos(angle), .y = std::sin(angle)};
    const Vector2 across{.x = -along.y, .y = along.x};
    const float seed = static_cast<float>(index) * 13.7F;
    const float s = game.run.debrisSquallFront - hashNoise(seed) * debrisSquallStreamLength;
    const float off = (hashNoise(seed + 1.0F) * 2.0F - 1.0F) * debrisSquallHalfWidth;
    const float playerAlong =
        Vector2DotProduct(Vector2Subtract(game.run.player.position, game.run.debrisSquallCenter), along);
    const bool broken = ((game.run.debrisSquallBrokenMask >> index) & 1U) != 0;
    return SquallRock{
        .position = Vector2Add(game.run.debrisSquallCenter,
                               Vector2Add(Vector2Scale(along, s), Vector2Scale(across, off))),
        .radius = 6.0F + hashNoise(seed + 2.0F) * 10.0F,
        .seed = seed,
        .live = game.run.debrisSquallPhase == 2 && !broken && std::abs(s - playerAlong) <= debrisSquallReach};
}

void updateDebrisSquall(Game& game, float deltaTime)
{
    const bool beltbreakerFight = std::ranges::any_of(
        game.run.bosses, [](const Boss& boss) { return boss.isBeltbreaker || boss.isBeltbreakerPlate; });
    // A bare Beltbreaker calls squalls straight through the player.
    const bool called = std::ranges::any_of(game.run.bosses, [&](const Boss& boss) { return beltbreakerBare(game, boss); });
    if (called && game.run.debrisSquallPhase == 0)
    {
        game.run.debrisSquallTimer = std::min(game.run.debrisSquallTimer, beltbreakerCalledSquallInterval);
    }
    if (currentBiome(game.run.waveNumber) != Biome::ShatteredBelt || (beltbreakerFight && !called))
    {
        game.run.debrisSquallPhase = 0;
        return;
    }
    if (bossQteActive(game))
    {
        return;
    }

    const float angle = game.run.debrisSquallAngle;
    const Vector2 along{.x = std::cos(angle), .y = std::sin(angle)};
    const Vector2 across{.x = -along.y, .y = along.x};

    if (game.run.debrisSquallPhase == 0)
    {
        game.run.debrisSquallTimer -= deltaTime;
        if (game.run.debrisSquallTimer > 0)
        {
            return;
        }
        const float ramp =
            std::clamp(static_cast<float>(game.run.waveNumber - 1) / 24.0F, 0.0F, 1.0F);
        game.run.debrisSquallTimer =
            std::lerp(debrisSquallIntervalStart, debrisSquallIntervalEnd, ramp);
        game.run.debrisSquallPhase = 1;
        game.run.debrisSquallPhaseTimer = debrisSquallWarnTime;
        game.run.debrisSquallAngle = static_cast<float>(GetRandomValue(0, 3599)) / 10.0F * DEG2RAD; // any direction
        const auto offset = called ? 0.0F
                                   : static_cast<float>(GetRandomValue(-static_cast<int32_t>(debrisSquallMaxOffset),
                                                                       static_cast<int32_t>(debrisSquallMaxOffset)));
        game.run.debrisSquallCenter =
            Vector2Add(game.run.player.position, Vector2Scale(across, offset));
        return;
    }

    if (game.run.debrisSquallPhase == 1)
    {
        game.run.debrisSquallPhaseTimer -= deltaTime;
        if (game.run.debrisSquallPhaseTimer <= 0)
        {
            game.run.debrisSquallPhase = 2;
            // Rocks enter just off screen from wherever the player now is along the band.
            game.run.debrisSquallFront =
                Vector2DotProduct(Vector2Subtract(game.run.player.position, game.run.debrisSquallCenter), along) -
                debrisSquallReach;
            game.run.debrisSquallEnemyTickTimer = 0;
            game.run.debrisSquallBrokenMask = 0;
            triggerShake(game, 3, 0.4F);
        }
        return;
    }

    game.run.debrisSquallFront += debrisSquallSpeed * deltaTime;
    if (game.run.debrisSquallFront - debrisSquallStreamLength >
        Vector2DotProduct(Vector2Subtract(game.run.player.position, game.run.debrisSquallCenter), along) +
            debrisSquallReach)
    {
        game.run.debrisSquallPhase = 0;
        return;
    }

    // Each rock hits what it touches; shooting one pops it, clearing that piece of the band.
    const auto inRocks = [&](Vector2 pos, float radius)
    {
        for (int32_t i = 0; i < debrisSquallRockCount; i++)
        {
            const SquallRock rock = debrisSquallRock(game, i);
            if (rock.live && CheckCollisionCircles(pos, radius, rock.position, rock.radius))
            {
                return true;
            }
        }
        return false;
    };

    for (auto& bullet : game.run.bullets)
    {
        if (!bullet.active)
        {
            continue;
        }
        for (int32_t i = 0; i < debrisSquallRockCount; i++)
        {
            const SquallRock rock = debrisSquallRock(game, i);
            if (!rock.live || !CheckCollisionCircles(bullet.position, bullet.radius, rock.position, rock.radius))
            {
                continue;
            }
            game.run.debrisSquallBrokenMask |= uint64_t{1} << i;
            spawnKillExplosion(game, rock.position, Palette::StructLight, 8, 0.8F);
            spawnKillExplosion(game, rock.position, Palette::ElementBurn, 6, 1.2F);
            playSFX(game, game.resources.sounds.rockBreak);
            triggerShake(game, 1.5F, 0.08F);
            game.run.score += debrisSquallRockScore;
            if (bullet.streak <= 0 && !bullet.phasing)
            {
                if (bullet.pierceRemaining > 0)
                {
                    bullet.pierceRemaining--;
                }
                else
                {
                    bullet.active = false;
                    break;
                }
            }
        }
    }

    // Attacks that aren't bullets pop rocks too: orbit blades (circling and flung), and the Nerve
    // ball and tornado. Without this the Bastion, which fires no bullets, couldn't clear a squall.
    const auto popRocksAt = [&](Vector2 pos, float radius)
    {
        for (int32_t i = 0; i < debrisSquallRockCount; i++)
        {
            const SquallRock rock = debrisSquallRock(game, i);
            if (rock.live && CheckCollisionCircles(pos, radius, rock.position, rock.radius))
            {
                game.run.debrisSquallBrokenMask |= uint64_t{1} << i;
                spawnKillExplosion(game, rock.position, Palette::StructLight, 8, 0.8F);
                spawnKillExplosion(game, rock.position, Palette::ElementBurn, 6, 1.2F);
                playSFX(game, game.resources.sounds.rockBreak);
                game.run.score += debrisSquallRockScore;
            }
        }
    };
    for (const auto& w : game.run.weapons)
    {
        if (w.type != WeaponType::Orbit)
        {
            continue;
        }
        const float ring = orbitRadius(w.level) * (w.evolved ? 1.4F : 1.0F);
        const int32_t count = orbitBladeCount(w.level);
        for (int32_t s = 0; s < count; s++)
        {
            popRocksAt(orbitBladePosition(game, game.run.player.position, ring, s, count), orbitBladeHitRadius);
        }
    }
    for (const auto& list : {&game.run.orbitBladeProjectiles, &game.run.nerveBallProjectiles})
    {
        for (const auto& proj : *list)
        {
            if (proj.active)
            {
                popRocksAt(proj.position, proj.radius);
            }
        }
    }
    for (const auto& spiral : game.run.nerveSpiralProjectiles)
    {
        for (int32_t s = 0; spiral.active && s < spiral.bladeCount; s++)
        {
            const float angle = spiral.age * nerveSpiralSpinSpeed +
                                static_cast<float>(s) * 2 * std::numbers::pi_v<float> / static_cast<float>(spiral.bladeCount);
            popRocksAt(Vector2Add(spiral.origin, Vector2{.x = std::cos(angle) * spiral.spinRadius,
                                                          .y = std::sin(angle) * spiral.spinRadius}),
                       orbitBladeHitRadius * 2.0F);
        }
    }

    if (game.run.player.health > 0 && game.run.player.immunityTimer <= 0 &&
        inRocks(game.run.player.position, game.run.player.radius))
    {
        damagePlayer(game, enemyDamage(game, debrisSquallPlayerDamage));
    }

    game.run.debrisSquallEnemyTickTimer -= deltaTime;
    if (game.run.debrisSquallEnemyTickTimer > 0)
    {
        return;
    }
    game.run.debrisSquallEnemyTickTimer = debrisSquallEnemyTickInterval;
    for (size_t i = 0; i < game.run.enemies.size(); i++)
    {
        const auto& enemy = game.run.enemies.at(i);
        if (enemy.active && enemy.health > 0 &&
            inRocks(enemy.position, enemyKinds.at(static_cast<size_t>(enemy.kind)).radius))
        {
            damageEnemy(game, i, debrisSquallEnemyDamage);
        }
    }
    for (auto& boss : game.run.bosses)
    {
        // Lure the bare core into its own squall.
        if (boss.isBeltbreaker && boss.health > 0 &&
            inRocks(beltbreakerCoreCenter(boss), boss.size.x * 0.4F))
        {
            damageBoss(game, boss, debrisSquallBossDamage, false);
        }
    }
}

namespace
{
auto rustPatchRamp(const Game& game) -> float
{
    return std::clamp(static_cast<float>(game.run.waveNumber - 26) / 24.0F, 0.0F, 1.0F);
}

void addRustPatch(Game& game, Vector2 position)
{
    if (game.run.rustPatches.size() >= static_cast<size_t>(rustPatchMaxCount))
    {
        return;
    }
    game.run.rustPatches.push_back(
        RustPatch{.position = position,
                  .radius = rustPatchStartRadius,
                  .age = 0,
                  .budTimer = std::lerp(rustPatchBudIntervalStart, rustPatchBudIntervalEnd,
                                        rustPatchRamp(game)),
                  .seed = static_cast<float>(GetRandomValue(0, 9999)) * 0.37F,
                  .active = true});
}

void burnRustPatch(Game& game, RustPatch& patch)
{
    patch.active = false;
    spawnKillExplosion(game, patch.position, Palette::ElementBurn, 10, 1.0F);
    spawnKillExplosion(game, patch.position, Palette::RustbloomAccent, 6, 0.7F);
}
} // namespace

void bloomRustPatch(Game& game, Vector2 position, bool guaranteed)
{
    if (currentBiome(game.run.waveNumber) != Biome::Rustbloom)
    {
        return;
    }
    const float ramp = rustPatchRamp(game);
    const auto chance =
        static_cast<int32_t>(std::lerp(static_cast<float>(rustPatchBloomChanceStart),
                                       static_cast<float>(rustPatchBloomChanceEnd), ramp));
    if (!guaranteed && GetRandomValue(0, 99) >= chance)
    {
        return;
    }
    // Dying on top of rust feeds that patch instead of starting a new one.
    const float maxRadius = std::lerp(rustPatchMaxRadiusStart, rustPatchMaxRadiusEnd, ramp);
    for (auto& patch : game.run.rustPatches)
    {
        if (patch.active && Vector2Distance(patch.position, position) < patch.radius)
        {
            // Never jump the edge onto a player standing outside: that would skip the tell.
            float grown = std::min(maxRadius, patch.radius + rustPatchFeedGrowth);
            const float playerDist = Vector2Distance(patch.position, game.run.player.position);
            if (playerDist >= patch.radius)
            {
                grown = std::min(grown, std::max(patch.radius, playerDist - game.run.player.radius));
            }
            patch.radius = grown;
            return;
        }
    }
    addRustPatch(game, position);
}

void updateRustPatches(Game& game, float deltaTime)
{
    if (currentBiome(game.run.waveNumber) != Biome::Rustbloom)
    {
        game.run.rustPatches.clear();
        return;
    }
    if (bossQteActive(game) || game.run.rustPatches.empty())
    {
        return;
    }

    const float ramp = rustPatchRamp(game);
    const float maxRadius = std::lerp(rustPatchMaxRadiusStart, rustPatchMaxRadiusEnd, ramp);
    const float spreadRate = std::lerp(rustPatchSpreadRateStart, rustPatchSpreadRateEnd, ramp);
    const float budInterval = std::lerp(rustPatchBudIntervalStart, rustPatchBudIntervalEnd, ramp);
    const size_t patchCount = game.run.rustPatches.size(); // buds join after this loop
    for (size_t i = 0; i < patchCount; i++)
    {
        auto& patch = game.run.rustPatches.at(i);
        if (Vector2Distance(patch.position, game.run.player.position) > entityDespawnRadius)
        {
            patch.active = false;
            continue;
        }
        patch.age += deltaTime;
        if (patch.radius < maxRadius)
        {
            patch.radius = std::min(maxRadius, patch.radius + spreadRate * deltaTime);
            continue;
        }
        patch.budTimer -= deltaTime;
        if (patch.budTimer <= 0)
        {
            patch.budTimer = budInterval;
            const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 budPos = Vector2Add(
                patch.position, Vector2Scale(Vector2{.x = std::cos(angle), .y = std::sin(angle)},
                                             patch.radius + rustPatchStartRadius * 0.5F));
            addRustPatch(game, budPos); // may reallocate: patch is not touched after this
        }
    }

    // Bullets flying over a patch scorch it down until it burns away. Bullets pass through, and a
    // patch the player stands in is out of reach of their own fire: step out or dash through it.
    for (auto& patch : game.run.rustPatches)
    {
        if (!patch.active ||
            CheckCollisionCircles(game.run.player.position, game.run.player.radius * 0.5F,
                                  patch.position, patch.radius))
        {
            continue;
        }
        for (const auto& bullet : game.run.bullets)
        {
            if (!bullet.active || !CheckCollisionCircles(bullet.position, bullet.radius,
                                                         patch.position, patch.radius))
            {
                continue;
            }
            patch.radius -= rustPatchBulletBurnRate * deltaTime;
            if (patch.radius < rustPatchMinRadius)
            {
                burnRustPatch(game, patch);
                break;
            }
            if (GetRandomValue(0, 3) == 0)
            {
                spawnKillExplosion(game, bullet.position, Palette::ElementBurn, 2, 0.5F);
            }
        }
    }

    // Dashing through rust burns it away; standing in a ripe patch ticks damage.
    bool inRust = false;
    for (auto& patch : game.run.rustPatches)
    {
        if (!patch.active ||
            !CheckCollisionCircles(game.run.player.position, game.run.player.radius * 0.5F,
                                   patch.position, patch.radius))
        {
            continue;
        }
        if (game.run.player.dashing)
        {
            burnRustPatch(game, patch);
            continue;
        }
        inRust = inRust || patch.age >= rustPatchRipenTime;
    }

    if (!inRust)
    {
        game.run.rustPatchTickTimer = rustPatchEntryGrace;
    }
    else if (game.run.player.health > 0 && game.run.player.immunityTimer <= 0)
    {
        game.run.rustPatchTickTimer -= deltaTime;
        if (game.run.rustPatchTickTimer <= 0)
        {
            game.run.rustPatchTickTimer = rustPatchTickInterval;
            damagePlayer(game, enemyDamage(game, rustPatchPlayerDamage));
        }
    }

    std::erase_if(game.run.rustPatches, [](const RustPatch& p) { return !p.active; });
}

void updateFluidHazardContact(Game& game)
{
    if (game.run.player.health <= 0)
    {
        return;
    }

    bool touching = false;
    for (const auto& asteroid : game.run.asteroids)
    {
        if (asteroid.active && asteroid.isFluid &&
            Vector2Distance(asteroid.position, game.run.player.position) <=
                asteroid.radius + game.run.player.radius)
        {
            touching = true;
            break;
        }
    }

    if (!touching)
    {
        return;
    }

    game.run.player.burnDps = solarForgeFluidBurnDps;
    game.run.player.burnRefreshTimer = solarForgeFluidBurnRefreshDuration;
}

void updateGasHazards(Game& game, float deltaTime)
{
    const bool shieldDashing = game.run.player.shieldDashing;

    for (auto& cloud : game.run.gasHazards)
    {
        if (!cloud.active)
        {
            continue;
        }

        cloud.life -= deltaTime;
        if (cloud.life <= 0)
        {
            cloud.active = false;
            continue;
        }

        const Vector2 toPlayer = Vector2Subtract(game.run.player.position, cloud.position);
        if (!cloud.anchored && Vector2Length(toPlayer) > 1)
        {
            cloud.position = Vector2Add(
                cloud.position,
                Vector2Scale(Vector2Normalize(toPlayer), wreckwormCloudDriftSpeed * deltaTime));
        }

        const float contactDist = cloud.radius + game.run.player.radius;
        if (Vector2Distance(cloud.position, game.run.player.position) > contactDist)
        {
            continue;
        }

        if (shieldDashing && !cloud.spores) // a Blight Gate cloud can't be dashed away: it is the toll
        {
            cloud.active = false;
            triggerShake(game, 4, 0.15F);
            continue;
        }

        if (cloud.spores)
        {
            game.run.player.slowTimer =
                std::max(game.run.player.slowTimer, blightGateSporeSlowRefresh);
            continue;
        }

        if (game.run.player.health > 0)
        {
            game.run.player.burnDps = wreckwormCloudBurnDps;
            game.run.player.burnRefreshTimer = wreckwormCloudBurnRefreshDuration;
        }
    }

    std::erase_if(game.run.gasHazards, [](const GasHazard& cloud) { return !cloud.active; });
}

void updateWaveSpawner(Game& game, float deltaTime)
{
    if (game.sandbox && !game.sandboxNaturalSpawnEnabled)
    {
        return;
    }

    if (game.run.bosses.empty())
    {
        game.run.waveTimer -= deltaTime;
        if (game.run.waveTimer <= 0)
        {
            game.run.waveNumber++;
            game.run.waveTimer = GameConstants::waveDuration;
            recordWaveReached(game, game.run.waveNumber);

            if (currentBiome(game.run.waveNumber) == Biome::ShatteredBelt &&
                     (game.run.waveNumber == 10 || game.run.waveNumber == 20 ||
                      game.run.waveNumber == 25))
            {
                spawnBeltbreaker(game, game.run.waveNumber);
            }
            else if (currentBiome(game.run.waveNumber) == Biome::Rustbloom &&
                     (game.run.waveNumber == 30 || game.run.waveNumber == 40 ||
                      game.run.waveNumber == 50))
            {
                spawnWreckworm(game, game.run.waveNumber);
            }
            else if (currentBiome(game.run.waveNumber) == Biome::SolarForge &&
                     (game.run.waveNumber == 60 || game.run.waveNumber == 70 ||
                      game.run.waveNumber == 75))
            {
                spawnSlagmaw(game, game.run.waveNumber);
            }
            else if (currentBiome(game.run.waveNumber) == Biome::Punctum &&
                     (game.run.waveNumber == 80 || game.run.waveNumber == 90 ||
                      game.run.waveNumber == 100))
            {
                spawnKraken(game, game.run.waveNumber);
            }
            else if (game.run.waveNumber % megaBossWaveInterval == 0)
            {
                spawnBossWave(game, megaBossHealthMult, megaBossSizeMult, true);
            }
            else if (game.run.waveNumber % miniBossWaveInterval == 0)
            {
                spawnBossWave(game, miniBossHealthMult, miniBossSizeMult, false);
            }
        }
    }

    if (game.run.bosses.empty())
    {
        game.run.enemySpawnTimer -= deltaTime;
        if (game.run.enemySpawnTimer <= 0 &&
            static_cast<int>(game.run.enemies.size()) < UpdateConstants::maxEnemies)
        {

            const int cycleIndex = (game.run.waveNumber - 1) / spawnRateCycleLength;
            const int cyclePosition = (game.run.waveNumber - 1) % spawnRateCycleLength + 1;

            const float baseInterval = std::max(
                1.2F - static_cast<float>(cycleIndex) * spawnRateCycleStep, spawnIntervalFloor);
            const float phaseMultiplier =
                cyclePosition <= spawnRateCycleLength / 2 ? 1.0F : spawnRateSecondHalfMultiplier;
            const float interval = std::max(baseInterval * phaseMultiplier, spawnIntervalFloor);

            constexpr float spawnRateMult = 0.85F;
            const bool inPunctum = currentBiome(game.run.waveNumber) == Biome::Punctum;
            const float punctumIntervalMult = inPunctum ? punctumSpawnIntervalMult : 1.0F;
            game.run.enemySpawnTimer = interval * spawnRateMult * punctumIntervalMult;

            const auto rawSpawnCount =
                static_cast<int>(static_cast<float>(1 + game.run.waveNumber / 4) * swarmSpawnCountMult);
            const int spawnCount =
                inPunctum ? std::max(1, rawSpawnCount / punctumSpawnCountDivisor) : rawSpawnCount;
            for (int i = 0; i < spawnCount &&
                            static_cast<int>(game.run.enemies.size()) < UpdateConstants::maxEnemies;
                 i++)
            {
                spawnEnemy(game);
            }
        }
    }

    game.run.asteroidSpawnTimer -= deltaTime;
    if (game.run.asteroidSpawnTimer <= 0 &&
        static_cast<int>(game.run.asteroids.size()) < maxAsteroid)
    {
        constexpr float asteroidIntervalMultiplier = 1.0F;
        game.run.asteroidSpawnTimer =
            static_cast<float>(GetRandomValue(8, 16)) / 10.0F * asteroidIntervalMultiplier;

        if (currentBiome(game.run.waveNumber) == Biome::ShatteredBelt && GetRandomValue(0, 2) == 0)
        {
            const float clusterAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 clusterCenter =
                Vector2Add(game.run.player.position, Vector2{.x = std::cos(clusterAngle) * 500,
                                                             .y = std::sin(clusterAngle) * 500});
            spawnRockCluster(game, clusterCenter);
            return;
        }

        AsteroidTier tier = AsteroidTier::Large;
        if (GetRandomValue(0, 1) == 1)
        {
            tier = AsteroidTier::Medium;
        }

        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const Vector2 spawnPos =
            Vector2Add(game.run.player.position,
                       Vector2{.x = std::cos(angle) * 500, .y = std::sin(angle) * 500});

        if (currentBiome(game.run.waveNumber) == Biome::SolarForge)
        {

            game.run.asteroids.push_back(Asteroid{.position = spawnPos,
                                                  .velocity = Vector2{},
                                                  .radius = asteroidRadius(tier),
                                                  .tier = tier,
                                                  .active = true,
                                                  .isFluid = true});
            return;
        }

        const float driftAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const Vector2 direction{.x = std::cos(driftAngle), .y = std::sin(driftAngle)};
        const float speed = static_cast<float>(GetRandomValue(25, 45)) / 10.0F;

        game.run.asteroids.push_back(Asteroid{.position = spawnPos,
                                              .velocity = Vector2Scale(direction, speed),
                                              .radius = asteroidRadius(tier),
                                              .tier = tier,
                                              .active = true});
    }
}

constexpr int eliteHazardCap = 3;

void updateSolarForgeEnemyFireParticles(Game& game, float deltaTime)
{
    for (const auto& enemy : game.run.enemies)
    {
        if (!enemy.active || enemy.phased)
        {
            continue;
        }
        const auto& kind = enemyKinds.at(static_cast<size_t>(enemy.kind));
        if (!kind.biomeExclusive || kind.biome != Biome::SolarForge)
        {
            continue;
        }
        if (GetRandomValue(0, 99) >= solarForgeEnemyFireChancePercent)
        {
            continue;
        }

        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto offset = static_cast<float>(GetRandomValue(0, static_cast<int32_t>(kind.radius)));
        game.run.flameParticles.push_back(Particle{
            .position = Vector2Add(
                enemy.position, Vector2{.x = std::cos(angle) * offset, .y = std::sin(angle) * offset}),
            .velocity = Vector2{.x = 0, .y = -solarForgeEnemyFireParticleSpeed * deltaTime},
            .radius = solarForgeEnemyFireParticleRadius,
            .life = solarForgeEnemyFireParticleLife,
            .maxLife = solarForgeEnemyFireParticleLife,
            .color = GetRandomValue(0, 1) == 0 ? Palette::ElementBurn : Palette::SolarForgeAccent});
    }

    for (const auto& boss : game.run.bosses)
    {
        if (!boss.isSlagmaw || boss.health <= 0)
        {
            continue;
        }
        if (GetRandomValue(0, 99) >= solarForgeEnemyFireChancePercent * 3)
        {
            continue;
        }
        const Vector2 center{.x = boss.position.x + boss.size.x / 2,
                             .y = boss.position.y + boss.size.y / 2};
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto offset =
            static_cast<float>(GetRandomValue(0, static_cast<int32_t>(boss.size.x / 2)));
        game.run.flameParticles.push_back(Particle{
            .position = Vector2Add(center, Vector2{.x = std::cos(angle) * offset,
                                                    .y = std::sin(angle) * offset}),
            .velocity = Vector2{.x = 0, .y = -solarForgeEnemyFireParticleSpeed * deltaTime},
            .radius = solarForgeEnemyFireParticleRadius * 1.5F,
            .life = solarForgeEnemyFireParticleLife,
            .maxLife = solarForgeEnemyFireParticleLife,
            .color = GetRandomValue(0, 1) == 0 ? Palette::ElementBurn : Palette::SolarForgeAccent});
    }
}

void updateAsteroids(Game& game, float deltaTime)
{
    for (auto& asteroid : game.run.asteroids)
    {
        if (!asteroid.active)
        {
            continue;
        }

        if (asteroid.grinderOrbit && !game.run.blackhole.active)
        {
            const float a = asteroid.clusterAngleDeg * DEG2RAD;
            asteroid.velocity = Vector2{.x = -std::sin(a) * grinderReleaseSpeed, .y = std::cos(a) * grinderReleaseSpeed};
            asteroid.grinderOrbit = false;
            asteroid.inCluster = false;
        }
        if (asteroid.grinderOrbit)
        {
            asteroid.clusterCenter = game.run.blackhole.position;
        }

        if (asteroid.inCluster)
        {
            asteroid.clusterAngleDeg = std::fmod(
                asteroid.clusterAngleDeg + asteroid.clusterRotationSpeed * deltaTime, 360.0F);
            asteroid.position = Vector2Add(
                asteroid.clusterCenter, Vector2{.x = std::cos(asteroid.clusterAngleDeg * DEG2RAD) *
                                                     asteroid.clusterOrbitRadius,
                                                .y = std::sin(asteroid.clusterAngleDeg * DEG2RAD) *
                                                     asteroid.clusterOrbitRadius});
        }
        else
        {
            asteroid.position = Vector2Add(asteroid.position,
                                           Vector2Scale(asteroid.velocity, deltaTime * frameScale));
        }

        if (Vector2Distance(asteroid.position, game.run.player.position) > asteroidDespawnRadius)
        {
            asteroid.active = false;
            continue;
        }

        if (game.run.blackhole.active)
        {
            const Vector2 toHole = Vector2Subtract(game.run.blackhole.position, asteroid.position);
            const float dist = Vector2Length(toHole);

            // Grinder: in the Belt, rock caught in the pull orbits the hole as shrapnel instead
            // of being swallowed.
            if (!asteroid.grinderOrbit && !game.run.blackhole.isKrakenPortal &&
                currentBiome(game.run.waveNumber) == Biome::ShatteredBelt &&
                dist <= game.run.blackhole.influenceRadius && dist > 0)
            {
                asteroid.grinderOrbit = true;
                asteroid.inCluster = true;
                asteroid.clusterCenter = game.run.blackhole.position;
                asteroid.clusterOrbitRadius =
                    std::clamp(dist, game.run.blackhole.radius * grinderOrbitMinRadiusMult,
                               game.run.blackhole.influenceRadius * grinderOrbitMaxInfluenceFrac);
                asteroid.clusterAngleDeg = std::atan2(-toHole.y, -toHole.x) * RAD2DEG;
                asteroid.clusterRotationSpeed = grinderOrbitSpeedDeg;
            }

            if (!asteroid.grinderOrbit && dist <= game.run.blackhole.influenceRadius && dist > 0)
            {
                const Vector2 pull = Vector2Scale(Vector2Normalize(toHole), blackHoleAsteroidPull);
                asteroid.position =
                    Vector2Add(asteroid.position, Vector2Scale(pull, deltaTime * frameScale));
            }

            if (dist <= game.run.blackhole.radius)
            {
                asteroid.active = false;
                breakAsteroid(game, asteroid);
                continue;
            }
        }

        if (game.run.player.health > 0 && game.run.player.immunityTimer <= 0 &&
            CheckCollisionCircles(game.run.player.position, game.run.player.radius,
                                  asteroid.position, asteroid.radius))
        {
            if (game.run.player.shieldActive)
            {
                game.run.player.shieldActive = false;
                game.run.player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
            }
            else
            {
                damagePlayer(game, enemyDamage(game, 1));
            }

            asteroid.active = false;
            breakAsteroid(game, asteroid);
        }
    }
}

void updateProjectiles(Game& game, float deltaTime)
{
    for (auto& projectile : game.run.bossProjectiles)
    {
        if (!projectile.active)
        {
            continue;
        }
        if (projectile.life > 0)
        {
            projectile.life -= deltaTime;
            projectile.active = projectile.life > 0;
        }

        if (projectile.homing)
        {
            if (projectile.fromPlayer)
            {
                if (projectile.huntingNewTarget)
                {

                    if (const auto target =
                            nearestEnemyWithin(game, projectile.position, mineSeekRadius);
                        target.has_value())
                    {
                        const Vector2 direction =
                            Vector2Normalize(Vector2Subtract(*target, projectile.position));
                        projectile.velocity = Vector2Scale(direction, homingProjSpeed * 1.5F);
                        projectile.huntingNewTarget = false;
                    }
                }
                else
                {
                    // After a ricochet, don't steer back into the enemy just bounced off.
                    const auto lastHit = std::ranges::find_if(
                        game.run.enemies, [&](const Enemy& e)
                        { return e.active && e.id == projectile.ricochetLastHit; });
                    const auto target =
                        (projectile.ricochetLastHit != 0 && lastHit != game.run.enemies.end())
                            ? nearestEnemyExcluding(game, projectile.position, lastHit->position)
                            : nearestEnemy(game, projectile.position);
                    if (target.has_value())
                    {
                        const Vector2 direction =
                            Vector2Normalize(Vector2Subtract(*target, projectile.position));
                        projectile.velocity = Vector2Scale(direction, homingProjSpeed * 1.5F);
                    }
                }
            }
            else
            {
                const Vector2 direction = Vector2Normalize(
                    Vector2Subtract(game.run.player.position, projectile.position));
                projectile.velocity = Vector2Scale(direction, homingProjSpeed);
            }
        }

        if (projectile.orbitTime > 0 && !projectile.fromPlayer)
        {
            projectile.orbitTime -= deltaTime;
            projectile.orbitAngle += projectile.orbitSpin * deltaTime;
            projectile.orbitRadius = std::max(0.0F, projectile.orbitRadius - projectile.orbitShrink * deltaTime);
            const Vector2 out{.x = std::cos(projectile.orbitAngle), .y = std::sin(projectile.orbitAngle)};
            projectile.position = Vector2Add(projectile.orbitCenter, Vector2Scale(out, projectile.orbitRadius));
            projectile.velocity =
                projectile.orbitTime > 0 ? Vector2{} : Vector2Scale(out, projectile.orbitFling);
        }
        if (projectile.isPetal)
        {
            // Slow to a hover, then dart once at where the ship is.
            const float before = projectile.petalTimer;
            projectile.petalTimer += deltaTime;
            if (projectile.petalTimer < petalStallTime)
            {
                const float speed = petalOutSpeed * std::max(0.05F, 1.0F - projectile.petalTimer / petalStallTime);
                projectile.velocity = Vector2Scale(Vector2Normalize(projectile.velocity), speed);
            }
            else if (before < petalStallTime)
            {
                projectile.velocity = Vector2Scale(
                    Vector2Normalize(Vector2Subtract(game.run.player.position, projectile.position)), petalDartSpeed);
            }
        }
        projectile.velocity = Vector2Add(projectile.velocity, Vector2Scale(projectile.accel, deltaTime));
        projectile.position = Vector2Add(projectile.position,
                                         Vector2Scale(projectile.velocity, deltaTime * frameScale));
        applyWormholeTransit(game, projectile.position, projectile.velocity, projectile.radius);

        if (Vector2Distance(projectile.position, game.run.player.position) > entityDespawnRadius)
        {
            projectile.active = false;
            continue;
        }

        for (auto& asteroid : game.run.asteroids)
        {
            if (!asteroid.active || !CheckCollisionCircles(projectile.position, projectile.radius,
                                                           asteroid.position, asteroid.radius))
            {
                continue;
            }

            if (!projectile.fromPlayer && projectile.ricochetRemaining > 0)
            {
                const Vector2 normal =
                    Vector2Normalize(Vector2Subtract(projectile.position, asteroid.position));
                const float speed = Vector2Length(projectile.velocity);
                const Vector2 incomingDir = Vector2Normalize(projectile.velocity);
                const float dot = Vector2DotProduct(incomingDir, normal);
                const Vector2 reflected =
                    Vector2Subtract(incomingDir, Vector2Scale(normal, 2.0F * dot));
                projectile.velocity = Vector2Scale(reflected, speed);
                projectile.ricochetRemaining--;
                break;
            }

            asteroid.active = false;
            breakAsteroid(game, asteroid);

            if (projectile.fromPlayer)
            {
                projectile.active = false;
                break;
            }
        }

        for (size_t j = 0; j < game.run.enemies.size(); j++)
        {
            auto& enemy = game.run.enemies.at(j);
            if (!projectile.active || !enemy.active || enemy.phased || projectile.relayTargetId != 0 ||
                (projectile.passesEnemies && !projectile.fromPlayer) ||
                (projectile.ricochetLastHit != 0 && enemy.id == projectile.ricochetLastHit) ||
                !CheckCollisionCircles(projectile.position, projectile.radius, enemy.position,
                                       enemyCollisionRadius(enemy)))
            {
                continue;
            }

            if (projectile.fromPlayer)
            {
                damageEnemy(game, j, projectile.damage);
                recordDamage(game, DamageSource::Homing, projectile.damage);
                // damageEnemy can spawn enemies (Hive Node), invalidating `enemy`.
                const Enemy& hit = game.run.enemies.at(j);
                if (!hit.active)
                {
                    recordWeaponKill(game, WeaponType::Homing);
                }
                if (projectile.ricochetRemaining > 0)
                {

                    if (const auto next =
                            nearestEnemyExcluding(game, projectile.position, hit.position);
                        next.has_value())
                    {
                        projectile.velocity = Vector2Scale(
                            Vector2Normalize(Vector2Subtract(*next, projectile.position)),
                            homingProjSpeed * 1.5F);
                        projectile.ricochetRemaining--;
                    }
                    // No bounce target: keep flying, past the enemy just hit.
                    projectile.ricochetLastHit = hit.id;
                }
                else if (projectile.ricochetLastHit != 0)
                {
                    // Bounces spent: stop homing and fly straight on through until despawn.
                    projectile.ricochetLastHit = hit.id;
                    projectile.homing = false;
                }
                else
                {
                    projectile.active = false;
                }
                break;
            }

            enemy.active = false;
        }

        if (projectile.fromPlayer)
        {
            for (size_t j = 0; j < game.run.eliteHazards.size(); j++)
            {
                auto& hazard = game.run.eliteHazards.at(j);
                if (!projectile.active || !hazard.active ||
                    !CheckCollisionCircles(projectile.position, projectile.radius, hazard.position,
                                           EliteHazardConstants::radius))
                {
                    continue;
                }

                projectile.active = false;
                damageEliteHazard(game, j, projectile.damage);
                recordDamage(game, DamageSource::Homing, projectile.damage);
                break;
            }

            for (auto& boss : game.run.bosses)
            {
                const Rectangle bossRect{.x = boss.position.x,
                                         .y = boss.position.y,
                                         .width = boss.size.x,
                                         .height = boss.size.y};
                if (!projectile.active || boss.health <= 0 ||
                    !CheckCollisionCircleRec(projectile.position, projectile.radius, bossRect))
                {
                    continue;
                }

                projectile.active = false;
                damageBoss(game, boss, projectile.damage);
                recordDamage(game, DamageSource::Homing, projectile.damage);
                break;
            }
        }

        if (projectile.active && !projectile.fromPlayer && game.run.player.health > 0 &&
            game.run.player.immunityTimer <= 0 &&
            CheckCollisionCircles(game.run.player.position, game.run.player.radius,
                                  projectile.position, projectile.radius))
        {
            if (game.run.player.shieldActive || game.run.player.dashing)
            {
                Vector2 targetPos{};
                bool foundTarget = false;
                float bestDist = 1.0e9F;
                for (auto& boss : game.run.bosses)
                {
                    if (boss.health <= 0)
                    {
                        continue;
                    }
                    const Vector2 c{.x = boss.position.x + boss.size.x / 2,
                                    .y = boss.position.y + boss.size.y / 2};
                    const float d = Vector2Distance(projectile.position, c);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        targetPos = c;
                        foundTarget = true;
                    }
                }
                for (auto& enemy : game.run.enemies)
                {
                    if (!enemy.active || enemy.phased)
                    {
                        continue;
                    }
                    const float d = Vector2Distance(projectile.position, enemy.position);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        targetPos = enemy.position;
                        foundTarget = true;
                    }
                }

                const float speed = Vector2Length(projectile.velocity) * deflectedBulletSpeedMult;
                projectile.velocity =
                    foundTarget
                        ? Vector2Scale(Vector2Normalize(Vector2Subtract(targetPos, projectile.position)),
                                      speed)
                        : Vector2Scale(projectile.velocity, -deflectedBulletSpeedMult);
                projectile.fromPlayer = true;
                projectile.huntingNewTarget = false;
                tryComboRefund(game);
                playSFX(game, game.resources.sounds.critical);
            }
            else
            {
                projectile.active = false;
                damagePlayer(game, enemyDamage(game, projectile.damage));
            }
        }
    }
}

void damageEliteHazard(Game& game, size_t index, int32_t amount, bool applyShake)
{
    auto& hazard = game.run.eliteHazards.at(index);
    hazard.health -= amount;
    spawnDamageNumber(game, hazard.position, amount);
    if (applyShake)
    {
        triggerShake(game, damageShakeIntensity(amount), damageShakeDuration);
    }
    if (hazard.health > 0)
    {
        return;
    }

    hazard.active = false;
    recordHazardKilled(game, hazard.role);
    game.run.score += eliteHazardScore;
    spawnKillExplosion(game, hazard.position, Palette::Crit, 18, 1.6F);
    playSFX(game, game.resources.sounds.explosion);
    gainNerve(game);

    spawnPickup(game, hazard.position, 0,
                GetRandomValue(0, 1) == 0 ? PickupType::Shield : PickupType::LifeOrb);
    spawnPickup(game, hazard.position, eliteHazardXpBonus, PickupType::XP);
    spawnRareBonusPickup(game, hazard.position);
    spawnRareDangerPickup(game, hazard.position);
}

void spawnEliteHazard(Game& game, EliteHazardRole role)
{

    const float halfExtentX = static_cast<float>(game.resources.screenWidth) / 2;
    const float halfExtentY = static_cast<float>(game.resources.screenHeight) / 2;
    const float orbitDist = std::min(halfExtentX, halfExtentY) * eliteHazardOrbitDistFrac;

    const auto health = static_cast<int32_t>(static_cast<float>(eliteHazardBaseHealth) *
                                             enemyHealthMult * enemyHealthScale(game));

    const auto spawnAngle = static_cast<float>(GetRandomValue(0, 359));
    const float spawnRad = spawnAngle * DEG2RAD;
    const Vector2 spawnPos =
        Vector2Add(game.run.player.position, Vector2{.x = std::cos(spawnRad) * orbitDist,
                                                     .y = std::sin(spawnRad) * orbitDist});

    game.run.eliteHazards.push_back(EliteHazard{.position = spawnPos,
                                                .angle = spawnAngle,
                                                .role = role,
                                                .health = health,
                                                .maxHealth = health,
                                                .active = true});
}

void updateEliteHazards(Game& game, float deltaTime)
{

    const float halfExtentX = static_cast<float>(game.resources.screenWidth) / 2;
    const float halfExtentY = static_cast<float>(game.resources.screenHeight) / 2;
    const float orbitDist = std::min(halfExtentX, halfExtentY) * eliteHazardOrbitDistFrac;

    game.run.eliteHazardSpawnTimer -= deltaTime;

    if (!game.sandbox && game.run.eliteHazardSpawnTimer <= 0)
    {
        if (static_cast<int>(game.run.eliteHazards.size()) < eliteHazardCap)
        {
            spawnEliteHazard(game, GetRandomValue(0, 1) == 0 ? EliteHazardRole::Warlord
                                                             : EliteHazardRole::Suppressor);
        }
        game.run.eliteHazardSpawnTimer =
            static_cast<float>(GetRandomValue(static_cast<int32_t>(eliteHazardSpawnIntervalMin),
                                              static_cast<int32_t>(eliteHazardSpawnIntervalMax)));
    }

    for (auto& hazard : game.run.eliteHazards)
    {
        if (!hazard.active)
        {
            continue;
        }

        hazard.angle += eliteHazardOrbitSpin * deltaTime;
        const float rad = hazard.angle * DEG2RAD;
        const Vector2 targetPos =
            Vector2Add(game.run.player.position,
                       Vector2{.x = std::cos(rad) * orbitDist, .y = std::sin(rad) * orbitDist});

        const float lerpT = std::clamp(eliteHazardFollowRate * deltaTime, 0.0F, 1.0F);
        hazard.position = Vector2Lerp(hazard.position, targetPos, lerpT);

        if (game.run.player.health > 0 && game.run.player.immunityTimer <= 0 &&
            CheckCollisionCircles(game.run.player.position, game.run.player.radius, hazard.position,
                                  EliteHazardConstants::radius))
        {
            damagePlayer(game, enemyDamage(game, eliteHazardContactDamage));
        }
    }
}

void spawnBlackHole(Game& game)
{
    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(150, 350));
    game.run.blackhole.position =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
    game.run.blackhole.radius = 25;
    game.run.blackhole.influenceRadius = 140;
    game.run.blackhole.active = true;
    game.run.blackhole.isKrakenPortal = false;
    game.run.blackhole.timer = static_cast<float>(GetRandomValue(60, 100)) / 10.0F;
    for (auto& dust : game.run.blackHoleDust)
    {
        dust.color = spawnBlackHoleDustParticle(game).color;
    }
}

void updateBlackHole(Game& game, float deltaTime)
{
    if (game.run.punctumTrapActive)
    {
        return;
    }

    game.run.blackhole.timer -= deltaTime;

    if ((!game.sandbox || game.sandboxNaturalSpawnEnabled) && !game.run.blackhole.active &&
        game.run.blackhole.timer <= 0)
    {
        spawnBlackHole(game);
    }
    else if (game.run.blackhole.active && game.run.blackhole.timer <= 0)
    {
        game.run.blackhole.active = false;
        game.run.blackhole.timer = static_cast<float>(GetRandomValue(50, 90)) / 10.0F;
    }

    if (game.run.blackhole.active)
    {
        const Vector2 toPlayer =
            Vector2Subtract(game.run.player.position, game.run.blackhole.position);
        const float dist = Vector2Length(toPlayer);

        if (dist > game.run.blackhole.influenceRadius)
        {
            const Vector2 drift = Vector2Scale(Vector2Normalize(toPlayer), blackHoleChaseSpeed);
            game.run.blackhole.position = Vector2Add(game.run.blackhole.position,
                                                     Vector2Scale(drift, deltaTime * frameScale));
        }
    }
}

auto spawnBlackHoleDustParticle(const Game& game) -> BlackHoleDustParticle
{
    const auto pick = [](const auto& colors)
    { return colors.at(static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(colors.size() - 1)))); };
    Color color = pick(Palette::AmbientDustColors);
    if (game.run.blackhole.isKrakenPortal)
    {
        color = pick(Palette::PunctumDustColors);
    }
    else if (currentBiome(game.run.waveNumber) == Biome::ShatteredBelt)
    {
        color = pick(Palette::ShatteredBeltDustColors);
    }
    else if (currentBiome(game.run.waveNumber) == Biome::Rustbloom)
    {
        color = pick(Palette::RustbloomDustColors);
    }

    return BlackHoleDustParticle{
        .radiusFrac = 1.0F,
        .armPeakAngle = static_cast<float>(GetRandomValue(0, blackHoleDustArmCount - 1)) /
                            static_cast<float>(blackHoleDustArmCount) * 2.0F *
                            std::numbers::pi_v<float>,
        .jitter = static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0F * 0.25F,
        .brightness = static_cast<float>(GetRandomValue(60, 100)) / 100.0F,
        .accelerating = GetRandomValue(0, 1) == 0,
        .fixedSpeed = blackHoleDustInwardSpeedMin +
                     (blackHoleDustInwardSpeedMax - blackHoleDustInwardSpeedMin) *
                         static_cast<float>(GetRandomValue(0, 1000)) / 1000.0F,
        .color = color};
}

void updateBlackHoleDust(Game& game, float deltaTime)
{
    if (!game.run.blackhole.active)
    {
        return;
    }

    for (auto& dust : game.run.blackHoleDust)
    {
        const float speed = dust.accelerating
                                ? blackHoleDustInwardSpeedMin +
                                      (blackHoleDustInwardSpeedMax - blackHoleDustInwardSpeedMin) *
                                          (1.0F - dust.radiusFrac)
                                : dust.fixedSpeed;
        dust.radiusFrac -= speed * deltaTime;

        if (dust.radiusFrac <= blackHoleDustCoreFraction)
        {
            dust = spawnBlackHoleDustParticle(game);
        }
    }
}

void spawnWormholePair(Game& game)
{
    const float angleA = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto distA = static_cast<float>(GetRandomValue(300, 600));
    const float angleB = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto distB = static_cast<float>(GetRandomValue(300, 600));

    game.run.wormhole.positionA =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angleA) * distA, .y = std::sin(angleA) * distA});
    game.run.wormhole.positionB =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angleB) * distB, .y = std::sin(angleB) * distB});
    game.run.wormhole.facingA = static_cast<WormholeFacing>(GetRandomValue(0, 3));
    game.run.wormhole.facingB = static_cast<WormholeFacing>(GetRandomValue(0, 3));
    game.run.wormhole.radius = wormholeRadius;
    game.run.wormhole.active = true;
    game.run.wormhole.timer = wormholeLifetime;
}

void updateWormhole(Game& game, float deltaTime)
{
    game.run.wormhole.timer -= deltaTime;

    if (!game.sandbox && !game.run.wormhole.active && game.run.wormhole.timer <= 0)
    {
        spawnWormholePair(game);
    }
    else if (game.run.wormhole.active && game.run.wormhole.timer <= 0)
    {
        game.run.wormhole.active = false;
        game.run.wormhole.timer =
            static_cast<float>(GetRandomValue(static_cast<int32_t>(wormholeSpawnCooldownMin),
                                              static_cast<int32_t>(wormholeSpawnCooldownMax)));
    }
}

auto applyWormholeTransit(const Game& game, Vector2& position, Vector2& velocity,
                          float entityRadius) -> bool
{
    if (!game.run.wormhole.active)
    {
        return false;
    }

    const bool atA =
        Vector2Distance(position, game.run.wormhole.positionA) <= game.run.wormhole.radius;
    const bool atB =
        !atA && Vector2Distance(position, game.run.wormhole.positionB) <= game.run.wormhole.radius;
    if (!atA && !atB)
    {
        return false;
    }

    const WormholeFacing entryFacing = atA ? game.run.wormhole.facingA : game.run.wormhole.facingB;
    const WormholeFacing exitFacing = atA ? game.run.wormhole.facingB : game.run.wormhole.facingA;
    const Vector2 exitPoint = atA ? game.run.wormhole.positionB : game.run.wormhole.positionA;

    const float deltaDegrees =
        (static_cast<float>(exitFacing) - static_cast<float>(entryFacing)) * 90.0F + 180.0F;
    velocity = Vector2Rotate(velocity, deltaDegrees * DEG2RAD);

    const Vector2 exitDir = wormholeFacingVector(exitFacing);

    position =
        Vector2Add(exitPoint, Vector2Scale(exitDir, game.run.wormhole.radius + entityRadius + 4));

    return true;
}

auto wormholeBentBeamSegments(const Game& game, Vector2 start,
                              Vector2 end) -> std::vector<std::pair<Vector2, Vector2>>
{
    if (!game.run.wormhole.active)
    {
        return {{start, end}};
    }

    const auto closestOnSegment = [&](Vector2 point) -> Vector2
    {
        const Vector2 seg = Vector2Subtract(end, start);
        const float lenSq = Vector2LengthSqr(seg);
        if (lenSq <= 0.0F)
        {
            return start;
        }
        const float t =
            std::clamp(Vector2DotProduct(Vector2Subtract(point, start), seg) / lenSq, 0.0F, 1.0F);
        return Vector2Add(start, Vector2Scale(seg, t));
    };

    const Vector2 closestA = closestOnSegment(game.run.wormhole.positionA);
    const Vector2 closestB = closestOnSegment(game.run.wormhole.positionB);
    const bool hitsA =
        Vector2Distance(closestA, game.run.wormhole.positionA) <= game.run.wormhole.radius;
    const bool hitsB =
        Vector2Distance(closestB, game.run.wormhole.positionB) <= game.run.wormhole.radius;

    if (!hitsA && !hitsB)
    {
        return {{start, end}};
    }

    const bool useA =
        hitsA && (!hitsB || Vector2Distance(start, closestA) <= Vector2Distance(start, closestB));
    const Vector2 entryPoint = useA ? closestA : closestB;
    const WormholeFacing entryFacing = useA ? game.run.wormhole.facingA : game.run.wormhole.facingB;
    const WormholeFacing exitFacing = useA ? game.run.wormhole.facingB : game.run.wormhole.facingA;
    const Vector2 exitCenter = useA ? game.run.wormhole.positionB : game.run.wormhole.positionA;

    const float deltaDegrees =
        (static_cast<float>(exitFacing) - static_cast<float>(entryFacing)) * 90.0F + 180.0F;
    const Vector2 entryDir = Vector2Normalize(Vector2Subtract(end, start));
    const Vector2 exitVelDir = Vector2Rotate(entryDir, deltaDegrees * DEG2RAD);

    const Vector2 exitFacingDir = wormholeFacingVector(exitFacing);
    const Vector2 exitPoint =
        Vector2Add(exitCenter, Vector2Scale(exitFacingDir, game.run.wormhole.radius + 4));

    const float remainingLen =
        std::max(0.0F, Vector2Distance(start, end) - Vector2Distance(start, entryPoint));
    const Vector2 newEnd = Vector2Add(exitPoint, Vector2Scale(exitVelDir, remainingLen));

    return {{start, entryPoint}, {exitPoint, newEnd}};
}
