/* SPDX-License-Identifier: GPL-3.0-or-later
 * Weak fallbacks for functions declared in wp3_deps.h. Each is replaced by
 * the owning package's definition as soon as that package is linked in.
 * Until then they keep WP3 code runnable: they raise EV_NOT_IMPLEMENTED
 * (except the AI goal hook, which is silent) and return the neutral value
 * that lets the calling action continue. mod_upgrade_cost and the vanilla
 * part of mod_veh_cost are Thinker Mod src/veh.cpp bodies (dabe62f2, MIT; see
 * engine/THINKER-LICENSE).
 * Integration: the fallbacks whose owners have landed were removed:
 * mon_colony_founded, make_base_unique (WP2), tech_is_preq, mod_veh_cost,
 * mod_upgrade_cost (WP5), mod_say_morale2 (WP4); world_climate duplicated
 * the WP2 weak placeholder in base_deps.cpp; double_cross, break_treaty and
 * act_of_aggression are WP8's. add_goal here replaces WP4's duplicate
 * placeholder.
 */
#include "engine.h"
#include "wp3_deps.h"

#define WP3_WEAK __attribute__((weak))

namespace alpha {
namespace engine {

WP3_WEAK int __cdecl probe(int, int, int, int) {
    not_implemented("probe");
    return 0;
}

WP3_WEAK void __cdecl add_goal(int, int, int, int, int, int) {
    // AI goal bookkeeping (WP6); nothing to record without the AI.
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
