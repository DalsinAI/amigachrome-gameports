# Load-repaired world snapshot buffer

`encode_initialized_snapshot` extends the frozen phase buffer encoder for worlds
loaded with the explicit original record repairs. It additionally writes:

* The exact recomputed alive faction-status byte 1.
* The exact recomputed revealed-map player flag `0x200` for factions 1–7;
  Planet's corresponding bit remains its original value.

Both values are recomputed from the immutable original container and the supplied
qualified same-input 89-row technology table. A caller cannot choose different
values. The remaining preference fields, faction flags and inherited phase
allowlist retain all the previous validation. Map/actor/order/accounting storage
continues to use the existing structural scope, without implying legal gameplay
transitions.

A private world projection restores the two additions to their original baseline
values while the unchanged phase encoder validates and writes every inherited
field. Only after it succeeds are the independently checked additions written
with explicit byte/little-endian encoding. No fallible operation follows those
writes. Other opaque bytes, inactive fields and the original footer remain intact.
The runtime RNG is returned separately.

Two private world allocations coexist during validation; their combined recorded
memory is bounded by the caller's limit. The caller World/output and inherited
stack-validation scratch are excluded. Both projections own their allocations.
No cast writes to the caller World or its baseline. Output and receipt aliases
into any World allocation or the supplied Rules reject before writes.

Tests compare complete buffers with an independent Python model, reload and
compare decoded state, re-encode identically, and continue with another edit.
They retain the prior movement/order/worker/accounting/phase-field cases and
rejection controls, and add exact repair mismatches, other status bytes, invalid
Rules, Rules aliases and the two-world memory boundary. Test-only mutations are
wire-format controls, not engine phase commits or player actions. Private owner
tutorial files are exercised with an explicit controlled technology table; these
are serialization tests, not claims about original scenario gameplay.

This is buffer persistence only. It does not implement filesystem atomicity,
O_EXCL, a player-save UI, event delivery or acknowledgment, phase admission,
turn advancement, full original load/runtime setup or executable compatibility.
The current frontend and user watch preview do not instantiate this encoder.

## Local qualification

Final host ASan/UBSan and independent static big-endian 68040 builds each passed
461 executed controls over 11 worlds (versions 7–12, empty worlds and three
private original owner tutorials): 107 complete exports, 354 rejections and
31 continued exports. Complete independently constructed output hashes agree.
Decoded reload, identical re-encoding, continued edits, exact two-world memory
bounds, full caller World/Rules preservation and inherited codec seals pass.

All 336 parent C++/header files remain byte-identical; the prior workflow is
retained verbatim with two added validation commands. Two fresh normal AmigaOS
SDK builds are byte-identical. They are build-only: native execution and remote
CI are unrun. No new original-executable execution is claimed for this wire
extension; original repair semantics retain their separate parent evidence.
