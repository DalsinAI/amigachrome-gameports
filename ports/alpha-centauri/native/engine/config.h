/* SPDX-License-Identifier: GPL-3.0-or-later
 * Config, LMConfig and AIPlans ported verbatim from Thinker Mod src/main.h at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE).
 * Defaults are Thinker's thinker.ini-less defaults. config_vanilla() pins the
 * settings documented as "original default" in Thinker docs/thinker.ini.
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

enum VideoMode {
    VM_Native = 0,
    VM_Custom = 1,
    VM_Window = 2,
};

struct LMConfig {
    int crater = 1;
    int volcano = 1;
    int jungle = 1;
    int uranium = 1;
    int sargasso = 1;
    int ruins = 1;
    int dunes = 1;
    int fresh = 1;
    int mesa = 1;
    int canyon = 0;
    int geothermal = 1;
    int ridge = 1;
    int borehole = 1;
    int nexus = 1;
    int unity = 1;
    int fossil = 1;
};

struct Config {
    int video_mode = VM_Native;
    int window_width = 1024;
    int window_height = 768;
    int minimised = 0; // internal variable
    int video_player = 2; // internal variable
    int playing_movie = 0;  // internal variable
    int screen_width = 1024; // internal variable
    int screen_height = 768; // internal variable
    int directdraw = 0;
    int disable_opening_movie = -1;
    int smac_only = 0;
    int smooth_scrolling = 0;
    int scroll_area = 40;
    int auto_minimise = 0;
    int render_base_info = 1;
    int render_high_detail = 1; // unlisted option
    int editor_free_units = 1; // unlisted option
    int autosave_interval = 1;
    int warn_on_former_replace = 1;
    int manage_player_bases = 0;
    int manage_player_units = 0;
    int render_probe_labels = 1;
    int foreign_treaty_popup = 0;
    int game_event_popup = 0;
    int new_base_names = 1;
    int new_unit_names = 1;
    int design_units = 1;
    int factions_enabled = 7;
    int social_ai = 1;
    int social_ai_bias = 10;
    int tech_balance = 0;
    int base_hurry = 0;
    int base_spacing = 3;
    int base_nearby_limit = -1;
    int expansion_limit = 100;
    int expansion_autoscale = 0;
    int limit_project_start = 0;
    int rival_friction_bias = 0;
    int rival_defiance_bias = 0;
    int max_satellites = 20;
    int new_world_builder = 1;
    int world_sea_levels[3] = {46,58,70};
    int world_hills_mod = 40;
    int world_ocean_mod = 40;
    int world_islands_mod = 16;
    int world_continents = 0;
    int world_polar_caps = 1;
    int world_mirror_x = 0;
    int world_mirror_y = 0;
    int modified_landmarks = 0;
    int time_warp_mod = 1;
    int time_warp_techs = 5;
    int time_warp_projects = 1;
    int time_warp_start_turn = 40;
    int spawn_free_units[9] = {0,0,1,0,0,1,1,0,1};
    int player_colony_pods = 0;
    int computer_colony_pods = 0;
    int player_formers = 0;
    int computer_formers = 0;
    int player_satellites[3] = {0,0,0};
    int computer_satellites[3] = {0,0,0};
    int faction_placement = 1;
    int nutrient_bonus = 0;
    int rare_supply_pods = 0;
    int simple_cost_factor = 0;
    int revised_tech_cost = 1;
    int tech_rate_modifier = 100; // internal variable
    int tech_stagnate_rate = 200;
    int fast_fungus_movement = 0;
    int magtube_movement_rate = 0;
    int road_movement_rate = 1; // internal variable
    int max_movement_rate = 255; // internal variable
    int chopper_attack_rate = 1;
    int base_event_turns = 10;
    int base_psych = 1;
    int nerve_staple_turns = 10;
    int nerve_staple_mod = -10;
    int delay_drone_riots = 0;
    int activate_skipped_units = 1; // unlisted option
    int probe_action_fix = 1; // unlisted option
    int counter_espionage = 0;
    int ignore_reactor_power = 0;
    int long_range_artillery = 0;
    int modify_upgrade_cost = 0;
    int modify_unit_support = 0;
    int modify_unit_limit = 0;
    int max_veh_num = MaxVehNum; // internal variable
    int skip_default_balance = 1; // unlisted option
    int early_research_start = 1; // unlisted option
    int base_capture_fix = 1; // unlisted option
    int facility_capture_fix = 1; // unlisted option
    int territory_border_fix = 1;
    int auto_relocate_hq = 1;
    int rebuild_secret_projects = 0;
    int steal_energy_rate = 100;
    int simple_hurry_cost = 1;
    int eco_damage_fix = 1;
    int clean_minerals = 16;
    int biology_lab_bonus = 2;
    int spawn_fungal_towers = 1;
    int spawn_spore_launchers = 1;
    int spawn_sealurks = 1;
    int spawn_battle_ogres = 1;
    int planetpearls = 1;
    int altitude_limit = 6; // internal variable
    int tile_output_limit[3] = {2,2,2};
    int soil_improve_value = 0;
    int aquatic_bonus_minerals = 1;
    int alien_guaranteed_techs = 1;
    int alien_early_start = 0;
    int cult_early_start = 0;
    int normal_elite_moves = 1;
    int native_elite_moves = 0;
    int native_weak_until_turn = -1;
    int native_lifecycle_levels[6] = {40,80,120,160,200,240};
    int cost_factor[MaxDiffNum] = {13,12,11,10,8,7};
    int tech_cost_factor[MaxDiffNum] = {124,116,108,100,84,76};
    int content_pop_player[MaxDiffNum] = {6,5,4,3,2,1};
    int content_pop_computer[MaxDiffNum] = {3,3,3,3,3,3};
    int unit_support_bonus[MaxDiffNum] = {0,0,0,0,0,0};
    int facility_talent_value[4] = {1,2,1,1};
    int facility_defense_value[4] = {100,100,100,100};
    int dream_twister_bonus = 50;
    int neural_amplifier_bonus = 50;
    int fungal_tower_bonus = 50;
    int planet_defense_bonus = 0;
    int sensor_defense_ocean = 0;
    int intercept_max_range = 2;
    int collateral_damage_value = 3;
    int repair_minimal = 1;
    int repair_fungus = 2;
    int repair_friendly = 1;
    int repair_airbase = 1;
    int repair_bunker = 1;
    int repair_base = 1;
    int repair_base_native = 10;
    int repair_base_facility = 10;
    int repair_nano_factory = 10;
    int repair_battle_ogre = 0;
    LMConfig landmarks;
    int minimal_popups = 0; // unlisted option
    int diplo_patience = 0; // internal variable
    uint32_t skip_random_events = 0; // internal variable
    uint32_t skip_random_factions = 0; // internal variable
    uint64_t skip_gov_facility = 0; // internal variable
    int faction_file_count = 14; // internal variable
    int reduced_mode = 0; // internal variable
    int debug_mode = 0; // internal variable
    int debug_verbose = 0; // internal variable
};

struct AIPlans {
    int main_region = -1;
    int main_region_x = -1;
    int main_region_y = -1;
    int main_sea_region = -1;
    int target_land_region = -1;
    int prioritize_naval = 0;
    int naval_scout_x = -1;
    int naval_scout_y = -1;
    int naval_airbase_x = -1;
    int naval_airbase_y = -1;
    int naval_start_x = -1;
    int naval_start_y = -1;
    int naval_end_x = -1;
    int naval_end_y = -1;
    int naval_beach_x = -1;
    int naval_beach_y = -1;
    int land_combat_units = 0;
    int sea_combat_units = 0;
    int air_combat_units = 0;
    int probe_units = 0;
    int missile_units = 0;
    int transport_units = 0;
    int unknown_factions = 0;
    int contacted_factions = 0;
    int enemy_factions = 0;
    int build_tubes = 0;
    /*
    Amount of minerals a base needs to produce before it is allowed to build secret projects.
    All faction-owned bases are ranked each turn based on the surplus mineral production,
    and only the top third are selected for project building.
    */
    int project_limit = 5;
    int median_limit = 5;
    int energy_limit = 15;
    /*
    PSI combat units are only selected for production if this score is higher than zero.
    Higher values will make the prototype picker choose these units more often.
    */
    int psi_score = 0;
    int keep_fungus = 0;
    int plant_fungus = 0;
    int satellite_goal = 0;
    int enemy_odp = 0;
    int enemy_sat = 0;
    int mil_strength = 0;
    int defense_modifier = 0;
    int max_offense_value = 0;
    int max_defense_value = 0;
    float enemy_base_range = 0;
    float enemy_mil_factor = 0;
    int enemy_bases = 0;
    int captured_bases = 0;
};

extern Config conf;
extern AIPlans plans[MaxPlayerNum];

/* Settings documented by Thinker as the original game's behaviour. Options
 * that select Thinker AI or UI behaviour are left for the owning package. */
void config_vanilla(Config& c);

} // namespace engine
} // namespace alpha
