# CD import and native resource cache

The original disc remains authoritative. Keep it in the local media repository.
No game data or converted cache may be committed or included in a public port
package without the appropriate asset rights.

## Import the disc

From the gameports checkout, with a new output directory:

```sh
python3 scripts/import_cd.py /path/to/LBA2.cue --out build/lba2-import
python3 ports/lba2-classic-community/convert_assets.py \
  --data build/lba2-import/data --out build/lba2-native-cache
```

The shared importer verifies copies of the original BIN/CUE, extracts the
ISO9660 data track, and exports audio tracks as 44.1 kHz stereo 16-bit PCM WAVs.
The CUE track numbers, index 00/01 positions and declared gaps remain in
`import.json`. WAVs omit pregaps; the original disc preserves them. Music WAVs
inside the filesystem are copied unchanged. Audio-track byte order is an
explicit setting or a recorded amplitude heuristic; listen before audio
qualification. Ambiguous audio is refused instead of guessed.

Supported intake: one BINARY BIN with AUDIO and MODE1/2352 or MODE2/2352 tracks,
standard 2048-byte ISO9660 filesystem extents. Unsupported modes, flags,
multiple BIN files, XA Form2 data, interleaved/multi-extent files, unsafe names
and conflicting extents are rejected. The importer is a reusable first step
for CDTV/CD32 media, **not CDTV/CD32 title qualification**. Preserve the raw disc:
filesystem extraction and WAVs cannot replace sector-addressed or hardware CD
behaviour. Failed imports remain in a temporary sibling folder for inspection;
only a completed import is renamed to the requested output directory.

## What is converted

Only ANIM.HQR, BODY.HQR and OBJFIX.HQR. Resources are decompressed and the
registered animation/body fixups are applied offline to their defined scalar
fields. Animation/frame validation, section bounds and overlap checks reject
unknown or malformed layouts. Compressed streams, texture bytes, scripts,
other banks, saves and movies are not blindly byte-swapped.

`cache.json` records source/cache SHA-256 hashes and entry counts. Each `.bec`
contains `LBA2BE01`, then four big-endian u32 fields (source length, source
CRC32, index count, reserved zero), followed by count index records (absolute
offset, uncompressed byte length, CRC32) and big-endian resource payloads.
The schema is version 1 and has no native pointers or compiler struct layout.

## Use on Amiga

Put `native-cache/` beside `lba2cc`. The Amiga default is
`PROGDIR:native-cache`; `LBA2_NATIVE_CACHE` can override it when present in the
program's C environment. The qualification Shell did not propagate SetEnv into
libnix's getenv, so the tested installation uses the default path.

Only bank loads with a registered fixup use the cache. Direct resource loads
retain their original fixups. Little-endian hosts retain the original loader.
Before using a bank, the engine checks its original length and CRC32 once per
process; every cached entry is checked for size, bounds and CRC32. A missing,
stale or corrupt cache falls back to the original decompression/fixup path.
No cache means no extra scan of the original bank. Cache files remain open
until normal process exit, avoiding a filesystem lookup for each resource.
Like the upstream HQR loader, this is a single-threaded path. Treat installed
data/cache files as immutable while the game runs.

## Equivalence probe

```sh
python3 ports/lba2-classic-community/tests/build_cache_probe.py \
  --stove build/team1-open3 \
  --source build/game-ports/sources/lba2-classic-community \
  --out build/cache-probe
```

Copy the probe beside `native-cache/`, then on AC090:

```text
Stack 2097152
native-cache-probe DH1:Games/LBA2/data/LBA2
```

The probe compares every non-empty bank resource with the pinned engine's
original decompressor plus native endian fixup. It must report no cache misses
and no mismatches. Its timing is a resource-loading microbenchmark; it does not
measure full startup or frame rate. See `evidence/native-cache-results.json`
for the recorded result and the remaining gameplay/release gates.

The original demo banks were also converted and compared on AC090: all 940
non-empty resources match the original decoder/fixup path exactly, with exit 0.
This adds asset-schema coverage; demo gameplay remains unqualified. See
[evidence](evidence/demo-native-cache-results.json).
