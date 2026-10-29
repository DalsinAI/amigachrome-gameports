# Production-only original container snapshots

`encode_production_snapshot` writes a caller-owned buffer containing the complete
supported original SC/SAV container, preserving opaque bytes. It patches only
base current minerals, production queue item0 and faction units_queue[512]. All
other decoded records and game/map metadata are compared to the immutable import
baseline first; unsupported changes are rejected instead of silently discarded.
Tiles, actors, prototypes, other base/faction fields and faction definitions
remain unchanged. This serializer establishes no order authority or legality.
The owning local session from the parent change provides the facility-order
boundary; existing frontends instantiate neither controller nor serializer.

The base fields use explicit LE int32 offsets0x40 and0x50 in304-byte legacy or
308-byte expansion base records. Faction queue bytes use0xa54 in legacy0x20bc
records and0xa58 in expansion0x20cc records. These follow the pinned original
schemas, including the four-byte legacy shift at this field. An earlier private
planning note incorrectly suggested0xa4c; neither the final code nor existing
session tests use that value. No packed structs or native object dumps are saved.

All validation precedes output writes. Output and receipt must be disjoint from
one another and all world storage; short/null/aliased buffers reject without
writes. Receipt contains byte length and separate runtime RNG. The original
footer tick is preserved and is never repurposed as live RNG. No allocation is
performed by the encoder. Unchanged imports re-encode byte exactly; the original
owned wire remains baseline after a buffer export.

This is not a filesystem save operation, player save/load UI, atomic file-create
protocol or original-executable save-compatibility claim. It does not combine
movement serialization with production changes, run original loading fixups,
complete production, process upkeep, advance turns or execute AI/diplomacy.

## Evidence checkpoint

Host ASan/UBSan and independent Python byte patches pass204 controls across wire
versions7..12, including12 controls from three unchanged owner tutorials. The
public corpus is192 controls. Eighteen direct exports test unchanged/full-field
patterns,72 unsupported edits reject without writes, and114 session contexts
include24 rejected loads. The90 loaded session contexts attempt225 orders:
130 accepted (106 changed,24 no-op) and95 rejected. These counts include orders
continued after a buffer reload, not played campaign turns.

Each successful output reloads and re-encodes identically. Session controls
compare the next order after reload with continuation from the still-running
session and compare resulting complete containers. Independent Python patches
use schema-derived offsets. Full output SHA256:
ae83cac16d1bae673260393ec8158d2519c5d641a8bdc97979f2ec559d17b835.
Public-only output SHA256:
912ad18d2a929c7bb266ad20ad19ec6b5b007a0f9732954571e0edd2230b4fd1.

Unrelated-edit controls cover base baseline/last-production/other queue slots/
queue length, faction energy, active faction, actual vehicle position, tile,
prototype, faction definition, map metadata and borrowed span changes. Empty
world, short/null destination, world/array/receipt aliases and unchanged error
receipts are also exercised. Initial host1 failed in a test driver that assumed
a vehicle existed; the fixture now contains a valid vehicle. Final host3 is
authoritative and retains the failed history privately.

The normal installed Amiga SDK builds the probe without toolchain overlays or
stubs. Actual AmigaOS execution and final-head independent big-endian CI are
UNRUN/pending at this committed checkpoint. No lab input/write occurred; the
user-owned watch remains untouched.
