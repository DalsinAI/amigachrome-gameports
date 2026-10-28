/* SPDX-License-Identifier: GPL-3.0-or-later
 * Weak fallbacks for the functions in wp7_deps.h. Bodies that exist in
 * Thinker Mod (dabe62f2, MIT; see engine/THINKER-LICENSE) are ported from it
 * so WP7 behaves the same before and after the owning package lands;
 * executable-only routines are no-ops documented in ENGINE-WP7.md.
 */
#include "engine.h"
#include "wp7_deps.h"

#define WP7_WEAK __attribute__((weak))

namespace alpha {
namespace engine {

static int game_victory_type = 0;
WP7_WEAK int* GameVictoryType = &game_victory_type;

/* Thinker plan.cpp reset_state invalidates WP6 plan caches; nothing to do
 * without WP6. */
WP7_WEAK void reset_state() {
}

/* Thinker goal.cpp */
WP7_WEAK void __cdecl clear_goals(int faction_id) {
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction_id].goals[i];
        goal.type = -1;
        goal.priority = 0;
        goal.x = 0;
        goal.y = 0;
        goal.base_id = 0;
    }
    for (int i = 0; i < MaxSitesNum; i++) {
        Goal& site = Factions[faction_id].sites[i];
        site.type = -1;
        site.priority = 0;
        site.x = 0;
        site.y = 0;
        site.base_id = 0;
    }
}

/* Executable routine: clears the AI's failed-target list. The bunglist is
 * kept as opaque saved bytes (EngineState::bunglist); left untouched. */
WP7_WEAK void __cdecl clear_bunglist(int UNUSED(faction_id)) {
}

/* Executable routines: Faction::diplo_agenda bit updates. */
WP7_WEAK void __cdecl agenda_on(int faction_id_1, int faction_id_2, uint32_t status) {
    Factions[faction_id_1].diplo_agenda[faction_id_2] |= status;
}

WP7_WEAK void __cdecl set_agenda(int faction_id_1, int faction_id_2, uint32_t status, int add) {
    if (add) {
        Factions[faction_id_1].diplo_agenda[faction_id_2] |= status;
    } else {
        Factions[faction_id_1].diplo_agenda[faction_id_2] &= ~status;
    }
}

/* Thinker move.cpp (non_ally_in_tile without its cache). */
WP7_WEAK bool can_build_base(int x, int y, int faction_id, int triad) {
    MAP* sq;
    if (!(sq = mapsq(x, y)) || *BaseCount >= MaxBaseNum) {
        return false;
    }
    if (!sq->allow_base() || sq->items & BIT_THERMAL_BORE) {
        return false;
    }
    int limit = clamp(*MapAreaTiles / 512, 1, 3);
    if (y < limit || y >= *MapAreaY - limit) {
        return false;
    }
    if (map_is_flat() && (x < 2 || x >= *MapAreaX - 2)) {
        return false;
    }
    if (sq->owner >= 0 && sq->owner != faction_id
    && Factions[faction_id].base_count > 0
    && !has_treaty(faction_id, sq->owner, DIPLO_VENDETTA)
    && has_treaty(faction_id, sq->owner, DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        return false;
    }
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->x == x && veh->y == y && veh->faction_id != faction_id
        && !has_pact(faction_id, veh->faction_id)) {
            return false;
        }
    }
    int range = conf.base_spacing - (conf.base_nearby_limit < 0 ? 1 : 0);
    int num = 0;
    int i = 0;
    for (auto& m : iterate_tiles(x, y, 0, TableRange[range])) {
        if (m.sq->is_base()) {
            if (conf.base_nearby_limit < 0 || i < TableRange[range - 1]
            || ++num > conf.base_nearby_limit) {
                return false;
            }
        }
        i++;
    }
    int alt = sq->alt_level();
    if (triad != TRIAD_SEA && alt >= ALT_SHORE_LINE) {
        return true;
    }
    if ((triad == TRIAD_SEA || triad == TRIAD_AIR)
    && (alt == ALT_OCEAN || alt == ALT_OCEAN_SHELF)) {
        return true;
    }
    return false;
}

/* Executable routine (difficulty start balancing). Thinker skips it unless
 * skip_default_balance = 0; the body is in no upstream source. */
WP7_WEAK void __cdecl balance() {
    not_implemented("balance");
}

} // namespace engine
} // namespace alpha
