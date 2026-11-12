#include "bestiary.hpp"

#include "game.hpp"
#include "palette.hpp"
#include "platform.hpp"
#include "progression.hpp"
#include <algorithm>
#include <fstream>
#include <vector>
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

    // The file is enemies, bosses, hazards with no counts. New Enemies are appended to the kind
    // list, so a save from before one was added is short by that many enemy flags: the bosses and
    // hazards are the tail, whatever is in front of them is the enemies.
    std::vector<int32_t> values;
    for (int32_t value = 0; file >> value;)
    {
        values.push_back(value);
    }
    const size_t tail = bestiary.bossKilled.size() + bestiary.hazardKilled.size();
    const size_t enemies = values.size() > tail ? std::min(values.size() - tail, bestiary.enemyKilled.size()) : 0;
    for (size_t i = 0; i < enemies; i++)
    {
        bestiary.enemyKilled.at(i) = values.at(i) != 0;
    }
    for (size_t i = 0; i < tail && enemies + i < values.size(); i++)
    {
        const bool killed = values.at(enemies + i) != 0;
        if (i < bestiary.bossKilled.size())
        {
            bestiary.bossKilled.at(i) = killed;
        }
        else
        {
            bestiary.hazardKilled.at(i - bestiary.bossKilled.size()) = killed;
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
        return Palette::ShatteredBeltAccent;
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
    if (boss.isBeltbreaker)
    {
        earnBossAchievement(game, BossAchievement::Beltbreaker);
    }
    else if (boss.isWreckwormHead && !boss.krakenSnakeVariant)
    {
        earnBossAchievement(game, BossAchievement::Wreckworm);
    }
    else if (boss.isSlagmaw)
    {
        earnBossAchievement(game, BossAchievement::Slagmaw);
    }
    else if (boss.isKraken)
    {
        earnBossAchievement(game, BossAchievement::Kraken);
    }
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
