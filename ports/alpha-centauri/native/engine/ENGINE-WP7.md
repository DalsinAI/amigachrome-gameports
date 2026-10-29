# WP7: new game and world generation

WP7 builds a new world and sets up the factions on the bound `EngineState`.
The code is Thinker Mod at `dabe62f2d1abba03de40f3dd3048494843f6a9af` (MIT,
see `THINKER-LICENSE`), ported line for line, plus new code for the
executable-only routines that no upstream has.

```cpp
std::unique_ptr<EngineState> s(new EngineState());
engine_reset(*s);
engine_bind(s.get());
config_vanilla(conf);
engine_load_rules("DH1:SMAC", 1);          // WP0b
NewGameSettings ng;                          // newgame.h
ng.seed = 12345; ng.player_faction = 3; ng.map_size = 2;
engine_new_game(ng);                         // map, factions, planetfall popup
```

## Files

| File | Contents | Source |
| --- | --- | --- |
| `mapgen.cpp`, `mapgen.h` | `world_generate`, `mod_world_build`, `world_climate`, `world_temperature`, `world_rivers`, `world_rainfall`, `world_analysis`, `world_rocky`, `world_riverbeds`, `world_shorelines`, `world_polar_caps`, `world_linearize_contours`, every `mod_world_*` landmark, `near_landmark`, `locate_landmark`, fungus placement | Thinker `mapgen.cpp` |
| `mapgen_terrain.cpp` | WP7-local `wp7::alt_set`, `alt_set_both`, `world_alt_set`, `minerals_at`, `bonus_at`, `goody_at` | Thinker `map.cpp` |
| `mapgen_path.cpp` | `map_wipe` | Thinker `savegame.cpp` |
| | `Path_continents` (Path::continents/continent/merge/make_abstract) | OpenSMACX `path.cpp` (GPL-3.0) |
| | `fixup_landmarks`, `world_site`, `GameHalted`, `WorldAddTemperature` | new |
| `setup.cpp`, `setup.h` | `find_start` with `find_reset`, `valid_start`, `apply_nutrient_bonus`, `apply_supply_pods`; `mod_setup_player`; `setup_game`; `init_world_config` | Thinker `map.cpp`, `faction.cpp`, `game.cpp` |
| `newgame.cpp`, `newgame.h` | `alien_start` | Thinker `game.cpp` |
| | `engine_new_game`, `NewGameSettings`, `planetfall`, `crash_landing`, `landing_site` | new |
| `wp7_deps.cpp`, `wp7_deps.h` | weak fallbacks for routines owned elsewhere (below) | Thinker or no-op |
| `lib/FastNoiseLite.h` | FastNoiseLite 1.0.1, MIT (licence in the file header) | vendored from Thinker `src/lib` |
| `../ui_newgame.cpp`, `../ui_newgame.h` | faction pick, settings and planetfall screens | new |

`engine_wp.h` entry points implemented: `world_generate`, `mod_world_build`,
`find_start`, `mod_setup_player`, `setup_game`, `init_world_config`,
`fixup_landmarks`, `world_linearize_contours`. `stubs/wp7.cpp` is deleted.
WP3's `wp3_deps.h` declares `world_climate` weak; `mapgen.cpp` defines it.

## Starting a game

`engine_new_game` follows Thinker `config_game(1)`, `map_menu` and
`control_game`: rules (`GameRules`, Blind Research on Citizen, the alien
faction rules), `FactionStatus[0]` and `MapWin->cOwner`, the planet size
(`WorldSizes`, the original `#WORLDSIZE` rows: 24x48, 32x64, 40x80, 50x100,
64x128, or a custom 16..512 size), the planet conditions (-1 = random, chosen
by `world_generate`), `map_init`, `mod_world_build`, `setup_game(0)`,
`alien_start` and `planetfall`. Every generator is seeded from
`NewGameSettings::seed` (`game_srand`, `random_reseed`, `next_rand_seed`), so
the same settings give the same game.

`planetfall(faction)` raises popup `PLANETFALL` once after setup;
`crash_landing(faction)` centres `MapWin` on the landing site and raises
`CRASHLANDING`. Both set `$STR0` title, `$STR1` leader, `$STR2` faction noun,
`$NUM0/$NUM1` the landing x/y. Integration wires
`turn_hooks().crash_landing = crash_landing` (WP1 `turn.h`).

## Screens (`ui_newgame.cpp`)

A state machine without drawing code: `newgame_open`, `newgame_key`
(up/down/left/right/enter/back), `newgame_view` (title and lines with the
selected row), `newgame_settings`, `newgame_done`. Screens: faction pick
(`MFactions[1..7]`), settings (difficulty, planet size, ocean, erosion, native
life, clouds, orbit, Look First, Blind Research, seed, Start) and planetfall
(captured from the `PLANETFALL` popup through a forwarding event sink). The
frontend draws `newgame_view()` with its own text renderer; wiring it into a
frontend window is not part of this package.

## Deviations (`Fix:` / `Original:` in the code)

- `mod_world_build` always runs `world_generate`: the original
  `world_build` (`conf.new_world_builder = 0`, the vanilla setting) is in
  no upstream source. Vanilla parity of map shapes is impossible.
- `world_generate` and start placement use the `wp7::` terrain copies
  instead of the WP3 `engine_wp.h` versions. Before the first turn Thinker's
  `alt_set` only stores the altitude, so the copies match WP3's behaviour
  and keep maps independent of WP3 landing; with `CurrentTurn > 0` they
  forward to the shared versions.
- `Path_continents` clears all region tile counts first (OpenSMACX keeps
  stale counts of unvisited regions and accumulates the pole regions across
  calls). As in the original, the ocean flood fill also counts the pole tiles
  it touches before they move to region 127.
- `fixup_landmarks` (exe-only, body unknown) removes landmark entries outside
  the map, terminates names and bounds the count to 64. A consistent table is
  left untouched, so load/save round trips are unaffected.
- `world_site` (exe-only, body unknown) rates the 21-tile radius of a land
  tile (dry/moist/rainy land, rivers, bonus resources) into 0..15; it only
  feeds the site bits written by `find_start`.
- `setup_game` clears the ten words from `SunspotDuration` to `ClimateLevel`
  like the original `memset`; that includes `SaveFileVersion`, which is then
  set to 12 with `upgrade_save_version()` (the original save code writes 12).
- Removed user-interface and network state: `prefs_*` (Alpha Centauri.ini),
  `wipe_undo`, `clear_all_offers`, `clear_hotseat_chat`,
  `clear_council_notify_2`, `clear_tamper`, `DiploStateB`,
  `net_game_close`, window redraws (`MapWin_clear_terrain`,
  `WorldWin_clear_terrain`, `do_all_non_input`; `draw_map(1)` is sent to the
  event sink instead), `console_world_generate`, DEBUG-only `setup_values`
  and the `mapdata` overlay.
- Thinker's local `GameRandom` objects carry their own state; here they get
  a local `uint32_t` backing store.
- FastNoiseLite: integer hashing, primed coordinates and the fractal seed
  increment wrap in unsigned arithmetic instead of overflowing `int`
  (undefined behaviour); results are bit-identical on two's complement
  targets.
- `debug()` format and three `snprintf` precisions are adjusted for
  `-Werror` (no behaviour change); two counters Thinker never reads are
  marked used for GCC 16.

## Weak fallbacks (`wp7_deps.cpp`)

Replaced at link time by the owning package's ordinary definition.

| Function | Owner | Fallback |
| --- | --- | --- |
| `reset_state` | WP6 | no-op (AI plan caches) |
| `clear_goals` | WP6 | Thinker `goal.cpp` body |
| `can_build_base` | WP6 | Thinker `move.cpp` body (no non-ally cache) |
| `clear_bunglist` | exe | no-op (bunglist kept as opaque saved bytes) |
| `consider_designs` | exe, WP5/WP6 | no-op: factions start with the basic units |
| `clear_monuments` | WP2 | zero `Monuments` |
| `agenda_on`, `set_agenda` | WP8 (exe) | set/clear `Faction::diplo_agenda` bits |
| `balance` | exe | `not_implemented`; never called with `skip_default_balance = 1` |
| `GameVictoryType` | WP8 | process-wide int |

Calls through `engine_wp.h` that are still stubs on this branch:
`tech_achieved`, `make_proto`, `name_proto`, `veh_avail`, `mod_tech_ai`,
`treaty_off`, `set_treaty`, `mod_time_warp`, `reset_territory`, and the
spotting and base calls inside `veh_init`/`base_init`.

## Floating point

`world_fractal` and fungus placement use `float`. On the 68040 GCC rounds
every `float` operation to single precision (`fsadd`/`fsmul`;
`FLT_EVAL_METHOD` 0 for `-m68040`), which matches x86-64 SSE, so seeded maps
are byte-identical on both. The tests check this on every run.

## Tests

```sh
python3 test_engine_wp7.py --out /tmp/wp7-host
python3 test_engine_wp7.py --cc m68k-linux-gnu-g++ --m68k-linux --out /tmp/wp7-m68k
```

Synthetic rules from `test_engine_wp0b.make_dir` (base and Alien Crossfire),
with the original `#WORLDBUILDER` defaults set in the test:

- unit checks: FastNoiseLite samples (bit pattern printed), `Path_continents`
  on a drawn map, `fixup_landmarks`, `world_site`, `setup_game` resets,
  rejected `NewGameSettings`;
- 8 map cases per variant over all five sizes and the planet conditions
  (including random): run twice, same bytes; different seeds, different
  bytes; every tile has a region; landmarks are on the map;
- 5 new games per variant: every living faction has a valid start (land for
  land factions, ocean for aquatic ones, not rocky, fungus or monolith, away
  from the poles, at least 4 tiles from other starts) and units or a base;
  a `PLANETFALL` popup; `crash_landing` centres the map; the save
  round-trips through `save_daemon_bytes`/`load_daemon_bytes` (only
  `PFLAG_MAP_REVEALED`, which `mod_load_daemon` recomputes, may differ; a
  second round trip is byte-exact);
- the screens driven by keys from faction pick to planetfall;
- with `--m68k-linux`, a host build runs the same cases and every map dump,
  save and screen output must be byte-identical to the m68k output.
