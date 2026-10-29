# Original facility maintenance proposal

`facility_upkeep` restores the complete normal nonnegative-cost arithmetic
branches of original `fac_maint`004F6510 and `base_maint`004F65F0. Sources:
Brendan Casey's GPL OpenSMACX commit
`2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`, `src/base.cpp`,
`src/veh.cpp::best_reactor`, and `src/faction.h` for flag0x20.

The input is the actual post-income/pre-maintenance ledger, physical facility
bits, effective free-maintenance bonus bits, parsed facility costs, difficulty,
highest researched reactor, current production item and Self-Aware Colony
ownership. A caller may propose the resulting state, scrapped facility and
shortage notification bitmaps without mutating its owned world.

`world_facility_upkeep` derives physical facilities, current queue item, owner
difficulty, highest reactor from real technology prerequisites/ownership,
owned Self-Aware Colony project25 and the original FACILITY/FREEFAC bonus
rules. Credit balance, accumulated maintenance total, ledger flags and the
mineral cost factor remain explicit facts at the correct execution stage.
A saved economy figure is not fresh income; this wrapper never silently uses
one. Research, commerce, energy allocation and earlier upkeep remain separate
prerequisites. It does not confer turn authority or persist the proposal.

Preserved original details:

- Visit physical facilities in ordinal order. The original single-argument
  built predicate rejects repeatable IDs (base34, SMAX65), even though the
  outer source loop runs up to the first project (base39, SMAX70).
- Command Center maintenance uses the highest researched reactor capped by
  `(difficulty+1)/2`. This branch returns before faction free-facility checks.
  Thinker's later simplification to fixed facility-table maintenance is not
  substituted.
- Self-Aware Colony rounds half maintenance with the persistent faction
  flag0x20. A proposal returns the updated flag for the next base; do not reset
  it independently for each base or facility.
- Each charge enters the maintenance ledger before resolving a shortage.
  Difficulty0/1 or currently producing that facility protects it and clamps
  credits to0. Otherwise it is scrapped and credits are REPLACED with
  `facility_cost*mineral_cost_factor`; this is not an additive refund.
- Signed32 overflow, negative built-facility costs/maintenance, malformed
  context and overlapping storage reject without writing output. Negative
  maintenance/income mods and undefined original signed overflow are outside
  the declared safe domain.

`test_facility_upkeep.py` independently models sequential balances, scrapping,
Command Centers, free facilities, carry flags, novice/queued protection,
repeatable exclusions and error/alias preservation under ASan/UBSan.
`test_world_facility_upkeep.py` independently decodes wire/technology/faction
bonuses across original versions7..12 and checks that all owned state,
transient inputs, rules and RNG remain unchanged.

Build genuine 68040/FPU HUNK controls with
`CXX=m68k-amigaos-g++ sh build_facility_upkeep_probe.sh leaf|world OUTPUT`.
AROS compatibility evidence is separate from AmigaOS3.2 qualification and
from a complete base or faction turn.
