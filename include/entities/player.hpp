#pragma once

#include "entities/item.hpp"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
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
    float boundaryWallTimer = 0;
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
    uint32_t ricochetLastHit = 0; // Enemy::id bounced off last; never re-hit in a row
    float splashRadius = 0;
    DamageSource source = DamageSource::Forward;
    bool ricochetThenPhase = false;
    bool phasing = false;
    // > 0: a laser bolt. Straight: a comet streak this long trailing behind position. Arc
    // (evolved Laser Shots): a U-shaped sweep of this radius, its front at position.
    float streak = 0;
    float halfWidth = 0;
    bool arc = false;
    float arcSkew = 0; // radians the arc is rotated off-center, so each one leans left or right
    // Distance flown since leaving the nose (accumulated, so a wormhole jump doesn't count);
    // the bolt grows out of the nose by this much instead of appearing through the hull.
    float traveled = 0;
    // Laser bolts phase through enemies, hitting each once. ponytail: fixed ring of 64 covers an
    // ever-widening arc sweeping a crowd (a bolt overlaps an enemy for a few frames).
    std::array<uint32_t, 64> phasedThrough{};
    uint8_t phasedCount = 0;
};

// A laser bolt as a polyline, shared by hit-testing and drawing so they can't drift.
struct LaserShape
{
    static constexpr int arcPoints = 21;
    std::array<Vector2, arcPoints> points{};
    int count = 0;
};

inline auto laserShape(const Bullet& b) -> LaserShape
{
    const Vector2 fwd = Vector2Normalize(b.velocity);
    const float traveled = b.traveled;
    LaserShape shape;
    if (!b.arc)
    {
        // Grows out of the nose instead of appearing full-length through the hull.
        shape.points.at(0) =
            Vector2Subtract(b.position, Vector2Scale(fwd, std::min(b.streak, traveled)));
        shape.points.at(1) = b.position;
        shape.count = 2;
        return shape;
    }
    constexpr float growDistance = 480.0F; // the arc gains its launch size again every this far
    constexpr float sweep = 105.0F * DEG2RAD; // each side; > 90 makes it a U, arms trailing back
    constexpr float pointiness = 0.35F;       // pulls the front forward into a pointed arch
    // No cap: it keeps widening until it despawns.
    const float growth = 1.0F + traveled / growDistance;
    const float radius = std::min(b.streak * growth, traveled * 0.5F);
    const Vector2 center = Vector2Subtract(b.position, Vector2Scale(fwd, radius));
    const Vector2 axis = Vector2Rotate(fwd, b.arcSkew); // the arch's own (leaning) center line
    shape.count = LaserShape::arcPoints;
    for (int i = 0; i < shape.count; i++)
    {
        // rel runs -1..1 across the arch; (1 - |rel|)^2 has a kink at 0, so two curved sides meet
        // at a slight angle at the front instead of a round U.
        const float rel = -1.0F + 2.0F * static_cast<float>(i) / static_cast<float>(shape.count - 1);
        const float pull = pointiness * radius * (1.0F - std::abs(rel)) * (1.0F - std::abs(rel));
        shape.points.at(static_cast<size_t>(i)) = Vector2Add(
            Vector2Add(center, Vector2Scale(Vector2Rotate(axis, rel * sweep), radius)),
            Vector2Scale(axis, pull));
    }
    return shape;
}

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
