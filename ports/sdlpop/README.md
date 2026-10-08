# SDLPoP — AmigaOS 3.x / AmigaChrome

Target: **AC090 / AmigaOS 3.x / 68040 + FPU**

Pinned upstream commit: `3c5add5fb7f83d4ceb542823ab66d00146c4271b`.

## Status

**COMPILED — provisional old-stove artifact.**

A pristine local archive of the exact pin rebuilt successfully with no network
access using the checked Amiga/OpenGPU patch.

Clean provisional SHA-256:

`51097f931538140895659af40f021d58ff20c348751c174bd093881c23600f6a`

The Amiga portability patch supplies the expected stack allocation behaviour,
uses `PROGDIR:` as the runtime directory, avoids the unavailable `log2f`
path, and applies the same allocation bridge inside stb_vorbis.

## Platform route

- SDL2 presentation/input → Open-family stack;
- target flags: 68040 + FPU;
- runtime path rooted at `PROGDIR:`.

## Runtime data

The source tree contains the engine runtime data used by the port, but original
commercial Prince of Persia material is not silently bundled or fetched.

## Release gate

Start game → level one → animation/timing → keyboard/controller → audio →
save/config → clean exit, followed by a fixed-stove clean-instance repeat.

**DONE means RELEASE.**
