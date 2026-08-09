"""
Balance constants mirrored from the C++ source. Every value below cites the
file:line it was extracted from (as of commit 03d5cf0) so this can be kept in
sync by hand when gameplay code changes. Difficulty modes are collapsed to
the "Hard" tier throughout, matching the game's move to a single fixed
difficulty (no more Easy/Normal/Hard branching).
"""

from dataclasses import dataclass, field

# ---------------------------------------------------------------------------
# Weapon base cooldowns / max levels (src/update.cpp weapon-fire switch, and
# per-weapon update functions: updateOrbitBladeContact, updateMines,
# updateBeamContact, aoePulse, fireChainLightning, updateFollowerDrones,
# updateLaserDrones, updateTurrets, updateFlamethrower)
# ---------------------------------------------------------------------------

WEAPON_BASE_COOLDOWN = {
    "Forward": 0.38,
    "Orbit": None,  # continuous contact damage, not cooldown-gated
    "Homing": 1.0,
    "Mine": 0.7,
    "Beam": None,  # continuous contact damage
    "Shock": 1.5,
    "Ricochet": None,  # amplifier, not its own weapon
    "FollowerDrone": 0.6,
    "LaserDrone": 0.6,
    "FlakCannon": 0.9,
    "Railgun": 1.8,
    "ChainLightning": 2.2,
    "TurretDeploy": 6.0,
    "Flamethrower": None,  # continuous, duty-cycle gated at level 3
}

WEAPON_MAX_LEVEL = {
    "Forward": 8, "Orbit": 8, "Homing": 8, "Mine": 8, "Beam": 8, "Shock": 8,
    "Ricochet": 3, "FollowerDrone": 4, "LaserDrone": 4, "FlakCannon": 3,
    "Railgun": 3, "ChainLightning": 3, "TurretDeploy": 3, "Flamethrower": 3,
}

EVOLVABLE_WEAPONS = {"Forward", "Orbit", "Homing", "Mine", "Beam", "Shock"}
EVOLVED_NAME = {
    "Forward": "Photon Cannon", "Orbit": "Aegis Ring", "Homing": "Seeker Swarm",
    "Mine": "Cluster Charges", "Beam": "Lance Sweep", "Shock": "Bulwark Pulse",
}

# ---------------------------------------------------------------------------
# Skills (include/entities/item.hpp Skills array; application sites in
# src/update.cpp as cited)
# ---------------------------------------------------------------------------

SKILL_MAX_LEVEL = {
    "WarheadTuning": 5,   # dmg x(1+0.15*lvl)              update.cpp:1789-1790
    "Overclock": 5,       # cooldown x(1-0.10*lvl)          update.cpp:1763-1764
    "BarrierMastery": 3,  # charge regen -0.3s/lvl (floor 1s); shield +0.4s/lvl
    "TractorBeam": 5,     # pickup magnet radius (not combat-relevant)
    "Thrusters": 5,       # move speed (survivability/kiting, not DPS)
    "HullPlating": 5,     # +1 max HP/level, full heal on grant
}

WARHEAD_DMG_PER_LEVEL = 0.15
OVERCLOCK_CD_REDUCTION_PER_LEVEL = 0.10
POSTCAP_DMG_PER_LEVEL = 0.05  # uncapped in the live game today — the core bug
POSTCAP_LEVELS_CAP = 20       # REBALANCE: hard ceiling, was uncapped. Caps this source at +100% dmg.

# ---------------------------------------------------------------------------
# REBALANCE PASS (2026-08-01): per-weapon shape multipliers, tuned via
# tools/balance_sim so all 14 weapons land in a healthy DPS band (~77-122 at
# Warhead 5 + Overclock 5, no evolution) instead of the previous 16-1520
# range (Photon Cannon was 94x the weakest weapon, Beam/ChainLightning were
# the floor). Evolutions are normalized to a uniform x1.5 damage budget plus
# ONE unique non-damage-equivalent mechanic each (pierce / projectile-destroy
# / dual-target / chain-detonate / oscillating-sweep / heal+knockback) —
# Photon Cannon previously stacked FOUR simultaneous bonuses (extra shot,
# x1.5 dmg, pierce, x0.8 cooldown) where every other evolution only got one.
# ---------------------------------------------------------------------------

FORWARD_MAX_SHOTS = 2          # was min(3, 1+level//3) — removed the 3rd shot tier
FORWARD_DAMAGE_MULT = 0.20     # was implicit 1.0 (full weaponDamage per shot)
PHOTON_CANNON_EXTRA_SHOTS = 0  # was +1 — removed, matches other evolutions' single-bonus pattern
PHOTON_CANNON_COOLDOWN_MULT = 1.0  # was 0.8 — removed, ditto

CONTINUOUS_DPS_MULTIPLIER = 2.1  # shared by Orbit/Beam, was Orbit-only 1.2
BEAM_DAMAGE_FRACTION = 1.0       # was 0.35 — Beam's arbitrary discount removed

MINE_DAMAGE_MULT = 0.73        # was implicit 1.0
SHOCK_DAMAGE_MULT = 1.56       # was implicit 1.0

FOLLOWER_DRONE_FRAC = {1: 0.4, 2: 0.6, 3: 0.6, 4: 0.75}   # was {0.5,0.7,0.7,0.9} shared w/ LaserDrone
LASER_DRONE_FRAC = {1: 0.55, 2: 0.76, 3: 0.76, 4: 0.98}   # now its own table, slight buff

FLAKCANNON_DAMAGE_MULT = 0.34  # was implicit 1.0

RAILGUN_BASE_MULT = 2.64       # was 2.0 (still x1.3 more at level 3)

CHAINLIGHTNING_DAMAGE_MULT = 1.9  # was implicit 1.0
CHAINLIGHTNING_COOLDOWN = 1.0      # was 2.2

TURRET_FIRE_INTERVAL = 0.7     # was 0.8

FLAMETHROWER_DAMAGE_MULT = 2.54  # was 2.0 (still x2 cones at lvl2+, x0.75 uptime at lvl3)

EVOLUTION_DAMAGE_MULT = 1.5    # uniform across all 6 evolutions (already true for 5 of them)

# ---------------------------------------------------------------------------
# Ships (include/entities/ship.hpp:47-97)
# ---------------------------------------------------------------------------

@dataclass
class ShipStats:
    hp: float
    armor: float
    speed: float
    dmg_mult: float
    shield_stacks: int
    dash_dist_mult: float
    orbit_spin_mult: float
    bullet_speed_mult: float
    beam_length_mult: float
    default_weapon: str


SHIPS = {
    "Bastion": ShipStats(8, 2, 4.0, 0.8, 4, 0.75, 2.2, 1.0, 1.0, "Orbit"),
    "Ranger": ShipStats(5, 1, 5.0, 1.0, 3, 1.0, 1.0, 1.4, 1.0, "Forward"),
    "Interceptor": ShipStats(4, 0, 6.5, 1.3, 2, 1.3, 1.0, 1.0, 1.5, "Beam"),
}

# ---------------------------------------------------------------------------
# Enemies (include/entities/enemy.hpp:121-310)
# ---------------------------------------------------------------------------

@dataclass
class EnemyKind:
    hp: float
    dmg: float
    speed: float
    pattern: str
    min_wave: int


ENEMY_KINDS = {
    "Drifter": EnemyKind(10, 1, 1.0, "Chase", 1),
    "Swarmling": EnemyKind(4, 1, 2.2, "Chase", 1),
    "Brute": EnemyKind(40, 3, 0.6, "Chase", 2),
    "Zigzagger": EnemyKind(12, 1, 1.4, "Zigzag", 2),
    "Charger": EnemyKind(14, 2, 0.8, "Charge", 3),
    "Orbiter": EnemyKind(10, 1, 1.2, "Orbit", 3),
    "Splitter": EnemyKind(16, 1, 1.0, "Chase", 2),
    "Turret": EnemyKind(20, 1, 0.0, "Turret", 3),
    "Sniper": EnemyKind(10, 1, 0.0, "Turret", 5),
    "ShieldedDrone": EnemyKind(35, 2, 0.9, "Chase", 4),
    "Bomber": EnemyKind(8, 0, 1.1, "Chase", 4),
    "Leech": EnemyKind(10, 0, 1.3, "Chase", 3),
    "SwarmMother": EnemyKind(30, 1, 0.4, "Spawner", 5),
    "PhaseWraith": EnemyKind(18, 2, 1.3, "Chase", 6),
    "Mine": EnemyKind(5, 4, 0.0, "Stationary", 3),
    "LaserFence": EnemyKind(30, 3, 0.0, "Stationary", 6),
    "VoidRift": EnemyKind(25, 1, 0.0, "Spawner", 7),
}

WAVE_ENEMY_SCALE_PER_WAVE = 0.035  # waveEnemyScale = 1 + (wave-1)*this, update.cpp:3166-3168
MAX_CONCURRENT_ENEMIES = 200
ELITE_CHANCE = 0.06
ELITE_HAZARD_CONTACT_DMG = 2
ELITE_HAZARD_CAP = 3  # collapsed to Hard's value

# ---------------------------------------------------------------------------
# Bosses (src/update.cpp ~3243-3307, include/entities/boss.hpp)
# ---------------------------------------------------------------------------

BOSS_BASE_HP = 500
BOSS_HP_PER_TIER = 500       # REBALANCE: was 250. Doubled to match capped-but-real player DPS
                             # growth — keeps wave-100 TTK healthy (~7-8s for an on-track build)
                             # while still creating a genuine, escalating wall deep into infinite
                             # mode once postCapDamageLevels caps out (see POSTCAP_LEVELS_CAP).
BOSS_MEGA_HP_MULT = 1.5
BOSS_MINI_HP_MULT = 0.5
BOSS_FINAL_HP_MULT = 3.0
BOSS_TYPE_HEALTH_MULT_RANGE = (0.85, 1.35)  # 20 BossType entries
BOSS_TYPE_SIZE_MULT_RANGE = (0.9, 1.25)
BOSS_ENRAGE_HEALTH_FRAC = 0.25
BOSS_ENRAGE_SPEED_MULT = 1.6
BOSS_MOVE_COUNT = 4          # bossMoveCountForDifficulty, collapsed to Hard's value
MINI_BOSS_WAVE_INTERVAL = 5
MEGA_BOSS_WAVE_INTERVAL = 10
BOSS_PROJECTILE_HP = 3

# 11 BossAttack values. movement_paused = boss holds position during SHOOTING
# (the other 7 keep full strafe/chase movement even while attacking — this is
# the M12 "boss dodge is out of control" root cause).
BOSS_ATTACKS = {
    "Beam":            {"windup": 1.0, "active": 4.0, "movement_paused": True},
    "Spread":          {"windup": 1.0, "active": 1.0, "movement_paused": False},
    "Slam":            {"windup": 1.3, "active": 1.6, "movement_paused": True},
    "WormholeBeam":    {"windup": 1.4, "active": 2.5, "movement_paused": True},
    "MineDrop":        {"windup": 1.0, "active": 0.5, "movement_paused": False},
    "ChargeDash":      {"windup": 1.2, "active": 0.5, "movement_paused": True},
    "SummonAdds":      {"windup": 1.0, "active": 0.6, "movement_paused": False},
    "ShockwaveStomp":  {"windup": 1.0, "active": 0.5, "movement_paused": False},
    "Barrage":         {"windup": 1.0, "active": 2.5, "movement_paused": False},
    "GravityWell":     {"windup": 1.0, "active": 2.0, "movement_paused": False},
    "HomingBarrage":   {"windup": 1.0, "active": 2.5, "movement_paused": False},
}

# ---------------------------------------------------------------------------
# Elemental debuffs (src/update.cpp, currentBurnDps() etc.)
# ---------------------------------------------------------------------------

FREEZE_SLOW_MULT = 0.5
BASE_BURN_DPS = 3.0
PICKUP_BUFF_DURATION = 60.0  # difficultyPickupDuration, collapsed to Hard's 60s tier

# ---------------------------------------------------------------------------
# Achievement-gated economy (src/achievements.cpp)
# ---------------------------------------------------------------------------

# REBALANCE (2026-08-01): was {3:0, 4:15, 5:50, 6:70} — a fresh save had only ONE weapon (ship
# default) and 3 slots through the wave-5 miniboss and wave-10 megaboss. Pulled forward so a fresh
# run has real weapon variety before its first boss fights (see also HOMING/MINE/SHOCK unlock
# waves below, achievements.cpp recordWaveReached).
SLOT_CAP_WAVE_GATES = {3: 0, 4: 8, 5: 35, 6: 55}  # slot count -> wave unlocked
HOMING_UNLOCK_WAVE = 3   # was 10
MINE_UNLOCK_WAVE = 7     # was 20
SHOCK_UNLOCK_WAVE = 12   # was 25
EVOLUTION_KILL_THRESHOLD = 500  # Hard tier (Easy 1000, Normal 700 — collapsed to Hard)
FLAK_CANNON_UNLOCK_KILL_COUNT = 200  # cumulative dash-or-nerve kills

# ---------------------------------------------------------------------------
# Nerve / dash / shield (src/update.cpp)
#
# IMPORTANT: nerveFrac() is computed but never read anywhere else in the
# codebase — the documented "+50% damage / +20% speed at full Nerve" effect
# is dead code, not a live mechanic. Nerve today only feeds ability-charge
# regen and the 100-meter burst below. Modeled here as an optional, separate
# bonus-damage term (off by default) rather than a damage/speed multiplier.
# ---------------------------------------------------------------------------

NERVE_MAX = 100
NERVE_KILL_GAIN = 6
MAX_CHARGES = 2
CHARGE_REGEN_TIME = 3.0       # -0.3s per Barrier Mastery level, floor 1s
SHIELD_BASE_DURATION = 1.2    # +0.4s per Barrier Mastery level
DASH_KILL_CHARGE_REFUND = 0.6
NERVE_BURST_DAMAGE = 260
NERVE_BURST_RANGE = 900
