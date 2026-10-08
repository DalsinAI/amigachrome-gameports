# C-Dogs SDL — AmigaOS 3.x / AmigaChrome

Target: **AC090 / AmigaOS 3.x / 68040 + FPU**

Pinned upstream source:

- repository: `https://github.com/cxong/cdogs-sdl.git`
- commit: `8263a6f0200a71498f8c47bd0fce0f2d2d678d84`

## Platform route

The port keeps C-Dogs game logic and routes platform services through the Open-family stack:

- SDL2 video/presentation → OpenGPU
- SDL2 controller path → OpenInput
- SDL2 mixer/audio → OpenGPU/AHI path
- Amiga preferences → `PROGDIR:Prefs/`
- network play → currently disabled by an Amiga ENet stub; OpenSocket integration is the intended replacement

The build target is 68040 + FPU.

## Current status

**TESTING — old-stove artifact only.**

A clean AmigaOS executable and engineering package have been produced with the
old GCC 16.2.0b stove. On an isolated AC090 lab it reaches the executable,
loads SDL/OpenGPU and the bundled data tree, then raises Amiga CPU alert
`0x80000005` before a usable game frame.

The old GCC stove is known to miscompile real AC090/68040 programs, so this
runtime result is preserved for A/B testing. Do not patch the game around that
alert until the approved fixed-stove rebuild has been tested.

## Data / publication status

Upstream describes current code as GPLv2 with BSD-licensed components and data
as a mix of CC0, CC-BY and CC-BY-SA. However, upstream's own
`doc/README_DATA.md` preserves historical caveats about some legacy
contributed campaigns and original sound samples.

Therefore the current full-data package is for engineering/testing only.
Do not publish a binary+data release until the asset provenance gate is closed.

**DONE means RELEASE.**
