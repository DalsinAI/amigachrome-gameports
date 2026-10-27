# Vanilla Research Progression Proposals

`world_original_research_cost` computes the complete recovered original,
**uncached vanilla** human research-cost path. It never reads `tech_cost`.
Current historical ranking, deferred discoveries and the original ranking-debt
counter are gameplay state, not replaced by an inferred technology count. Actual
geometry supplies integer `quick_root(tile_count)`; the stored map-root cache is
not treated as fresh. Current difficulty, turn, base RESEARCH, discovery rate,
faction TECHCOST and stagnation determine the remaining factors.

The complete source is OpenSMACX `src/technology.cpp::tech_rate`, offset
005BE6B0, and `src/general.cpp::quick_root`, offset 005290E0, pinned at
`2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`, Brendan Casey(C)2013–2021.
`research_cost_reference.cpp` compiles the verbatim original cost body against
test globals with `tech_cost=-1`; every accepted leaf case compares to it on
both host and native builds. The original top-factor scan excludes faction 0.
Zero discovery-rate rules produce the original cost 999999999. Normal costs
clamp to 1–99999999. Malformed negative history/custom costs and overflowing
original intermediates reject atomically instead of wrapping.

## Contribution And Completion

The complete original `base_research` / `tech_research` bodies are not present
in OpenSMACX. Contribution and directed progress therefore use an explicitly
selected **vanilla-compatible restoration policy**, grounded in Thinker
`src/base.cpp:2619–2647,4054–4067` and `src/tech.cpp:181–240`, pinned at
`dabe62f2d1abba03de40f3dd3048494843f6a9af`, MIT. This deliberately uses the
original vanilla cost above, not Thinker's default revised cost. It retains the
original-compatible startup gates, not Thinker's enabled early-research default.
No mod-option knobs are exposed by these APIs.

`research_labs` converts only a fresh lab contribution, preserving accumulation.
`research_contribution` additionally advances an explicit running ledger against
a freshly computed cost. Behavior includes:

- Turn 0, no-tech-advances scenarios and tutorial messages through turn 5 skip
  research without modifying the ledger.
- The original negative-RESEARCH startup threshold and lowest-two-difficulty
  delay before turn 5 suppress contributed labs. Punishment Sphere halves labs.
- Effective RESEARCH clamps to -5…5. Integer fractional points carry through the
  actual 0…99 rounding-remainder ledger, with original operation order.
- Zero research points cannot complete a target, even if prior accumulation is
  already high. Positive points crossing the fresh cost complete one technology
  and discard excess accumulation, matching the source's reset to zero.

`world_base_research` is a **low-level explicit-stage boundary**. The caller's
fresh labs and running `{labs_total,rounding_remainder,accumulated}` ledger must
already be authoritative for this stage. Copying a stale displayed lab count
does not establish freshness. Saved lab/cost/accumulation/remainder fields never
substitute for those inputs. The `world_upkeep_research` composition below establishes internal lab freshness
without accepting such a scalar.

It requires the actual current player/faction's human base-game base and a
selected target. Original `tech_avail` prerequisite/ownership gates
(complete OpenSMACX005BAC20) are checked. AI/SMAX, repeatable
Transcendent Thought, expansion-only targets and missing targets reject. An
already-selected target advances under Blind Research through the same source
contribution/cost/ownership mechanics; completion emits an automatic next-target
event instead of a directed choice. No target is fabricated.

A pending FIRST_SECRETS flag is processed in the source order after a positive
contribution. Below cost it grants the current selected target while preserving
accumulated points; this is explicitly marked `bonus_current_target`. If cost
completion already grants that target, a pending or newly generated Secrets
bonus instead marks `bonus_next_target_required` for another real selection and
acquisition. A newly discovered Secrets technology can generate a future bonus
even when the current bonus was consumed. Those events remain explicit and no
second target is guessed.

Acquisition returns proposed owner bits, target reset, achievement/ranking/debt
counters and fresh technology fungus/commerce effects. The effects are rebuilt
with proposed ownership before bonus/clamp ordering, not incremented after an
already-clamped saved cache. First-discovery Secrets is an explicit event.
**Every completion requires acquisition and next-choice handling.** Prototype
design/upgrades, free facilities, map reveal, sharing/datalinks, bonus discovery,
notifications and further research selection are not silently applied or
declared complete. Nothing mutates the world, ledger, ownership, queues or turn.

## Verification

`test_research_progress.py` passes **8,142** ASan/UBSan cases against independent
integer models and the compiled complete original cost body. Coverage includes
all startup boundaries, difficulty/RESEARCH combinations, fractional carry,
Sphere, exact thresholds/excess loss, zero points, disabled research, custom
costs, map sizes, historical factors, malformed domains and late overflows.

`test_world_research.py` passes **326** ASan/UBSan connected wire cases, spanning
all supported import versions, real selected-target/prerequisite/ownership
gates, cost-history fields, no-tech/tutorial starts, bonus-secret gates,
acquisition/effects proposals, unsupported contexts and late counter/effect
overflows. Persisted lab/cost/accumulation/remainder fields are deliberately
poisoned. Probes check byte preservation of every wire/decoded/rule/stage region,
output sentinels on every rejection and aliases at multiple offsets.

Strict native builds use `build_research_probe.sh leaf|world OUTPUT`, 68040/FPU,
warnings as errors and no exceptions/RTTI. Actual native execution is recorded
separately after guest runs; build success alone is not native runtime proof.

```
python3 test_research_progress.py --out /tmp/alpha-research-leaf
python3 test_world_research.py --out /tmp/alpha-research-world
CXX=m68k-amigaos-g++ ./build_research_probe.sh leaf /tmp/research
CXX=m68k-amigaos-g++ ./build_research_probe.sh world /tmp/world-research
```
