/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP2: base upkeep, production, unit births, growth, riots.
 * Declarations ported from Thinker Mod src/base.h, src/build.h (hurry),
 * src/monument.h and the terranx.exe globals that the base computation uses
 * (src/engine.h), at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 *
 * The entry points listed under WP2 in engine_wp.h are defined in
 * engine/base_*.cpp. This header adds the rest of Thinker's base.h and the
 * new WP2 interfaces (player hurry, production queue editing, base names).
 * Include it after engine.h.
 */
#pragma once

namespace alpha {
namespace engine {

const uint32_t GOV_ALLOW_COMBAT =
    (GOV_MAY_PROD_LAND_COMBAT | GOV_MAY_PROD_NAVAL_COMBAT | GOV_MAY_PROD_AIR_COMBAT);

/* ---- terranx.exe globals of the base computation (engine/base_globals.cpp) ----
 * Scratch values recomputed by base_compute() for *CurrentBaseID; they are not
 * part of the saved game. They live in one BaseComputeState so a test or a UI
 * can snapshot them; the Thinker names are pointers into it. */
struct BaseComputeState {
    int32_t base_upkeep_state;        // BaseUpkeepState
    int32_t base_upkeep_draw_id = -1; // BaseUpkeepDrawID (no base window open)
    int32_t base_upkeep_flag;         // BaseUpkeepFlag
    int32_t control_upkeep_a;         // ControlUpkeepA
    int32_t base_growth_rate;         // BaseGrowthRate
    int32_t base_veh_pacifism_count;  // BaseVehPacifismCount
    int32_t base_forces_supported;    // BaseForcesSupported
    int32_t base_forces_maint_cost;   // BaseForcesMaintCost
    int32_t base_forces_maint_count;  // BaseForcesMaintCount
    int32_t base_terraform_reduce;    // BaseTerraformReduce
    int32_t base_terraform_energy;    // BaseTerraformEnergy
    int32_t base_resource_convoy_to[4];
    int32_t base_resource_convoy_from[4];
    int32_t base_commerce_import[8];
    int32_t base_commerce_export[8];
    int32_t base_psych_talents[5];
    int32_t base_psych_ndrones[5];
    int32_t base_psych_sdrones[5];
    int32_t base_tile_flags[25];
    int32_t skip_tech_screen_b;       // SkipTechScreenB
    int32_t game_victory_type;        // GameVictoryType
    int32_t prototype_flag_a;         // dword_90EA40 (production of units suppressed)
    int32_t prototype_flag_b;         // dword_90EA3C
};
extern BaseComputeState base_compute_state;
void base_compute_state_reset();

extern int* BaseUpkeepState;
extern int* BaseUpkeepDrawID;
extern int* BaseUpkeepFlag;
extern int* ControlUpkeepA;
extern int* BaseGrowthRate;
extern int* BaseVehPacifismCount;
extern int* BaseForcesSupported;
extern int* BaseForcesMaintCost;
extern int* BaseForcesMaintCount;
extern int* BaseTerraformReduce;
extern int* BaseTerraformEnergy;
extern int* BaseResourceConvoyTo;
extern int* BaseResourceConvoyFrom;
extern int* BaseCommerceImport;
extern int* BaseCommerceExport;
extern int* BasePsychTalents;
extern int* BasePsychNDrones;
extern int* BasePsychSDrones;
extern int* BaseTileFlags;
extern int* SkipTechScreenB;
extern int* GameVictoryType;
extern int* dword_90EA40;
extern int* dword_90EA3C;

/* ---- Thinker base.h (functions not already in engine_wp.h or bases.h) ---- */
void __cdecl mod_bases_reset(int region, int faction_id, int defend_only);
int __cdecl base_queue(int base_id);
int __cdecl mineral_factor(int faction_id, int SE_industry);
int __cdecl mod_base_lose_minerals(int base_id, int item_id);
void __cdecl mod_base_check_support();
void __cdecl mod_base_energy_costs();
int __cdecl base_psych_content_pop();
void __cdecl psych_check(int faction_id, int32_t* content_pop, int32_t* base_limit);

char* prod_name(int item_id);
int mineral_cost(int base_id, int item_id);
int hurry_cost(int base_id, int item_id, int hurry_mins);
int unit_support_plan();
int unit_support_cost(int SE_support);
int unit_support_free(int SE_support, int pop_size);
int base_unused_space(int base_id);
int base_growth_goal(int base_id);
int stockpile_energy(int base_id);
int stockpile_energy_active(int base_id);
int terraform_eco_damage(int base_id);
int mineral_output_modifier(int base_id);
int energy_grid_output(int base_id);
int satellite_output(int satellites, int pop_size, bool full_value);
bool satellite_bonus(int base_id, int* nutrient, int* mineral, int* energy);
int __cdecl morale_mod(int base_id, int faction_id, int triad);
int __cdecl breed_mod(int base_id, int faction_id);
int __cdecl worm_mod(int base_id, int faction_id);
int __cdecl own_base_rank(int base_id);
int __cdecl base_rank(int faction_id, int position);
int __cdecl pick_specialist(BASE* base, int econ_val, int labs_val, int psych_val);
int __cdecl best_specialist();
bool can_build(int base_id, int item_id);
bool can_build_unit(int base_id, int unit_id);
bool can_staple(int base_id);
bool base_maybe_riot(int base_id);
bool base_can_riot(int base_id, bool allow_staple);
bool base_pop_boom(int base_id);
bool can_use_teleport(int base_id);
bool can_link_artifact(int base_id);
int __cdecl fac_maint(int facility_id, int faction_id);

/* Thinker build.cpp */
int hurry_item(int base_id, int mins, int cost);

/* OpenSMACX base.cpp name_base / make_base_unique (vanilla base naming). */
void __cdecl name_base(int faction_id, char* name, bool save_offset, bool sea_base);
void __cdecl make_base_unique(int base_id);
/* Base name lists from the faction files (#BASES / #WATERBASES) and
 * basename.txt #GENERIC. Filled by the rules/text loader; empty lists make
 * name_base fall back to "<noun> <n>" like the original. */
struct BaseNameLists {
    std::vector<std::string> land[MaxPlayerNum];
    std::vector<std::string> sea[MaxPlayerNum];
    std::vector<std::string> generic;
};
extern BaseNameLists base_name_lists;

/* Thinker monument.cpp */
void __cdecl monument(int is_new_event);
void __cdecl clear_monuments();
void __cdecl mon_colony_founded(int faction_id, char* base_name);
void __cdecl mon_tech_discovered(int faction_id, int tech_id);
void __cdecl mon_secrets_of_tech(int faction_id);
void __cdecl mon_prototype_built(int faction_id);
void __cdecl mon_facility_built(int faction_id, char* facility_name);
void __cdecl mon_secret_project(int faction_id, int project_id);
void __cdecl mon_enemy_destroyed(int faction_id, int enemy_id);
void __cdecl mon_conquer_base(int faction_id, char* base_name);
void __cdecl mon_naval_unit_built(int faction_id);
void __cdecl mon_air_unit_built(int faction_id);
void __cdecl mon_native_life_bred(int faction_id);
void __cdecl mon_first_in_space(int faction_id);
void __cdecl mon_built_preserve(int faction_id);
void __cdecl mon_winning_unify(int faction_id, int winner_id);
void __cdecl mon_winning_trans(int faction_id, int winner_id);
void __cdecl mon_killed_faction(int faction_id, int killed_id);

/* ---- New WP2 interfaces for the frontend (engine/base_hurry.cpp) ---- */
enum BaseHurryResult {
    HURRY_DONE = 0,          // credits spent, minerals added, BSTATE_HURRY_PRODUCTION set
    HURRY_NOTHING = 1,       // nothing left to buy (cost or minerals zero)
    HURRY_NOT_ALLOWED = 2,   // can_hurry_item() refused (stockpile, already hurried, riots)
    HURRY_NO_CREDITS = 3,    // not enough energy credits
    HURRY_BAD_ARGS = 4,
};
/* Cost in energy credits to buy `mins` minerals of the current item
 * (mins < 0: everything still missing). */
int base_hurry_quote(int base_id, int mins);
/* Player hurry (BaseWin hurry button / Ctrl+H in Thinker gui.cpp). */
int base_hurry_player(int base_id, int mins);

/* Production queue editing. Items use the original encoding: unit ids >= 0,
 * facilities and projects as -facility_id. queue_items[0] is the current
 * item; queue_size counts the extra entries (0..9). Changing slot 0 goes
 * through mod_base_change (retool penalty, units_queue bookkeeping). */
int base_queue_count(int base_id);                     // 1 + queue_size
bool base_queue_valid_item(int base_id, int item_id);  // veh_avail / facility_avail
int base_queue_append(int base_id, int item_id);       // returns slot or -1
int base_queue_insert(int base_id, int slot, int item_id);
int base_queue_remove(int base_id, int slot);          // slot >= 1, or 0 to advance
int base_queue_move(int base_id, int from, int to);    // slots >= 1
void base_queue_clear(int base_id);                    // keep current item only

} // namespace engine
} // namespace alpha
