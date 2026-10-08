# Chocolate Doom — AmigaOS 3.x / AmigaChrome

Target: **AC090 / AmigaOS 3.x / 68040 + FPU**

Pinned upstream commit: `895f581c5d91497bdda0516612da803fe5843e28`.

## Status

**COMPILED — provisional old-stove artifact.**

A pristine local archive of the exact pin rebuilt successfully with no network
access using `build-amigaos3.sh`.

Clean provisional SHA-256:

`d3ae7440920fe9bc2213a945c5aeec3576f7a4405147d86f9774d654a63b16b0`

The binary is not a release artifact. It must be rebuilt and runtime-qualified
with the approved fixed GCC stove.

## Platform route

- SDL2 presentation/input → Open-family SDL/OpenGPU/OpenInput stack;
- SDL2_mixer → current SDL/AHI audio route;
- SDL2_net disabled for the first runtime gate;
- target flags: 68040 + FPU.

## Runtime data

Chocolate Doom requires a compatible IWAD. No Doom IWAD is stored here.
Freedoom or another explicitly authorised compatible IWAD may be used for
runtime qualification.

## Release gate

Title/menu → start map → framebuffer/rendering → keyboard/controller → audio →
save/config → clean exit, then repeat on a clean fixed-stove instance.

**DONE means RELEASE.**
