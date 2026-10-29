# WP2: base upkeep, production, growth and riots

WP2 ports the base turn of Thinker Mod (`src/base.cpp`, `src/monument.cpp`,
parts of `src/map.cpp`, `src/build.cpp`, `src/basewin.cpp`, `src/game.cpp`,
`src/faction.cpp`, `src/veh.cpp` at
`dabe62f2d1abba03de40f3dd3048494843f6a9af`, MIT, see `THINKER-LICENSE`) into
the engine. `engine/stubs/wp2.cpp` is removed. Base names follow the original
`name_base` as documented by OpenSMACX.

```cpp
for (int i = 0; i < *BaseCount; i++) {
    if (mod_base_upkeep(i)) { i--; }   // 1: the base was removed
}
```

## Files

| File | Contents |
| --- | --- |
| `base_upkeep.cpp` | Thinker `base.cpp` except `set_base`/`base_init`/`base_kill` (already in core `bases.cpp`): capture, reset/change/build, `base_compute` chain (`mod_base_support` -> `mod_base_yield` (worker allocation) -> `mod_base_nutrient` -> `mod_base_minerals` -> `mod_base_energy` (commerce, black market) -> `mod_base_psych`), `mod_base_research`, `mod_base_growth`, `mod_drone_riot`, `mod_base_drones`, `mod_base_maint`, `mod_base_ecology`, `mod_base_production` (unit births through `veh_init`, facilities, secret projects, satellites, stockpile energy), `mod_base_upkeep`, `mod_cost_factor`, `mod_base_making`, `mod_base_lose_minerals`, `facility_avail`, specialists, `hurry_cost`, `mineral_cost`, `stockpile_energy`, support helpers and the other base queries of Thinker's `base.h` |
| `base_actions.cpp` | `mod_base_hurry`, `hurry_item` (build.cpp), `action_staple` (basewin.cpp), player hurry (`base_hurry_quote`, `base_hurry_player`), multi-item production queue API (`base_queue_count/valid_item/append/insert/remove/move/clear`), `wp2_ui` |
| `base_yield.cpp` | `resource_yield`, `mod_crop_yield`, `mod_mine_yield`, `mod_energy_yield` (map.cpp) |
| `base_monument.cpp` | `monument.cpp`: `monument`, `clear_monuments`, all `mon_*` |
| `base_names.cpp` | `name_base`, `mod_name_base`, `make_base_unique`, `base_name_lists` |
| `base_globals.cpp` | The base compute globals (`BaseUpkeepState`, `BaseUpkeepDrawID`, `BaseGrowthRate`, `BaseForces*`, convoy/commerce/psych rows, `BaseTileFlags`, ...) |
| `base_deps.cpp`/`.h` | Weak copies of helpers owned by other packages (see below) |
| `base_wp2.h` | Declarations of all of the above |
| `tests/wp2_test.cpp`, `../test_engine_wp2.py` | Controls |

Functions ported: 77 in `base_upkeep.cpp`, 15 in `base_actions.cpp`, 4 in
`base_yield.cpp`, 18 in `base_monument.cpp`, 4 in `base_names.cpp`, 25 weak
helpers in `base_deps.cpp`.

## Deviations ("Fix:" in the sources)

- `mod_base_reset`: Thinker falls back to the executable's `base_reset` for
  some configurations; here it always uses `mod_base_build` + `mod_base_change`.
- `mod_base_hurry`: the executable's AI `base_hurry()` paths are not ported
  (return 0). Project ranking needs `facility_score`/`governor_priorities`
  (WP6) through the weak hook `wp2_project_hurry_score` (returns 0).
- `mod_base_psych` replaces every call of the executable's `base_psych`.
- `base_terraform` of the executable (AI former requests) is skipped.
- `base_radius`: the original AI goals at supply pods (`add_goal`) are skipped.
- `mod_name_base`: Thinker's `new_base_names` mode (basenames\*.txt) is not
  ported; both settings use `name_base`. Name lists come from
  `base_name_lists` (the rules loader does not read `#BASES`/`#WATERBASES`
  /`#GENERIC` yet; empty lists give "<noun> <n>").
- `make_base_unique` restarts its scan at base 0 after each rename (OpenSMACX
  restarted at 1).
- The base compute globals live in `base_globals.cpp`, outside `EngineState`;
  `engine_bind` does not swap them, `base_compute_state_reset()` clears them.
  They are scratch values recomputed by `base_compute`, except
  `SkipTechScreenB`/`GameVictoryType`, which are weak so the owning package
  can move them into the state.
- Window/sound/movie calls (`interlude`, `wave_it`, `amovie_*`, `help_*`,
  `Console_focus`, `GraphicWin_redraw`, `BaseWin_*`, `ambience`, blurbs of
  the `BasePop` dialogs, voice files) raise `EV_MESSAGE "ui:<what>"` through
  `wp2_ui`; popups go through `popp`/`popb`/`NetMsg_pop` (`POP2` -> `popp`).
- Assertions comparing against the executable (`base_making`, `crop_yield`,
  `proto_cost`, `veh_cost`, ...) are removed.
- `steal_tech` names the tech with `Tech[id].name` instead of `say_tech`.

## Weak helpers (`base_deps.cpp`)

Verbatim Thinker code, weak so the owning package's definition wins at link:
`bitmask`, `un_charter`, `global_trade_pact`, `victory_done`,
`full_game_turn`, `voice_of_planet`, `generators` (game.cpp); `has_wmode`,
`steal_tech`, `spying`, `council_votes`, `great_beelzebub`, `great_satan`,
`aah_ooga`, `climactic_battle` (faction.cpp); `steal_energy` (probe.cpp);
`satellite_count`, `satellite_goal`, `defender_count`, `garrison_count`,
`consider_staple` (plan/path/build.cpp); `prototype_factor`,
`mod_proto_cost`, `mod_base_cost`, `mod_veh_cost` (veh.cpp). Placeholders
that raise `EV_NOT_IMPLEMENTED`: `prune_protos`, `upgrade_prototypes`,
`mod_world_volcano`, `world_climate`; weak data `DiploFearScale`,
`DiploCanSurrender` (WP8).

## Tests

`python3 test_engine_wp2.py --out DIR` (host, ASan/UBSan) and
`python3 test_engine_wp2.py --cc m68k-linux-gnu-g++ --m68k-linux --out DIR`
(qemu-m68k -cpu m68040). Synthetic rules and maps, no game data.

- Differential oracles, 10000 randomized cases: `mod_cost_factor` vs
  `cost_factor.cpp`; `mod_base_psych` (Thinker default `Config`) vs
  `base_psych.cpp`; `mod_base_support` vs `base_support.cpp`;
  `mod_base_nutrient` vs `base_nutrients.cpp`; `mod_base_minerals` vs
  `base_minerals.cpp`; `mod_base_energy` vs `base_energy.cpp` (nearest HQ,
  fewer than 36 bases); commerce vs `base_commerce.cpp`.
- Behaviour: unit births (position, home, morale, command center, prototype
  cost and flag, colony pod population and ABANDONBASE), facility completion
  and queue advance, queue API, stockpile energy (and Planetary Energy Grid),
  retool penalties, secret project completion and other bases switched,
  ALREADYPROJECT, growth, population limits, pop boom, starvation and base
  death, drone riots, riot facility loss, golden age, nerve staple, hurry
  costs and player hurry, maintenance and POWERSHORT, capture/liberation and
  size-1 destruction, worker allocation and tile yields, specialists,
  monuments, base names; 30 upkeep turns over eight bases.

## Not done

- AI parts that depend on WP6 (`select_build`, `facility_score`) and WP5
  (`veh_avail`, `find_proto`) use their stubs: production choices of
  computer bases and queue validation of units are neutral until those land.
- `base_terraform`, the executable's `base_hurry`, Thinker's new base names.
- No FS-UAE run.
