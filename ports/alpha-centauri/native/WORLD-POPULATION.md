# Fresh Population Stage

`world_population_begin` connects the existing split growth arithmetic to an
owned base-game human world. It recomputes food from actual worked tiles,
satellites and actor-order convoys; derives the current nutrient cost factor,
pending growth plus physical Creche/Golden Age, physical Hab Complex/Dome,
Ascetic Virtues, faction population penalty and Cloning Vats. Saved displayed
intake/surplus, cached growth and caller-provided nutrient totals are not inputs.
The Golden Age bit is read from the current staged world; this adapter does not
compute psych/drone status or establish that a previous phase refreshed it.

Growth arithmetic is the explicitly documented default **Thinker restoration
policy**, `induktio/thinker@dabe62f2d1abba03de40f3dd3048494843f6a9af`,
`src/base.cpp::mod_base_growth`, MIT. It is not claimed as a recovered complete
original growth routine. No mod option is imported. The objective predicate is
adapted from the complete original OpenSMACX `is_objective` (005AC060),
`b-casey/OpenSMACX@2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`,
Brendan Casey, GPL-3.0-or-later. It covers all-base, explicit event, secret
project and scenario facility objective branches. Nutrient accounting and cost
factors retain their separately documented recovered-original semantics.

The proposal reports population/storage, required recomputation, removal and
starvation supply-convoy cancellations. Once food exports exist, the pinned
source cancels all stationary home-base supply convoys physically on a base,
including other resource channels; exterior harvesters and moved convoys remain.
The actor bitset proposes changing order to NONE and changes no source state.

`world_population_finish(before, after, ...)` recomputes begin against `before`
instead of trusting a cached receipt. The caller must stage `after` privately
with exactly the proposed population/storage, the proposed convoy order
cancellations and a valid new worked mask. Every other decoded field and opaque
container byte must match. Fresh intake/convoys are then computed from this new
population and assignment. Ordinary growth adds the **new** population's
surplus; population boom applies the original pre-growth accumulation ceiling.
The old population world and old worker mask cannot complete a growth/loss
transition. Starvation preserves the pinned original assignment of pre-loss
surplus to storage while providing fresh post-loss resources for later stages.

Both operations are read-only and atomic on error, including alias errors and
late recompute failures. The explicit `full_turn` input belongs to the scheduler;
it does not grant persistent authority or bypass any existing worker-stage
qualification. Expansion, AI/governor allocation and worker upkeep1 remain
unsupported by this adapter. It currently uses the qualified inspection0
resource path. Base removal is an explicit event: it is not implemented by
silently setting population to zero in a live player world. Drone/revolt,
base-kill/faction elimination, UI notifications, faction aggregates, turn
scheduling and checkpoint persistence remain separate dependencies.

Run the connected host corpus with ASan/UBSan:

```sh
python3 ports/alpha-centauri/native/test_world_population.py --out /tmp/population-proof
```

Optionally add private original scenario and matching original rules using
`--original /path/Borehole.SC --original-rules /path/alpha.txt`. Every original
base is queried without renumbering engine inputs; only the independent oracle
renumbers its private copy. No original assets enter the repository.

Build the actual m68k HUNK probe using `build_world_population_probe.sh OUTPUT`
and the genuine `CXX=m68k-amigaos-g++`. CLI:

```text
world-population-probe RULES BEFORE.SC BASE_ID FULL_TURN [AFTER.SC|-] [upkeep|editor]
```

Its text row reports error; eleven derived growth facts; six transition fields;
required-recompute, starvation, cancellation count; six current food fields;
and sixty-four cancellation words. The host corpus independently derives wire
facts, intake, convoys, nutrient accounting, cost factors and growth outcomes;
it tests changed-population recomputation, stale state refusal, unrelated writes,
objective branches, shortage cancellation and input/output preservation.

The connected host corpus passed 244 worlds / 612 queries, including 92 fresh
post-change recomputations and all eleven original Boreholes bases. In genuine
FS-UAE/AROS 68040/FPU, 23 selected HUNK cases returned RC0 and matched the
independent oracle's complete output, including the two actual human bases.
The native receipt identifies both exact executables; the final expanded
alias-guard probe also passed all 23 cases in the actual guest. This is population-stage evidence,
not a completed campaign-turn or AmigaOS 3.2.3 qualification.
