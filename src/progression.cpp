#include "progression.hpp"

#include "achievements.hpp"
#include "entities/ship.hpp"
#include "game.hpp"
#include "update.hpp"
#include <algorithm>
#include <format>

auto achievementName(int32_t id) -> std::string
{
    if (id < achievementWeaponMaxed)
    {
        return std::format("{} kills with {}", evolutionKillThreshold,
                           weaponDisplayName(static_cast<WeaponType>(id - achievementWeaponKills)));
    }
    if (id < achievementDashTurret)
    {
        return std::format("Max {}", weaponDisplayName(static_cast<WeaponType>(id - achievementWeaponMaxed)));
    }
    if (id == achievementDashTurret)
    {
        return "Dash-kill a Turret";
    }
    if (id == achievementDashOrbiter)
    {
        return "Dash-kill an Orbiter";
    }
    if (id == achievementDashLaserFence)
    {
        return "Dash-kill a Laser Fence";
    }
    if (id == achievementDashNerveKills)
    {
        return std::format("{} dash or Nerve kills", flakCannonUnlockKillCount);
    }
    if (id < achievementBosses)
    {
        return std::format("Reach wave {}", achievementWaveMilestones.at(static_cast<size_t>(id - achievementWaves)));
    }
    constexpr std::array<const char*, static_cast<size_t>(BossAchievement::Count)> bosses{
        "Defeat the Beltbreaker", "Defeat the Wreckworm", "Defeat Slagmaw", "Defeat the Kraken", "Defeat the Banished"};
    return bosses.at(static_cast<size_t>(id - achievementBosses));
}

auto skillNodeName(int32_t node) -> std::string
{
    const SkillNode& n = skillTree.at(static_cast<size_t>(node));
    switch (n.kind)
    {
    case NodeKind::Ship:
        return std::string(ships.at(static_cast<size_t>(n.value)).name);
    case NodeKind::Weapon:
        return std::string(weaponDisplayName(static_cast<WeaponType>(n.value)));
    case NodeKind::Passive:
        return std::string(Skills.at(static_cast<size_t>(n.value)).name);
    case NodeKind::Evolution:
        return std::string(evolvedWeaponName.at(static_cast<size_t>(n.value)));
    case NodeKind::Slot:
        return std::format("Weapon slot {}", n.value);
    }
    return "";
}

auto skillNodeOwned(const Game& game, int32_t node) -> bool
{
    return node == 0 || game.resources.achievements.owned.at(static_cast<size_t>(node));
}

auto skillNodeAvailable(const Game& game, int32_t node) -> bool
{
    if (skillNodeOwned(game, node))
    {
        return false;
    }
    return std::ranges::all_of(skillTree.at(static_cast<size_t>(node)).prereqs,
                               [&](int8_t req) { return req < 0 || skillNodeOwned(game, req); });
}

auto skillPoints(const Game& game) -> int32_t
{
    const auto& a = game.resources.achievements;
    return static_cast<int32_t>(std::ranges::count(a.earned, true)) -
           static_cast<int32_t>(std::ranges::count(a.owned, true));
}

// The sandbox buys for free (unlimited points) and the purchase is saved, so a normal run can
// then be tested with it.
auto buySkillNode(Game& game, int32_t node) -> bool
{
    if (!skillNodeAvailable(game, node) || (!game.sandbox && skillPoints(game) <= 0))
    {
        return false;
    }
    auto& a = game.resources.achievements;
    a.owned.at(static_cast<size_t>(node)) = true;
    if (game.sandbox)
    {
        // Keep earned >= owned so leaving the sandbox never shows negative points.
        for (auto& earned : a.earned)
        {
            if (!earned && skillPoints(game) < 0)
            {
                earned = true;
            }
        }
    }
    saveAchievements(a);
    return true;
}

void earnAchievement(Game& game, int32_t id)
{
    auto& a = game.resources.achievements;
    if (a.earned.at(static_cast<size_t>(id)))
    {
        return;
    }
    a.earned.at(static_cast<size_t>(id)) = true;
    saveAchievements(a); // saved at once, so the tree shows the point straight away
    game.run.achievementToast = std::format("Achievement: {}  +1 skill point", achievementName(id));
    game.run.achievementToastTimer = 4.0F;
}

void earnBossAchievement(Game& game, BossAchievement boss)
{
    earnAchievement(game, achievementBosses + static_cast<int32_t>(boss));
}

void resetProgression(Game& game)
{
    auto& a = game.resources.achievements;
    a.earned.fill(false);
    a.owned.fill(false);
    saveAchievements(a);
}

auto isPassiveUnlocked(const Game& game, SkillType skill) -> bool
{
    if (game.sandbox)
    {
        return true;
    }
    for (int32_t n = 0; n < skillNodeCount; n++)
    {
        const SkillNode& node = skillTree.at(static_cast<size_t>(n));
        if (node.kind == NodeKind::Passive && node.value == static_cast<int32_t>(skill))
        {
            return skillNodeOwned(game, n);
        }
    }
    return true; // not in the tree
}

auto isShipUnlocked(const Game& game, ShipClass ship) -> bool
{
    if (game.sandbox)
    {
        return true;
    }
    for (int32_t n = 0; n < skillNodeCount; n++)
    {
        const SkillNode& node = skillTree.at(static_cast<size_t>(n));
        if (node.kind == NodeKind::Ship && node.value == static_cast<int32_t>(ship))
        {
            return skillNodeOwned(game, n);
        }
    }
    return false;
}

namespace
{
auto skillTreeUnit(const Game& game) -> float
{
    return std::min(static_cast<float>(game.resources.windowWidth) / 15.5F,
                    static_cast<float>(game.resources.windowHeight) / 8.2F);
}
} // namespace

auto skillTreeNodePos(const Game& game, int32_t node) -> Vector2
{
    const float unit = skillTreeUnit(game);
    const Vector2 p = skillTree.at(static_cast<size_t>(node)).pos;
    return Vector2{.x = static_cast<float>(game.resources.windowWidth) / 2 + p.x * unit,
                   .y = static_cast<float>(game.resources.windowHeight) / 2 - 1.5F * unit + p.y * unit};
}

auto skillTreeNodeRadius(const Game& game) -> float { return skillTreeUnit(game) * 0.3F; }
