# C-Dogs SDL for AmigaOS 3.x (work in progress)

Upstream: [cxong/cdogs-sdl](https://github.com/cxong/cdogs-sdl) (GPL-2.0)
at `8263a6f0200a71498f8c47bd0fce0f2d2d678d84`, the pin used for the AROS
port. Target: AmigaOS 3.2.3, 68040 + FPU, AGA or RTG (OpenRTG), through
openamigasdl's SDL 2 (DalsinAI/openamigasdl, branch
claude/project-thread-j9wchb).

    ports/cdogs-sdl/build.sh SOURCE OPENSDL_DIR [OUT]

## State (6 Oct 2026, paused)

- **Builds:** `cdogs-sdl` links (2.3 MB, C-Dogs' own -O2, `-m68040 -m68881`)
  with the OS 3.2 stove. No editor; network play stubbed.
- **Stages:** data, graphics, missions and dogfights as upstream ships them.
  The 331 sounds are converted from OGG to 22 kHz mono WAV, because our
  SDL2_mixer reads WAV only. Music stays out (OGG): no music yet.
- **Runs: not yet.** The first run on OpenRTG stops in `SDL_Init`:
  "SDL not built with haptic (force feedback) support". C-Dogs asks for
  SDL_INIT_HAPTIC, and our SDL has only the dummy haptic driver.
- **Not done yet:** an icon with STACK and ACGAME_DISPLAY ToolTypes; nothing
  was tested in a game.

## What the patch changes

`patches/0001-amigaos3-bootstrap.patch`, against the pin:

| File | Change |
| --- | --- |
| `CMakeLists.txt` | `AMIGA_OS3` option: editor off, the vendored nanopb, SDL2 and SDL2_mixer as imported targets from `OPENSDL_DIR`. |
| `src/cdogs/enet/amiga.c`, `include/enet/amiga.h`, `enet.h`, `unix.c`, `CMakeLists.txt` | ENet's platform layer for AmigaOS 3.x without a network: initialising succeeds, every socket call fails, so `enet_host_create` returns NULL and the game carries on without network play. |
| `src/cdogs/cwolfmap/zip/miniz.h` | `ftell`/`fseek` on AmigaOS (libnix has no `ftello`/`fseeko`). |
| `src/cdogs/rlutil/rlutil.h` | No termios in libnix: `getch` reads, `kbhit` returns 0. |
| `src/cdogs/find_steam_game.h` | No Steam on AmigaOS: the lookup finds nothing (no `getpwuid`). |

`amigaos3.cmake` is the CMake toolchain for bebbo's m68k-amigaos-gcc.

## Next

1. Drop SDL_INIT_HAPTIC on AmigaOS (or let SDL report no haptic devices).
2. Config path: it looks for `SYS:Prefs/Env-Archive/.config/...`; point it at
   `PROGDIR:` in `files.c`, as the AROS profile does.
3. Title/menu, keyboard and joystick, one campaign room, on AGA and OpenRTG;
   frame times; clean exit.
4. Icon with STACK and ACGAME_DISPLAY ToolTypes.
5. Music: an OGG decoder for SDL2_mixer (stb_vorbis) or converted modules.
