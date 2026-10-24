# Open3 SDL2 gameplay candidate

Status: **TESTING**, not a release. This is the normal OpenJazz executable,
not the separate asset-loader probe. The registered data remains external.

## Offline clean build

```sh
python3 ports/openjazz/build_open3.py \
  --source /path/to/local/openjazz-git-cache \
  --stove /path/to/verified-open3-stove \
  --out build/jazz-gameplay-a --jobs 2
```

The recipe exports the catalog's exact commit, applies every checked port patch
without fuzz, and verifies the open3 compiler/runtime inventory. It never patches
the source cache, downloads data, installs modules or deploys an executable.
SDL2 comes from the selected stove. Networking and scaling are disabled;
68040 + FPU and the Amiga null-pointer compiler workaround are enabled.
The receipt records patch, SDK and executable hashes and the actual commands.
The pinned commit supplies SOURCE_DATE_EPOCH; compiler path mapping removes the
export directory from source-location strings. Two independent clean exports
produced identical binaries; see [the evidence](evidence/gameplay-20261024.json).

## Isolated guest run

Copy `engine/OpenJazz` to a dedicated test folder, keep the original game files
in its `data` subfolder, and create an empty writable `User` folder. Use the
existing platform SDL2.module. Do not replace the live machine's libraries.

```text
SetEnv SDL_OPENGPU 0
CD DH1:Games/OpenJazz-Test/User
DH1:Games/OpenJazz-Test/OpenJazz --mute --window DH1:Games/OpenJazz-Test/data
```

For a direct Diamondus Level One check, add `--world 0 --level 0` before the data
argument. This still displays the normal difficulty selector; activate the window
and confirm Medium with Enter. Arrow keys move, Space jumps, Left Alt fires,
and Escape opens the in-game menu. These are upstream bindings, not a complete
list of tested controls.

## Observations and remaining gates

On 24 October, the isolated AC090 lab displayed the normal menu and animated
attract demo. The direct-level run displayed Diamondus Level One without the
DEMO overlay. Right-arrow input moved the player, scrolled the camera into the
cave and collected items, with the score increasing. Escape opened the in-game
menu. Those observations establish a bounded gameplay smoke test, not a full
level completion or performance qualification. The runtime's FPS counter is
not an engine frame-rate measurement.

The run used SDL2's **software** renderer with audio muted, the recorded existing
SDL2.module, and the deployed host runtime. It did not use the newly merged SDL3
renderer. The lab uses a copied Kickstart 3.2.3 ROM, not the current OpenKick ROM;
Instance-32 was untouched.

Closing the first window reached the engine's shutdown log and wrote its
configuration, but did not return a Shell exit code before the lab was stopped.
This is an open shutdown failure, not a clean-exit pass. A mouse attempt to select
Save Game opened Load Game instead; pointer selection needs a focused coordinate
check before blaming either the game or SDL2. No save/load pass is claimed.
Jump was sent but not captured in flight, so it is not marked passed.

Next gates: confirm input release/focus and pointer targeting; isolate the exit
failure; verify config across a fresh launch; exercise save/load, jump/fire,
fullscreen/resizing, controller and audible AHI output. The existing missing
sound-reference warnings remain open. Repeat with the current supported SDL2
runtime and a clean intended ROM before release. No SDL2 backend changes or
private replacement libraries are included.
