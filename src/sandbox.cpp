#include "sandbox.hpp"

#include "achievements.hpp"
#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "entities/ship.hpp"
#include "menu.hpp"
#include "raylib.h"
#include "raymath.h"
#include "update.hpp"
#include <cmath>
#include <format>

namespace
{

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

void sandboxSpawnSelectedEnemy(Game& game)
{
    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(200, 320));
    const Vector2 pos =
        Vector2Add(game.run.player.position,
                   Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
    spawnEnemyAt(game, game.sandboxKindIndex, pos);
}

void sandboxSpawnSignatureBoss(Game& game)
{
    if (currentBiome(game.run.waveNumber) == Biome::ShatteredBelt)
    {
        spawnBeltbreaker(game, game.run.waveNumber);
    }
    else if (currentBiome(game.run.waveNumber) == Biome::Rustbloom)
    {
        spawnWreckworm(game, game.run.waveNumber);
    }
    else if (currentBiome(game.run.waveNumber) == Biome::SolarForge)
    {
        spawnSlagmaw(game, game.run.waveNumber);
    }
}

void sandboxClearBoard(Game& game)
{
    game.run.enemies.clear();
    game.run.eliteHazards.clear();
    game.run.asteroids.clear();
    game.run.bossProjectiles.clear();
    game.run.mines.clear();
    game.run.bosses.clear();
    game.run.bossDeathShockwaves.clear();
}

void sandboxFullHeal(Game& game)
{
    game.run.player.health = game.run.player.maxHealth;
    game.run.player.shieldStacks = currentShip(game).maxShieldStacks;
    game.run.player.nerve = UpdateConstants::nerveMax;
}

void sandboxResetAbilities(Game& game)
{
    const ShipDef& ship = currentShip(game);
    game.run.weapons = {Weapon{.type = ship.defaultWeapon, .level = 1}};
    game.run.skillLevels.fill(0);
    game.run.skillLevels.at(
        static_cast<size_t>(weaponGrantSkill.at(static_cast<size_t>(ship.defaultWeapon)))) = 1;
}

void sandboxToggleDeath(Game& game) { game.sandboxDeathEnabled = !game.sandboxDeathEnabled; }

void sandboxToggleNaturalSpawn(Game& game)
{
    game.sandboxNaturalSpawnEnabled = !game.sandboxNaturalSpawnEnabled;
}

void sandboxCycleShip(Game& game, int32_t dir)
{
    const auto count = static_cast<int32_t>(ShipClass::Count);
    game.resources.settings.shipIndex = (game.resources.settings.shipIndex + dir + count) % count;
    resetRun(game);
}

void sandboxForceAttackNearestBoss(Game& game)
{
    if (game.run.bosses.empty())
    {
        return;
    }
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

void sandboxCycleWave(Game& game, int32_t dir)
{
    if (dir > 0)
    {
        game.run.waveNumber++;
    }
    else if (game.run.waveNumber > 1)
    {
        game.run.waveNumber--;
    }
}

void sandboxDropSelectedPickup(Game& game)
{
    const auto option = sandboxPickupOption(game.sandboxPickupIndex);
    spawnPickup(game, mouseWorldPos(game), 0, option.type, option.element, option.mechanism);
}

namespace SandboxMenuLayout
{
constexpr int32_t stepperCount = 5;
constexpr int32_t stepperRowWidth = 480;
constexpr int32_t stepperRowHeight = 40;
constexpr int32_t stepperRowGap = 10;
constexpr int32_t stepperTopY = 150;
constexpr int32_t stepperMinusWidth = 44;
constexpr int32_t stepperPlusWidth = 44;

constexpr int32_t buttonWidth = 300;
constexpr int32_t buttonHeight = 42;
constexpr int32_t buttonGapX = 14;
constexpr int32_t buttonGapY = 10;
constexpr int32_t buttonCols = 3;
constexpr int32_t buttonTopY =
    stepperTopY + stepperCount * (stepperRowHeight + stepperRowGap) + 40;

constexpr int32_t modeToggleWidth = 160;
constexpr int32_t modeToggleHeight = 40;

constexpr int32_t categoryTabWidth = 150;
constexpr int32_t categoryTabHeight = 38;
constexpr int32_t categoryTabGap = 10;
constexpr int32_t categoryTabTopY = 150;

constexpr int32_t previewWidth = 360;
constexpr int32_t previewHeight = 300;
constexpr int32_t previewTopY = 220;

constexpr int32_t arrowButtonWidth = 50;
constexpr int32_t arrowButtonHeight = 50;
}

}

auto sandboxMenuButtons() -> const std::vector<SandboxMenuButton>&
{
    static const std::vector<SandboxMenuButton> buttons{
        {[](const Game&) { return std::string("Spawn Selected Enemy"); },
         sandboxSpawnSelectedEnemy},
        {[](const Game&) { return std::string("Spawn Boss"); }, spawnBoss},
        {[](const Game&) { return std::string("Spawn Miniboss"); }, spawnMiniboss},
        {[](const Game&) { return std::string("Spawn Swarm Boss"); }, spawnSwarmBoss},
        {[](const Game&) { return std::string("Spawn Signature Boss"); },
         sandboxSpawnSignatureBoss},
        {[](const Game&) { return std::string("Force Selected Attack (Nearest Boss)"); },
         sandboxForceAttackNearestBoss},
        {[](const Game&) { return std::string("Drop Selected Pickup"); },
         sandboxDropSelectedPickup},
        {[](const Game&) { return std::string("Clear Board"); }, sandboxClearBoard},
        {[](const Game&) { return std::string("Level-Up Picker"); }, startLevelUp},
        {[](const Game&) { return std::string("Full Heal"); }, sandboxFullHeal},
        {[](const Game&) { return std::string("Reset Abilities"); }, sandboxResetAbilities},
        {[](const Game&) { return std::string("Spawn Black Hole"); }, spawnBlackHole},
        {[](const Game&) { return std::string("Spawn Wormhole"); }, spawnWormholePair},
        {[](const Game&) { return std::string("Spawn Warlord Hazard"); },
         [](Game& game) { spawnEliteHazard(game, EliteHazardRole::Warlord); }},
        {[](const Game&) { return std::string("Spawn Suppressor Hazard"); },
         [](Game& game) { spawnEliteHazard(game, EliteHazardRole::Suppressor); }},
        {[](const Game& game)
         { return std::format("Death: {}", game.sandboxDeathEnabled ? "ON" : "OFF"); },
         sandboxToggleDeath},
        {[](const Game& game)
         {
             return std::format("Natural Spawn: {}",
                                game.sandboxNaturalSpawnEnabled ? "ON" : "OFF");
         },
         sandboxToggleNaturalSpawn},
    };
    return buttons;
}

auto sandboxMenuSteppers() -> const std::vector<SandboxMenuStepper>&
{
    static const std::vector<SandboxMenuStepper> steppers{
        {"Enemy Kind",
         [](const Game& game)
         { return std::string(enemyKinds.at(static_cast<size_t>(game.sandboxKindIndex)).name); },
         [](Game& game)
         {
             const auto count = static_cast<int32_t>(enemyKinds.size());
             game.sandboxKindIndex = (game.sandboxKindIndex - 1 + count) % count;
         },
         [](Game& game)
         {
             const auto count = static_cast<int32_t>(enemyKinds.size());
             game.sandboxKindIndex = (game.sandboxKindIndex + 1) % count;
         }},
        {"Boss Attack",
         [](const Game& game) {
             return std::string(
                 bossAttackNames.at(static_cast<size_t>(game.sandboxBossAttackIndex)));
         },
         [](Game& game)
         {
             game.sandboxBossAttackIndex =
                 (game.sandboxBossAttackIndex - 1 + bossAttackCount) % bossAttackCount;
         },
         [](Game& game)
         {
             game.sandboxBossAttackIndex = (game.sandboxBossAttackIndex + 1) % bossAttackCount;
         }},
        {"Wave",
         [](const Game& game)
         {
             return std::format("{} ({})", game.run.waveNumber,
                                biomeName(currentBiome(game.run.waveNumber)));
         },
         [](Game& game) { sandboxCycleWave(game, -1); },
         [](Game& game) { sandboxCycleWave(game, 1); }},
        {"Ship", [](const Game& game) { return std::string(currentShip(game).name); },
         [](Game& game) { sandboxCycleShip(game, -1); },
         [](Game& game) { sandboxCycleShip(game, 1); }},
        {"Pickup",
         [](const Game& game) { return std::string(sandboxPickupName(game.sandboxPickupIndex)); },
         [](Game& game)
         {
             game.sandboxPickupIndex = (game.sandboxPickupIndex - 1 +
                                        static_cast<int32_t>(sandboxPickupCount)) %
                                       static_cast<int32_t>(sandboxPickupCount);
         },
         [](Game& game)
         {
             game.sandboxPickupIndex =
                 (game.sandboxPickupIndex + 1) % static_cast<int32_t>(sandboxPickupCount);
         }},
    };
    return steppers;
}

auto sandboxStepperRowRect(const Game& game, int32_t index) -> Rectangle
{
    const float scale = guiUiScale(game);
    return Rectangle{
        .x = static_cast<float>(game.resources.windowWidth) / 2 -
             static_cast<float>(SandboxMenuLayout::stepperRowWidth) * scale / 2,
        .y = (static_cast<float>(SandboxMenuLayout::stepperTopY) +
              static_cast<float>(index) *
                  static_cast<float>(SandboxMenuLayout::stepperRowHeight +
                                     SandboxMenuLayout::stepperRowGap)) *
             scale,
        .width = static_cast<float>(SandboxMenuLayout::stepperRowWidth) * scale,
        .height = static_cast<float>(SandboxMenuLayout::stepperRowHeight) * scale};
}

auto sandboxStepperMinusRect(const Game& game, int32_t index) -> Rectangle
{
    const Rectangle row = sandboxStepperRowRect(game, index);
    const float scale = guiUiScale(game);
    return Rectangle{.x = row.x, .y = row.y,
                      .width = static_cast<float>(SandboxMenuLayout::stepperMinusWidth) * scale,
                      .height = row.height};
}

auto sandboxStepperPlusRect(const Game& game, int32_t index) -> Rectangle
{
    const Rectangle row = sandboxStepperRowRect(game, index);
    const float scale = guiUiScale(game);
    const float plusW = static_cast<float>(SandboxMenuLayout::stepperPlusWidth) * scale;
    return Rectangle{.x = row.x + row.width - plusW, .y = row.y, .width = plusW,
                      .height = row.height};
}

auto sandboxStepperValueRect(const Game& game, int32_t index) -> Rectangle
{
    const Rectangle row = sandboxStepperRowRect(game, index);
    const float scale = guiUiScale(game);
    const float minusW = static_cast<float>(SandboxMenuLayout::stepperMinusWidth) * scale;
    const float plusW = static_cast<float>(SandboxMenuLayout::stepperPlusWidth) * scale;
    return Rectangle{.x = row.x + minusW, .y = row.y, .width = row.width - minusW - plusW,
                      .height = row.height};
}

auto sandboxMenuButtonRect(const Game& game, int32_t index) -> Rectangle
{
    const float scale = guiUiScale(game);
    const int32_t col = index % SandboxMenuLayout::buttonCols;
    const int32_t row = index / SandboxMenuLayout::buttonCols;
    const auto totalWidth =
        static_cast<float>(SandboxMenuLayout::buttonCols * SandboxMenuLayout::buttonWidth +
                           (SandboxMenuLayout::buttonCols - 1) * SandboxMenuLayout::buttonGapX);
    const float startX = static_cast<float>(game.resources.windowWidth) / 2 - totalWidth * scale / 2;
    return Rectangle{
        .x = startX + static_cast<float>(col) *
                          static_cast<float>(SandboxMenuLayout::buttonWidth +
                                             SandboxMenuLayout::buttonGapX) *
                          scale,
        .y = static_cast<float>(SandboxMenuLayout::buttonTopY +
                                row * (SandboxMenuLayout::buttonHeight +
                                       SandboxMenuLayout::buttonGapY)) *
             scale,
        .width = static_cast<float>(SandboxMenuLayout::buttonWidth) * scale,
        .height = static_cast<float>(SandboxMenuLayout::buttonHeight) * scale};
}

auto sandboxMenuCloseButtonRect(const Game& game) -> Rectangle
{
    const float scale = guiUiScale(game);
    return Rectangle{.x = static_cast<float>(game.resources.windowWidth) - 160 * scale - 20 * scale,
                      .y = 20 * scale, .width = 160 * scale, .height = 40 * scale};
}

auto sandboxToggleIndicatorRect(const Game& game) -> Rectangle
{
    const float scale = guiUiScale(game);
    return Rectangle{.x = 10 * scale, .y = 10 * scale, .width = 200 * scale, .height = 34 * scale};
}

auto browserCategoryLabel(int32_t category) -> std::string_view
{
    switch (category)
    {
    case 0:
        return "Enemies";
    case 1:
        return "Bosses";
    case 2:
        return "Player";
    case 3:
        return "Pickups";
    case 4:
        return "Weapons";
    case 5:
        return "Particles";
    default:
        return "";
    }
}

auto browserParticleStyleName(int32_t index) -> std::string_view
{
    switch (index)
    {
    case 0:
        return "Flame";
    case 1:
        return "Dash Trail";
    case 2:
        return "Death Burst";
    default:
        return "";
    }
}

auto browserItemCount(int32_t category) -> int32_t
{
    switch (category)
    {
    case 0:
        return static_cast<int32_t>(enemyKinds.size());
    case 1:
        return static_cast<int32_t>(bossTypes.size());
    case 2:
        return static_cast<int32_t>(ShipClass::Count);
    case 3:
        return static_cast<int32_t>(sandboxPickupCount);
    case 4:
        return static_cast<int32_t>(WeaponType::Count);
    case 5:
        return browserParticleStyleCount;
    default:
        return 1;
    }
}

auto browserIndexRef(Game& game) -> int&
{
    switch (game.browserCategoryIndex)
    {
    case 0:
        return game.sandboxKindIndex;
    case 1:
        return game.browserBossTypeIndex;
    case 2:
        return game.browserShipIndex;
    case 3:
        return game.sandboxPickupIndex;
    case 4:
        return game.browserWeaponIndex;
    case 5:
    default:
        return game.browserParticleIndex;
    }
}

auto browserCurrentName(const Game& game) -> std::string
{
    switch (game.browserCategoryIndex)
    {
    case 0:
        return std::string(enemyKinds.at(static_cast<size_t>(game.sandboxKindIndex)).name);
    case 1:
        return std::string(bossTypes.at(static_cast<size_t>(game.browserBossTypeIndex)).name);
    case 2:
        return std::string(ships.at(static_cast<size_t>(game.browserShipIndex)).name);
    case 3:
        return std::string(sandboxPickupName(game.sandboxPickupIndex));
    case 4:
        return std::string(
            weaponDisplayName(static_cast<WeaponType>(game.browserWeaponIndex)));
    case 5:
        return std::string(browserParticleStyleName(game.browserParticleIndex));
    default:
        return "";
    }
}

auto browserCurrentDetails(const Game& game) -> std::string
{
    switch (game.browserCategoryIndex)
    {
    case 0:
    {
        const auto& kind = enemyKinds.at(static_cast<size_t>(game.sandboxKindIndex));
        const std::string biome =
            kind.biomeExclusive ? std::string(biomeName(kind.biome)) : std::string("All Biomes");
        return std::format("Biome: {}   HP: {}   Damage: {}   Speed: {:.1f}", biome, kind.health,
                           kind.contactDamage, kind.speed);
    }
    case 1:
    {
        const auto& type = bossTypes.at(static_cast<size_t>(game.browserBossTypeIndex));
        return std::format("HP Mult: {:.2f}   Size Mult: {:.2f}", type.healthMult, type.sizeMult);
    }
    default:
        return "";
    }
}

void browserCycleItem(Game& game, int32_t dir)
{
    const int32_t count = browserItemCount(game.browserCategoryIndex);
    int& idx = browserIndexRef(game);
    idx = (idx + dir + count) % count;
}

auto sandboxBrowserModeToggleRect(const Game& game) -> Rectangle
{
    const float scale = guiUiScale(game);
    return Rectangle{.x = 20 * scale, .y = 20 * scale,
                      .width = static_cast<float>(SandboxMenuLayout::modeToggleWidth) * scale,
                      .height = static_cast<float>(SandboxMenuLayout::modeToggleHeight) * scale};
}

auto sandboxBrowserCategoryButtonRect(const Game& game, int32_t index) -> Rectangle
{
    const float scale = guiUiScale(game);
    const auto totalWidth =
        static_cast<float>(browserCategoryCount * SandboxMenuLayout::categoryTabWidth +
                           (browserCategoryCount - 1) * SandboxMenuLayout::categoryTabGap);
    const float startX = static_cast<float>(game.resources.windowWidth) / 2 - totalWidth * scale / 2;
    return Rectangle{
        .x = startX + static_cast<float>(index) *
                          static_cast<float>(SandboxMenuLayout::categoryTabWidth +
                                             SandboxMenuLayout::categoryTabGap) *
                          scale,
        .y = static_cast<float>(SandboxMenuLayout::categoryTabTopY) * scale,
        .width = static_cast<float>(SandboxMenuLayout::categoryTabWidth) * scale,
        .height = static_cast<float>(SandboxMenuLayout::categoryTabHeight) * scale};
}

auto sandboxBrowserPreviewRect(const Game& game) -> Rectangle
{
    const float scale = guiUiScale(game);
    return Rectangle{.x = static_cast<float>(game.resources.windowWidth) / 2 -
                          static_cast<float>(SandboxMenuLayout::previewWidth) * scale / 2,
                      .y = static_cast<float>(SandboxMenuLayout::previewTopY) * scale,
                      .width = static_cast<float>(SandboxMenuLayout::previewWidth) * scale,
                      .height = static_cast<float>(SandboxMenuLayout::previewHeight) * scale};
}

auto sandboxBrowserPrevButtonRect(const Game& game) -> Rectangle
{
    const Rectangle preview = sandboxBrowserPreviewRect(game);
    const float scale = guiUiScale(game);
    return Rectangle{.x = preview.x - static_cast<float>(SandboxMenuLayout::arrowButtonWidth) * scale -
                          10 * scale,
                      .y = preview.y + preview.height / 2 -
                          static_cast<float>(SandboxMenuLayout::arrowButtonHeight) * scale / 2,
                      .width = static_cast<float>(SandboxMenuLayout::arrowButtonWidth) * scale,
                      .height = static_cast<float>(SandboxMenuLayout::arrowButtonHeight) * scale};
}

auto sandboxBrowserNextButtonRect(const Game& game) -> Rectangle
{
    const Rectangle preview = sandboxBrowserPreviewRect(game);
    const float scale = guiUiScale(game);
    return Rectangle{.x = preview.x + preview.width + 10 * scale,
                      .y = preview.y + preview.height / 2 -
                          static_cast<float>(SandboxMenuLayout::arrowButtonHeight) * scale / 2,
                      .width = static_cast<float>(SandboxMenuLayout::arrowButtonWidth) * scale,
                      .height = static_cast<float>(SandboxMenuLayout::arrowButtonHeight) * scale};
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
    game.sandboxNaturalSpawnEnabled = false;
    game.sandboxBrowserMode = false;
    game.browserCategoryIndex = 0;
    game.browserBossTypeIndex = 0;
    game.browserWeaponIndex = 0;
    game.browserShipIndex = 0;
    game.browserParticleIndex = 0;
}

void updateSandboxInput(Game& game)
{
    if (IsKeyPressed(KEY_TAB))
    {
        game.state = GameState::SANDBOX_MENU;
        return;
    }

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
        sandboxSpawnSelectedEnemy(game);
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
        sandboxSpawnSignatureBoss(game);
    }

    if (IsKeyPressed(KEY_K))
    {
        sandboxClearBoard(game);
    }

    if (IsKeyPressed(KEY_L))
    {
        startLevelUp(game);
    }

    if (IsKeyPressed(KEY_H))
    {
        sandboxFullHeal(game);
    }

    if (IsKeyPressed(KEY_R))
    {
        sandboxResetAbilities(game);
    }

    if (IsKeyPressed(KEY_G))
    {
        sandboxToggleDeath(game);
    }

    if (IsKeyPressed(KEY_T))
    {
        sandboxToggleNaturalSpawn(game);
    }

    if (IsKeyPressed(KEY_I))
    {
        const int32_t dir = IsKeyDown(KEY_LEFT_SHIFT) ? -1 : 1;
        sandboxCycleShip(game, dir);
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
        sandboxCycleWave(game, 1);
    }
    if (IsKeyPressed(KEY_MINUS))
    {
        sandboxCycleWave(game, -1);
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
        sandboxDropSelectedPickup(game);
    }

    if (IsKeyPressed(KEY_O))
    {
        sandboxForceAttackNearestBoss(game);
    }
}

void updateSandboxMenuInput(Game& game)
{
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_TAB))
    {
        game.state = GameState::GAMEPLAY;
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        return;
    }

    const Vector2 mouse = GetMousePosition();

    if (CheckCollisionPointRec(mouse, sandboxMenuCloseButtonRect(game)))
    {
        game.state = GameState::GAMEPLAY;
        return;
    }

    if (CheckCollisionPointRec(mouse, sandboxBrowserModeToggleRect(game)))
    {
        game.sandboxBrowserMode = !game.sandboxBrowserMode;
        return;
    }

    if (game.sandboxBrowserMode)
    {
        for (int32_t i = 0; i < browserCategoryCount; i++)
        {
            if (CheckCollisionPointRec(mouse, sandboxBrowserCategoryButtonRect(game, i)))
            {
                game.browserCategoryIndex = i;
                return;
            }
        }
        if (CheckCollisionPointRec(mouse, sandboxBrowserPrevButtonRect(game)))
        {
            browserCycleItem(game, -1);
            return;
        }
        if (CheckCollisionPointRec(mouse, sandboxBrowserNextButtonRect(game)))
        {
            browserCycleItem(game, 1);
            return;
        }
        return;
    }

    const auto& steppers = sandboxMenuSteppers();
    for (int32_t i = 0; i < static_cast<int32_t>(steppers.size()); i++)
    {
        if (CheckCollisionPointRec(mouse, sandboxStepperMinusRect(game, i)))
        {
            steppers.at(static_cast<size_t>(i)).decrement(game);
            return;
        }
        if (CheckCollisionPointRec(mouse, sandboxStepperPlusRect(game, i)))
        {
            steppers.at(static_cast<size_t>(i)).increment(game);
            return;
        }
    }

    const auto& buttons = sandboxMenuButtons();
    for (int32_t i = 0; i < static_cast<int32_t>(buttons.size()); i++)
    {
        if (CheckCollisionPointRec(mouse, sandboxMenuButtonRect(game, i)))
        {
            buttons.at(static_cast<size_t>(i)).action(game);
            return;
        }
    }
}

auto sandboxPickupName(int32_t index) -> std::string_view { return sandboxPickupOption(index).name; }

auto sandboxPickupPreview(int32_t index) -> PickupCatalogEntry { return sandboxPickupOption(index); }
