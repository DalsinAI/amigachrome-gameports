# Persistent construction segment

`FacilityUpkeepSession` owns the imported world and its parsed technologies,
unit definitions, resource values and rules. Its only state-changing operation
calls `FacilityProductionStage::apply_fresh_materials`: current support, worked
tile/convoy minerals and the world-derived cost factor supply construction.
It exposes const world inspection, never mutable world storage.

The session records each accepted base exactly once. A waiting construction
counts as processed too, so saving/reloading cannot repeatedly credit the same
segment's mineral surplus. Completed construction records both resource
recomputation and production-choice requirements. These events survive resume
and have no public clear operation. Failed applications do not set any ledger
bit or change the world or caller receipt.

This closes the previous per-object construction ledger boundary across
checkpoint/resume. It does **not** implement a complete turn scheduler, fresh
starvation/riot evaluation, worker reassignment, AI, unit production or an
EndTurn action. Construction still has its existing base-game human, single
queue, Creche/Recycling and inspection-stage restrictions. In particular the
current production stage reads starvation/riot state under its documented
limited material-freshness contract. The session does not promote that contract
to complete fresh upkeep.

`start()` explicitly begins a new construction segment from a raw supported
container and refuses an already open session. Continue a saved segment using
`resume()`. No API resets processed bits, clears pending events or advances the
turn. The future authoritative turn engine must own segment creation and event
completion; raw imports do not prove that another engine has not already run
upkeep. Explicit save rollback remains possible, as in ordinary savegames.

## Checkpoint format

The custom `ACFUPK01` envelope has a fixed 256-byte header followed by the exact
current world encoded with `encode_facility_production_snapshot`. This retains
qualified movement/orders/construction changes and all opaque container bytes.
It is not an original Windows save-compatibility certification.

All integers are little endian. CRCs use the standard IEEE CRC-32 polynomial.
They detect accidental corruption; they do not authenticate a hostile host.

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 8 | `ACFUPK01` |
| 8 | 4 | Header size, 256 |
| 12 | 4 | Payload size |
| 16 | 4 | Runtime RNG seed |
| 20 | 4 | Exact input rules CRC-32 |
| 24 | 4 | Encoded payload CRC-32 |
| 28 | 4 | Entire checkpoint CRC-32 with this field zeroed |
| 32 | 4 | Base count |
| 36 | 4 | Current human faction |
| 40 | 4 | Construction segment phase, 1 |
| 44 | 20 | Reserved, must be zero |
| 64 | 64 | Processed bases, low bit first |
| 128 | 64 | Resource recomputation pending |
| 192 | 64 | Production choice pending |

Resume validates the complete envelope before replacing an open session.
Every set processed bit must refer to a real currently owned base. The two
pending masks must agree and be subsets of processed. Pending construction must
have a supported facility queue, the completion flag and the actual built bit.
Payload length, rules identity, human ownership and base count must agree.
Failed parse, checksum, semantic-ledger or allocation checks leave any existing
session intact. Checkpoint encoding uses a temporary buffer so errors leave the
caller output unchanged; overlapping output/world/session buffers reject.

## Validation

Run `python3 test_facility_upkeep_session.py --out NEW_DIRECTORY` from any
directory. It strictly compiles and uses AddressSanitizer/UndefinedBehaviorSanitizer
against independently assembled wire-level expected checkpoints. LeakSanitizer
is disabled because the isolated runtime cannot inspect `/proc` threads.

The 39 cases cover legacy versions 7/9/10, industry costs, waiting and completed
construction, resume/repeat suppression, unqualified worker/upkeep branches,
unavailable facilities, exact whole-container preservation, short and aliased
outputs, wrong rules, constrained-memory resume and invalid ledgers with correct
checksums. The first host case damages every header byte plus eight payload
samples; other cases damage four representative bytes. Every case also checks a
checksum-valid inconsistent ledger against an already loaded session.

`build_facility_upkeep_session_probe.sh OUTPUT` builds the same probe as a
68040/FPU HUNK with strict warnings. The native probe samples four corruption
positions per case rather than repeating the full host header sweep.

Native controlled completion, waiting and rejected-upkeep cases additionally
compare complete checkpoints against the independent host wire oracle. Their
receipts record actual emulator execution separately from host validation.
