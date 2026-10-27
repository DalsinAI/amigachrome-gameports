# Current Stage1 Accounting

`world_upkeep_accounting` produces one coherent read-only accounting proposal
for the current owned human base-game world. It calls `world_worker_resolution`
itself; callers cannot substitute an arbitrary worker result, mineral intake,
food surplus, support cost or saved displayed resource total. The resolver
recomputes real availability, current support/convoys, tile/satellite yields,
specialists, full mineral facts, nutrient accounting, energy allocation and
psych, including iterative worker trimming and proposed-mask ecology.

The result contains the complete final resolution and the original
energy-shortfall convoy cancellation proposal. A second output contains every
projected actor support state, in original world actor order. Support is
recomputed against the same immutable current world and its complete summary
must equal the resolver's support summary. Both outputs commit together only
after every upstream stage succeeds; short buffers, overlaps, unsupported
stages and late arithmetic errors leave both untouched. Any buffer tail beyond
the actual actor count remains intact; a zero-actor world accepts a null buffer
with zero capacity. No input is changed, including decoded live records,
original opaque bytes, rules, citizens or stage context.

This is actual stage1 accounting. Its final faction food-surplus and nine
hypothetical efficiency statistics are proposals computed once from the current
faction state. Intermediate resolver trials never accumulate those totals.
After a population transition, the scheduler must privately stage the **actual
new population** in the current owned world and call this API again. The worker
stage resolves the new assignment and specialist count and computes new food;
pre-growth population or surplus is never passed as a replacement scalar.

Commerce and energy-grid income remain explicit authoritative upstream trade
facts. The API does not generate trade, mutate research/credits or apply orders.
`resolved.requires_riot_event` remains an explicit event requirement; fresh
unrest is not silently cleared to permit a turn commit. Growth/removal, riots,
production events, faction aggregation and persistent turn authority remain
separate scheduler responsibilities. The worker/specialist/psych portions are
the documented pinned Thinker default restoration policy, while mineral,
nutrient and energy-shortfall components retain their separately documented
recovered-original semantics. This is not a public End Turn action or a claim
of completed campaign play.

Host proof:

```sh
python3 ports/alpha-centauri/native/test_world_upkeep_accounting.py --out /tmp/upkeep-accounting-proof
```

The independent resolved-worker oracle is composed with independently decoded
actor support states and the complete physical-base energy cancellation
predicate. The corpus includes changed populations, more than sixteen
specialists, iterative riot trimming, preserved pending riot events, stale
resource-display poison, real energy shortfall, signed overflow, source aliases,
short/null/crossed outputs and actor-buffer tail guards. Private originals may
be added with `--original /path/Borehole.SC --original-rules /path/alpha.txt`.
Only the independent oracle renumbers its private copy; its original global
base index is retained for bureaucracy psych. Engine inputs keep their actual
base indices, opaque contents and actor ordering.

Native HUNK build: `build_world_upkeep_accounting_probe.sh OUTPUT` with the
genuine `CXX=m68k-amigaos-g++`. The probe CLI is:

```text
probe RULES WORLD.SC BASE_ID COMMERCE ENERGY_GRID [inspection|editor|control]
```

`ALPHA_RESOLUTION_SOURCE_ROOT` (build) and `--source-root` (host test) optionally
select an isolated integration checkout; ordinary use takes all dependencies
from the same source directory. Original copyrighted assets are not committed.
