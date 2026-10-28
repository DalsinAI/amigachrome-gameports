# Alpha Centauri engine core

`engine/` is the shared game engine that the work packages (WP1..WP8) build
on. It is a near-verbatim port of Thinker Mod at
`dabe62f2d1abba03de40f3dd3048494843f6a9af` (MIT, see `THINKER-LICENSE`),
without Windows, without the original executable, and safe on big-endian
68k hosts. Ported functions keep Thinker's names, signatures and bodies;
every deviation is marked in the code with `Original:` or `Fix:` comments.

The pure modules in the parent directory (`original_*.cpp`, `world_*.cpp`,
...) are not touched by the engine and do not depend on it.

## Layout

| File | Contents |
| --- | --- |
| `engine.h` | The one header to include. Defines `__cdecl`/`__thiscall` as nothing, `UNUSED`, `assert` (off unless `ENGINE_ASSERTS`), `debug()`, `min/max/clamp/bit_count`, `Point`/`MapTile`, then pulls in everything below. |
| `engine_enums.h`, `engine_types.h`, `engine_base.h`, `engine_veh.h`, `engine_data.h` | Thinker's enums and records (`VEH`, `UNIT`, `BASE`, `MAP`, `Faction`, `MFaction`, `CChassis`, `CWeapon`, `CRules`, `GamePrefs`, ...) with the original field order. They are ordinary native structs; nothing depends on their in-memory layout. |
| `engine_win.h` | The few window fields the saved game carries (`Console`, `MapWindow`, `WorldWindow`, `StatusWindow`). |
| `config.h` | Thinker `Config`/`AIPlans`; `config_vanilla(conf)` selects original-game behaviour. |
| `game_state.h/.cpp` | `EngineState`, `engine_reset`, `engine_bind`, `map_init`, all Thinker globals. |
| `random.h/.cpp` | `game_rand` (original MSVC `rand`), `random`/`random_get`, `GameRandom`, deterministic `next_rand`. |
| `events.h/.cpp` | Event sink that replaces popups and window updates. |
| `map.h/.cpp` | Map accessors (`mapsq`, `wrap`, `map_range`, `bit_set`, `base_find`, `whose_territory`, ...). |
| `faction.h/.cpp` | Faction accessors (`has_tech`, `is_alive`, `set_alive`, `has_treaty`, `has_project`, ...). |
| `veh.h/.cpp` | `veh_init`, `veh_kill`, `kill`, `veh_put`, `veh_lift`, `veh_drop`, `veh_top`, `stack_fix`, `stack_sort`, `stack_veh`, ... |
| `bases.h/.cpp` | `base_init`/`base_kill` (Thinker `mod_base_init`/`mod_base_kill`), `set_base`, `has_fac_built`, `set_fac`, ... |
| `savegame.h/.cpp` | Original SAV container read/write. |
| `save_fields.inc` | Little-endian field visitors for every saved record. Rewrite with `tools/gen_save_fields.py` after changing a saved struct; `--check` is part of the tests. |
| `engine_wp.h` | Entry points owned by WP1..WP8. |
| `stubs/wpN.cpp` | Placeholders for those entry points (written once by `tools/gen_stubs.py`). |
| `tests/engine_test.cpp` | Unit checks and the save round-trip tool used by `../test_engine.py`. |

## State

All game data lives in one `EngineState` (full original capacity: 2048
vehicles plus the `veh_fake` slot, 512 bases, 512 prototypes, 8 factions,
map up to 4096x4096). Code uses Thinker's global names, which are pointers
into the bound state:

```cpp
#include "engine.h"
using namespace alpha::engine;

std::unique_ptr<EngineState> s(new EngineState()); // ~450 KB: allocate on the heap
engine_reset(*s);       // empty game, SaveFileVersion 12, no map
engine_bind(s.get());   // Vehs, Bases, Factions, Units, MapWin, *VehCount, ... now point into *s
config_vanilla(conf);   // original behaviour for Thinker options
*MapAreaX = 80; *MapAreaY = 40; map_init();
int veh_id = veh_init(BSC_SCOUT_PATROL, 1, x, y);
```

- `engine_bind` re-points every global; several states may exist (AI
  look-ahead, comparisons) but exactly one is bound. Not thread-safe.
- `Vehs` is `&state.vehs[1]`, so `Vehs[-1]` and `Vehs[MaxVehNum]` exist as in
  the original.
- The RNG state (`EngineState::rng`) is part of the state, so binding a state
  also selects its generators. `game_srand(seed)` and `random_reseed(seed)`
  make runs reproducible; `next_rand_seed(x, y)` replaces the timer source.
- Rules tables (`Rules`, `Chassis`, `Weapon`, `Tech`, `Facility`, ...) are part
  of the state but nothing fills them yet; they are zero after
  `engine_reset`. Loading `alpha.txt`/`alphax.txt` into them is not part of WP0.

## Events instead of popups

`popp`, `popb`, `X_pop*`, `X_pops*`, `X_dialog`, `NetMsg_pop`, `draw_map`,
`draw_tile` and `draw_radius` keep their Thinker signatures and forward to the
current `EventSink` (`set_event_sink`). The default `HeadlessSink` logs each
request and answers popups with `choices[label]` or `default_choice`, so the
engine runs without any UI. `parse_says`/`parse_num` fill `ParseStrBuffer`
and `ParseNumTable` for the UI to substitute `$WORD0`/`$NUM0`.

Engine notifications (`engine_notify`): `EV_VEH_CREATED`, `EV_VEH_KILLED`,
`EV_BASE_CREATED`, `EV_BASE_KILLED` (ids above it shift down by one),
`EV_MAP_CHANGED`, `EV_GAME_LOADED`, `EV_GAME_SAVED`, `EV_NOT_IMPLEMENTED`
(raised by every stub with the function name) and `EV_MESSAGE`. The UI
installs its own sink; it must not keep `PopupRequest` string pointers after
the call returns.

## Saved games

```cpp
int game_read_bytes(const uint8_t* data, size_t length, SaveInfo* info = nullptr);
int game_write_bytes(std::vector<uint8_t>& out);
int load_daemon_bytes(const uint8_t* data, size_t length); // SaveLoadStatus
int save_daemon_bytes(std::vector<uint8_t>& out);
int load_daemon(const char* path);
int save_daemon(const char* path);
void upgrade_save_version();
const char* save_error_name(int error);
```

- Layout: `"TERRAN\0" 0x1A`, int32 0x56, Thinker `game_data()`, then the
  uint32 seed footer (optional on read). Each field is written little-endian
  one at a time, so 68k builds read and write the same bytes as x86.
- `game_read_bytes` decodes into a scratch copy of the bound state and
  commits it only on success, so a rejected file leaves the game unchanged.
  It applies Thinker's version conversions (v7..v10) and nothing else.
  `load_daemon_bytes` then applies the load-time fixups of
  `mod_load_daemon` (`fixup_landmarks`, `AltNatural`, `PFLAG_MAP_REVEALED`,
  `set_alive`, the cursor list, `rebuild_vehicle_bits`/`rebuild_base_bits`,
  `ControlTurnMove`, ...).
- Data versions 7..12 are supported. A save is written in its own version
  (v7..v10 with the inverse conversions), so a container round trip
  reproduces the file byte for byte, apart from the `field_23D20` word, which
  is written as 0 like Thinker, and map counts that `map_init` recomputes.
  `upgrade_save_version()` switches the state to v12 for writing.
- Rejected with an explicit `SaveError`: bad header, versions outside 7..12,
  encrypted (Ironman/network, `STATE_UNK_10000000`) and PBEM
  (`MRULES_UNK_20`) saves, out-of-range player, project base, record counts
  or map size, the Thinker tamper check, truncation and trailing bytes.

## What each package fills in

A package implements the functions listed in its section of `engine_wp.h`
in its own `engine/<topic>.cpp` files and deletes `stubs/wpN.cpp` in the same
commit. The build compiles every `.cpp` in `engine/` and `engine/stubs/`, so
no build edit is needed. New declarations go in the package's own header,
included from its sources (or added to `engine.h` with one line), not into
`engine_wp.h`, to keep merges conflict-free. Do not edit another package's
stub file; if a function is in the wrong section, move it in your commit and
say so.

| Package | Section in `engine_wp.h` | Notes |
| --- | --- | --- |
| WP1 turn scheduler | `control_turn`, `turn_upkeep`, `faction_upkeep`, `production_phase`, `repair_phase`, `next_player_turn`, `social_upkeep`, `rankings`, `reset_territory`, ... | `game_year` is already exact in the stub. |
| WP2 base upkeep | `mod_base_upkeep`, `mod_base_production`, `base_first`, `base_compute`, `mod_base_*` yields, psych, growth, riots, `mod_name_base`, `mod_capture_base`, `mod_cost_factor`, `facility_avail`, `find_relocate_base` | `base_init`/`base_kill` already call `base_first`, `mod_name_base`, `mod_cost_factor`, `base_compute`, `reset_territory`, `find_relocate_base`. |
| WP3 orders and movement | `order_veh`, `veh_speed`, `mod_hex_cost`, `zoc_*`, `goody_box`, `spot_*`, `veh_wake`, terraforming and altitude, `action_*` | `veh_init` calls `spot_all`. |
| WP4 combat | `mod_battle_compute`, `mod_battle_fight_2`, `mod_best_defender`, `psi_factor`, `promote`, `battle_kill*`, morale | The `psi_factor` stub returns its input. |
| WP5 research, social, design | `tech_*`, `mod_tech_*`, `social_calc`, `society_avail`, `make_proto`, `propose_proto`, `name_proto`, `veh_avail` | |
| WP6 computer players | `mod_enemy_*`, `enemy_strategy`, `mod_social_ai`, `mod_tech_ai`, `alien_fauna`, `veh_kill_lift`, `del_site`, `add_site` | `veh_kill` calls `veh_kill_lift`; the stub calls `veh_lift`. |
| WP7 new game | `world_generate`, `mod_world_build`, `find_start`, `mod_setup_player`, `setup_game`, `fixup_landmarks`, `world_linearize_contours` | `load_daemon_bytes` calls `fixup_landmarks`. |
| WP8 diplomacy and victory | `treaty_on/off`, `set_treaty`, `atrocity`, `encounter`, `enemy_diplomacy`, `end_of_game`, `mod_eliminate_player`, `is_objective` | |

## Building and testing

```sh
CXX=m68k-amigaos-g++ ./build_engine.sh out/program main.cpp   # Amiga 68040 flags
CXX=g++ CXXFLAGS="-O1 -g" ./build_engine.sh out/program main.cpp
python3 test_engine.py --out /tmp/engine-host                  # host, ASan/UBSan
python3 test_engine.py --cc m68k-linux-gnu-g++ --m68k-linux --out /tmp/engine-m68k
python3 test_engine.py --out DIR --original GAME.SAV           # real saves you own (not committed)
```

`test_engine.py` checks that `save_fields.inc` is current, runs the unit
checks, round-trips synthetic saves of every supported version (built by
`test_original_game.fixture`, with no original assets), compares the decoded
values with `test_original_game.decode`, checks that a `load_daemon` +
`save_daemon` cycle is stable, and requires every corrupted variant to be
rejected. It writes `receipt.json` to the output directory.
