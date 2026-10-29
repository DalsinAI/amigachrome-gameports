# SPDX-License-Identifier: GPL-3.0-or-later
"""pygame frontend smoke test with SDL_VIDEODRIVER=dummy: a scripted session
(next unit, moves, research panel, base window, production panel, End Turn,
save) renders frames and saves PNG screenshots."""
import os
import subprocess
import sys

import pytest

os.environ.setdefault('SDL_VIDEODRIVER', 'dummy')
os.environ.setdefault('SDL_AUDIODRIVER', 'dummy')
pygame = pytest.importorskip('pygame')

import alpha_smac as ac  # noqa: E402
from alpha_smac._lib import PYTHON_DIR  # noqa: E402
from alpha_smac.frontend import App, parse_script  # noqa: E402


def colours(surface):
    w, h = surface.get_size()
    return {tuple(surface.get_at((x, y)))[:3] for x in range(0, w, 7) for y in range(0, h, 7)}


def test_frontend_frames_and_png(rules, artifacts_dir, tmp_path):
    g = ac.new_game(rules, seed=777, map_size=2)
    app = App(g, save_dir=tmp_path)
    try:
        turn0 = g.turn
        script = parse_script('N,8,9,,T,2,ESC,B,I,,P,2,ESC,ESC,E,,O,,V')
        frames = app.loop(max_frames=len(script) + 3, script=script, fps=0)
        assert frames == len(script) + 3
        assert g.turn == turn0 + 1                  # E ended the turn through the engine
        assert any(p.name.startswith('AlphaPlay-') for p in tmp_path.iterdir())  # V saved
        assert app.log and app.status
        shot = artifacts_dir / 'alpha_smac_map.png'
        pygame.image.save(app.screen, str(shot))
        assert shot.stat().st_size > 5000 and len(colours(app.screen)) > 30
        # A native panel drawn by the frontend.
        app.key(ord('T'))
        assert app.panel == 'research'
        app.draw()
        shot2 = artifacts_dir / 'alpha_smac_research.png'
        pygame.image.save(app.screen, str(shot2))
        loaded = pygame.image.load(str(shot2))
        assert loaded.get_size() == (1024, 768)
    finally:
        app.close()
        g.close()


def test_cli_screenshot(rules, artifacts_dir):
    shot = artifacts_dir / 'alpha_smac_cli.png'
    env = dict(os.environ, PYTHONPATH=str(PYTHON_DIR), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    r = subprocess.run([sys.executable, '-m', 'alpha_smac', '--synthetic', '--seed', '777', '--map-size', '2', '--frames', '12',
                        '--script', 'E,E,O,,B,I', '--screenshot', str(shot)], capture_output=True, text=True, timeout=600, env=env,
                       cwd=str(PYTHON_DIR))
    assert r.returncode == 0, r.stdout + r.stderr
    assert 'Screenshot:' in r.stdout and shot.stat().st_size > 5000
