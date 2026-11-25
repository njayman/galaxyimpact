#include "tutorial.hpp"

#include "entities/boss.hpp"
#include "entities/enemy.hpp"
#include "raymath.h"
#include "settings.hpp"
#include "update.hpp"

#include <algorithm>
#include <array>

namespace
{
constexpr float moveDistance = 260;  // how far the Move step wants the ship to travel
constexpr float aheadDistance = 260; // scripted drones spawn this far in front of the ship
constexpr float spawnGap = 1.0F;     // seconds between Level step drones
constexpr float qteRetryGap = 1.5F;  // seconds before a missed Boss QTE is forced again
constexpr float invulnerable = 0.5F; // refreshed every frame: the tutorial ship cannot be hurt
constexpr int32_t drifterKind = 0;   // enemyKinds[0]
constexpr float beltbreakerHealthFrac = 0.35F; // the tutorial miniboss is an easy Beltbreaker

constexpr std::array<TutorialStep, 11> order{
    TutorialStep::Welcome, TutorialStep::Move,   TutorialStep::Shoot, TutorialStep::Level,
    TutorialStep::Upgrade, TutorialStep::Shield, TutorialStep::Dash,  TutorialStep::Parry,
    TutorialStep::Qte,     TutorialStep::Boss,   TutorialStep::Done};

auto targetAlive(const Game& game) -> bool
{
    return std::any_of(game.run.enemies.begin(), game.run.enemies.end(),
                       [&](const Enemy& e) { return e.active && e.id == game.tutorial.targetId; });
}

// One drone at a time, in front of the ship. Returns its id, or 0 when none could spawn.
auto spawnDrone(Game& game) -> uint32_t
{
    const size_t before = game.run.enemies.size();
    const Vector2 pos = Vector2Add(game.run.player.position, Vector2{.x = 0, .y = -aheadDistance});
    spawnEnemyAt(game, drifterKind, pos);
    return game.run.enemies.size() > before ? game.run.enemies.back().id : 0;
}

auto droneAlive(const Game& game) -> bool
{
    return std::any_of(game.run.enemies.begin(), game.run.enemies.end(),
                       [](const Enemy& e) { return e.active && e.kind == drifterKind; });
}

void enterAction(Game& game)
{
    auto& t = game.tutorial;
    const Player& p = game.run.player;
    t.timer = 0;
    t.prevShield = p.shieldActive;
    t.prevDash = p.dashing;
    t.prevCombo = p.dashing && p.shieldActive;
    switch (t.step)
    {
    case TutorialStep::Move:
        t.start = p.position;
        break;
    case TutorialStep::Shoot:
        t.targetId = spawnDrone(game);
        break;
    case TutorialStep::Level:
        t.startLevel = game.run.level;
        t.spawnCooldown = 0;
        break;
    case TutorialStep::Qte:
        spawnBeltbreaker(game, 0);
        if (!game.run.bosses.empty())
        {
            Boss& boss = game.run.bosses.front();
            boss.health = static_cast<int>(static_cast<float>(boss.maxHealth) * beltbreakerHealthFrac);
            boss.maxHealth = std::max(1, boss.health);
            sandboxForceBossQte(game, boss, BossAttack::PlateHurl);
        }
        t.qteRetry = qteRetryGap;
        break;
    default:
        break;
    }
}

// Moves to the next step's dialog. Steps without an action (Welcome, Upgrade) run straight on.
void nextStep(Game& game)
{
    auto& t = game.tutorial;
    const auto idx = std::find(order.begin(), order.end(), t.step) - order.begin();
    t.step = order.at(static_cast<size_t>(idx) + 1);
    t.dialog = true;
    t.timer = 0;
}

void finishTutorial(Game& game)
{
    game.tutorial.active = false;
    game.resources.settings.tutorialState = 1;
    saveSettings(game.resources.settings);
    game.state = GameState::TITLE;
    game.menuIndex = 0;
}

// Completes the current action step when its condition is met. Edge-based steps need a fresh
// press, so a key already held through the dialog cannot pass a step.
void checkAction(Game& game, float deltaTime)
{
    auto& t = game.tutorial;
    const Player& p = game.run.player;
    const bool combo = p.dashing && p.shieldActive;
    const bool shieldRise = p.shieldActive && !t.prevShield;
    const bool dashRise = p.dashing && !t.prevDash;
    const bool comboRise = combo && !t.prevCombo;
    t.prevShield = p.shieldActive;
    t.prevDash = p.dashing;
    t.prevCombo = combo;

    switch (t.step)
    {
    case TutorialStep::Move:
        if (Vector2Distance(p.position, t.start) >= moveDistance)
        {
            nextStep(game);
        }
        break;
    case TutorialStep::Shoot:
        if (!targetAlive(game))
        {
            nextStep(game);
        }
        break;
    case TutorialStep::Level:
        if (game.run.level > t.startLevel)
        {
            nextStep(game); // the level-up card screen has already run and been answered
            break;
        }
        t.spawnCooldown -= deltaTime;
        if (!droneAlive(game) && t.spawnCooldown <= 0)
        {
            if (t.droneOut)
            {
                // Easy: the first drone's death tops the XP bar up. Its orbs are not chased.
                game.run.xp = std::max(game.run.xp, game.run.xpToNext);
                t.droneOut = false;
            }
            else
            {
                t.droneOut = spawnDrone(game) != 0;
                t.spawnCooldown = spawnGap;
            }
        }
        break;
    case TutorialStep::Shield:
        if (shieldRise)
        {
            nextStep(game);
        }
        break;
    case TutorialStep::Dash:
        if (dashRise)
        {
            nextStep(game);
        }
        break;
    case TutorialStep::Parry:
        if (comboRise)
        {
            nextStep(game);
        }
        break;
    case TutorialStep::Qte:
    {
        if (game.run.bosses.empty())
        {
            break; // the Boss step takes over once the shield is down
        }
        Boss& boss = game.run.bosses.front();
        if (boss.beltbreakerPunishTimer > 0 && !bossQteActive(game))
        {
            nextStep(game); // clean clear: the shield shattered
            break;
        }
        // A missed QTE is forced again, so the player must answer it to go on.
        t.qteRetry -= deltaTime;
        if (!bossQteActive(game) && t.qteRetry <= 0)
        {
            sandboxForceBossQte(game, boss, BossAttack::PlateHurl);
            t.qteRetry = qteRetryGap;
        }
        break;
    }
    case TutorialStep::Boss:
        if (game.run.bosses.empty())
        {
            nextStep(game);
        }
        break;
    default:
        break;
    }
}
} // namespace

void startTutorial(Game& game)
{
    game.tutorial = TutorialState{.active = true, .start = game.run.player.position};
}

auto updateTutorial(Game& game, float deltaTime) -> bool
{
    auto& t = game.tutorial;
    if (!t.active)
    {
        return false;
    }
    Player& p = game.run.player;
    p.immunityTimer = std::max(p.immunityTimer, invulnerable);

    if (IsKeyPressed(KEY_ESCAPE) || pausePressed(game))
    {
        game.state = GameState::PAUSED; // the pause menu offers only resume, restart or exit
        game.menuIndex = 0;
        return true;
    }

    if (t.dialog)
    {
        const bool next = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
                          IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (next)
        {
            if (t.step == TutorialStep::Done)
            {
                finishTutorial(game);
                return true;
            }
            t.dialog = false;
            if (t.step == TutorialStep::Welcome)
            {
                nextStep(game);
            }
            else if (t.step == TutorialStep::Upgrade)
            {
                nextStep(game);
            }
            else
            {
                enterAction(game);
            }
        }
        return true;
    }

    t.timer += deltaTime;
    checkAction(game, deltaTime);
    return t.dialog;
}

auto tutorialDialog(const Game& game) -> const char*
{
    const bool touch = touchInputActive(game);
    switch (game.tutorial.step)
    {
    case TutorialStep::Welcome:
        return "Welcome, pilot. Nothing can hurt you here. Follow the prompts.";
    case TutorialStep::Move:
        return touch ? "Move your ship with the Left Stick. Fly a short way to continue."
                     : "Move your ship with WASD. Fly a short way to continue.";
    case TutorialStep::Shoot:
        return "Your weapons fire on their own. A drone is ahead.";
    case TutorialStep::Level:
        return "Destroy drones to gain XP. Fill the bar to level up.";
    case TutorialStep::Upgrade:
        return "Pick an upgrade. Upgrades make your weapons stronger. Weapons evolve at max level.";
    case TutorialStep::Shield:
        return touch ? "Hold a Shield button to raise your shield. It blocks damage."
                     : "Hold Right Click to raise your shield. It blocks damage.";
    case TutorialStep::Dash:
        return touch ? "Tap a Dash button to dash." : "Left Click dashes.";
    case TutorialStep::Parry:
        return "Dash and shield at the same time to parry.";
    case TutorialStep::Qte:
        return "A miniboss is here. Its shield only breaks in a Boss QTE. Answer each prompt on screen.";
    case TutorialStep::Boss:
        return "Its shield is down. Destroy the miniboss.";
    case TutorialStep::Done:
        return "Tutorial complete. Good flying, pilot.";
    }
    return "";
}

auto tutorialObjective(const Game& game) -> const char*
{
    const bool touch = touchInputActive(game);
    switch (game.tutorial.step)
    {
    case TutorialStep::Move:
        return touch ? "Move with the Left Stick" : "Move with WASD";
    case TutorialStep::Shoot:
        return "Destroy the drone";
    case TutorialStep::Level:
        return "Destroy drones until you level up";
    case TutorialStep::Shield:
        return touch ? "Hold a Shield button" : "Hold Right Click to shield";
    case TutorialStep::Dash:
        return touch ? "Tap a Dash button" : "Left Click to dash";
    case TutorialStep::Parry:
        return "Dash and shield together";
    case TutorialStep::Qte:
        return "Answer the Boss QTE prompts";
    case TutorialStep::Boss:
        return "Destroy the miniboss";
    default:
        return "";
    }
}
