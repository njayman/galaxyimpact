#pragma once

#include "entities/boss.hpp"
#include "entities/enemy.hpp"
#include <array>
#include <cstdint>
#include <string_view>

constexpr int32_t bestiaryGenericBossCount = static_cast<int32_t>(bossTypes.size());
constexpr int32_t bestiaryBossBeltbreaker = bestiaryGenericBossCount + 0;
constexpr int32_t bestiaryBossWreckworm = bestiaryGenericBossCount + 1;
constexpr int32_t bestiaryBossSlagmaw = bestiaryGenericBossCount + 2;
constexpr int32_t bestiaryBossKraken = bestiaryGenericBossCount + 3;
constexpr int32_t bestiaryBossBanished = bestiaryGenericBossCount + 4;
constexpr size_t bestiaryBossKindCount = static_cast<size_t>(bestiaryGenericBossCount) + 5;
constexpr size_t bestiaryHazardKindCount = 2;

struct Bestiary
{
    std::array<bool, enemyKinds.size()> enemyKilled{};
    std::array<bool, bestiaryBossKindCount> bossKilled{};
    std::array<bool, bestiaryHazardKindCount> hazardKilled{};
};

auto loadBestiary() -> Bestiary;
void saveBestiary(const Bestiary& bestiary);

auto bossBestiaryIndex(const Boss& boss) -> int32_t;
auto bossBestiaryName(int32_t index) -> std::string_view;
auto bossBestiaryShape(int32_t index) -> BossShape;
auto bossBestiaryColor(int32_t index) -> Color;
auto hazardBestiaryName(EliteHazardRole role) -> std::string_view;
