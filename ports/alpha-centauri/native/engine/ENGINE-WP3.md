# WP3: unit orders, movement, path search and terraforming

WP3 ports Thinker Mod `dabe62f2d1abba03de40f3dd3048494843f6a9af` (MIT, see
`THINKER-LICENSE`) with the vanilla configuration (`config_vanilla`). It
implements the WP3 section of `engine_wp.h` and replaces `stubs/wp3.cpp`.

| File | Contents |
|------|----------|
| `veh_action.cpp` | Thinker `veh_action.cpp`: `order_veh` and the `action_*` functions |
| `veh_move.cpp` | Thinker `veh.cpp` (part): speed, supply pods, monolith, skip/wake, visibility |
| `terrain.cpp` | Thinker `map.cpp` (part): altitude changes, `mod_hex_cost`, bonus/mineral/pod tests |
| `path.cpp` | Thinker `path.cpp` (part): `TileSearch`, zones of control, cargo helpers; new `Path_find`/`Path_move` |
| `orders.cpp` | new: replacements for executable-only helpers and the player order API |
| `veh_action.h` | WP3 declarations beyond `engine_wp.h` |
| `veh_action_ui.h` | private presentation, multiplayer and popup shims (namespace `wp3_ui`) |
| `wp3_deps.h/.cpp` | weak fallbacks for functions other packages own |
| `tests/wp3_test.cpp`, `../test_engine_wp3.py` | controls |

Deliberate changes from Thinker are marked `Fix:` in the source. Thinker's
own `Fix:` comments are kept as they are.

## Functions

Ported from Thinker (73 functions, 3 of them file-static):

- `veh_action.cpp` (22 + 1): `terrain_avail`, `contribution`,
  `terraform_cost`, `action`, `action_build`, `action_terraform`,
  `action_go_to`, `action_road_to`, `action_destroy`, `action_home`,
  `action_airdrop`, `action_arty`, `action_destruct`, `action_oblit`,
  `valid_patrol`, `action_patrol`, `shoot_it`, `action_tectonic`,
  `action_fungal`, `action_give`, `action_gate`, `order_veh`; the static
  `battle_fight` is Thinker's `mod_battle_fight` body, kept local because
  WP4 defines the global `mod_battle_fight`.
- `veh_move.cpp` (19 + 1): `veh_find`, `proto_speed`, `veh_speed`,
  `want_monolith`, `monolith`, `do_unity_crash`, `goody_box` (static
  `goody_rand`), `study_artifact`, `veh_skip`, `mod_veh_skip`, `veh_wake`,
  `find_return_base`, `spot_base`, `spot_stack`, `unspot_stack`, `spot_loc`,
  `want_to_wake`, `wake_stack`, `spot_all`.
- `terrain.cpp` (9 + 1): `alt_set`, `alt_set_both`, `world_alt_set`,
  `world_raise_alt`, `world_lower_alt`, `mod_hex_cost`, `mod_minerals_at`,
  `mod_bonus_at`, `mod_goody_at` (static `mapgen_rand`).
- `path.cpp` (19): `TileSearch::reset`, `add_start`, `init` (3 overloads),
  `get_route`, `has_zoc`, `get_prev`, `get_node`, `get_next`, `zoc_any`,
  `zoc_veh`, `zoc_sea`, `zoc_move`, `path_get_next`, `path_cost`,
  `veh_cargo_loaded`, `cargo_capacity`, `has_transport`.
- `orders.cpp` (1): `say_loc` (Thinker `gui.cpp`).

New code, because no upstream has a decompiled source for these executable
functions:

- `Path_find`, `Path_move` (terranx `Path::find`, `Path::move`): a
  least-cost search (Dijkstra) over the eight `BaseOffset` directions using
  `mod_hex_cost` (step cost `16 * cost + 1`, so equal costs prefer fewer
  tiles and free mag tube steps still progress). It applies the same rules as
  `order_veh`: land units stay on land and sea units stay at sea, except in a
  base of their own or of a pact partner; air units go anywhere. Tiles held
  by other factions without a pact are blocked, except the destination. A
  land step between two tiles in an enemy zone of control is blocked, unless
  the search uses `PATH_IGNORE_ZOC` or `faction_id < 0`. `PATH_AVOID_FUNGUS`
  avoids fungus without a road. The search expands at most
  `min(tiles, 65536)` nodes.
  Differences from the original: the original's search window, its
  tie-breaking and its AI-specific cost tweaks are unknown, so the route
  between equal-cost routes may differ. `unk` is ignored.
- `delete_landmark` (terranx 0x5927D0): `alt_set` calls it. It removes a
  landmark anchored exactly on the tile.
- `supply_options` (0x593A00): its check mode returns 1. A human owner
  entering their own base gets popup `SUPPLYOPTIONS`. Answer 1 helps build:
  `mod_veh_cost * mineral row cost` minerals go into production, message
  `SUPPLYHELP`, and the crawler is removed. Any other answer, or an AI
  owner, just moves in.
- `quick_zoc` (0x593830) finds an enemy unit exerting a zone of control (it
  is used only for the tutorial text).
- `base_on_sea` (0x50DE50), `port_to_port` (0x50E160), `radius_move_2`
  (0x5A65D0) and `veh_ready` (0x5C1B70), each from its documented purpose.
- Player order API for the UI and turn loop (`veh_action.h`): `order_move`,
  `order_go_to`, `order_road_to`, `order_terraform`, `order_build_base`,
  `order_sentry`, `order_hold`, `order_wake`, `order_skip`, `order_disband`,
  `order_set_home`, `order_return_home`, `order_convoy`, `order_upgrade`,
  `order_airdrop`, `order_patrol`, `order_continue`. Each one checks the
  legality that the original's Console menus checked. It then calls the
  ported action function (`order_veh`, `action_terraform`, `action_build`,
  `action_home`, ...). Go-to and road-to orders are then stepped with
  `action` until the unit has no moves left. Upgrades follow Thinker's
  `veh_upgrade` with the `part_upgrade` conditions and cost
  `10 * mod_upgrade_cost` credits. Disbanding in an own base adds half the
  unit's mineral value to production.

## Deviations (`Fix:` and shims)

- `order_veh` reads `Factions[f].diplo_status[tgt_fc_id]`. When the target
  tile is empty (for example a land unit stepping onto empty ocean),
  `tgt_fc_id` is -1, so the original read the field in front of the array.
  Fix: `diplo_with()` returns 0 for an owner outside 0..7.
- Popups go through the event sink. In `veh_action_ui.h`, `Popup`,
  `Dialogs_item` and `BasePop_exec_3` record the item values, put them in
  `$WORD9` as `"v0,v1,..."` and call `popp(filename, label)`. The answer is
  an index into that list. Cancel (a negative answer) selects the first
  item, as Escape did.
- `X_text_open` opens the `script.txt` section through the rules loader's
  `text_open`, so the item texts are real when the file exists. Fix: the
  popup goes ahead even when the file is missing; the item values decide the
  choice.
- Presentation calls are inert or forward to the sink: `Console_*`,
  `BaseWin_*`, `GraphicWin_redraw`, `FX_*`, `clock_wait`, `veh_scoot`,
  `TutWin_*`, `draw_tiles` (forwards to `draw_radius`), and `tut_check`
  (returns 0, no tutorial). Multiplayer synchronisation (`synch_veh`,
  `NetDaemon_*`, `message_*`) is inert; the engine has no network play.
- Tutorial counters (`dword_8CC210`, `dword_945AF4`, `dword_945AF8`) are
  file-static. The `Console` field 0x23BF0 is `MapWin_field_23BF0`.
- `_stricmp`, `parse_string`, `label_get`, `text_open` and `text_get` come
  from the rules loader (`rules.h`), not from WP3.
- `CharLowerA` became a `tolower` loop, the `mod_hex_cost` debug cross-check
  against the executable was removed, and `veh_cargo` and `veh_fake` stay in
  the core `veh.cpp`.
- `order_terraform` reads the status letter from `Order[]` only when the
  rules are loaded.

## Weak fallbacks (`wp3_deps.cpp`)

These functions are called by WP3 but owned elsewhere and not declared in
`engine_wp.h`. Each is defined `__attribute__((weak))`, so the owner's
ordinary definition replaces it at link time (checked with host g++,
m68k-linux and m68k-amigaos `nm`: the symbols are `W`):

| Function | Owner | Fallback |
|----------|-------|----------|
| `world_climate` | WP7 | `not_implemented` |
| `double_cross`, `break_treaty`, `act_of_aggression`, `probe` | WP8 / probe teams | `not_implemented`; the int ones return 0 (go ahead) |
| `mon_colony_founded`, `make_base_unique` | WP2 | `not_implemented` |
| `tech_is_preq` | WP5 | `not_implemented`, returns 0 |
| `add_goal` | WP6 | silent |
| `mod_veh_cost` | WP5 | Thinker body without the prototype surcharge |
| `mod_upgrade_cost` | WP5 | Thinker vanilla branch |
| `mod_say_morale2` | WP4 | morale name only |
| `prefs_save` | UI | no-op |
| `boom_veh`, `boom_veh_at`, `boom_tile` | WP4 | `draw_tile` |

WP4 (`team3/alpha-wp4-combat`) has non-weak placeholders for several of
these in `stubs/wp4_deps.cpp`; they win over these. WP6 declares `action`,
`action_build`, `action_gate`, `study_artifact`, `Path_find` and `Path_move`
in its `engine_wp.h` section with these signatures. Its placeholders in
`stubs/wp6_deps.cpp` must be deleted when both packages are merged.

## Core change

One commit adds runtime fields to `EngineState`, bound in `engine_bind`:
`VehAttackFlags`, `ControlTurnC`, `MapBaseSubmergedCount`,
`MapBaseIdClosestSubmergedVeh` (reset to -1), `WorldSkipTerritory`,
`dword_9B22E0` (reset to -1), `dword_90DB84` and `StrBuffer`.

## Not done

- Thinker `path.cpp` AI helpers (`defend_tile`, `safe_path`, `search_*`,
  `route_dist`, `update_move_path`, `contiguous`, ...) belong to WP6.
- Probe-team actions (`probe`) and the climate model (`world_climate`) are
  fallbacks only (see above).
- Terraforming progress for units outside `order_continue` and `action`, and
  air-unit refuelling at the start of a turn, belong to the WP1 turn loop.
  WP3 provides `action`, `veh_skip`/`mod_veh_skip` and `order_veh`.
- `Path_find` is not a byte-for-byte reconstruction of `Path::find` (see
  above).

## Tests

```
python3 test_engine_wp3.py --out OUT                                      # host, ASan/UBSan
python3 test_engine_wp3.py --cc m68k-linux-gnu-g++ --m68k-linux --out OUT # qemu-m68k (big-endian)
```

The tests set synthetic rules tables in C++ (chassis, weapons, armor,
reactors, terraform rates with `TerraformRules` bits, units, `Rules`), so no
original assets are needed.

- `unit`: speeds (chassis, transport and air rules, damage, Maritime
  Control Center). Terraforming: farm, soil enricher, mine, solar (rocky
  bonus), forest, road (river, rocky and forest extra time), mag tube,
  bunker, airbase, sensor, remove fungus (fungicidal tanks halve the time),
  condenser, borehole, raise, lower and level. The checks cover turn counts,
  tile bits and removed incompatible bits, pooled work of several formers
  with a super former, illegal orders and `terraform_cost`. Also covered:
  base building with a colony pod (adjacency, pole, triad, rocky
  flattening); single steps, roads, and land/ocean blocking; zones of
  control and `Path_find` around them; go-to across a round and a flat map;
  transports (board, `stack_veh` load, carry, unload, sentry); air fuel
  (needlejet crash after two turns, copter hit-point loss, gravship); supply
  pods; the monolith; visibility; and hold/sentry/wake, home/return home,
  disband, upgrade, convoy and the supply crawler popup.
- `hexcost`: `mod_hex_cost` against `terrain_price.h` (OpenSMACX `hex_cost`
  arithmetic) on random adjacent tiles. Each case varies the items, altitude,
  rockiness, unit (land, sea, air, native, psi, former, artifact), faction
  (including natives), PLANET rating, Xenoempathy Dome and `toggle`.
- `budget`: one `order_veh` step from a random state (terrain, moves spent,
  retry flag 0x8000, owner, RNG seed) against `move_budget.cpp`
  (`resolve_move_budget`). The position, `moves_spent`, retry flag and RNG
  state after the step must match the oracle, including fractional-move
  failures.
