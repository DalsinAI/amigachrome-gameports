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

## Paths and music

The game is built with `DATADIR="PROGDIR:data"`, `USERDIR="PROGDIR:User"` and
`LOCALEDIR="PROGDIR:locale"`. `PROGDIR:` is the home folder of the process
that has it: a thread, or any library working in another process, asking for
`PROGDIR:data/...` makes AmigaDOS ask for a volume, and the game's own base
folder joined with a "/" (`./PROGDIR:data`) names a volume called
`./PROGDIR`. So `patches/0002` turns `PROGDIR:` into the program's real
folder when the game starts (`GetProgramDir()` and `NameFromLock()`, in
`fs_init`, before anything reads a path), and builds the data, user and locale
folders from that: every path the game uses is a full path
(`DH1:Games/Neverball/data`). `path_is_abs` knows a volume's `NAME:` as a full
path on the Amiga. Run from a Shell, from a drawer, from Workbench or by
`Run`, it finds its data the same way.

The music is read through the game's own file system, like the sounds, and
handed to SDL2_mixer from memory (`Mix_LoadMUS_RW`): the tracks are in the
data archive, not files on the disk, so there is no name to give the mixer.

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

## AmigaNeverBall (OpenUp, 10 October 2026)

`patches/0003` names the game AmigaNeverBall on its title screen (white over red, the Boing
Ball's colours) and in its window title, and makes the Boing Ball the default ball. The ball is
`overlay/data/ball/boing-ball` (our own red and white checks); `build.sh` copies it beside the
release data and makes its solid from the basic ball's in that data. The other balls stay in
Options > Ball.
