#include "bestiary.hpp"

#include "game.hpp"
#include "palette.hpp"
#include "platform.hpp"
#include <fstream>
#include <string>

namespace
{
auto bestiaryFilePath() -> std::string { return getSaveDataDir() + "bestiary.txt"; }
}

auto loadBestiary() -> Bestiary
{
    Bestiary bestiary{};

    std::ifstream file(bestiaryFilePath());
    if (!file)
    {
        return bestiary;
    }

    for (auto& killed : bestiary.enemyKilled)
    {
        if (int32_t value = 0; file >> value)
        {
            killed = value != 0;
        }
    }
    for (auto& killed : bestiary.bossKilled)
    {
        if (int32_t value = 0; file >> value)
        {
            killed = value != 0;
        }
    }
    for (auto& killed : bestiary.hazardKilled)
    {
        if (int32_t value = 0; file >> value)
        {
            killed = value != 0;
        }
    }

    return bestiary;
}

void saveBestiary(const Bestiary& bestiary)
{
    std::ofstream file(bestiaryFilePath(), std::ios::trunc);
    if (!file)
    {
        return;
    }

    for (const auto killed : bestiary.enemyKilled)
    {
        file << (killed ? 1 : 0) << ' ';
    }
    for (const auto killed : bestiary.bossKilled)
    {
        file << (killed ? 1 : 0) << ' ';
    }
    for (const auto killed : bestiary.hazardKilled)
    {
        file << (killed ? 1 : 0) << ' ';
    }

    file.close();
    platformSyncSaveData();
}

auto bossBestiaryIndex(const Boss& boss) -> int32_t
{
    if (boss.isBeltbreaker)
    {
        return bestiaryBossBeltbreaker;
    }
    if (boss.isWreckwormHead)
    {
        return bestiaryBossWreckworm;
    }
    if (boss.isSlagmaw)
    {
        return bestiaryBossSlagmaw;
    }
    if (boss.isKraken)
    {
        return bestiaryBossKraken;
    }
    if (boss.isBanished)
    {
        return bestiaryBossBanished;
    }
    return boss.typeIndex;
}

auto bossBestiaryName(int32_t index) -> std::string_view
{
    if (index >= 0 && index < bestiaryGenericBossCount)
    {
        return bossTypes.at(static_cast<size_t>(index)).name;
    }
    if (index == bestiaryBossBeltbreaker)
    {
        return "Beltbreaker";
    }
    if (index == bestiaryBossWreckworm)
    {
        return "Wreckworm";
    }
    if (index == bestiaryBossSlagmaw)
    {
        return "Slagmaw";
    }
    if (index == bestiaryBossKraken)
    {
        return "Kraken";
    }
    return "Banished";
}

auto bossBestiaryShape(int32_t index) -> BossShape
{
    if (index >= 0 && index < bestiaryGenericBossCount)
    {
        return bossTypes.at(static_cast<size_t>(index)).shape;
    }
    if (index == bestiaryBossWreckworm)
    {
        return BossShape::Segment;
    }
    return BossShape::HexPlated;
}

auto bossBestiaryColor(int32_t index) -> Color
{
    if (index >= 0 && index < bestiaryGenericBossCount)
    {
        return bossTypes.at(static_cast<size_t>(index)).color;
    }
    if (index == bestiaryBossBeltbreaker)
    {
        return Palette::StructMid;
    }
    if (index == bestiaryBossWreckworm)
    {
        return Palette::RustbloomAccent;
    }
    if (index == bestiaryBossSlagmaw)
    {
        return Palette::Charge;
    }
    if (index == bestiaryBossKraken)
    {
        return Palette::PunctumHaze;
    }
    return Palette::Crit;
}

auto hazardBestiaryName(EliteHazardRole role) -> std::string_view
{
    return role == EliteHazardRole::Warlord ? "Warlord" : "Suppressor";
}

void recordEnemyKilled(Game& game, int32_t enemyKindIndex)
{
    if (enemyKindIndex < 0 || static_cast<size_t>(enemyKindIndex) >= enemyKinds.size())
    {
        return;
    }
    auto& killed = game.resources.bestiary.enemyKilled.at(static_cast<size_t>(enemyKindIndex));
    if (killed)
    {
        return;
    }
    killed = true;
    saveBestiary(game.resources.bestiary);
}

void recordBossKilled(Game& game, const Boss& boss)
{
    const int32_t index = bossBestiaryIndex(boss);
    if (index < 0 || static_cast<size_t>(index) >= bestiaryBossKindCount)
    {
        return;
    }
    auto& killed = game.resources.bestiary.bossKilled.at(static_cast<size_t>(index));
    if (killed)
    {
        return;
    }
    killed = true;
    saveBestiary(game.resources.bestiary);
}

void recordHazardKilled(Game& game, EliteHazardRole role)
{
    auto& killed = game.resources.bestiary.hazardKilled.at(static_cast<size_t>(role));
    if (killed)
    {
        return;
    }
    killed = true;
    saveBestiary(game.resources.bestiary);
}
