#pragma once

#include "entities/item.hpp"
#include "game.hpp"
#include "palette.hpp"
#include "raylib.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

enum class ShipClass : std::uint8_t
{
    Bastion,
    Ranger,
    Interceptor,

    Count
};

enum class DashQuirk : std::uint8_t
{
    Push,
    Hybrid,
    Damage
};

struct ShipDef
{
    std::string_view name;
    std::string_view description;
    Color color;
    float radius;
    float speed;
    float maxHealth;
    float armor;
    float damageMult;
    int32_t maxShieldStacks;
    float dashDistanceMult;
    DashQuirk dashQuirk;
    WeaponType defaultWeapon;
    float orbitSpinMult;
    float bulletSpeedMult;
    float beamLengthMult;
    float forwardFireRateMult;
};

constexpr std::array<ShipDef, static_cast<size_t>(ShipClass::Count)> ships{
    ShipDef{.name = "Bastion",
            .description =
                "Heavy hull. Shield-dash shoves enemies aside instead of cutting through. "
                "Orbit blades spin much faster.",
            .color = Palette::Shield,
            .radius = 18,
            .speed = 4,
            .maxHealth = 8,
            .armor = 2,
            .damageMult = 0.8F,
            .maxShieldStacks = 4,
            .dashDistanceMult = 0.75F,
            .dashQuirk = DashQuirk::Push,
            .defaultWeapon = WeaponType::Orbit,
            .orbitSpinMult = 2.2F,
            .bulletSpeedMult = 1.0F,
            .beamLengthMult = 1.0F,
            .forwardFireRateMult = 1.0F},
    ShipDef{.name = "Ranger",
            .description =
                "All-rounder. Dash both pushes and damages. Faster bullets, faster forward fire rate.",
            .color = Palette::Accent,
            .radius = 15,
            .speed = 5,
            .maxHealth = 5,
            .armor = 1,
            .damageMult = 1.0F,
            .maxShieldStacks = 3,
            .dashDistanceMult = 1.0F,
            .dashQuirk = DashQuirk::Hybrid,
            .defaultWeapon = WeaponType::Forward,
            .orbitSpinMult = 1.0F,
            .bulletSpeedMult = 1.4F,
            .beamLengthMult = 1.0F,
            .forwardFireRateMult = 2.0F},
    ShipDef{.name = "Interceptor",
            .description =
                "Light frame. Dash cuts through everything in its path and reaches further. "
                "Longer laser.",
            .color = Palette::Crit,
            .radius = 12,
            .speed = 6.5F,
            .maxHealth = 4,
            .armor = 0,
            .damageMult = 1.3F,
            .maxShieldStacks = 2,
            .dashDistanceMult = 1.3F,
            .dashQuirk = DashQuirk::Damage,
            .defaultWeapon = WeaponType::Beam,
            .orbitSpinMult = 1.0F,
            .bulletSpeedMult = 1.0F,
            .beamLengthMult = 1.5F,
            .forwardFireRateMult = 1.0F},
};

inline auto currentShip(const Game& game) -> const ShipDef&
{
    return ships.at(static_cast<size_t>(game.resources.settings.shipIndex));
}
