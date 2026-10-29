# Original loader record repairs

`OriginalWorld::load_with_record_repairs` explicitly imports a private candidate
world and applies three recovered original save-loader repairs before swapping it
into the owning world. Failure preserves the previous world. `load` retains its
raw import behavior. The supplied technology table must be parsed from the
appropriate original rules; the API checks all 89 rows and prerequisite bounds.
The memory limit has the same per-candidate meaning as `load`; replacement may
briefly retain both the previous and candidate allocations.

The repairs are:

* Recompute faction 1–7 revealed-map flag `0x200` from nonzero satellite counts or
  an achieved technology with flag 8. Preserve Planet's record, future-tech 88
  exclusion and the original None/Disable prerequisite asymmetry.
* Recompute faction 1–7 alive bits from any owned base or a vehicle whose actual
  design has colony plan 8. Preserve alive bit 0 and other faction-status bytes.
* Rebuild vehicle occupancy, then base occupancy, and their tile faction nibble.
  First actor/base record wins at shared coordinates. Vehicle rebuilding tests
  the old base bit. Thus a stale base bit can suppress a vehicle faction refresh
  even when the following base rebuild clears it. Preserve that original order.
  Tile territory owner, visibility, site nibble and unrelated item bits survive.

The original map routines scan every tile against ascending actor records. The
owned implementation clears the relevant bits and applies records in reverse
order, producing the same first-record priority in linear time.

Original source evidence: Thinker `savegame.cpp::mod_load_daemon`, `map.cpp`
`rebuild_vehicle_bits`/`rebuild_base_bits`, MIT pin
`dabe62f2d1abba03de40f3dd3048494843f6a9af`; both original loader and helper
instruction streams were also inspected privately. Original code and assets are
not part of this repository change.

The test compares complete container projections against a separate tile-first
Python implementation, including versions 7–12, collisions, stale bits, signed
satellite values, technology prerequisites, immutable baseline bytes/RNG,
previous-world preservation on malformed input/rules and exact memory bounds.
The projection writer exists only in the test probe. It is not an engine save
API or an atomic file protocol.

This is a decoded-record loading prerequisite. It does not run original path
initialization, contour linearization, cursor/UI setup, runtime RNG restoration,
PBEM/network/tamper handling, turn admission or upkeep. Existing snapshot codecs
still reject unsupported preference/reveal changes. No player controls or
frontend are wired to this import; no playable-game or native execution claim.

## Local qualification

Host ASan/UBSan and an independent static big-endian 68040 executable both passed
291 worlds / 333 executed controls (297 successful projections, 36 rejected
loads), including three private owner tutorials with the owner's original rules.
The complete projected output hashes agree. Public synthetic controls cover
288 worlds; owner inputs and original-executable observation receipts stay private.

Both changed owned executables also match 96 bounded original observations per
edition: the actual original revealed-map/alive loader prefix and real complete
`has_tech`, followed by both original occupancy helpers with normal returns.
Complete eight-faction records, the alive byte and all controlled map tiles match;
all other mapped global bytes remain unchanged. The prefix stops before
ironman/file callbacks. These are controlled helper observations, not a complete
original loader, command loop or turn execution.

Both builds retain the raw importer regression's 67 passing controls. Two fresh
normal AmigaOS SDK builds are byte-identical, but have not executed on AmigaOS.
Remote CI and native qualification remain unrun; this local child is unpublished.
The existing frontend, watch preview and parent source branch are unchanged.
