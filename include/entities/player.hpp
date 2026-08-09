#pragma once

#include "entities/item.hpp"
#include "raylib.h"
#include <array>
#include <cstdint>

class Player
{
  public:
    Vector2 position;
    // This frame's actual world-space displacement (post-dash/black-hole-pull/etc) - not an
    // input direction, a measured delta. Lets effects spawned off the player (e.g. flamethrower
    // particles) inherit the player's own motion so they don't visually lag/shorten while moving
    // in the same direction they're fired.
    Vector2 velocity;
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
    float confusedTimer;
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

    float dashTrailTimer;
    bool secondWindReady;

    float overdriveTimer;
};

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

class FollowerDrone
{
  public:
    Vector2 position;
    float orbitAngle;
    float attackTimer;
};

class LaserDrone
{
  public:
    Vector2 position;
    float orbitAngle;
    float attackTimer;
    float beamFlashTimer;
    Vector2 beamTarget;
};

class Turret
{
  public:
    Vector2 position;
    float life;
    float fireTimer;
};

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

class OrbitBladeProjectile
{
  public:
    Vector2 position;
    Vector2 velocity;
    float radius;
    int damage;
    bool active;
};

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
