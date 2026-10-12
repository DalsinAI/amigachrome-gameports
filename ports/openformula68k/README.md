# OpenFormula 68K

OpenFormula 68K is an original Formula / Grand Prix racing game for AmigaOS
3.x RTG systems. It is not a port or clone of a commercial racing game and it
contains no licensed Formula One teams, drivers, circuits, cars or logos.

The first playable vertical slice targets the AmigaChrome AC090 but remains a
normal AmigaOS program:

- 68040 plus FPU reference target;
- 640x360 indexed RTG framebuffer;
- one chunky-screen presentation per frame through ACGame/OpenRTG;
- OpenGfx acceleration when present;
- keyboard, OpenInput controller and lowlevel joystick paths;
- generated AHI engine, wind, impact and countdown audio;
- no external game assets.

## Gameplay in 0.1

- twelve-car race at the fictional **Boing Ring**;
- three laps by default;
- start grid and countdown;
- arcade-sim acceleration, braking, grip and off-road slowdown;
- hills, sweeping bends, hairpins and chicanes;
- AI speed and racing-line changes;
- collisions, race position, lap timing and best lap;
- title and chequered-flag screens.

## Build

```sh
STOVE=~/AmigaChrome/stoves/os32-gcc16 \
  sh ports/openformula68k/build.sh
```

The output is:

```text
build/openformula68k/OpenFormula68K-0.1.0.zip
```

## Run

```text
Stack 600000
OpenFormula68K
```

Options:

```text
LAPS=n
SECONDS=n
NOAUDIO
AUTOSTART
AUTO
AGA
```

The supported default is RTG. `AUTO` permits the ACGame backend to choose RTG
or AGA; `AGA` explicitly exercises the planar compatibility path.

## Controls

```text
Cursor left/right      steer
Cursor up / Fire 1     accelerate
Cursor down / Fire 2   brake
Return / Play          start
Escape                 exit
```

## Roadmap

The next passes are wet-weather grip and spray, qualifying, pit stops,
car setup, damage, a second circuit, replay/ghost laps, proper Workbench
GlowIcons and a direct test campaign on Instance-32.
