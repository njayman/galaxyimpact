"""
Build presets: a fixed loadout (weapons + skills + ship) plus a "power-online
wave" that models when the build is considered complete. Levels between now
and that wave are assumed linear; every level gained past a slot being maxed
becomes a postCapDamageLevels stack, which is the crux of the whole
rebalance question (that stack is uncapped in the live game).
"""

from dataclasses import dataclass, field
from typing import List, Tuple

import constants as C


@dataclass
class WeaponSlot:
    name: str
    evolved: bool = False


@dataclass
class BuildPreset:
    name: str
    ship: str
    weapons: List[WeaponSlot]
    skills: dict           # skill name -> target level
    power_online_wave: int  # wave by which every weapon/skill above is at max level
    ricochet_level: int = 0
    postcap_levels_per_wave: float = 0.3  # levels/wave gained after power_online_wave


PRESETS = [
    BuildPreset(
        name="photon_cannon_rush",
        ship="Ranger",
        weapons=[WeaponSlot("Forward", evolved=True), WeaponSlot("Homing"),
                 WeaponSlot("Shock"), WeaponSlot("Mine")],
        skills={"WarheadTuning": 5, "Overclock": 5},
        power_online_wave=30,
    ),
    BuildPreset(
        name="balanced_six_slot",
        ship="Ranger",
        weapons=[WeaponSlot("Forward"), WeaponSlot("Orbit"), WeaponSlot("Homing"),
                 WeaponSlot("Mine"), WeaponSlot("Beam"), WeaponSlot("Shock")],
        skills={"WarheadTuning": 5, "Overclock": 5},
        power_online_wave=50,
    ),
    BuildPreset(
        name="all_evolved_six_slot",
        ship="Ranger",
        weapons=[WeaponSlot("Forward", True), WeaponSlot("Orbit", True), WeaponSlot("Homing", True),
                 WeaponSlot("Mine", True), WeaponSlot("Beam", True), WeaponSlot("Shock", True)],
        skills={"WarheadTuning": 5, "Overclock": 5},
        power_online_wave=70,
    ),
    BuildPreset(
        name="m15_amplifier_heavy",
        ship="Interceptor",
        weapons=[WeaponSlot("Beam", True), WeaponSlot("FollowerDrone"), WeaponSlot("LaserDrone"),
                 WeaponSlot("Railgun"), WeaponSlot("ChainLightning"), WeaponSlot("TurretDeploy")],
        skills={"WarheadTuning": 5, "Overclock": 5},
        power_online_wave=70,
    ),
    BuildPreset(
        name="flak_flame_ricochet",
        ship="Bastion",
        weapons=[WeaponSlot("Orbit", True), WeaponSlot("FlakCannon"), WeaponSlot("Flamethrower"),
                 WeaponSlot("Ricochet")],
        skills={"WarheadTuning": 5, "Overclock": 5},
        power_online_wave=40,
        ricochet_level=3,
    ),
]


def weapons_by_name(names: List[str]) -> List[BuildPreset]:
    return [p for p in PRESETS if p.name in names] or PRESETS
