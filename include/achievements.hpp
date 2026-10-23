#pragma once

#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "progression.hpp"
#include <array>
#include <cstdint>
#include <string_view>

struct Achievements
{
    std::array<int32_t, static_cast<size_t>(WeaponType::Count)> killsByWeapon{};
    std::array<int32_t, enemyKinds.size()> dashKillsByEnemyKind{};
    int32_t dashOrNerveKills{};
    int32_t highestWaveReached{};
    std::array<bool, achievementCount> earned{};    // each is worth one skill point
    std::array<bool, skillNodeCount> owned{};       // skill tree nodes bought

    bool infiniteModeUnlocked{false};
};

auto loadAchievements() -> Achievements;
void saveAchievements(const Achievements& achievements);
auto weaponDisplayName(WeaponType weapon) -> std::string_view;
auto effectiveWeaponName(const Weapon& weapon) -> std::string_view;

constexpr int32_t evolutionKillThreshold = 500;
constexpr int32_t maxWeaponSlotCap = 6;
constexpr int32_t flakCannonUnlockKillCount = 200;
