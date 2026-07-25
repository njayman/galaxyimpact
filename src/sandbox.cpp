#include "sandbox.hpp"

#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "raylib.h"
#include "raymath.h"
#include "update.hpp"
#include <cmath>

void enterSandbox(Game& game)
{
    resetRun(game);
    game.sandbox = true;
    game.state = GameState::GAMEPLAY;
    game.blackhole.active = false;
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
        const Vector2 pos = Vector2Add(game.player.position, Vector2{.x = std::cos(angle) * dist,
                                                                     .y = std::sin(angle) * dist});
        spawnEnemyAt(game, game.sandboxKindIndex, pos);
    }

    if (IsKeyPressed(KEY_B))
    {
        spawnBoss(game);
    }

    if (IsKeyPressed(KEY_K))
    {
        game.enemies.clear();
        game.asteroids.clear();
        game.bossProjectiles.clear();
        game.mines.clear();
        game.bosses.clear();
        game.bossDeathShockwaves.clear();
    }

    if (IsKeyPressed(KEY_L))
    {
        startLevelUp(game);
    }

    if (IsKeyPressed(KEY_H))
    {
        game.player.health = game.player.maxHealth;
        game.player.shieldStacks = playerConstants::maxShieldStack;
        game.player.nerve = UpdateConstants::nerveMax;
    }

    if (IsKeyPressed(KEY_R))
    {
        game.weapons = {Weapon{.type = WeaponType::Forward, .level = 1}};
        game.skillLevels.fill(0);
        game.skillLevels.at(static_cast<size_t>(SkillType::ForwardShot)) = 1;
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

    if (IsKeyPressed(KEY_O) && !game.bosses.empty())
    {
        Boss* nearest = &game.bosses.front();
        float bestDist = Vector2Distance(game.player.position, nearest->position);
        for (auto& boss : game.bosses)
        {
            const float dist = Vector2Distance(game.player.position, boss.position);
            if (dist < bestDist)
            {
                bestDist = dist;
                nearest = &boss;
            }
        }
        forceBossAttack(game, *nearest, static_cast<BossAttack>(game.sandboxBossAttackIndex));
    }
}
