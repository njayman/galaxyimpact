#pragma once

#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include <array>
#include <cstdint>
#include <string_view>

// Persistent, cross-run progression state (M9): weapon-slot cap, which weapon/ability types and
// evolutions are unlocked, and the raw counters that unlock them. Lives in GameResources, loaded
// once at startup, saved immediately whenever a threshold is newly crossed.
struct Achievements
{
    std::array<int32_t, static_cast<size_t>(WeaponType::Count)> killsByWeapon{};
    std::array<int32_t, enemyKinds.size()> dashKillsByEnemyKind{};
    int32_t dashOrNerveKills{};
    int32_t highestWaveReached{};
    int32_t slotCap{3};
    std::array<bool, static_cast<size_t>(WeaponType::Count)> typeUnlocked{};
    std::array<bool, static_cast<size_t>(WeaponType::Count)> evolutionUnlocked{};

    // M11: true once the wave-100 placeholder final boss has been defeated at least once.
    bool infiniteModeUnlocked{false};
};

auto loadAchievements() -> Achievements;
void saveAchievements(const Achievements& achievements);
auto weaponDisplayName(WeaponType weapon) -> std::string_view;
