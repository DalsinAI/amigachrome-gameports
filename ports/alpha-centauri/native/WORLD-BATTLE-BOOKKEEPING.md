# Owned current-world battle bookkeeping frame

`stage_world_battle_bookkeeping` binds the qualified numeric before/after-cleanup
transition to actual current World faction matrices and monument bytes. It clones
all current decoded records, immutable original container and rebased views,
current monuments, movement RNG and the current 4,096-byte history. It writes only
combat_total/count and six monument fields in the clone; source and prior output
remain unchanged on failure. The raw imported monument span remains the baseline.

Facts explicitly supply the original pre-cleanup attacker/defender faction IDs,
loss branch, runtime wins/losses, current player, multiplayer/PBEM profile and
starting year. CurrentTurn, faction rule_flags and combat matrices come from the
current owned World. Faction IDs are not recovered from actor indices after dense
compaction. No default runtime authority or starting year is invented. Explicit
edition must match the supported import layout profile (SMAC7..10/SMAX11..12);
cross-edition legacy-save initialization remains unqualified.

The original monument stride is0x4f4. Enemy-destroyed event6 fields are year0x150,
opponent0x158, leader0x15c, awarded0x160, rank0x164, latest-event focus0x498. The
unused event parameter at0x154 and every unrelated byte remain exact. These
locations follow the inspected source globals and complete original helper
receipts, not an in-memory C++ structure dump. Field reads/writes are explicit LE.
Both editions use decoded faction combat matrices at wire0x18c/0x1ac.

The result includes separate before/after numeric snapshots and an owned World
containing the after snapshot. It does not execute casualty cleanup or prove
that it occurred: the caller must compose the correct phase and retain mandatory
later effects. Preparing this clone is not a legal attack, event acknowledgment,
revision/session commit, full battle caller, player save or turn advancement.
Active-player monument UI, multiplayer/PBEM notifications and incompatible edition
profiles stay held without publishing a partial World. Artifact friction/treaty,
base/cargo effects and the full caller remain additional dependencies.

Host and independent68040 qualification uses six synthetic versions, empty and
2,048-unit/512-base Worlds, plus three private original tutorials. It deliberately
changes current actors, terrain, bases, faction accounting, prototypes,
definitions, history and monuments away from their imported baseline. Every
before/after numeric field and all current monument bytes are compared with the
independent Python model. Complete unchanged decoded collateral is checked too.
Continuation, replacement and source destruction check both owned frames and
rebased spans. Failure guards preserve the prior output and source. The inherited
17,380 numeric controls are rerun separately on both architectures.

Normal SDK builds are compile-only. Actual AmigaOS execution, frontend/event UI,
World+body-state battle composition, variable-length persistence, session attack
commit, AI/turn/diplomacy and full playable games remain incomplete or held.
