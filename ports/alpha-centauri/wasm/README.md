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
| `compat/libstdcxx_compat.h` | `std::__gcd` (libstdc++ only; `engine/combat.cpp` uses it), force-included. |
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

Output size (emscripten 6.0.13, `-O2`): `alpha.wasm` about 3.6 MB,
`alpha.js` about 160 KB.

## Playing

1. Open the page and choose your game folder (or add files). They are read
   into the tab's memory; nothing is sent anywhere. Movies (`.wve`) and
   programs are skipped. Tick "Keep them in this browser" to store them in
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

`test_wasm_browser.js` checks the page loads and shows the file prompt, then
with the fixture: title screen, Crossfire, New game through its screens, the
map, End Turn, save, back to the title, Play a saved game through the file
dialog, and exit, with no console or page errors.

The fixture is synthetic (computed PCX patterns, the play test's rules, a
free system font under the original font names). It shows that the code path
works, not what the original art looks like.

## Not done

- Not tested with an original install (none is available here). The original
  PCX/TTF/WAV files go through the same readers as on the Amiga.
- Audio was not heard: the fixture has no WAV files. SDL2's Web Audio backend
  is linked; browsers start audio only after a click, and Start is that click.
- The engine runs on the browser's main thread: a long End Turn holds the
  page until it finishes.
- The intro movie needs the prepared `.amov` cache (`../native/STARTUP-MOVIES.md`);
  `.wve` files are not read.
- The local-player candidate build (`ALPHA_LOCAL_PLAYER_CANDIDATE`) is not built here.
