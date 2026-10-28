/* SPDX-License-Identifier: GPL-3.0-or-later
 * Vehicle records and stacks ported verbatim from Thinker Mod src/veh.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Changes: window bookkeeping in veh_kill/kill raises EV_VEH_KILLED through
 * the event sink; network-only code removed (marked "Original:").
 * Extracted as whole functions; edit by hand from here on.
 */
#include "engine.h"

namespace alpha {
namespace engine {

int __cdecl can_arty(int unit_id, bool allow_sea_arty) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    if (unit_id < 0 || unit_id >= MaxProtoNum) {
        return false;
    }
    UNIT* u = &Units[unit_id];
    if ((u->offense_value() <= 0 // PSI + non-combat
    || u->defense_value() < 0) // PSI
    && unit_id != BSC_SPORE_LAUNCHER) { // Spore Launcher exception
        return false;
    }
    int triad = u->triad();
    if (triad == TRIAD_SEA) {
        return allow_sea_arty;
    }
    if (triad == TRIAD_AIR) {
        return false;
    }
    return has_abil(unit_id, ABL_ARTILLERY); // TRIAD_LAND
}

int __cdecl has_abil(int unit_id, VehAblFlag ability) {
    int faction_id = unit_id / MaxProtoFactionNum;
    // workaround fix for legacy base_build that may use incorrect unit_id
    assert(!thinker_enabled(faction_id) || (unit_id >= 0 && unit_id < MaxProtoNum));
    if (unit_id < 0 || unit_id >= MaxProtoNum) {
        return false;
    }
    if (Units[unit_id].ability_flags & ability) {
        return true;
    }
    if (!faction_id) {
        return false; // skip basic prototypes from #UNITS
    }
    if (is_alien(faction_id) && ability == ABL_DEEP_RADAR) {
        return true; // Caretakers + Usurpers > "Deep Radar" ability for all units
    }
    // Pirates > "Marine Detachment" ability for combat sea units with Adaptive Doctrine
    for (int i = 0; i < MFactions[faction_id].faction_bonus_count; i++) {
        if (MFactions[faction_id].faction_bonus_id[i] == RULE_FREEABIL) {
            int abil_bonus_id = MFactions[faction_id].faction_bonus_val1[i];
            if (has_tech(Ability[abil_bonus_id].preq_tech, faction_id)
            && (ability & (1 << abil_bonus_id))) {
                return true;
            }
        }
    }
    // Nethack Terminus allows all probe teams to act as they have "Algorithmic Enhancement".
    // Manual implies fusion reactor or better is required for this but has_abil does not check it.
    if (ability == ABL_ALGO_ENHANCEMENT && Units[unit_id].is_probe()
    && has_project(FAC_NETHACK_TERMINUS, faction_id)) {
        return true;
    }
    return false;
}

int __cdecl arty_range(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    UNIT* u = &Units[unit_id];
    if (can_arty(unit_id, true)) {
        if (*MultiplayerActive) {
            return Rules->artillery_max_rng;
        }
        return min(MaxTableRange, Rules->artillery_max_rng
            + (conf.long_range_artillery > 0 && u->triad() == TRIAD_SEA
            && u->offense_value() > 0 && u->ability_flags & ABL_ARTILLERY
            ? conf.long_range_artillery : 0));
    }
    return 0;
}

int __cdecl drop_range(int faction_id) {
    if (!has_orbital_drops(faction_id)) {
        return max(0, Rules->max_airdrop_rng_wo_orbital_insert);
    }
    return max(*MapAreaX, *MapAreaY);
}

bool has_orbital_drops(int faction_id) {
    return has_tech(Rules->tech_preq_orb_insert_wo_space, faction_id)
        || has_project(FAC_SPACE_ELEVATOR, faction_id);
}

bool has_ability(int faction_id, VehAbl abl, VehChassis chs, VehWeapon wpn) {
    int F = Ability[abl].flags;
    int triad = Chassis[chs].triad;
    bool is_combat = Weapon[wpn].offense_value != 0;
    bool is_former = Weapon[wpn].mode == WMODE_TERRAFORM;
    bool is_probe = Weapon[wpn].mode == WMODE_PROBE;

    if ((triad == TRIAD_LAND && !(F & AFLAG_ALLOWED_LAND_UNIT))
    || (triad == TRIAD_SEA && !(F & AFLAG_ALLOWED_SEA_UNIT))
    || (triad == TRIAD_AIR && !(F & AFLAG_ALLOWED_AIR_UNIT))) {
        return false;
    }
    if ((!(F & AFLAG_ALLOWED_COMBAT_UNIT) && is_combat)
    || (!(F & AFLAG_ALLOWED_TERRAFORM_UNIT) && is_former)
    || (!(F & AFLAG_ALLOWED_NONCOMBAT_UNIT) && !is_combat && !is_former)) {
        return false;
    }
    if ((F & AFLAG_NOT_ALLOWED_PROBE_TEAM && is_probe)
    || (F & AFLAG_ONLY_PROBE_TEAM && !is_probe)) {
        return false;
    }
    if (F & AFLAG_TRANSPORT_ONLY_UNIT && Weapon[wpn].mode != WMODE_TRANSPORT) {
        return false;
    }
    if (F & AFLAG_NOT_ALLOWED_FAST_UNIT && Chassis[chs].speed > 1) {
        return false;
    }
    if (F & AFLAG_NOT_ALLOWED_PSI_UNIT && Weapon[wpn].offense_value < 0) {
        return false;
    }
    return has_tech(Ability[abl].preq_tech, faction_id);
}

bool is_native_unit(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    return unit_id >= 0 && unit_id < MaxProtoFactionNum && Units[unit_id].offense_value() < 0;
}

/*
Fractional healing at monoliths is not implemented and for this reason
Battle Ogres may be excluded from healing/promoting at monoliths.
*/
bool is_battle_ogre(int unit_id) {
    return unit_id == BSC_BATTLE_OGRE_MK1 || unit_id == BSC_BATTLE_OGRE_MK2 || unit_id == BSC_BATTLE_OGRE_MK3;
}

bool can_repair(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    return conf.repair_battle_ogre > 0 || !is_battle_ogre(unit_id);
}

bool can_monolith(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    return conf.repair_battle_ogre >= 10 || !is_battle_ogre(unit_id);
}

int __cdecl veh_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq && !sq->veh_in_tile()) {
        return -1;
    }
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[i].x == x && Vehs[i].y == y) {
            return veh_top(i);
        }
    }
    if (!sq) {
        return -1;
    }
    if (!*VehBitError) {
        debug("VehBitError %d %d\n", x, y);
    }
    if (*GameState & STATE_SCENARIO_EDITOR || *GameState & STATE_DEBUG_MODE || *MultiplayerActive) {
        if (*VehBitError) {
            return -1;
        }
        *VehBitError = 1;
    }
    rebuild_vehicle_bits();
    return -1;
}

int __cdecl veh_who(int x, int y) {
    MAP* sq = mapsq(x, y);
    return (sq ? sq->veh_who() : -1);
}

int __cdecl veh_top(int veh_id) {
    if (veh_id < 0) {
        return -1;
    }
    int top_veh_id = veh_id;
    while (Vehs[top_veh_id].prev_veh_id_stack >= 0) {
        top_veh_id = Vehs[top_veh_id].prev_veh_id_stack;
    }
    return top_veh_id;
}

void __cdecl stack_put(int veh_id, int x, int y) {
    int next_veh_id = veh_top(veh_id);
    int veh_id_loop;
    if (next_veh_id >= 0) {
        do {
            veh_id_loop = Vehs[next_veh_id].next_veh_id_stack;
            veh_put(next_veh_id, x, y);
            next_veh_id = veh_id_loop;
        } while (veh_id_loop >= 0);
    }
}

/*
Sort the veh stack with the transports moved to the top.
*/
void __cdecl stack_sort(int veh_id) {
    int x = Vehs[veh_id].x;
    int y = Vehs[veh_id].y;
    int next_veh_id = veh_top(veh_id);
    int veh_id_put = -1;
    int veh_id_loop;
    if (veh_id >= 0 && next_veh_id >= 0) {
        do {
            veh_id_loop = Vehs[next_veh_id].next_veh_id_stack;
            if (veh_cargo(next_veh_id) || has_abil(Vehs[next_veh_id].unit_id, ABL_CARRIER)) {
                veh_id_put = next_veh_id;
                veh_put(next_veh_id, -3, -3);
            }
            next_veh_id = veh_id_loop;
        } while (veh_id_loop >= 0);
        stack_put(veh_id_put, x, y);
    }
}

/*
Sort the veh stack with colony pods at the top followed by combat/offensive units.
*/
void __cdecl stack_sort_2(int veh_id) {
    int x = Vehs[veh_id].x;
    int y = Vehs[veh_id].y;
    int next_veh_id = veh_top(veh_id);
    int veh_id_put = -1;
    int veh_id_loop;
    if (next_veh_id >= 0) {
        do {
            veh_id_loop = Vehs[next_veh_id].next_veh_id_stack;
            if (Units[Vehs[next_veh_id].unit_id].plan == PLAN_COLONY) { // Colony Pod
                veh_id_put = next_veh_id;
                veh_put(next_veh_id, -3, -3);
            }
            next_veh_id = veh_id_loop;
        } while (veh_id_loop >= 0);
    }
    next_veh_id = veh_at(x, y);
    if (next_veh_id >= 0) {
        do {
            veh_id_loop = Vehs[next_veh_id].next_veh_id_stack;
            if (Units[Vehs[next_veh_id].unit_id].plan <= PLAN_COMBAT) {
                veh_id_put = next_veh_id;
                veh_put(next_veh_id, -3, -3);
            }
            next_veh_id = veh_id_loop;
        } while (veh_id_loop >= 0);
    }
    stack_put(veh_id_put, x, y);
}

int __cdecl stack_fix(int veh_id) {
    if (veh_id < 0 || !*MultiplayerActive
    || (Vehs[veh_id].next_veh_id_stack < 0 && Vehs[veh_id].prev_veh_id_stack < 0)) {
        return veh_id;
    }
    for (int i = 0; i < *VehCount; i++) {
        // Fix: only promote and sort stacks in the same tile
        if (Vehs[veh_id].x == Vehs[i].x && Vehs[veh_id].y == Vehs[i].y) {
            veh_promote(i);
            stack_sort(veh_id);
        }
    }
    return veh_top(veh_id);
}

int __cdecl stack_veh(int veh_id, int flag) {
    if (veh_id < 0) {
        return flag != 0 ? veh_id : 0;
    }
    VEH* const veh = &Vehs[veh_id];
    const int faction_id = veh->faction_id;
    if (!veh_cargo(veh_id) && !has_abil(veh->unit_id, ABL_CARRIER)) {
        if (!flag) {
            return 0;
        }
        return veh_drop(veh_lift(veh_id), -2, -2);
    }
    if (*MultiplayerActive) {
        stack_fix(veh_id);
    }
    MAP* const veh_sq = mapsq(veh->x, veh->y);
    if (flag) {
        if (flag > 1) {
            if (veh_sq && veh_sq->base_who() >= 0) {
                return veh_drop(veh_lift(veh_id), -2, -2);
            }
        }
        stack_sort(veh_id);
    } else {
        for (int i = veh_top(veh_id); i >= 0; i = Vehs[i].next_veh_id_stack) {
            Vehs[i].state &= ~VSTATE_IN_TRANSPORT;
        }
    }
    bool at_open_sea = veh_sq && is_ocean(veh_sq) && veh_sq->base_who() < 0;
    bool is_air = veh->triad() == TRIAD_AIR;
    int cargo_val = veh_cargo(veh_id);
    int carrier_val = has_abil(veh->unit_id, ABL_CARRIER) ? max(1, cargo_val) : 0;
    if (!faction_id) {
        cargo_val = 99;
    }
    auto do_load = [&](int cur_id) {
        if (flag || !(Vehs[cur_id].state & VSTATE_IN_TRANSPORT)) {
            sleep(cur_id);
            Vehs[cur_id].waypoint_x[0] = veh_id;
            if (flag) {
                veh_drop(veh_lift(cur_id), -2, -2);
                Vehs[cur_id].state &= ~VSTATE_UNK_10000;
            } else {
                Vehs[cur_id].state |= VSTATE_IN_TRANSPORT;
            }
            if (carrier_val > 0) {
                --carrier_val;
            }
            if (cargo_val > 0) {
                --cargo_val;
            }
        }
    };
    int max_iter = (flag <= 1) ? 2 : 1;
    for (int iter = 0; iter < max_iter; ++iter) {
        int cur = (flag ? veh_id : veh_top(veh_id));
        int next = cur;
        while (next >= 0) {
            if (!cargo_val && !carrier_val) {
                break;
            }
            cur = next;
            next = Vehs[next].next_veh_id_stack;
            if (Vehs[cur].faction_id != faction_id) {
                continue;
            }
            if (!(cargo_val && Vehs[cur].triad() == TRIAD_LAND && !veh_cargo(cur))
            && !(carrier_val && Vehs[cur].triad() == TRIAD_AIR && !has_abil(Vehs[cur].unit_id, ABL_CARRIER))) {
                continue;
            }
            if (!iter) {
                if (Vehs[cur].order == ORDER_SENTRY_BOARD && Vehs[cur].waypoint_x[0] == veh_id) {
                    do_load(cur);
                }
                continue;
            }
            if (is_air && veh_sq->base_who() < 0 && !(veh_sq->items & BIT_AIRBASE)) {
                continue;
            }
            if (!is_human(Vehs[cur].faction_id) || (veh->state & VSTATE_ON_ALERT && Vehs[cur].state & VSTATE_ON_ALERT)) {
                int base_plan = Factions[Vehs[cur].faction_id].region_base_plan[region_at(Vehs[cur].x, Vehs[cur].y)];
                if (Vehs[cur].state & VSTATE_UNK_40000) {
                    if ((Vehs[cur].state & VSTATE_UNK_20000) && base_plan == PLAN_NAVAL_TRANSPORT) {
                        do_load(cur);
                    }
                    continue;
                }
                if (Vehs[cur].order != ORDER_HOLD) {
                    int cur_plan = Vehs[cur].plan();
                    if (cur_plan != PLAN_ARTIFACT && cur_plan != PLAN_TERRAFORM) {
                        if (at_open_sea || !on_map(Vehs[cur].x, Vehs[cur].y)
                        || cur_plan >= PLAN_COLONY || base_plan != PLAN_DEFENSE) {
                            do_load(cur);
                        }
                        continue;
                    }
                }
            } else if (Vehs[cur].order == ORDER_SENTRY_BOARD) {
                if (Vehs[cur].waypoint_x[0] < 0) {
                    do_load(cur);
                }
                continue;
            }
            if (at_open_sea) {
                do_load(cur);
            }
        }
    }
    if (flag) {
        return veh_drop(veh_lift(veh_id), -2, -2);
    }
    return (veh_cargo(veh_id) == 0) ? carrier_val : cargo_val;
}

void __cdecl stack_kill(int veh_id) {
    if (veh_id < 0) {
        return;
    }
    int cur_id = veh_top(veh_id);
    while (cur_id >= 0) {
        int next_id = Vehs[cur_id].next_veh_id_stack;
        veh_kill(cur_id);
        if (next_id > cur_id) {
            next_id--;
        }
        cur_id = next_id;
    }
}

/*
Temporarily remove the specified unit from its current tile and stack in preparation for
another action such as interacting with the stack, moving or killing it.
Return Value: veh_id
*/
int __cdecl veh_lift(int veh_id) {
    bool prev_stack_exists = false;
    int16_t prev_veh_id = Vehs[veh_id].prev_veh_id_stack;
    int16_t next_veh_id = Vehs[veh_id].next_veh_id_stack;
    if (prev_veh_id >= 0) {
        prev_stack_exists = true;
        Vehs[prev_veh_id].next_veh_id_stack = next_veh_id;
    }
    int x = Vehs[veh_id].x;
    int y = Vehs[veh_id].y;
    if (next_veh_id >= 0) {
        Vehs[next_veh_id].prev_veh_id_stack = prev_veh_id;
    } else if (!prev_stack_exists && on_map(x, y)) {
        bit_set(x, y, BIT_VEH_IN_TILE, false);
    }
    *VehDropLiftVehID = veh_id;
    *VehLiftX = x;
    *VehLiftY = y;
    Vehs[veh_id].x = -1;
    Vehs[veh_id].y = -1;
    Vehs[veh_id].next_veh_id_stack = -1;
    Vehs[veh_id].prev_veh_id_stack = -1;
    return veh_id;
}

/*
Move the specified unit to the provided coordinates.
Return Value: veh_id (first param), seems not to be used
*/
int __cdecl veh_drop(int veh_id, int x, int y) {
    int veh_id_dest = veh_at(x, y);
    Vehs[veh_id].next_veh_id_stack = (int16_t)veh_id_dest;
    Vehs[veh_id].prev_veh_id_stack = -1;
    Vehs[veh_id].x = (int16_t)x;
    Vehs[veh_id].y = (int16_t)y;
    *VehDropLiftVehID = -1;
    if (veh_id_dest < 0) {
        if (y < 0) {
            return veh_id;
        }
        if (on_map(x, y) && !(bit_at(x, y) & BIT_BASE_IN_TILE)) {
            owner_set(x, y, Vehs[veh_id].faction_id);
        }
    } else {
        Vehs[veh_id_dest].prev_veh_id_stack = (int16_t)veh_id;
    }
    if (on_map(x, y)) {
        uint32_t flags = (Vehs[veh_id].faction_id && Vehs[veh_id].triad() != TRIAD_AIR)
            ? (BIT_VEH_IN_TILE|BIT_SUPPLY_REMOVE) : BIT_VEH_IN_TILE;
        bit_set(x, y, flags, true);
    }
    return veh_id;
}

/*
Relocate an existing unit to the specified tile.
*/
void __cdecl veh_put(int veh_id, int x, int y) {
    veh_drop(veh_lift(veh_id), x, y);
}

/*
Set the unit's status to sentry/board.
*/
void __cdecl sleep(int veh_id) {
    Vehs[veh_id].order = ORDER_SENTRY_BOARD;
    Vehs[veh_id].waypoint_x[0] = -1;
    Vehs[veh_id].waypoint_y[0] = 0;
}

/*
Move the specified unit to the bottom of the stack.
*/
void __cdecl veh_demote(int veh_id) {
    if (veh_id >= 0) {
        int16_t next_veh_id = Vehs[veh_id].next_veh_id_stack;
        if (next_veh_id >= 0) {
            int16_t last_veh_id;
            do {
                last_veh_id = next_veh_id;
                next_veh_id = Vehs[last_veh_id].next_veh_id_stack;
            } while (next_veh_id >= 0);
            if (last_veh_id != veh_id) {
                veh_lift(veh_id);
                Vehs[last_veh_id].next_veh_id_stack = (int16_t)veh_id;
                Vehs[veh_id].prev_veh_id_stack = last_veh_id;
                Vehs[veh_id].next_veh_id_stack = -1;
                Vehs[veh_id].x = Vehs[last_veh_id].x;
                Vehs[veh_id].y = Vehs[last_veh_id].y;
            }
        }
    }
}

/*
Move the specified unit to the top of the stack.
*/
void __cdecl veh_promote(int veh_id) {
    int veh_id_top = veh_top(veh_id);
    if (veh_id_top >= 0 && veh_id_top != veh_id) {
        veh_put(veh_id, Vehs[veh_id_top].x, Vehs[veh_id_top].y);
    }
}

/*
Clear the specified unit (called by veh_init/veh_fake).
*/
void __cdecl veh_clear(int veh_id, int unit_id, int faction_id) {
    VEH* veh = &Vehs[veh_id];
    veh->x = -4;
    veh->y = -4;
    veh->state = 0;
    veh->flags = 0;
    veh->pad_0 = 0;
    veh->unit_id = (int16_t)unit_id;
    veh->faction_id = (uint8_t)faction_id;
    veh->year_end_lurking = 0;
    veh->damage_taken = 0;
    veh->order = ORDER_NONE;
    veh->waypoint_count = 0;
    veh->patrol_current_point = 0;
    for (int i = 0; i < 4; i++) {
        veh->waypoint_x[i] = -1;
        veh->waypoint_y[i] = -1;
    }
    veh->morale = clamp(MFactions[faction_id].rule_morale + 1, 0, MaxMoraleNum-1);
    veh->movement_turns = 0;
    veh->order_auto_type = 0;
    veh->visibility = 0;
    veh->moves_spent = 0;
    veh->rotate_angle = 2;
    veh->iter_count = 0;
    veh->status_icon = 0;
    veh->probe_action = 0;
    veh->probe_sabotage_id = 0;
    veh->home_base_id = -1;
    veh->next_veh_id_stack = -1;
    veh->prev_veh_id_stack = -1;
}

int __cdecl veh_init(int unit_id, int faction_id, int x, int y) {
    if (*VehCount >= conf.max_veh_num) {
        return -1;
    }
    int veh_id = (*VehCount)++;
    VEH* veh = &Vehs[veh_id];
    Faction* plr = &Factions[faction_id];
    // log_say("Adding Vehicle (v,who,v2)", veh_id, faction_id, unit_id);
    debug("veh_init %2d %2d %d %d %d\n", x, y, faction_id, unit_id, veh_id);

    if (Units[unit_id].plan <= PLAN_NAVAL_TRANSPORT) {
        plr->total_combat_units++;
    }
    plr->units_active[unit_id]++;
    veh_clear(veh_id, unit_id, faction_id);

    int base_id = base_find(x, y);
    if (Units[unit_id].plan == PLAN_ARTIFACT) {
        if (*GameState & STATE_SCN_VICT_ALL_ARTIFACTS_OBJ_UNIT) {
            veh->flags |= VFLAG_IS_OBJECTIVE;
        }
    } else if (base_id >= 0 && Bases[base_id].faction_id == faction_id) {
        veh->home_base_id = base_id;
        if (*ComputeBaseID == base_id) {
            *ComputeBaseID = -1;
        }
    }
    veh_drop(veh_id, x, y);
    spot_all(veh_id, 1);

    if (plr->target_x < 0) {
        return veh_id;
    }
    MAP* sq = mapsq(x, y);
    if (is_ocean(sq)) {
        return veh_id;
    }
    UNIT* unit = &Units[veh->unit_id];
    if (unit->plan > PLAN_DEFENSE && unit->plan != PLAN_PROBE) {
        return veh_id;
    }
    int region_id = sq->region;
    if (region_at(plr->target_x, plr->target_y) != region_id) {
        return veh_id;
    }
    int best_enemy = -1;
    int best_enemy_bases = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id
        && (plr->diplo_status[i] & DIPLO_VENDETTA)
        && plr->region_total_bases[region_id] > best_enemy_bases) {
            best_enemy_bases = plr->region_total_bases[region_id];
            best_enemy = i;
        }
    }
    if (best_enemy >= 0) {
        base_find_3(x, y, best_enemy, region_id, -1, -1);
        if (vector_dist(x, y, plr->target_x, plr->target_y) > *BaseFindDist) {
            return veh_id;
        }
    }
    int bases = plr->region_total_bases[region_id];
    int target = clamp(2 * bases / 3, 3, 8);

    if (plr->region_base_plan[region_id] == 7) {
        int alt_target = 2 * plr->unk_70 / 0x400000;
        if (target >= alt_target) {
            target = alt_target;
        }
    }
    if (plr->unk_71 - bit_count(plr->unk_72) >= target) {
        return veh_id;
    }
    int weap_val = weap_strat(unit->weapon_id, faction_id);
    int arm_val = arm_strat(unit->armor_id, faction_id);

    if (veh->triad() != TRIAD_LAND || has_abil(veh->unit_id, ABL_DROP_POD)) {
        return veh_id;
    }
    if (unit->plan == PLAN_PROBE) {
        if (!(plr->unk_72 & 2)) {
            veh->state |= VSTATE_UNK_40000;
            plr->unk_71++;
            plr->unk_72 |= 2;
        }
        return veh_id;
    }
    if (weap_val >= arm_val) {
        if (plr->region_base_plan[region_id] != 7) {
            int cap = clamp(2 * bases / 3, 3, 8);
            if (!(game_rand() % (cap + 2))) {
                veh->state |= VSTATE_UNK_20000;
                return veh_id;
            }
        }
    } else if ((plr->unk_72 & 1) || plr->region_base_plan[region_id] == 7) {
        return veh_id;
    }
    veh->state |= VSTATE_UNK_40000;
    plr->unk_71++;
    if (weap_val < arm_val) {
        plr->unk_72 |= 1;
    }
    return veh_id;
}

int __cdecl veh_init_free(int unit_id, int faction_id, int x, int y) {
    int veh_id = veh_init(unit_id, faction_id, x, y);
    if (veh_id >= 0) {
        Vehs[veh_id].home_base_id = -1;
        if (thinker_move_upkeep(faction_id)) {
            // Set these flags to disable any non-Thinker unit automation
            Vehs[veh_id].state |= VSTATE_UNK_40000;
            Vehs[veh_id].state &= ~VSTATE_UNK_2000;
        }
    }
    return veh_id;
}

int __cdecl mod_veh_kill(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    debug("disband %2d %2d %s\n", veh->x, veh->y, veh->name());
    veh_kill(veh_id);
    return VEH_SKIP;
}

void __cdecl veh_kill(int veh_id) {
    if (veh_id < 0) {
        return;
    }
    VEH* veh = &Vehs[veh_id];
    const int x = veh->x;
    const int y = veh->y;
    const int unit_id = veh->unit_id;
    const int faction_id = veh->faction_id;
    const int home_base_id = veh->home_base_id;
    // log_say("KILLINGVEHICLE", veh_id, faction_id, unit_id);
    debug("veh_kill %2d %2d %d %d %d\n", x, y, faction_id, unit_id, veh_id);

    // Original: DeletionList_add_deletion(DeleteList, veh_id) in network games (not supported).
    if (Units[unit_id].plan <= PLAN_NAVAL_TRANSPORT) {
        --Factions[faction_id].total_combat_units;
    }
    --Factions[faction_id].units_active[unit_id];
    veh_kill_lift(veh_id);

    if (veh_id < *VehCount - 1) {
        memmove(&Vehs[veh_id], &Vehs[veh_id + 1], (*VehCount - veh_id - 1) * sizeof(VEH));
    }
    if (veh_id < conf.max_veh_num) {
        --(*VehCount);
    }
    for (int i = 0; i < *VehCount; i++) {
        VEH* v = &Vehs[i];
        if (v->prev_veh_id_stack > veh_id) {
            --v->prev_veh_id_stack;
        }
        if (v->next_veh_id_stack > veh_id) {
            --v->next_veh_id_stack;
        }
        if (v->order == ORDER_SENTRY_BOARD) {
            if (v->waypoint_x[0] == veh_id) {
                v->order = ORDER_NONE;
            } else if (v->waypoint_x[0] > veh_id) {
                --v->waypoint_x[0];
            }
        }
    }
    auto update_veh_ref = [&](int32_t* ptr) {
        if (*ptr == veh_id) { *ptr = -1; }
        else if (*ptr > veh_id) { --(*ptr); }
    };
    update_veh_ref(&MapWin->VehID);
    update_veh_ref(&MapWin->ReadyVehID);
    update_veh_ref(&StatusWin->pending_veh_id);
    update_veh_ref(&StatusWin->active_veh_id);

    if (*CurrentVehID && *CurrentVehID >= veh_id) {
        --(*CurrentVehID);
    }
    // Original: network veh_id pointer fixup (dword_93E908) omitted; no network play.
    // Original: BaseWin_check_loc/BaseWin_check_base, now a sink notification.
    engine_notify(EV_VEH_KILLED, veh_id, unit_id, faction_id, home_base_id, x, y);
}

void __cdecl kill(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    const int x = veh->x;
    const int y = veh->y;
    const int unit_id = veh->unit_id;
    const int faction_id = veh->faction_id;
    const int home_base_id = veh->home_base_id;
    (void)home_base_id; // see BaseWin_check_base below
    const int deep_radar = has_abil(unit_id, ABL_DEEP_RADAR);

    if (!veh_cargo(veh_id)) { // remove redundant Spore Launcher reference
        veh_kill(veh_id);
    } else if (!on_map(x, y)) {
        if (x == -2) {
            stack_kill(veh_id);
            return;
        }
        stack_veh(veh_id, 2);
        stack_kill(veh_id);
    } else if (!is_ocean(mapsq(x, y))) {
        veh_kill(veh_id);
    } else {
        stack_veh(veh_id, 2);
        stack_kill(veh_id);
    }
    // Original: BaseWin_check_base(BaseWin, home_base_id); covered by EV_VEH_KILLED.
    if (!*VehBattleState && on_map(x, y)) {
        int alt_id = veh_at(x, y);
        if (faction_id != MapWin->cOwner
        || (stack_check(alt_id, 1, faction_id, -1, -1) && !deep_radar)) {
            draw_tile(x, y, 2);
        } else {
            draw_radius(x, y, 2, 2);
        }
    }
}

/*
Calculate the cargo capacity of a unit. This version removes a redundant reference on
Spore Launchers which was never triggered since their carry_capacity was set to zero.
*/
int __cdecl veh_cargo(int veh_id) {
    VEH* v = &Vehs[veh_id];
    UNIT* u = &Units[v->unit_id];
    if (u->carry_capacity && v->unit_id < MaxProtoFactionNum && u->offense_value() < 0) {
        return v->morale + 1;
    }
    return u->carry_capacity;
}

/*
Calculate the armor strategy for the specified armor id.
Return Value: Armor strategy
*/
int __cdecl arm_strat(int armor_id, int faction_id) {
    assert(armor_id >= 0 && armor_id < MaxArmorNum);
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    if (!*ExpansionEnabled && armor_id > ARM_PSI_DEFENSE) {
        return 1;
    }
    int value = Armor[armor_id].defense_value;
    if (value < 0) {
        value = psi_factor((Rules->psi_combat_ratio_def[TRIAD_LAND]
            * Factions[faction_id].enemy_best_weapon_value)
            / Rules->psi_combat_ratio_atk[TRIAD_LAND], faction_id, false, false);
    }
    return value;
}

/*
Calculate the weapon strategy for the specified weapon id.
Return Value: Weapon strategy
*/
int __cdecl weap_strat(int weapon_id, int faction_id) {
    assert(weapon_id >= 0 && weapon_id < MaxWeaponNum);
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    if (!*ExpansionEnabled && (weapon_id == WPN_RESONANCE_LASER || weapon_id == WPN_RESONANCE_BOLT
    || weapon_id == WPN_STRING_DISRUPTOR)) {
        return 1;
    }
    int value = Weapon[weapon_id].offense_value;
    if (value < 0) {
        value = psi_factor((Rules->psi_combat_ratio_atk[TRIAD_LAND]
            * Factions[faction_id].enemy_best_armor_value)
            / Rules->psi_combat_ratio_def[TRIAD_LAND], faction_id, true, false);
    }
    return value;
}

/*
Calculate the weapon value for the specified prototype.
Return Value: Weapon value
*/
int __cdecl weap_val(int unit_id, int faction_id) {
    return weap_strat(Units[unit_id].weapon_id, faction_id);
}

/*
Calculate the armor value for the specified armor id.
Return Value: Armor value
*/
int __cdecl arm_val(int armor_id, int faction_id) {
    return 2 * ((faction_id >= 0) ? arm_strat(armor_id, faction_id) : Armor[armor_id].defense_value);
}

/*
Calculate the armor value for the specified prototype.
Return Value: Armor value
*/
int __cdecl armor_val(int unit_id, int faction_id) {
    return arm_val(Units[unit_id].armor_id, faction_id);
}

/*
Various unit stack related calculations based on type parameter (0-19) and conditions.
*/
int __cdecl stack_check(int veh_id, int type, int cond1, int cond2, int cond3) {
    assert(type >= 0 && type <= 19);
    int value = 0;
    for (int i = veh_top(veh_id); i >= 0; i = Vehs[i].next_veh_id_stack) {
        switch (type) {
        case 0:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2) && Vehs[i].unit_id == cond1) {
                value++;
            }
            break;
        case 1:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                value++;
            }
            break;
        case 2:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2) && (uint8_t)Vehs[i].plan() == cond1) {
                value++;
            }
            break;
        case 3:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2) && Vehs[i].triad() == cond1) {
                value++;
            }
            break;
        case 4:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                value += Vehs[i].offense_value();
            }
            break;
        case 5:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                value += Vehs[i].defense_value();
            }
            break;
        case 6:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2)
            && has_abil(Vehs[i].unit_id, (VehAblFlag)cond1)) {
                value++;
            }
            break;
        case 7:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                value += Units[Vehs[i].unit_id].cost;
            }
            break;
        case 8:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                int triad = Vehs[i].triad();
                if (triad == TRIAD_LAND) {
                    value--;
                } else if (triad == TRIAD_SEA) {
                    value += veh_cargo(i);
                }
            }
            break;
        case 9:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2) && (int8_t)Vehs[i].order == cond1) {
                value++;
            }
            break;
        case 10:
            if (Vehs[i].faction_id == cond1) {
                value++;
            }
            break;
        case 11:
            if ((cond3 < 0 || Vehs[i].faction_id == cond3)
            && (Vehs[i].state & cond1) == (uint32_t)cond2) {
                value++;
            }
            break;
        case 12:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                if (arm_strat(Units[Vehs[i].unit_id].armor_id, Vehs[i].faction_id)
                > weap_strat(Units[Vehs[i].unit_id].weapon_id, Vehs[i].faction_id)) {
                    value++;
                }
            }
            break;
        case 13:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1) && Vehs[i].is_missile()) {
                value++;
            }
            break;
        case 14:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1)
            && (Vehs[i].plan() == PLAN_COMBAT || Vehs[i].plan() == PLAN_DEFENSE)) {
                value++;
            }
            break;
        case 15:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1) && can_arty(Vehs[i].unit_id, true)) {
                value++;
            }
            break;
        case 16:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1)
            && (Vehs[i].plan() == PLAN_COMBAT || Vehs[i].plan() == PLAN_DEFENSE
            || Vehs[i].plan() == PLAN_RECON)) {
                value++;
            }
            break;
        case 17:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2)
            && (uint8_t)Units[Vehs[i].unit_id].group_id == cond1) {
                value++;
            }
            break;
        case 18:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1)
            && (Vehs[i].triad() == TRIAD_AIR && Vehs[i].range() > 1)) {
                value++;
            }
            break;
        case 19:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1) && !Vehs[i].offense_value()) {
                value++;
            }
            break;
        default:
            break;
        }
    }
    return value;
}

/*
Check if the land unit inside an air transport can disembark.
The transport must be in either a base or an airbase to do so.
*/
int __cdecl veh_jail(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq = mapsq(veh->x, veh->y);
    if (veh->triad() == TRIAD_LAND && veh->order == ORDER_SENTRY_BOARD
    && veh->waypoint_x[0] >= 0 && Vehs[veh->waypoint_x[0]].triad() == TRIAD_AIR
    && sq && !sq->is_airbase()) { // Fix: also allow airbases with owner = 0
        return true;
    }
    return false;
}

/*
Purpose: Initialize/reset the fake veh_id used as a placeholder for various UI elements.
Return Value: veh_id (not used by active vehs, original value 2048)
*/
int __cdecl veh_fake(int unit_id, int faction_id) {
    veh_clear(conf.max_veh_num, unit_id, faction_id);
    return conf.max_veh_num;
}

} // namespace engine
} // namespace alpha
