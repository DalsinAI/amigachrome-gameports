# Enigma with OXP

Source pin: `6f0c51b26cc013cb6c935046d7d1e6a20bea4179`
([upstream](https://github.com/Enigma-Game/Enigma)), GPL-2.0-or-later.
The port patch replaces Xerces headers, DOM/parser/serializer calls and CMake
linkage with the existing OpenAmigaXML OXP parser and its optional schema module.
No Xerces headers or library are required. The adapter is Enigma-specific;
OXP retains its own public C API.

## Offline build

Prepare the pinned upstream checkout with the existing source intake command:

```sh
python3 scripts/game_port_prepare.py prepare --game enigma --allow-network
```

Stage a private Open4 stove from the checksummed release archive and explicit
OpenNDK/SDL SDK donors (never change a shared stove):

```sh
python3 ports/enigma/stage_toolchain.py \
  --archive /path/to/open4.tar.xz --ndk-prefix /path/to/openndk-donor \
  --sdk-prefix /path/to/target-sdk --destination build/enigma-open4-stove
```

Build with a committed OXP extension revision and an explicit target overlay:

```sh
nice -n 19 sh ports/enigma/build-amiga.sh \
  --source build/game-ports/sources/enigma \
  --stove build/enigma-open4-stove --sdk-overlay /path/to/target-overlay \
  --oxp-source /path/to/openamigaxml \
  --oxp-ref 303deba5a9ed6d7d38b9cac524b8bd91c080ff67 \
  --out build/enigma-open4-soft --jobs 4
```

The historically named `build_open3.py` now requires verified Open4 provenance.
It exports both source commits into a fresh output directory, applies patches
without fuzz, builds native host tolua, then cross-builds with OpenNDK,
68040 soft-float specs and the selected SDK overlay. It records patch and input
hashes, commands, a link map and executable hash. The final link rejects Xerces
symbols. `configure_open3.py` is the configuration-only compatibility entry.
No network access or live-instance deployment occurs during the build.

## XML validation and runtime

The adapter supports Enigma's local schemas, DOM edits/imports, defaulted versus
explicit attributes, filtered score serialization and UTF-8/UTF-16 conversion.
Unmapped external resources are rejected. XML files saved by Enigma omit schema
default attributes, preserving the score checksum/reload semantics.
See OpenAmigaXML `oxp/SCHEMA.md` for the deliberately bounded XSD subset.

Checks recorded on 31 October 2026: 72 parser fixtures plus API checks,
96 mutation/schema checks, 28 adapter checks, and all 1,634 supplied Enigma XML
files validate against the seven schemas. The isolated AROS native tests also pass with the new Open4 soft-float
build and FPU disabled: 96 schema checks and 28 adapter checks, both exit 0.
These results qualify the XML paths. The clean full executable renders its
loading screen and loads actor models; menu/gameplay remain unqualified.
Test sources are in `tests/`; native logs and build receipts are in `evidence/`.

An earlier experimental executable reached SDL video/audio initialization and
loaded state, object definitions and 787 objects, then failed network setup in
the isolated AROS lab. `--robinson` now skips network initialization for offline
play. Native paths use `PROGDIR:data` and `PROGDIR:User`.

Remaining qualification: Open4 soft-float runtime, instruction audit, rendered
level/input, save/reload, audio and normal exit, then the intended ROM/hardware.
The selected SDK currently includes static curl, zlib and SDL_ttf dependencies;
the project module/OpenFont/z.library policy must be satisfied before release.
This integration is a draft, not a release or a claim of playable Enigma.
