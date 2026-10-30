# Alpha Centauri port: Python version

Play the native Alpha Centauri port from Python. The game logic is not
reimplemented: it is the same C++ engine (`../native/engine`, the Thinker
port) and play layer (`../native/play_session.cpp`, `alpha::play::Session`)
the native game uses, reached through a small C API.

```
asmac.h / asmac.cpp      extern "C" shim over Session (plain ints, structs, strings)
asmac_view.cpp           the map window's drawing with the user's art, PCX, audio catalog
build_python.sh          builds alpha_smac/libasmac.so (host g++ -O2 -fPIC -shared)
alpha_smac/_lib.py       ctypes binding (structs mirror asmac.h, ABI version check)
alpha_smac/game.py       Game API: new game, load/save, units, bases, factions, map, orders, panels
alpha_smac/frontend.py   pygame frontend (python -m alpha_smac): title, new game, map, panels
alpha_smac/view.py       MapView (native map/minimap/base window/console), new-game screens, PCX
alpha_smac/audio.py      native audio events and music through pygame.mixer
alpha_smac/fixture.py    writes a synthetic install (computed art/fonts/tones) for tests
alpha_smac/headless.py   computer-only games (python -m alpha_smac --headless N)
alpha_smac/playtest.py   the native 20-turn play test, played through the Python API
tests/                   pytest suite
```

## Build and run

```
sh build_python.sh                       # about a minute; or: python -m alpha_smac --build ...
pip install pygame                       # 2.6.1 tested; only the window needs it
python -m alpha_smac --data ~/SMAX       # your own SMAC/SMAX folder: title screen
python -m alpha_smac --data ~/SMAX --skip-title --seed 5   # straight into a new game
python -m alpha_smac --data ~/SMAX --load AlphaPlay-001.SAV
python -m alpha_smac --synthetic --headless 50 --seed 9
```

Options: `--seed --faction 1..7 --difficulty 0..5 --map-size 0..4`,
`--base-game` (alpha.txt even if alphax.txt exists), `--save-dir` (where V
saves), `--headless N [--save FILE] [--keep-player]`, `--skip-title`,
`--no-audio`, and for scripted windows `--frames N --script KEYS
--screenshot PNG`.

## Game flow

`--data` opens the native title screen (`original_frontend.cpp`):
`openinga.pcx` / `xopeninga.pcx` with the native buttons and keys, A and X
pick the Alpha Centauri or Alien Crossfire rules, N starts a new game,
P plays a saved game (a list of the `.SAV` files in `--save-dir` and the
data folder), M switches the menu music, Q exits. N runs the native
new-game screens (`ui_newgame`: faction, planet and rules, planetfall)
through the C API and then `Session::adopt_new_game`, like the native
program. ESC on the first screen goes back to the title.

## Original game files

The original rules and art are not part of this repository and are never
committed. `--data PATH` reads your installation the way the native build
does: `alphax.txt` (or `alpha.txt`), `labels.txt` and the faction `.txt`
files, matched as given, lower or upper case. Nothing is copied.

With the art in that folder the map window is drawn by the native
renderers, ported from `original_map_window.cpp` into `asmac_view.cpp` and
reached through the C API into an RGB frame that pygame blits: `texture.pcx`
terrain, rocky/fungus/river/road/forest/farm/mine overlays (`ter1.pcx`),
territory borders and fog (`MapViewExtras`), the factions' base art, the
`Units.pcx` sprites and the CVR vehicle and infantry models, the minimap,
`console.pcx` under the HUD and the base window. Text uses the original
fonts (`Arial.ttf`, `arialn.ttf`, `ALPHC___.TTF`) found there. A missing
file falls back the way the native window does (markers, plain fills); only
without `texture.pcx` and `Units.pcx` (as with `--synthetic`) the frontend
draws flat-colour squares itself, in the same projection.

Without the rules the engine has none, so neither the native new-game path
nor this one can start a real game. `--synthetic` writes the rules directory
of the native play test (`native/test_play_session.py`: synthetic tables,
invented names such as "Noun1", no original text) and plays on that,
straight into the map; it exercises every system but it is not the
original game's content. `python -m alpha_smac.fixture DIR` writes a whole
synthetic install (those rules plus computed PCX images, a free font under
the original font names and computed tones, via `../wasm/make_fixture.py`)
to try the title, art and sound paths; it is not the game's art either.

## Sound and music

The native audio events (`../native/AUDIO.md`, `audio_events.h`) play
through `pygame.mixer` from your own files: the catalog default of each
event (`ok.wav`, `menu up.wav`, `menu down.wav`, `cpu turn complete.wav`,
...) looked up as `fx/<name>` then `<name>`, remapped or silenced by
`alpha-audio.cfg` (`map.<event>=file|none`, the volume and on/off keys).
The title loops `opening menu.wav`; in a game the faction's music set
(`music/<set>.wav`, `music/<set>-N.wav`, else `aset1`) plays as a
playlist. Ctrl+M toggles the music, Ctrl+N the effects, `--no-audio`
disables both. Missing files or no audio device are silent. Voice-overs
and movies are not played.

## Window keys

They follow the native map window (`original_map_window.cpp`, `play_key`):
keypad or digits 1-9 move (into an enemy: battle odds, RETURN attacks),
5/SPACE skip, N/TAB next unit, E end turn, B build base (colony pod) or next
base, I base window, P production (base window open) or social engineering,
T research, K design, X diplomacy, G/R go to / road to (then click),
r f F m s S terraform, l h H j w D U unit orders, M log, O fog,
C centre, V save, Ctrl+L reload, Q quit (twice with unsaved turns).
Arrows and the wheel scroll, +/- zoom, left click selects or attacks,
right click centres, a minimap click moves the view, the END TURN button
ends the turn. In panels 8/2/4/6 or arrows move, RETURN applies, ESC
closes; a click on an entry selects it, a click on the selected entry
applies it, a click outside the panel or a right click closes it. Panels
(research, production, social engineering, design, battle, diplomacy) and
the base window are drawn by the native ui_*.cpp renderers into an RGB
frame and placed inside the map area, clear of the HUD.

## Tests

```
python -m pytest -q tests
```

- `test_play_parity.py`: builds native `play_control` for the host exactly
  as `native/test_play_session.py` builds its plain host reference, plays
  the 20-turn script (seed 777) natively and through `alpha_smac.playtest`,
  and requires byte-identical output and `play.SAV`/`final.SAV`.
- `test_headless.py`: a 60-turn computer-only game (deterministic per seed)
  and `--headless 50 --save`, then load and continue.
- `test_frontend_smoke.py`: `SDL_VIDEODRIVER=dummy`, scripted keys
  (moves, research, base window, production, End Turn, save), PNG output.
- `test_api.py`: records, orders, production, research, save/load, errors.
- `test_art_flow.py`: with a fixture install, title -> N -> the three
  new-game screens -> map drawn with the art -> research (mouse),
  base window, production (mouse), social, design, minimap click,
  END TURN button -> V save -> Ctrl+L reload -> title P -> load; PNGs
  `flow_*.png`; the audio events played; MapView / NewGameFlow / audio
  catalog API; the CLI title and `--skip-title` paths.

Known native behaviour, left as is: loading a save does not reproduce one
byte of the engine view (offset 35317 of `asmac_view_bytes`: 0xFF in the
saved game, 0xFD in the loaded one) because the native load path sets it
differently. That is the native engine's behaviour and is not changed here. Two loads of one save
are identical and play on identically, which is what the tests check.

`ALPHA_SMAC_ARTIFACTS=DIR` keeps the parity receipt, the headless summary
and the screenshots; `ALPHA_SMAC_LIB` selects another library build.
