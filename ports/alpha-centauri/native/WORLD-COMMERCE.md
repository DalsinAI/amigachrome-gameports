# Original world commerce proposals

`world_commerce` takes a decoded original World, qualified same-file technology
Rules and an explicit edition-tagged eight-faction commerce bonus stage.
`fresh_world_commerce` instead obtains all eight bonus proposals through the
existing original technology-effects recomputation. It never reads saved
tech_commerce_bonus values or commits the recomputed counters. A malformed or
unsupported dependency in any of those eight fresh recomputations rejects the
fresh query. Neither entry point claims a completed original upkeep phase.

The result contains the selected base's zero-based rank, the loader's global
commerce-technology count, seven matching foreign base IDs (or -1), seven
imports and exports, total imports and an explicit branch-qualified marker.
This marker remains true if sanctions or aliens clear an otherwise qualifying
trade calculation; it records the original scratch initialization boundary.

Ranking and trade valuation use the original first energy field, independently
of fresh intake_2 and worker yields. Bases sort by first_energy*512+base_index,
so a higher index wins ties. The current base's rank selects the foreign base
with the same rank. The original saved faction base_count must be nonzero;
actual decoded foreign bases alone do not override a zero counter.

Either side's treaty bit 2 admits a trade link. Only the selected faction's pact
bit 1 suppresses the half-value discount. The trade denominator is one plus the
number of all 89 parsed technology rows having both prerequisites >= -1 and
commerce flag 4; this deliberately differs from faction ownership normalization
and includes Future Tech index 88 when the loader predicate permits it.
ProposalPassCount[3] > ProposalPassCount[4] doubles value. The raw governor ID
adds one to the corresponding side. Signed division truncates toward zero.
Original source arithmetic is performed before sanctions and, in SMAX, either
faction's alien flag clears both amounts. SMAC ignores those alien flags.

All executed signed32 arithmetic is checked, including score multiplication,
bonus+1, numerator multiplication/rounding, doubling, governor additions and
import accumulation. Original wrapping domains reject transactionally instead
of reproducing compiler-dependent overflow. Output must be disjoint from all
inputs, the World object, raw container and every decoded allocation, including
partial overlap. Input tables, world records, wire data and RNG remain unchanged.
No heap allocation is used by the query; rank selection uses bounded local
arrays for at most 512 bases.

No diplomacy change, income/accounting write, authority, persistence, upkeep,
turn, AI, renderer or frontend control is implemented. The explicit-bonus entry
point requires an actual coherent phase supplied by the caller. It does not
make saved counters authoritative for a live game. The fresh entry point is an
independent current-world proposal, not proof that original turn ordering and
all other income phases are restored.
