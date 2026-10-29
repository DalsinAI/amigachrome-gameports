# WP8: diplomacy, Planetary Council, victory and score

Files: `diplo.h`, `diplo.cpp`, `diplo_event.cpp`, `victory.cpp`, `council.cpp`,
`wp8_deps.cpp`, `../ui_diplo.h`, `../ui_diplo.cpp`, `tests/wp8_test.cpp`,
`../test_engine_wp8.py`. `stubs/wp8.cpp` is removed. Source: Thinker Mod `dabe62f2`
(MIT, see `THINKER-LICENSE`), vanilla config. `transcending` follows the OpenSMACX
reconstruction. Code that exists only in terranx.exe is new and listed below.

## Ported functions

engine_wp.h WP8 section (all 12): `treaty_on`, `treaty_off`, `set_treaty`, `atrocity`,
`get_mood`, `encounter`, `enemy_diplomacy`, `diplomacy_check`, `end_of_game`,
`mod_eliminate_player`, `transcending`, `corner_market`.

Other Thinker functions in the same files (declared in `diplo.h`; several are
the strong definitions of weak placeholders in WP2/WP3):

| File | Functions | Thinker source |
| --- | --- | --- |
| diplo.cpp | `agenda_on/off`, `set_agenda`, `major_atrocity`, `break_treaty`, `intervention`, `double_cross`, `act_of_aggression`, `spying`, `council_votes`, `eligible`, `great_beelzebub`, `great_satan`, `aah_ooga`, `climactic_battle`, `at_climax`, `cause_friction`, `reputation`, `get_patience`, `energy_value`, `get_pact*`, `get_his_her/him_her/he_she`, `mod_wants_to_attack` | faction.cpp, faction_diplo.cpp |
| diplo.cpp | `un_charter`, `global_trade_pact`, `victory_done`, `full_game_turn`, `voice_of_planet` | game.cpp |
| diplo_event.cpp | `pact_attacks/unpact/truce/treaty`, `enemies_war/unpact/team_up/trade_tech/treaty`, `comm_check`, `territory`, `action_sat_attack` (weak) | faction_event.cpp, basewin.cpp |
| victory.cpp | `get_rating`, `num_objectives`, `most_objectives`, `compute_score`, `generators`, `vulnerable`, `mod_mind_control` | score.cpp, game.cpp, probe.cpp |

## New code (exe-only routines)

- `pact_ends(f1, f2)`: clears `DIPLO_PACT|DIPLO_HAVE_SURRENDERED` on both sides
  (the treaty stays). Each side's units in the other's bases or stacks go to their
  home base, else to the nearest own base (`veh_put`), else are removed
  (`veh_kill`, highest id first). Raises `EV_MESSAGE "pact_ends"`.
- `ascending` (= `voice_of_planet()`), `trade_maps` (shares visibility bits and calls
  `synch_bit`), `communicate` (calls `DiploHooks::communicate`; headless it records
  the contact and clears `DiploStateA`).
- Comm-link proposals: `diplo_proposal_valid/accepted/apply` for truce, treaty,
  pact, end pact and vendetta. Acceptance runs `diplomacy_check(f, f2, 1)` and uses
  patience (`diplo_patience` against `get_patience`), mood, hostility, Great Satan,
  revenge, reputation and goodwill. It draws no random numbers.
- Minimal Planetary Council (`council.cpp`):
  - `can_call_council` requires:
    - the council is not disabled and the game is not over;
    - comm links with at least half the other voting factions;
    - `min_turns_between_councils` since the faction's last call.
  - `council_proposal_valid` checks:
    - `Proposal[].preq_tech`;
    - the per-proposal interval;
    - the first session elects the Governor;
    - per-proposal state, for example the Supreme Leader needs `RULES_VICTORY_DIPLOMATIC` and `eligible`.
  - Elections have two candidates: the proposer and the eligible faction with the
    most votes.
    - Governor: the candidate with more votes wins.
    - Supreme Leader: the winner needs 3/4 of all votes. That gives diplomatic victory
      (`VIC_DIPLOMATIC_SOLO/COOP/LOSS`, `STATE_VICTORY_DIPLOMATIC`).
  - Other proposals pass with more yes than no votes. Their effects:
    - Unity Core: +500 credits to every faction.
    - Solar shade and polar caps: climate change.
    - Trade pact and U.N. Charter: tracked through `ProposalPassCount`.
  - Computer players vote by their relation to the proposer or candidate. They vote
    for a Supreme Leader only if that faction is a pact ally (in a cooperative game,
    or after surrendering).
  - Humans vote through `X_pop_2("council", "ELECT"/"VOTE")`.
  - `call_council` lets a computer player call the council for the Governor, or for
    Supreme Leader when it predicts a win.
- `install_wp8_turn_hooks()` sets WP1's `turn_hooks()`: `pact_ends`,
  `can_call_council` and `call_council`.
- `ui_diplo.cpp`: a platform-free model of the proposal dialog. It shows the mood
  name and the open proposals, and gives an answer for each one.
  `install_ui_diplo(chooser)` routes `communicate()` through the dialog.

## Deviations ("Fix:" / "Original:")

Thinker's own `Fix:` comments are kept as they are. This port adds:

- Bounds checks that reject invalid faction ids, where the original indexed
  blindly:
  - `treaty_off`, `atrocity`, `major_atrocity`, `cause_friction`;
  - `encounter`, `enemy_diplomacy`, `diplomacy_check`;
  - `compute_score`, `mod_eliminate_player`, `pact_ends`.
- `diplo_event.cpp`:
  - `soc_category` is bounded.
  - A divisor is guarded when `AI_fight` is 2.
  - A guard covers `plr.ranking == 0`.
  - A base off the map is skipped.
- `energy_compute` (exe-only) is replaced by `energy_compute_net`, which uses the
  energy surplus.
- `compute_score`:
  - When the table pointer is null, the 9-entry table aliases `unk_110..unk_118`
    through an accessor. This is the same behaviour without indexing past a struct field.
  - The secret project and Ascent base ids have upper bounds.
- `end_of_game`:
  - A full 9-entry table is passed in the scenario score path.
  - `winner_id == -1` is guarded.
  - `subspace_gen_req > 0` is required.
  - An unset `EndingMissionYear` (0) means no time limit.
- UI (`Original:`):
  - These go to the event sink as `EV_MESSAGE "ui:<what>"`:
    - movies, credits, the score screen, Hall of Fame and the replay;
    - the war ambience and caption redraws.
  - Interludes go through `turn_hooks().interlude`.
  - The first-contact tutorial popups are dropped.
  - `net_treaty_on` becomes `treaty_on`.
  - `report_score` is replaced by computing every faction's score.
  - `end_of_game` returns 1 so that WP1 stops with `TURN_GAME_OVER`.
- `set_treaty` leaves out the `counter_espionage` branch, which is off in the
  vanilla config; probe teams are deferred.
- The diplomacy runtime globals (`DiploFriction`, `DiploStateA`, `CouncilProposals`
  (Thinker `CouncilProposal`), `GameVictoryType`, ...) live in one static block in
  `diplo.cpp`, not in `EngineState`. They are not saved, as in the original.
  `diplo_runtime_reset()` clears them.

## Dependencies on other packages

- WP8 calls WP2–7 only through `engine_wp.h`.
- `wp8_deps.cpp` provides weak fallbacks that raise `EV_NOT_IMPLEMENTED`:
  - `BaseCommerceImport`, `mon_winning_unify`, `mon_killed_faction` (WP2);
  - `contiguous` (WP3);
  - `scan_prototypes`.
- Victories set by other packages:
  - Conquest is set in `mod_setup_player` (WP7).
  - Transcendence is set in base production (WP2).
  - WP8's `end_of_game` and `compute_score` act on both.
- Integration note: WP4's `stubs/wp4_deps.cpp` defines `break_treaty`,
  `act_of_aggression`, `cause_friction`, `major_atrocity` and `un_charter` as
  strong, non-weak symbols. These clash with WP8 at link time and must be dropped
  when WP4 and WP8 are merged; WP4 says it will do this.
- WP1's own unit test (`test_engine_wp1.py`, synthetic worlds where factions own
  only scouts) relied on the WP8 stub. With the real `mod_eliminate_player`,
  `faction_upkeep` eliminates factions without bases or colony pods, as the game
  does, so 18 of WP1's 200 movement-refill checks fail. WP1's worlds need a base
  or colony pod per faction; WP1 files are not changed here.

## Tests

`python3 test_engine_wp8.py --out DIR` (host ASan/UBSan) and
`python3 test_engine_wp8.py --cc m68k-linux-gnu-g++ --m68k-linux --out DIR` (qemu-m68k
68040, plus a plain host build whose output must match). Synthetic states only. Checks:
- treaty state changes;
- mood thresholds and friction;
- atrocity, major atrocity and the U.N. Charter;
- proposals and the dialog model;
- `pact_ends` moving units home;
- Governor elections, Unity Core and the interval;
- victories:
  - Supreme Leader: solo win, failed vote and loss;
  - economic: loss and solo win;
  - conquest score;
  - transcendence and the Ascent bonus;
- score components, native life, Ironman and the time limit;
- 45 End Turns with the WP8 hooks (a computer player calls the council), same seed
  twice.

## Not done

- Probe teams.
- The full council: vetoes, the other computer-player proposals, buying votes, and
  the original council dialogs.
- The original diplomacy dialogs: trades, tribute, loans, tech or base swaps.
- Energy loans in proposals.
