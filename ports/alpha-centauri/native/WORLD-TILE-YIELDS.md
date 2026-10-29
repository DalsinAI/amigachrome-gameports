# Owned-world original tile-yield binding

This layer connects the original numeric tile leaves to `OriginalWorld` records.
`world_tile_yield_context` returns the explicit facts, and `world_tile_yields`
calculates nutrient, mineral, energy and the returned counter/scratch effects.
Neither writes the world, saved accounting or runtime RNG.

## Inputs and stage boundary

The caller supplies matching parsed technology/basic/resource rules, a base,
querying faction and tile coordinate, plus `WorldTileYieldStage`. Runtime variant
and ExpansionEnabled are explicit; save format version does not identify the
running game. Terraform preview, ControlUpkeepA, prior reduction and prior energy
scratch are transient facts. `current_base` is required only when the original
land-solar branch needs Echelon Mirror adjacency. It is never guessed from the
queried base or loaded save.

The binder reads the tile's real map-seed resource bonus, physical facilities,
Golden Age and events; querying-faction pending Planet/Economy, saved fungus
counters and base count; base-owner Aquatic; normalized technology ownership;
Merchant Exchange project ordinal 3 and SMAX Manifold Harmonics ordinal 33; and
saved governor/weather state. Querying faction, queried-base owner and current
base owner can differ. Echelon ownership uses the actual current-base owner,
eight source offsets and the map's horizontal wrap/bounds. The centre is excluded.

Saved fungus counters are explicitly snapshot state. They are not a substitute
for the separate fresh technology-effects stage after ownership changes. This
API does not refresh those caches or claim a complete worker/upkeep pipeline.
Saved post-calculation base intake is never fed into a fresh tile calculation.

## Validation and ownership

Loaded world shape, bounded coordinates/parity, base/faction references, technology
prerequisites, relevant project references and required current-base state are
validated. Original signed 32-bit arithmetic checks remain in the leaf API.
Unusable output aliases into the complete world object, original container, any
decoded arrays, rules/resources or transient stage are rejected. Both output
objects remain unchanged on failure. No heap allocation occurs in this layer.

The existing normalized `world_has_technology`/`world_has_project` helpers and
frozen original tile leaves are reused without changing their implementation.
Original leaf behavior and attribution remain in [TILE-YIELDS.md](TILE-YIELDS.md):
OpenSMACX GPL-3.0 source and private original instruction/helper comparisons.
The binding's record derivation is checked independently against the explicit
little-endian save schema; no proprietary tables or container bytes are included.

## Controls and remaining gates

The probe parses the source rules/save, poisons and frees the input buffers, then
compares every explicit context word and result word against an independent
Python raw-wire reader plus the existing independent leaf model. Controls span
formats 7 through 12, both explicit editions, flat/cylindrical maps, distinct
faction roles, resource overrides, event/project/technology state, invalid
coordinates and CurrentBase, empty worlds and deliberate mirror boundaries.
Startup controls check five invalid stage/rule contexts per loaded world and
input/output aliases for each nonempty world. A within-process fingerprint
checks original wire, decoded records, rules and runtime RNG after the query
batch; no native structure bytes are written to the output protocol.

Normal SDK compilation is separate from actual AmigaOS execution. Independent
big-endian CI and native execution must pass at the final frozen artifact before
qualification. There is no frontend, worker selection, fresh base aggregation,
support/convoy stage, production/growth, research advancement, turn, AI or player
save protocol in this change.
