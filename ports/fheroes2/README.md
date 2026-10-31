# fheroes2 (Heroes of Might and Magic II) for AmigaOS 3.x

Status: **COMPILED**. On AROS m68k (FS-UAE, 68040), without game data, it starts, opens its window, takes a key
and exits cleanly ([evidence](evidence/RUNTIME-AROS-20261030.md)). It has not yet run with game data or on AmigaOS
3.2.3. Not TESTING, not RELEASE.

fheroes2 is the free GPL-2.0-or-later engine for Heroes of Might and Magic II.
The port builds it for **AmigaOS 3.x on a 68040, soft float**: the executable
has no FPU instructions, so it runs on a 68040 with or without an FPU (maths go
through the mathieee libraries). It draws and plays sound through OpenRTG's
**SDL2.module** and **SDL2_mixer.module**, and its zlib calls go to the
system's **z.library** (OpenZlib). It links no SDL, zlib or other library of
its own.

Pinned source: `ihhub/fheroes2` commit
`c0e8128701b29c46ad4f7f63dd31f874439981cd` (27 October 2026),
[`source.json`](source.json), [`gameports/catalog.json`](../../gameports/catalog.json).

## Build

Everything comes from GitHub, pinned by commit or SHA-256, so the build runs in
the cloud as well as on a lab machine.

```sh
# 1. The SDK: OpenAmigaGCC 16.2.0-open3, OpenNDK as its NDK, the CyberGraphX
#    and AHI developer headers from the AROS tree that toolchain/aros-local
#    pins, and OpenRTG's SDL2 + SDL2_mixer (modules and link stubs).
bash ports/fheroes2/build_sdk.sh /work/fheroes2-sdk

# 2. The game, from a clone that has the pinned commit.
git clone https://github.com/ihhub/fheroes2.git /work/fheroes2-src
bash ports/fheroes2/build-amigaos3.sh /work/fheroes2-sdk /work/fheroes2-src /work/fheroes2-out
```

The SDK step takes about two minutes, the game about two more on eight cores.
The output folder holds `receipt.json` (source, patches, hashes),
`build.log`, and the package `fheroes2/`:

| In `fheroes2/` | |
| --- | --- |
| `fheroes2` | The executable (about 8.3 MB stripped) |
| `files/data/resurrection.h2d` | The engine's own data, from the source tree |
| `maps/` | Empty until game data is staged |
| `data/` | Empty until game data is staged |
| `docs/` | Licence, changelog, this file |

No vendor NDK is used: OpenNDK (`openamigasdk/OpenNDK` in the open3 source
release) is installed as the compiler's `ndk-include`. Aminet is not needed.

## Game data

Game data is never committed or shipped. Either fetch the demo, which is what
fheroes2's own `GET_HOMM2_DEMO` option fetches (dataset `homm2-demo` in
[`gameports/runtime-data.json`](../../gameports/runtime-data.json)):

```sh
python3 scripts/fetch_runtime_data.py --root . --game fheroes2 --allow-network
python3 ports/fheroes2/stage_data.py --package /work/fheroes2-out/fheroes2 \
    --archive build/runtime-data/h2demo.zip
```

or stage your own Heroes II (its `DATA` and `MAPS` folders, and `ANIM` and
`MUSIC` if you have them):

```sh
python3 ports/fheroes2/stage_data.py --package /work/fheroes2-out/fheroes2 --game-dir /path/to/HEROES2
```

## On the Amiga

- `LIBS:OpenGPU/SDL2.module` and `LIBS:OpenGPU/SDL2_mixer.module` (OpenRTG),
  and `LIBS:z.library` (OpenZlib). z.library is required: the engine's own
  data (`resurrection.h2d`), its icons and saved games are zlib-compressed.
  The calls use OpenZlib's LVOs (`z.lvo`); that table is still marked "ABI
  test" against Aminet's z.library, so use OpenZlib's.
- An RTG screen; 640x480 is the game's native size.
- Fast RAM: the executable is 8.3 MB and the game's data loads on top of that.
  Plan on 64 MB.
- Run it from its folder, `fheroes2`, with a stack of at least 262144 bytes
  (`Stack 262144` first in a Shell; the right figure is still to be measured). Settings and saved games go in
  `PROGDIR:User`.

## What the port changes

`patches/0001-amigaos3.patch`, applied to the pinned source at build time:

- `serialize.h`: AmigaOS is big-endian and libnix has no `<endian.h>`, so the
  byte-order macros are given directly. The engine already reads its
  little-endian data files correctly on big-endian machines.
- `system.cpp`: the home folder is `PROGDIR:User`; joining a path onto a
  volume or assign (`PROGDIR:`) does not add a `/`, which on AmigaDOS would mean
  the parent folder; `PROGDIR:` is searched for data; the case-insensitive path
  search is skipped, as AmigaDOS names are already case-insensitive.
- `settings.cpp`: the build-time install path (a Unix path) is not searched.
- `rand.cpp`: AmigaOS has no entropy device, so `std::random_device` throws
  (it aborted the game before `main()`); the RNG is seeded from the clocks and
  the thread's stack address instead.
- `screen.cpp`: without opengpu.library SDL has no accelerated renderer; the
  game falls back to SDL's software renderer instead of dropping its window.

The port's own files:

- `zlib/zlib.h`, `zlib/zconf.h`, `zlib/zlib_zlibrary.c`: the four zlib calls the
  game makes (`uncompress`, `compress2`, `compressBound`, `crc32`), each one
  z.library call (LVOs from amigachrome-guest `libraries/openzlib/z.lvo`).
- `compat/statvfs.cpp`: libnix has no `statvfs()`, which the open3
  libstdc++'s `std::filesystem::space()` refers to. The game never calls it.
- `conf_to_sfd.py`: makes the CyberGraphX SFD for sfdc from AROS's
  `cybergraphics.conf`, for the build-time headers SDL's back ends need.

The game also links GCC's `libatomic` (64-bit `std::atomic` in the audio code)
from the open3 compiler.

## Still to do (release gates)

Run on an AmigaOS 3.2.3 lab copy with the demo: title screen, new game,
adventure map scrolling, a battle, keyboard and mouse, sound effects through
SDL2_mixer, music (the demo's music is MIDI, which needs Timidity patches),
save and load, and a clean exit; then the same on a 68040 with no FPU. Speed on
a real 68040 is unknown: the engine was written for PCs.
