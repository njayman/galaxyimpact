#pragma once

#include "entities/boss.hpp"
#include "entities/item.hpp"
#include <array>
#include <cstddef>

namespace DemoConfig
{

#if defined(__EMSCRIPTEN__)
constexpr bool isDemoBuild = true;
#else
constexpr bool isDemoBuild = false;
#endif

constexpr std::array<std::size_t, 2> allowedBossTypeIndices{0, 11};

constexpr std::array<BossAttack, 4> allowedBossAttacks{
    BossAttack::Beam, BossAttack::Barrage, BossAttack::HomingBarrage, BossAttack::GravityWell};

constexpr std::array<WeaponType, 2> allowedWeapons{WeaponType::Forward, WeaponType::Homing};

auto isBossTypeAllowed(std::size_t index) -> bool;
auto isBossAttackAllowed(BossAttack attack) -> bool;
auto isWeaponAllowed(WeaponType type) -> bool;
}
