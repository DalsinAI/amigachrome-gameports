# Alpha Centauri in the browser (WebAssembly)

This directory builds the native Amiga Alpha Centauri port for a web browser.
It is the same program: the title screen (`native/original_frontend.cpp`),
the embedded map window (`native/original_map_window.cpp`), the play layer
(`native/play_session.cpp`, `native/ui_*.cpp`) and the Thinker-based engine
(`native/engine/*.cpp`) are compiled from `../native` without changes, from
the same source list as `native/build_original_frontend.sh`.

No original game data is included or downloaded. The page asks the player for
their own Sid Meier's Alpha Centauri / Alien Crossfire install folder or
files, exactly like the Amiga build reads them from its own directory.

## Files

| File | Purpose |
| --- | --- |
| `build_wasm.sh OUTDIR` | Builds `alpha.js` + `alpha.wasm` and copies `index.html`, `shell.js` into OUTDIR. |
| `index.html`, `shell.js` | The page: file picking, canvas, keyboard/mouse, file requester dialog, saves list. |
| `amiga/` | Browser stand-in for the AmigaOS calls the frontend makes (see below). |
| `amiga/wasm_runtime.cpp` | Engine yield points for long AI turns, the cooperative audio thread, audio call records. |
| `compat/libstdcxx_compat.h` | `std::__gcd` (libstdc++ only; `engine/combat.cpp` uses it), force-included. |
| `compat/sdl_coop_thread.h` | Force-included into `native/audio_service.cpp` only: its music thread runs cooperatively. |
| `seed_ports.sh` | Fills emscripten's SDL2/SDL2_ttf port cache from git tags when its archive download is refused. |
| `test_wasm_play.py` | The native 20-turn play test built as WebAssembly and run under node; must match the host build byte for byte. |
| `make_fixture.py`, `test_wasm_browser.js` | Synthetic stand-in install and a Chromium/Playwright run of the page. |

## Building

Needs emscripten (tested: emsdk 6.0.13, `emsdk install latest`). SDL2 2.32.10
and SDL2_ttf 2.20.2 come from emscripten's own ports (`-sUSE_SDL=2
-sUSE_SDL_TTF=2`). If emscripten cannot download them (it fetches GitHub
archive URLs), run `seed_ports.sh` once; it clones the exact tags the
installed port files name and seeds the cache.

```sh
./emsdk install latest && ./emsdk activate latest   # in an emsdk checkout
# with emsdk's upstream/emscripten on PATH and EM_CONFIG=<emsdk>/.emscripten:
sh ports/alpha-centauri/wasm/seed_ports.sh           # only if port downloads fail
sh ports/alpha-centauri/wasm/build_wasm.sh /tmp/alpha-web
python3 -m http.server -d /tmp/alpha-web 8000        # any static server
# open http://localhost:8000/index.html
```

Output size (emscripten 6.0.13, `-O2`): `alpha.wasm` 3,634,886 bytes,
`alpha.js` 163,840 bytes.

Options: `OPT` replaces `-O2` for every compile, `LINKFLAGS` is added to the
final link only (for example `LINKFLAGS=--profiling-funcs` for named
functions in browser profiles), and `ALPHA_LOCAL_PLAYER_CANDIDATE=1` builds
the same optional local-player candidate as `native/build_original_frontend.sh`
(see "Local-player candidate" below).

## Playing

1. Open the page and choose your game folder (or add files). They are read
   into the tab's memory; nothing is sent anywhere. Movies (`.wve`) and
   programs are skipped; prepared intro movies (`opening.amov`,
   `openingx.amov`) are read. Tick "Keep them in this browser" to store them in
   this browser's IndexedDB for the next visit ("Forget kept files" removes them).
2. The title screen needs `openinga.pcx`, `ALPHC___.TTF` and `arialn.ttf`
   (Arial Narrow; take it from your system fonts if the install lacks it).
   New games need `alpha.txt` / `alphax.txt` and the faction files. The map
   needs `texture.pcx`, `Units.pcx`, `console.pcx`, the faction art and
   `Arial.ttf` (or it falls back to `arialn.ttf`). These are the same files
   the Amiga build lists in `../native/README.md`.
3. Press Start, click the picture to give it the keyboard. Keys and mouse are
   those of the Amiga build (`../native/README.md`, "Playing the engine game"):
   N new game, P play a saved game, X/A choose Crossfire/Alpha Centauri, and
   in the map keypad/digits move, E ends the turn, V saves, Q quits.
   The square at the window's top left, or the "Close window" button, is
   the Amiga close gadget.
4. V writes `AlphaPlay-NNN.SAV`. Saves are listed under "Saved games" (click to
   download) and, with the box ticked, kept in the browser. P on the title
   screen opens a dialog listing your `.SAV`/`.SC` files, or lets you open
   one from your computer.

File names match regardless of case and `PROGDIR:` means the game folder,
as on the Amiga.

### Sound and music

The native audio service (`native/AUDIO.md`: menu music, world music sets,
effects from `alpha-audio.cfg`, faction, technology and facility voices)
plays through SDL2's Web Audio backend. Browsers start sound only after the
player has clicked or pressed a key; the Start button is that gesture, and
any later click resumes a suspended audio context.

The native service reads PCM WAV. Original installs ship voices and music as
MP3 (and some players have OGG/FLAC music), which the Amiga build expects to
be converted beforehand. The page does that itself: after the files are
picked (or restored from the browser), every `voices/*.mp3` and
`music/*.{mp3,ogg,flac,m4a,opus}` without a matching `.wav` is decoded with
the browser's own decoder and written next to it as 16-bit mono 22050 Hz WAV,
the format the native mixer uses. Nothing is uploaded.

The audio service's music feeder thread (`SDL_CreateThread`) has no thread
to run on without pthreads, so `compat/sdl_coop_thread.h` maps it to a
cooperative slot that `amiga/wasm_runtime.cpp` steps once per input wait or
engine yield (one loop iteration each time, at most every 20 ms). The native
source is unchanged.

The "Sound" field under the picture shows the output level, measured by an
analyser on everything the game sends to the speakers.

### Intro movie

`V` on the title screen plays the intro from the prepared movie cache
(`opening.amov` / `openingx.amov`, made by `native/prepare_startup_movie.py`
from the install's `.wve`; see `../native/STARTUP-MOVIES.md`). The original
`.wve` (EA TQI video) has no decoder in the browser and is not read: when an
install has a `.wve` but not its `.amov`, the page says so after the files
are picked and the title screen's own "Intro unavailable" message appears on
`V`; the game continues normally.

### Long AI turns

The engine runs on the page's main thread, as the native program runs on
the Amiga's one task. During End Turn it gives the browser control back at
most every 25 ms: the link wraps (`-Wl,--wrap`) the engine's `debug()`
logger, `Path_find`, the AI tile search step, `iterate_tiles`, `action` and
`order_veh`, plus window drawing, and each wrapper first calls the yield
point and then the original function (ASYNCIFY suspends the call stack and
resumes it after a browser task). Music keeps streaming, the page keeps
painting and taking input, and after 300 ms an "AI turn in progress"
indicator with the elapsed time covers the map until the program waits for
input again.

A Web Worker was the other option. It was not taken: the native frontend's
audio (SDL2's Web Audio backend needs an `AudioContext`, which workers do not
have), the file requester dialog, the canvas and the kept files would each
need a message protocol between worker and page, while the yield points need
no change in any native file and keep everything else as tested.

### Local-player candidate

`ALPHA_LOCAL_PLAYER_CANDIDATE` is a qualification build of the read-only
`L` view: a bounded set of local actions (unit moves, one facility, forecast,
save/reload) on an original base-game save, without End Turn. The engine
play mode (`N` new game, `P` play a saved game) is the full game and is
built by default, so the candidate is not needed to play; it is off by
default here as it is natively. `ALPHA_LOCAL_PLAYER_CANDIDATE=1 build_wasm.sh`
builds it (3,725,477 bytes of wasm), and the browser walk passes on that
build; its `L` path itself needs an original base-game save and was not run.

## How the Amiga parts run in a browser

The frontend talks to Intuition, graphics, CyberGraphX and ASL directly. The
WebAssembly build puts `amiga/include` first on the include path; its
`<exec/types.h>`, `<proto/intuition.h>`, ... forward to `amiga/alpha_amiga_shim.h`,
which declares only what the frontend uses. `amiga/amiga_shim.cpp` implements it:

- `OpenWindowTags` makes an RGB frame; the most recently opened window is the
  one shown on the canvas. `WritePixelArray`, `ReadPixelArray`,
  `FillPixelArray`, `Move`/`Draw` and pens draw into that frame.
- Keyboard and mouse become `IDCMP_VANILLAKEY`, `IDCMP_RAWKEY` (cursor keys,
  F1-F10) and `IDCMP_MOUSEBUTTONS` messages for `GetMsg`.
- `WaitPort` sleeps through emscripten's ASYNCIFY, so the native blocking
  loops run as written while the browser paints and delivers input. SDL's
  `SDL_Delay` (intro movie) does the same.
- The ASL file requester is a page dialog.

SDL2 is used, as on the Amiga, only for audio, WAV loading and SDL_ttf text.
One wrapper header (`amiga/include/SDL2/SDL_ttf.h`) maps SDL_ttf 2.20's
`struct _TTF_Font` tag to the `struct TTF_Font` that `original_ui_text.h`
forward-declares.

Compiler differences: clang warns on brace elision in the engine tables,
GCC-only warning pragmas, set-only globals and one indentation in
`startup_movie.cpp`; those four warnings are disabled, everything else
stays `-Wall -Wextra -Werror`.

## Checks

```sh
# The native play test as WebAssembly under node, against the host build.
python3 ports/alpha-centauri/native/test_play_session.py --out /tmp/play-host
python3 ports/alpha-centauri/wasm/test_wasm_play.py --out /tmp/play-wasm \
  --native-receipt /tmp/play-host/receipt.json
# The page in Chromium (Playwright), with a synthetic install.
python3 ports/alpha-centauri/wasm/make_fixture.py /tmp/alpha-fixture
node ports/alpha-centauri/wasm/test_wasm_browser.js /tmp/alpha-web /tmp/alpha-shots /tmp/alpha-fixture
```

`test_wasm_play.py` builds `native/play_control.cpp` with every source that
`test_play_session.py` uses, plays a new game for 20 End Turns with base,
production, attack, research, social engineering, design, hurry, save, load
and continue, twice, then compares stdout, `play.SAV` and `final.SAV` with a
plain host build. With `--native-receipt` it also compares the hashes with a
native receipt (host ASan/UBSan or m68k-linux/qemu).

`test_wasm_browser.js` checks, with the fixture, in Chromium:

1. the page loads and shows the file prompt; the folder is picked and its
   MP3 voices and music are converted;
2. the title screen and its menu music (analyser level above silence);
3. `V` intro movie from `opening.amov` with its soundtrack, to the end;
4. Crossfire, `T` technology browser, a technology voice-over (heard), back;
5. `O` and `E` open the file requester and cancel it;
6. `N` new game on a Huge planet through its screens, the map, world audio
   (`alpha-audio.cfg` opened, faction music set started, `turn_start`
   effect) and its level;
7. the research, social, design and comm-link panels, base window and
   production;
8. End Turns with Chromium's CPU throttled 10x (`THROTTLE`, `TURNS`; default
   6): a 10 ms page timer must never be held more than 150 ms while the
   engine works, Playwright must get answers during the turn, and the
   "AI turn in progress" indicator must show (screenshot
   `09-ai-turn-in-progress.png`); results in `end-turns.json`;
9. `V` save, back to the title, `P` reopens the save, `O` opens it in the
   read-only view, `Q` exits;

with no console or page errors. Screenshots, the program output and
`result.json` go to the shots directory. `CPUPROFILE=file.cpuprofile`
also records a Chromium CPU profile of the first End Turn.

The fixture is synthetic (computed PCX patterns, the play test's rules, a
free system font under the original font names, sine-tone WAV/MP3 sound and
a computed `.amov`). It shows that the code paths work, not what the
original art or sound is like.

## Not done

- Not tested with an original install (none is available here). The original
  PCX/TTF/WAV/MP3 files go through the same readers as on the Amiga, and
  MP3 decoding is the browser's own.
- The `.wve` intro movies are not decoded in the browser; the prepared
  `.amov` cache must come from `native/prepare_startup_movie.py`.
- Native play mode calls only some of the audio service: world music, the
  `turn_start` effect and the menu sounds. The technology and facility voice
  hooks (`audio_tech_discovered`, `audio_facility_complete`) have no caller
  in the native play layer yet, so they are not heard in a game in either
  build; the technology voice is heard from the `T` browser.
- After End Turn the map redraw (saved-world refresh and full window draw)
  runs without a yield: with 10x CPU throttling the page waited up to about
  360 ms at that point (about 36 ms at full speed).
