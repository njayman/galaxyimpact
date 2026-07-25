#pragma once

#include "raylib.h"

namespace playerConstants
{
constexpr int maxShieldStack = 3;
}

class Player
{
  public:
    Vector2 position;
    float radius;
    Color color;
    float speed;
    int health;
    int maxHealth;
    bool shieldActive;
    float shieldTimer;
    float shieldCooldownTimer;
    float immunityTimer;
    float blackHoleCoreTimer;
    float slowTimer;
    int charges;
    float chargeRegenTimer;
    bool dashing;
    float dashTimer;
    Vector2 dashVelocity;
    int shieldStacks;
    bool halfLifeOrb;
    float nerve;
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
    bool evolved; // base mines only home on enemies and need near-contact to
                  // trigger; evolved mines also seek asteroids and trigger
                  // at their full blast radius
};
