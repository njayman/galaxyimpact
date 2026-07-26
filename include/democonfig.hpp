#pragma once

#include "entities/boss.hpp"
#include "entities/item.hpp"
#include <array>
#include <cstddef>

// DemoConfig is the single place deciding what a demo build ships with -
// change isDemoBuild's definition, or the allowed* lists below, to change
// what's available. Every spot that samples boss types/attacks/weapons goes
// through isBossTypeAllowed/isBossAttackAllowed/isWeaponAllowed, so the full
// (non-demo) build gets everything for free without touching those call
// sites.
namespace DemoConfig
{
// The web build is the free-to-try slice; desktop (Steam/GOG) ships full.
#if defined(__EMSCRIPTEN__)
constexpr bool isDemoBuild = true;
#else
constexpr bool isDemoBuild = false;
#endif

// Indices into bossTypes (entities/boss.hpp): Warbringer, Beamforge.
constexpr std::array<std::size_t, 2> allowedBossTypeIndices{0, 11};

constexpr std::array<BossAttack, 4> allowedBossAttacks{
    BossAttack::Beam, BossAttack::Barrage, BossAttack::HomingBarrage, BossAttack::GravityWell};

constexpr std::array<WeaponType, 2> allowedWeapons{WeaponType::Forward, WeaponType::Homing};

auto isBossTypeAllowed(std::size_t index) -> bool;
auto isBossAttackAllowed(BossAttack attack) -> bool;
auto isWeaponAllowed(WeaponType type) -> bool;
} // namespace DemoConfig
