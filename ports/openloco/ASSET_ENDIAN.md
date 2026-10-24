# Little-endian assets on AC090

24 October 2026: **engineering asset-loader tests pass; gameplay is blocked**.
Upstream is pinned to `6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e`.

Patches 0012 and 0013 decode wire metadata at the load boundary. Original game
files remain unchanged. The full application still has audio disabled and uses
the existing frozen SDL3 SDK; this does not qualify a new renderer or OpenAL.

## Scope

- G1 headers and element records are read explicitly as little-endian values.
  Bitmap, palette and RLE ranges are checked before use. Indexed pixels remain
  bytes. The remaining native RLE row-offset cast is replaced by a bytewise read.
- CSS counts, offsets and WAVEFORMATEX fields are decoded explicitly. PCM ranges,
  channel counts, block alignment and sample rates are checked. Native 16-bit
  audio-buffer conversion occurs once when loading a buffer on a big-endian
  machine; this is code coverage, not an audible-audio pass.
- Retail DAT headers and embedded dependency headers use explicit wire readers.
  Checksums consume the original low flag byte, independent of host byte order.
- Fixed object fields are normalized once after checksum verification and before
  validation. The schema covers all 34 object types and their nested fixed
  records. Byte arrays remain bytes; the active vehicle sound union is selected
  by its discriminator. The field inventory in `schema/fixed-object-fields.json`
  was derived from the exact pin's Clang AST and is an audit aid. Review the
  inventory and C++ field walk whenever the upstream pin changes.
- Object export writes little-endian headers and reverses normalized fixed
  fields in a temporary copy. The original in-memory object and source DAT are
  not rewritten. Native object-index caches remain native caches.

This is not a bulk byte swap or a new on-disc format. Variable object tables
still need further audit. The S5 game-state/entity/map structures are outside
this patch set and must not be fed to native code after merely swapping a
version field.

## Reproduce the probes

First build the game with the normal offline recipe to obtain its clean patched
source export. Then configure the probes with all three source paths:

```sh
cmake -S ports/openloco/probes -B build/endian-probes \
  -DOPENLOCO_SOURCE="$PWD/build/openloco-clean/source" \
  -DFMT_SOURCE="$PWD/build/game-ports/dependencies/fmt" \
  -DSFL_SOURCE="$PWD/build/game-ports/dependencies/sfl"
cmake --build build/endian-probes --target port-probes -j2
ctest --test-dir build/endian-probes --output-on-failure
build/endian-probes/asset-wire /path/to/g1.dat /path/to/css1.dat build/canonical
build/endian-probes/object-wire build/object-paths.txt build/objects.canonical
```

The object list contains one DAT path per line. Use the actual case-sensitive
host filenames. `asset-wire` also runs synthetic valid/truncated/malformed tests.
`object-wire` tests unaligned header decoding, truncated-header rejection, the
checksum flag byte and a typed climate record before checking the supplied DATs.
Both executables run without arguments for their synthetic tests only.

For AC090, configure a separate directory with the Amiga toolchain file and
verified open3 stove. Run the HUNK probes in an isolated lab against the same
owner-supplied files, then compare canonical outputs byte for byte with the host.
The assertions remain enabled in Release probe builds. No game assets or
canonical payloads belong in Git.

## Actual results and limits

The final clean source export compiles to a full HUNK application. All 55
repository tests and three host probe tests pass. The final host and AC090
probes both pass 4,122 G1 elements, 32 CSS sounds, and 545 DAT objects covering
all 34 types and 50,153,847 decompressed bytes. All three canonical output files
match byte for byte across host and target. See [the receipt](evidence/endian-assets.json).
The compiler/linker still emits duplicate libstdc++/fmt section diagnostics;
these are retained in the local build logs, not suppressed or declared fixed.

The final full application gets past the previous graphics/audio-header and
object-checksum failures. Its startup log reaches `boot -> title`, then reports
`Unable to load S5: Unsupported S5 version`. That is the next concrete barrier:
S5 saved-game/scenario state needs a schema-aware import. A log transition is
not a visible menu or gameplay pass. No graphics, gameplay, sound, save/load,
performance, physical AC090 or release qualification is claimed. Instance-32
was not modified by these OpenLoco tests.
