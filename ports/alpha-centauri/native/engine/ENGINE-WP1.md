# WP1: turn scheduler and End Turn

Files: `turn.h`, `turn.cpp`, `turn_faction.cpp`, `tests/wp1_test.cpp`, `../test_engine_wp1.py`.
`stubs/wp1.cpp` is removed. Source: Thinker Mod `dabe62f2` (MIT, see `THINKER-LICENSE`).

## Ported functions (engine_wp.h WP1 section, all 16)

| Function | Thinker source | Notes |
| --- | --- | --- |
| `control_turn` | gameturn.cpp | Logic only, as the resumable `control_turn_run()` (below). |
| `turn_upkeep` | gameturn.cpp | Air fuel, rankings, turn and year counter, climate, fauna, fungal towers, alien arrival, Caretakers, random events, territory every 4th turn. |
| `faction_upkeep` | gameturn.cpp | social, repair, production, energy, diplomacy/strategy/AI hooks, corner market, hurry cost, elimination, research pick, council, autosave. |
| `repair_phase` | gameturn.cpp | Clears `moves_spent` (movement refill) and repairs. |
| `production_phase` | gameturn.cpp | Loans, base upkeep loop, AI maintenance discount, interest, first-turn free techs, sanctions. |
| `allocate_energy` | gameturn.cpp | Thinker version. |
| `next_player_turn` | gameturn.cpp | Network hand-over only; returns 0 outside network play. |
| `social_upkeep`, `social_upheaval` | faction.cpp | |
| `tech_research` | tech.cpp | |
| `rankings` | score.cpp | |
| `game_year` | game.cpp | |
| `do_fungal_towers` | gameturn.cpp | |
| `reset_territory` | new | exe-only (0x523DD0), see below. |
| `mod_time_warp` | map.cpp | Thinker path (`conf.time_warp_mod`); the vanilla exe `time_warp` raises `EV_NOT_IMPLEMENTED`. |
| `mod_auto_save` | savegame.cpp | Writes `Autosave_<year>.sav` in the current directory. |

Also ported: `random_events` (static): the global part (solar flares, dust cloud,
sunspots, AI betrayal reset, base event countdown, perihelion) and the choice of base
and event (`map_rand`). The 23 individual base events go to
`TurnHooks::random_base_event`; without it `random_events:base_event` is reported as
not implemented.

## End Turn API (turn.h)

```cpp
TurnResult r = engine_end_turn();
// r.status: TURN_HUMAN (r.faction_id's turn begins), TURN_ROUND_END (no human this
// round), TURN_GAME_OVER, TURN_STOPPED; r.turn, r.year; r.events (ordered).
```

The original `control_turn` never returns: it calls `Console_human_turn` (the
interactive map loop) for each human and once at the end of a round without a human.
Those calls are the return points of `control_turn_run()`. `ControlTurnMove` holds the
human whose turn is in progress, the same convention `load_daemon` uses, so a loaded
game resumes correctly: the next `engine_end_turn()` ends that human's turn and
continues with the following faction. On a new game (turn 0) the first call runs
turn upkeep (turn 1) and every faction up to the first human.

Order inside one turn: `turn_upkeep`, then for faction 0..7 if alive: hotseat switch
(human only), `faction_upkeep` (movement refill happens here, at the start of the
faction's own turn), then the human stop or `mod_enemy_turn` (WP6 entry point).

`TurnEvent` kinds: `TE_ENGINE` (any `engine_notify`), `TE_POPUP` (label and answer),
`TE_TURN_BEGIN`, `TE_TERRITORY`, `TE_FACTION_UPKEEP`, `TE_AI_TURN`,
`TE_HUMAN_TURN_END`, `TE_HUMAN_TURN`, `TE_ROUND_END`, `TE_GAME_OVER`. Events are
forwarded to the installed sink as well; the phase markers reach it as `EV_MESSAGE`
with `a` = the `TE_*` kind.

`turn_order_next(id)` gives the next living faction in turn order.

## Hooks (`turn_hooks()`)

Routines outside engine_wp.h are called through optional hooks; a null hook is
skipped: `crash_landing`, `world_climate`, `interlude`, `pact_ends`,
`can_call_council`, `call_council`, `plans_upkeep`, `probe_upkeep`, `move_upkeep`,
`mon_tech_discovered`, `energy_limit`, `random_base_event`, `consider_designs`.
Integration wires them once the owning packages land.

## WP1-local helper copies (`namespace wp1`)

`full_game_turn`, `victory_done`, `voice_of_planet`, `climactic_battle`,
`council_votes`, `aah_ooga`, `great_beelzebub`, `great_satan`, `reputation`,
`set_agenda`, `num_objectives`, `breed_mod`, `base_can_riot`, `fac_maint`,
`init_save_game`, plus `energy_limit` and `pop2`. They are Thinker bodies whose
files belong to WP2/WP8; kept in their own namespace so nothing clashes. Replace
with the owners' versions at integration. `_stricmp` comes from the WP0b rules
loader.

## Deviations

- `control_turn`: windows, hotseat password, PBEM save-and-quit, diplomacy and council
  notifications at the start of a human turn, and `FactionCombatWin/Loss` are left out.
  The hotseat switch keeps `cOwner`, the `SWITCHING`/`SWITCHING2` popup and the
  `earned_techs_saved` loop.
- `turn_upkeep`: Thinker's build-date stamp into the save is not written (keeps the
  saved bytes independent of the build). `MapWin_main_caption` becomes the
  `TE_TURN_BEGIN` event. Fix: climate counters use 32-bit wrap-around arithmetic.
- `production_phase`: Fix: the reset of `social_support .. unk_47` is done field by
  field instead of one `memset` over the native layout.
- `social_upkeep`/`social_upheaval`: Fix: SE fields copied explicitly instead of
  casting `&SE_Politics_pending` to `CSocialCategory*`.
- `allocate_energy`: Fix: division guarded when `rule_techcost` and the turn are 0.
  The human `INFRASTRUCTURE` popup no longer opens `social_select`; the UI acts on
  the answer.
- `repair_phase`: Fix: `sq` null-checked in the bunker/airbase test.
- `tech_research`: Fix: invalid arguments return instead of asserting.
- `rankings`, `num_objectives`: Fix: lower-bound checks on the turn and the facility index.
- `init_save_game`: Fix: `base_id_attack_target` bounds-checked.
- `faction_upkeep`: `reset_netmsg_status`, `Paths` reset and `do_all_non_input` are UI
  or WP3 state and are left out.
- `energy_limit` (exe 0x445440) is in no upstream source. Without a hook it returns 10
  (no limit); `allocate_energy` keeps its efficiency balance limits. Affects only
  computer players.
- `reset_territory` (exe 0x523DD0) is in no upstream source. It is rebuilt from the
  documented rule (conceptsx.txt #CONCEPT31): nearest base on the same continent or
  ocean (`base_find_3(x, y, -1, region, -1, -1)`, which Thinker patches into this
  routine), at most 7 tiles from a land base and 3 from a sea base. It also counts
  `region_territory_tiles`. Not checked against the original.
- `mod_auto_save`: path without `saves/auto/`.

## Tests

```sh
python3 test_engine_wp1.py --out DIR                                    # host ASan/UBSan
python3 test_engine_wp1.py --cc m68k-linux-gnu-g++ --m68k-linux --out DIR
```

- 200 unit checks on synthetic worlds with synthetic rules values set directly: year
  and turn counter, single human, all-computer, hotseat and eliminated humans, event
  order, movement refill, repair, loans, interest, hurry costs, territory, upheaval
  cost, rankings and might history, fungal towers, same-seed determinism, and
  save during a human turn, load, End Turn equal to the unsaved game.
- Every synthetic save of `test_engine.valid_cases()` (18) plus 4 with a human
  faction: 6 End Turn calls each, run twice with identical bytes and event digests;
  under `--m68k-linux` also a plain host build, whose saves and per-call output must be
  byte-identical to the big-endian run.

## Not done

- The 23 base random events (hook), council and diplomacy steps at a human turn start,
  network play, `ui_turn.cpp` (no UI in this package).
- No FS-UAE/AROS run: the m68k-amigaos toolchain in /opt/alpha-lab compiles the engine
  but its libstdc++ fails to link (`pthread_*` undefined), independent of WP1.
