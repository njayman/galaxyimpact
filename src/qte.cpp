#include "update.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>

// --- Boss QTE cutscene ---------------------------------------------------------
//
// A boss owns the cutscene through game.run.qteBossId. While it runs the player
// has no direct control, the HUD shows only the current prompt and the world
// crawls at qteSlowMult. The owning boss poses prompts (qtePromptForThreat),
// polls qteReadInput, plays the ship's reaction with qteStartAction and calls
// bossQteEnd when its sequence is done or the player fails a prompt.
// -------------------------------------------------------------------------------

auto bossQteActive(const Game& game) -> bool { return game.run.qteBossId >= 0; }

auto bossQteSlowdown(const Game& game) -> float
{
    if (!bossQteActive(game) || game.run.player.grabbed || game.run.player.launched)
    {
        return 1.0F;
    }
    return UpdateConstants::qteSlowMult;
}

void clearQtePlayerPose(Player& player)
{
    player.velocity = Vector2{};
    player.dashing = false;
    player.shieldActive = false;
    player.shieldDashing = false;
    player.comboActive = false;
    player.nerveCharging = false;
}

void bossQteBegin(Game& game, int32_t bossId)
{
    game.run.qteBossId = bossId;
    game.run.qtePrompt = QtePrompt::None;
    game.run.qteTimer = 0;
    game.run.qteTimerMax = 0;
    game.run.qteActionTimer = 0;
    game.run.qteActionVelocity = Vector2{};
    game.run.qteStickDir = 0;
    clearQtePlayerPose(game.run.player);
}

void bossQteEnd(Game& game)
{
    game.run.qteBossId = -1;
    game.run.qtePrompt = QtePrompt::None;
    game.run.qteTimer = 0;
    game.run.qteTimerMax = 0;
    game.run.qteActionTimer = 0;
    game.run.qteActionVelocity = Vector2{};
    clearQtePlayerPose(game.run.player);
}

auto qteDirVector(QtePrompt prompt) -> Vector2
{
    switch (prompt)
    {
    case QtePrompt::Up:
        return Vector2{.x = 0, .y = -1};
    case QtePrompt::Left:
        return Vector2{.x = -1, .y = 0};
    case QtePrompt::Right:
        return Vector2{.x = 1, .y = 0};
    case QtePrompt::Down:
        return Vector2{.x = 0, .y = 1};
    default:
        return Vector2{};
    }
}

auto qteCardinal(Vector2 dir) -> QtePrompt
{
    if (std::fabs(dir.x) >= std::fabs(dir.y))
    {
        return dir.x > 0 ? QtePrompt::Right : QtePrompt::Left;
    }
    return dir.y > 0 ? QtePrompt::Down : QtePrompt::Up;
}

// A random side step perpendicular to the threat's travel direction.
auto qteSidestep(Vector2 threatDir) -> Vector2
{
    const float side = GetRandomValue(0, 1) == 0 ? 1.0F : -1.0F;
    return Vector2{.x = -threatDir.y * side, .y = threatDir.x * side};
}

auto qtePromptForThreat(const Game& game, Vector2 threatDir) -> QtePrompt
{
    const int32_t pick = GetRandomValue(0, 99);
    int32_t edge = UpdateConstants::qteParryChance;
    if (pick < edge)
    {
        return QtePrompt::Parry;
    }
    edge += UpdateConstants::qteDashChance;
    if (pick < edge)
    {
        return QtePrompt::Dash;
    }
    edge += UpdateConstants::qteShieldChance;
    if (pick < edge)
    {
        return QtePrompt::Shield;
    }
    // Full nerve (built up by answering prompts) unlocks the burst finisher.
    if (game.run.player.nerve >= UpdateConstants::nerveMax && GetRandomValue(0, 1) == 0)
    {
        return QtePrompt::Nerve;
    }
    return qteCardinal(qteSidestep(threatDir));
}

// 1 = the asked input was pressed this frame, -1 = a different input was, 0 = nothing yet.
auto qteReadInput(Game& game, QtePrompt prompt) -> int32_t
{
    // Touch: flicking the move stick past 0.6 counts as one directional press.
    QtePrompt stickDir = QtePrompt::None;
    if (game.touch.visible && Vector2Length(game.touch.moveStick.axis) > 0.6F)
    {
        stickDir = qteCardinal(game.touch.moveStick.axis);
    }
    const bool stickFlick =
        stickDir != QtePrompt::None && static_cast<int32_t>(stickDir) != game.run.qteStickDir;
    game.run.qteStickDir = static_cast<int32_t>(stickDir);

    const auto dirPressed = [&](QtePrompt dir, int32_t key, int32_t arrow)
    { return IsKeyPressed(key) || IsKeyPressed(arrow) || (stickFlick && stickDir == dir); };
    const bool up = dirPressed(QtePrompt::Up, KEY_W, KEY_UP);
    const bool left = dirPressed(QtePrompt::Left, KEY_A, KEY_LEFT);
    const bool right = dirPressed(QtePrompt::Right, KEY_D, KEY_RIGHT);
    const bool down = dirPressed(QtePrompt::Down, KEY_S, KEY_DOWN);
    const bool shield = shieldPressed(game);
    const bool dash = dashPressed(game);
    const bool nerve = nervePressed(game);
    const bool combo = (!game.touch.visible && IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) ||
                       (shield && dashDown(game)) || (dash && shieldDown(game));
    const bool directional = up || left || right || down;

    if (prompt == QtePrompt::Parry)
    {
        if (combo)
        {
            return 1;
        }
        // A lone shield or dash press may be the first half of the combo.
        return directional || nerve ? -1 : 0;
    }

    bool asked = false;
    switch (prompt)
    {
    case QtePrompt::Up:
        asked = up;
        break;
    case QtePrompt::Left:
        asked = left;
        break;
    case QtePrompt::Right:
        asked = right;
        break;
    case QtePrompt::Down:
        asked = down;
        break;
    case QtePrompt::Shield:
        asked = shield;
        break;
    case QtePrompt::Dash:
        asked = dash;
        break;
    case QtePrompt::Nerve:
        asked = nerve;
        break;
    default:
        return 0;
    }
    if (asked)
    {
        return 1;
    }
    return directional || shield || dash || nerve ? -1 : 0;
}

// The ship acts out the answered prompt; nerve builds with every success.
void qteStartAction(Game& game, QtePrompt prompt, Vector2 threatDir)
{
    auto& player = game.run.player;
    Vector2 move{};
    switch (prompt)
    {
    case QtePrompt::Up:
    case QtePrompt::Left:
    case QtePrompt::Right:
    case QtePrompt::Down:
        move = Vector2Scale(qteDirVector(prompt), UpdateConstants::qteDodgeDistance);
        break;
    case QtePrompt::Dash:
        move = Vector2Scale(qteSidestep(threatDir), UpdateConstants::qteDashDistance);
        player.dashing = true;
        break;
    case QtePrompt::Shield:
        player.shieldActive = true;
        break;
    case QtePrompt::Parry:
        // Lunge into the threat (threatDir points from it toward the player).
        move = Vector2Scale(threatDir, -UpdateConstants::qteParryLunge);
        player.dashing = true;
        player.shieldActive = true;
        player.shieldDashing = true;
        break;
    case QtePrompt::Nerve:
        player.nerve = 0;
        playSFX(game, game.resources.sounds.nerveRelease);
        break;
    case QtePrompt::None:
        break;
    }
    if (prompt != QtePrompt::Nerve)
    {
        player.nerve = std::min(UpdateConstants::nerveMax, player.nerve + UpdateConstants::qteNerveGain);
    }
    game.run.qteActionTimer = UpdateConstants::qteActionDuration;
    game.run.qteActionVelocity = Vector2Scale(move, 1.0F / UpdateConstants::qteActionDuration);
}

// Runs on real (unslowed) time from updateGameplay.
void updateBossQte(Game& game, float deltaTime)
{
    if (!bossQteActive(game))
    {
        return;
    }
    auto& run = game.run;
    if (run.qteTimer > 0)
    {
        run.qteTimer -= deltaTime;
    }
    if (run.qteActionTimer > 0)
    {
        const float step = std::min(deltaTime, run.qteActionTimer);
        run.player.position =
            Vector2Add(run.player.position, Vector2Scale(run.qteActionVelocity, step));
        run.qteActionTimer -= deltaTime;
        if (run.qteActionTimer <= 0)
        {
            clearQtePlayerPose(run.player);
        }
    }

    // Dispatch to the boss running the cutscene (add other bosses' QTEs here).
    if (Boss* core = findBeltbreakerById(game, run.qteBossId); core != nullptr)
    {
        updateBeltbreakerPlateRush(game, *core, deltaTime);
        return;
    }
    // Owner died mid-cutscene.
    bossQteEnd(game);
    for (auto& boss : game.run.bosses)
    {
        if (boss.isBeltbreakerPlate && boss.plateRushPhase != 0)
        {
            sendRushPlateHome(boss);
        }
    }
}
