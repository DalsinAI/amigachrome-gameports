# Alpha integration record — 28 October 2026

Branch `team3/alpha-integration` = `feat/alpha-upkeep-stage` (#118, 0dd99dd) merged
(true merges, history kept) with:

| Branch | Result |
|---|---|
| feat/alpha-owned-prebattle-prefix | merged (see renames below) |
| feat/alpha-notification-layout | merged |
| feat/alpha-cvr-faction-palette, manual-research-choice, human-research-pool | merged |
| feat/alpha-research-eligibility | included through human-research-pool |
| feat/alpha-original-human-costs | merged (code already identical in prebattle; keeps docs/evidence) |
| feat/alpha-basic-speeder | merged; speeder profile ported onto the current map window |
| feat/alpha-startup-movies | merged; intro button is action 8 (V), local actions moved below the status box |
| feat/alpha-base-mineral-accounting, signed-raw-ecology, owned-unit-support | already ancestors of prebattle-prefix |
| feat/alpha-technology-effects-draft | skipped: superseded by prebattle 4bd057f (same files, later revision) |
| feat/alpha-original-research-cost | skipped: superseded by #118 research_progress/world_research (complete original tech_rate cost) |
| research/alpha-smac-graphics-crop-context | skipped: does not merge cleanly (map window, frontend/map build scripts) |

## Renames made to join the two accounting lines

#118 and prebattle-prefix independently added `base_minerals`, `base_energy`,
`base_psych`, `energy_allocation` and `world_energy` (sources, probes, tests, docs).
Prebattle versions keep the names; the #118 versions are now `upkeep_*`
(`test_upkeep_*.py`, `UPKEEP-BASE-PSYCH.md`, `UPKEEP-ENERGY-ALLOCATION.md`,
`WORLD-TECHNOLOGY-EFFECTS.md`) with their own include guards. #118 types sharing
names with prebattle types got an `Upkeep` prefix. #118's fresh tile-yield variant
is `world_upkeep_fresh_tile_yields` in `world_upkeep_tile_yields.cpp`. Shared stable
intake/convoy code uses prebattle's `technology_effects` path. Historical evidence
JSON still names the old #118 test files.

## Controls (Ubuntu host, 4 cores)

Host: `c++` with ASan/UBSan, each `test_*.py --out DIR`. m68k: `--cc
m68k-linux-gnu-g++ --m68k-linux` (qemu-m68k 68040), when the control supports it.
Fixture controls get `--fixtures` from the output of test_inactive_prototype_allocation
(unreferenced, reactor_stage), reactor_stage (reactor_sequence) or reactor_sequence
(reactor_suspension).

Host 241/241 pass. m68k 199/201 pass, 40 host-only. The two m68k failures
(`test_local_unit_wait`, `test_ready_search`: probe exit 7, empty output) fail
identically on unmodified `feat/alpha-owned-prebattle-prefix`. They are not caused by
this integration.

| Control | Host | m68k |
|---|---|---|
| test_air_port_combat | PASS | PASS |
| test_artifact_capture | PASS | PASS |
| test_artillery_combat | PASS | PASS |
| test_attack_modifier_ledger | PASS | PASS |
| test_automatic_prototype_name | PASS | PASS |
| test_base_combat | PASS | PASS |
| test_base_commerce | PASS | n/a |
| test_base_energy | PASS | PASS |
| test_base_minerals | PASS | PASS |
| test_base_nutrients | PASS | PASS |
| test_base_psych | PASS | PASS |
| test_base_suffix | PASS | PASS |
| test_base_support | PASS | PASS |
| test_basic | PASS | PASS |
| test_basic_combat | PASS | PASS |
| test_battle_aftermath | PASS | PASS |
| test_battle_bookkeeping | PASS | PASS |
| test_battle_entry | PASS | PASS |
| test_citizen_definitions | PASS | PASS |
| test_colony | PASS | PASS |
| test_colony_scenario | PASS | PASS |
| test_combat_alias_order | PASS | PASS |
| test_combat_crt | PASS | n/a |
| test_combat_difficulty | PASS | PASS |
| test_combat_factions | PASS | PASS |
| test_combat_labels | PASS | PASS |
| test_combat_prototype | PASS | PASS |
| test_combat_round | PASS | PASS |
| test_combat_rounds | PASS | n/a |
| test_completion_tail | PASS | PASS |
| test_cost_factor | PASS | n/a |
| test_custom_prototype_construction | PASS | PASS |
| test_cvr_infantry | PASS | PASS |
| test_cvr_lighting | PASS | PASS |
| test_cvr_lod | PASS | PASS |
| test_cvr_model | PASS | PASS |
| test_cvr_palette | PASS | PASS |
| test_cvr_raster | PASS | PASS |
| test_cvr_speeder | PASS | PASS |
| test_defense_abilities | PASS | PASS |
| test_direct_prototype_retirement | PASS | PASS |
| test_discovery_reactor_sequence | PASS | PASS |
| test_discovery_reactor_stage | PASS | PASS |
| test_discovery_reactor_suspension | PASS | PASS |
| test_drone_transition | PASS | n/a |
| test_energy_allocation | PASS | PASS |
| test_energy_convoy_orders | PASS | PASS |
| test_energy_inefficiency | PASS | PASS |
| test_existing_prototype_reuse | PASS | PASS |
| test_exploration | PASS | PASS |
| test_facility_availability | PASS | PASS |
| test_facility_definitions | PASS | PASS |
| test_facility_install_prefix | PASS | PASS |
| test_facility_notice | PASS | PASS |
| test_facility_production | PASS | n/a |
| test_facility_upkeep | PASS | n/a |
| test_facility_upkeep_session | PASS | n/a |
| test_faction | PASS | PASS |
| test_field_attack | PASS | n/a |
| test_fresh_facility_materials | PASS | n/a |
| test_fungus_combat | PASS | PASS |
| test_ground_modifiers | PASS | PASS |
| test_historical_aftermath | PASS | PASS |
| test_human_allocation_trace | PASS | PASS |
| test_human_component_pruning | PASS | PASS |
| test_human_upkeep_transaction | PASS | n/a |
| test_human_worker_allocation | PASS | PASS |
| test_inactive_prototype_allocation | PASS | PASS |
| test_initialized_snapshot | PASS | PASS |
| test_land_combat | PASS | PASS |
| test_land_engagement | PASS | PASS |
| test_land_resolution | PASS | PASS |
| test_leader_speech | PASS | PASS |
| test_load_record_repairs | PASS | PASS |
| test_local_accounting | PASS | PASS |
| test_local_accounting_snapshot | PASS | PASS |
| test_local_actions | PASS | PASS |
| test_local_actions_snapshot | PASS | PASS |
| test_local_player | PASS | PASS |
| test_local_production_session | PASS | PASS |
| test_local_unit_skip | PASS | PASS |
| test_local_unit_wait | PASS | FAIL(1) |
| test_local_unit_wake | PASS | PASS |
| test_local_worker_actions | PASS | PASS |
| test_local_workers | PASS | PASS |
| test_local_workers_snapshot | PASS | PASS |
| test_logic | PASS | PASS |
| test_monument_effects | PASS | PASS |
| test_monument_presentation | PASS | PASS |
| test_monument_storage | PASS | PASS |
| test_move_budget | PASS | PASS |
| test_movement | PASS | PASS |
| test_movement_snapshot | PASS | PASS |
| test_native_combat_morale | PASS | PASS |
| test_node_artifacts | PASS | PASS |
| test_notice_labels | PASS | PASS |
| test_notice_stages | PASS | PASS |
| test_original_faction | PASS | PASS |
| test_original_game | PASS | PASS |
| test_original_human_cost | PASS | PASS |
| test_original_map | PASS | PASS |
| test_original_world | PASS | PASS |
| test_pcx_image | PASS | PASS |
| test_phase_snapshot | PASS | PASS |
| test_popup_response | PASS | PASS |
| test_prebattle_aggression | PASS | PASS |
| test_prebattle_notification | PASS | PASS |
| test_prebattle_prefix | PASS | PASS |
| test_prebattle_territory | PASS | PASS |
| test_production_accrual | PASS | PASS |
| test_production_cost | PASS | PASS |
| test_production_loans | PASS | PASS |
| test_production_retool | PASS | PASS |
| test_production_snapshot | PASS | PASS |
| test_promoted_aftermath | PASS | PASS |
| test_promoted_land_body | PASS | PASS |
| test_promotion_selection | PASS | PASS |
| test_proposed_energy_allocation | PASS | n/a |
| test_proposer_record_stages | PASS | PASS |
| test_prototype | PASS | PASS |
| test_prototype_prefix_context | PASS | PASS |
| test_prototype_retirement_queue | PASS | PASS |
| test_psi_abilities | PASS | PASS |
| test_queue_shift | PASS | PASS |
| test_ready_search | PASS | FAIL(1) |
| test_repair_vehicle | PASS | n/a |
| test_research_choice | PASS | PASS |
| test_research_options | PASS | PASS |
| test_research_pool | PASS | PASS |
| test_research_pool_world | PASS | PASS |
| test_research_progress | PASS | n/a |
| test_resources | PASS | PASS |
| test_resumed_commands | PASS | PASS |
| test_retirement_history | PASS | PASS |
| test_retreat_destination | PASS | PASS |
| test_retreat_init | PASS | PASS |
| test_riot_morale | PASS | PASS |
| test_rules | PASS | PASS |
| test_saved_speed | PASS | PASS |
| test_secrets_monument | PASS | PASS |
| test_sensor_combat | PASS | PASS |
| test_social | PASS | PASS |
| test_social_upkeep | PASS | PASS |
| test_stable_specialist_accounting | PASS | n/a |
| test_startup_movie | PASS | PASS |
| test_supply_pods | PASS | PASS |
| test_surface_actions | PASS | n/a |
| test_technology_browser | PASS | PASS |
| test_technology_effects | PASS | PASS |
| test_technology_effects_alias | PASS | PASS |
| test_technology_effects_world | PASS | PASS |
| test_terrain_defense | PASS | PASS |
| test_territorial_aftermath | PASS | PASS |
| test_territorial_combat | PASS | PASS |
| test_tile_bonus | PASS | PASS |
| test_tile_yields | PASS | PASS |
| test_unit_availability | PASS | PASS |
| test_unit_readiness | PASS | PASS |
| test_units | PASS | PASS |
| test_unreferenced_prototype_allocation | PASS | PASS |
| test_unused_prototype_retirement | PASS | PASS |
| test_upkeep_base_energy | PASS | n/a |
| test_upkeep_base_minerals | PASS | n/a |
| test_upkeep_base_psych | PASS | n/a |
| test_upkeep_energy_allocation | PASS | n/a |
| test_upkeep_world_energy | PASS | n/a |
| test_vehicle_removal | PASS | PASS |
| test_vehicle_stack | PASS | PASS |
| test_worker_allocation | PASS | n/a |
| test_world_actions | PASS | PASS |
| test_world_air_port_combat | PASS | PASS |
| test_world_artillery_combat | PASS | PASS |
| test_world_base_accounting | PASS | n/a |
| test_world_base_budget | PASS | n/a |
| test_world_base_combat | PASS | PASS |
| test_world_base_commerce | PASS | n/a |
| test_world_base_minerals | PASS | n/a |
| test_world_base_psych | PASS | n/a |
| test_world_basic_combat | PASS | PASS |
| test_world_battle_bookkeeping | PASS | PASS |
| test_world_battle_entry | PASS | PASS |
| test_world_battle_reporting | PASS | PASS |
| test_world_casualty | PASS | PASS |
| test_world_citizen_energy | PASS | PASS |
| test_world_combat_difficulty | PASS | PASS |
| test_world_combat_factions | PASS | PASS |
| test_world_combat_prototype | PASS | PASS |
| test_world_combat_sensors | PASS | n/a |
| test_world_combat_text | PASS | PASS |
| test_world_commerce | PASS | PASS |
| test_world_convoy_nutrients | PASS | PASS |
| test_world_defense_abilities | PASS | PASS |
| test_world_energy | PASS | PASS |
| test_world_energy_allocation | PASS | n/a |
| test_world_energy_hq | PASS | PASS |
| test_world_facility_options | PASS | PASS |
| test_world_facility_upkeep | PASS | n/a |
| test_world_field_combat | PASS | n/a |
| test_world_fresh_minerals | PASS | PASS |
| test_world_fresh_nutrients | PASS | PASS |
| test_world_fresh_tile_yields | PASS | PASS |
| test_world_fungus_combat | PASS | PASS |
| test_world_ground_modifiers | PASS | PASS |
| test_world_human_allocation | PASS | PASS |
| test_world_movement_price | PASS | PASS |
| test_world_native_combat_morale | PASS | PASS |
| test_world_pacifism | PASS | PASS |
| test_world_police | PASS | PASS |
| test_world_population | PASS | n/a |
| test_world_prebattle_diplomacy | PASS | PASS |
| test_world_prebattle_territory | PASS | PASS |
| test_world_production_accrual | PASS | PASS |
| test_world_production_cost | PASS | PASS |
| test_world_production_prefix | PASS | PASS |
| test_world_production_queue | PASS | PASS |
| test_world_production_retool | PASS | PASS |
| test_world_promotion | PASS | PASS |
| test_world_psi_abilities | PASS | PASS |
| test_world_psych | PASS | PASS |
| test_world_repair_vehicle | PASS | PASS |
| test_world_research | PASS | n/a |
| test_world_sensor_combat | PASS | PASS |
| test_world_social_upkeep | PASS | PASS |
| test_world_stable_intake | PASS | PASS |
| test_world_supported_minerals | PASS | n/a |
| test_world_technology_effects | PASS | n/a |
| test_world_terraform_ecology | PASS | PASS |
| test_world_terrain_defense | PASS | PASS |
| test_world_territorial_combat | PASS | PASS |
| test_world_tile_yields | PASS | PASS |
| test_world_traded_budget | PASS | n/a |
| test_world_unit_availability | PASS | PASS |
| test_world_unit_stats | PASS | PASS |
| test_world_unit_support | PASS | PASS |
| test_world_upkeep_accounting | PASS | n/a |
| test_world_upkeep_research | PASS | n/a |
| test_world_vehicle_removal | PASS | PASS |
| test_world_worker_radius | PASS | PASS |
| test_world_worker_resolution | PASS | n/a |
| test_world_worker_upkeep | PASS | n/a |
| test_world_zoc | PASS | PASS |
