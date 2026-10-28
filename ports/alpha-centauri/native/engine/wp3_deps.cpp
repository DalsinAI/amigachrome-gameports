/* SPDX-License-Identifier: GPL-3.0-or-later
 * Weak fallbacks for functions declared in wp3_deps.h. Each is replaced by
 * the owning package's definition as soon as that package is linked in.
 * Until then they keep WP3 code runnable: they raise EV_NOT_IMPLEMENTED
 * (except the AI goal hook, which is silent) and return the neutral value
 * that lets the calling action continue. mod_upgrade_cost and the vanilla
 * part of mod_veh_cost are Thinker Mod src/veh.cpp bodies (dabe62f2, MIT; see
 * engine/THINKER-LICENSE).
 */
#include "engine.h"
#include "wp3_deps.h"

#define WP3_WEAK __attribute__((weak))

namespace alpha {
namespace engine {

WP3_WEAK void __cdecl world_climate() {
    not_implemented("world_climate");
}

WP3_WEAK void __cdecl double_cross(int, int, int) {
    not_implemented("double_cross");
}

WP3_WEAK int __cdecl break_treaty(int, int, uint32_t) {
    not_implemented("break_treaty");
    return 0; // 0 = go ahead with the attack
}

WP3_WEAK int __cdecl act_of_aggression(int, int) {
    not_implemented("act_of_aggression");
    return 0;
}

WP3_WEAK int __cdecl probe(int, int, int, int) {
    not_implemented("probe");
    return 0;
}

WP3_WEAK void __cdecl mon_colony_founded(int, char*) {
    not_implemented("mon_colony_founded");
}

WP3_WEAK void __cdecl make_base_unique(int) {
    not_implemented("make_base_unique");
}

WP3_WEAK int __cdecl tech_is_preq(int, int, int) {
    not_implemented("tech_is_preq");
    return 0;
}

WP3_WEAK void __cdecl add_goal(int, int, int, int, int, int) {
    // AI goal bookkeeping (WP6); nothing to record without the AI.
}

WP3_WEAK int __cdecl mod_veh_cost(int unit_id, int base_id, int32_t* has_proto_cost) {
    // Thinker mod_veh_cost without the prototype surcharge (needs WP5 prototype_factor).
    int cost = Units[unit_id].cost;
    if (base_id >= 0 && unit_id < MaxProtoFactionNum
    && (Units[unit_id].offense_value() < 0 || unit_id == BSC_SPORE_LAUNCHER)
    && has_fac_built(FAC_BROOD_PIT, base_id)) {
        cost = (cost * 3) / 4;
    }
    if (Units[unit_id].plan == PLAN_COLONY && base_id >= 0) {
        cost = clamp(cost, 1, 999);
    }
    if (has_proto_cost) {
        *has_proto_cost = 0;
    }
    return cost;
}

WP3_WEAK int __cdecl mod_upgrade_cost(int faction_id, int new_unit_id, int old_unit_id) {
    // Thinker mod_upgrade_cost, vanilla branch (conf.modify_upgrade_cost = 0).
    UNIT* old_unit = &Units[old_unit_id];
    UNIT* new_unit = &Units[new_unit_id];
    int modifier = new_unit->cost;
    if (new_unit_id >= MaxProtoFactionNum && !new_unit->is_prototyped()) {
        modifier = ((new_unit->cost + 1) / 2 + new_unit->cost)
            * ((new_unit->cost + 1) / 2 + new_unit->cost + 1);
    }
    int cost = max(0, new_unit->speed() - old_unit->speed())
        + max(0, new_unit->armor_cost() - old_unit->armor_cost())
        + max(0, new_unit->weapon_cost() - old_unit->weapon_cost())
        + modifier;
    if (has_project(FAC_NANO_FACTORY, faction_id)) {
        cost /= 2;
    }
    return cost;
}

WP3_WEAK void __cdecl mod_say_morale2(char* output, int veh_id, int) {
    VEH* veh = &Vehs[veh_id];
    int morale = clamp((int)veh->morale, 0, MaxMoraleNum - 1);
    snprintf(output, StrBufLen, "%s",
        veh->is_native_unit() ? Morale[morale].name_lifecycle : Morale[morale].name);
}

WP3_WEAK void __cdecl prefs_save(int) {
}

/* Thinker boom_veh/boom_veh_at/boom_tile draw an explosion (exe boom); the
 * engine only asks the UI to redraw the tile. */
WP3_WEAK void __cdecl boom_veh(int x, int y, int, int) {
    draw_tile(x, y, 2);
}

WP3_WEAK void __cdecl boom_veh_at(int x, int y, int flag) {
    boom_veh(x, y, flag, veh_at(x, y));
}

WP3_WEAK void __cdecl boom_tile(int x, int y, int) {
    draw_tile(x, y, 2);
}

} // namespace engine
} // namespace alpha
