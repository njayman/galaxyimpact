#include "sandbox.hpp"

#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "entities/ship.hpp"
#include "raylib.h"
#include "raymath.h"
#include "update.hpp"
#include <cmath>

void enterSandbox(Game& game)
{
    resetRun(game);
    game.sandbox = true;
    game.state = GameState::GAMEPLAY;
    game.run.blackhole.active = false;
    game.sandboxDeathEnabled = false;
    game.sandboxBossAttackIndex = 0;
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
        game.run.weapons = {Weapon{.type = WeaponType::Forward, .level = 1}};
        game.run.skillLevels.fill(0);
        game.run.skillLevels.at(static_cast<size_t>(SkillType::ForwardShot)) = 1;
    }

    if (IsKeyPressed(KEY_G))
    {
        game.sandboxDeathEnabled = !game.sandboxDeathEnabled;
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
