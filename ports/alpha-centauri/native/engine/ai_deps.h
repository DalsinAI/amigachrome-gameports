/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP6 computer players: helpers the AI needs that are not in engine_wp.h.
 * - ai_deps.cpp: private copies of Thinker Mod helpers (src/base.cpp,
 *   src/faction.cpp, src/game.cpp, src/map.cpp, src/net.cpp, src/veh.cpp,
 *   src/veh_action.cpp, src/veh_combat.cpp at
 *   dabe62f2d1abba03de40f3dd3048494843f6a9af; MIT, see engine/THINKER-LICENSE);
 * - ai_exe.cpp: replacements for terranx.exe routines Thinker calls through
 *   function pointers (from OpenSMACX where it has them, else new code).
 * Included by ai.h; everything is in namespace alpha::engine::ai.
 */
#pragma once

namespace alpha {
namespace engine {
namespace ai {

const bool DEF = true;
const bool ATT = false;
const int DEBUG = 0; // Thinker release build: debug-only map overlays are off

const uint32_t GOV_ALLOW_COMBAT =
    (GOV_MAY_PROD_LAND_COMBAT | GOV_MAY_PROD_NAVAL_COMBAT | GOV_MAY_PROD_AIR_COMBAT);

const int ManifoldHarmonicsBonus[][3] = {
    {0,0,0}, // Planet < 0
    {0,0,1}, // Planet = 0
    {1,0,1}, // Planet = 1
    {1,1,1}, // Planet = 2
    {1,1,2}, // Planet > 2
};

struct BItem {
    int item_id;
    int explore;
    int discover;
    int build;
    int conquer;
    int energy;
};

/* WP3 owns these (engine_wp.h mod_* names; Thinker map.h inline aliases). */
inline int goody_at(int x, int y) { return mod_goody_at(x, y); }
inline int bonus_at(int x, int y) { return mod_bonus_at(x, y); }

/* ---- game.cpp ---- */
bool un_charter();
bool victory_done();
bool voice_of_planet();

/* ---- faction.cpp ---- */
bool has_aircraft(int faction_id);
bool has_ships(int faction_id);
bool has_wmode(int faction_id, VehWeaponMode mode);
int __cdecl council_votes(int faction_id);
int __cdecl aah_ooga(int faction_id, int pact_faction_id);
int __cdecl great_beelzebub(int faction_id, int is_aggressive);
int __cdecl climactic_battle();
int __cdecl great_satan(int faction_id, int is_aggressive);
int __cdecl mod_wants_to_attack(int faction_id, int faction_id_tgt, int faction_id_unk);

/* ---- base.cpp ---- */
bool base_can_riot(int base_id, bool allow_staple);
int base_unused_space(int base_id);
int base_growth_goal(int base_id);
bool can_build(int base_id, int item_id);
bool can_build_unit(int base_id, int unit_id);
bool can_link_artifact(int base_id);
bool can_staple(int base_id);
bool can_use_teleport(int base_id);
int unit_support_plan();
char* prod_name(int item_id);
void __cdecl psych_check(int faction_id, int32_t* content_pop, int32_t* base_limit);
int __cdecl mineral_factor(int faction_id, int SE_industry);
int mineral_output_modifier(int base_id);
void __cdecl mod_bases_reset(int region, int faction_id, int defend_only);

/* ---- veh.cpp ---- */
VehArmor best_armor(int faction_id, int max_cost);
VehWeapon best_weapon(int faction_id);
VehReactor best_reactor(int faction_id);
int proto_offense(int unit_id);
int proto_defense(int unit_id);
int __cdecl prototype_factor(int unit_id);
int __cdecl mod_upgrade_cost(int faction_id, int new_unit_id, int old_unit_id);
void __cdecl part_upgrade(int faction_id, int new_unit_id, int old_unit_id);
void __cdecl full_upgrade(int faction_id, int new_unit_id, int old_unit_id);
int __cdecl create_proto(int faction_id, VehChassis chs, VehWeapon wpn, VehArmor arm,
    VehAblFlag abls, VehReactor rec, VehPlan ai_plan);
int __cdecl mod_veh_skip(int veh_id);
int set_move_to(int veh_id, int x, int y);
int set_move_next(int veh_id, int offset);
int set_road_to(int veh_id, int x, int y);
int set_action(int veh_id, int act, char icon);
int set_order_none(int veh_id);
int set_convoy(int veh_id, ResType res);
int set_board_to(int veh_id, int trans_veh_id);

/* ---- map.cpp ---- */
int bonus_yield(int res_type);
int fungus_yield(int faction_id, ResType res_type);
int total_yield(int x, int y, int faction_id);
int item_yield(int x, int y, int faction_id, int bonus, MapItem item);

/* ---- veh_action.cpp ---- */
int __cdecl terrain_avail(FormerItem frm_id, int ocean, int faction_id);
inline bool has_terra(FormerItem frm_id, int ocean, int faction_id) {
    return terrain_avail(frm_id, ocean, faction_id);
}

/* ---- veh_combat.cpp ---- */
bool use_nerve_gas(int faction_id_atk, int faction_id_def);
bool use_nerve_gas(int faction_id_atk);
int __cdecl mod_battle_fight(int veh_id, int offset, int table_offset, int option, int* def_id);

/* ---- net.cpp (single player only) ---- */
int __cdecl net_action_build(int veh_id, const char* name);
int __cdecl net_action_gate(int veh_id, int base_id);
int __cdecl net_action_destroy(int veh_id, int flag, int x, int y);

/* Fix: probe team actions (Thinker probe.cpp, deferred in the plan) are not
 * ported yet. combat_move skips its subvert branch while this is false and the
 * placeholder below is never reached. */
const bool ProbeActionsPorted = false;
inline int probe(int UNUSED(veh_id), int UNUSED(base_id), int UNUSED(veh_id_tgt), int UNUSED(flag)) {
    return 0;
}

/* ---- ai_exe.cpp: terranx.exe routines ---- */
int __cdecl garrison_check(int base_id);
int __cdecl base_territory(int faction_id, int x, int y);
int __cdecl planet_buster(int veh_id);
int __cdecl abstract_at(int x, int y);
int __cdecl world_site(int x, int y, int toggle);
void __cdecl retire_proto(int unit_id, int faction_id);
void __cdecl set_course(int veh_id, int type, int x, int y);
void Path_territory(int x, int y, int region, int faction_id);
bool territory_route(int x1, int y1, int x2, int y2, bool any_land);
void draw_tiles(int x, int y, int flag);
int __cdecl base_on_sea(int base_id, int region_sea);
int __cdecl POP2(const char* label, const char* pcx_filename, int a3);
int MapWin_focus(MapWindow* map_win, int x, int y);
int Console_focus(Console* console, int x, int y, int faction_id);

/* ---- ai_entry.cpp: work counters (struct AIStats in ai_api.h) ---- */
extern AIStats stats;

/* Every AI route query goes through this counter to engine Path_find (WP3). */
inline int Path_find(int x1, int y1, int x2, int y2, int unit_id, int faction_id, int flags, int unk) {
    stats.path_find_calls++;
    return alpha::engine::Path_find(x1, y1, x2, y2, unit_id, faction_id, flags, unk);
}

} // namespace ai
} // namespace engine
} // namespace alpha
