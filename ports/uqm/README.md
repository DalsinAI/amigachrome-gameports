# The Ur-Quan Masters 0.8.0 — AmigaOS 3.x / AmigaChrome

Target: **AC090 / AmigaOS 3.x / 68040 + FPU**

The source archive is already pinned in `gameports/catalog.json` by URL and
SHA-256. Source and game content remain separate inputs.

## Current status

**COMPILED / TESTING — approved open3 build.**

The clean 24 October AmigaOS build with GCC 16.2.0-open3 produces SHA-256
`ce39e3a7c020194d3a6249e4f28477f2d8c64c9211f7f0f0cdf9254c421ea6fe`.
This supersedes the provisional old-stove binary. It is not a release.

Runtime testing found and fixed native Amiga volume paths being prefixed with
Unix `/`, and enabled ZIP content support in the cross profile. An interim
direct-content build reached the main menu, new game and Sol solar-system view,
then returned to Shell with RC 0. The final packaged-content binary also reached Sol and returned RC 0.
See [the exact run receipt](evidence/runtime-20261024.json) for environment,
hashes and remaining gates.

## Portability work

The build recipe keeps the upstream source pristine and applies the Amiga
adaptation to its copied build tree. The important target fixes are:

- reuse UQM's AROS cross-build profile as a generic big-endian m68k profile;
- use the AmigaOS/OpenGPU SDL2, GL and pthread personality;
- supply `NAME_MAX=255` for Amiga where POSIX `_POSIX_NAME_MAX` is absent;
- use libnix `readdir()` rather than absent `readdir_r()`;
- preserve native Amiga volume/assign paths at the physical UIO boundary;
- enable ZIP I/O for the official `.uqm` content packages;
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
the pinned UQM 0.8.0 content archive from `gameports/runtime-data.json`,
kept external to the repository. The fetcher verifies its SHA-256. Set the
content directory to a folder containing `version` and a `packages` subfolder
with `uqm-0.8.0-content.uqm`. Pass a writable directory with `--configdir`.

Required first visible gate:

1. main menu;
2. start a new game;
3. solar-system view;
4. one alien conversation screen;
5. then input/audio/save/clean-exit qualification.

**DONE means RELEASE.**
