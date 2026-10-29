# WP5: research, social engineering and unit design

Files: `tech.h`/`tech.cpp` (research), `social.cpp` (social engineering),
`design.h`/`design.cpp` (unit design), `wp5_deps.h`/`wp5_deps.cpp` (weak
helpers from other packages), and `tests/wp5_test.cpp` with
`../test_engine_wp5.py`. `stubs/wp5.cpp` has been removed.

The sources are Thinker Mod `dabe62f2` (MIT, see `THINKER-LICENSE`), ported with
the vanilla configuration (`config_vanilla`). Routines that exist only in
`terranx.exe` are written fresh from the documented rules.

## Ported from Thinker (nearly line for line)

- **`tech.cpp`:**
  - `tech_str`, `say_tech`, `tech_level`, `tech_category`, `tech_avail`, `tech_effects`
  - `mod_tech_selection`, `mod_tech_pick`, `mod_tech_rate`, `tech_is_preq`
  - `tech_advance`, `tech_achieved`, `tech_alt_cost`, `tech_alt_val`
  - `steal_tech`, from `faction.cpp`
- **`social.cpp`:** `compute_faction_modifiers`, `social_calc`, `society_avail`.
- **`design.cpp`, from `veh.cpp`:**
  - `transport_val`, `prototype_factor`, `mod_proto_cost`, `mod_base_cost`, `mod_veh_cost`
  - `mod_upgrade_cost`, `mod_upgrade_prototype`, `part_upgrade`, `full_upgrade`
  - `veh_avail`, `create_proto`, `mod_propose_proto`, `mod_make_proto` (plus the `make_proto` int wrapper), `mod_name_proto`
  - `best_armor`, `best_weapon`, `best_reactor`, `proto_offense`, `proto_defense`
- **`design.cpp`, from `build.cpp`:**
  - `find_proto`
  - `unit_is_better` and `unit_score`. Both are weak because WP6 owns `build.cpp`.

## Written fresh (exe-only)

- **`tech_pick`.** Behaviour depends on who is choosing:
  - **Human, normal research:** the human gets the original directed pool. That is the `tech_avail` candidates, ordered by category (growth, tech, wealth, power) and then by id. After the first offer, an id is withheld when `(id - tech_ranking + faction) % 3 == 0`. Ids 0..6 are exempt from this while `tech_ranking == 0`.
  - **Computer players, blind research and turn 0:** these use `mod_tech_ai`, which is WP6.
  - **Trade or steal (`other >= 0`):** the list is every `tech_avail` tech that the other faction owns. A human chooses from it. A computer player takes the best `mod_tech_val`.
  - **How the human choice is made:** through `set_tech_chooser`. Without a chooser, the choice is an event-sink popup: the label is `TECHPICK` (or `ACQUIRETECH`/`STEALTECH`), the answer is a list index, and -1 cancels.
  - **Pool helpers:** `research_pool` and `tech_tradeable` expose the lists for the UI.
- **`tech_rate`.** This is the original cached cost. It returns `tech_cost` while that is nonnegative, and otherwise uses the Thinker vanilla formula. `mod_tech_rate` calls it whenever `revised_tech_cost` is off.
- **`tech_selection`.** Calls `mod_tech_selection`.
- **`research_change`.** This is the manual target change from the research screen. It follows the evidence on the manual-research-choice branch:
  - The player must confirm when there is a target and the labs are nonzero.
  - A confirmed change halves the labs, truncating toward zero.
  - The cached cost is kept.
- **`social_set`.** Applies the pending choices to `SE_*_pending`, runs `tech_effects` and recomputes the faction's bases.
- **`social_select`.** This is the human Social Engineering screen, run through `set_social_chooser`. Without a chooser it shows the `SOCIALSELECT` popup and keeps the current choices.
- **`social_change`.** Pays the upheaval cost against the choices the turn started with. `SE_upheaval_cost_paid` records what has been paid, so changing again in the same turn charges or refunds only the difference.
- **`propose_proto`:**
  1. A design with identical components that the faction can already use is reused (the obsolete mark is cleared).
  2. Otherwise the first free slot is used. If there is none, an own obsolete prototype with no units or queue entries is replaced.
  3. The new design gets the given name (or `name_proto`) and the most advanced component tech as `preq_tech`.
  4. The plan is set from `ai_plan`; `PLAN_PROTOTYPE_DONE` marks the design as already prototyped.
  5. `UNIT_UNK_80` is set so the tech screen can offer to show the new design.
- **`name_proto`.** Uses Thinker's naming rules for every design; the original naming code is not available. Missiles are named "weapon chassis". No reactor suffix is added.
- **`retire_proto`.** Marks the design obsolete for the faction. An own design with no units and no queue entries is deleted.
- **`prune_protos`.** Retires designs that the new unit is better than (`unit_is_better`) when they have the same triad and role. For humans this only happens with the auto-prune preference set, or when `flag` is set.
- **`upgrade_prototypes`.** Offers `mod_upgrade_prototype(..., flag=1)` for each own or basic unit type with vehicles that the new design improves on. It then clears `UNIT_UNK_100`.
- **`upgrade_any_prototypes`.** Runs `upgrade_prototypes` for each design flagged `UNIT_UNK_100`.
- **`consider_designs`.** This is the automatic design step when a tech is gained. It uses the best weapon, armor and reactor:
  - Infantry defender, attacker and combined unit.
  - Fastest land attacker, formers and probe team.
  - Sea combat and transport.
  - Air attacker.
  - A design is skipped when an available design with the same chassis already covers it.
- **`design_new_veh`.** Raises `EV_MESSAGE "design_new_veh"` so the UI can open the workshop.
- **`scan_prototypes` and `scanned_prototypes`.** For diplomacy: lists the other faction's prototyped designs that this faction could build but has no equal for.
- **`say_stats_2`.** Writes "A-D-M" text.

## Deviations (`Fix:` / `Original:` in code)

- Multiplayer and PBEM waits and network messages are removed.
  - `tech_advance` keeps only the PBEM deferral.
- Popups, sounds, movies and windows go to the event sink:
  - `X_pop`/`popp`/`NetMsg_pop`, `EV_MESSAGE "ambience"`, `"interlude"`, `"design_new_veh"`.
  - Thinker's tech popup and new-tech window become one `X_pop_2(PopupScriptFile, WEDEVELOP/...)` with `numbers[0] = tech_id`.
- Asserts on arguments are replaced with range checks.
- `tech_pick` never returns a tech that cannot be researched. While `tech_ai` is a placeholder, it falls back to the first pool entry.
- A `-1` result from a reactor-upgrade proposal is skipped.
- A cancelled upgrade popup (-1) declines the upgrade.
- `social_calc` returns `int` (always 0), matching the `engine_wp.h` signature.
- The `DiploWinState` check is gone; the engine has no diplomacy window.

## Interfaces for other packages

- **Weak in `wp5_deps.cpp`, so the owner's strong copy wins:**
  - From WP2: `mod_bases_reset`, `unit_support_plan`, `base_can_riot`, `mon_secrets_of_tech`, `SkipTechScreenB`, `dword_90EA3C`.
  - From WP1 and WP2: `full_game_turn`.
  - From WP6: `need_police`.
- **Strong in WP5, replacing WP2's weak placeholders:** `steal_tech`, `prototype_factor`, `mod_proto_cost`, `mod_base_cost`, `mod_veh_cost`, `prune_protos`, `upgrade_prototypes`.
- **Strong in WP5, replacing WP3's weak placeholders:** `tech_is_preq`, `mod_veh_cost`, `mod_upgrade_cost`.
- **`SkipTechScreenA`** is defined in `tech.cpp`.

## Tests

```sh
python3 test_engine_wp5.py --out /tmp/wp5-host
python3 test_engine_wp5.py --cc m68k-linux-gnu-g++ --m68k-linux --out /tmp/wp5-m68k
```

The unit checks use synthetic rules and cover:

- Availability, levels, categories and effects.
- The pool against an independent model, chooser and popup picks, and the trade list.
- Manual changes, discovery side effects, and Datalinks, TECHSHARE, free-facility, map-trade and prototype-trade propagation.
- Advance and steal.
- Social effects with every faction rule, social changes and refunds, and the social screen.
- `make_proto` and `propose_proto` slot handling, naming, costs, upgrades (AI, human popup, partial), pruning, automatic designs, reactor upgrades, `scan_prototypes` and `find_proto`.

Oracles:

- **Prototype cost and `make_proto` derivation** (cost, carry, plan, role) are checked against `test_prototype.cost/derive`: 3,584 cases, both variants.
- **The original research cost** is checked against `test_research_progress.cost_expected`: 4,000 cases.

## Not done

- `ui_research.cpp`, `ui_social.cpp` and `ui_design.cpp`. There is no `ui_*` panel registry yet; the engine exposes chooser hooks and events for them.
- No FS-UAE run.
- The original exe-only naming and `consider_designs` choices cannot be reproduced exactly.
