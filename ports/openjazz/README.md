# OpenJazz — m68k AROS

**Status:** COMPILED / RUNTIME TEST PENDING

## Upstream

- repository: https://github.com/AlisterT/openjazz.git
- pinned commit: `a1626f4edd4a7af72c54103021790c56d8ecfced`

## Compile result

- target: m68k AROS / 68040
- executable: `OpenJazz`
- observed size: 2,340,948 bytes
- SHA-256: `b60483559a5fffc1aca2a079a8bc45a6b5c8d0283de25503082f78e72e2d8f21`

## Successful configuration

- SDL2
- `NETWORK=OFF`
- `SCALE=OFF`
- `SDL_VERSION=2`
- Release build

Compilation reached 100%. The first final link left the GCC signed-integer division helper `__divsi3` unresolved.

The successful relink appended the target GCC runtime returned by:

    m68k-aros-g++ --sysroot="$AROS_SYSROOT" -m68040 -print-libgcc-file-name

## Data

OpenJazz requires Jazz Jackrabbit game data. For first-light testing we use the **Jazz Jackrabbit 1.1 shareware** dataset locally. It is not committed here.

## Gate

Title/menu -> enter shareware level -> scrolling -> keyboard/controller input -> audio -> clean exit.
