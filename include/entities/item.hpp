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
    // Temporary buff: dash and shield-block cost no ability charge for the duration (see
    // Player::overdriveTimer, updateAbilityCharges).
    Overdrive,
    // M23: risk-reward world pickup, visually distinct (bad colors/shapes, hazard-enemy visual
    // language) — never offered as a level-up/reward choice, only spawns as a rare world drop.
    Danger,

    Count
};

// Static/Freeze/Burn/Confuse — permanent-until-death debuffs applied to regular enemies only
// (bosses and elite hazards are immune). See ElementMechanism for how a pickup delivers one.
enum class ElementType : std::uint8_t
{
    Static,
    Freeze,
    Burn,
    Confuse,

    Count
};

// Three different ways an Elemental pickup can deliver its ElementType to enemies.
enum class ElementMechanism : std::uint8_t
{
    Infusion, // Grants the player a timed buff: their hits apply the debuff while it's active.
    Nova,     // Instantly applies the debuff to every enemy on the field, once.
    Field,    // Drops a lingering zone that applies the debuff to anything that touches it.

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

// The Field mechanism's lingering zone: applies `element` once to any enemy that touches it.
class ElementalField
{
  public:
    Vector2 position;
    float radius;
    float timer;
    ElementType element;
    bool active;
};

constexpr std::array<std::string_view, static_cast<size_t>(ElementType::Count)> elementNames{
    "Static", "Freeze", "Burn", "Confuse"};

constexpr std::array<Color, static_cast<size_t>(ElementType::Count)> elementColors{
    Palette::ElementStatic, Palette::ElementFreeze, Palette::ElementBurn, Palette::ElementConfuse};

constexpr std::array<std::string_view, static_cast<size_t>(ElementMechanism::Count)>
    elementMechanismNames{"Infusion", "Nova", "Field"};

// One entry per selectable non-XP pickup variant — shared by the level-up "Pickup" choice, the
// post-cap reward pool, and the sandbox pickup dropper, so there's a single list to maintain.
struct PickupCatalogEntry
{
    PickupType type;
    ElementType element;
    ElementMechanism mechanism;
    std::string_view name;
};

constexpr std::array<PickupCatalogEntry, 20> pickupCatalog{
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
    PickupCatalogEntry{PickupType::Elemental, ElementType::Static, ElementMechanism::Field,
                       "Static Field"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Freeze, ElementMechanism::Infusion,
                       "Freeze Infusion"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Freeze, ElementMechanism::Nova,
                       "Freeze Nova"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Freeze, ElementMechanism::Field,
                       "Freeze Field"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Burn, ElementMechanism::Infusion,
                       "Burn Infusion"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Burn, ElementMechanism::Nova,
                       "Burn Nova"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Burn, ElementMechanism::Field,
                       "Burn Field"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Confuse, ElementMechanism::Infusion,
                       "Confuse Infusion"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Confuse, ElementMechanism::Nova,
                       "Confuse Nova"},
    PickupCatalogEntry{PickupType::Elemental, ElementType::Confuse, ElementMechanism::Field,
                       "Confuse Field"},
};

// M15: Ricochet/FollowerDrone/LaserDrone are amplifier abilities rather than firing weapons, but
// share the same slot/skill/level machinery as the 6 original weapons (see grantOrLevelWeapon) —
// none of the 8 additions ever evolves, so their `skillLinkedPassive`/`evolvedWeaponName` entries
// below are inert placeholders (evolutionUnlocked never gets set for them).
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
};

// M23: tracks the one active Danger-pickup weapon downgrade (if any). `amount` is the raw level
// deduction applied, restored verbatim when `timer` runs out rather than snapshotting/restoring
// an absolute level — simpler, and correct even if the weapon keeps leveling up mid-debuff.
class WeaponDowngrade
{
  public:
    WeaponType type;
    int32_t amount;
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
    SkillType::TurretDeploy, SkillType::Flamethrower};

// Placeholder for the 8 M15 additions (indices 6-13) — never read since their `evolutionUnlocked`
// flag never gets set, so the evolve-eligibility check in rollLevelUpChoices never passes for them.
constexpr std::array<SkillType, static_cast<size_t>(WeaponType::Count)> skillLinkedPassive{
    SkillType::Damage,    SkillType::Barrier, SkillType::Cooldown, SkillType::PickupRadius,
    SkillType::MoveSpeed, SkillType::MaxHp,   SkillType::Damage,   SkillType::Damage,
    SkillType::Damage,    SkillType::Damage,  SkillType::Damage,   SkillType::Damage,
    SkillType::Damage,    SkillType::Damage};

constexpr std::array<std::string_view, static_cast<size_t>(WeaponType::Count)> evolvedWeaponName{
    "Photon Cannon", "Aegis Ring",      "Seeker Swarm",   "Cluster Charges", "Lance Sweep",
    "Bulwark Pulse", "Ricochet",        "Follower Drone", "Laser Drone",     "Flak Cannon",
    "Railgun",       "Chain Lightning", "Turret Deploy",  "Flamethrower"};

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

    Count
};

constexpr std::array<std::string_view, static_cast<size_t>(DamageSource::Count)> damageSourceNames{
    "Forward", "Orbit",           "Homing",        "Mine",           "Beam",        "Shock",
    "Dash",    "Nerve",           "Elemental",     "Follower Drone", "Laser Drone", "Flak Cannon",
    "Railgun", "Chain Lightning", "Turret Deploy", "Flamethrower"};

struct DamageMeter
{
    std::array<int32_t, static_cast<size_t>(DamageSource::Count)> bySource{};
    int32_t total = 0;
};
