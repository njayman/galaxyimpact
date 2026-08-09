"""
Pure math model of combat/economy formulas. No graphics, no game loop, no
raylib — this is what turns a "build" (equipped weapons + skills + ship +
wave number) into DPS/HP/survivability numbers that can be swept over many
permutations.

Approximations are flagged with a comment where the source data was
incomplete or the mechanic (drones, turret uptime, flamethrower duty cycle)
doesn't reduce to a clean formula. Everything else is a direct translation
of the constants in constants.py.
"""

from dataclasses import dataclass, field
from typing import Optional

import constants as C


@dataclass
class Context:
    """Everything needed to evaluate a weapon's output at a point in time."""
    warhead_lvl: int = 0
    overclock_lvl: int = 0
    postcap_lvl: int = 0
    ship_dmg_mult: float = 1.0
    overcharge: bool = False
    min_cooldown: float = 0.05
    # Crowd assumption: how many enemies are close enough to be hit by
    # pierce/ricochet/AoE at once. This is the single biggest "it depends on
    # play" unknown — expose it as a CLI knob rather than guessing.
    enemies_in_range: int = 3


def weapon_damage(level: int, ctx: Context) -> float:
    dmg = (6 + level * 2) * (1 + C.WARHEAD_DMG_PER_LEVEL * ctx.warhead_lvl)
    dmg *= 1 + C.POSTCAP_DMG_PER_LEVEL * ctx.postcap_lvl
    dmg *= ctx.ship_dmg_mult
    if ctx.overcharge:
        dmg *= 1.5
    return dmg


def cooldown_after_skills(base_cd: float, ctx: Context, evolution_cd_mult: float = 1.0) -> float:
    cd = base_cd * (1 - C.OVERCLOCK_CD_REDUCTION_PER_LEVEL * ctx.overclock_lvl) * evolution_cd_mult
    return max(cd, ctx.min_cooldown)


@dataclass
class WeaponOutput:
    single_target_dps: float
    crowd_dps: float          # accounts for pierce/ricochet/AoE at ctx.enemies_in_range
    is_continuous: bool = False


def weapon_output(name: str, level: int, evolved: bool, ctx: Context,
                   ricochet_level: int = 0) -> WeaponOutput:
    """Dispatch to the per-weapon formula. `ricochet_level` (0-3) is the
    Ricochet amplifier's bounce count, applied only to the weapons it can
    attach to (base Forward, Homing, FlakCannon L1-2, Turret)."""
    level = max(1, level)
    evo = C.EVOLUTION_DAMAGE_MULT if evolved else 1.0

    if name == "Forward":
        shots = min(C.FORWARD_MAX_SHOTS, 1 + level // 4) + C.PHOTON_CANNON_EXTRA_SHOTS if evolved \
            else min(C.FORWARD_MAX_SHOTS, 1 + level // 4)
        dmg = weapon_damage(level, ctx) * C.FORWARD_DAMAGE_MULT * evo
        cd_mult = C.PHOTON_CANNON_COOLDOWN_MULT if evolved else 1.0
        cd = cooldown_after_skills(C.WEAPON_BASE_COOLDOWN["Forward"], ctx, cd_mult)
        pierce = 2 if evolved else ricochet_level
        single = shots * dmg / cd
        crowd = single * (1 + min(pierce, ctx.enemies_in_range - 1))
        return WeaponOutput(single, crowd)

    if name == "Orbit":
        dmg = weapon_damage(level, ctx) * C.CONTINUOUS_DPS_MULTIPLIER * evo
        # continuous contact ring — every enemy touching it takes the tick
        return WeaponOutput(dmg, dmg * ctx.enemies_in_range, is_continuous=True)

    if name == "Homing":
        missiles = 2 if evolved else 1
        dmg = weapon_damage(level, ctx) * evo
        cd = cooldown_after_skills(C.WEAPON_BASE_COOLDOWN["Homing"], ctx)
        single = dmg / cd  # missiles split across distinct targets when evolved, not stacked on one
        crowd = missiles * dmg / cd
        return WeaponOutput(single, crowd)

    if name == "Mine":
        count = 2 + level // 3 + (1 if evolved else 0)
        dmg = weapon_damage(level, ctx) * C.MINE_DAMAGE_MULT * evo
        cd = cooldown_after_skills(C.WEAPON_BASE_COOLDOWN["Mine"], ctx)
        single = dmg / cd  # only one mine likely to be under the boss at a time
        crowd = count * dmg / cd
        return WeaponOutput(single, crowd)

    if name == "Beam":
        dmg = weapon_damage(level, ctx) * C.BEAM_DAMAGE_FRACTION * C.CONTINUOUS_DPS_MULTIPLIER * evo
        return WeaponOutput(dmg, dmg * ctx.enemies_in_range, is_continuous=True)

    if name == "Shock":
        dmg = weapon_damage(level, ctx) * C.SHOCK_DAMAGE_MULT * evo
        cd = cooldown_after_skills(C.WEAPON_BASE_COOLDOWN["Shock"], ctx)
        single = dmg / cd
        crowd = dmg * ctx.enemies_in_range / cd
        return WeaponOutput(single, crowd)

    if name == "FollowerDrone":
        # APPROXIMATION: shipCurrentDamage() formula wasn't pinned down exactly;
        # modeled here as weapon_damage() at the given level, same as other weapons.
        count = 2 if level >= 3 else 1
        interval = 0.6 / 1.3 if level >= 2 else 0.6
        dmg = weapon_damage(level, ctx) * C.FOLLOWER_DRONE_FRAC[level]
        single = count * dmg / interval
        return WeaponOutput(single, single)

    if name == "LaserDrone":
        count = 2 if level >= 3 else 1
        interval = 0.6
        dmg = weapon_damage(level, ctx) * C.LASER_DRONE_FRAC[level]
        single = count * dmg / interval
        return WeaponOutput(single, single)

    if name == "FlakCannon":
        pellets = 3 + (2 if level >= 2 else 0)
        dmg = weapon_damage(level, ctx) * C.FLAKCANNON_DAMAGE_MULT
        cd = cooldown_after_skills(C.WEAPON_BASE_COOLDOWN["FlakCannon"], ctx)
        pierce = 1 if level >= 3 else ricochet_level
        single = pellets * dmg / cd  # all pellets can converge on one target at close range
        crowd = single * (1 + min(pierce, ctx.enemies_in_range - 1))
        return WeaponOutput(single, crowd)

    if name == "Railgun":
        dmg = weapon_damage(level, ctx) * C.RAILGUN_BASE_MULT * (1.3 if level >= 3 else 1)
        cd = cooldown_after_skills(C.WEAPON_BASE_COOLDOWN["Railgun"], ctx)
        single = dmg / cd
        # pierce=999 but bosses/asteroids stop the bullet on first contact —
        # crowd bonus only applies to regular-enemy clears, not boss TTK.
        crowd = single * min(ctx.enemies_in_range, 6)
        return WeaponOutput(single, crowd)

    if name == "ChainLightning":
        links = level
        dmg = weapon_damage(level, ctx) * C.CHAINLIGHTNING_DAMAGE_MULT
        cd = cooldown_after_skills(C.CHAINLIGHTNING_COOLDOWN, ctx)
        single = dmg / cd  # first link only, vs a single target like a boss
        crowd = links * dmg / cd
        return WeaponOutput(single, crowd)

    if name == "TurretDeploy":
        count = 2 if level >= 3 else 1
        dmg = weapon_damage(level, ctx)
        cd = C.WEAPON_BASE_COOLDOWN["TurretDeploy"]
        turret_life, turret_fire_interval = 8.0, C.TURRET_FIRE_INTERVAL
        # APPROXIMATION: turret uptime averaged over its deploy cooldown cycle.
        single = count * dmg * (turret_life / turret_fire_interval) / cd
        return WeaponOutput(single, single)

    if name == "Flamethrower":
        cones = 2 if level >= 2 else 1
        uptime = 0.75 if level >= 3 else 1.0  # cycles off 1s every 4s at L3
        dmg = weapon_damage(level, ctx) * C.FLAMETHROWER_DAMAGE_MULT * cones * uptime
        return WeaponOutput(dmg, dmg * ctx.enemies_in_range, is_continuous=True)

    if name == "Ricochet":
        return WeaponOutput(0.0, 0.0)  # amplifier only, no direct damage

    raise ValueError(f"unknown weapon {name!r}")


def boss_hp(wave: int, tier_kind: str = "mega", boss_type_health_mult: float = 1.0,
            is_final: bool = False,
            base_hp: Optional[float] = None, hp_per_tier: Optional[float] = None) -> float:
    base_hp = C.BOSS_BASE_HP if base_hp is None else base_hp
    hp_per_tier = C.BOSS_HP_PER_TIER if hp_per_tier is None else hp_per_tier
    tier = wave // 10
    hp = base_hp + tier * hp_per_tier
    hp *= {"mega": C.BOSS_MEGA_HP_MULT, "mini": C.BOSS_MINI_HP_MULT}[tier_kind]
    hp *= boss_type_health_mult
    if is_final:
        hp *= C.BOSS_FINAL_HP_MULT
    return hp


def enemy_hp(base_hp: float, wave: int) -> float:
    return base_hp * (1 + (wave - 1) * C.WAVE_ENEMY_SCALE_PER_WAVE)


def enemy_dmg(base_dmg: float, wave: int) -> float:
    return base_dmg * (1 + (wave - 1) * C.WAVE_ENEMY_SCALE_PER_WAVE)


def spawn_count(wave: int) -> int:
    return 1 + wave // 4


def spawn_interval(wave: int) -> float:
    cycle_index = (wave - 1) % 10
    base = max(1.2 - cycle_index * 0.1, 0.15)
    if cycle_index >= 5:
        base *= 0.6
    return base


def incoming_threat_estimate(wave: int, avg_enemy_hp: float = 15, avg_enemy_dmg: float = 1.5) -> float:
    """Rough proxy for how dangerous a wave is: damage-per-second the player
    would take if surrounded by the wave's typical concurrent enemy count,
    all landing hits at once per second. Not a positional simulation —
    purely a scaling-curve sanity check."""
    concurrent = min(spawn_count(wave) * (1 / max(spawn_interval(wave), 0.01)), C.MAX_CONCURRENT_ENEMIES)
    return concurrent * enemy_dmg(avg_enemy_dmg, wave)
