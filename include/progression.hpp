#pragma once

#include "entities/item.hpp"
#include "raylib.h"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

struct Game;
enum class ShipClass : std::uint8_t;

// --- Achievements and the skill tree (docs/design/progression.md) -----------------
//
// Every achievement earned is worth one point; each point buys one node in the skill tree. A
// node is buyable once all its prerequisites are owned. A run starts with only the Ranger and
// Forward Shot: other weapons, passives, evolutions, extra slots and ships come from the tree.
// The sandbox ignores the gates and buys nodes for free.
// ---------------------------------------------------------------------------------

constexpr int32_t weaponTypeCount = static_cast<int32_t>(WeaponType::Count);

// Achievement ids, in blocks.
constexpr int32_t achievementWeaponKills = 0;                                 // + weapon
constexpr int32_t achievementWeaponMaxed = achievementWeaponKills + weaponTypeCount; // + weapon
constexpr int32_t achievementDashTurret = achievementWeaponMaxed + weaponTypeCount;
constexpr int32_t achievementDashOrbiter = achievementDashTurret + 1;
constexpr int32_t achievementDashLaserFence = achievementDashOrbiter + 1;
constexpr int32_t achievementDashNerveKills = achievementDashLaserFence + 1;
constexpr int32_t achievementWaves = achievementDashNerveKills + 1; // + wave milestone index
constexpr std::array<int32_t, 6> achievementWaveMilestones{5, 10, 25, 50, 75, 100};
constexpr int32_t achievementBosses = achievementWaves + static_cast<int32_t>(achievementWaveMilestones.size());
enum class BossAchievement : std::uint8_t
{
    Beltbreaker,
    Wreckworm,
    Slagmaw,
    Kraken,
    Banished,
    Count
};
constexpr int32_t achievementCount = achievementBosses + static_cast<int32_t>(BossAchievement::Count);

auto achievementName(int32_t id) -> std::string;

enum class NodeKind : std::uint8_t
{
    Ship,
    Weapon,
    Passive,   // an "upgrade helper"
    Evolution, // an "upgrade": value is the weapon it evolves
    Slot       // value is the slot count it raises the cap to
};

struct SkillNode
{
    NodeKind kind;
    int32_t value; // ShipClass, WeaponType, SkillType or slot count by kind
    std::array<int8_t, 3> prereqs{-1, -1, -1};
    Vector2 pos; // layout units: Ranger at the origin, ~1 unit between neighbours
};

// Ranger hub (node 0, always owned); Offence branch to the right toward the Interceptor,
// Defence to the left toward the Bastion, Utility below. Along each branch: weapons, then
// helpers, then upgrades, then the ship.
constexpr std::array<SkillNode, 31> skillTree{{
    {NodeKind::Ship, 1 /* Ranger */, {-1, -1, -1}, {0, 0}},
    // Offence
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::Homing), {0, -1, -1}, {1.0F, 0}},          // 1
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::Railgun), {1, -1, -1}, {2.0F, -0.7F}},     // 2
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::Sniper), {2, -1, -1}, {3.0F, -1.1F}},      // 3
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::FlakCannon), {1, -1, -1}, {2.0F, 0.7F}},   // 4
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::Flamethrower), {4, -1, -1}, {3.0F, 0.4F}}, // 5
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::ChainLightning), {4, -1, -1}, {3.0F, 1.3F}}, // 6
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::Beam), {3, -1, -1}, {4.0F, -1.4F}},        // 7
    {NodeKind::Passive, static_cast<int32_t>(SkillType::Damage), {2, 5, -1}, {4.0F, -0.2F}},       // 8
    {NodeKind::Passive, static_cast<int32_t>(SkillType::Cooldown), {6, -1, -1}, {4.0F, 1.1F}},     // 9
    {NodeKind::Evolution, static_cast<int32_t>(WeaponType::Forward), {8, -1, -1}, {5.0F, -0.6F}},  // 10
    {NodeKind::Evolution, static_cast<int32_t>(WeaponType::Homing), {9, 1, -1}, {5.0F, 0.7F}},     // 11
    {NodeKind::Ship, 2 /* Interceptor */, {10, 11, -1}, {6.0F, 0}},       // 12
    {NodeKind::Evolution, static_cast<int32_t>(WeaponType::LaserShots), {12, 24, -1}, {6.8F, 0.9F}}, // 13
    // Defence
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::Mine), {0, -1, -1}, {-1.0F, 0}},             // 14
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::Shock), {14, -1, -1}, {-2.0F, -0.7F}},       // 15
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::TurretDeploy), {15, -1, -1}, {-3.0F, -1.1F}}, // 16
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::FollowerDrone), {14, -1, -1}, {-2.0F, 0.7F}}, // 17
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::LaserDrone), {17, -1, -1}, {-3.0F, 0.9F}},   // 18
    {NodeKind::Weapon, static_cast<int32_t>(WeaponType::Ricochet), {16, -1, -1}, {-4.0F, -1.4F}},    // 19
    {NodeKind::Passive, static_cast<int32_t>(SkillType::Barrier), {16, 18, -1}, {-4.0F, 0.2F}},      // 20
    {NodeKind::Passive, static_cast<int32_t>(SkillType::MaxHp), {19, -1, -1}, {-5.0F, -1.0F}},       // 21
    {NodeKind::Evolution, static_cast<int32_t>(WeaponType::Shock), {21, 15, -1}, {-5.6F, -0.2F}},    // 22
    {NodeKind::Ship, 0 /* Bastion */, {20, 22, -1}, {-6.2F, 0.5F}},         // 23
    // Utility
    {NodeKind::Passive, static_cast<int32_t>(SkillType::MoveSpeed), {0, -1, -1}, {0, 1.3F}},         // 24
    {NodeKind::Passive, static_cast<int32_t>(SkillType::PickupRadius), {24, -1, -1}, {0.9F, 2.2F}},  // 25
    {NodeKind::Slot, 4, {0, -1, -1}, {-0.9F, 2.0F}},                                                 // 26
    {NodeKind::Slot, 5, {26, 25, -1}, {-0.9F, 3.0F}},                                                // 27
    {NodeKind::Slot, 6, {27, -1, -1}, {-0.9F, 4.0F}},                                                // 28
    {NodeKind::Evolution, static_cast<int32_t>(WeaponType::Mine), {25, 14, -1}, {0.9F, 3.2F}},       // 29
    {NodeKind::Evolution, static_cast<int32_t>(WeaponType::Orbit), {23, 20, -1}, {-6.9F, 1.4F}},     // 30
}};
constexpr int32_t skillNodeCount = static_cast<int32_t>(skillTree.size());

auto skillNodeName(int32_t node) -> std::string;
auto skillNodeDescription(int32_t node) -> std::string;
constexpr float skillTreeHoldDuration = 0.7F; // hold to unlock
auto skillNodeOwned(const Game& game, int32_t node) -> bool;
auto skillNodeAvailable(const Game& game, int32_t node) -> bool; // not owned, prerequisites owned
auto skillPoints(const Game& game) -> int32_t;                    // earned minus spent
auto buySkillNode(Game& game, int32_t node) -> bool;
void earnAchievement(Game& game, int32_t id);
void earnBossAchievement(Game& game, BossAchievement boss);
void resetProgression(Game& game);

// Screen layout of the tree, shared by its update and draw.
auto skillTreeNodePos(const Game& game, int32_t node) -> Vector2;
auto skillTreeNodeRadius(const Game& game) -> float;

auto isPassiveUnlocked(const Game& game, SkillType skill) -> bool;
auto isShipUnlocked(const Game& game, ShipClass ship) -> bool;
