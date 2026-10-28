/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholders for functions that WP4 (combat) calls but that belong to other
 * packages and are not declared in engine_wp.h (declarations: combat.h).
 * Each raises EV_NOT_IMPLEMENTED and returns a neutral value. The owning
 * package deletes the matching function here when it defines the real one:
 *   WP8: cause_friction, major_atrocity
 * Integration: mon_enemy_destroyed is WP2's (base_monument.cpp); un_charter
 * and world_climate were duplicates of WP2's weak definitions in
 * base_deps.cpp (un_charter there is Thinker's body) and are removed here;
 * break_treaty, act_of_aggression (WP8) and add_goal (WP6) duplicated WP3's
 * weak placeholders in wp3_deps.cpp, which remain until the owners land.
 * Thinker Mod signatures (dabe62f2, MIT; see engine/THINKER-LICENSE).
 */
#include "../combat.h"

namespace alpha {
namespace engine {

void __cdecl cause_friction(int UNUSED(faction_id), int UNUSED(faction_id_with), int UNUSED(friction)) {
    not_implemented("cause_friction");
}

void __cdecl major_atrocity(int UNUSED(faction_id), int UNUSED(faction_id_tgt)) {
    not_implemented("major_atrocity");
}

} // namespace engine
} // namespace alpha
