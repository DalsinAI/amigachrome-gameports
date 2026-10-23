# Owner-supplied Jazz asset validation

This probe links the actual pinned OpenJazz engine and changes only its executable
entry point. It runs normal startup (panel, fonts and audio initialization),
constructs the menu, constructs each selected scene or level, then shuts down.
It does not enter a scene playback or gameplay loop. Production source and its
SDL2 recipe remain unchanged.

The level probe includes the real block, sprite, animation, palette, event and
sound-reference loaders. It samples the loaded collision map at every 8-pixel
mask cell and hashes canonical one-byte values, avoiding native structs and
byte-order-dependent hashes. Matching hashes prove this selected decoded state;
they do not establish equivalence of every sprite, palette, event or sound.

## Build from the exact source

Use upstream commit `a1626f4edd4a7af72c54103021790c56d8ecfced` from the
existing catalog. Export it to a fresh directory; do not modify a source cache:

```sh
mkdir -p build/jazz-source
git -C /path/to/openjazz-cache archive a1626f4edd4a7af72c54103021790c56d8ecfced | tar -x -C build/jazz-source
for patch in ports/openjazz/patches/*.patch; do
    git apply --directory=build/jazz-source "$patch"
done
cmake -S ports/openjazz/validation -B build/jazz-assets-host \
    -DOPENJAZZ_SOURCE="$PWD/build/jazz-source" -DCMAKE_BUILD_TYPE=Release
cmake --build build/jazz-assets-host --target OpenJazz -j2
python3 ports/openjazz/validation/run_assets.py \
    --probe build/jazz-assets-host/engine/OpenJazz \
    --data /path/to/original-jazz-data --out build/jazz-assets-results
```

The host needs SDL2 development files. The engine's bundled libraries are built
from the same source export; networking and scaling are off. No downloads or
asset redistribution are involved. Each host case has a fresh writable temporary
user folder; source assets remain read-only inputs. The probe emits no asset
images or sound files into the repository. Test only trusted owner-supplied
files: this is not malformed-input safety qualification.

For Amiga, use a separate output directory and add:

```sh
-DCMAKE_TOOLCHAIN_FILE="$PWD/toolchain/amigaos3-opengpu.cmake" \
-DAMIGA_STOVE=/path/to/verified-open3-stove \
-DAMIGA_CPU_FLAGS="-m68040 -m68881 -fno-delete-null-pointer-checks"
```

Stage that build's `engine/OpenJazz` as `asset-probe`, with original files in
`data` and empty `User`, `Results` and `Empty` directories inside a dedicated
lab folder. The host runner generates `ac090-assets.script`; its default guest
root is `DH1:Games/OpenJazz-AssetValidation` (override with `--guest-root`).
Run that script explicitly in an isolated AC090 lab with the existing SDL2
runtime. It does not install modules or autostart anything. Inspect each result,
all Shell return codes, and `MISSING RC 4`; compare only `STATE` lines with the
host reference, not timestamps or configuration warnings. A linked HUNK file
is not a passed target test. Keep an external bound on the lab run and stop it
if a case hangs. The host runner bounds each case with a timeout.

## Dataset and limits

The local `RGJAZZ_2.iso` is original Jazz Jackrabbit data, not Jazz Jackrabbit 2.
All 106 torrent pieces passed; all 345 extracted disc files match the staged
source checksums. Asset hashes and exact probe/SDK fingerprints are recorded in
[provenance](../evidence/asset-provenance.json).

Both host and AC090 suites pass: one menu, 18 scene files and 79 level files
on each platform. All 79 canonical level collision-map digests match exactly.
Every AC090 case returns 0. The target negative control returns 4; results are
in [AC090 evidence](../evidence/ac090-assets.json) and
[Shell return codes](../evidence/ac090-shell-results.txt). Its negative
control rejects an empty data directory with engine error/exit 4. Full results
are in [host evidence](../evidence/host-assets.json).

The host reports 15 missing named sound references across 10 levels, retained in
[sound warnings](../evidence/sound-reference-warnings.json). Successful level
construction does not resolve those warnings. The same warnings occur on AC090.
All 22 SOUNDS.000 sample ranges are in bounds, and the nine missing names are
absent from that bank; see the [sound bank audit](../evidence/sound-bank-audit.json). Music-module decoding, audible
AHI output and listening checks remain unqualified. The root data directory contains
35 PSM music files; scene/level construction is not full playback of those files.

No endian conversion is selected here: the engine's scalar file readers already
assemble little-endian values explicitly, while indexed pixels remain bytes.
Native caches should follow measured bottlenecks and verified schemas; a bulk
byte swap would corrupt the original formats. No gameplay, input, save/load,
clean application exit, full graphics, audio or release qualification is claimed.
