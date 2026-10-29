/* SPDX-License-Identifier: GPL-3.0-or-later
 * Functions WP3 calls that other packages own but that are not declared in
 * engine_wp.h. Signatures are Thinker Mod's (dabe62f2, MIT; see
 * engine/THINKER-LICENSE). wp3_deps.cpp defines each one as a WEAK fallback,
 * so the owning package's ordinary definition replaces it at link time
 * without any edit here (see ENGINE-WP3.md for the list and the fallback
 * behaviour). Include engine.h first.
 */
#pragma once

namespace alpha {
namespace engine {

void __cdecl world_climate();                                                   // WP7 mapgen
void __cdecl double_cross(int faction_id_atk, int faction_id_def, int faction_id_other); // WP8
int __cdecl break_treaty(int faction_id, int faction_id_tgt, uint32_t status);  // WP8
int __cdecl act_of_aggression(int faction_id_atk, int faction_id_def);          // WP8
int __cdecl probe(int veh_id, int tgt_base_id, int tgt_veh_id, int toggle);     // probe teams
void __cdecl mon_colony_founded(int faction_id, char* base_name);              // WP2 monument
void __cdecl make_base_unique(int base_id);                                    // WP2 base names
int __cdecl tech_is_preq(int preq_tech_id, int parent_tech_id, int range);      // WP5
void __cdecl add_goal(int faction_id, int type, int priority, int x, int y, int base_id); // WP6
int __cdecl mod_veh_cost(int unit_id, int base_id, int32_t* has_proto_cost);    // WP5
int __cdecl mod_upgrade_cost(int faction_id, int new_unit_id, int old_unit_id); // WP5
void __cdecl mod_say_morale2(char* output, int veh_id, int faction_id_vs_native); // WP4
void __cdecl prefs_save(int save_factions);                                     // UI/config
void __cdecl boom_veh(int x, int y, int flag, int veh_id);                      // WP4 (explosion)
void __cdecl boom_veh_at(int x, int y, int flag);                               // WP4
void __cdecl boom_tile(int x, int y, int flag);                                 // WP4

} // namespace engine
} // namespace alpha
