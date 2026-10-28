/* SPDX-License-Identifier: GPL-3.0-or-later
 * Entry points owned by the parallel work packages (WP1..WP8).
 * Names and signatures are Thinker Mod's (src/ headers at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af, MIT; see engine/THINKER-LICENSE)
 * or, for routines that only exist in terranx.exe, the signature of Thinker's
 * function-pointer typedef in src/engine.h (BOOL -> int).
 *
 * Until a package lands, engine/stubs/wpN.cpp defines its functions as
 * placeholders that raise EV_NOT_IMPLEMENTED (see events.h) and return a
 * neutral value. A package implements these functions in its own
 * engine/<topic>.cpp files and deletes its stub file in the same commit; the
 * build compiles every .cpp file in engine/ and engine/stubs/, so no build edit is needed.
 * Packages add further declarations in their own headers, not here, to keep
 * merges conflict-free.
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

/* ---- WP1: turn scheduler and End Turn (engine/turn.cpp) ---- */
void __cdecl control_turn();
void __cdecl turn_upkeep();
void __cdecl faction_upkeep(int faction_id);
void __cdecl production_phase(int faction_id);
void __cdecl repair_phase(int faction_id);
void __cdecl allocate_energy(int faction_id);
int __cdecl next_player_turn();
void __cdecl social_upkeep(int faction_id);
int __cdecl social_upheaval(int faction_id, CSocialCategory* choices);
void __cdecl tech_research(int faction_id, int value);
void __cdecl rankings(int flag);
int __cdecl game_year(int turn);
void __cdecl do_fungal_towers();
void __cdecl reset_territory(); // exe-only, new code
void __cdecl mod_time_warp();
void __cdecl mod_auto_save();

/* ---- WP2: base upkeep, production, growth, riots (engine/base_*.cpp) ---- */
int __cdecl mod_base_upkeep(int base_id);
int __cdecl mod_base_production();
int __cdecl mod_base_build(int base_id, int has_gov);
void __cdecl base_first(int base_id);
void __cdecl base_compute(int update_prev);
void __cdecl mod_base_change(int base_id, int item_id);
int __cdecl mod_base_making(int item_id, int base_id);
int __cdecl mod_base_hurry();
int __cdecl mod_cost_factor(int faction_id, BaseResType type, int base_id);
void __cdecl mod_base_support();
void __cdecl mod_base_yield();
void __cdecl mod_base_nutrient();
void __cdecl mod_base_minerals();
void __cdecl mod_base_energy();
void __cdecl mod_base_psych(int base_id, int SE_talent, int SE_police);
int __cdecl mod_base_growth();
void __cdecl mod_drone_riot();
void __cdecl mod_base_drones();
void __cdecl mod_base_maint();
void __cdecl mod_base_ecology();
void __cdecl mod_base_research();
void __cdecl mod_name_base(int faction_id, char* name, bool save_offset, bool water);
void __cdecl mod_capture_base(int base_id, int faction_id, int is_probe);
void __cdecl mod_base_reset(int base_id, int has_gov);
int __cdecl resource_yield(BaseResType type, int faction_id, int base_id, int x, int y);
int __cdecl mod_crop_yield(int faction_id, int base_id, int x, int y, int flag);
int __cdecl mod_mine_yield(int faction_id, int base_id, int x, int y, int flag);
int __cdecl mod_energy_yield(int faction_id, int base_id, int x, int y, int flag);
int __cdecl mod_black_market(int base_id, int energy);
int __cdecl facility_avail(FacilityId item_id, int faction_id, int base_id, int queue_count);
void find_relocate_base(int faction_id); // static in Thinker base.cpp
void __cdecl action_staple(int base_id); // basewin.cpp; called by WP6 (select_build)

/* ---- WP3: unit orders, movement, terraforming (engine/veh_action.cpp, engine/path.cpp) ---- */
int __cdecl order_veh(int veh_id, int offset, int flag);
int __cdecl veh_speed(int veh_id, int skip_morale);
int __cdecl proto_speed(int unit_id);
int __cdecl mod_hex_cost(int unit_id, int faction_id, int x1, int y1, int x2, int y2, int toggle);
int __cdecl terraform_cost(int x, int y, int faction_id);
int __cdecl zoc_any(int x, int y, int faction_id);
int __cdecl zoc_move(int x, int y, int faction_id);
int __cdecl zoc_veh(int x, int y, int faction_id);
int __cdecl zoc_sea(int x, int y, int faction_id);
int __cdecl goody_box(int veh_id);
int __cdecl mod_goody_at(int x, int y);
int __cdecl mod_bonus_at(int x, int y);
int __cdecl mod_minerals_at(int x, int y);
void __cdecl monolith(int veh_id);
int __cdecl want_monolith(int veh_id);
void __cdecl spot_all(int veh_id, int flag);
void __cdecl spot_base(int base_id, int faction_id);
void __cdecl spot_stack(int veh_id, int faction_id);
void __cdecl unspot_stack(int veh_id);
void __cdecl spot_loc(int x, int y, int faction_id);
int __cdecl veh_wake(int veh_id);
void __cdecl veh_skip(int veh_id);
void __cdecl wake_stack(int veh_id);
void __cdecl alt_set(int x, int y, int altitude);
void __cdecl alt_set_both(int x, int y, int altitude);
void __cdecl world_alt_set(int x, int y, int altitude, int toggle);
void __cdecl world_raise_alt(int x, int y);
void __cdecl world_lower_alt(int x, int y);
int __cdecl action_terraform(int veh_id, int item_id, int toggle);
int __cdecl action_airdrop(int veh_id, int tx, int ty, int flags);
void __cdecl action_destroy(int veh_id, int tgt_item, int tgt_x, int tgt_y);
int __cdecl contribution(int veh_id, int frm_id);
int __cdecl find_return_base(int veh_id);
/* Also called by WP6 (computer players); signatures as in WP3 veh_action.h. */
int __cdecl action(int veh_id);
int __cdecl action_build(int veh_id, const char* name);
void __cdecl action_gate(int veh_id, int base_id);
int __cdecl study_artifact(int veh_id);
int __cdecl Path_find(int x1, int y1, int x2, int y2, int unit_id, int faction_id, int flags, int unk);
int __cdecl Path_move(int veh_id, int faction_id);

/* ---- WP4: combat (engine/combat.cpp) ---- */
void __cdecl mod_battle_compute(int veh_id_atk, int veh_id_def, int* offense_out, int* defense_out, int combat_type);
int __cdecl mod_battle_fight_2(int veh_id, int offset, int tx, int ty, int table_offset, int option, int* def_id);
int __cdecl mod_best_defender(int veh_id_def, int veh_id_atk, int combat_type);
int __cdecl psi_factor(int value, int faction_id, int is_attack, int is_fungal_twr);
void __cdecl promote(int veh_id);
int __cdecl interceptor(int faction_id_def, int faction_id_atk, int tx, int ty);
int __cdecl battle_kill(int veh_id, int* num_killed, int* num_relics, int* num_credits, int veh_id_atk, int faction_id_atk);
void __cdecl battle_kill_stack(int veh_id, int* num_killed, int* num_relics, int* num_credits, int veh_id_atk, int faction_id_atk);
void __cdecl planet_busting(int veh_id, int tx, int ty);
int __cdecl mod_morale_alien(int veh_id, int faction_id_vs_native);
int __cdecl morale_veh(int veh_id, int check_drone_riot, int faction_id_vs_native);
int __cdecl get_basic_offense(int veh_id_atk, int veh_id_def, int psi_combat_type, int is_bombard, int unk_tgl);
int __cdecl get_basic_defense(int veh_id_def, int veh_id_atk, int psi_combat_type, int is_bombard);

/* ---- WP5: research, social engineering, unit design (engine/tech.cpp, engine/design.cpp) ---- */
void __cdecl tech_achieved(int faction_id, int tech_id, int faction_id_2, int is_share);
int __cdecl tech_advance(int faction_id);
int __cdecl tech_avail(int tech_id, int faction_id);
void __cdecl tech_effects(int faction_id);
int __cdecl mod_tech_selection(int faction_id);
int __cdecl mod_tech_pick(int faction_id, int flag, int probe_faction_id, const char* label);
int __cdecl mod_tech_rate(int faction_id);
int __cdecl tech_level(int tech_id, int lvl);
int __cdecl social_calc(CSocialCategory* category, CSocialEffect* effect, int faction_id, int toggle, int is_quick_calc);
int __cdecl society_avail(int soc_category, int soc_model, int faction_id);
void __cdecl make_proto(int unit_id, int chassis_id, int weapon_id, int armor_id, int ability, int reactor_id);
int __cdecl propose_proto(int faction_id, int chassis_id, int weapon_id, int armor_id, int abilities,
    int reactor_id, int plan, char* name);
void __cdecl name_proto(char* name, int unit_id, int faction_id, int chassis_id, int weapon_id,
    int armor_id, int abilities, int reactor_id);
int __cdecl mod_upgrade_prototype(int faction_id, int new_unit_id, int old_unit_id, int flag);
int __cdecl veh_avail(int unit_id, int faction_id, int base_id);
int find_proto(int base_id, TriadFlag triad, VehWeaponMode mode, bool defend);

/* ---- WP6: AI (engine/ai_*.cpp) ---- */
void __cdecl mod_enemy_turn(int faction_id);
int __cdecl mod_enemy_veh(int veh_id);
int __cdecl mod_enemy_move(int veh_id);
void __cdecl enemy_strategy(int faction_id);
void __cdecl enemy_capabilities(int faction_id);
int __cdecl mod_social_ai(int faction_id, int a2, int a3, int a4, int a5, CSocialCategory* a6);
int __cdecl mod_tech_val(int tech_id, int faction_id, int flag);
int __cdecl mod_tech_ai(int faction_id);
int __cdecl mod_alien_move(int veh_id);
void __cdecl alien_fauna();
void design_units(int faction_id);
int select_build(int base_id);
int __cdecl veh_kill_lift(int veh_id); // AI map bookkeeping, then veh_lift
void __cdecl del_site(int faction_id, int type, int x, int y, int max_dist);
void __cdecl add_site(int faction_id, int type, int priority, int x, int y);

/* ---- WP7: new game and world generation (engine/mapgen.cpp, engine/setup.cpp) ---- */
void world_generate(uint32_t seed);
void __cdecl mod_world_build();
void __cdecl find_start(int faction_id, int* tx, int* ty);
int __cdecl mod_setup_player(int faction_id, int setup_id, int is_probe);
void __cdecl setup_game(int flag);
void init_world_config();
void __cdecl fixup_landmarks(); // exe-only, called by map_read
void __cdecl world_linearize_contours();

/* ---- WP8: diplomacy, council, victory, score (engine/diplo.cpp, engine/victory.cpp) ---- */
void __cdecl treaty_on(int faction_id_1, int faction_id_2, uint32_t status);
void __cdecl treaty_off(int faction_id_1, int faction_id_2, uint32_t status);
void __cdecl set_treaty(int faction_id_1, int faction_id_2, uint32_t status, bool add);
void __cdecl atrocity(int faction_id, int faction_id_tgt, int skip_init_check, int skip_human_check);
int __cdecl get_mood(int friction);
void __cdecl encounter(int faction_id_1, int faction_id_2, int x, int y, int fungal_tower, int native_unit);
void __cdecl enemy_diplomacy(int faction_id);
void __cdecl diplomacy_check(int faction_id, int faction_id_2, int full_check);
int __cdecl end_of_game(int flag);
int __cdecl mod_eliminate_player(int faction_id, int setup_id);
int __cdecl transcending(int faction_id);
int __cdecl corner_market(int faction_id);
int __cdecl is_objective(int base_id);

} // namespace engine
} // namespace alpha
