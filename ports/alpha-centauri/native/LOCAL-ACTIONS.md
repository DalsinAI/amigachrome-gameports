# Owning local action and snapshot integration

The historical `LocalProductionSession` now has an explicit base-game
`load_local_land_actions` path. It owns the unit definitions parsed from the
same rules input as its technologies, facilities and basic/world rules. Its
ordinary `load_local` path remains production-only and cannot issue movement
commands. No separate caller-supplied player, movement rate or mutable rules
object can change a loaded command's authority. All failed loads retain the
previous world, owned rules, enabled capabilities, suspension and revision.

A `LocalStepCommand` is a private session-lifetime/revision-bound intent.
Preparation checks local ownership and coordinate bounds, but does not claim a
legal move or quote a price. Commit evaluates the frozen bounded clear-land
core with the selected human and owned road rate. A checked refusal is reported
in `StepResult`, without changing world or revision. A successful move returns
repaint/persistence requirements and increments the same revision used by
facility plans. Consequently a move invalidates an older production
confirmation, and a changed production order invalidates an older step command.
Suspension, reload, close, cross-session use and address reuse also invalidate
commands. Copying a private command does not create fresh authority.

The existing action subset is unchanged: ordinary base-game human land units,
fully seen cleared land, no foreign contacts/presence, pods, fungus, monoliths,
transport, abilities, standing orders or fractional attempts. Expansion actions
remain explicitly unsupported. FREEABIL action gates remain conservative even
though the separately qualified speed/price code handles its documented edition
rules. This is not original order_veh, a pathfinder, general movement legality,
AI, combat, diplomacy or a new turn. Exhaustion never silently refills movement.
One owning engine thread is required; no GUI or original modal/network runtime
is synthesized. Existing frontends do not instantiate this controller.

## Preserving a mixed runtime world

The separate movement-only and production-only encoders still reject a mixed
world. `encode_local_actions_snapshot` adds a buffer-only union boundary: tiles
and vehicles; base current minerals and queue item0; faction units_queue[512].
Every other decoded field, preferences, game/map metadata and borrowed-span
binding is checked against the immutable original import. Vehicle references
and complete reciprocal/single-root/acyclic stacks and occupancy bits are
validated before any output write. Stack validation allocates a bounded heads
array and can return out_of_memory transactionally.

The complete original supported plain SC/SAV7..12 container and opaque bytes are
preserved, with explicit LE patches. Legacy base/faction strides are304/0x20bc;
expansion strides308/0x20cc. Base current minerals and queue0 are0x40/0x50 in
both. Legacy units_queue is0xa54, modern0xa58: this is a four-byte field shift,
not the larger shift of later legacy fields. Mineral baseline, prior production,
inactive queue entries and footer tick remain unchanged. Runtime RNG is returned
separately. Native structs are never dumped into a save.

Output/receipt must be disjoint from the world storage and each other; errors
preserve both. Serialization certifies the supported record boundary, not order
authority/legality. Live inspection/rendering must use the owned record getters;
original bytes and borrowed actor spans remain the import baseline. There is no
filesystem save transaction, SDK atomic-create assumption, player save/load UI
or original-executable save-compatibility claim here. Private probes use fresh
evidence paths and refuse readable existing targets; that is not an atomic file
commit protocol. No construction completion, upkeep or research discovery is
implemented by these commands or encoders.

## Evidence checkpoint

Host ASan/UBSan passes203 action contexts:43 public synthetic controls and160
edges of three unchanged original tutorials. Initial attempts produce51 moves
(6 public,45 original),76 preparation refusals,75 checked action refusals and one
load refusal. These are independent reset contexts, not51 campaign turns. The
public cases omit three old caller-supplied player/rate tests because those
arguments are no longer accepted by a session command; all46 inherited raw
boundary/sequence tests remain in validation.

The shared-authority synthetic case resets its baseline, performs two moves and
three facility switches, rejects stale commands in both directions, and proves
exhaustion preserves a current production confirmation. Both older encoders
reject the mixed state without writes. The combined output reloads into another
action-enabled session, retains spent movement/RNG, refuses further exhausted
movement and continues production. Both live and reloaded paths then encode
identically. An independent Python patch of the complete144890-byte container
matches SHA256 `c7f90d413bc6fb356b9ab07bbcec383f28879438442c9564df91cf8b374df889`.
Queue becomes-3, current minerals60 from the unchanged111 baseline, expenditure6,
and the old unit counter decrements only once. No turn was advanced.

Additional checks cover copied/replayed/foreign/suspended/reloaded/address-reused
commands, empty sessions, changes to caller input buffers, missing unit sections,
insufficient memory, action-to-production-only imports, unchanged failure
outputs and all decoded records. Reference-path equality calls the frozen
bounded core, not the original Windows engine. Independent Python checks the
expected boundary statuses and complete mixed container. No full original-game
action differential is claimed.

The combined encoder passes163 host codec controls:112 public across versions7
through12 plus51 controls on the original tutorials. These generic codec edits
exercise storage/serialization fields, not legal player movement. Positive
outputs match independent Python byte patches, reload every decoded field,
re-encode identically and preserve source files; unrelated changes, bad
references/stacks, short/aliased output and existing targets refuse without
writes. Legacy production, production-snapshot, raw action, unit-stat and price
regressions remain required. Thirteen frozen core source files are unchanged;
all79 workflow commands retain the parent suites plus the new controls.

Normal Amiga SDK builds use installed profiles and no overlays/stubs. Actual
AmigaOS execution and final-head independent68040 CI remain pending at this
committed evidence checkpoint. The user-owned watch preview is untouched. A
local merge-header conflict error and test-driver initializer issue were fixed
before these final passes; private history is retained. Full restored games are
still incomplete.
