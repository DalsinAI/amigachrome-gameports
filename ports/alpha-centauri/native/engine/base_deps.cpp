/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP2: weak definitions of helpers the base code needs from other Thinker
 * Mod files (game.cpp, faction.cpp, probe.cpp, plan.cpp, path.cpp, build.cpp,
 * veh.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af), copied verbatim.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * They are weak so the package that ports the owning file supplies the strong
 * definition; see base_deps.h. Changes: asserts comparing with terranx.exe
 * routines removed (the executable is not available).
 * Integration: steal_tech, prototype_factor, mod_proto_cost, mod_base_cost,
 * mod_veh_cost, prune_protos and upgrade_prototypes are WP5's (tech.cpp,
 * design.cpp); their weak copies were removed here. world_climate and
 * mod_world_volcano are WP7's. un_charter, global_trade_pact, victory_done,
 * full_game_turn, voice_of_planet, generators, spying, council_votes,
 * great_beelzebub, great_satan, aah_ooga, climactic_battle, DiploFearScale
 * and DiploCanSurrender are WP8's (diplo.cpp, victory.cpp).
 */
#include "engine.h"
#include "base_wp2.h"
#include "base_deps.h"

namespace alpha {
namespace engine {

/*
Calculate the offset and bitmask for the specified input.
*/
__attribute__((weak)) void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask) {
    *offset = input / 8;
    *mask = 1 << (input & 7);
}

__attribute__((weak)) bool has_wmode(int faction_id, VehWeaponMode mode) {
    for (int i = 0; i < MaxWeaponNum; i++) {
        if (Weapon[i].mode == mode && has_tech(Weapon[i].preq_tech, faction_id)) {
            return true;
        }
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        UNIT* u = &Units[i];
        if (veh_avail(i, faction_id, -1) && u->weapon_mode() == mode) {
            return true;
        }
    }
    return false;
}

__attribute__((weak)) int __cdecl steal_energy(int base_id) {
    BASE* base = &Bases[base_id];
    int64_t energy = Factions[base->faction_id].energy_credits;
    if (energy > 0) {
        return clamp((energy * conf.steal_energy_rate * Bases[base_id].pop_size)
            / 100 / max(1, Factions[base->faction_id].pop_total + 1), int64_t{0}, energy);
    }
    return 0;
}

__attribute__((weak)) int satellite_count(int faction_id, int item_id) {
    Faction& f = Factions[faction_id];
    int value;
    switch (item_id) {
        case FAC_SKY_HYDRO_LAB: value = f.satellites_nutrient; break;
        case FAC_ORBITAL_POWER_TRANS: value = f.satellites_energy; break;
        case FAC_NESSUS_MINING_STATION: value = f.satellites_mineral; break;
        case FAC_ORBITAL_DEFENSE_POD: default: value = f.satellites_ODP; break;
    }
    return value;
}

__attribute__((weak)) int satellite_goal(int faction_id, int item_id) {
    Faction& f = Factions[faction_id];
    AIPlans& p = plans[faction_id];
    int goal = p.satellite_goal;
    if (item_id == FAC_ORBITAL_DEFENSE_POD) {
        int nukes = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (faction_id != i && is_alive(i) && at_war(faction_id, i)) {
                nukes = max(nukes, Factions[i].planet_busters);
            }
        }
        if (p.enemy_odp > 0 || p.enemy_sat > 0 || nukes > 2) {
            goal = clamp(goal/4, 0, 4) + clamp(f.base_count/8 + nukes/2, 2, 12);
        } else {
            goal = clamp(goal/4, 0, 4) + clamp(f.base_count/8, 2, 4);
        }
    } else {
        if (f.base_count <= 5) {
            goal /= 2;
        }
    }
    if (f.base_count <= 10) {
        goal /= 2;
    }
    return clamp(goal, 0, conf.max_satellites);
}

__attribute__((weak)) int defender_count(int x, int y, int veh_skip_id) {
    int num = 0;
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->x == x && veh->y == y
        && veh->order != ORDER_SENTRY_BOARD
        && veh->at_target()
        && i != veh_skip_id) {
            num += veh->eval_garrison();
        }
    }
    return (num+1)/4;
}

__attribute__((weak)) int garrison_count(int x, int y) {
    int num = 0;
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->x == x && veh->y == y
        && veh->is_garrison_unit()
        && veh->order != ORDER_SENTRY_BOARD
        && veh->at_target()) {
            num++;
        }
    }
    return num;
}

__attribute__((weak)) int consider_staple(int base_id) {
    BASE* b = &Bases[base_id];
    if (can_staple(base_id)
    && (!un_charter() || (*SunspotDuration > 1 && *DiffLevel >= DIFF_LIBRARIAN))
    && base_can_riot(base_id, true)
    && b->nerve_staple_count < 4
    && b->pop_size / 4 >= b->nerve_staple_count
    && b->drone_total + b->specialist_adjust > 0
    && (b->faction_id == b->faction_id_former || !is_alive(b->faction_id_former)
    || want_revenge(b->faction_id, b->faction_id_former))) {
        if (b->drone_riots_active() || b->drone_total + b->specialist_adjust
        > b->talent_total + (b->nutrient_surplus > 0 && b->mineral_surplus > 0)) {
            action_staple(base_id);
        }
    }
    return 0;
}

__attribute__((weak)) int wp2_project_hurry_score(int UNUSED(base_id), FacilityId UNUSED(item_id)) {
    return 0;
}

} // namespace engine
} // namespace alpha
