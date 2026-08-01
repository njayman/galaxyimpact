#pragma once

#include "entities/item.hpp"
#include "raylib.h"
#include <array>
#include <cstdint>

class Player
{
  public:
    Vector2 position;
    float radius;
    Color color;
    float speed;
    float health;
    float maxHealth;
    bool shieldActive;
    float shieldTimer;
    float shieldCooldownTimer;
    float immunityTimer;
    float blackHoleCoreTimer;
    float bossBodyTimer;
    float slowTimer;
    int charges;
    float chargeRegenTimer;
    bool dashing;
    float dashTimer;
    Vector2 dashVelocity;
    int shieldStacks;
    float nerve;
    bool nerveCharging;
    float nerveChargeTimer;
    float nerveChargeFeedTimer;
    std::array<float, static_cast<size_t>(ElementType::Count)> elementalBuffTimer;
    float regenTimer;
    float regenRate;
    float overchargeTimer;
    bool dashTrailUnlocked;
    bool secondWindReady;
};

// Every field is still value-initialized via aggregate init at every construction site; the
// missing-member-init check below is a false positive triggered only by the defaulted fields.
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
class Bullet
{
  public:
    Vector2 position;
    Vector2 velocity;
    float radius;
    Color color;
    bool active;
    int damage;
    int pierceRemaining = 0;
    int ricochetRemaining = 0;
    float splashRadius = 0;
    DamageSource source = DamageSource::Forward;
};

// M15 amplifier: orbits/follows the player, melees whichever active enemy is nearest within
// meleeRange every attackInterval seconds. See updateFollowerDrones (src/update.cpp).
class FollowerDrone
{
  public:
    Vector2 position;
    float orbitAngle;
    float attackTimer;
};

// M15 amplifier: same cadence as FollowerDrone but attacks at range instead of melee. See
// updateLaserDrones (src/update.cpp).
class LaserDrone
{
  public:
    Vector2 position;
    float orbitAngle;
    float attackTimer;
    float beamFlashTimer;
    Vector2 beamTarget;
};

// M15 Turret Deploy: a stationary auto-firing emplacement that despawns after `life` seconds.
class Turret
{
  public:
    Vector2 position;
    float life;
    float fireTimer;
};

// Pure VFX for Chain Lightning's arc — no gameplay effect, just fades out over `timer`.
class ChainLightningBolt
{
  public:
    Vector2 from;
    Vector2 to;
    float timer;
};

class Mine
{
  public:
    Vector2 position;
    Vector2 velocity;
    float fuse;
    float radius;
    int damage;
    bool active;
    bool evolved;
};

// A blade launched off the orbit ring toward the mouse cursor; pierces enemies (no
// deactivate-on-hit) until it despawns out of range. Also reused for the Ranger nerve burst's
// thrown ball, which behaves identically (travels, pierces, despawns out of range).
class OrbitBladeProjectile
{
  public:
    Vector2 position;
    Vector2 velocity;
    float radius;
    int damage;
    bool active;
};

// The Bastion nerve burst: a ring of blades spinning around a moving center, travelling in the
// aim direction like a thrown beyblade.
class NerveSpiralProjectile
{
  public:
    Vector2 origin;
    Vector2 direction;
    float age;
    float speed;
    float life;
    float spinRadius;
    int32_t bladeCount;
    int32_t damagePerBlade;
    bool active;
};
