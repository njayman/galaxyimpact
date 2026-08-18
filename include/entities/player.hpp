#pragma once

#include "entities/item.hpp"
#include "raylib.h"
#include <array>
#include <cstdint>

class Player
{
  public:
    Vector2 position;

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

    float burnDps = 0;
    float burnDamageAccum = 0;
    float burnRefreshTimer = 0;

    bool grabbed = false;
    int32_t grabbingTentacle = -1;

    bool launched = false;
    Vector2 launchVelocity{};
    int32_t launchRound = 0;
    int32_t launchTargetTentacle = -1;

    bool comboActive = false;
    bool comboRefundEligible = false;
    bool invulnerable = false;
    float lastDashClickTime = -100.0F;
    float lastShieldClickTime = -100.0F;

    bool shieldDashing = false;
    bool shieldDashKill = false;
    float shieldDashGraceTimer = 0;
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
    bool ricochetThenPhase = false;
    bool phasing = false;
};

class FollowerDrone
{
  public:
    Vector2 position;
    float attackTimer;
    float shotTimer;
};

class LaserDrone
{
  public:
    Vector2 position;
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
