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
    WeaponType defaultWeapon;
    float orbitSpinMult;
    float bulletSpeedMult;
    float beamLengthMult;
    float forwardFireRateMult;
};

constexpr std::array<ShipDef, static_cast<size_t>(ShipClass::Count)> ships{
    ShipDef{.name = "Bastion",
            .description = "Orbit blades spin much faster.",
            .color = Palette::Shield,
            .radius = 15,
            .speed = 5,
            .maxHealth = 5,
            .armor = 1,
            .damageMult = 1.0F,
            .maxShieldStacks = 3,
            .dashDistanceMult = 1.0F,
            .defaultWeapon = WeaponType::Orbit,
            .orbitSpinMult = 2.2F,
            .bulletSpeedMult = 1.0F,
            .beamLengthMult = 1.0F,
            .forwardFireRateMult = 1.0F},
    ShipDef{.name = "Ranger",
            .description = "All-rounder. Faster bullets, faster forward fire rate.",
            .color = Palette::Accent,
            .radius = 15,
            .speed = 5,
            .maxHealth = 5,
            .armor = 1,
            .damageMult = 1.0F,
            .maxShieldStacks = 3,
            .dashDistanceMult = 1.0F,
            .defaultWeapon = WeaponType::Forward,
            .orbitSpinMult = 1.0F,
            .bulletSpeedMult = 1.4F,
            .beamLengthMult = 1.0F,
            .forwardFireRateMult = 2.0F},
    ShipDef{.name = "Interceptor",
            .description = "Longer laser.",
            .color = Palette::Crit,
            .radius = 15,
            .speed = 5,
            .maxHealth = 5,
            .armor = 1,
            .damageMult = 1.0F,
            .maxShieldStacks = 3,
            .dashDistanceMult = 1.0F,
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
