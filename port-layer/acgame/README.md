# ACGame AGA/040 port layer

ACGame is the small common layer for AmigaChrome game ports targeting an AGA A1200-class machine with a 68040 and large Zorro III Fast RAM.

The first rule is architectural: Fast RAM is the workspace; Chip RAM is the presentation boundary. A large Z3 allocation does not make AGA DMA read Fast RAM.

## What is here

- `acgame_c2p8_ref` and `acgame_c2p8_fast`: 8-bit chunky to eight AGA-style
  bitplanes. The reference is simple and correct; the fast one gives the same
  bytes through an 8x8 bit transpose in 32-bit registers and is what the
  backend uses.
- `acgame_ham8_encode_row`: greedy RGB-to-HAM8 scanline encoder with a 64-colour base palette, plus a matching decoder for tests/offline tooling.
- The Amiga backend (`src/platform_aros_aga.c`, AmigaOS 3.x and AROS m68k):
  AGA or RTG 8-bit screens, keyboard, mouse, and pads through OpenInput with
  lowlevel.library as the fallback. See `AROS_AGA.md`.

Gameplay should normally use indexed 256-colour AGA. HAM8 is intended for title art, portraits, briefings, loading screens and other static or slowly changing presentation where horizontal dependency is acceptable.

This copy (amigachrome-guest `gameports/acgame/`) is the one the game ports
build from: amigachrome-gameports' `scripts/build_game_ports_job.py` and the
OpenOMF profile take ACGame from here. amigachrome-gameports
`port-layer/acgame/` holds the same files.

## Still to come

The platform layer is still to provide:

- 320x200, 320x240 and 320x256 indexed AGA surfaces;
- two Chip-RAM planar display buffers;
- Fast-RAM chunky render surfaces;
- HAM8 display setup and pre-encoded presentation frames;
- Paula-first audio with an optional AHI backend;
- timer and file wrappers suitable for AmigaOS 3.x and AROS 68k.

The backend opens AGA and RTG screens on OS 3.2.3 and reads pads through OpenInput (tested on an AmigaChrome lab instance, 9 October 2026). No claim is made yet that any of the four games links on 68k against this backend.


## Canonical copy

The canonical ACGame source currently lives in `DalsinAI/amigachrome-guest/gameports/acgame/`.
This copy is kept as a synchronized mirror for the game-ports repository. The Kitchen build job
and the OpenOMF AGA profile consume the guest copy at the pinned `GUEST_PORT_COMMIT`; changes
must land there first so the two trees do not become independent implementations.
