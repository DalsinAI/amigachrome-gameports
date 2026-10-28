# WP6: computer players

`engine/ai_*.cpp` is the Thinker Mod computer player at
`dabe62f2d1abba03de40f3dd3048494843f6a9af` (MIT, see `THINKER-LICENSE`),
ported function by function:

| File | Thinker source | Contents |
| --- | --- | --- |
| `ai_path.cpp` | `src/path.cpp` | `TileSearch`, `path_get_next`, `path_cost`, `route_dist`, `contiguous`, `update_move_path`, ... |
| `ai_move.cpp` | `src/move.cpp` | `move_upkeep`, priority maps (`mapdata`, `mapnodes`), colony/former/crawler/combat/transport/nuclear moves, `base_tile_score` |
| `ai_veh_turn.cpp` | `src/veh_turn.cpp` | `mod_enemy_turn`, `mod_enemy_veh`, `mod_enemy_move`, `mod_alien_move`, `veh_kill_lift` |
| `ai_goal.cpp` | `src/goal.cpp` | `add_goal`, `add_site`, `del_site`, `has_goal`, `wipe_goals`, `clear_goals`, `find_priority_goal` |
| `ai_plan.cpp` | `src/plan.cpp` | `plans_upkeep`, `design_units`, `reset_state` |
| `ai_build.cpp` | `src/build.cpp` | `select_build` and the production scorers |
| `ai_strategy.cpp` | `src/faction_event.cpp` | `enemy_capabilities`, `enemy_strategy` |
| `ai_social.cpp` | `src/faction.cpp` | `mod_social_ai`, `social_score`, `max_social_value` |
| `ai_tech.cpp` | `src/tech.cpp` | `mod_tech_val`, `mod_tech_ai`, `tech_is_preq` |
| `ai_fauna.cpp` | `src/gameturn.cpp` | `alien_fauna` |
| `ai_deps.cpp` | `src/base.cpp`, `faction.cpp`, `game.cpp`, `map.cpp`, `net.cpp`, `veh.cpp`, `veh_action.cpp`, `veh_combat.cpp` | private copies of helpers the AI needs (below) |
| `ai_exe.cpp` | OpenSMACX (GPL-3) or new code | replacements for `terranx.exe` routines Thinker calls through pointers |
| `ai_entry.cpp`, `ai_api.h` | — | work counters and the turn hook wrappers |

Everything AI-internal is in `namespace alpha::engine::ai`, so the private
helper copies never clash with the packages that own the same Thinker
functions. The `engine_wp.h` WP6 entry points (`mod_enemy_turn`,
`mod_enemy_veh`, `mod_enemy_move`, `enemy_strategy`, `enemy_capabilities`,
`mod_social_ai`, `mod_tech_val`, `mod_tech_ai`, `mod_alien_move`,
`alien_fauna`, `design_units`, `select_build`, `veh_kill_lift`, `del_site`,
`add_site`) are defined in `alpha::engine`. They are never redeclared inside
`ai`: argument-dependent lookup on engine enums would make calls ambiguous.

`stubs/wp6.cpp` is deleted.

## Turn hooks (WP1)

`ai_api.h` adds what the turn loop calls besides `engine_wp.h`:

```cpp
void ai_plans_upkeep(int faction_id);          // faction upkeep, first step   (hook plans_upkeep)
void ai_move_upkeep(int faction_id, int mode); // after mod_social_ai, mode 0 = UM_Full (hook move_upkeep)
int  ai_probe_upkeep(int faction_id);          // returns 0, probe teams deferred (hook probe_upkeep)
void ai_reset_state();                         // after a game is loaded or started
const AIStats& ai_stats(); void ai_stats_reset();
```

Thinker's `faction_upkeep` order is: `plans_upkeep`, social upkeep,
repair, production (calls `select_build`), energy, diplomacy,
`enemy_strategy`, `mod_social_ai(f, -1, -1, -1, -1, 0)`, `probe_upkeep`,
`move_upkeep(f, UM_Full)`. Unit moves then go through `mod_enemy_turn` /
`mod_enemy_veh` / `mod_enemy_move`; natives through `alien_fauna` and
`mod_alien_move`. WP1's `consider_designs` hook has no Thinker body (the
executable's designer); Thinker designs units in `design_units`, which
`mod_social_ai` and `plans_upkeep` call.

## Core change

`engine_wp.h` declares the cross-package functions the AI calls and that
their packages had outside `engine_wp.h` (separate commit
"Declare the WP2 and WP3 entry points the computer players call"; the
`social_set` line landed in the next commit):

- WP2 `action_staple`;
- WP3 `action`, `action_build`, `action_gate`, `study_artifact`,
  `Path_find`, `Path_move`;
- WP5 `social_set`.

`stubs/wp6_deps.cpp` holds placeholders for those eight functions. **Delete
it when WP2, WP3 and WP5 are merged** (they define all eight with the same
signatures); a leftover placeholder is a duplicate symbol at link time. A
local merge with `team3/alpha-engine` at `dbf8741` (WP1-WP5 joined) linked
cleanly after deleting it and nothing else.

## Private copies to replace at integration

`ai_deps.cpp` copies Thinker helpers into `ai`. Several are also ported by
other packages (found on `team3/alpha-engine` at `dbf8741`). They can stay
(no clash), or be dropped in favour of the shared ones once those are in
`engine_wp.h`:

| AI copy | Also in |
| --- | --- |
| `un_charter`, `victory_done`, `voice_of_planet`, `has_wmode`, `council_votes`, `aah_ooga`, `great_beelzebub`, `climactic_battle`, `great_satan` | WP2 `base_deps` (from WP8) |
| `base_can_riot`, `base_unused_space`, `base_growth_goal`, `can_build`, `can_build_unit`, `can_link_artifact`, `can_staple`, `can_use_teleport`, `unit_support_plan`, `prod_name`, `psych_check`, `mineral_factor`, `mineral_output_modifier`, `mod_bases_reset` | WP2 `base_upkeep.cpp` |
| `best_armor`, `best_weapon`, `best_reactor`, `proto_offense`, `proto_defense`, `prototype_factor`, `mod_upgrade_cost`, `part_upgrade`, `full_upgrade`, `create_proto`, `retire_proto` | WP5 `design.cpp` |
| `use_nerve_gas` (both), `mod_battle_fight` | WP4 `combat.cpp`, `combat_fight.cpp` |
| `terrain_avail`, `mod_veh_skip`, `base_on_sea` | WP3 `veh_action.cpp`, `veh_move.cpp`, `orders.cpp` |

AI-only copies with no other owner: `has_aircraft`, `has_ships`,
`mod_wants_to_attack` (+ `evaluate_attack`), `valid_relocate_base`,
`veh_upgrade`, `set_move_to`, `set_move_next`, `set_road_to`, `set_action`,
`set_order_none`, `set_convoy`, `set_board_to`, `bonus_yield`,
`fungus_yield`, `total_yield`, `item_yield`, `net_action_build`,
`net_action_gate`, `net_action_destroy`.

## Replacements for executable routines (`ai_exe.cpp`)

| Routine | Source |
| --- | --- |
| `garrison_check`, `base_territory`, `planet_buster`, `abstract_at`, `base_on_sea`, `Path::territory` | OpenSMACX (GPL-3.0-or-later, attributed in the file) |
| `world_site` | new: `clamp(1 + base_tile_score / 8, 1, 15)` |
| `retire_proto` | new: sets the faction bit in `obsolete_factions` |
| `set_course` (native transports) | new: a go-to order on waypoint 0, or `ORDER_NONE` |
| `Path::find` inside `contiguous()` | new: `territory_route`, a breadth-first search over the territory marked by `Path_territory` (or any same-region land) |
| `draw_tiles`, `POP2`, `MapWin_focus`, `Console_focus` | `draw_tile` on the centre, `popp`, no map view (return 0) |

## Deviations

Behaviour (`Fix:` in the code):

- `ai_move.cpp` `target_priority`: unowned tiles indexed `plans[-1]`; uses
  slot 0 (only read for owned tiles).
- `ai_deps.cpp` `set_move_to`: an off-map target skips the unit. Seen in
  play: `trans_move` still finds a naval start node after the naval plan was
  dropped (`naval_end` = -1, -1).
- `ai_exe.cpp` `abstract_at`: bounds check.
- `ai_fauna.cpp`: `MaxVehModNum` (8192) is beyond the engine's `MaxVehNum`
  (2048); the modified limit is `MaxVehNum * 3 / 4`.
- `ai_deps.h` `ProbeActionsPorted = false`: probe team actions
  (Thinker `probe.cpp`, deferred in the plan) are not ported; `combat_move`
  skips its subvert branch and `ai_probe_upkeep` returns 0.
- Thinker's own `Fix:` comments are kept as they are.

Removed (`Original:` in the code): the executable's `enemy_move`,
`social_ai` and `consider_designs` fallbacks for factions Thinker does not
manage (with `config_vanilla`, `factions_enabled = 7`, Thinker manages
every computer faction; unmanaged units skip their turn); multiplayer and
`NetDaemon` branches; `net_show`; `Console_update_data`; `Console_upgrade`;
asserts comparing against executable functions (`tech_val`,
`upgrade_cost`, `alien_base`).

## Determinism and the 68040 budget

AI randomness uses the engine RNG (`random`, `game_rand`) in the bound
state. `mapdata` is an `unordered_map`, but every iteration over it feeds a
totally ordered priority queue, so bucket order does not matter. Thinker
uses `float`/`double` in `battle_priority`, `battle_eval`, the build
weights and `plans` (`enemy_mil_factor`, `enemy_base_range`); the tests
compare these between an x86-64 host build and an m68k build (68881 FPU)
and found no difference so far (see Tests).

`ai_stats()` counts work per call site (not in Thinker, counters only):
`enemy_turns`, `veh_moves`, `move_upkeeps` (priority map rebuilds),
`tile_search_steps` (tiles visited by `TileSearch`), `path_find_calls`,
`build_choices`, `strategy_calls`. `move_upkeep` and the tile searches
dominate.

Rough measure (2026-10-28, local merge with `team3/alpha-engine` `dbf8741`
so that WP1-WP5 are real code; m68k-linux build `-O1 -g`, instructions
counted with `qemu-m68k -cpu m68040 -one-insn-per-tb -d nochain,exec`,
seed 1, synthetic expansion world, 7 factions):

| Run | m68k instructions |
| --- | --- |
| setup only (rules load, map, factions) | 41.5 M |
| setup + turn 1 (19 bases, 75 units) | 150.8 M |
| setup + turns 1-5 (35 bases, 163 units at turn 5) | 787.7 M |

That is about 109 M instructions for turn 1 and 159 M per turn on average
for turns 2-5 (the counts include the test's own order executor and
digest, which are small next to the AI). At an assumed 15-20 MIPS for a
25 MHz 68040 this is roughly 8-11 s per full computer-player round at that
size, and it grows with unit count (`tile_search_steps` per turn rose from
24 k to 46 k over those turns). Wall times printed by the test under qemu
or ASan are not 68040 times.

## Tests

`engine/tests/wp6_test.cpp`, driven by `test_engine_wp6.py`:

```sh
python3 test_engine_wp6.py --out /tmp/wp6-host                     # host, ASan/UBSan
python3 test_engine_wp6.py --out /tmp/wp6-m68k --cc m68k-linux-gnu-g++ --m68k-linux \
    --compare /tmp/wp6-host/receipt.json                             # qemu-m68k -cpu m68040
python3 test_engine_wp6.py --out /tmp/wp6-own --rules DIR            # owner rules, local only
```

- `unit`: helper checks (tech prerequisites, goals and sites, garrison
  size, tile search order, base tile scores, region helpers, technology
  values, one plans/move/build pass).
- `turns`: the synthetic rules of `test_engine_wp0b.py`, with consistent
  component, unit, technology and facility tables set by the test (test
  values, not game data), a seeded 48x32 map, seven computer factions with
  2-3 bases each, units and native life. Each turn runs the faction upkeep
  AI order above plus one `mod_enemy_move` per unit and native moves; a
  test executor carries out go-to orders and finished production. A digest
  over unit positions and orders, production, goals, region plans, `plans`,
  research and social choices, prototypes and map ownership is printed
  per turn. The harness requires identical digests on rerun and, with
  `--compare`, between builds.

With only `engine_wp.h` stubs for WP1-WP5 the AI's actions mostly reach
placeholders (`action_build`, `Path_find`, `veh_avail`, ...); the run then
tests that the AI code is crash-free and deterministic, not that it plays
well. Each run prints which stubs it hit.
