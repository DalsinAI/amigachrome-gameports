/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholder entry points for WP4 (combat).
 * Written by engine/tools/gen_stubs.py from engine_wp.h. Each function raises
 * EV_NOT_IMPLEMENTED and returns a neutral value. WP4 deletes this file
 * when it lands its real implementation.
 */
#include "../engine.h"

namespace alpha {
namespace engine {

void __cdecl mod_battle_compute(int UNUSED(veh_id_atk), int UNUSED(veh_id_def), int* UNUSED(offense_out), int* UNUSED(defense_out), int UNUSED(combat_type)) {
    not_implemented("mod_battle_compute");
}

int __cdecl mod_battle_fight_2(int UNUSED(veh_id), int UNUSED(offset), int UNUSED(tx), int UNUSED(ty), int UNUSED(table_offset), int UNUSED(option), int* UNUSED(def_id)) {
    not_implemented("mod_battle_fight_2");
    return 0;
}

int __cdecl mod_best_defender(int UNUSED(veh_id_def), int UNUSED(veh_id_atk), int UNUSED(combat_type)) {
    not_implemented("mod_best_defender");
    return 0;
}

int __cdecl psi_factor(int value, int UNUSED(faction_id), int UNUSED(is_attack), int UNUSED(is_fungal_twr)) {
    not_implemented("psi_factor");
    return value;
}

void __cdecl promote(int UNUSED(veh_id)) {
    not_implemented("promote");
}

int __cdecl interceptor(int UNUSED(faction_id_def), int UNUSED(faction_id_atk), int UNUSED(tx), int UNUSED(ty)) {
    not_implemented("interceptor");
    return 0;
}

int __cdecl battle_kill(int UNUSED(veh_id), int* UNUSED(num_killed), int* UNUSED(num_relics), int* UNUSED(num_credits), int UNUSED(veh_id_atk), int UNUSED(faction_id_atk)) {
    not_implemented("battle_kill");
    return 0;
}

void __cdecl battle_kill_stack(int UNUSED(veh_id), int* UNUSED(num_killed), int* UNUSED(num_relics), int* UNUSED(num_credits), int UNUSED(veh_id_atk), int UNUSED(faction_id_atk)) {
    not_implemented("battle_kill_stack");
}

void __cdecl planet_busting(int UNUSED(veh_id), int UNUSED(tx), int UNUSED(ty)) {
    not_implemented("planet_busting");
}

int __cdecl mod_morale_alien(int UNUSED(veh_id), int UNUSED(faction_id_vs_native)) {
    not_implemented("mod_morale_alien");
    return 0;
}

int __cdecl morale_veh(int UNUSED(veh_id), int UNUSED(check_drone_riot), int UNUSED(faction_id_vs_native)) {
    not_implemented("morale_veh");
    return 0;
}

int __cdecl get_basic_offense(int UNUSED(veh_id_atk), int UNUSED(veh_id_def), int UNUSED(psi_combat_type), int UNUSED(is_bombard), int UNUSED(unk_tgl)) {
    not_implemented("get_basic_offense");
    return 0;
}

int __cdecl get_basic_defense(int UNUSED(veh_id_def), int UNUSED(veh_id_atk), int UNUSED(psi_combat_type), int UNUSED(is_bombard)) {
    not_implemented("get_basic_defense");
    return 0;
}

} // namespace engine
} // namespace alpha
