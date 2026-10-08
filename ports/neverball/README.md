# Neverball / Neverputt — OpenUp release port

**Release rule:** this port is not DONE until the packaged build has passed the runtime gates below.

## Architecture

This is intentionally not a separate SDL/Mesa port.

- SDL 2 API: OpenGPU's `SDL2.module` via its `libSDL2.a` stub.
- OpenGL: OpenGPU's `GL.module` / `libGL.a`.
- Audio: OpenGPU SDL audio backend to AHI.
- Game controllers: SDL joystick API routed through `openinput.library`.
- Keyboard/mouse/timers: OpenGPU SDL native Amiga backends.
- CPU target: 68040 + FPU, the canonical OpenUp game target.

The upstream game/core code remains unchanged unless a proven Amiga-specific defect requires a patch.

## Upstream pin

Neverball 1.6.0, commit:

`a1ed09911dca262d80049c12a2824d683af494d6`

GPL-2.0-or-later. The package carries the upstream licence/notices and bundled data licences.

## Build

```sh
export OPENUP_SDK=/path/to/open-gpu-sdk
export PATH="$OPENUP_SDK/bin:$PATH"
sh ports/neverball/build.sh build/game-ports/sources/neverball
```

The result is `build/neverball-release/package/Neverball/`.

## Release gates

All are mandatory:

1. Reproducible clean cross-build of Neverball and Neverputt.
2. OpenGPU runtime modules load from a clean OpenUp installation.
3. Title screen and at least three Neverball levels render correctly.
4. Neverputt starts and completes one hole.
5. Keyboard controls work.
6. Mouse focus/capture/release works without stuck input.
7. OpenInput controller enumerates and controls menus and gameplay.
8. Music and effects play through AHI with no clipping/stall.
9. Windowed/fullscreen transitions survive where supported by the runtime.
10. Save/config/high-score writes succeed beneath `PROGDIR:User`.
11. Clean exit returns to Workbench with no lingering task or open screen.
12. Package contains all required upstream notices and SHA-256 inventory.
13. Test is repeated on a clean instance, not only the development instance.

Only after all thirteen gates pass may README/PORT_STATUS say **RELEASE**.
