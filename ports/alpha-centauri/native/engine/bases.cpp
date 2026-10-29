/* SPDX-License-Identifier: GPL-3.0-or-later
 * Base creation/removal and facility bit helpers, ported verbatim from Thinker
 * Mod src/base.cpp (replay_base from src/game.cpp) at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * mod_base_init/mod_base_kill keep the original bookkeeping order; the steps
 * owned by other packages (base_first, mod_name_base, mod_cost_factor,
 * base_compute, reset_territory, find_relocate_base, del_site) are declared in
 * engine_wp.h and provided by stubs until those packages land.
 * Changes: MessageWin/BaseWin edits replaced by EV_BASE_KILLED/EV_BASE_CREATED.
 * Extracted as whole functions; edit by hand from here on.
 */
#include "engine.h"

namespace alpha {
namespace engine {

int __cdecl mod_base_init(int faction_id, int x, int y) {
    MAP* sq;
    if (*BaseCount >= MaxBaseNum) {
        return -1;
    }
    if (faction_id < 0 || faction_id >= MaxPlayerNum
    || !(sq = mapsq(x, y)) || sq->is_base()) {
        assert(0);
        return -1;
    }
    Faction* plr = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    bit_set(x, y, BIT_BASE_IN_TILE, 1);
    owner_set(x, y, faction_id);
    synch_bit(x, y, faction_id);
    int base_id = (*BaseCount)++;
    BASE* base = &Bases[base_id];
    memset(base, 0, sizeof(BASE)); // Fix: initialize all variables
    debug("base_init %2d %2d %d %d\n", x, y, base_id, faction_id);
    plr->base_count++;
    plr->region_total_bases[sq->region]++;
    replay_base(REPLAY_INIT, x, y, faction_id);
    base->x = x;
    base->y = y;
    base->faction_id_former = faction_id;
    base->faction_id = faction_id;
    base->nerve_staple_turns_left = 0;
    base->assimilation_turns_left = 0;
    base->ai_plan_status = 0;
    base->pop_size = has_project(FAC_PLANETARY_TRANSIT_SYSTEM, faction_id)
        ? PlanetaryTransitPopSize : 1;
    base->visibility = (1 << faction_id) | sq->visibility;
    base->name[0] = '\0';
    mod_name_base(faction_id, &base->name[0], 1, is_ocean(sq));
    base->state_flags = 0;
    base->event_flags = 0;
    base->governor_flags = plr->base_governor_adv;
    base->minerals_accumulated = 0;
    base->nutrients_accumulated = 0;
    base->autoforward_land_base_id = -1;
    base->autoforward_sea_base_id = -1;
    base->autoforward_air_base_id = -1;
    base->queue_size = 0;
    base_first(base_id);

    if (*CurrentTurn) {
        base->minerals_accumulated = plr->SE_support < -1 ? 0 : Rules->retool_exemption;
    } else {
        int unit_id;
        if (has_tech(Units[BSC_FORMERS].preq_tech, faction_id)) {
            unit_id = BSC_FORMERS;
        } else {
            unit_id = plr->diff_level > 1 ? BSC_SCOUT_PATROL : BSC_COLONY_POD;
        }
        base->queue_items[0] = unit_id;
        base->minerals_accumulated = Units[unit_id].cost * mod_cost_factor(faction_id, RSC_MINERAL, -1) / 2;
    }
    base->worked_tiles = 0;
    base->specialist_total = 0;
    base->specialist_adjust = 0;
    base->eco_damage = 0;
    base->unk_y = -1;
    base->unk_x = -1;
    memset(&base->facilities_built, 0, 12);
    base->random_event_turns = 0;
    base->nerve_staple_count = 0;
    base->defend_goal = 0;
    base->defend_range = 0;
    base->pad_6 = 0;
    base->pad_7 = 0;
    base->pad_8 = 0;

    if (plr->base_count == 1) {
        set_fac(FAC_HEADQUARTERS, base_id, 1);
        if (*MultiplayerActive) {
            base->pop_size = 3;
            if (m->rule_talent || plr->SE_planet_base > 0) {
                base->pop_size = 4;
            }
            if (m->rule_drone || m->rule_population) {
                base->pop_size--;
            }
            set_fac(FAC_RECYCLING_TANKS, base_id, 1);
            if (*DiffLevel >= 3) {
                set_fac(FAC_RECREATION_COMMONS, base_id, 1);
            }
            if (plr->SE_economy_base > 0) {
                set_fac(FAC_ENERGY_BANK, base_id, 1);
            }
        } else {
            base->pop_size = clamp((*CurrentTurn + 49) / 50, 1, 5);
            if (*CurrentTurn >= 75) {
                set_fac(FAC_RECYCLING_TANKS, base_id, 1);
            }
            if (*CurrentTurn >= 100) {
                set_fac(FAC_RECREATION_COMMONS, base_id, 1);
            }
            if (*CurrentTurn >= 150) {
                set_fac(FAC_ENERGY_BANK, base_id, 1);
            }
        }
    }
    if (is_ocean(sq)) {
        set_fac(FAC_PRESSURE_DOME, base_id, 1);
    }
    for (int i = 0; i < MaxPlayerNum; i++) {
        base->factions_pop_size_intel[i] =
            (i == faction_id || (1 << i) & base->visibility ? base->pop_size : 0);
    }
    // Original function accepts IDs including satellites and stockpile energy but these
    // cannot be built on the base normally and will be ignored by additional set_fac checks.
    for (int i = 0; i < m->faction_bonus_count; i++) {
        if (m->faction_bonus_id[i] == RULE_FACILITY) {
            int item_id = clamp(m->faction_bonus_val1[i], 1, Fac_All_ID_Last);
            set_fac((FacilityId)item_id, base_id, 1);
        }
    }
    for (int i = 0; i < m->faction_bonus_count; i++) {
        if (m->faction_bonus_id[i] == RULE_FREEFAC) {
            int item_id = clamp(m->faction_bonus_val1[i], 1, Fac_All_ID_Last);
            if (has_tech(Facility[item_id].preq_tech, faction_id)) {
                set_fac((FacilityId)item_id, base_id, 1);
            }
        }
    }
    for (int i = 1; i <= Fac_All_ID_Last; i++) {
        if (has_tech(Facility[i].free_tech, faction_id)) {
            set_fac((FacilityId)i, base_id, 1);
        }
    }
    base->production_id_last = base->queue_items[0];
    base->minerals_accumulated_2 = base->minerals_accumulated;
    base->mineral_surplus_final = 0;
    base_mark(base_id);
    reset_territory();
    set_base(base_id);
    base_compute(1); // Always update
    *GameDrawState |= 2u;
    engine_notify(EV_BASE_CREATED, base_id, 0, faction_id, 0, x, y);
    return base_id;
}

void __cdecl mod_base_kill(int base_id) {
    if (base_id < 0 || base_id >= *BaseCount) {
        assert(0);
        return;
    }
    BASE* base = &Bases[base_id];
    int base_x = base->x;
    int base_y = base->y;
    int faction_id = base->faction_id;
    Faction* plr = &Factions[faction_id];
    debug("base_kill %2d %2d %d %d\n", base->x, base->y, base_id, faction_id);

    if (has_fac_built(FAC_HEADQUARTERS, base_id)) {
        if (plr->corner_market_turn > *CurrentTurn) {
            if (plr->corner_market_cost) {
                plr->corner_market_turn = 0;
                plr->corner_market_cost = 0;
                if (faction_id == *CurrentPlayerFaction) {
                    popp(ScriptFile, "CORNERFOILED", 0, "econwin_sm.pcx", 0);
                } else {
                    parse_says(0, get_adjective(faction_id), -1, -1);
                    popp(ScriptFile, "CORNERTHEMFOILED", 0, "econwin_sm.pcx", 0);
                }
            }
        }
    }
    if (base->pop_size < 0) {
        base->pop_size = 0;
    }
    replay_base(REPLAY_KILL, base_x, base_y, faction_id);
    --plr->base_count;
    --plr->region_total_bases[region_at(base_x, base_y)];
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->home_base_id == base_id) {
            if (veh->faction_id == faction_id) {
                base->faction_id = -1;
                int near_base_id = base_find_2(base_x, base_y, faction_id);
                base->faction_id = faction_id;
                if (near_base_id < 0) {
                    kill(i);
                } else {
                    veh->home_base_id = near_base_id;
                }
            } else {
                veh->home_base_id = -1;
            }
        }
    }
    bit_set(base_x, base_y, BIT_BASE_IN_TILE, 0);
    if (base_id < *BaseCount - 1) {
        memmove(&Bases[base_id], &Bases[base_id + 1], (*BaseCount - base_id - 1) * sizeof(BASE));
    }
    base = nullptr;
    --(*BaseCount);
    if (base_id == *ComputeBaseID) {
        *ComputeBaseID = -1;
    }
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[i].home_base_id > base_id) {
            Vehs[i].home_base_id--;
        }
    }
    for (int i = 0; i < *BaseCount; i++) {
        int16_t* fwd = &Bases[i].autoforward_land_base_id;
        for (int j = 0; j < 3; j++, fwd++) {
            if (*fwd == base_id) *fwd = -1;
            else if (*fwd > base_id) (*fwd)--;
        }
    }

    // Original: MessageWin list entries tagged (base_id | 0x40000000) are removed
    // and later tags shift down by one. The UI does this on EV_BASE_KILLED.
    engine_notify(EV_BASE_KILLED, base_id, 0, faction_id, 0, base_x, base_y);

    for (int i = 0; i <= SP_ID_Last - SP_ID_First; i++) {
        if (SecretProjects[i] == base_id) {
            SecretProjects[i] = (conf.rebuild_secret_projects ? SP_Unbuilt : SP_Destroyed);
        } else if (SecretProjects[i] > base_id) {
            SecretProjects[i]--;
        }
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (Factions[i].base_id_attack_target == base_id) {
            Factions[i].base_id_attack_target = -1;
        } else if (Factions[i].base_id_attack_target > base_id) {
            Factions[i].base_id_attack_target--;
        }
    }
    for (int i = 0; i < 49; i++) {
        int tx = wrap(base_x + TableOffsetX[i]);
        int ty = base_y + TableOffsetY[i];
        if (on_map(tx, ty)) {
            site_set(tx, ty, 0);
            if (i < 21) {
                bit_set(tx, ty, BIT_BASE_RADIUS, 0);
                if (using_at(tx, ty) == faction_id) {
                    using_set(tx, ty, 0);
                }
            }
        }
    }
    if (!is_ocean(mapsq(base_x, base_y))) {
        for (int i = *VehCount - 1; i >= 0; --i) {
            VEH* veh = &Vehs[i];
            if (veh->triad() == TRIAD_SEA
            && !map_range(base_x, base_y, veh->x, veh->y)) {
                kill(i);
            }
        }
    } else {
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehs[i];
            if (veh->plan() == PLAN_NAVAL_TRANSPORT
            && !map_range(base_x, base_y, veh->x, veh->y)) {
                stack_veh(i, 0);
            }
        }
        for (int i = *VehCount - 1; i >= 0; --i) {
            VEH* veh = &Vehs[i];
            if (veh->triad() == TRIAD_LAND && !veh->in_transit()
            && !map_range(base_x, base_y, veh->x, veh->y)) {
                kill(i);
            }
        }
    }
    // Original: BaseWin_base_killed(BaseWin, base_id); covered by EV_BASE_KILLED.
    for (int i = 0; i < *BaseCount; i++) {
        base_mark(i);
    }
    reset_territory();
    find_relocate_base(faction_id);
    *GameDrawState |= 2u;
}

void __cdecl set_base(int base_id) {
    *CurrentBaseID = base_id;
    *CurrentBase = &Bases[base_id];
}

void __cdecl base_mark(int base_id) {
    int x = Bases[base_id].x;
    int y = Bases[base_id].y;
    int faction_id = Bases[base_id].faction_id;
    for (int i = 0; i < TableRange[3]; i++) {
        int x2, y2;
        MAP* sq = next_tile(x, y, i, &x2, &y2);
        if (sq) {
            if (i < 21) {
                bit_set(x2, y2, BIT_BASE_RADIUS, 1);
                using_set(x2, y2, faction_id);
            } else {
                site_set(x2, y2, 0);
            }
        }
    }
    for (int i = 0; i < MaxPlayerNum; i++) {
        // Fix most likely error that used offset location here instead of base location
        del_site(i, AI_GOAL_COLONIZE, x, y, 3);
    }
}

bool has_facility(FacilityId item_id, int base_id) {
    assert(base_id >= 0 && base_id < *BaseCount);
    if (item_id <= 0) { // zero slot unused
        return false;
    }
    if (item_id >= SP_ID_First) {
        return project_base(item_id) == base_id;
    }
    return has_fac_built(item_id, base_id) || redundant(item_id, Bases[base_id].faction_id);
}

bool has_free_facility(FacilityId item_id, int faction_id) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    MFaction& m = MFactions[faction_id];
    for (int i = 0; i < m.faction_bonus_count; i++) {
        if (m.faction_bonus_val1[i] == item_id
        && (m.faction_bonus_id[i] == RULE_FACILITY
        || (m.faction_bonus_id[i] == RULE_FREEFAC
        && has_tech(Facility[item_id].preq_tech, faction_id)))) {
            return true;
        }
    }
    return false;
}

/*
Determine if the facility is redundant due to a secret project counting as that facility.
*/
int __cdecl redundant(FacilityId item_id, int faction_id) {
    FacilityId project_id;
    switch (item_id) {
      case FAC_NAVAL_YARD:
        project_id = FAC_MARITIME_CONTROL_CENTER;
        break;
      case FAC_PERIMETER_DEFENSE:
        project_id = FAC_CITIZENS_DEFENSE_FORCE;
        break;
      case FAC_COMMAND_CENTER:
        project_id = FAC_COMMAND_NEXUS;
        break;
      case FAC_BIOENHANCEMENT_CENTER:
        project_id = FAC_CYBORG_FACTORY;
        break;
      case FAC_QUANTUM_CONVERTER:
        project_id = FAC_SINGULARITY_INDUCTOR;
        break;
      case FAC_AEROSPACE_COMPLEX:
        project_id = FAC_CLOUDBASE_ACADEMY;
        break;
      case FAC_ENERGY_BANK:
        project_id = FAC_PLANETARY_ENERGY_GRID;
        break;
      default:
        return false;
    }
    return has_project(project_id, faction_id);
}

/*
Check if the base already has a particular facility built or if it's in the queue.
*/
int __cdecl has_fac(FacilityId item_id, int base_id, int queue_count) {
    if (base_id < 0 || base_id >= *BaseCount) { // Fix: added bounds checking
        assert(0);
        return false;
    }
    if (item_id >= FAC_SKY_HYDRO_LAB) {
        return false;
    }
    bool is_built = has_fac_built(item_id, base_id);
    if (is_built || !queue_count) {
        return is_built;
    }
    if (queue_count <= 0) {
        return false;
    }
    for (int i = 0; i < queue_count && i < 10; i++) {
        if (Bases[base_id].queue_items[i] == -item_id) {
            return true;
        }
    }
    return false;
}

/*
Check if the facility is built on the given base while excluding any satellites.
Includes additional conditions to return false if base_id or item_id is invalid.
*/
int __cdecl has_fac_built(FacilityId item_id, int base_id) {
    return base_id >= 0 && base_id < MaxBaseNum && item_id >= 0 && item_id <= Fac_ID_Last
        && !!(Bases[base_id].facilities_built[item_id/8] & (1 << (item_id % 8)));
}

/*
Modify built facilities on the base. Original set_fac does not check variable bounds.
*/
void __cdecl set_fac(FacilityId item_id, int base_id, bool add) {
    if (base_id >= 0 && base_id < MaxBaseNum && item_id >= 0 && item_id <= Fac_ID_Last) {
        if (add) {
            Bases[base_id].facilities_built[item_id/8] |= (1 << (item_id % 8));
        } else {
            Bases[base_id].facilities_built[item_id/8] &= ~(1 << (item_id % 8));
        }
    }
}

/*
Store base related events for the endgame replay screen.
0 = create base, 1 = change base owner, 2 = kill base.
*/
int __cdecl replay_base(int event, int x, int y, int faction_id) {
    assert(mapsq(x, y) && event >= 0 && event <= 2);
    debug("replay_base %d %d %d %d %d %d\n",
        *ReplayEventSize, *CurrentTurn, event, faction_id, x, y);
    if (*ReplayEventSize >= 0 && *ReplayEventSize < 8192) {
        ReplayEvent* p = &ReplayEvents[*ReplayEventSize/8];
        p->event = event;
        p->faction_id = faction_id;
        p->turn = *CurrentTurn;
        p->x = x;
        p->y = y;
        *ReplayEventSize += 8;
    }
    return *ReplayEventSize;
}

} // namespace engine
} // namespace alpha
