/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP2: base actions. mod_base_hurry, hurry_item and proto_extra_cost are
 * Thinker Mod src/build.cpp, action_staple is src/basewin.cpp, and the player
 * hurry follows the Ctrl+H handler of src/gui.cpp, all at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * The production queue editing functions are new: the original edits the
 * queue inside the base window (BaseWin); these keep its encoding and its
 * units_queue bookkeeping (see base_queue/mod_base_change).
 */
#include "engine.h"
#include "base_wp2.h"
#include "base_deps.h"

#ifndef DEBUG
#define DEBUG 0
#endif

namespace alpha {
namespace engine {

void wp2_ui(const char* what, int a, int b, const char* text) {
    std::string msg = "ui:";
    msg += what ? what : "";
    if (text) {
        msg += " ";
        msg += text;
    }
    engine_notify(EV_MESSAGE, a, b, 0, 0, -1, -1, msg.c_str());
}

static int proto_extra_cost(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    return unit_id >= MaxProtoFactionNum && !Units[unit_id].is_prototyped()
        ? prototype_factor(unit_id) : 0;
}

int __cdecl mod_base_hurry() {
    const int base_id = *CurrentBaseID;
    BASE* b = &Bases[base_id];
    Faction* f = &Factions[b->faction_id];
    AIPlans* p = &plans[b->faction_id];
    assert(b == *CurrentBase);
    const int t = b->item();
    bool is_cheap = conf.simple_hurry_cost || b->minerals_accumulated >= Rules->retool_exemption;
    bool is_project = t <= -SP_ID_First && t >= -SP_ID_Last;
    bool player_gov = is_human(b->faction_id);
    int hurry_option;

    if (conf.base_hurry < 1) {
        return 0;
    } else if (player_gov) {
        if (!conf.manage_player_bases) {
            // Original: return base_hurry(); (terranx.exe governor hurry).
            // Fix: not ported; player bases are only hurried by the player.
            return 0;
        }
        hurry_option = (b->governor_flags & GOV_ACTIVE) &&
            (b->governor_flags & GOV_MAY_HURRY_PRODUCTION) ?
            ((b->governor_flags & GOV_MAY_PROD_SP) ? 2 : 1) : 0;
    } else {
        if (!thinker_enabled(b->faction_id)) {
            // Original: return base_hurry(); Fix: terranx.exe AI hurry not ported.
            return 0;
        }
        hurry_option = conf.base_hurry;
    }
    if (hurry_option < (is_project ? 2 : 1) || !b->can_hurry_item()) {
        return 0;
    }
    int mins = mineral_cost(base_id, t) - b->minerals_accumulated;
    int cost = hurry_cost(base_id, t, mins);
    int credits = max(0, f->energy_credits - f->hurry_cost_total);
    int reserve = clamp(*CurrentTurn * f->base_count / 16, 20, 500)
        * (conf.design_units && !player_gov && !(*CurrentTurn % 4) ? 2 : 1)
        * (!p->contacted_factions || is_project ? 1 : 4)
        * (b->defend_goal > 3 && p->enemy_factions > 0 ? 1 : 2)
        * (has_fac_built(FAC_HEADQUARTERS, base_id) ? 1 : 2) / 16;
    int divisor = max(1, 10 * b->mineral_surplus);
    int turns = (10 * max(0, mins) + divisor - 1) / divisor;

    if (!is_cheap || mins < 1 || cost < 1 || credits - cost < reserve) {
        return 0;
    }
    if (is_project) {
        int delay = player_gov ? 0 : clamp((*DiffLevel < DIFF_THINKER) + random(4), 0, 3);
        int threshold = 4*mod_cost_factor(b->faction_id, RSC_MINERAL, -1);

        if (has_project((FacilityId)-t) || turns < 2 + delay
        || (b->defend_goal > 3 && p->enemy_factions > 0)
        || (delay > 0 && b->mineral_surplus < 4)
        || b->minerals_accumulated < threshold) { // Avoid double cost threshold
            return 0;
        }
        for (int i = 0; i < *BaseCount; i++) {
            if (Bases[i].faction_id == b->faction_id
            && Bases[i].item() == t
            && Bases[i].minerals_accumulated > b->minerals_accumulated) {
                return 0;
            }
        }
        int proj_score = 0;
        std::vector<int> values;
        for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
            if (Facility[i].preq_tech != TECH_Disable) {
                // Original: facility_score((FacilityId)i, Wgov) with governor_priorities (WP6)
                int score = wp2_project_hurry_score(base_id, (FacilityId)i) + random(8);
                if (i == -t) {
                    proj_score = score;
                }
                values.push_back(score);
            }
        }
        std::sort(values.begin(), values.end());
        if (values.size() > 0 && proj_score < max(4, values[values.size()/2])) {
            return 0;
        }
        mins = mineral_cost(base_id, t) - b->minerals_accumulated
            - b->mineral_surplus/2 - delay*b->mineral_surplus;
        cost = hurry_cost(base_id, t, mins);

        if (cost > 0 && cost < credits && mins > 0 && mins > b->mineral_surplus) {
            hurry_item(base_id, mins, cost);
            if (DEBUG && conf.minimal_popups) {
                return 1;
            }
            if (!(*GameState & STATE_GAME_DONE) && b->faction_id != MapWin->cOwner
            && has_treaty(b->faction_id, MapWin->cOwner, DIPLO_COMMLINK)) {
                parse_says(0, MFactions[b->faction_id].adj_name_faction, -1, -1);
                parse_says(1, Facility[-t].name, -1, -1);
                popp(ScriptFile, "DONEPROJECT", 0, "secproj_sm.pcx", 0);
            }
            return 1;
        }
        return 0;
    }
    if (!is_project && t < 0 && turns > 1 && cost < credits/16
    && (!player_gov || b->governor_flags & GOV_MAY_PROD_FACILITIES)
    && credits >= clamp(80*f->base_count, 2000, 5000) && credits / 256 > random(64)) {
        return hurry_item(base_id, mins, cost);
    }
    if (t < 0 && (turns > 1 || b->drone_riots_active()) && cost < credits/8) {
        if ((t == -FAC_RECREATION_COMMONS || t == -FAC_PUNISHMENT_SPHERE
        || (t == -FAC_NETWORK_NODE && has_project(FAC_VIRTUAL_WORLD, b->faction_id)))
        && b->drone_total + b->specialist_adjust > b->talent_total) {
            return hurry_item(base_id, mins, cost);
        }
    }
    if (t < 0 && turns > 1 && cost < credits/8) {
        if (t == -FAC_RECYCLING_TANKS || t == -FAC_PRESSURE_DOME
        || t == -FAC_TREE_FARM || t == -FAC_HEADQUARTERS) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_CHILDREN_CRECHE && base_unused_space(base_id) > 2
        && b->nutrient_surplus > 0 && f->SE_growth_pending < GrowthPopBoom) {
            return hurry_item(base_id, mins, cost);
        }
        if ((t == -FAC_HAB_COMPLEX || t == -FAC_HABITATION_DOME)
        && base_unused_space(base_id) == 0 && b->nutrient_surplus > 0) {
            return hurry_item(base_id, mins, cost);
        }
        if ((t == -FAC_GENEJACK_FACTORY || t == -FAC_ROBOTIC_ASSEMBLY_PLANT
        || t == -FAC_NANOREPLICATOR || t == -FAC_QUANTUM_CONVERTER)
        && b->mineral_intake > Facility[-t].cost + 2*Facility[-t].maint
        && b->mineral_intake_2 < 40) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_PERIMETER_DEFENSE && b->defend_range < 12 && p->enemy_factions > 0) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_AEROSPACE_COMPLEX
        && (f->satellites_ODP > 0 || f->satellites_nutrient > 0
        || f->satellites_mineral > 0 || f->satellites_energy > 0)) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_PSI_GATE && b->defend_range < 16
        && p->main_region != p->target_land_region
        && region_at(b->x, b->y) == p->target_land_region) {
            return hurry_item(base_id, mins, cost);
        }
    }
    if (t >= 0 && turns > 1 && cost < credits/8 && mins < 35) {
        if (proto_extra_cost(t) > 0 && cost > 50) {
            return 0;
        }
        if (Units[t].is_combat_unit()) {
            int val = (cost < credits/16)
                + (p->enemy_bases > 0)
                + (p->enemy_factions > 0)
                + (b->defend_goal > 2)
                + (b->defend_range < 8)
                + (b->defend_range < 12)
                + (b->state_flags & BSTATE_COMBAT_LOSS_LAST_TURN ? 2 : 0)
                + max(-2, 2 - defender_count(b->x, b->y, -1));
            if (b->mineral_surplus*2 > b->mineral_intake_2 && cost < 40) {
                val += random(clamp((credits - cost) / 256, 0, 8));
            }
            if (val > 4) {
                return hurry_item(base_id, mins, cost);
            }
        }
        if ((Units[t].is_former() || Units[t].is_supply())
        && turns > (*CurrentTurn + base_id) % 16
        && (cost < credits/16 || b->mineral_surplus < p->median_limit)) {
            return hurry_item(base_id, mins, cost);
        }
        if (Units[t].is_colony() && b->pop_size > 1
        && (cost < credits/16 || turns > (*CurrentTurn + base_id) % 16)
        && (b->drone_riots_active()
        || (!base_unused_space(base_id) && b->nutrient_surplus > 1)
        || (base_can_riot(base_id, true)
        && b->drone_total + b->specialist_adjust > b->talent_total))) {
            return hurry_item(base_id, mins, cost);
        }
    }
    return 0;
}

int hurry_item(int base_id, int mins, int cost) {
    BASE* b = &Bases[base_id];
    Faction* f = &Factions[b->faction_id];
    debug("hurry_item %d %d mins: %d cost: %d credits: %d %s / %s\n",
        *CurrentTurn, base_id, mins, cost, f->energy_credits, b->name, prod_name(b->item()));
    f->energy_credits -= cost;
    b->minerals_accumulated += mins;
    b->state_flags |= BSTATE_HURRY_PRODUCTION;

    // This also enables "zoom to base" feature in the message window
    if (b->faction_id == MapWin->cOwner) {
        parse_says(0, b->name, -1, -1);
        parse_says(1, prod_name(b->item()), -1, -1);
        popb("GOVHURRY", 0, -1, "facblt_sm.pcx", 0);
    }
    return 1;
}

void __cdecl action_staple(int base_id) {
    const int player_id = *CurrentPlayerFaction;
    BASE* const base = &Bases[base_id];
    const char* image = MFactions[base->faction_id].is_alien() ? "ALdrone_sm.pcx" : "drone_sm.pcx";
    bool failed = true;
    if (conf.nerve_staple_turns > 0) {
        ++base->nerve_staple_count;
        failed = (base->nerve_staple_count > 8) || (base->nerve_staple_count > 2 && !(game_rand() & 1));
    }
    if (failed) {
        if (base->faction_id == player_id) {
            parse_says(0, base->name, -1, -1);
            popp(ScriptFile, "NERVESTAPLEFAIL", 0, image, 0);
        }
    } else {
        base->state_flags &= ~BSTATE_DRONE_RIOTS_ACTIVE;
        base->nerve_staple_turns_left += conf.nerve_staple_turns;
        draw_tile(base->x, base->y, 2);
        if (base->faction_id == player_id) {
            parse_says(0, base->name, -1, -1);
            popp(ScriptFile, "NERVESTAPLEDONE", 0, image, 0);
        } else {
            parse_says(0, base->name, -1, -1);
            parse_says(1, get_title(base->faction_id), -1, -1);
            parse_says(2, get_name(base->faction_id), -1, -1);
            NetMsg_pop(NetMsg, "NERVESTAPLETHEM", 5000, 0, 0);
        }
    }
    if (conf.nerve_staple_turns > 0) {
        atrocity(base->faction_id, base->faction_id_former,
            base->faction_id_former != base->faction_id, 1);
    }
}


/*
Energy credits needed to buy `mins` minerals of the current production item.
*/
int base_hurry_quote(int base_id, int mins) {
    if (base_id < 0 || base_id >= *BaseCount) {
        return 0;
    }
    BASE* base = &Bases[base_id];
    int missing = max(0, mineral_cost(base_id, base->item()) - base->minerals_accumulated);
    if (mins < 0 || mins > missing) {
        mins = missing;
    }
    return hurry_cost(base_id, base->item(), mins);
}

/*
Player hurry, as the Thinker Ctrl+H handler and the original BaseWin hurry
dialog: pay hurry_cost for the minerals and mark the base as hurried.
*/
int base_hurry_player(int base_id, int mins) {
    if (base_id < 0 || base_id >= *BaseCount) {
        return HURRY_BAD_ARGS;
    }
    BASE* base = &Bases[base_id];
    Faction* f = &Factions[base->faction_id];
    int missing = max(0, mineral_cost(base_id, base->item()) - base->minerals_accumulated);
    if (mins < 0 || mins > missing) {
        mins = missing;
    }
    int cost = hurry_cost(base_id, base->item(), mins);
    if (cost <= 0 || mins <= 0) {
        return HURRY_NOTHING;
    }
    if (!base->can_hurry_item()) {
        return HURRY_NOT_ALLOWED; // Original: wave_it(8) Cannot execute order
    }
    if (max(0, f->energy_credits - f->hurry_cost_total) < cost) {
        return HURRY_NO_CREDITS; // Original: wave_it(9) Insufficient energy
    }
    f->energy_credits -= cost;
    base->minerals_accumulated += mins;
    base->state_flags |= BSTATE_HURRY_PRODUCTION;
    draw_tile(base->x, base->y, 2);
    return HURRY_DONE;
}

int base_queue_count(int base_id) {
    if (base_id < 0 || base_id >= *BaseCount) {
        return 0;
    }
    return clamp(Bases[base_id].queue_size, 0, 9) + 1;
}

bool base_queue_valid_item(int base_id, int item_id) {
    if (base_id < 0 || base_id >= *BaseCount
    || item_id < -SP_ID_Last || item_id >= MaxProtoNum) {
        return false;
    }
    BASE* base = &Bases[base_id];
    if (item_id >= 0) {
        return veh_avail(item_id, base->faction_id, base_id) != 0;
    }
    return facility_avail((FacilityId)-item_id, base->faction_id, base_id, base->queue_size + 1) != 0;
}

/*
Insert an item at queue slot >= 1 (slot 0 is the item in production and is
changed with mod_base_change). Units in the queue are counted in units_queue
only when they become the current item, like base_queue().
*/
int base_queue_insert(int base_id, int slot, int item_id) {
    if (base_id < 0 || base_id >= *BaseCount) {
        return -1;
    }
    BASE* base = &Bases[base_id];
    int count = base_queue_count(base_id);
    if (count >= 10 || slot < 1 || slot > count
    || item_id < -SP_ID_Last || item_id >= MaxProtoNum) {
        return -1;
    }
    for (int i = count; i > slot; i--) {
        base->queue_items[i] = base->queue_items[i - 1];
    }
    base->queue_items[slot] = item_id;
    base->queue_size = count;
    draw_radius(base->x, base->y, 2, 2);
    return slot;
}

int base_queue_append(int base_id, int item_id) {
    return base_queue_insert(base_id, base_queue_count(base_id), item_id);
}

/*
Remove a queued item (slot >= 1). Slot 0 replaces the current item with the
next queued one through mod_base_change.
*/
int base_queue_remove(int base_id, int slot) {
    if (base_id < 0 || base_id >= *BaseCount) {
        return -1;
    }
    BASE* base = &Bases[base_id];
    int count = base_queue_count(base_id);
    if (slot == 0) {
        // Like picking the next entry in the base window: the change goes
        // through mod_base_change (units_queue, retool penalty).
        if (count <= 1) {
            return -1;
        }
        mod_base_change(base_id, base->queue_items[1]);
        slot = 1;
    }
    if (slot < 1 || slot >= count) {
        return -1;
    }
    for (int i = slot; i < count - 1; i++) {
        base->queue_items[i] = base->queue_items[i + 1];
    }
    base->queue_items[count - 1] = 0;
    base->queue_size = count - 2;
    draw_radius(base->x, base->y, 2, 2);
    return slot;
}

int base_queue_move(int base_id, int from, int to) {
    int count = base_queue_count(base_id);
    if (count <= 0 || from < 1 || to < 1 || from >= count || to >= count) {
        return -1;
    }
    BASE* base = &Bases[base_id];
    int item_id = base->queue_items[from];
    if (from < to) {
        for (int i = from; i < to; i++) {
            base->queue_items[i] = base->queue_items[i + 1];
        }
    } else {
        for (int i = from; i > to; i--) {
            base->queue_items[i] = base->queue_items[i - 1];
        }
    }
    base->queue_items[to] = item_id;
    draw_radius(base->x, base->y, 2, 2);
    return to;
}

void base_queue_clear(int base_id) {
    if (base_id < 0 || base_id >= *BaseCount) {
        return;
    }
    BASE* base = &Bases[base_id];
    for (int i = 1; i < 10; i++) {
        base->queue_items[i] = 0;
    }
    base->queue_size = 0;
    draw_radius(base->x, base->y, 2, 2);
}

} // namespace engine
} // namespace alpha
