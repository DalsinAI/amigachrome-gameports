# Alpha Centauri port: Python version

Play the native Alpha Centauri port from Python. The game logic is not
reimplemented: it is the same C++ engine (`../native/engine`, the Thinker
port) and play layer (`../native/play_session.cpp`, `alpha::play::Session`)
the native game uses, reached through a small C API.

```
asmac.h / asmac.cpp      extern "C" shim over Session (plain ints, structs, strings)
build_python.sh          builds alpha_smac/libasmac.so (host g++ -O2 -fPIC -shared)
alpha_smac/_lib.py       ctypes binding (structs mirror asmac.h, ABI version check)
alpha_smac/game.py       Game API: new game, load/save, units, bases, factions, map, orders, panels
alpha_smac/frontend.py   pygame frontend (python -m alpha_smac)
alpha_smac/headless.py   computer-only games (python -m alpha_smac --headless N)
alpha_smac/playtest.py   the native 20-turn play test, played through the Python API
tests/                   pytest suite
```

## Build and run

```
sh build_python.sh                       # about a minute; or: python -m alpha_smac --build ...
pip install pygame                       # 2.6.1 tested; only the window needs it
python -m alpha_smac --data ~/SMAX       # your own SMAC/SMAX folder, new game
python -m alpha_smac --data ~/SMAX --load AlphaPlay-001.SAV
python -m alpha_smac --synthetic --headless 50 --seed 9
```

Options: `--seed --faction 1..7 --difficulty 0..5 --map-size 0..4`,
`--base-game` (alpha.txt even if alphax.txt exists), `--save-dir` (where V
saves), `--headless N [--save FILE] [--keep-player]`, and for scripted
windows `--frames N --script KEYS --screenshot PNG`.

## Original game files

The original rules and art are not part of this repository and are never
committed. `--data PATH` reads your installation the way the native build
does: `alphax.txt` (or `alpha.txt`), `labels.txt` and the faction `.txt`
files, matched as given, lower or upper case. Nothing is copied.

Without them the engine has no rules, so neither the native new-game path
nor this one can start a real game. `--synthetic` writes the rules directory
of the native play test (`native/test_play_session.py`: synthetic tables,
invented names such as "Noun1", no original text) and plays on that; it
exercises every system but it is not the original game's content. The
frontend draws the map itself (no texture or unit art is loaded), so the
window works the same with either rules source; original art, sound,
movies and the title screen still need the native frontend.

## Window keys

They follow the native map window (`original_map_window.cpp`, `play_key`):
keypad or digits 1-9 move (into an enemy: battle odds, RETURN attacks),
5/SPACE skip, N/TAB next unit, E end turn, B build base (colony pod) or next
base, I base window, P production (base window open) or social engineering,
T research, K design, X diplomacy, G/R go to / road to (then click),
r f F m s S terraform, l h H j w D U unit orders, M log, O fog,
C centre, V save, Ctrl+L reload, Q quit (twice with unsaved turns).
Arrows and the wheel scroll, +/- zoom, left click selects or attacks,
right click centres. In panels 8/2/4/6 or arrows move, RETURN applies,
ESC closes. Panels and the base window are drawn by the native ui_*.cpp
renderers into an RGB frame.

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

`ALPHA_SMAC_ARTIFACTS=DIR` keeps the parity receipt, the headless summary
and the screenshots; `ALPHA_SMAC_LIB` selects another library build.
