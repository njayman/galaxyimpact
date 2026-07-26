#include "democonfig.hpp"

#include <algorithm>

namespace DemoConfig
{

auto isBossTypeAllowed(std::size_t index) -> bool
{
    if (!isDemoBuild)
    {
        return true;
    }
    return std::find(allowedBossTypeIndices.begin(), allowedBossTypeIndices.end(), index) !=
           allowedBossTypeIndices.end();
}

auto isBossAttackAllowed(BossAttack attack) -> bool
{
    if (!isDemoBuild)
    {
        return true;
    }
    return std::find(allowedBossAttacks.begin(), allowedBossAttacks.end(), attack) !=
           allowedBossAttacks.end();
}

auto isWeaponAllowed(WeaponType type) -> bool
{
    if (!isDemoBuild)
    {
        return true;
    }
    return std::find(allowedWeapons.begin(), allowedWeapons.end(), type) != allowedWeapons.end();
}

} // namespace DemoConfig
