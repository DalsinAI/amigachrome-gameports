# Human Worker Upkeep Candidate

`world_worker_upkeep_candidate` accepts the actual human upkeep stage 1 and
proposes current worker assignments and fresh resource intake. It does not
mutate the world or advance a turn. Existing original inspection-stage APIs
remain unchanged, including their rejection of upkeep stage 1.

The scoring is the initial greedy part of Thinker's `mod_base_yield`, pinned at
`dabe62f2d1abba03de40f3dd3048494843f6a9af` (MIT; see
`THINKER-LICENSE.txt`). The complete original worker allocator is unavailable.
This is a documented restoration policy, not original-engine parity and not
the complete Thinker governor. It preserves valid directed assignments, removes
unavailable/high bits, resets empty or overassigned assignments, then selects
freshly scored tiles with the original tile-index tie break.

The connected adapter derives the real population, pending sliders, actual
facilities/projects and caps, physical HQ distance, fresh support/pacifism,
fresh convoys, current satellite counts and all available tile yields.
Fungus queries recompute current technology/faction effects. Saved intake,
surplus and support totals are not material inputs. It recomputes selected
tile intake and ecology after choosing the proposed mask, rather than using a
saved total or treating the score's approximated mineral balance as accounting.

For directed human physical availability this policy explicitly reuses the
qualified read-only inspection radius restrictions, without claiming the
original upkeep branch's AI goals/bookkeeping are implemented. AI, SMAX,
editor mode and active citizen governors reject.

The candidate stops before specialist substitution and drone optimisation.
`unallocated` workers are not silently converted to specialists. Any existing
specialists, remaining workers or newly chosen workers produce an explicit
specialist-resolution requirement. Every base capable of riots produces an
explicit riot-resolution requirement. A caller must finish fresh citizen
allocation and psych/drone resolution before applying a turn. No persistent
stage authority, End Turn control, unit movement refill or faction/year
advance is supplied by this component.

Validation: 6,015 independent integer-model leaf cases passed under ASan/UBSan
and in a genuine 68040/FPU HUNK run under FS-UAE/AROS. The 438,458-byte native
output is identical to the host oracle, SHA-256
`bfedd5aba014d976270eb613ddad8c1ae15100dd1b64f0f9798d88c234da18ff`.
The first native attempt exposed unsupported `%lld` parsing in genuine libnix;
the final probe uses `%ld`, and the failed attempt is retained separately in
runtime evidence. It was not counted as a passing calculation.

The connected independent wire/radius/tile/support suite passes 56 worlds,
including 50 accepted current-stage proposals, preserved assignments, missing
and overassigned workers, malformed high bits, specialists, insufficient tile
capacity, zero population, empty mask, stage/editor/governor/SMAX rejection,
fresh fungus and technology overflow. Every input region, output sentinel and
alias boundary is checked. All 56 connected worlds also passed in the genuine
68040/FPU FS-UAE/AROS guest, each RC0 with complete outputs identical to the
independent host oracle. The 43,239-byte aggregate SHA-256 is
`2bb51f98ab3acac6e53fe09449b4a8e4bad37aaf0ef48c5c964a6fedb1ae258c`;
the final native receipt records the exact binary and each case separately.

Run `test_worker_allocation.py` and `test_world_worker_upkeep.py` for host
validation. Native build scripts are `build_worker_allocation_probe.sh` and
`build_world_worker_upkeep_probe.sh`, using the OpenAmigaGCC 68040/FPU toolchain.
