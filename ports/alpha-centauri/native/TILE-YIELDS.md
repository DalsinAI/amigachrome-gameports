# Original fresh tile-yield boundary

`calculate_tile_yields` computes original nutrient, mineral and energy leaves
from a decoded tile and explicit current-stage facts. It returns values and
the original terraform-counter/base-energy side effects as data. It neither
mutates a world nor allocates workers or advances production, growth or turns.
Saved base intake is deliberately absent from the input.

This is a checked numeric boundary for a **base-relative** query. It does not
emulate negative base-index memory reads. `Variant::base` and
`Variant::alien_crossfire` select edition rules; the base profile rejects an
expansion-enabled context. The actual runtime expansion flag is explicit.
Original file data version is not a substitute for it.

The caller must provide fresh facts from the same authoritative stage:

- The decoded tile and actual resource bonus0..3. `tile_resource_bonus` derives
  the latter from bounded coordinates, map seed and GameRules without using or
  advancing the world RNG. It includes generated placements, explicit resource
  overrides, deep-ocean exclusion and the original signed coordinate shifts.
- Physical facility bits, not queued facilities; current pending Planet and
  Economy, fungus yields, base count, Golden Age and base event flags.
- Actual technology eligibility for each three-resource limit and the sea
  mineral improvement. No technology defaults are guessed by this API.
- Manifold and governor ownership for the querying faction; Merchant Exchange
  location for the queried base; Aquatic rule flag for the **base owner**.
- Dust-cloud state, solar-flare mask, upkeep selector, and adjacent Echelon
  Mirrors owned by the original CurrentBase owner. The mirror count is bounded
  to8; this interface does not guess that owner or walk a world map for callers.
- Nine resource triples in original table order: ocean, base, bonus, forest,
  recycling, improved land, improved sea, monolith and borehole. Some original
  channels are intentionally unused by these leaves.
- The incoming terraform counter and incoming base-energy scratch value.

Ordinary importing or inspecting a saved world does not establish these live
facts. No controller or frontend calls this new boundary yet.

## Original behavior retained

These calculations follow the two original instruction paths, rather than
later mod changes. Important examples are the following:

- A nonzero configured borehole nutrient value adds the bonus nutrient value
  even without a nutrient resource bonus. This is the original conditional.
- Soil Enricher and then Condenser each add signed-truncating half of the
  current nutrient value. Their rounding is sequential.
- Mineral mining includes roughness twice in its original branches, a separate
  road-dependent improvement limit, and a later technology/resource limit.
- SMAX has Manifold, Aquatic, Aquafarm, Subsea Trunkline, Thermocline and
  expansion-enabled deep-sea branches that SMAC does not have.
- Mineral/energy fungus ignores terraform preview; nutrient fungus does not.
- Original monolith energy still passes the ordinary energy limit. Its nutrient
  and mineral leaves bypass their respective ordinary limits.
- Base energy at pending Economy3/4 preserves the original arithmetic, rather
  than the later corrected mod formula. Base energy scratch is clamped before
  events, dust and solar-flare multiplication; the returned energy can differ.
- Fungus energy bypasses ordinary river/resource/landmark/economy/project
  additions, but still receives the original later weather adjustments.

Each original addition/multiplication must remain representable as signed32.
The API rejects overflow, even if a later clamp would make the final number
small. Unsupported metadata, aliases and overflow leave the entire result
unchanged. The result contains separate reduction deltas and the combined
counter, so callers need not infer or replay side effects from saved totals.

## Evidence and limits

Private bounded execution used the owner's two unchanged executables, with
real original bonus/bitmask/technology helpers and real original adjacency
tables. No renderer/chooser/game callback was replaced or stubbed. No PE startup
or Windows imports/full game were run.

The owned sanitizer build matches **62,420** independently observed original
leaf cases:6,960 nutrient,12,000 mineral and12,250 energy contexts per edition.
Each leaf value and its terraform-counter delta match; the energy base scratch
field also matches. Those controls use explicit faction1/base0/position2,2.
Separate original bonus helpers match the owned coordinate implementation in
**36,400** cases per edition, including widths up to4096, negative transformed
coordinate shifts, arbitrary map seeds and all resource overrides.

Additional private joint observations call the actual original nutrient,
mineral and energy leaves consecutively with the complete context, comparing
all three values, final counter, base scratch and each counter delta. Contexts
with unsupported signed overflow or a physically inconsistent supplied bonus
are excluded from that original comparison, rather than counted as passes.

Public controls use only owned generated data and independent checked-integer
Python models. `test_tile_yields.py` currently executes66,626 contexts
(66,587 accepted,18 invalid,21 overflow), including mixed resource triples,
base/event effects and pre-clamp overflow. `test_tile_bonus.py` executes36,415
contexts (36,400 accepted,13 invalid,2 aliased). Each probe checks transactional
failure; the yield probe also checks direct input/output alias rejection.

The normal Amiga SDK probes compile. Actual AmigaOS execution and final-head
independent big-endian CI remain unrun at this source checkpoint. No worker
allocation, support/convoy aggregation, world stage binding, production
completion, research, upkeep, AI, combat or playable-turn claim is made.
There is no player input/save command or filesystem commit protocol here.
