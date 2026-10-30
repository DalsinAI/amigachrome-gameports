# SPDX-License-Identifier: GPL-3.0-or-later
"""A computer-only game of 60 turns through the API (every faction played by
the engine's AI), deterministic for a seed, and the --headless CLI."""
import os
import subprocess
import sys

import alpha_smac as ac
from alpha_smac._lib import PYTHON_DIR
from alpha_smac.headless import play_headless

TURNS = 60


def play(rules, seed):
    g = ac.new_game(rules, seed=seed, faction=1, difficulty=2, map_size=1)
    lines = []
    res = play_headless(g, TURNS, emit=lines.append)
    facs = g.factions(alive_only=False)
    g.close()
    return res, lines, facs


def test_ai_game_60_turns(rules, artifacts_dir):
    res, lines, facs = play(rules, 2026)
    (artifacts_dir / 'headless-60.txt').write_text('\n'.join(lines) + '\n')
    print('\n'.join(lines))
    assert res['turns'] == TURNS or res['game_over']
    assert res['turn'] >= 1 + TURNS
    assert res['bases'] > 7          # the computer players founded new bases
    alive = [f for f in facs if f.alive]
    assert len(alive) >= 2
    assert not any(f.human for f in alive)  # the player's faction was handed to the AI
    assert facs[0].bases >= 2       # ... and it expanded too
    # The same seed gives the same game.
    res2, lines2, _ = play(rules, 2026)
    assert res2['digest'] == res['digest']
    assert [l.rsplit(' seconds=', 1)[0] for l in lines2] == [l.rsplit(' seconds=', 1)[0] for l in lines]


def test_headless_cli(rules, tmp_path):
    env = dict(os.environ, PYTHONPATH=str(PYTHON_DIR))
    save = tmp_path / 'end.SAV'
    r = subprocess.run([sys.executable, '-m', 'alpha_smac', '--synthetic', '--headless', '50', '--seed', '9', '--save', str(save)],
                       capture_output=True, text=True, timeout=1800, env=env, cwd=str(PYTHON_DIR))
    assert r.returncode == 0, r.stdout[-2000:] + r.stderr[-2000:]
    final = [l for l in r.stdout.splitlines() if l.startswith('FINAL ')]
    assert final and 'turns=50 ' in final[0], r.stdout[-2000:]
    assert save.stat().st_size > 1000
    # The saved game loads and continues.
    g = ac.load(rules, save)
    assert g.turn >= 51
    t = g.end_turn()
    assert t.status == ac.Status.OK
