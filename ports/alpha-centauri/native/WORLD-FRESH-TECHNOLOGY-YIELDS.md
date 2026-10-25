# Fresh technology effects in original tile accounting

`world_fresh_tile_yield_context` and `world_fresh_tile_yields` are additive
read-only queries. They retain the existing explicit edition, ExpansionEnabled,
terraform preview, current-base, control-upkeep and scratch contract. They reuse
the saved/owned binder for physical terrain, projects, economics and other facts,
then replace only the three fungus values with the complete checked
`world_technology_effects` result. The original `world_tile_yield_context` and
`world_tile_yields` remain byte-identical snapshot queries, including their saved
fungus-cache interpretation. Callers must choose the intended stage explicitly.

The fresh stage uses actual owned known-tech bits, normalized prerequisite
semantics, faction rule commerce and bonus records, and pending economy. It does
not infer a runtime edition from the save version or insert an expansion-only
technology exclusion absent from the original helper. Saved commerce/fungus
cache cells are never inputs to the final leaf calculation. Complete effect
recomputation is checked, so commerce or intermediate fungus-bonus overflow
rejects even if the current tile would not use that result. No saved cell,
original byte, actor, accounting field, revision, ownership or RNG is changed.

Complete output ranges must be disjoint from Rules, BasicRules, ResourceInfo,
the transient stage, World object, raw import and every decoded allocation.
Checked extent/product arithmetic rejects wrapping ranges. Both context and
result outputs remain untouched on failure, including partial tail overlaps.
The new source reuses the exact frozen106 technology-effect implementation;
the previous technology-effects alias correction is retained unchanged.

## Connected intake and convoys

The existing read-only stable-human intake and convoy-derived nutrient proposals
now call the fresh tile path. Their selected/harvested-fungus refusals are removed
only after private complete-original composition controls. The normal selected
mask, directed-human, no-specialist, no-worker-allocation and runtime-stage gates
remain. Incoming/outgoing transfer, actor order, resource-channel, scratch and
signed-overflow rules are unchanged. The initial convoy context propagates
fresh-stage overflow explicitly. These remain proposals, without actor support
flag commits, cache commits, worker reallocation, research awards or upkeep/turn
advancement.

Synthetic fixture definition commerce/bonus count fields, which the old tile
stage never read and left opaque, are now initialized to a real zero-commerce,
empty-bonus definition. Separate successful fungus controls use three real bonus
records and known-tech flags while poisoning all saved effect cache cells.
Invalid count and signed-overflow controls reject transactionally. This change
makes the new consumed facts explicit; it does not weaken a production guard.

## Proof boundaries

Independent Python reads the original-format wire fields, derives technology
effects separately, replaces the three explicit leaf inputs and evaluates the
existing independent original leaf model. Public synthetic controls cover save
versions7..12 and both runtime editions independently; querying faction differs
from physical base owner, and project/facility/terrain/preview/scratch facts vary.
Private controls add the owner's two actual rules tables and three base tutorials;
there is no original expansion save claim. Whole World/rules/wire/RNG fingerprints
and partial-alias rejection are checked. The old snapshot suite remains required.

Additional private controls execute unchanged complete original `tech_effects`,
`base_support`, `base_yield` and nutrient helpers in that order for64 base and64
expansion contexts. Both selected and exterior convoy tiles are fungus. The
contexts vary population0..5, physical facilities, satellites, positive/negative
fungus bonuses and seven real supply actors. All seeded globals and selected base
bytes match the independent model, including the five refreshed cache cells and
original actor-state/support effects. The current scratch page is captured; its
convoy aggregates are independently compared, while unrelated scratch bytes are
not claimed fully modeled. Original Manifold project33 is explicitly absent in
these controlled composition cases; broader leaf/binding project coverage is
separate. No original bytes/assets or original-derived vectors are distributed.
The owned result is compared directly to the corresponding original proposal
fields, not described as execution of a complete original turn.

Normal SDK builds are separate from actual AmigaOS execution. Local independent
Linux68040/QEMU is additional proof, not final-head GitHub CI or native AmigaOS.
No UI, filesystem save protocol, deployment, discovery transaction or full-game
completion is introduced. The user's running watch lab is untouched.
