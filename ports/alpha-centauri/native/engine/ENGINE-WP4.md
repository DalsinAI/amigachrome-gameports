# WP4: combat

Port of Thinker Mod `dabe62f2` `src/veh_combat.cpp` (whole file) and the
combat part of `src/veh.cpp` (`battle_kill`, `battle_kill_stack`,
`battle_kill_credits`), MIT, see `THINKER-LICENSE`. Vanilla configuration
(`config_vanilla`).

| File | Contents |
| --- | --- |
| `combat.h` | Declarations, battle runtime globals, `BattleResult`, `BattleObserver`, `engine_attack`. |
| `combat.cpp` | `psi_factor`, `mod_say_morale2`, `mod_morale_alien`, `morale_veh` (Thinker `mod_morale_veh`), `mod_offense_proto`, `mod_armor_proto`, `get_basic_offense`/`get_basic_defense` (Thinker `mod_get_basic_*`), `find_defender`, `mod_best_defender`, `battle_fight_parse_num`, `terrain_defense`, `use_nerve_gas` (2), `add_bat`, `mod_battle_compute`. |
| `combat_fight.cpp` | `planet_busting`, `promote`, `interceptor`, `mod_battle_fight`, `mod_battle_fight_2` (odds, treaty prompts, nerve gas, hasty, difficulty, native handicaps, artillery bombardment, rounds, retreat, marine detachment, promotion, collateral damage, stack kills, base population loss and destruction, planetpearls, relics), `battle_kill`, `battle_kill_stack`, `battle_kill_credits`. |
| `combat_attack.cpp` | New: `engine_attack`, `engine_attack_odds`, `shoot`, capture of an undefended base, `battle_outcome_name`. |
| `stubs/wp4_deps.cpp` | Placeholders for functions combat calls that other packages own and `engine_wp.h` does not declare (see below). |
| `tests/wp4_test.cpp`, `tests/wp4_oracles.*`, `../test_engine_wp4.py` | Tests. |

29 Thinker functions ported, 5 new (`engine_attack`, `engine_attack_odds`,
`shoot`, the base-capture move, the observer/record plumbing).

## UI entry point

```cpp
#include "engine/combat.h"
std::unique_ptr<BattleResult> r(new BattleResult());   // ~5 KB
int outcome = engine_attack(veh_id, x, y, r.get());     // ATTACK_EXECUTE
int outcome = engine_attack(veh_id, x, y, r.get(), ATTACK_EXECUTE | ATTACK_INTERACTIVE);
int odds = engine_attack_odds(veh_id, x, y, &def_id);   // no state change
```

- Adjacent target: normal battle (land artillery units bombard, as in the
  original). Target 2..`artillery_max_rng` tiles away: artillery, if the
  unit can bombard. Empty enemy base next to a land combat unit: the unit
  moves in and `mod_capture_base` (WP2) runs.
- `BattleResult` carries the outcome, the chosen defender, strengths and
  morale as the status window showed them, the four modifier lines per side
  (`percent`, labels.txt id, text), hit points before/after, rounds and hits,
  kills, relics, collateral damage, planetpearls, promotions, retreat target,
  base destroyed/captured, and the ordered `BattleEvent` list (start, each
  round with both rolls, artillery hits, retreat, detachment, promotions,
  kills, `boom` animations, planet buster, base events). Vehicle ids in the
  result are the ids after the battle (ids above a removed vehicle shift).
- `set_battle_observer()` receives the same events live, for animation.
- `ATTACK_INTERACTIVE` sends BADIDEA/GOODIDEA, treaty and USENERVE prompts to
  the event sink (`popp`); a refusal gives `BATTLE_CANCELLED` and refunds
  the moves like the original.
- `Rules->move_rate_roads == 0` (rules not loaded) gives `BATTLE_NO_RULES`.

## Deviations

"Engine:" comments mark changes that only replace the executable or windows;
"Fix:" comments are Thinker's own and are kept.

- Windows, sound, network and tutorial calls (`BattleWin_*`, `StatusWin_*`,
  `veh_scoot`, `FX_play`, `clock_wait`, `TutWin_*`, `synch_veh`, `POP2/3`,
  `do_task` loop of the planet buster animation, `interlude`, `invasions`)
  are no-ops; `boom` and the status-window moments become `BattleEvent`s;
  `NetMsg_pop`/`popp`/`parse_*`/`draw_*` go to the core event sink.
- Thinker's DEBUG asserts against the executable's `offense_proto`,
  `armor_proto`, `defense_value` and `battle_fight_2` are removed.
- `VehBattleState` is a single word in `EngineState`; Thinker's write to
  `VehBattleState[1]` (never read) is dropped.
- The battle display globals (`VehBattleModCount`, `VehBattleModifier`,
  `VehBattleDisplay`, `VehBattleDisplayTerrain`, `VehBasicBattleMorale`,
  `VehDrawAttackID/DefendID`, `FactionCombatWin/Loss`) are module statics in
  `combat.cpp`, shared by all `EngineState`s (they are not saved).
  `VehAttackFlags` is the core global added by WP3 (cherry-picked commit).
  `VehBattleDisplayTerrain` is `const char**`.
- `label_get`/`Difficulty` are the rules loader's (WP0b); `label_id_of`
  maps a `label_get` pointer back to its labels.txt line for the UI.
- `shoot()` exists only in the executable. New: a human attacker in
  interactive mode is asked through `break_treaty`; otherwise a strike on a
  faction not at war is `act_of_aggression`.
- Base capture after clearing a base is done by `order_veh` in the original;
  `engine_attack` does it for an adjacent empty base. Move cost is
  `mod_hex_cost` (WP3), or `move_rate_roads` while WP3 is a stub.
- Kept as in Thinker although it looks odd: in a single-round weapon duel
  (land/sea artillery against a ship) the loop only stops after a hit when
  the battle is rendered (`render_battle`), so unseen duels run to the end.

## Core changes (own commits)

- `5a3921b` (cherry-picked from WP3 `955dcfa`): shared runtime globals,
  `VehAttackFlags` used here.
- `a03f198` `config_vanilla`: native life cycle table `{45,90,170,250,330,330}`
  (was `{...,250,0,330}`, which gave level 5 for turns 250..329; the original
  `morale_alien` gives 4, then 6 from turn 330). Found by the oracle test.

## Functions of other packages

Called through `engine_wp.h` (stubs until they land): `veh_speed`,
`proto_speed`, `mod_hex_cost`, `zoc_move`, `mod_goody_at`, `veh_skip`,
`spot_base`, `world_lower_alt` (WP3); `mod_capture_base`, `mod_base_reset`
(WP2); `treaty_on`, `atrocity`, `mod_eliminate_player`, `is_objective` (WP8);
`veh_kill_lift` (WP6, via `veh_kill`).

Declared in `combat.h`, placeholders in `stubs/wp4_deps.cpp` (the owner
deletes its entry): `break_treaty`, `act_of_aggression`, `cause_friction`,
`major_atrocity`, `un_charter` (WP8), `mon_enemy_destroyed` (WP2),
`add_goal` (WP6), `world_climate` (WP7).

## Tests

```sh
python3 test_engine_wp4.py --out /tmp/wp4-host
python3 test_engine_wp4.py --cc m68k-linux-gnu-g++ --m68k-linux --out /tmp/wp4-m68k
```

Differential oracles (must match exactly):
- `combat_rounds.cpp`/`combat_crt.cpp`: 300 random full battles (reactors,
  damage, weapons, armor, morale, terrain); rounds, hits, winner and final
  hit points from the same CRT seed.
- prebattle prefix modules (`origin/feat/alpha-owned-prebattle-prefix`, read
  from this directory when merged, else via `git archive`):
  `combat_psi_factor` (psi_factor), `terrain_defense`, `native_combat_morale`
  (mod_morale_alien), `basic_combat_strengths` (get_basic_offense/defense,
  both duel modes), `sensor_combat_adjustment`, `inspect_combat_difficulty`,
  `artillery_combat` (weapon duel stage). Cases where Thinker deliberately
  differs from the original are not compared: Children's Creche, Brood Pit
  and Headquarters morale (Thinker fixes), air defenders in
  `terrain_defense`.

Scenario checks: strengths and hasty modifier, odds preview without state
change, retreat, artillery bombardment and damage limits, collateral damage,
base population loss and destruction, base capture, best defender,
promotion, interceptor scramble, artifact capture, planet buster, psi combat
and planetpearls, invalid input.

Until WP3 lands the test builds a copy of `stubs/wp3.cpp` with Thinker's
`veh_speed`/`proto_speed` (the stub returns 0, which zeroes every offense).

## Not done

- No `ui_battle.cpp`: the UI part (battle animation and status panel) is
  left to the UI packages, which get everything through `BattleResult` and
  `BattleObserver`.
- Probe team actions (`probe.cpp`) are WP8-deferred; probes fight probes
  through the normal battle.
- No FS-UAE/AROS run yet: the Amiga cross-compile of all engine sources
  succeeds, but linking the engine test fails in the toolchain's libstdc++
  (`pthread_*` from `eh_globals.o`); not checked whether the core alone links.
