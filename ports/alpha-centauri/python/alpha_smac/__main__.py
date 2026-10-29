# SPDX-License-Identifier: GPL-3.0-or-later
"""python -m alpha_smac: play the native engine from Python.

  python -m alpha_smac --data ~/SMAX                   # pygame window, new game
  python -m alpha_smac --data ~/SMAX --load game.SAV   # continue a save
  python -m alpha_smac --synthetic                     # no original files: synthetic test rules
  python -m alpha_smac --synthetic --headless 50       # 50 computer-only turns, then a summary

--data is your own SMAC/SMAX folder (alpha.txt or alphax.txt, labels.txt and
the faction .txt files); nothing from it is copied or shipped. Without it
the engine has no rules: only --synthetic (the native play test's invented
rules, no original text) can start a game then.
"""
import argparse
import sys

from . import _lib
from .game import Game, GameError, Status, new_game_available
from .rules import find_rules, synthetic_rules


def build_parser():
    ap = argparse.ArgumentParser(prog='python -m alpha_smac', description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    src = ap.add_mutually_exclusive_group()
    src.add_argument('--data', metavar='PATH', help='your SMAC/SMAX folder (alpha.txt / alphax.txt, labels.txt, faction files)')
    src.add_argument('--synthetic', action='store_true', help='use the synthetic test rules of the native play test (no original files)')
    ap.add_argument('--base-game', action='store_true', help='with --data: use alpha.txt even if alphax.txt is present')
    ap.add_argument('--load', metavar='SAV', help='load a saved game instead of starting a new one')
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--faction', type=int, default=1, help='player faction 1..7')
    ap.add_argument('--difficulty', type=int, default=2, help='0 Citizen .. 5 Transcend')
    ap.add_argument('--map-size', type=int, default=1, help='0 tiny .. 4 huge')
    ap.add_argument('--headless', type=int, metavar='N', help='play N computer-only turns without a window and print a summary')
    ap.add_argument('--keep-player', action='store_true', help='with --headless: keep the player human (passive) instead of computer-run')
    ap.add_argument('--save', metavar='SAV', help='with --headless: save the final state here')
    ap.add_argument('--save-dir', default='.', help='where V saves AlphaPlay-NNN.SAV (default: current directory)')
    ap.add_argument('--frames', type=int, help='window: stop after N frames')
    ap.add_argument('--script', help='window: comma-separated keys fed one per frame (see frontend.parse_script)')
    ap.add_argument('--screenshot', metavar='PNG', help='window: save the last frame as PNG')
    ap.add_argument('--lib', help='path of libasmac.so (default: alpha_smac/libasmac.so or $ALPHA_SMAC_LIB)')
    ap.add_argument('--build', action='store_true', help='build libasmac.so first if it is missing')
    return ap


def start_game(a, emit=print):
    if a.data:
        rules = find_rules(a.data, False if a.base_game else None)
    elif a.synthetic:
        rules = synthetic_rules()
    else:
        raise SystemExit('No rules: pass --data PATH (your SMAC/SMAX folder) or --synthetic (test rules, no original files).')
    emit(f'Rules: {rules.describe()}')
    g = Game()
    g.load_rules_checked(rules.path, rules.expansion)
    if a.load:
        from pathlib import Path
        r = g.load_bytes(Path(a.load).read_bytes())
        if r != Status.OK:
            raise GameError(r, f'load {a.load}')
    else:
        if not new_game_available():
            raise SystemExit('This build has no new-game package (engine/newgame.cpp).')
        r = g.start(a.seed, a.faction, a.difficulty, a.map_size)
        if r != Status.OK:
            raise GameError(r, 'new game')
    emit(f'Game: player faction {g.player}, turn {g.turn}, year {g.year}, {g.base_count} bases, {g.vehicle_count} units')
    return g


def main(argv=None):
    a = build_parser().parse_args(argv)
    _lib.load(a.lib, auto_build=a.build)
    g = start_game(a)
    if a.headless is not None:
        from .headless import play_headless
        res = play_headless(g, a.headless, keep_player=a.keep_player)
        if a.save:
            g.save(a.save)
            print(f'Saved {a.save}')
        return 0 if res['status'] in (Status.OK, Status.GAME_OVER) else 1
    from .frontend import parse_script, run_frontend
    frames, _ = run_frontend(g, frames=a.frames, script=parse_script(a.script) if a.script else None, screenshot=a.screenshot,
                             save_dir=a.save_dir, fps=0 if a.frames else 30)
    if a.screenshot:
        print(f'Screenshot: {a.screenshot} ({frames} frames)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
