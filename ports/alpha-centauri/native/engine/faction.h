/* SPDX-License-Identifier: GPL-3.0-or-later
 * Faction accessors (Thinker Mod src/faction.h subset, MIT; see
 * engine/THINKER-LICENSE). Diplomacy actions (treaty_on/off, atrocity, ...)
 * belong to WP8 and social engineering to WP5 (see engine_wp.h).
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

bool valid_player(int faction_id);
bool valid_triad(int triad);
int __cdecl has_tech(int tech_id, int faction_id);
bool has_chassis(int faction_id, VehChassis chs);
bool has_weapon(int faction_id, VehWeapon wpn);
bool has_project(FacilityId item_id);
bool has_project(FacilityId item_id, int faction_id);
int project_base(FacilityId item_id);
int facility_count(FacilityId item_id, int faction_id);
int prod_count(int item_id, int faction_id, int base_skip_id);
bool is_human(int faction_id);
bool is_alien(int faction_id);
bool is_alive(int faction_id);
void set_human(int faction_id, bool active);
void set_alive(int faction_id, bool active);
bool thinker_enabled(int faction_id);
bool thinker_move_upkeep(int faction_id);
bool at_war(int faction1, int faction2);
bool has_pact(int faction1, int faction2);
bool both_neutral(int faction1, int faction2);
bool both_non_enemy(int faction1, int faction2);
bool want_revenge(int faction1, int faction2);
bool allow_expand(int faction_id);
bool has_plr_rule(int faction_id, FactionRuleType rule, int* val1, int* val2);
bool has_active_veh(int faction_id, VehPlan plan);
int veh_count(int faction_id, int unit_id);
int find_hq(int faction_id);
int __cdecl has_treaty(int faction_id_1, int faction_id_2, uint32_t status);
int __cdecl has_agenda(int faction_id_1, int faction_id_2, uint32_t status);
char* __cdecl get_adjective(int faction_id);
char* __cdecl get_noun(int faction_id);
char* __cdecl get_name(int faction_id);
char* __cdecl get_title(int faction_id);
char* __cdecl get_adjective_leader(int faction_id);
char* __cdecl get_adjective_insult_leader(int faction_id);
char* __cdecl get_adjective_faction(int faction_id);
char* __cdecl get_adjective_insult_faction(int faction_id);
char* __cdecl get_insult_leader(int faction_id);

} // namespace engine
} // namespace alpha
