#include "update.hpp"
#include "update_constants.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>

// --- Boss QTE cutscene ---------------------------------------------------------
//
// A boss owns the cutscene through game.run.qteBossId. While it runs the player
// has no direct control, the HUD shows only the current prompt and the world
// crawls at qteSlowMult. The owning boss poses prompts (qtePromptForThreat),
// polls qteReadInput, plays the ship's reaction with qteStartAction and calls
// bossQteEnd when its sequence is done or the player fails a prompt. A Mash
// prompt counts each Nerve press (SPACE / NERVE button) toward qteMashTarget
// instead, with no wrong answer; the boss fails it when its timer runs out first. bossQteCanStart gates every
// trigger, including the shared minimum gap between Boss QTEs.
// -------------------------------------------------------------------------------

auto bossQteActive(const Game& game) -> bool
{
    return game.run.qteBossId >= 0 || game.run.qteEnemyId >= 0;
}

// No cutscene running or blocking one; the 20 s gap is checked separately.
auto qteSlotFree(const Game& game) -> bool
{
    const auto& run = game.run;
    return !bossQteActive(game) && run.player.health > 0 && !run.player.grabbed &&
           !run.player.launched && run.bossCutscenePhase == 0;
}

auto bossQteCanStart(const Game& game, QteKind kind) -> bool
{
    return qteSlotFree(game) && game.run.qteCooldowns.at(static_cast<size_t>(kind)) <= 0;
}

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

void bossQteBegin(Game& game, int32_t bossId, QteKind kind)
{
    game.run.qteBossId = bossId;
    game.run.qteKind = static_cast<int32_t>(kind);
    game.run.qtePrompt = QtePrompt::None;
    game.run.qteTimer = 0;
    game.run.qteTimerMax = 0;
    game.run.qteActionTimer = 0;
    game.run.qteActionVelocity = Vector2{};
    game.run.qteStickDir = 0;
    game.run.qteMashCount = 0;
    game.run.qteMashTarget = 0;
    game.run.qteChainLive = false;
    game.run.qteFinisherCharge = 0;
    clearQtePlayerPose(game.run.player);
}

void bossQteEnd(Game& game)
{
    if (game.run.qteBossId >= 0 && game.run.qteKind >= 0)
    {
        game.run.qteCooldowns.at(static_cast<size_t>(game.run.qteKind)) = UpdateConstants::qteMinGap;
    }
    game.run.qteKind = -1;
    game.run.qteBossId = -1;
    game.run.qteEnemyId = -1;
    game.run.qteFinisherPhase = 0;
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

auto qtePromptForThreat(const Game& game, Vector2 threatDir, int32_t parryChance) -> QtePrompt
{
    const int32_t pick = GetRandomValue(0, 99);
    int32_t edge = parryChance;
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

auto qteReadRawInput(Game& game, QtePrompt prompt) -> int32_t;

// 1 = the asked input was pressed this frame, -1 = a different input was, 0 = nothing yet.
// Until the chain is live (first correct answer) a wrong press reads as 0: the player is
// usually mid-fight on other controls when a QTE pops, so only the timer can fail it.
auto qteReadInput(Game& game, QtePrompt prompt) -> int32_t
{
    const int32_t answer = qteReadRawInput(game, prompt);
    return answer < 0 && !game.run.qteChainLive ? 0 : answer;
}

auto qteReadRawInput(Game& game, QtePrompt prompt) -> int32_t
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

    if (prompt == QtePrompt::Mash)
    {
        return nerve ? 1 : 0; // other inputs are ignored, never wrong
    }
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
    case QtePrompt::Mash:
    case QtePrompt::None:
        break;
    }
    if (prompt != QtePrompt::Nerve)
    {
        player.nerve = std::min(UpdateConstants::nerveMax, player.nerve + UpdateConstants::qteNerveGain);
    }
    game.run.qteChainLive = true; // from here a wrong press breaks the chain
    game.run.qteFinisherCharge += prompt == QtePrompt::Parry   ? UpdateConstants::qteFinisherChargeParry
                                  : prompt == QtePrompt::Nerve ? UpdateConstants::qteFinisherChargeNerve
                                                               : UpdateConstants::qteFinisherChargeAnswer;
    game.run.qteActionTimer = UpdateConstants::qteActionDuration;
    game.run.qteActionVelocity = Vector2Scale(move, 1.0F / UpdateConstants::qteActionDuration);
}

// The finisher, on real time: lock the controls, turn to face the target, gather a charge,
// fire. Returns true on the frame the shot lands; the caller deals its damage and ends the QTE.
void qteBeginFinisher(Game& game)
{
    game.run.qtePrompt = QtePrompt::None;
    game.run.qteTimerMax = 0;
    game.run.qteAimDir = aimAtMouse(game); // turn from wherever the ship faced
    game.run.qteFinisherPhase = 1;
    game.run.qteFinisherTimer = UpdateConstants::qteFinisherTurn;
}

auto qteUpdateFinisher(Game& game, Vector2 target, float deltaTime) -> bool
{
    auto& run = game.run;
    run.qteFinisherTarget = target;
    const Vector2 toTarget = Vector2Normalize(Vector2Subtract(target, run.player.position));
    run.qteFinisherTimer -= deltaTime;
    if (run.qteFinisherPhase == 1)
    {
        run.qteAimDir = Vector2Normalize(Vector2Lerp(run.qteAimDir, toTarget, std::min(1.0F, deltaTime * 16.0F)));
        if (run.qteFinisherTimer <= 0)
        {
            run.qteFinisherPhase = 2;
            run.qteFinisherTimer = UpdateConstants::qteFinisherCharge;
            playSFX(game, game.resources.sounds.nerveCharge);
        }
        return false;
    }
    run.qteAimDir = toTarget;
    if (run.qteFinisherPhase == 2)
    {
        if (run.qteFinisherTimer <= 0)
        {
            run.qteFinisherPhase = 3;
            run.qteFinisherTimer = UpdateConstants::qteFinisherShot;
            playSFX(game, game.resources.sounds.beamFire);
            triggerShake(game, 3, 0.1F);
        }
        return false;
    }
    if (run.qteFinisherTimer > 0)
    {
        return false;
    }
    triggerShake(game, 8, 0.3F);
    return true;
}

// Runs on real (unslowed) time from updateGameplay.
void updateBossQte(Game& game, float deltaTime)
{
    auto& run = game.run;
    if (!bossQteActive(game))
    {
        for (float& cooldown : run.qteCooldowns)
        {
            cooldown = std::max(0.0F, cooldown - deltaTime);
        }
        return;
    }
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

    if (run.qteEnemyId >= 0)
    {
        updateSniperQte(game, deltaTime);
        return;
    }
    Boss* owner = findBossById(game, run.qteBossId);
    if (owner != nullptr && run.qteFinisherPhase > 0)
    {
        // A clean chain: the ship turns on the boss and fires everything it charged.
        const Vector2 center{.x = owner->position.x + owner->size.x / 2, .y = owner->position.y + owner->size.y / 2};
        if (qteUpdateFinisher(game, center, deltaTime))
        {
            if (static_cast<QteKind>(run.qteKind) == QteKind::PlateRush)
            {
                owner->beltbreakerShielded = false; // the shot is what shatters the shield
            }
            spawnKillExplosion(game, center, Palette::Crit, 40, 2.0F);
            playSFX(game, game.resources.sounds.explosion);
            if (owner->isBanished)
            {
                damageBanishedEyeBurst(game, *owner, banishedEyeBurstDamageDivisor); // only bursts hurt it
            }
            else
            {
                // The Kraken's core takes the shot, like a staggered tentacle's hit.
                damageBoss(game, *owner,
                           std::max(1, static_cast<int32_t>(static_cast<float>(owner->maxHealth) * run.qteFinisherCharge)),
                           true, false, owner->isKraken);
            }
            if (owner->health > 0)
            {
                endBossQteClean(game, *owner);
            }
            else
            {
                bossQteEnd(game);
            }
        }
        return;
    }
    // Dispatch to the boss running the cutscene (add other bosses' QTEs here).
    if (owner != nullptr)
    {
        switch (static_cast<QteKind>(run.qteKind))
        {
        case QteKind::PlateRush:
            updateBeltbreakerPlateRush(game, *owner, deltaTime);
            return;
        case QteKind::Constrict:
            updateWreckwormConstrict(game, *owner, deltaTime);
            return;
        case QteKind::Rush:
            updateRushQte(game, *owner, deltaTime);
            return;
        case QteKind::Swallowed:
            updateSlagmawSwallowed(game, *owner, deltaTime);
            return;
        case QteKind::DraggedUnder:
            updateKrakenDraggedUnder(game, *owner, deltaTime);
            return;
        case QteKind::Juggled:
            updateBanishedJuggled(game, *owner, deltaTime);
            return;
        }
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
