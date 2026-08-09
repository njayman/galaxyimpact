#include "sandbox.hpp"

#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "entities/ship.hpp"
#include "raylib.h"
#include "raymath.h"
#include "update.hpp"
#include <array>
#include <cmath>

namespace
{
// Index 0 is XP (sandbox-only extra, not part of the shared catalog since it's never offered as a
// deliberate pickup/level-up choice); indices 1.. mirror pickupCatalog (include/entities/item.hpp),
// the single shared list also used by the level-up "Pickup" choice and the post-cap reward pool.
constexpr size_t sandboxPickupCount = pickupCatalog.size() + 1;

auto sandboxPickupOption(int32_t index) -> PickupCatalogEntry
{
    if (index == 0)
    {
        return PickupCatalogEntry{PickupType::XP, ElementType::Static, ElementMechanism::Infusion,
                                  "XP"};
    }
    return pickupCatalog.at(static_cast<size_t>(index) - 1);
}
}

void enterSandbox(Game& game)
{
    resetRun(game);
    game.sandbox = true;
    game.state = GameState::GAMEPLAY;
    game.run.blackhole.active = false;
    game.sandboxDeathEnabled = false;
    game.sandboxBossAttackIndex = 0;
    game.sandboxPickupIndex = 0;
}

void updateSandboxInput(Game& game)
{
    if (IsKeyPressed(KEY_RIGHT_BRACKET))
    {
        game.sandboxKindIndex = (game.sandboxKindIndex + 1) % static_cast<int>(enemyKinds.size());
    }
    if (IsKeyPressed(KEY_LEFT_BRACKET))
    {
        game.sandboxKindIndex = (game.sandboxKindIndex - 1 + static_cast<int>(enemyKinds.size())) %
                                static_cast<int>(enemyKinds.size());
    }

    if (IsKeyPressed(KEY_E))
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto dist = static_cast<float>(GetRandomValue(200, 320));
        const Vector2 pos =
            Vector2Add(game.run.player.position,
                       Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
        spawnEnemyAt(game, game.sandboxKindIndex, pos);
    }

    if (IsKeyPressed(KEY_B))
    {
        if (IsKeyDown(KEY_LEFT_SHIFT))
        {
            spawnMiniboss(game);
        }
        else
        {
            spawnBoss(game);
        }
    }

    if (IsKeyPressed(KEY_Y))
    {
        spawnSwarmBoss(game);
    }

    if (IsKeyPressed(KEY_V))
    {
        // Signature bosses aren't on the generic B/Shift+B spawn path, and only exist at
        // specific waves in play - this lets tuning use the sandbox's wave (-/+ ) to pick any
        // appearance of a signature boss (e.g. wave 10/20/25 for the Beltbreaker) and spawn it
        // on demand regardless of the wave-progression timer.
        if (currentBiome(game.run.waveNumber) == Biome::ShatteredBelt)
        {
            spawnBeltbreaker(game, game.run.waveNumber);
        }
        else if (currentBiome(game.run.waveNumber) == Biome::Rustbloom)
        {
            spawnWreckworm(game, game.run.waveNumber);
        }
    }

    if (IsKeyPressed(KEY_K))
    {
        game.run.enemies.clear();
        game.run.eliteHazards.clear();
        game.run.asteroids.clear();
        game.run.bossProjectiles.clear();
        game.run.mines.clear();
        game.run.bosses.clear();
        game.run.bossDeathShockwaves.clear();
    }

    if (IsKeyPressed(KEY_L))
    {
        startLevelUp(game);
    }

    if (IsKeyPressed(KEY_H))
    {
        game.run.player.health = game.run.player.maxHealth;
        game.run.player.shieldStacks = currentShip(game).maxShieldStacks;
        game.run.player.nerve = UpdateConstants::nerveMax;
    }

    if (IsKeyPressed(KEY_R))
    {
        const ShipDef& ship = currentShip(game);
        game.run.weapons = {Weapon{.type = ship.defaultWeapon, .level = 1}};
        game.run.skillLevels.fill(0);
        game.run.skillLevels.at(
            static_cast<size_t>(weaponGrantSkill.at(static_cast<size_t>(ship.defaultWeapon)))) = 1;
    }

    if (IsKeyPressed(KEY_G))
    {
        game.sandboxDeathEnabled = !game.sandboxDeathEnabled;
    }

    if (IsKeyPressed(KEY_I))
    {
        const auto count = static_cast<int32_t>(ShipClass::Count);
        const int32_t dir = IsKeyDown(KEY_LEFT_SHIFT) ? -1 : 1;
        game.resources.settings.shipIndex =
            (game.resources.settings.shipIndex + dir + count) % count;
        resetRun(game);
    }

    if (IsKeyPressed(KEY_PERIOD))
    {
        game.sandboxBossAttackIndex = (game.sandboxBossAttackIndex + 1) % bossAttackCount;
    }
    if (IsKeyPressed(KEY_COMMA))
    {
        game.sandboxBossAttackIndex =
            (game.sandboxBossAttackIndex - 1 + bossAttackCount) % bossAttackCount;
    }

    if (IsKeyPressed(KEY_EQUAL))
    {
        game.run.waveNumber++;
    }
    if (IsKeyPressed(KEY_MINUS) && game.run.waveNumber > 1)
    {
        game.run.waveNumber--;
    }

    if (IsKeyPressed(KEY_N))
    {
        spawnBlackHole(game);
    }

    if (IsKeyPressed(KEY_M))
    {
        spawnWormholePair(game);
    }

    if (IsKeyPressed(KEY_U))
    {
        spawnEliteHazard(game, IsKeyDown(KEY_LEFT_SHIFT) ? EliteHazardRole::Suppressor
                                                         : EliteHazardRole::Warlord);
    }

    if (IsKeyPressed(KEY_APOSTROPHE))
    {
        game.sandboxPickupIndex =
            (game.sandboxPickupIndex + 1) % static_cast<int>(sandboxPickupCount);
    }
    if (IsKeyPressed(KEY_SEMICOLON))
    {
        game.sandboxPickupIndex =
            (game.sandboxPickupIndex - 1 + static_cast<int>(sandboxPickupCount)) %
            static_cast<int>(sandboxPickupCount);
    }

    if (IsKeyPressed(KEY_P))
    {
        const auto option = sandboxPickupOption(game.sandboxPickupIndex);
        spawnPickup(game, mouseWorldPos(game), 0, option.type, option.element, option.mechanism);
    }

    if (IsKeyPressed(KEY_O) && !game.run.bosses.empty())
    {
        Boss* nearest = &game.run.bosses.front();
        float bestDist = Vector2Distance(game.run.player.position, nearest->position);
        for (auto& boss : game.run.bosses)
        {
            const float dist = Vector2Distance(game.run.player.position, boss.position);
            if (dist < bestDist)
            {
                bestDist = dist;
                nearest = &boss;
            }
        }
        forceBossAttack(game, *nearest, static_cast<BossAttack>(game.sandboxBossAttackIndex));
    }
}

auto sandboxPickupName(int32_t index) -> std::string_view
{
    return sandboxPickupOption(index).name;
}
