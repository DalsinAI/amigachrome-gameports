# Owned human population transaction

`HumanUpkeepTransaction` stages the population and initial worker-assignment
prefix of human base upkeep in a private owned world. Its source world never
changes. This establishes a concrete mutation/rollback boundary for the future
full upkeep scheduler; it is not a public EndTurn implementation.

`start()` parses and owns matching rules, unit definitions and resource values,
and imports a supported base-game world for its saved current human faction.
`prepare_population(base, full_turn)` then:

1. Derives current food/convoys, world cost factor, growth limits, projects and
   objectives through `world_population_begin`.
2. Stages the proposed population, stored food and source-derived starvation
   convoy-order cancellations in a separate imported world.
3. Derives the actual stage1 human worker candidate from that changed world,
   including fresh tiles, support, convoys, satellites and current caps.
4. Applies the provisional worked mask. When it is a legal no-specialist full
   assignment, runs the split population finish against the actual before and
   after worlds, so post-growth food uses the new population and new workers.
5. Qualifies the narrow snapshot representation before retaining any candidate
   or phase result. A late worker/resource/serialization failure discards all
   private changes and leaves the transaction unprepared.

The existing worker helper explicitly requires later specialist optimisation
whenever it adds a worker, and later riot evaluation whenever that base can
riot. These events remain blocking even if a provisional assignment already
permits a fresh numeric food calculation. Unallocated workers and existing
specialists also block. Base removal is retained as an event; the transaction
does not fabricate base deletion or reference remapping.

The phases are `unprepared`, `base_removal_pending`, `specialists_pending`,
`riots_pending` and `population_checked`. The last means only that this prefix
has no remaining worker/riot event under the existing helper contracts. It
does not qualify a complete upkeep or turn: support projections, fresh full
psych/Golden Age ordering, commerce, economy/research ledgers, maintenance,
production, ecology, faction aggregation and AI still need their authoritative
phases. The current growth query uses its documented current-state Golden Age
fact; this transaction does not turn that into a fresh happiness result.

The public API exposes const source/proposed worlds and a const receipt.
There is no commit, phase override, event clear, mutable-world getter or year
advance operation. A prepared transaction refuses a second preparation,
including after resume. `full_turn` is an explicit engine scheduling fact,
persisted in the checkpoint; do not infer that fact from saved display fields.

## Narrow transport union

`encode_human_upkeep_snapshot` extends the existing qualified movement/order/
facility transport with population, stored food, worked mask, specialist count/
types, base economy/psych/labs totals and faction credit/research ledgers.
It seals every other decoded field, including year/turn and preferences, and
preserves the complete opaque container. It explicitly qualifies only base-game
legacy versions. This encoder grants no authority or original-engine save
compatibility; a pending transaction may contain an unresolved worker proposal.

The population prefix currently mutates only population, food, worked mask and
derived actor orders. Separate encoder controls qualify the specialist and
credit/research field offsets for later phases. Their transport support does
not mean this prefix has executed energy allocation or credited income.

## Deterministic checkpoint reconstruction

The custom `ACHUPK01` envelope contains a fixed 64-byte little-endian header,
the untouched source container and the exact private proposed container. The
header stores the payload size, runtime seed, exact rules CRC-32, source and
proposal CRC-32, whole-envelope CRC-32, selected base, scheduling fact and phase.
The whole-envelope CRC is computed with its own field zeroed. Reserved bytes
must be zero. This is not an original Windows save format or a defence against
hostile source edits/save rollback.

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 8 | `ACHUPK01` |
| 8 | 4 | Header size, 64 |
| 12 | 4 | Size of each of the two world containers |
| 16 | 4 | Runtime RNG seed, unchanged by this deterministic prefix |
| 20 | 4 | Exact rules CRC-32 |
| 24 | 4 | Source CRC-32 |
| 28 | 4 | Proposed-container CRC-32 |
| 32 | 4 | Whole-envelope CRC-32, this field zeroed during calculation |
| 36 | 4 | Base, or `0xffffffff` while unprepared |
| 40 | 4 | Explicit full-turn scheduling fact, 0/1 |
| 44 | 4 | Derived phase, 0..4 |
| 48 | 16 | Reserved, zero |

Resume does not trust a stored phase or a candidate's food/mask/credit totals.
It rebuilds the private transaction from the stored source and matching rules,
then compares the complete independently generated envelope with the saved
envelope before replacing an existing transaction. Correctly resealing an
invented phase or altered private payload cannot clear a pending event.
Algorithm/policy changes require an explicit checkpoint version/migration;
the envelope is tied to the current deterministic restoration policy.

## Verification

`test_human_upkeep_transaction.py --out NEW_DIRECTORY` strictly builds and runs
AddressSanitizer/UndefinedBehaviorSanitizer controls against independent wire,
population and worker models. LeakSanitizer remains disabled for the isolated
runtime's `/proc` restriction. The cases exercise actual private growth and
starvation changes, post-growth food recomputation, unresolved worker/riot/base
removal events, a failure after private population staging, exact source
preservation, repeat suppression, deterministic checkpoint resume and rejection
of checksum-valid phase/payload edits.

Separate transport controls verify population/specialist/credit/research fields
against the independent wire schemas for versions 7/9/10; forbidden coordinates,
social effects, turn changes and invalid population/worker/allocation values
leave the full output buffer and receipt untouched.

`build_human_upkeep_transaction_probe.sh OUTPUT` builds the same controls as a
strict 68040/FPU HUNK. Native execution receipts distinguish genuine AROS68k/
FS-UAE proof from host tests; neither qualifies real AmigaOS3.2 or full turns.

All 20 controls also executed as the strict native HUNK under genuine AROS68k
in FS-UAE 68040/FPU, each guest return code 0, with complete stdout and output
bytes matching the independent host oracle. Ten outputs are transaction
checkpoints and ten are narrow-encoder/control buffers; the latter include
intentionally unchanged sentinel buffers on rejection. Combined output length
is 4,460,980 bytes, SHA-256
`b9d20cf2c2687f0ad8a0feca54db28ee655029c6b88e765ee6e41a8810fd7723`.
See `HUMAN-UPKEEP-TRANSACTION-NATIVE-EVIDENCE.json` for per-case hashes and scope.
