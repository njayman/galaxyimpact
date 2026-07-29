#pragma once

#include "raylib.h"
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
