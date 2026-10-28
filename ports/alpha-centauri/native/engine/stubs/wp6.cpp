/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholder entry points for WP6 (AI).
 * Written by engine/tools/gen_stubs.py from engine_wp.h. Each function raises
 * EV_NOT_IMPLEMENTED and returns a neutral value. WP6 deletes this file
 * when it lands its real implementation.
 */
#include "../engine.h"

namespace alpha {
namespace engine {

void __cdecl mod_enemy_turn(int UNUSED(faction_id)) {
    not_implemented("mod_enemy_turn");
}

int __cdecl mod_enemy_veh(int UNUSED(veh_id)) {
    not_implemented("mod_enemy_veh");
    return 0;
}

int __cdecl mod_enemy_move(int UNUSED(veh_id)) {
    not_implemented("mod_enemy_move");
    return 0;
}

void __cdecl enemy_strategy(int UNUSED(faction_id)) {
    not_implemented("enemy_strategy");
}

void __cdecl enemy_capabilities(int UNUSED(faction_id)) {
    not_implemented("enemy_capabilities");
}

int __cdecl mod_social_ai(int UNUSED(faction_id), int UNUSED(a2), int UNUSED(a3), int UNUSED(a4), int UNUSED(a5), CSocialCategory* UNUSED(a6)) {
    not_implemented("mod_social_ai");
    return 0;
}

int __cdecl mod_tech_val(int UNUSED(tech_id), int UNUSED(faction_id), int UNUSED(flag)) {
    not_implemented("mod_tech_val");
    return 0;
}

int __cdecl mod_tech_ai(int UNUSED(faction_id)) {
    not_implemented("mod_tech_ai");
    return 0;
}

int __cdecl mod_alien_move(int UNUSED(veh_id)) {
    not_implemented("mod_alien_move");
    return 0;
}

void __cdecl alien_fauna() {
    not_implemented("alien_fauna");
}

void design_units(int UNUSED(faction_id)) {
    not_implemented("design_units");
}

int select_build(int UNUSED(base_id)) {
    not_implemented("select_build");
    return 0;
}

int __cdecl veh_kill_lift(int veh_id) {
    not_implemented("veh_kill_lift");
    return veh_lift(veh_id);
}

void __cdecl del_site(int UNUSED(faction_id), int UNUSED(type), int UNUSED(x), int UNUSED(y), int UNUSED(max_dist)) {
    not_implemented("del_site");
}

void __cdecl add_site(int UNUSED(faction_id), int UNUSED(type), int UNUSED(priority), int UNUSED(x), int UNUSED(y)) {
    not_implemented("add_site");
}

} // namespace engine
} // namespace alpha
