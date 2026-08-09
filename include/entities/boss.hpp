#pragma once

#include "palette.hpp"
#include "raylib.h"
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace bossConstants
{
constexpr float maxSlamRadius = 1000;
}

enum class BossShape : std::uint8_t
{
    Saucer,
    SpikedRing,
    TwinDome,
    Crystal,
    HexPlated,
    // The Wreckworm (head and every chain segment alike): an organic hull, deliberately the
    // opposite read of the geometric/mechanical Beltbreaker.
    Segment
};

enum class BossState : std::uint8_t
{
    IDLE,
    WINDING_UP,
    SHOOTING
};

enum class BossAttack : std::uint8_t
{
    Beam,
    Spread,
    Slam,
    WormholeBeam,
    MineDrop,
    ChargeDash,
    SummonAdds,
    ShockwaveStomp,
    Barrage,
    GravityWell,
    HomingBarrage,
    // The Beltbreaker core only (see spawnBeltbreaker's moveset): launches every currently
    // attached, full-health plate at the player as a fast dash, then forces them home shortly
    // after. See the BossAttack::PlateHurl handling in updateBoss/startBossAttack.
    PlateHurl,
    // The Wreckworm head only: teleports to a random point well outside the player's view then
    // charges straight through (and past) them at high speed, dragging its chain along behind it
    // via the normal snake-follow code. See updateWreckwormChain / the BurrowCharge handling in
    // updateBoss/startBossAttack.
    BurrowCharge,
    // The Wreckworm head only: its chain segments arc into a partial ring (one gap) around the
    // player's position and contract - reuses the generic boss/player body-contact code for the
    // actual damage, this attack just drives segment positions. See the CoilClamp handling in
    // updateBoss/startBossAttack.
    CoilClamp,

    Count
};

constexpr int bossAttackCount = static_cast<int>(BossAttack::Count);

constexpr std::array<std::string_view, bossAttackCount> bossAttackNames{
    "Beam",          "Spread",     "Slam",         "WormholeBeam", "MineDrop",   "ChargeDash",
    "SummonAdds",    "ShockwaveStomp", "Barrage", "GravityWell",  "HomingBarrage",
    "PlateHurl",     "BurrowCharge", "CoilClamp"};

// Every field is still value-initialized via aggregate init at every construction site; the
// missing-member-init check below is a false positive triggered only by isFinalBoss's default.
// Field order otherwise follows logical grouping (core / Beltbreaker-core / Beltbreaker-plate /
// debuffs) rather than size, which every designated-init call site below relies on staying in
// this exact declaration order - not reordering for the padding check's sake.
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class Boss
{
  public:
    Vector2 position;
    Vector2 size;
    Color color;
    Color baseColor;
    int health;
    int maxHealth;
    BossState state;
    BossAttack attack;
    std::vector<BossAttack> moveset;
    float attackTimer;
    float stateTimer;
    Vector2 targetPosition;
    bool slamHit;
    bool beamShieldLatched;
    Vector2 wormholeBeamOrigin;
    Vector2 chargeVelocity;
    float barrageTimer;
    int spreadWindupShots;
    bool hitByDash;
    bool isMega;
    bool isSwarm;
    float strafePhase;
    float hitFlashTimer;
    BossShape shape;
    bool orbitContact;
    float orbitDamageAccum;
    bool beamContact;
    float beamDamageAccum;

    // M12 dodge-window fix: a brief forced-still period after SHOOTING resolves, on top of
    // SHOOTING itself always holding position now — extends the hit window past the attack's own
    // duration instead of the boss immediately resuming its evasive strafe.
    float recoveryTimer{};

    // M11: the wave-100 (and every 100th wave after, once infinite mode is unlocked) placeholder
    // final boss. Defeating it flips Achievements::infiniteModeUnlocked.
    bool isFinalBoss = false;

    // The Beltbreaker (Shattered Belt signature boss, waves 10/20/25): a core plus its own plates
    // - each plate is a REAL Boss entry in game.run.bosses (isBeltbreakerPlate), not a synthetic
    // hit-test or a separate Enemy. That gets a plate full weapon collision for free (every
    // damageBoss call site already loops the whole bosses vector) and makes ownership an explicit
    // stored id instead of "whichever boss happens to be nearest" - see plateOwnerId.
    //
    // Cycle: plates orbit the core (plateAttached) and fully block its damage while any of them
    // are alive and attached. shieldGenProgress ticks up at a rate scaled by
    // (alive attached plates / plateCount) - kill plates and it crawls. At 1.0 the core goes
    // beltbreakerShielded (fully invulnerable on its own) and every attached plate detaches,
    // becoming an independent boss (its own moveset - see spawnBeltbreaker) that chases and
    // attacks the player using the same generic boss movement/attack code as any other boss.
    // Shortly before beltbreakerShieldTimer runs out, detached plates get called home
    // (plateReturning) to their own slot's live orbit position, reattach, and heal back to full.
    bool isBeltbreaker = false;
    int32_t plateCount = 0;
    float plateAngleDeg = 0;
    float plateRotationSpeed = 0;
    bool beltbreakerShielded = false;
    float beltbreakerShieldTimer = 0;
    float shieldGenProgress = 0;
    bool beltbreakerReturnTriggered = false;
    // Stable per-instance id (Game::bossSpawnCount at spawn time, never reused).
    int32_t instanceId = -1;
    // Core-only: countdown to the next "whim" harassment excursion (a single healthy attached
    // plate sent out solo, independent of the full generate/detach/return cycle). See
    // updateBeltbreakerCore.
    float harassTimer = 0;

    // Plate-only fields (isBeltbreakerPlate = true).
    bool isBeltbreakerPlate = false;
    int32_t plateOwnerId = -1;
    int32_t plateSlotIndex = 0;
    bool plateAttached = true;
    bool plateReturning = false;
    // >0 while on a timed excursion (whim harassment or PlateHurl) - forces plateReturning once
    // it runs out, regardless of whether the core's shield is even up yet. Left at 0 for a plate
    // detached via the normal full shield-generation cycle, which instead waits for the core's
    // own beltbreakerReturnTriggered.
    float plateExcursionTimer = 0;

    // The Wreckworm (Rustbloom signature boss, waves 30/40/50): a head plus its own chain of
    // segments - each segment is a REAL Boss entry (isWreckwormSegment), same ownership pattern
    // as the Beltbreaker's plates (see plateOwnerId above), just chain-follow instead of orbit.
    // Head-only fields:
    bool isWreckwormHead = false;
    int32_t segmentCount = 0;
    // Grows a notch after every Molt (see triggerWreckwormMolt in update.cpp) - the chain reads
    // as more dangerous once shortened, not just visually different.
    float wreckwormSpeedMult = 1.0F;
    // Health fractions (descending) still left to trigger a Molt - popped off as the head's
    // health crosses each one. Empty once every scheduled Molt for this fight has fired.
    std::vector<float> moltThresholds;
    // CoilClamp-only: the ring's single gap, chosen fresh each cast (see startBossAttack) so the
    // escape route isn't always in the same place.
    float coilClampGapDeg = 0;

    // Segment-only fields (isWreckwormSegment = true). Position is driven entirely by
    // updateWreckwormChain (snake-follow) or, during CoilClamp, by the head's own attack code -
    // a segment never runs its own movement/attack state machine (see the early returns in
    // updateBossMovement/updateBoss).
    bool isWreckwormSegment = false;
    int32_t segmentOwnerId = -1;
    int32_t segmentIndex = 0;
    // Armor segments are dull, tanky, low-priority filler; infected segments are the real
    // (softer, brighter) target. See wreckwormArmorDamageMult in damageBoss.
    bool isArmorSegment = false;

    // Elemental debuffs (mirrors Enemy's fields) - bosses used to be entirely immune to Static /
    // Freeze / Confuse / Burn from the player's elemental weapons/pickups. See
    // applyElementDebuff/applyActiveElementalDebuffs overloads and damageBoss in update.cpp.
    bool debuffStatic = false;
    bool debuffFreeze = false;
    bool debuffConfuse = false;
    float burnDps = 0;
    float burnDamageAccum = 0;
};

struct BossType
{
    std::string_view name;
    Color color;
    float healthMult;
    float sizeMult;
    BossShape shape;
};

constexpr std::array<BossType, 20> bossTypes{
    BossType{.name = "Warbringer",
             .color = Palette::BossIdle,
             .healthMult = 1.0F,
             .sizeMult = 1.0F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "Ashen Sentinel",
             .color = Palette::StructDark,
             .healthMult = 1.1F,
             .sizeMult = 1.05F,
             .shape = BossShape::HexPlated},
    BossType{.name = "Void Herald",
             .color = Palette::Void,
             .healthMult = 0.9F,
             .sizeMult = 0.95F,
             .shape = BossShape::Crystal},
    BossType{.name = "Crimson Warden",
             .color = Palette::Accent,
             .healthMult = 1.05F,
             .sizeMult = 1.0F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "Dim Reaver",
             .color = Palette::AccentDim,
             .healthMult = 1.15F,
             .sizeMult = 1.1F,
             .shape = BossShape::Crystal},
    BossType{.name = "Static Wraith",
             .color = Palette::Haze,
             .healthMult = 0.85F,
             .sizeMult = 0.9F,
             .shape = BossShape::Crystal},
    BossType{.name = "Barrier Colossus",
             .color = Palette::Shield,
             .healthMult = 1.3F,
             .sizeMult = 1.2F,
             .shape = BossShape::HexPlated},
    BossType{.name = "Ember Titan",
             .color = Palette::Charge,
             .healthMult = 1.1F,
             .sizeMult = 1.05F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "Judgment Spire",
             .color = Palette::Crit,
             .healthMult = 0.95F,
             .sizeMult = 0.95F,
             .shape = BossShape::TwinDome},
    BossType{.name = "Hollow Idol",
             .color = Palette::BossHoming,
             .healthMult = 1.0F,
             .sizeMult = 1.0F,
             .shape = BossShape::Saucer},
    BossType{.name = "Shatter Prime",
             .color = Palette::BossSpread,
             .healthMult = 1.05F,
             .sizeMult = 1.0F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "Beamforge",
             .color = Palette::BossBeam,
             .healthMult = 1.0F,
             .sizeMult = 1.0F,
             .shape = BossShape::HexPlated},
    BossType{.name = "Structural Wyrm",
             .color = Palette::StructMid,
             .healthMult = 1.2F,
             .sizeMult = 1.15F,
             .shape = BossShape::HexPlated},
    BossType{.name = "Lightbound Custodian",
             .color = Palette::StructLight,
             .healthMult = 0.9F,
             .sizeMult = 0.95F,
             .shape = BossShape::TwinDome},
    BossType{.name = "Nullpoint Marauder",
             .color = Palette::Void,
             .healthMult = 1.1F,
             .sizeMult = 1.0F,
             .shape = BossShape::Crystal},
    BossType{.name = "Cinder Duke",
             .color = Palette::Accent,
             .healthMult = 0.95F,
             .sizeMult = 1.0F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "Rimehollow",
             .color = Palette::Shield,
             .healthMult = 1.0F,
             .sizeMult = 1.05F,
             .shape = BossShape::Crystal},
    BossType{.name = "Fracture Queen",
             .color = Palette::AccentDim,
             .healthMult = 1.2F,
             .sizeMult = 1.1F,
             .shape = BossShape::TwinDome},
    BossType{.name = "Stormcaller",
             .color = Palette::BossSpread,
             .healthMult = 0.9F,
             .sizeMult = 0.9F,
             .shape = BossShape::SpikedRing},
    BossType{.name = "The Last Signal",
             .color = Palette::Crit,
             .healthMult = 1.35F,
             .sizeMult = 1.25F,
             .shape = BossShape::TwinDome},
};

// Every field is still value-initialized via aggregate init at every construction site; the
// missing-member-init check below is a false positive triggered only by huntingNewTarget's default.
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
class BossProjectile
{
  public:
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool homing;
    bool active;
    bool fromPlayer;

    int damage;
    int health;

    // Evolved (Seeker Swarm) player homing missiles only: true while flying straight because no
    // untargeted enemy existed at spawn time, false once it has a locked target (or for any
    // non-evolved / boss-owned projectile, which never sets this).
    bool huntingNewTarget = false;

    // M15 Ricochet, player homing missiles only: bounces remaining before the missile deactivates
    // on its next enemy hit instead of retargeting. Always 0 for boss-owned projectiles.
    int32_t ricochetRemaining = 0;
};

class BossDeathShockwave
{
  public:
    float timer;
    Vector2 position;
    bool hit;
};
