# Local player qualification candidate

This branch adds the first player-facing slice on the frozen #113 integration:
load a supported base-game world, issue the existing restricted land moves,
confirm an available facility with its mineral switching penalty, write a new
checkpoint, reopen it and continue with the same spent movement and RNG seed.

The ordinary frontend build retains its read-only entry points. The candidate
is selected explicitly with `build_local_player_frontend.sh OUTPUT`. It needs
the normal installed AmigaOS/OpenGPU SDK, original owner-supplied assets and
68040/FPU. It has not been compiled against that SDK or run on AmigaOS here.
Do not deploy or merge as native-qualified until those gates pass. The draft
#113 dependency chain also retains its existing native qualification gates.

## Controls in the candidate

- Menu **L**: open a base-game scenario, save or our `session.aclocal` checkpoint.
- **TAB** selects a unit; **C** centres it; **WASD** scrolls; **+/-** zooms.
- **7/8/9, 4/6, 1/2/3** move the selected unit northwest/north/northeast,
  west/east, southwest/south/southeast. The existing bounded action core rejects
  unsupported movement; exhaustion never refills itself. Close base details
  before moving. The HUD reports remaining/total movement points.
- **B** selects a base; **P** opens/cycles eligible facility proposals. The modal
  shows the target and minerals before/after including switching loss.
  **ENTER** commits the bound proposal; **ESC** cancels it.
- **V** creates a fresh `PROGDIR:AlphaLocal-...` directory and writes
  `session.aclocal`. **R** reopens the last successful save in this view.
  To reopen a checkpoint after restarting, use **L** and select that file.
- **Q** or the close gadget requests exit. Dirty sessions require a second
  exit request. Reload with unsaved changes similarly needs a second R.

This remains a bounded offline local session. No next turn, AI, combat,
diplomacy, research awards, production completion or worker allocation is added.
The base-game ownership and active-human gates remain in the owning session.
Expansion stays read-only. Facility orders retain the single-slot queue limit.
The renderer uses live owned records following a move/order/reload; borrowed
original wire remains the import baseline. Saved intake/surplus values are still
saved-turn observations, not recalculated or advanced by these local orders.

## Checkpoint contract

`ACLOCAL1` is our checkpoint envelope, not a promise of compatibility with the
original Windows game. Its 28-byte little-endian header contains the magic,
CRC32 over bytes 12..end, payload length, payload CRC32, runtime RNG seed and
original rules-text CRC32. It requires the same rules text on reload. CRCs detect
accidental corruption; they do not authenticate content. The payload is the
complete supported original container encoded by the existing mixed-action
encoder. Opaque original bytes and movement expenditure are preserved.

Encoding is transactional and rejects output aliases into the adapter or any
owned world records. Truncation, malformed headers, corruption, incompatible
rules and failed imports leave the loaded world, dirty flag and pending plan
intact. A successful reload cancels pending confirmation. A successful move
also cancels it; rejected movement leaves it available.

The native file path reserves a new directory with DOS CreateDir before opening
its payload. It never intentionally replaces an existing save. Failed writes
retain the dirty session and leave a partial slot, which cannot pass the length/
CRC checks. Power-loss durability, host filesystem bridging semantics and the
actual native CreateDir/write/close behavior require native qualification.

## Checks and next native run

`test_local_player.py --out NEW_DIRECTORY` builds/runs the portable adapter with
host ASan/UBSan. `--cc m68k-linux-gnu-g++ --m68k-linux` uses actual independent
big-endian Linux 68040 execution. Both commands are included in alpha-rules CI,
with every inherited #113 workflow command retained.

The control performs two moves, facility changes, complete checkpoint/reload,
continued production, preserved exhaustion, short/aliased output rejection,
header/payload corruption, failed rules reload, pending-plan preservation,
dirty-state transitions and close. Python separately verifies zlib CRCs and
all 144,890 payload bytes against explicit original-container patches. No
proprietary fixture or asset is required or committed.

On the SDK host, build `build_local_player_control.sh OUTPUT`, then execute its
output on AmigaOS with the generated `rules.txt original.SC OUTPUT.aclocal`.
Require RC0, the expected control count and exact checkpoint hash. Then build
the candidate frontend, run the controls above using owned original data, test
failed/duplicate save slots and dirty exit/reload, and restart/reopen a saved
checkpoint. Capture exact SDK/binary hashes and native logs before changing
qualification status. Current evidence is in LOCAL-PLAYER-EVIDENCE-20261026.json.
