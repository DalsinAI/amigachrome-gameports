# Original research cost query — host-only preparation

`original_research_cost` is a bounded, read-only implementation of the original
`tech_rate` threshold calculation. Despite its original name, **this is the lab
points needed for a discovery, not the lab points generated per turn**. The original formula is faction-level: it has no target-technology ID argument.
Thinker's revised cost is a separate target-dependent formula. This query has
no callbacks, network hooks, technology grants, RNG or world mutations.

`world_research_cost` binds qualified saved fields directly and derives the
original `MapAreaSqRoot` from the owned tile count using recovered integer
`quick_root`. No runtime default or approximate floating-point square root is
substituted. A separate owned context API is available for explicit snapshots;
an unknown map-root context fails when calculation needs it, but cached/disabled
shortcuts do not read unused inputs.

## Sources and fidelity boundary

Public source: OpenSMACX `technology.cpp::tech_rate`, `general.cpp::quick_root`,
`faction.cpp::is_human`, `map.cpp::map_init`, pin
2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c (Brendan Casey, GPL-3.0-or-later).
Thinker `tech.cpp::mod_tech_rate` and `savegame.cpp` at
 dabe62f2d1abba03de40f3dd3048494843f6a9af were inspected for comparison.

Original signed arithmetic was checked privately against unchanged original
`tech_rate` and `quick_root` helpers from the owner's exact supported executable.
Only bounded helpers ran under host qemu-i386: no PE startup, Windows imports,
full game or discovery side effects. Proprietary code/data remain private.

Two source subtleties matter:

* OpenSMACX's source uses unsigned declarations in several intermediates, while
  the original instructions use signed comparisons/division. This implementation
  follows original signed safe-domain behaviour, verified against actual helpers.
* Original maximum rank scans faction slots **1–7**, excluding Planet0.
  Thinker's comparison scans0–7 and uses 64-bit products as an overflow fix.
  We preserve the original scan and reject unsafe signed32 arithmetic instead of
  claiming Thinker's widened result is original behaviour. In private controls,
  raising Planet0's rank to one million leaves faction1's original cost unchanged.

No revised technology cost, `tech_balance`, rate modifier or early-research-start
patch is used.

## Formula and saved dependencies

The source checks saved `tech_cost` first: every nonnegative cached value,
including zero, is returned unchanged. Discovery rate0 then returns999999999.
Otherwise the original sequence is retained, with signed truncation at every
integer division:

1. Clamp `2*earned_techs_saved - unk_27 + tech_ranking` to2..9999 and halve it.
   Find the maximum `2*earned_techs_saved + tech_ranking` across factions1–7 and
   halve that maximum. The original unknown field is used as saved, without
   inventing a broader semantic name or recomputing it from ownership.
2. Obtain human status from `FactionStatus[0]`. Human difficulty is the saved
   faction `diff_level`; AI difficulty is saved global `DiffLevel`. Difficulty
   below3 receives the original +1 adjustment. Human and AI base factors differ.
3. Apply the original rank-relative factor clamp, current-turn penalty and
   maximum-rank catch-up reduction. Stagnation uses turn divisor12 rather than8.
4. Multiply by the rank factor adjusted by saved **SE_research_base**, clamped
   −1..1. This is not pending SE research (which affects per-base lab production).
5. Apply RULES discovery percentage, faction definition `rule_techcost`, original
   integer map-area square root /56 and stagnation's additional half cost.
6. Clamp the final computed cost to1..99999999.

Original signed32 intermediate overflow is rejected transactionally. Difficulty
used by calculation is restricted to original levels0–5; malformed contexts,
negative map root or out-of-range qualified discovery percentage fail. Cache
and disabled shortcuts intentionally do not validate unused calculation inputs.

The query does not reset the cached cost. Original cost changes are state-machine
work: a discovery clears accumulated points, sets cost−1 and recomputes it.
Returning the stored cache rather than silently recalculating is deliberate.

## What still separates this from a research turn

Per-base research input is produced by base energy/labs accounting. Original
research-stage dependencies include scenario no-tech-advances, Punishment Sphere,
startup lab delay, pending SE research and the saved fractional carry
`net_random_event`. Thinker's `mod_base_research` can change startup delay;
`labs_total` alone is not the final points passed to `tech_research`.

Then `tech_research` can apply a climactic-battle AI bonus, update accumulated
points, select research, call `tech_advance`/`tech_achieved`, trigger secrets/free
advances and discovery notifications, synchronize multiplayer and reset cost.
Thinker changes some of these branches and offers revised tech costs. None are
implemented or stubbed by this cost query. PR86's mechanical eligibility query
also does not implement the human `tech_pick` offered-choice filter.

## Controls and current qualification

Public tests provide deterministic generated contexts, overflow/invalid cases,
cache/disabled short circuits and10003 integer-root boundary controls. Owned-world
binding tests independently decode the original wire fields and exercise legacy
and current layouts with human/AI, cached/computed and stagnation contexts.
Private tests add three original base tutorials. An original expansion save is
not claimed; the current-layout binding contexts are synthetic.

Normal SDK raw/world probes build, but have **not run on AmigaOS**. This work was
explicitly host-only while the user's preview exclusively owned the lab: no lab
reads, inputs, writes, start, stop or restore were performed. Independent big-endian
CI and native qualification remain future gates. Nothing is wired into gameplay
or deployed, and this branch must not be merged as native-qualified on host tests
alone. Source changes and private controls are prepared for the next integration.
