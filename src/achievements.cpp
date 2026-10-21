#include "achievements.hpp"

#include "entities/item.hpp"
#include "entities/ship.hpp"
#include "game.hpp"
#include "platform.hpp"
#include <algorithm>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

namespace
{
// Reads `count` ints into arr, ignoring any past its size (a newer file) and leaving the defaults
// for any missing (an older file).
template <typename T, size_t N>
void readInts(std::istream& file, std::array<T, N>& arr, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        int32_t value = 0;
        if (!(file >> value))
        {
            return;
        }
        if (i < N)
        {
            arr.at(i) = static_cast<T>(value);
        }
    }
}

template <typename T, size_t N>
void readSizedInts(std::istream& file, std::array<T, N>& arr)
{
    if (size_t count = 0; file >> count)
    {
        readInts(file, arr, count);
    }
}

template <typename T, size_t N>
void writeSizedInts(std::ostream& file, const std::array<T, N>& arr)
{
    file << ' ' << N;
    for (const T value : arr)
    {
        file << ' ' << static_cast<int32_t>(value);
    }
}

// Unversioned saves stored arrays back to back with no lengths: W weapon kills, E dash kills,
// 3 stats, W type unlocks, W evolution unlocks, then (in later files) the infinite-mode flag.
// The weapon/enemy counts changed between releases, so infer them: try weapon counts from
// newest down and take the first layout whose total length fits and whose 0/1 flags and slot
// cap are valid. Returns nullopt if nothing fits.
auto parseLegacyAchievements(const std::vector<int32_t>& v) -> std::optional<Achievements>
{
    constexpr size_t newestLegacyWeaponCount = 15; // before Laser Shots
    constexpr size_t oldestWeaponCount = 6;
    const auto isFlag = [](int32_t x) { return x == 0 || x == 1; };
    for (size_t w = newestLegacyWeaponCount; w >= oldestWeaponCount; w--)
    {
        for (const size_t flag : {1U, 0U})
        {
            if (v.size() < 3 * w + 3 + flag)
            {
                continue;
            }
            const size_t e = v.size() - 3 * w - 3 - flag;
            if (e > enemyKinds.size())
            {
                continue;
            }
            const size_t stats = w + e;
            const size_t types = stats + 3;
            const size_t evos = types + w;
            const int32_t slotCap = v.at(stats + 2);
            const bool flagsValid =
                std::all_of(v.begin() + static_cast<std::ptrdiff_t>(types),
                            v.begin() + static_cast<std::ptrdiff_t>(evos + w + flag), isFlag);
            if (!flagsValid || slotCap < 3 || slotCap > 6)
            {
                continue;
            }

            Achievements a{};
            for (size_t i = 0; i < w && i < a.killsByWeapon.size(); i++)
            {
                a.killsByWeapon.at(i) = v.at(i);
                a.typeUnlocked.at(i) = v.at(types + i) != 0;
                a.evolutionUnlocked.at(i) = v.at(evos + i) != 0;
            }
            for (size_t i = 0; i < e; i++)
            {
                a.dashKillsByEnemyKind.at(i) = v.at(w + i);
            }
            a.dashOrNerveKills = v.at(stats);
            a.highestWaveReached = v.at(stats + 1);
            a.slotCap = slotCap;
            a.infiniteModeUnlocked = flag != 0 && v.back() != 0;
            return a;
        }
    }
    return std::nullopt;
}
} // namespace

auto loadAchievements() -> Achievements
{
    Achievements achievements{};

    std::ifstream file(achievementsFilePath());
    if (!file)
    {
        return achievements;
    }

    std::string marker;
    file >> marker;
    if (marker != "v2")
    {
        file.clear();
        file.seekg(0);
        std::vector<int32_t> values;
        for (int32_t value = 0; file >> value;)
        {
            values.push_back(value);
        }
        return parseLegacyAchievements(values).value_or(achievements);
    }

    readSizedInts(file, achievements.killsByWeapon);
    readSizedInts(file, achievements.dashKillsByEnemyKind);
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
    readSizedInts(file, achievements.typeUnlocked);
    readSizedInts(file, achievements.evolutionUnlocked);
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

    file << "v2";
    writeSizedInts(file, achievements.killsByWeapon);
    writeSizedInts(file, achievements.dashKillsByEnemyKind);
    file << ' ' << achievements.dashOrNerveKills << ' ' << achievements.highestWaveReached << ' '
         << achievements.slotCap;
    writeSizedInts(file, achievements.typeUnlocked);
    writeSizedInts(file, achievements.evolutionUnlocked);
    file << ' ' << (achievements.infiniteModeUnlocked ? 1 : 0);

    file.close();
    platformSyncSaveData();
}

// Sandbox treats every weapon, evolution and slot as unlocked without touching the save.
auto isWeaponTypeUnlocked(const Game& game, WeaponType weapon) -> bool
{
    if (game.sandbox || currentShip(game).defaultWeapon == weapon)
    {
        return true;
    }
    return game.resources.achievements.typeUnlocked.at(static_cast<size_t>(weapon));
}

auto isEvolutionUnlocked(const Game& game, WeaponType weapon) -> bool
{
    return game.sandbox ||
           game.resources.achievements.evolutionUnlocked.at(static_cast<size_t>(weapon));
}

auto weaponSlotCap(const Game& game) -> int32_t
{
    return game.sandbox ? maxWeaponSlotCap : game.resources.achievements.slotCap;
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
    else if (id == SkillType::Railgun)
    {
        unlockType(game, achievements, WeaponType::Sniper, "Sniper");
    }
    else if (id == SkillType::LaserShots)
    {
        unlockType(game, achievements, WeaponType::Beam, "Beam Sweep");
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
