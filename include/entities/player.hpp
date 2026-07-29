#pragma once

#include "raylib.h"

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
