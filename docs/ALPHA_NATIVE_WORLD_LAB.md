# Native Alpha world in FS-UAE

`scripts/run_alpha_native_world.py` reproduces the independent native world
laboratory. It runs the actual Amiga HUNK executable, original demo art/fonts,
official Borehole scenario and genuine OpenGPU SDL module inside AROS m68k.
The capture helper reads FS-UAE's actual OpenGL framebuffer; the input helper
queues real SDL events through FS-UAE. Neither substitutes game or emulator code.

Prepare the native executable and SDK with the frontend build scripts and
`ALPHA_NATIVE_SDK.md`. The tested AROS lane needs the documented libnix FPU40
startup compatibility patch. This does not qualify native AmigaOS3.2, full
turns, AI, combat or unsupported movement.

```sh
python3 scripts/run_alpha_native_world.py \
  --aros-disk /path/to/extracted/aros-disk \
  --assets /path/to/prepared-original-demo-assets \
  --world-binary /path/to/alpha-original-view-local-final \
  --fs-uae /path/to/FS-UAE/Linux/x86-64/fs-uae \
  --sdl-headers /path/to/opengpu-build/include/SDL2 \
  --sdl-module /path/to/opengpu-build/SDL2.module \
  --out /path/to/new-run-directory
```

The source disks are copied into the new run directory and are never mounted
writable. An existing output directory is refused, including an output nested
inside either source disk. The copies receive their own startup script,
screen preferences and executable/module. Do not run the source disks elsewhere
while copying them. Disk copying follows symlinks into independent copied files.

The tested machine is A1200,68040/FPU,64MiB motherboard RAM, no Z2 Fast RAM,
UAEGFX-Z2 with4MiB RTG memory. AROS ScreenModePrefs selects the observed mode
`0x10071400`,1280×1024×16. The guest uses a1MiB stack and sets
`SDL_OPENGPU=0` and `SDL_RENDER_DRIVER=software`. This remains the actual
OpenGPU SDL implementation using its software presentation path.

The copied DH1 directory must contain `alpha.txt`, `Borehole.SC`, `texture.pcx`,
`Units.pcx`, `console.pcx`, `ALPHC___.TTF`, `arialn.ttf` and the faction art/model
files referenced by the scenario. Use `stage_alpha_demo_assets.py` for original
demo preparation; no proprietary files are committed by this runner.

Default input first sends18 TAB presses to reach supported infantry in this
pinned scenario, then frames2700/3400/4000/4600 send keypad northwest,V,R,Q.
The first selected owned vehicle is an unsupported transport. The move
can still be refused by the bounded controller; that refusal is meaningful and must
not be reported as successful movement. `--sequence` accepts explicit
`frame:SDL-scancode:SDL-keycode` triples. `--seconds` defaults to120. Frames are
captured every250 swaps, plus150/500/900; host Pillow optionally converts PPM
captures into PNG without altering framebuffer content.

Outputs include `machine.fs-uae`, `emulator.log`, the copied game's
`world-run.log`, actual framebuffers, any newly created checkpoints and
`receipt.json` with hashes and observed guest actions. Exit0 requires a native
world draw, a captured framebuffer and actual guest `WORLD_RC 0`. Host emulator
timeout is recorded separately because the guest OS can remain running after
the world exits. The smoke receipt does not certify successful moves or saves;
inspect the recorded native action messages and checkpoint files.

`--prepare-only` stages both disks and compiles the genuine helpers without
launching FS-UAE. It explicitly records that no runtime test occurred.
