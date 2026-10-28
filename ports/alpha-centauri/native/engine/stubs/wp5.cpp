/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholder entry points for WP5 (research, social engineering, unit design).
 * Written by engine/tools/gen_stubs.py from engine_wp.h. Each function raises
 * EV_NOT_IMPLEMENTED and returns a neutral value. WP5 deletes this file
 * when it lands its real implementation.
 */
#include "../engine.h"

namespace alpha {
namespace engine {

void __cdecl tech_achieved(int UNUSED(faction_id), int UNUSED(tech_id), int UNUSED(faction_id_2), int UNUSED(is_share)) {
    not_implemented("tech_achieved");
}

int __cdecl tech_advance(int UNUSED(faction_id)) {
    not_implemented("tech_advance");
    return 0;
}

int __cdecl tech_avail(int UNUSED(tech_id), int UNUSED(faction_id)) {
    not_implemented("tech_avail");
    return 0;
}

void __cdecl tech_effects(int UNUSED(faction_id)) {
    not_implemented("tech_effects");
}

int __cdecl mod_tech_selection(int UNUSED(faction_id)) {
    not_implemented("mod_tech_selection");
    return 0;
}

int __cdecl mod_tech_pick(int UNUSED(faction_id), int UNUSED(flag), int UNUSED(probe_faction_id), const char* UNUSED(label)) {
    not_implemented("mod_tech_pick");
    return 0;
}

int __cdecl mod_tech_rate(int UNUSED(faction_id)) {
    not_implemented("mod_tech_rate");
    return 0;
}

int __cdecl tech_level(int UNUSED(tech_id), int lvl) {
    not_implemented("tech_level");
    return lvl;
}

int __cdecl social_calc(CSocialCategory* UNUSED(category), CSocialEffect* UNUSED(effect), int UNUSED(faction_id), int UNUSED(toggle), int UNUSED(is_quick_calc)) {
    not_implemented("social_calc");
    return 0;
}

int __cdecl society_avail(int UNUSED(soc_category), int UNUSED(soc_model), int UNUSED(faction_id)) {
    not_implemented("society_avail");
    return 0;
}

void __cdecl make_proto(int UNUSED(unit_id), int UNUSED(chassis_id), int UNUSED(weapon_id), int UNUSED(armor_id), int UNUSED(ability), int UNUSED(reactor_id)) {
    not_implemented("make_proto");
}

int __cdecl propose_proto(int UNUSED(faction_id), int UNUSED(chassis_id), int UNUSED(weapon_id), int UNUSED(armor_id), int UNUSED(abilities), int UNUSED(reactor_id), int UNUSED(plan), char* UNUSED(name)) {
    not_implemented("propose_proto");
    return 0;
}

void __cdecl name_proto(char* UNUSED(name), int UNUSED(unit_id), int UNUSED(faction_id), int UNUSED(chassis_id), int UNUSED(weapon_id), int UNUSED(armor_id), int UNUSED(abilities), int UNUSED(reactor_id)) {
    not_implemented("name_proto");
}

int __cdecl mod_upgrade_prototype(int UNUSED(faction_id), int UNUSED(new_unit_id), int UNUSED(old_unit_id), int UNUSED(flag)) {
    not_implemented("mod_upgrade_prototype");
    return 0;
}

int __cdecl veh_avail(int UNUSED(unit_id), int UNUSED(faction_id), int UNUSED(base_id)) {
    not_implemented("veh_avail");
    return 0;
}

int find_proto(int UNUSED(base_id), TriadFlag UNUSED(triad), VehWeaponMode UNUSED(mode), bool UNUSED(defend)) {
    not_implemented("find_proto");
    return 0;
}

} // namespace engine
} // namespace alpha
