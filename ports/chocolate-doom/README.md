# Chocolate Doom — m68k AROS

**Status:** COMPILED / RUNTIME TEST PENDING

## Upstream

- repository: https://github.com/chocolate-doom/chocolate-doom.git
- pinned commit: `895f581c5d91497bdda0516612da803fe5843e28`

## Compile result

- target: m68k AROS / 68040
- executable: `chocolate-doom`
- observed size: 1,689,112 bytes
- SHA-256: `b99fe3a2b55497bce6240413474c899b3ba4da2cf9a22140c8e46f561af04b5f`

## Required compatibility shim

The AROS SDK provides static `libSDL2_mixer.a`, while Chocolate Doom's CMake expects the target `SDL2_mixer::SDL2_mixer`.

For the successful build we created an imported CMake target with that name, pointing at:

    $AROS_SYSROOT/lib/libSDL2_mixer.a

and exposed:

    $AROS_SYSROOT/include/SDL2

SDL2 networking was disabled for first light.

## First-light data

Use **Freedoom** or a user-supplied compatible IWAD. No Doom IWAD is stored here.

Suggested first test:

    chocolate-doom -iwad freedoom1.wad

## Gate

Title/menu -> start map -> software framebuffer -> keyboard/controller input -> audio status -> clean exit.
