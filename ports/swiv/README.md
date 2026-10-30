# SWIV on RTG

Dale, 30 October 2026: "port swiv to rtg ... really upgrade the graphics but
keep the gameplay".

SWIV (Storm, 1991) has no released source. So the gameplay is kept the only
exact way: **the original game code runs unchanged** on the 68040, from the
player's own SWIV disks, read at run time. Nothing of the game is in this
repository or in any build. What is ours is the picture: once a frame we read
what the game has given the chipset and redraw it, upscaled, on an RTG screen.

## Stages

1. **nativertg** (here, done): turns an OCS/ECS chipset picture into a
   true-colour picture 2, 4 or 8 times the size.
   - Bitplanes 1-6 with BPLCON1 delays, single and dual playfield (PF2PRI),
     extra half-brite.
   - The eight hardware sprites, attached pairs, and BPLCON2's PF1P/PF2P
     priorities. Single playfield counts as playfield 1 for sprite priority,
     as the runtime's own Denise (`amigachrome` gfx.c) does.
   - A pixel-art upscaler of our own: each pixel becomes 2x2 and a corner
     takes its neighbours' colour only where two edges meet on a diagonal,
     so curves and slopes come out smooth while flat areas, straight edges
     and the game's colours stay exact (no new colours, no blur). 4x and 8x
     repeat it.
   - 12-bit OCS colours spread to 24-bit.
   Plain C99, no hardware access: the same code builds for the 68040 and for
   the host tests.
2. **The SWIV hook** (next, needs the game disks): a loader of our own that
   boots SWIV from the player's disk images, finds where the game keeps its
   copper list, bitplane and sprite pointers, and once a frame fills a
   `nativertg_frame` and presents the upscaled picture on the RTG board
   (ACGame's RTG screen, `WritePixelArray`, or straight into the board's
   memory while the game has the machine). The game's own timing, input
   and sound stay as they are.
3. **Further graphics upgrades** (after 2 works): a replacement table keyed
   by a hash of each sprite or tile as it is blitted, so redrawn high
   resolution art can stand in for the original pieces one by one, and a
   true-colour background layer for the scrolling map.

## Test

```sh
tests/run_tests.sh          # host: compose, dual playfield, EHB, sprites, scaler
cc -std=c99 -O2 -Iinclude src/nativertg.c tools/nativertg_demo.c -o demo
./demo 4 native.ppm upscaled.ppm   # a made-up scene, no game data
```

Not yet built with the os32-gcc16 stove or run on an Amiga: the cloud has no
m68k compiler and the dev machine was not reachable on 30 October.

Copyright (c) 2026 AmigaChrome Team
