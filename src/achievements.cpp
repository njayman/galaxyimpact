#include "achievements.hpp"

#include "entities/item.hpp"
#include "entities/ship.hpp"
#include "game.hpp"
#include "platform.hpp"
#include <fstream>
#include <string>
#include <string_view>

namespace
{
auto achievementsFilePath() -> std::string { return getSaveDataDir() + "achievements.txt"; }

void showToast(Game& game, std::string_view message)
{
    constexpr float toastDuration = 4.0F;
    game.run.achievementToast = message;
    game.run.achievementToastTimer = toastDuration;
}

auto enemyKindIndexByName(std::string_view name) -> int32_t
{
    for (size_t i = 0; i < enemyKinds.size(); i++)
    {
        if (enemyKinds.at(i).name == name)
        {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

void unlockType(Game& game, Achievements& achievements, WeaponType weapon, std::string_view label)
{
    const auto idx = static_cast<size_t>(weapon);
    if (achievements.typeUnlocked.at(idx))
    {
        return;
    }
    achievements.typeUnlocked.at(idx) = true;
    showToast(game, std::string("Achievement unlocked: ") + std::string(label) + "!");
    saveAchievements(achievements);
}
}

auto weaponDisplayName(WeaponType weapon) -> std::string_view
{
    return Skills.at(static_cast<size_t>(weaponGrantSkill.at(static_cast<size_t>(weapon)))).name;
}

auto effectiveWeaponName(const Weapon& weapon) -> std::string_view
{
    if (weapon.evolved)
    {
        return evolvedWeaponName.at(static_cast<size_t>(weapon.type));
    }
    return weaponDisplayName(weapon.type);
}

auto loadAchievements() -> Achievements
{
    Achievements achievements{};

    std::ifstream file(achievementsFilePath());
    if (!file)
    {
        return achievements;
    }

    for (auto& count : achievements.killsByWeapon)
    {
        if (int32_t value = 0; file >> value)
        {
            count = value;
        }
    }
    for (auto& count : achievements.dashKillsByEnemyKind)
    {
        if (int32_t value = 0; file >> value)
        {
            count = value;
        }
    }
    if (int32_t value = 0; file >> value)
    {
        achievements.dashOrNerveKills = value;
    }
    if (int32_t value = 0; file >> value)
    {
        achievements.highestWaveReached = value;
    }
    if (int32_t value = 0; file >> value)
    {
        achievements.slotCap = value;
    }
    for (auto& unlocked : achievements.typeUnlocked)
    {
        if (int32_t value = 0; file >> value)
        {
            unlocked = value != 0;
        }
    }
    for (auto& unlocked : achievements.evolutionUnlocked)
    {
        if (int32_t value = 0; file >> value)
        {
            unlocked = value != 0;
        }
    }
    if (int32_t value = 0; file >> value)
    {
        achievements.infiniteModeUnlocked = value != 0;
    }

    return achievements;
}

void saveAchievements(const Achievements& achievements)
{
    std::ofstream file(achievementsFilePath(), std::ios::trunc);
    if (!file)
    {
        return;
    }

    for (const auto count : achievements.killsByWeapon)
    {
        file << count << ' ';
    }
    for (const auto count : achievements.dashKillsByEnemyKind)
    {
        file << count << ' ';
    }
    file << achievements.dashOrNerveKills << ' ' << achievements.highestWaveReached << ' '
         << achievements.slotCap;
    for (const auto unlocked : achievements.typeUnlocked)
    {
        file << ' ' << (unlocked ? 1 : 0);
    }
    for (const auto unlocked : achievements.evolutionUnlocked)
    {
        file << ' ' << (unlocked ? 1 : 0);
    }
    file << ' ' << (achievements.infiniteModeUnlocked ? 1 : 0);

    file.close();
    platformSyncSaveData();
}

auto isWeaponTypeUnlocked(const Game& game, WeaponType weapon) -> bool
{
    if (currentShip(game).defaultWeapon == weapon)
    {
        return true;
    }
    return game.resources.achievements.typeUnlocked.at(static_cast<size_t>(weapon));
}

void recordWeaponKill(Game& game, WeaponType weapon)
{
    Achievements& achievements = game.resources.achievements;
    const auto idx = static_cast<size_t>(weapon);
    achievements.killsByWeapon.at(idx)++;

    if (achievements.evolutionUnlocked.at(idx))
    {
        return;
    }

    if (achievements.killsByWeapon.at(idx) >= evolutionKillThreshold)
    {
        achievements.evolutionUnlocked.at(idx) = true;
        saveAchievements(achievements);
        showToast(game, std::string("Achievement unlocked: ") +
                            std::string(weaponDisplayName(weapon)) + " evolution!");
    }
}

void recordDashKill(Game& game, int32_t enemyKindIndex)
{
    Achievements& achievements = game.resources.achievements;
    if (enemyKindIndex < 0 || static_cast<size_t>(enemyKindIndex) >= enemyKinds.size())
    {
        return;
    }
    achievements.dashKillsByEnemyKind.at(static_cast<size_t>(enemyKindIndex))++;

    if (enemyKindIndex == enemyKindIndexByName("Turret"))
    {
        unlockType(game, achievements, WeaponType::Ricochet, "Ricochet");
    }
    else if (enemyKindIndex == enemyKindIndexByName("Orbiter"))
    {
        unlockType(game, achievements, WeaponType::FollowerDrone, "Follower Drone");
    }
    else if (enemyKindIndex == enemyKindIndexByName("Laser Fence"))
    {
        unlockType(game, achievements, WeaponType::LaserDrone, "Laser Drone");
    }
}

void recordDashOrNerveKill(Game& game)
{
    Achievements& achievements = game.resources.achievements;
    achievements.dashOrNerveKills++;
    if (achievements.dashOrNerveKills >= flakCannonUnlockKillCount)
    {
        unlockType(game, achievements, WeaponType::FlakCannon, "Flak Cannon");
    }
}

void recordSkillMaxed(Game& game, SkillType id)
{
    Achievements& achievements = game.resources.achievements;
    if (id == SkillType::ForwardShot)
    {
        unlockType(game, achievements, WeaponType::Railgun, "Railgun");
    }
    else if (id == SkillType::OrbitBlades)
    {
        unlockType(game, achievements, WeaponType::ChainLightning, "Chain Lightning");
    }
    else if (id == SkillType::BeamSweep)
    {
        unlockType(game, achievements, WeaponType::TurretDeploy, "Turret Deploy");
    }
    else if (id == SkillType::HomingMissiles)
    {
        unlockType(game, achievements, WeaponType::Flamethrower, "Flamethrower");
    }
}

void recordWaveReached(Game& game, int32_t wave)
{
    Achievements& achievements = game.resources.achievements;
    if (wave <= achievements.highestWaveReached)
    {
        return;
    }
    achievements.highestWaveReached = wave;

    bool changed = false;
    if (wave >= 3 && !achievements.typeUnlocked.at(static_cast<size_t>(WeaponType::Homing)))
    {
        achievements.typeUnlocked.at(static_cast<size_t>(WeaponType::Homing)) = true;
        showToast(game, "Achievement unlocked: Homing Missiles!");
        changed = true;
    }
    if (wave >= 8 && achievements.slotCap < 4)
    {
        achievements.slotCap = 4;
        showToast(game, "Achievement unlocked: 4th weapon slot!");
        changed = true;
    }
    if (wave >= 7 && !achievements.typeUnlocked.at(static_cast<size_t>(WeaponType::Mine)))
    {
        achievements.typeUnlocked.at(static_cast<size_t>(WeaponType::Mine)) = true;
        showToast(game, "Achievement unlocked: Mine Layer!");
        changed = true;
    }
    if (wave >= 12 && !achievements.typeUnlocked.at(static_cast<size_t>(WeaponType::Shock)))
    {
        achievements.typeUnlocked.at(static_cast<size_t>(WeaponType::Shock)) = true;
        showToast(game, "Achievement unlocked: Shockwave!");
        changed = true;
    }
    if (wave >= 35 && achievements.slotCap < 5)
    {
        achievements.slotCap = 5;
        showToast(game, "Achievement unlocked: 5th weapon slot!");
        changed = true;
    }
    if (wave >= 55 && achievements.slotCap < 6)
    {
        achievements.slotCap = 6;
        showToast(game, "Achievement unlocked: 6th weapon slot!");
        changed = true;
    }

    if (changed)
    {
        saveAchievements(achievements);
    }
}
