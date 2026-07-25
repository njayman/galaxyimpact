#pragma once

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

    Count
};

class Pickup
{
  public:
    Vector2 position;
    int value;
    PickupType type;
    bool active;
    float lifetime; // counts down; despawns at 0 if never collected
    float maxLifetime;
};

enum class WeaponType : std::uint8_t
{
    Forward,
    Orbit,
    Homing,
    Mine,
    Beam,
    Shock,

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

enum class SkillType : std::uint8_t
{
    ForwardShot,
    OrbitBlades,
    HomingMissiles,
    MineLayers,
    BeamSweep,
    ShockWave,
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
    SkillType::ForwardShot, SkillType::OrbitBlades, SkillType::HomingMissiles,
    SkillType::MineLayers,  SkillType::BeamSweep,   SkillType::ShockWave};

constexpr std::array<SkillType, static_cast<size_t>(WeaponType::Count)> skillLinkedPassive{
    SkillType::Damage,       SkillType::Barrier,   SkillType::Cooldown,
    SkillType::PickupRadius, SkillType::MoveSpeed, SkillType::MaxHp};

constexpr std::array<std::string_view, static_cast<size_t>(WeaponType::Count)> evolvedWeaponName{
    "Photon Cannon",   "Aegis Ring",  "Seeker Swarm",
    "Cluster Charges", "Lance Sweep", "Bulwark Pulse"};

enum class ChoiceType : std::uint8_t
{
    Skill,
    Evolve,
    LifeOrb,
    Shield
};

struct LevelUpChoice
{
    ChoiceType type = ChoiceType::Skill;
    SkillType skill = SkillType::ForwardShot;
    std::optional<WeaponType> weapon = std::nullopt;
    int count = 0;
};

// DamageSource identifies what dealt a hit, for the live damage meter (see
// DamageMeter). Mirrors WeaponType's order with one extra entry for dash
// contact damage, which isn't tied to a weapon.
enum class DamageSource : std::uint8_t
{
    Forward,
    Orbit,
    Homing,
    Mine,
    Beam,
    Shock,
    Dash,

    Count
};

constexpr std::array<std::string_view, static_cast<size_t>(DamageSource::Count)> damageSourceNames{
    "Forward", "Orbit", "Homing", "Mine", "Beam", "Shock", "Dash"};

// DamageMeter is the player's outgoing damage this frame only - reset at the
// top of every gameplay frame, populated as hits land, drawn as-is (see
// drawDamageMeter). Zero when nothing was hit this frame, by design.
struct DamageMeter
{
    std::array<int32_t, static_cast<size_t>(DamageSource::Count)> bySource{};
    int32_t total = 0;
};
