/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholders for functions that WP4 (combat) calls but that belong to other
 * packages and are not declared in engine_wp.h (declarations: combat.h).
 * Each raises EV_NOT_IMPLEMENTED and returns a neutral value. The owning
 * package deletes the matching function here when it defines the real one:
 *   WP8: break_treaty, act_of_aggression, cause_friction, major_atrocity
 *   WP6: add_goal
 * Integration: mon_enemy_destroyed is WP2's (base_monument.cpp); un_charter
 * and world_climate were duplicates of WP2's weak definitions in
 * base_deps.cpp (un_charter there is Thinker's body) and are removed here.
 * Thinker Mod signatures (dabe62f2, MIT; see engine/THINKER-LICENSE).
 */
#include "../combat.h"

namespace alpha {
namespace engine {

int __cdecl break_treaty(int UNUSED(faction_id), int UNUSED(faction_id_tgt), uint32_t UNUSED(status)) {
    not_implemented("break_treaty");
    return 0; // go ahead with the attack
}

int __cdecl act_of_aggression(int UNUSED(faction_id_atk), int UNUSED(faction_id_def)) {
    not_implemented("act_of_aggression");
    return 0;
}

void __cdecl cause_friction(int UNUSED(faction_id), int UNUSED(faction_id_with), int UNUSED(friction)) {
    not_implemented("cause_friction");
}

void __cdecl major_atrocity(int UNUSED(faction_id), int UNUSED(faction_id_tgt)) {
    not_implemented("major_atrocity");
}

void __cdecl add_goal(int UNUSED(faction_id), int UNUSED(type), int UNUSED(priority),
int UNUSED(x), int UNUSED(y), int UNUSED(base_id)) {
    not_implemented("add_goal");
}

} // namespace engine
} // namespace alpha
