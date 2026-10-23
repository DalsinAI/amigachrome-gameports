# Little Big Adventure 2 Classic Community

Target: AmigaOS 3.x / 68040 + FPU / AC090, OpenAmigaGCC 16.2.0-open3.
Status: reproducible executable with bounded AC090 smoke tests; full runtime and release qualification remain open.

Source: `LBALab/lba2-classic-community`
`2fa1c7a66de2ce8707d85cb9a304dbd6f96c8ab8`. The separate `source.json`
catalog keeps this SDL3 lane out of existing SDL2 port builds.

Two clean builds in different output directories produced identical HUNK
executables, SHA-256
`4078ef897573f10201ce9d1c822ac25e35c361191fe3d6d78a8ea38a17440b73`.
This is reproducible compile/link evidence, not gameplay qualification.

## Build

Prepare the exact source with the existing offline-by-default preparer:

```sh
python3 scripts/game_port_prepare.py prepare --catalog ports/lba2-classic-community/source.json --game lba2-classic-community
```

If absent, source preparation needs an explicitly approved `--allow-network`
run. No game assets are fetched. Stage the compiler using local inputs:

```sh
python3 scripts/stage_open2_stove.py --release 16.2.0-open3 \
  --archive /path/to/openamigagcc-16.2.0-open3-x86_64-linux.tar.xz \
  --ndk-prefix /path/to/contained-ndk-prefix \
  --sdk-prefix /path/to/existing-opengpu-sdk-prefix \
  --destination "$PWD/build/team1-open3"
```

The script name is retained for existing callers; `--release` selects the
archive, SHA-256 and receipt identity. The open3 archive hash is
`9fcc136ab59e3bb5b91d2764d0bf290b709d72d7380be4ce0cdf005edf8eff21`.
The stager rejects external NDK symlinks and replacing compiler/runtime files.
Imported SDK dependencies retain their unqualified status.

Build SDL3 from OpenRTG `0c98fe780e6531946be44bebbb3a9db0c7bb7ab2`, using
`library/modules/sdl3/build.sh` with `STOVE` pointing to the compiler's
**prefix**. That platform builder can download missing inputs; approve those
fetches explicitly or supply its pinned local tarball and backend cache.
Its `UPSTREAM.json` pins SDL 3.4.14 archive SHA-256
`30d4aa2b3037718142b32dffd4e72f917ebb6cc5227150e7bb9c45efb2153aeb`
and SDL3_68k `c82dea042718200eea401e7ed62d88b997bc9fd9`.

The game recipe uses `STOVE` as the stove **root**, consistent with other game
recipes, and `SDL3_SDK` as that SDL3 builder's output directory:

```sh
STOVE="$PWD/build/team1-open3" SDL3_SDK=/path/to/sdl3-build \
  sh ports/lba2-classic-community/build-amigaos3.sh --out "$PWD/build/lba2-clean"
```

Choose a new output directory each time. The runner verifies compiler/runtime
hashes and pristine pinned source, exports without modifying the cache, applies
checked patches, and writes configure/build logs and a receipt in the output.
`SOURCE_DATE_EPOCH` comes from the source commit; diagnostic source paths are
normalized. It links the SDL3 module client, never a private static SDL stack.

## Patches and checks

- Select the OpenGPU SDL3 module client without pretending it is a desktop
  shared library. Disable ELF PIC on those application objects: HUNK assembly
  rejects the generated GOT references.
- Make IPO optional and disable it here. The attempted open2 LTO link left
  `MulMatrix`, `PcxHeader` and `Index` unresolved. The canonical open3 recipe
  uses the validated non-IPO configuration; open3 LTO is not qualified.
- Supply the missing `lrintl` operation using the FPU's current rounding mode,
  preserving the extended operand. A double cast can change half-integer ties.
  The implementation is a port-owned C source; upstream game mathematics and
  the SDL2 backend are unchanged.

The rounding probe passed 80,060 exact comparisons against the Linux m68k
reference under `qemu-m68k -cpu m68040`, covering four rounding modes and
extended-precision values around half-integer ties:

```sh
m68k-linux-gnu-gcc -m68040 -O2 -frounding-math -fno-builtin \
  -DLBA2_LRINTL_SYMBOL=openamiga_lrintl \
  ports/lba2-classic-community/compat/lrintl.c \
  ports/lba2-classic-community/tests/rounding.c -static -lm -o /tmp/lba2-rounding
qemu-m68k -cpu m68040 /tmp/lba2-rounding
```

Upstream host-quick tests: 96 passed, one skipped. Discovery initially failed
under the sandbox and passed when retried outside it. The full ASM-equivalence
Docker suite was not run: the local `lba2-test` image was absent. The game's
FPCR scan passed. None of these is an Amiga gameplay test.

## Platform and release gates

SDL3 provides AHI output. The inspected platform baseline still lacks the
SDL2 OpenGPU renderer integration; later Team 4 work requires its own source
pins and verification. No SDL3/platform changes are carried in this port.

Keep classic retail data owner-supplied. Engine code is GPL-2.0-only; retain
upstream and bundled third-party licences/notices when packaging. No assets,
NDK headers or binaries are committed here.

The [original playable demo](DEMO.md) has been located and inspected. Use
`build-demo-amigaos3.sh` with its separate demo manifest; its data cannot be
used with the retail executable above.

Follow [asset preprocessing](../../docs/ASSET_PREPROCESSING.md) and
[release policy](../../RELEASE_POLICY.md). Still required: endian and FPU
gameplay comparisons, usable title/game frames, controls/text/gamepad, AHI
sound and music, Smacker video, persistence, focus/fullscreen, memory/performance
and clean shutdown on clean 68040/AC090 targets. No FIRST LIGHT or RELEASE claim.

## Native CD intake and cache

The [CD importer and native resource converter](NATIVE_CACHE.md) are implemented.
The AC090 probe compared all 2,651 non-empty animation/body/object resources
against the pinned original loader byte for byte. A three-tick headless scene
smoke run also completed. These are bounded engineering results; display,
input, AHI/music, persistence and full gameplay qualification remain open.
