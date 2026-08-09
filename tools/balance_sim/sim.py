#!/usr/bin/env python3
"""
Galaxy Impact balance simulator — pure-math, headless, re-runnable anytime.

Three report modes:
  --mode parity        Single-weapon DPS at max level, evolved vs not, single-
                        target vs crowd. Answers "which weapons are DPS
                        outliers" in general (the Photon Cannon question).
  --mode curve          For each build preset, DPS / boss-HP / time-to-kill-
                        boss / enemy-HP / incoming-threat across a wave range.
                        Answers "does player power keep pace with scaling".
  --mode progression    For a FRESH save (worst case: no opportunistic dash-
                        kill unlocks), how many real weapon types + slots are
                        available at each wave. Answers "do I have enough
                        options to survive/beat the early bosses" — this was
                        the actual bug (a fresh save had 1 weapon through the
                        wave-5 miniboss and wave-10 megaboss), not a DPS gap.

Every balance-critical constant can be overridden via CLI flags instead of
editing constants.py, so this can be re-run with different hypotheses
without touching code. Examples:

    python3 sim.py --mode parity
    python3 sim.py --mode curve --waves 100 --postcap-cap 40
    python3 sim.py --mode curve --builds balanced_six_slot,photon_cannon_rush --csv out.csv
"""
import argparse
import csv
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import constants as C
import model
from builds import PRESETS, weapons_by_name


def build_ctx(build, wave, args) -> model.Context:
    ship = C.SHIPS[build.ship]
    progressed_wave = min(wave, build.power_online_wave)
    frac = progressed_wave / build.power_online_wave if build.power_online_wave else 1.0
    warhead_lvl = round(build.skills.get("WarheadTuning", 0) * frac)
    overclock_lvl = round(build.skills.get("Overclock", 0) * frac)

    postcap_per_wave = args.levels_per_wave if args.levels_per_wave is not None else build.postcap_levels_per_wave
    postcap_lvl = max(0.0, (wave - build.power_online_wave) * postcap_per_wave)
    if args.postcap_cap is not None:
        postcap_lvl = min(postcap_lvl, args.postcap_cap)

    return model.Context(
        warhead_lvl=warhead_lvl,
        overclock_lvl=overclock_lvl,
        postcap_lvl=postcap_lvl,
        ship_dmg_mult=ship.dmg_mult,
        enemies_in_range=args.enemies_in_range,
    )


def weapon_level_at(build, weapon_name, wave):
    max_lvl = C.WEAPON_MAX_LEVEL[weapon_name]
    frac = min(1.0, wave / build.power_online_wave) if build.power_online_wave else 1.0
    return max(1, round(max_lvl * frac))


def build_dps(build, wave, args):
    ctx = build_ctx(build, wave, args)
    single_total = 0.0
    crowd_total = 0.0
    for slot in build.weapons:
        lvl = weapon_level_at(build, slot.name, wave)
        out = model.weapon_output(slot.name, lvl, slot.evolved, ctx, ricochet_level=build.ricochet_level)
        single_total += out.single_target_dps
        crowd_total += out.crowd_dps
    return single_total, crowd_total


def boss_hp_at(wave, args, tier_kind="mega"):
    return model.boss_hp(
        wave, tier_kind=tier_kind,
        boss_type_health_mult=args.boss_type_health_mult,
        base_hp=args.boss_hp_base, hp_per_tier=args.boss_hp_per_tier,
    )


def run_parity(args):
    print(f"{'Weapon':<16}{'Evolved':<9}{'SingleDPS':>12}{'CrowdDPS':>12}")
    ctx = model.Context(warhead_lvl=5, overclock_lvl=5, postcap_lvl=0, ship_dmg_mult=1.0,
                        enemies_in_range=args.enemies_in_range)
    rows = []
    for name, max_lvl in C.WEAPON_MAX_LEVEL.items():
        if name == "Ricochet":
            continue
        evolvable = name in C.EVOLVABLE_WEAPONS
        for evolved in ([False, True] if evolvable else [False]):
            out = model.weapon_output(name, max_lvl, evolved, ctx)
            label = C.EVOLVED_NAME[name] if evolved else name
            rows.append((label, evolved, out.single_target_dps, out.crowd_dps))

    rows.sort(key=lambda r: -r[2])
    for label, evolved, single, crowd in rows:
        print(f"{label:<16}{'yes' if evolved else 'no':<9}{single:>12.1f}{crowd:>12.1f}")

    if rows:
        top, bottom = rows[0], rows[-1]
        print(f"\nSpread: {top[0]} ({top[2]:.1f} dps) is {top[2]/max(bottom[2],1e-9):.1f}x "
              f"{bottom[0]} ({bottom[2]:.1f} dps) at maxed level, warhead 5 + overclock 5.")


def run_curve(args):
    presets = weapons_by_name(args.builds.split(",")) if args.builds else PRESETS
    waves = range(args.wave_start, args.waves + 1, args.wave_step)

    csv_rows = []
    for build in presets:
        print(f"\n=== {build.name} (ship={build.ship}, online wave {build.power_online_wave}) ===")
        print(f"{'Wave':>5}{'PlayerDPS':>12}{'BossHP':>10}{'TTK_boss':>10}{'EnemyHP':>10}{'TTK_enemy':>10}{'Threat':>10}")
        for wave in waves:
            single_dps, crowd_dps = build_dps(build, wave, args)
            bhp = boss_hp_at(wave, args)
            ttk_boss = bhp / single_dps if single_dps > 0 else float("inf")
            ehp = model.enemy_hp(14, wave)  # ~median base HP across enemy table
            ttk_enemy = ehp / single_dps if single_dps > 0 else float("inf")
            threat = model.incoming_threat_estimate(wave)
            print(f"{wave:>5}{single_dps:>12.1f}{bhp:>10.0f}{ttk_boss:>10.2f}{ehp:>10.1f}{ttk_enemy:>10.3f}{threat:>10.1f}")
            csv_rows.append({
                "build": build.name, "wave": wave, "player_dps": round(single_dps, 2),
                "player_crowd_dps": round(crowd_dps, 2), "boss_hp": round(bhp, 1),
                "ttk_boss_seconds": round(ttk_boss, 3), "enemy_hp": round(ehp, 2),
                "ttk_enemy_seconds": round(ttk_enemy, 4), "incoming_threat": round(threat, 2),
            })

    if args.csv:
        with open(args.csv, "w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=list(csv_rows[0].keys()))
            writer.writeheader()
            writer.writerows(csv_rows)
        print(f"\nWrote {len(csv_rows)} rows to {args.csv}")


def run_progression(args):
    gates = sorted(C.SLOT_CAP_WAVE_GATES.items(), key=lambda kv: kv[1])
    print("Fresh save, worst case (no opportunistic dash-kill unlocks). Boss waves marked *mini* "
          "(every 5) / **MEGA** (every 10).\n")
    print(f"{'Wave':>5}{'SlotCap':>9}{'RealWeapons':>13}  Unlocked")
    checkpoints = sorted(set([1] + list(range(5, args.waves + 1, 5)) +
                             [C.HOMING_UNLOCK_WAVE, C.MINE_UNLOCK_WAVE, C.SHOCK_UNLOCK_WAVE]))
    for wave in checkpoints:
        if wave > args.waves:
            continue
        slot_cap = 3
        for count, gate_wave in gates:
            if wave >= gate_wave:
                slot_cap = count
        unlocked = ["ship-default"]
        if wave >= C.HOMING_UNLOCK_WAVE:
            unlocked.append("Homing")
        if wave >= C.MINE_UNLOCK_WAVE:
            unlocked.append("Mine")
        if wave >= C.SHOCK_UNLOCK_WAVE:
            unlocked.append("Shock")
        marker = " **MEGA**" if wave % 10 == 0 else (" *mini*" if wave % 5 == 0 else "")
        print(f"{wave:>5}{slot_cap:>9}{len(unlocked):>13}  {', '.join(unlocked)}{marker}")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--mode", choices=["parity", "curve", "progression"], default="curve")
    p.add_argument("--builds", default=None, help="comma-separated preset names (default: all)")
    p.add_argument("--waves", type=int, default=100, help="last wave to simulate")
    p.add_argument("--wave-start", type=int, default=5)
    p.add_argument("--wave-step", type=int, default=5)
    p.add_argument("--enemies-in-range", type=int, default=3,
                    help="assumed crowd size for pierce/ricochet/AoE weapons")
    p.add_argument("--levels-per-wave", type=float, default=None,
                    help="override postCapDamageLevels growth rate for ALL builds (levels/wave "
                         "gained after a build's power-online wave). Live game has no cap on this.")
    p.add_argument("--postcap-cap", type=float, default=C.POSTCAP_LEVELS_CAP,
                    help=f"cap postCapDamageLevels at this value (default {C.POSTCAP_LEVELS_CAP}, "
                         "the rebalanced value — was uncapped before). Pass a huge number to "
                         "simulate the old uncapped behavior.")
    p.add_argument("--boss-hp-base", type=float, default=None)
    p.add_argument("--boss-hp-per-tier", type=float, default=None)
    p.add_argument("--boss-type-health-mult", type=float, default=1.0,
                    help="BossType.healthMult stand-in, range is 0.85-1.35 in the live game")
    p.add_argument("--csv", default=None, help="write curve-mode results to this CSV path")
    args = p.parse_args()

    if args.mode == "parity":
        run_parity(args)
    elif args.mode == "progression":
        run_progression(args)
    else:
        run_curve(args)


if __name__ == "__main__":
    main()
