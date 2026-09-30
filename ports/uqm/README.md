# The Ur-Quan Masters 0.8.0 — m68k AROS

**Status:** COMPILED / RUNTIME TEST PENDING

## Upstream

- source archive: https://downloads.sourceforge.net/project/sc2/UQM/0.8/uqm-0.8.0-src.tgz
- source archive SHA-256: `24f2f7db9cf7faf53b95f9e2580e6f596205a98ed0c335cfe834c64785ad4f5a`

## Compile result

- target: m68k AROS / 68040
- executable: `uqm-aros`
- observed size: 2,039,984 bytes
- SHA-256: `6cec31c74a6e5425723dd0fdedf5bb670c3741b277a2a1215619bbe161c052fc`

## Build shape

The successful bootstrap uses the AROS m68k cross tools through wrapper names expected by UQM's build system.

Environment highlights:

    BUILD_HOST=AROS
    BUILD_HOST_ENDIAN=big
    AROS_SDK=$AROS_SYSROOT
    CFLAGS="--sysroot=$AROS_SYSROOT -m68040"
    CXXFLAGS="--sysroot=$AROS_SYSROOT -m68040"
    LDFLAGS="--sysroot=$AROS_SYSROOT -m68040"

First-light configuration uses SDL2 graphics, internal MixSDL, disables Ogg codec support and disables netplay.

## Content

The official UQM 0.8.0 base content package is used locally for testing and is not committed here.

Expected layout:

    content/packages/uqm-0.8.0-content.uqm

Launch:

    uqm-aros -n content

## Gate

Main menu -> new game -> solar-system view -> one alien conversation -> input/audio -> clean exit.
