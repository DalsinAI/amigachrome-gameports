# ACGame AGA/040 port layer

ACGame is the small common layer for AmigaChrome game ports targeting an AGA A1200-class machine with a 68040 and large Zorro III Fast RAM.

The first rule is architectural: Fast RAM is the workspace; Chip RAM is the presentation boundary. A large Z3 allocation does not make AGA DMA read Fast RAM.

## First implementation checkpoint

This directory currently contains two portable reference primitives:

- `acgame_c2p8_ref`: 8-bit chunky to eight AGA-style bitplanes. It is deliberately simple and correct, not the final 68040-optimized converter.
- `acgame_ham8_encode_row`: greedy RGB-to-HAM8 scanline encoder with a 64-colour base palette, plus a matching decoder for tests/offline tooling.

Gameplay should normally use indexed 256-colour AGA. HAM8 is intended for title art, portraits, briefings, loading screens and other static or slowly changing presentation where horizontal dependency is acceptable.

## Planned Amiga backend

The next platform layer will provide:

- 320x200, 320x240 and 320x256 indexed AGA surfaces;
- two Chip-RAM planar display buffers;
- Fast-RAM chunky render surfaces;
- 68040-specialized C2P selected behind the same API;
- HAM8 display setup and pre-encoded presentation frames;
- keyboard, mouse and joystick input;
- Paula-first audio with an optional AHI backend;
- timer and file wrappers suitable for AmigaOS 3.x and AROS 68k.

No claim is made yet that these reference routines open an Amiga screen or that any of the four games links on 68k. They are the tested conversion seam on which those ports can converge.

