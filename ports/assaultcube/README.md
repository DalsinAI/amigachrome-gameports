# AssaultCube — AmigaChrome source intake

**Status:** AMIGA CLIENT CROSS-BUILD GREEN VIA OPENGPU SDK / RUNTIME FIRST-LIGHT NEXT  
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

## Amiga server first light

The pinned source now cross-builds a native AmigaOS `ac_server` with the gcc16 68040/FPU stove. The bootstrap patch removes Linux-only `-rdynamic`, supplies Amiga fallbacks for directory iteration, filesystem free-space probing, thread naming and millisecond sleeps, and keeps the standalone server SDL-free. ENet is built static for m68k and links through libnix `libsocket`, `libpthread` and the existing Amiga zlib archive.

Current proof: `ac_server` is an AmigaOS loadseg executable (about 742 KiB). This is a compile/link gate only; runtime server validation is still required.

## OpenGPU SDK client first light

The native client now cross-builds against the packaged OpenGPU SDK in the gcc16 stove rather than loose SDL/GL artefacts. `sdl2-config` supplies SDL 2.32.10, libGL, the 68040/FPU ABI flags and the GCC 16 loop-miscompile workaround. The Amiga client requests a 1 MiB stack as required by the OpenGPU GL runtime. Graphics, input, networking, maps, bots and game logic link successfully. Audio is deliberately stubbed for the first runtime proof and will move to OpenAudio after graphics/input first light.

## Open2 stream and allocation follow-up, 12 October 2026

Patch 0003 makes `getchar`, `putchar` and `putstring` out-of-line on AmigaOS
only. The HUNK linker was comparing different optimised inline copies; the
patch supplies one definition without changing the stream contract or global
optimisation flags. Non-Amiga definitions remain inline. Patch 0004 pairs
`newstring` (array allocation) with `delete[]` in `gmode_parse`.

Both client and server rebuilt with Open2. The targeted duplicate-section and
array-deletion warnings are absent; this does not claim every warning is gone.
A diagnostic using the exact new method bodies passed 21 checks in an isolated
AmigaOS guest, in addition to host regression tests. Actual AssaultCube
client/server runtime and audio integration remain unqualified.

[Raw evidence and hashes](../../evidence/open2-20261012-followup/README.md).

## Reproducible source preparation

Both Amiga recipes verify the checkout HEAD against `gameports/catalog.json`
and check each required patch in forward or reverse direction. The server
applies 0001, 0003 and 0004; the client also applies 0002 automatically. A repeat
build accepts those exact patches already applied; a mismatching patch fails
with its filename. The old server grep for one `__amigaos__` occurrence could
skip an incomplete bootstrap, and the client previously required manual 0002
application.

Use a disposable checkout at the pin above. `STOVE` is a stove root containing
`prefix/` and a verified compiler receipt; current recipes require open3.
Build the ENet/server first, then the client in the same checkout:

```sh
export STOVE=/path/to/verified-open3-stove
export ASSAULTCUBE_SRC=/path/to/pinned-assaultcube/source
sh ports/assaultcube/build-amiga-server.sh
sh ports/assaultcube/build-amiga-client.sh
```

The client now gives an explicit server/ENet prerequisite error when the archive
is absent. This checks existence, not the provenance of an arbitrary preexisting
ENet archive: start from a clean checkout and build it through the server recipe.
The first-light audio stub remains; these changes add no audio or runtime pass.
