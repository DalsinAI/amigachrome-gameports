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
   Progress (31 October): `tools/swivharness.py` runs the game's own code
   from the player's disk image under Unicorn (a 68000 core used only as a
   study tool on the host), with a floppy that answers DMA reads with
   AmigaDOS MFM sectors built from the ADF, the vertical blank and copper
   interrupts, a copper pass per frame, an area-mode blitter, and CIA
   ports. It boots the game's kernel ("Kernel V 1.0, RPW and JC, 1991",
   22 sectors at disk offset 0xD9400), loads the file catalogue
   (AMDLS0.CAT, about 130 files: AMPROG.OBJ, the .LIN sprite sets, .RAW
   pictures, .MOD music, .SND effects) and AMPROG.OBJ, and reaches the
   title screen. Every custom-chip write is logged; `--dump` writes the
   display as the copper last set it up, and `tools/nativertg_frame.c`
   draws that through nativertg at 1x and upscaled.
   What the title screen uses: one copper list at $22C00 (double-buffered
   with $23AE0), 4 lores bitplanes 320x256 with 44-byte rows (BPL1MOD 4),
   BPLCON2 $3F (sprites behind nothing), all eight sprite pointers set,
   a fifth plane switched on for a band at lines $34-$3B.
   `--fire FRAME` holds fire for ten frames; with presses at 1500, 2000
   and 2500 the game starts and plays (68,000 blits by frame 3500; no
   line or fill blits seen so far).
   **Engine change** (`tools/swivhires.py`, `--hires`): every bit of chip
   RAM has a 4x4 block of sub-pixels, and every blit the game starts is
   also done on those blocks with the same channels, shifts, masks and
   minterm, so bobs, map tiles and shadows land in a 1280x1024 picture at
   the exact places the game puts them. Graphics a blit reads fresh from
   the disk are upscaled first, in colour: the game draws one bob or tile
   as a run of blits, one per bitplane, so the run's planes and its
   cut-out mask are put back together, upscaled once, and split again.
   Pictures the CPU writes to the screen (title, score panel) are upscaled
   in colour from the screen. Dithered ground keeps its pixels (a texture
   test per pixel); outlines are smoothed. The game's own memory is never
   touched, so play is unchanged. Checked against the original frame:
   in 99.2% of an in-game frame's pixels the 4x4 block is mostly the
   original colour; the rest are smoothed edges and a few specks where a
   cut-out mask and its bob were smoothed apart (to tidy next).
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

The harness needs `pip install unicorn`; its stops are taken at block
starts only, because a stop inside a translated block lost condition codes.

```sh
python3 tools/swivharness.py SWIV.adf out --frames 1500 --dump
cc -std=c99 -O2 -Iinclude src/nativertg.c tools/nativertg_frame.c -o nrtgframe
./nrtgframe out/frame.nrtg 4 native.ppm upscaled.ppm
```

Not yet built with the os32-gcc16 stove or run on an Amiga: the cloud has no
m68k compiler and the dev machine was not reachable on 30 October.

Copyright (c) 2026 AmigaChrome Team
