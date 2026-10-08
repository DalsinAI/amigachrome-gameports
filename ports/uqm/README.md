# The Ur-Quan Masters 0.8.0 — AmigaOS 3.x / AmigaChrome

Target: **AC090 / AmigaOS 3.x / 68040 + FPU**

The source archive is already pinned in `gameports/catalog.json` by URL and
SHA-256. Source and game content remain separate inputs.

## Current status

**COMPILED — provisional old-stove artifact.**

The AmigaOS/OpenGPU lane now compiles and final-links the full UQM executable.
The successful provisional old-stove binary is an AmigaOS loadseg executable
with SHA-256:

`4173391559e65d7cfb6c330cd2820bc33cb97a6402175f895c11c3f057f386b8`

It is not a release binary. The installed GCC 16.2.0b stove has known compiler
defects on AC090/68040 and the port must be rebuilt and runtime-qualified with
the approved fixed stove.

## Portability work

The build recipe keeps the upstream source pristine and applies the Amiga
adaptation to its copied build tree. The important target fixes are:

- reuse UQM's AROS cross-build profile as a generic big-endian m68k profile;
- use the AmigaOS/OpenGPU SDL2, GL and pthread personality;
- supply `NAME_MAX=255` for Amiga where POSIX `_POSIX_NAME_MAX` is absent;
- use libnix `readdir()` rather than absent `readdir_r()`;
- use the existing `HOME` path on Amiga instead of Unix `getpwuid()`;
- account for libnix `strcasecmp` / `stricmp` despite UQM's cross-probe not
  seeing their declarations;
- force UQM's own bundled GNU regex implementation because the Amiga sysroot
  exposes `regex.h` without a linkable POSIX `regcomp/regexec` provider.

The bundled regex object was verified to provide `regcomp`, `regexec`,
`regfree` and `regerror` in the successful final link.

## Local dependencies

The build uses:

- the selected Amiga GCC stove root via `STOVE`;
- the AmigaOS/OpenGPU SDK below that stove;
- `UQM_DEPS` for the current local libpng/zlib compatibility root.

No source or dependency fetch is performed by this build script.

## Runtime/content gate

UQM game content is deliberately not bundled. Runtime qualification requires
an already-authorised local UQM 0.8 content tree, or explicit approval before
any external content fetch.

Required first visible gate:

1. main menu;
2. start a new game;
3. solar-system view;
4. one alien conversation screen;
5. then input/audio/save/clean-exit qualification.

**DONE means RELEASE.**
