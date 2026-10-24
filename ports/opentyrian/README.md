# OpenTyrian — active open3 intake

Source pin: `ac5d0e71f4245744350c6116ce24f10624da58f8`,
[upstream](https://github.com/opentyrian/opentyrian). GPL-2.0-or-later engine.
SDL2, 68040 + FPU, libnix, GCC 16.2.0-open3. Networking disabled.

Prepare with `scripts/game_port_prepare.py prepare --game opentyrian --allow-network`.
Build the exported pin offline, applying only the checked-in native-path patch:

```sh
python3 ports/opentyrian/build_open3.py --source build/game-ports/sources/opentyrian \
  --stove /path/to/verified-open3 --out build/opentyrian-open3 --jobs 2
python3 scripts/fetch_runtime_data.py --root . --game opentyrian --allow-network
```

The helper writes `receipt.json` and `build.log`; it does not deploy or fetch.
Put the binary beside `data/` with the contents of the upstream-linked Tyrian
2.1 ZIP (flat, lowercase names). User files go in `PROGDIR:User`.
Run `opentyrian --no-sound --no-joystick --data=data` for the muted smoke.
The data digest is pinned in the runtime manifest. Assets stay external;
freeware is not permission to redistribute every asset without review.

## Observed 24 October 2026

Clean compile/link passed. Executable SHA-256:
`7b05261b65ed89aee05cf8c45e81027587f85718b7ac8af600aa91104e5951d3`.
Compiler archive SHA-256:
`9fcc136ab59e3bb5b91d2764d0bf290b709d72d7380be4ce0cdf005edf8eff21`.

Isolated copied lab, runtime `zen x86-64-v3 20261024T070109Z`, Kickstart
3.2.3 47.115, CRC32 `b18d3b67`, SDL_OPENGPU=0, muted. Title and moving
attract demo rendered, but the 320x200 window appeared to clip the scaled
picture. Keyboard/menu/gameplay control is not qualified. Both launches
exited via Escape with Shell RC 0 and created configuration/save-container
files under User. Creating a save container is not a gameplay save/load pass.

Remaining: full-frame presentation, deliberate menu/ship movement/fire,
actual game save/load, audio, input devices and repeat on intended ROM/runtime.
This is TESTING, not RELEASE. The isolated lab was stopped after the run;
Instance-32 and platform repositories were untouched.
