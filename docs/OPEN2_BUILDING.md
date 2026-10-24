# Building ports with OpenAmigaGCC 16.2.0-open2

Canonical target: **AmigaOS 3.x, AC090, 68040 + FPU, libnix**. Do not confuse this with the older AROS build instructions in BUILDING.md.

The published Open2 toolchain contains the newer C++/libpthread/libnix fixes. Its GCC banner still reads `16.2.0b`, so the banner alone cannot distinguish old and fixed installations. Verify the release archive and retain its provenance.

## Stage an isolated compiler and SDK

Use locally available inputs; this script makes no network requests:

```sh
python3 scripts/stage_open2_stove.py \
  --archive /path/to/openamigagcc-16.2.0-open2-x86_64-linux.tar.xz \
  --ndk-prefix /path/to/open2-prefix-with-installed-NDK \
  --sdk-prefix /path/to/existing-OpenGPU-stove/prefix \
  --destination "$PWD/build/open2-stove"
export STOVE="$PWD/build/open2-stove"
```

The script accepts only the release archive SHA-256 `5ece099a6fb8923a35eacf420bfa5fced06546ac1822714c76a2ae52466c978c`. It refuses an existing destination, protects the release's compiler/runtime files against replacement, and records every supplemental OpenGPU SDK file. Supply NDK headers generated using the Open2 install procedure; the proprietary NDK is not distributed by this repository.

**Important:** copied OpenGPU SDK archives are imported dependencies, not freshly rebuilt or newly runtime-qualified code. The receipt says this explicitly. Rebuild/qualify those dependencies separately before release. Do not copy an entire old stove over Open2: that would replace the very libpthread, libnix and C++ fixes being selected.

## Compiler selection

All AmigaOS port recipes use `STOVE`. The shared CMake toolchain now resolves:

1. Explicit `-DAMIGA_STOVE=...`.
2. A non-empty `AMIGA_STOVE` environment variable.
3. A non-empty `STOVE` environment variable.
4. Legacy `~/AmigaChrome/stoves/os32-gcc16` default, retained for compatibility.

Contradictory non-empty `STOVE` and `AMIGA_STOVE` environment values fail configuration instead of silently mixing headers and runtimes. An explicit CMake override remains authoritative. The chosen location is printed during configure. Inspect `CMakeFiles/*/CMakeCXXCompiler.cmake` or the build log to verify the actual C/C++ executable. A `CMAKE_PREFIX_PATH` pointing to Open2 headers does **not** by itself select the Open2 compiler.

Seven regression tests exercise stove selection. Staging tests additionally protect every release file (including existing NDK headers), fingerprint supplied NDK files, and reject Python versions without the tar data safety filter. Use Python 3.12 or newer as the supported baseline.

## Exact source inputs and clean builds

Use the full upstream revision in `gameports/catalog.json`. Source preparation is offline by default. The October 12 pass made independent, clean checkouts from existing local Git caches at those exact commits; it did not copy cached binaries or dirty worktrees. No shared cache, original instance or installed system stove was changed.

With prepared sources under the normal `build/game-ports/sources/` paths:

```sh
STOVE="$PWD/build/open2-stove" sh ports/openjazz/build-amigaos3.sh
STOVE="$PWD/build/open2-stove" sh ports/chocolate-doom/build-amigaos3.sh
STOVE="$PWD/build/open2-stove" sh ports/sdlpop/build-amigaos3.sh
STOVE="$PWD/build/open2-stove" JOBS=2 sh ports/nxengine-evo/build-amigaos3.sh
```

Current AssaultCube recipes require a verified open3 stove and automatically check/apply the complete patch set for each target. Build its ENet/server first, then the client in the same disposable pinned checkout. The client intentionally retains the first-light audio stub; building it is not an audio pass. See `ports/assaultcube/README.md` for the current procedure. The commands below require `STOVE` to select open3, despite this document’s historical Open2 title.

```sh
STOVE="$PWD/build/open2-stove" sh ports/assaultcube/build-amiga-server.sh
STOVE="$PWD/build/open2-stove" sh ports/assaultcube/build-amiga-client.sh
```

### Stage required codecs explicitly

The five affected recipes no longer search a WebKit build tree or arbitrary
external prefix. Before building AssaultCube, UQM, AstroMenace or NXEngine,
provide the exact inventory pinned in `toolchain/port-dependencies.lock.json`:

```sh
python3 scripts/port_dependencies.py stage \
  --stove "$STOVE" --source /path/to/explicit-m68k-codec-inputs
python3 scripts/port_dependencies.py verify --stove "$STOVE"
```

The source directory must contain the locked `include/` and `lib/` files.
No source root is guessed and no network is used. The script checks all hashes
before staging and writes only `$STOVE/prefix/m68k-amigaos/port-deps/`.
Build recipes recheck the manifest and every file before changing sources or
configuring. Extra unrecorded headers, symlinks, missing files and mismatched
bytes are rejected. Old `ZLIB_ROOT` / `UQM_DEPS` overrides outside the verified
directory are rejected rather than silently accepted.

The lock admits only zlib, PNG, JPEG and FreeType archives and matching headers;
it cannot import the donor's standard C, pthread or C++ runtime headers/libraries.
A relocated stove verifies without the original donor directory. The lock is a
frozen inventory of existing inputs, **not** proof these codecs were rebuilt with
Open2. Reproducing their original source/toolchain provenance remains a release
qualification task; this repository does not ship the codec binaries.

The newer review pass includes 113 files in this inventory. See
[review-fix evidence](../evidence/open2-20261013-resume/README.md).

## What the compiler proofs establish

Run the proof scripts from the exact Open2 source release, not an arbitrary newer branch:

```sh
bash /path/to/openamigagcc/tests/repro/prove.sh "$STOVE/prefix/bin"
bash /path/to/openamigagcc/tests/baserel/eh-check.sh "$STOVE/prefix"
```

The first combines code checks with focused `qemu-m68k` reproducers. The second is static link/archive validation; it does **not** run an AmigaOS game or demonstrate multithreaded application correctness. Note their different argument shapes: `bin` versus `prefix`.

See the [October 12 evidence](../evidence/open2-20261012/README.md). All newly built games still need clean-instance AC090 graphics, input, audio, save/configuration and clean-exit qualification. The old compiler hold is not blanket-cleared for old artifacts or imported dependencies.
