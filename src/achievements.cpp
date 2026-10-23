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
            }
            for (size_t i = 0; i < e; i++)
            {
                a.dashKillsByEnemyKind.at(i) = v.at(w + i);
            }
            a.dashOrNerveKills = v.at(stats);
            a.highestWaveReached = v.at(stats + 1);
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
    if (marker != "v2" && marker != "v3")
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
    if (marker == "v2")
    {
        // Before the skill tree: the old direct unlocks are dropped (progress resets), stats stay.
        std::array<int32_t, 2 * static_cast<size_t>(WeaponType::Count) + 1> oldUnlocks{};
        int32_t slotCap = 0;
        file >> slotCap;
        readSizedInts(file, oldUnlocks);
        readSizedInts(file, oldUnlocks);
        if (int32_t value = 0; file >> value)
        {
            achievements.infiniteModeUnlocked = value != 0;
        }
        return achievements;
    }
    if (int32_t value = 0; file >> value)
    {
        achievements.infiniteModeUnlocked = value != 0;
    }
    readSizedInts(file, achievements.earned);
    readSizedInts(file, achievements.owned);

    return achievements;
}

void saveAchievements(const Achievements& achievements)
{
    std::ofstream file(achievementsFilePath(), std::ios::trunc);
    if (!file)
    {
        return;
    }

    file << "v3";
    writeSizedInts(file, achievements.killsByWeapon);
    writeSizedInts(file, achievements.dashKillsByEnemyKind);
    file << ' ' << achievements.dashOrNerveKills << ' ' << achievements.highestWaveReached << ' '
         << (achievements.infiniteModeUnlocked ? 1 : 0);
    writeSizedInts(file, achievements.earned);
    writeSizedInts(file, achievements.owned);

    file.close();
    platformSyncSaveData();
}

// The skill tree gates weapons, evolutions and slots; the sandbox treats everything as unlocked.
// A ship's signature weapon comes with the ship.
auto isWeaponTypeUnlocked(const Game& game, WeaponType weapon) -> bool
{
    if (game.sandbox || weapon == WeaponType::Forward || currentShip(game).defaultWeapon == weapon)
    {
        return true;
    }
    for (int32_t n = 0; n < skillNodeCount; n++)
    {
        const SkillNode& node = skillTree.at(static_cast<size_t>(n));
        const bool unlocks = (node.kind == NodeKind::Weapon && node.value == static_cast<int32_t>(weapon)) ||
                             (node.kind == NodeKind::Ship &&
                              ships.at(static_cast<size_t>(node.value)).defaultWeapon == weapon);
        if (unlocks && skillNodeOwned(game, n))
        {
            return true;
        }
    }
    return false;
}

auto isEvolutionUnlocked(const Game& game, WeaponType weapon) -> bool
{
    if (game.sandbox)
    {
        return true;
    }
    for (int32_t n = 0; n < skillNodeCount; n++)
    {
        const SkillNode& node = skillTree.at(static_cast<size_t>(n));
        if (node.kind == NodeKind::Evolution && node.value == static_cast<int32_t>(weapon))
        {
            return skillNodeOwned(game, n);
        }
    }
    return false;
}

auto weaponSlotCap(const Game& game) -> int32_t
{
    if (game.sandbox)
    {
        return maxWeaponSlotCap;
    }
    int32_t cap = 3;
    for (int32_t n = 0; n < skillNodeCount; n++)
    {
        const SkillNode& node = skillTree.at(static_cast<size_t>(n));
        if (node.kind == NodeKind::Slot && skillNodeOwned(game, n))
        {
            cap = std::max(cap, node.value);
        }
    }
    return cap;
}

void recordWeaponKill(Game& game, WeaponType weapon)
{
    Achievements& achievements = game.resources.achievements;
    const auto idx = static_cast<size_t>(weapon);
    achievements.killsByWeapon.at(idx)++;
    if (achievements.killsByWeapon.at(idx) >= evolutionKillThreshold)
    {
        earnAchievement(game, achievementWeaponKills + static_cast<int32_t>(weapon));
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
        earnAchievement(game, achievementDashTurret);
    }
    else if (enemyKindIndex == enemyKindIndexByName("Orbiter"))
    {
        earnAchievement(game, achievementDashOrbiter);
    }
    else if (enemyKindIndex == enemyKindIndexByName("Laser Fence"))
    {
        earnAchievement(game, achievementDashLaserFence);
    }
}

void recordDashOrNerveKill(Game& game)
{
    Achievements& achievements = game.resources.achievements;
    achievements.dashOrNerveKills++;
    if (achievements.dashOrNerveKills >= flakCannonUnlockKillCount)
    {
        earnAchievement(game, achievementDashNerveKills);
    }
}

void recordSkillMaxed(Game& game, SkillType id)
{
    for (size_t w = 0; w < weaponGrantSkill.size(); w++)
    {
        if (weaponGrantSkill.at(w) == id)
        {
            earnAchievement(game, achievementWeaponMaxed + static_cast<int32_t>(w));
        }
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
    saveAchievements(achievements);
    for (size_t i = 0; i < achievementWaveMilestones.size(); i++)
    {
        if (wave >= achievementWaveMilestones.at(i))
        {
            earnAchievement(game, achievementWaves + static_cast<int32_t>(i));
        }
    }
}
