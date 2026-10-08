# AssaultCube — AmigaChrome source intake

**Status:** SOURCE PINNED / BUILD NOT STARTED  
**Priority:** first 3D port

## Upstream

- repository: https://github.com/assaultcube/AC.git
- pinned commit: `13f0d8eea4822dee5c976661218d022be74342e3`
- source licence: Cube/AssaultCube zlib-like licence; see upstream `source/README.txt` and `source/README_CUBEENGINE.txt`
- media: mixed per-asset/package licences. Treat game data separately from the engine source.

## Why this is first

The client is small by modern 3D-game standards and already uses a traditional C++/SDL2/OpenGL stack. Its build currently links SDL2, SDL2_image, OpenGL, OpenAL, Vorbis, zlib and ENet. That makes it a useful first target for the AmigaChrome SDL/OpenGPU/OpenRTG path without dragging in a modern AAA engine.

## First porting scan

1. Add an Amiga/AROS platform stanza rather than changing the upstream Linux one.
2. Build the dedicated server first: it avoids SDL/OpenGL/OpenAL and gives an early endian/network/toolchain check.
3. Build the client with graphics/audio/network features individually gated.
4. Route rendering through the AmigaChrome GL/Warp3D/OpenGPU compatibility layer when that layer is ready.
5. Keep online protocol behaviour compatible; platform fixes are fine, gameplay/cheat changes are out of scope.

## First-light gate

Executable starts -> menu -> offline bot map -> movement/input -> textured 3D scene -> audio -> clean exit.

No upstream source or game data is committed to this repository.
