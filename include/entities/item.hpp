#pragma once

#include "palette.hpp"
#include "raylib.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace ItemConstants
{
constexpr int maxAbilitySlots = 6;
}

enum class PickupType : std::uint8_t
{
    XP,
    LifeOrb,
    Shield,
    Elemental,
    Regen,
    DashTrail,
    MagnetPulse,
    Overcharge,
    SecondWind,

    Overdrive,

    Danger,

    Count
};

enum class ElementType : std::uint8_t
{
    Static,
    Freeze,
    Burn,

    Count
};

enum class ElementMechanism : std::uint8_t
{
    Infusion,
    Nova,

    Count
};

class Pickup
{
  public:
    Vector2 position;
    int value;
    PickupType type;
    ElementType element;
    ElementMechanism mechanism;
    bool active;
    float lifetime;
    float maxLifetime;
};

constexpr std::array<std::string_view, static_cast<size_t>(ElementType::Count)> elementNames{
    "Static", "Freeze", "Burn"};

constexpr std::array<Color, static_cast<size_t>(ElementType::Count)> elementColors{
    Palette::ElementStatic, Palette::ElementFreeze, Palette::ElementBurn};

constexpr std::array<std::string_view, static_cast<size_t>(ElementMechanism::Count)>
    elementMechanismNames{"Infusion", "Nova"};

struct PickupCatalogEntry
{
    PickupType type;
    ElementType element;
    ElementMechanism mechanism;
    std::string_view name;
};

constexpr std::array<PickupCatalogEntry, 14> pickupCatalog{
    PickupCatalogEntry{PickupType::LifeOrb, ElementType::Static, ElementMechanism::Infusion,
                       "Life Orb"},
    PickupCatalogEntry{PickupType::Shield, ElementType::Static, ElementMechanism::Infusion,
                       "Shield"},
    PickupCatalogEntry{PickupType::Regen, ElementType::Static, ElementMechanism::Infusion, "Regen"},
    PickupCatalogEntry{PickupType::DashTrail, ElementType::Static, ElementMechanism::Infusion,
                       "Dash Trail"},
    PickupCatalogEntry{PickupType::MagnetPulse, ElementType::Static, ElementMechanism::Infusion,
                       "Magnet Pulse"},
    PickupCatalogEntry{PickupType::Overcharge, ElementType::Static, ElementMechanism::Infusion,
                       "Overcharge"},
    PickupCatalogEntry{PickupType::SecondWind, ElementType::Static, ElementMechanism::Infusion,
                       "Second Wind"},
    PickupCatalogEntry{PickupType::Overdrive, ElementType::Static, ElementMechanism::Infusion,
                       "Overdrive"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Static, ElementMechanism::Infusion,
                       "Static Infusion"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Static, ElementMechanism::Nova,
                       "Static Nova"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Freeze, ElementMechanism::Infusion,
                       "Freeze Infusion"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Freeze, ElementMechanism::Nova,
                       "Freeze Nova"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Burn, ElementMechanism::Infusion,
                       "Burn Infusion"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Burn, ElementMechanism::Nova,
                       "Burn Nova"},
};

enum class WeaponType : std::uint8_t
{
    Forward,
    Orbit,
    Homing,
    Mine,
    Beam,
    Shock,
    Ricochet,
    FollowerDrone,
    LaserDrone,
    FlakCannon,
    Railgun,
    ChainLightning,
    TurretDeploy,
    Flamethrower,
    Sniper,

    Count
};

class Weapon
{
  public:
    WeaponType type;
    int level;
    float timer;
    bool evolved;
    float flashTimer;

    bool disabled = false;
};

class WeaponDowngrade
{
  public:
    WeaponType type;
    bool unevolved = false;
    bool disabled = false;
    float timer;
};

enum class SkillType : std::uint8_t
{
    ForwardShot,
    OrbitBlades,
    HomingMissiles,
    MineLayers,
    BeamSweep,
    ShockWave,
    Ricochet,
    FollowerDrone,
    LaserDrone,
    FlakCannon,
    Railgun,
    ChainLightning,
    TurretDeploy,
    Flamethrower,
    Sniper,
    Damage,
    Barrier,
    Cooldown,
    PickupRadius,
    MoveSpeed,
    MaxHp,

    Count
};

struct Skill
{
    std::string_view name;
    std::string_view description;
    int maxLevel;
};

constexpr std::array<Skill, static_cast<size_t>(SkillType::Count)> Skills{
    Skill{.name = "Forward Shot",
          .description = "Fires toward your cursor. Levels add damage.",
          .maxLevel = 8},
    Skill{.name = "Orbit Blades",
          .description = "Pulses damage to anything circling you.",
          .maxLevel = 8},
    Skill{.name = "Homing Missiles",
          .description = "Auto-fires at the nearest enemy.",
          .maxLevel = 8},
    Skill{.name = "Mine Layer", .description = "Drops a scattering blast pulse.", .maxLevel = 8},
    Skill{.name = "Beam Sweep",
          .description = "Fires a piercing beam toward your cursor.",
          .maxLevel = 8},
    Skill{.name = "Shockwave",
          .description = "A slow, heavy-hitting pulse around you.",
          .maxLevel = 8},
    Skill{.name = "Ricochet",
          .description = "Single-hit projectiles bounce to extra enemies.",
          .maxLevel = 3},
    Skill{.name = "Follower Drone",
          .description = "A melee drone that orbits and auto-attacks.",
          .maxLevel = 4},
    Skill{.name = "Laser Drone",
          .description = "A ranged drone that beams the nearest enemy.",
          .maxLevel = 4},
    Skill{.name = "Flak Cannon",
          .description = "Forward cone burst with splash on impact.",
          .maxLevel = 3},
    Skill{
        .name = "Railgun", .description = "Slow, high-damage, fully-piercing bolt.", .maxLevel = 3},
    Skill{.name = "Chain Lightning",
          .description = "Arcs between nearby enemies on hit.",
          .maxLevel = 3},
    Skill{.name = "Turret Deploy",
          .description = "Deploys a stationary auto-firing turret.",
          .maxLevel = 3},
    Skill{.name = "Flamethrower", .description = "Continuous forward damage cone.", .maxLevel = 3},
    Skill{.name = "Sniper",
          .description = "Auto-fires a piercing bolt at the farthest enemy.",
          .maxLevel = 3},
    Skill{.name = "Warhead Tuning", .description = "+15% damage on all weapons.", .maxLevel = 5},
    Skill{.name = "Barrier Mastery",
          .description = "+shield duration, faster charge regen.",
          .maxLevel = 3},
    Skill{.name = "Overclock", .description = "-10% weapon cooldowns.", .maxLevel = 5},
    Skill{.name = "Tractor Beam", .description = "+20% pickup magnet radius.", .maxLevel = 5},
    Skill{.name = "Thrusters", .description = "+10% move speed.", .maxLevel = 5},
    Skill{.name = "Hull Plating", .description = "+1 max health, fully healed.", .maxLevel = 5},
};

constexpr std::array<SkillType, static_cast<size_t>(WeaponType::Count)> weaponGrantSkill{
    SkillType::ForwardShot,  SkillType::OrbitBlades,   SkillType::HomingMissiles,
    SkillType::MineLayers,   SkillType::BeamSweep,     SkillType::ShockWave,
    SkillType::Ricochet,     SkillType::FollowerDrone, SkillType::LaserDrone,
    SkillType::FlakCannon,   SkillType::Railgun,       SkillType::ChainLightning,
    SkillType::TurretDeploy, SkillType::Flamethrower,  SkillType::Sniper};

constexpr std::array<SkillType, static_cast<size_t>(WeaponType::Count)> skillLinkedPassive{
    SkillType::Damage,    SkillType::Barrier, SkillType::Cooldown, SkillType::PickupRadius,
    SkillType::MoveSpeed, SkillType::MaxHp,   SkillType::Damage,   SkillType::Damage,
    SkillType::Damage,    SkillType::Damage,  SkillType::Damage,   SkillType::Damage,
    SkillType::Damage,    SkillType::Damage,  SkillType::Damage};

constexpr std::array<std::string_view, static_cast<size_t>(WeaponType::Count)> evolvedWeaponName{
    "Photon Cannon", "Aegis Ring",      "Seeker Swarm",   "Cluster Charges", "Lance Sweep",
    "Bulwark Pulse", "Ricochet",        "Follower Drone", "Laser Drone",     "Flak Cannon",
    "Railgun",       "Chain Lightning", "Turret Deploy",  "Flamethrower",    "Sniper"};

enum class ChoiceType : std::uint8_t
{
    Skill,
    Evolve,
    Pickup
};

struct LevelUpChoice
{
    ChoiceType type = ChoiceType::Skill;
    SkillType skill = SkillType::ForwardShot;
    std::optional<WeaponType> weapon = std::nullopt;
    PickupType pickupType = PickupType::Regen;
    ElementType element = ElementType::Static;
    ElementMechanism mechanism = ElementMechanism::Infusion;
};

enum class DamageSource : std::uint8_t
{
    Forward,
    Orbit,
    Homing,
    Mine,
    Beam,
    Shock,
    Dash,
    Nerve,
    Elemental,
    FollowerDrone,
    LaserDrone,
    FlakCannon,
    Railgun,
    ChainLightning,
    TurretDeploy,
    Flamethrower,
    Sniper,

    Count
};

constexpr std::array<std::string_view, static_cast<size_t>(DamageSource::Count)> damageSourceNames{
    "Forward", "Orbit",           "Homing",        "Mine",           "Beam",        "Shock",
    "Dash",    "Nerve",           "Elemental",     "Follower Drone", "Laser Drone", "Flak Cannon",
    "Railgun", "Chain Lightning", "Turret Deploy", "Flamethrower",   "Sniper"};

struct DamageMeter
{
    std::array<int32_t, static_cast<size_t>(DamageSource::Count)> bySource{};
    int32_t total = 0;
};
