# SPDX-License-Identifier: GPL-3.0-or-later
"""The window with an install's art, fonts and sounds: a synthetic install
(alpha_smac.fixture: computed PCX images, a free font under the original
font names, computed tones; nothing from the original game) walked through
title -> new game screens -> map drawn by the native renderers -> panels
with keys and mouse -> end turn -> save -> load, with PNGs of each step.
SDL_VIDEODRIVER and SDL_AUDIODRIVER are the dummy drivers."""
import os
import subprocess
import sys

import pytest

os.environ.setdefault('SDL_VIDEODRIVER', 'dummy')
os.environ.setdefault('SDL_AUDIODRIVER', 'dummy')
pygame = pytest.importorskip('pygame')

import alpha_smac as ac  # noqa: E402
from alpha_smac import fixture  # noqa: E402
from alpha_smac._lib import PYTHON_DIR  # noqa: E402
from alpha_smac.audio import Audio  # noqa: E402
from alpha_smac.frontend import BATTLE_RECT, END_BUTTON, HUD_Y, MINIMAP, PANEL_RECT, TITLE_BUTTONS, App  # noqa: E402
from alpha_smac.rules import RulesDir  # noqa: E402
from alpha_smac.view import NG_DONE, NG_FACTION, NG_PLANETFALL, NG_SETTINGS, NGK_DOWN, NGK_ENTER, Frame, MapView, \
    NewGameFlow, load_pcx  # noqa: E402


@pytest.fixture(scope='session')
def install(tmp_path_factory, asmac_lib):
    return fixture.make(tmp_path_factory.mktemp('install') / 'smax')


def colours(surface, rect=None):
    x0, y0, w, h = rect or (0, 0, *surface.get_size())
    return {tuple(surface.get_at((x, y)))[:3] for x in range(x0, x0 + w, 5) for y in range(y0, y0 + h, 5)}


def shot(app, path, shots):
    app.draw()
    pygame.image.save(app.screen, str(path))
    shots.append(path)
    assert path.stat().st_size > 3000


def test_view_newgame_and_audio_api(install):
    g = ac.Game()
    g.load_rules_checked(install, True)
    ng = NewGameFlow(g, 777)
    assert ng.view().screen == NG_FACTION and ng.view().lines
    ng.key(NGK_ENTER)
    assert ng.screen == NG_SETTINGS and ng.view().title == 'Planet and rules'
    while ng.view().row != 10:
        ng.key(NGK_DOWN)
    ng.key(NGK_ENTER)
    assert ng.screen == NG_PLANETFALL
    ng.key(NGK_ENTER)
    assert ng.screen == NG_DONE and ng.start_status == 0
    assert ng.finish() == ac.Status.OK and g.turn == 1 and g.player > 0
    mv = MapView(install, True)
    try:
        mv.update(g, reopen=True)
        assert mv.has_art and 'faction art 7/7' in mv.describe()
        f = Frame(640, 400)
        u = g.unit(g.next_unit())
        mv.viewer = -1
        labels = mv.draw_map(f, (u.x, u.y), 1, u.id)
        assert labels and len(set(f.bytes()[::97])) > 8
        t = mv.screen_tile((u.x, u.y), 1, 640, 400, 320, 200)
        assert t == (u.x, u.y)
        assert mv.tile_position((u.x, u.y), 1, 640, 400, u.x, u.y) == (320, 200)
        assert mv.draw_minimap(f, (400, 200, 224, 174), (u.x, u.y), 1, 640, 400)
        assert mv.minimap_click((400, 200, 224, 174), 512, 287) is not None
        assert mv.draw_base_window(f, (0, 0, 640, 400), 0) is not None
    finally:
        mv.close()
        g.close()
    assert load_pcx(install, 'TEXTURE.PCX')[:2] == (1024, 768)
    a = Audio(install)
    assert a.events['menu_open'] == 'menu up.wav' and a.events['unit_move'] == 'ok.wav' and a.events['unit_attack'] is None
    assert a.music_level == 50 and a.effect_path('base_built').endswith('mmenu.wav')
    if a.ok:
        assert a.event('menu_open') and a.played == [('menu_open', 'menu up.wav')]
        assert not a.event('unit_attack')                    # no file mapped: silent
        assert a.game_music('gaians') == 1 and a.music_track == 'aset1.wav'
    a.close()
    silent = Audio(install, enabled=False)
    assert not silent.event('menu_open') and silent.played == []


def test_title_newgame_map_panels_turn_save_load(install, artifacts_dir, tmp_path):
    shots = []
    out = artifacts_dir
    app = App(None, save_dir=tmp_path, rules=RulesDir(install, True))
    try:
        assert app.mode == 'title' and app.original_fonts
        app.newgame_seed = 777
        shot(app, out / 'flow_01_title.png', shots)
        assert len(colours(app.screen)) > 20
        # N on the title screen: the native faction screen.
        x, y, w, h = dict(TITLE_BUTTONS)['N']
        app.click(x + 20, y + 20)
        assert app.mode == 'newgame' and app.flow.view().title == 'Choose your faction'
        shot(app, out / 'flow_02_faction.png', shots)
        app.key(13)
        assert app.flow.screen == NG_SETTINGS
        shot(app, out / 'flow_03_settings.png', shots)
        for _ in range(12):
            if app.flow.view().row == 10:
                break
            app.key(ord('2'))
        app.key(13)
        assert app.flow.screen == NG_PLANETFALL
        shot(app, out / 'flow_04_planetfall.png', shots)
        app.key(13)
        g = app.g
        assert app.mode == 'map' and g.turn == 1 and app.player > 0
        # The map drawn with the art by the native renderer, the console HUD.
        assert app.view.has_art and 'faction art 7/7' in app.view.describe()
        app.view.viewer = -1                                  # whole map, like O
        shot(app, out / 'flow_05_map_art.png', shots)
        assert len(colours(app.screen, (0, 44, 1024, 467))) > 12
        assert len(colours(app.screen, (112, HUD_Y, 800, 257))) > 12
        # Research: a click on an entry selects it, a second click applies it.
        app.key(ord('T'))
        app.draw()
        assert app.panel == 'research'
        rows = [i for i in app.panel_items if i[5].strip().startswith('Test') and not i[4]]
        target = rows[2]
        app.click(target[0] + 10, target[1] - 5)
        assert app.model.selected == 2
        shot(app, out / 'flow_06_research.png', shots)
        app.click(target[0] + 10, target[1] - 5)
        assert 'RETURN again confirms' in app.status or app.model.current == app.model.ids[2]
        px, py, pw, ph = PANEL_RECT
        assert py + ph <= HUD_Y and px >= 0 and px + pw <= 1024    # inside the map area, clear of the HUD
        app.click(px - 5, py + 10)                                  # outside: closes
        assert app.panel is None
        # Base window, then its production panel (P), a click builds the selected item.
        app.key(ord('I'))
        assert app.base_open
        shot(app, out / 'flow_07_base.png', shots)
        app.key(ord('P'))
        app.draw()
        assert app.panel == 'production'
        rows = [i for i in app.panel_items if not i[4] and i[5].split('  ')[0] in
                {ac.game.text(app.model.items[k].name) for k in range(app.model.count)}]
        assert rows
        app.click(rows[-1][0] + 5, rows[-1][1] - 5)
        assert app.model.selected == app.model.count - 1
        shot(app, out / 'flow_08_production.png', shots)
        app.click(rows[-1][0] + 5, rows[-1][1] - 5, button=1)
        assert app.g.base(app.selected_base).current_item == app.model.items[app.model.count - 1].id
        app.click(500, 300, button=3)                               # right click closes
        assert app.panel is None
        app.key(27)
        assert not app.base_open
        # Social engineering (P on the map) and design (K) panels.
        app.key(ord('P'))
        assert app.panel == 'social'
        shot(app, out / 'flow_09_social.png', shots)
        app.key(27)
        app.key(ord('K'))
        assert app.panel == 'design'
        shot(app, out / 'flow_10_design.png', shots)
        app.key(27)
        # Minimap click centres the view.
        mx, my, mw, mh = MINIMAP
        cam = list(app.cam)
        app.click(mx + mw // 4, HUD_Y + my + mh // 4)
        assert app.cam != cam
        # End turn with the button.
        x, y, w, h = END_BUTTON
        app.click(x + 30, y + 10)
        assert g.turn == 2
        shot(app, out / 'flow_11_turn2.png', shots)
        app.key(ord('V'))
        saved = sorted(tmp_path.glob('AlphaPlay-*.SAV'))
        assert len(saved) == 1
        app.key(ord('l'), ctrl=True)
        assert app.status.startswith('Reloaded') and app.g.turn == 2
        played = [e for e, _ in app.audio.played]
        if app.audio.ok:
            for e in ('menu_open', 'menu_close', 'ui_click', 'turn_complete', 'turn_start'):
                assert e in played, (e, played)
            assert app.audio.music_track == 'aset1.wav'
    finally:
        app.close()
    # A second window: P on the title screen lists the save; RETURN loads it.
    app = App(None, save_dir=tmp_path, rules=RulesDir(install, True))
    try:
        app.key(ord('P'))
        assert app.mode == 'loadlist' and app.load_list == saved
        shot(app, out / 'flow_12_loadlist.png', shots)
        app.key(13)
        assert app.mode == 'map' and app.g.turn == 2
        shot(app, out / 'flow_13_loaded.png', shots)
    finally:
        app.close()
    assert all(p.exists() for p in shots)


def test_battle_panel_mouse(rules, artifacts_dir):
    """Attack preview panel: inside the map area; a click inside attacks
    (RETURN), a click outside closes it (ESC)."""
    g = ac.new_game(rules, seed=777, map_size=2)
    app = App(g, save_dir='/nonexistent-dir-for-test')
    try:
        x, y, w, h = BATTLE_RECT
        assert y + h <= HUD_Y
        u = g.unit(g.next_unit())
        app.selected = u.id
        app.attack = (u.id, u.x, u.y)
        app.panel = 'battle'
        app.draw()
        app.click(x - 10, y + 10)
        assert app.panel is None
    finally:
        app.close()


def test_synthetic_fallback_and_cli_title(install, artifacts_dir):
    """--synthetic has no art: own drawing in the same projection. --data with
    the install opens on the title screen (Q leaves)."""
    env = dict(os.environ, PYTHONPATH=str(PYTHON_DIR), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    png = artifacts_dir / 'flow_cli_title.png'
    r = subprocess.run([sys.executable, '-m', 'alpha_smac', '--data', str(install), '--frames', '3', '--script', ',,',
                        '--screenshot', str(png)], capture_output=True, text=True, timeout=600, env=env, cwd=str(PYTHON_DIR))
    assert r.returncode == 0, r.stdout + r.stderr
    assert png.stat().st_size > 3000
    png2 = artifacts_dir / 'flow_cli_skip_title.png'
    r = subprocess.run([sys.executable, '-m', 'alpha_smac', '--data', str(install), '--skip-title', '--map-size', '2', '--frames', '4',
                        '--script', 'O,T,,', '--screenshot', str(png2), '--no-audio'], capture_output=True, text=True, timeout=600,
                       env=env, cwd=str(PYTHON_DIR))
    assert r.returncode == 0, r.stdout + r.stderr
    assert png2.stat().st_size > 5000
