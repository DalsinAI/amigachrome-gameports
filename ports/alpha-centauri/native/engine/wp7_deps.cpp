/* SPDX-License-Identifier: GPL-3.0-or-later
 * Weak fallbacks for the functions in wp7_deps.h. Bodies that exist in
 * Thinker Mod (dabe62f2, MIT; see engine/THINKER-LICENSE) are ported from it
 * so WP7 behaves the same before and after the owning package lands;
 * executable-only routines are no-ops documented in ENGINE-WP7.md.
 */
#include "engine.h"
#include "wp7_deps.h"

#define WP7_WEAK __attribute__((weak))

/* Integration: agenda_on, set_agenda and GameVictoryType are WP8's
 * (diplo.cpp); reset_state and clear_goals are WP6's (integration.cpp).
 * Their fallbacks were removed here. */

namespace alpha {
namespace engine {

/* Executable routine: clears the AI's failed-target list. The bunglist is
 * kept as opaque saved bytes (EngineState::bunglist); left untouched. */
WP7_WEAK void __cdecl clear_bunglist(int UNUSED(faction_id)) {
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
