# SPDX-License-Identifier: GPL-3.0-or-later
"""pygame frontend: python -m alpha_smac --data PATH.

Screens follow the native program: the title screen (original_frontend.cpp:
A / X rules, N new game, P play a saved game, M menu music, Q exit), the
new-game screens (ui_newgame: faction, planet and rules, planetfall) and the
map window (original_map_window.cpp play mode).

With the user's art (texture.pcx and Units.pcx in the data folder) the map
is drawn by the native renderers through asmac_view (terrain, overlays,
borders, fog, faction base art, unit sprites and CVR models, minimap,
console.pcx HUD, base window). Without art (--synthetic) the map squares
are drawn here with flat colours in the same projection; everything else
is the same. Panels are the native ui_*.cpp renderers.

Map keys (play_key): keypad/digits 1-9 move (into an enemy: odds, RETURN
attacks), 5/SPACE skip, N/TAB next unit, E end turn, B build base (colony
pod) else next base, I base window, P production (base window open) else
social engineering, T research, K design, X diplomacy, G/R go to / road to
(click), r f F m s S terraform, l h H j w D U orders, M messages, O fog,
C centre, +/- zoom, V save, Ctrl+L reload, Q quit (twice when unsaved).
Ctrl+M music on/off, Ctrl+N sound effects on/off. Arrows and the wheel
scroll; left click selects, attacks or picks a target; right click
centres; a minimap click moves the view. In panels: 8/2/4/6 or arrows,
RETURN applies, ESC closes; a click on an entry selects it, a click on the
selected entry applies it, a click outside the panel or a right click
closes it.
"""
import time
from pathlib import Path

from . import _lib as L
from .game import Cmd, Dir, Game, Key, MsgKind, Status, TileBits, command_name, diplomacy_available, panel_key, render_panel, \
    status_name

W, H = 1024, 768
TOP_H = 44
MAP_Y, MAP_W, MAP_H = TOP_H, 1024, 768 - 257 - TOP_H
HUD_Y, HUD_H = 768 - 257, 257
CONSOLE_X = (W - 800) // 2
MINIMAP = (CONSOLE_X + 563, 52, 224, 174)        # in the HUD frame
END_BUTTON = (CONSOLE_X + 563, HUD_Y + 228, 224, 26)
PANEL_RECT = ((W - 744) // 2, MAP_Y + 36, 744, 398)
BATTLE_RECT = ((W - 544) // 2, MAP_Y + 85, 544, 300)
BASEWIN_RECT = PANEL_RECT
MSG_RECT = (8, MAP_Y + MAP_H - 178, 560, 170)

FACTION_RGB = [(170, 170, 170), (220, 200, 40), (60, 170, 60), (220, 80, 60), (90, 120, 230), (210, 120, 40),
               (170, 90, 200), (60, 190, 190)]
INK_RGB = [(150, 172, 153), (128, 148, 195), (184, 194, 88), (216, 205, 92), (232, 224, 112), (205, 164, 90)]
MSG_RGB = {MsgKind.INFO: (180, 190, 180), MsgKind.ACTION: (150, 180, 220), MsgKind.WARNING: (230, 160, 80),
           MsgKind.COMBAT: (235, 110, 100), MsgKind.BASE: (220, 210, 120), MsgKind.RESEARCH: (140, 210, 160)}
MENU_INK = (238, 221, 170)
BASE_EDIT_SET, BASE_EDIT_QUEUE_ADD, BASE_EDIT_QUEUE_REMOVE, BASE_EDIT_HURRY, BASE_EDIT_CLOSE = 2, 3, 4, 5, 6

# Title screen buttons (original_frontend.cpp draw(); same window coordinates).
TITLE_BUTTONS = [('A', (45, 78, 220, 42)), ('X', (277, 78, 230, 42)), ('M', (45, 244, 462, 42)), ('Q', (45, 296, 462, 42)),
                 ('N', (45, 640, 462, 42)), ('P', (45, 692, 462, 42))]


def cs(v):
    """A C char array (bytes or ctypes array) as text."""
    v = getattr(v, 'value', v)
    return bytes(v).split(b'\0')[0].decode('cp1252', 'replace')


def tile_rgb(t):
    if t.ocean:
        return [(12, 28, 78), (20, 48, 118), (38, 86, 156)][min(max(t.altitude, 0), 2)]
    r, g, b = [(156, 134, 92), (112, 132, 64), (62, 122, 52)][min(t.rainfall, 2)]
    lift = max(0, t.altitude - 3) * 14
    r, g, b = r + lift, g + lift, b + lift
    if t.rocky >= 2:
        r, g, b = (r + 130) // 2, (g + 125) // 2, (b + 120) // 2
    if t.items & TileBits.FUNGUS:
        r, g, b = (r + 170) // 2, (g + 60) // 2, (b + 140) // 2
    return min(r, 255), min(g, 255), min(b, 255)


class App:
    def __init__(self, game=None, title='Alpha Centauri (Python)', save_dir='.', data_dir=None, rules=None, audio=True,
                 start_title=None, pygame_mod=None):
        import pygame
        self.pg = pygame_mod or pygame
        pg = self.pg
        pg.display.init()
        pg.font.init()
        self.screen = pg.display.set_mode((W, H))
        pg.display.set_caption(title)
        self.rules = rules
        self.data_dir = Path(data_dir) if data_dir else (rules.path if rules else (game.rules_dir if game else Path('.')))
        self.expansion = rules.expansion if rules else (game.expansion if game and game.expansion is not None else True)
        self.save_dir = Path(save_dir)
        self._fonts()
        from .view import Frame, MapView
        self.Frame = Frame
        self.map_frame = Frame(MAP_W, MAP_H)
        self.hud_frame = Frame(W, HUD_H)
        self.full_frame = Frame(W, H)
        self.view = MapView(self.data_dir, self.expansion)
        from .audio import Audio
        self.audio = Audio(self.data_dir, enabled=audio)
        self.zoom = 1
        self.cam = [0, 0]
        self.g = None
        self.mode = 'title'
        self.selected = -1
        self.selected_base = -1
        self.base_open = False
        self.panel = None
        self.model = None
        self.panel_items = []
        self.attack = None
        self.target_mode = None
        self.messages_open = False
        self.log = []
        self.status = ''
        self.dirty = False
        self.quit_warned = False
        self.last_save = None
        self.contact_index = 0
        self.running = True
        self.frames = 0
        self.hover = None
        self.flow = None
        self.newgame_seed = None      # None: the clock, like the native program
        self.load_list = []
        self.load_index = 0
        self.menu_status = f'Data folder: {self.data_dir}'
        self.music_menu = False
        self.title_art = {}
        if start_title is None:
            start_title = game is None
        if game is not None and not start_title:
            self.enter_map(game)
        else:
            self.enter_title()

    # ---- fonts (original_ui_text.cpp / original_frontend.cpp names) ----
    def _font(self, names, size, bold=False):
        from .view import find_file
        for n in names:
            p = find_file(self.data_dir, n)
            if p:
                try:
                    f = self.pg.font.Font(p, size)
                    f.set_bold(bold)
                    return f, True
                except Exception:  # noqa: BLE001
                    pass
        f = self.pg.font.Font(None, size + 5)
        f.set_bold(bold)
        return f, False

    def _fonts(self):
        self.font, self.original_fonts = self._font(['Arial.ttf', 'arialn.ttf'], 12)
        self.font_b, _ = self._font(['Arial_Bold.ttf', 'Arial.ttf', 'ALPHC___.TTF'], 14, bold=True)
        self.font_s, _ = self._font(['arialn.ttf', 'Arial.ttf'], 12)
        self.menu_font, _ = self._font(['ALPHC___.TTF'], 20)
        self.detail_font, _ = self._font(['arialn.ttf'], 18)

    # ---- modes ----
    def enter_title(self):
        self.mode = 'title'
        self.panel = None
        if not self.music_menu:
            self.music_menu = self.audio.menu_music(True)

    def title_image(self):
        key = 'x' if self.expansion else 'a'
        if key not in self.title_art:
            from .view import load_pcx
            img = load_pcx(self.data_dir, 'xopeninga.pcx' if self.expansion else 'openinga.pcx')
            self.title_art[key] = self.pg.image.frombuffer(img[2], (img[0], img[1]), 'RGB').copy() if img and img[:2] == (1024, 768) else None
        return self.title_art[key]

    def rules_available(self, expansion):
        from .view import find_file
        return bool(find_file(self.data_dir, 'alphax.txt' if expansion else 'alpha.txt'))

    def new_session(self):
        g = Game()
        r = g.load_rules(self.data_dir, self.expansion)
        if r != Status.OK:
            self.menu_status = f'Rules: {g.rules_error[:140]}'
            g.close()
            return None
        return g

    def start_new_game(self, seed=None):
        from .view import NewGameFlow
        g = self.new_session()
        if g is None:
            return False
        try:
            self.flow = NewGameFlow(g, int(time.time()) if seed is None else seed)
        except RuntimeError as e:
            self.menu_status = str(e)
            g.close()
            return False
        self.flow_game = g
        self.mode = 'newgame'
        self.audio.event('menu_open')
        return True

    def newgame_key(self, code):
        from .view import NG_DONE, NGK_BACK, NGK_DOWN, NGK_ENTER, NGK_LEFT, NGK_RIGHT, NGK_UP
        keys = {ord('8'): NGK_UP, ord('w'): NGK_UP, ord('2'): NGK_DOWN, ord('s'): NGK_DOWN, ord('4'): NGK_LEFT, ord('a'): NGK_LEFT,
                ord('6'): NGK_RIGHT, ord('d'): NGK_RIGHT, 13: NGK_ENTER, 10: NGK_ENTER, 27: NGK_BACK, 8: NGK_BACK}
        k = keys.get(code)
        if k is None or self.flow.done:
            return
        self.flow.key(k)
        if k == NGK_ENTER:
            self.audio.event('ui_click')
        if not self.flow.done:
            return
        g = self.flow_game
        if self.flow.screen != NG_DONE:
            self.menu_status = 'New game cancelled.'
            g.close()
            self.flow = None
            self.enter_title()
            return
        r = self.flow.finish()
        self.flow = None
        if r != Status.OK:
            self.menu_status = f'The new game could not start: {status_name(r)}'
            g.close()
            self.enter_title()
            return
        self.enter_map(g)

    def open_load_list(self):
        seen, files = set(), []
        for d in (self.save_dir, self.data_dir):
            try:
                for p in sorted(Path(d).iterdir()):
                    if p.suffix.lower() in ('.sav', '.sc') and p.resolve() not in seen:
                        seen.add(p.resolve())
                        files.append(p)
            except OSError:
                pass
        self.load_list, self.load_index = files, 0
        self.mode = 'loadlist'
        if not files:
            self.menu_status = 'No saved game (.SAV) in the save or data folder.'

    def load_selected(self):
        if not self.load_list:
            self.enter_title()
            return
        path = self.load_list[self.load_index]
        g = self.new_session()
        if g is None:
            self.enter_title()
            return
        r = g.load_bytes(path.read_bytes())
        if r != Status.OK:
            self.menu_status = f'Could not load {path.name}: {status_name(r)}'
            g.close()
            self.enter_title()
            return
        self.last_save = path
        self.enter_map(g)

    def enter_map(self, game):
        if self.g is not None and self.g is not game:
            self.g.close()
        self.g = game
        self.mode = 'map'
        self.panel = None
        self.base_open = False
        self.dirty = False
        self.view.update(game, reopen=True)
        self.player = game.player
        self.view.viewer = self.player
        self.music_menu = False
        self.audio.game_music(self.view.faction_stem(self.player) if self.player > 0 else '')
        self.audio.event('turn_start')
        self.refresh()
        self.drain()
        self.selected = -1
        self.next_unit()
        own = self.g.bases(self.player)
        self.selected_base = own[0].id if own else -1
        if self.selected < 0 and own:
            self.centre(own[0].x, own[0].y)
        self.status = f'Turn {game.turn}, mission year {game.year}. Keypad moves, E ends the turn.'

    # ---- game state ----
    def refresh(self):
        self.view.update(self.g)
        self.mw, self.mh, self.tiles = self.g.tiles()
        self.units = self.g.units()
        self.bases = self.g.bases()

    def drain(self):
        self.log.extend(self.g.messages())
        self.log = self.log[-400:]

    def sound_for(self, msgs):
        for m in msgs:
            t = m.text
            if m.kind == MsgKind.COMBAT and t.startswith('Unit lost'):
                self.audio.event('unit_destroyed')
            elif m.kind == MsgKind.BASE and 'base founded' in t and t.startswith(self.faction_noun()):
                self.audio.event('base_built')
            elif t.startswith('WEDEVELOP') or t.startswith('THESECRET'):
                self.audio.event('tech_discovered')

    def faction_noun(self):
        f = self.g.faction(self.player) if self.player > 0 else None
        return f.noun if f else '\0'

    def run(self, kind, a=0, b=0, c=0, d=0, e=0):
        r = self.g.run(kind, a, b, c, d, e)
        if r == Status.OK:
            self.dirty = True
        self.quit_warned = False
        msgs = self.g.messages()
        self.log.extend(msgs)
        self.sound_for(msgs)
        self.status = msgs[-1].text if msgs else f'{command_name(kind)}: {status_name(r)}'
        self.refresh()
        if self.g.game_over:
            self.panel = 'gameover'
        return r

    def centre(self, x, y):
        if (x + y) & 1:
            y += 1
        self.cam = [x, y]

    def own_selected(self):
        u = self.g.unit(self.selected) if self.selected >= 0 else None
        if u and u.own:
            return u
        self.status = 'Select one of your units first (TAB, N or a click).'
        return None

    def next_unit(self):
        v = self.g.next_unit(self.selected)
        if v >= 0:
            self.selected = v
            u = self.g.unit(v)
            self.centre(u.x, u.y)
        else:
            self.status = 'All units have moved. E ends the turn.'

    def after_order(self):
        u = self.g.unit(self.selected) if self.selected >= 0 else None
        if not u or not u.own or u.moves_left <= 0 or u.order:
            self.next_unit()

    def unit_order(self, kind, b=0):
        if not self.own_selected():
            return
        self.run(kind, self.selected, b)
        self.after_order()

    def move(self, direction):
        u = self.own_selected()
        if not u:
            return
        ok, x, y = self.g.step_target(self.selected, direction)
        if not ok:
            self.status = 'Off the map.'
            return
        if self.g.hostile_at(x, y):
            self.attack_preview(self.selected, x, y)
            return
        if self.run(Cmd.MOVE, self.selected, direction) == Status.OK:
            after = self.g.unit(self.selected)
            if after and (after.x, after.y) != (u.x, u.y):
                self.audio.event('unit_move')
                self.centre(after.x, after.y)
            if not after or not after.own or after.moves_left <= 0:
                self.next_unit()

    def attack_preview(self, veh, x, y):
        self.attack = (veh, x, y)
        self.g.run(Cmd.ODDS, veh, x, y)
        self.log.extend(self.g.messages())
        self.panel = 'battle'
        self.audio.event('menu_open')

    def end_turn(self):
        self.panel = None
        self.target_mode = None
        self.audio.event('turn_complete')
        r = self.run(Cmd.END_TURN)
        self.messages_open = True
        if r == Status.OK:
            self.audio.event('turn_start')
            self.selected = -1
            self.next_unit()
            self.status = f'Turn {self.g.turn}, mission year {self.g.year}. Your move.'

    def open_panel(self, name):
        g = self.g
        m = None
        if name == 'production':
            m = g.production_panel(self.selected_base) if self.selected_base >= 0 else None
            if m is None:
                self.status = 'Choose one of your bases first (B or a click).'
        elif name == 'research':
            m = g.research_panel()
            if m is not None:
                m.confirm = 0
        elif name == 'social':
            m = g.social_panel()
        elif name == 'design':
            m = g.design_panel()
        elif name == 'diplo':
            ids = g.contacts()
            if not diplomacy_available():
                self.status = 'Diplomacy is not in this build.'
            elif not ids:
                self.status = 'No faction has been contacted yet.'
            else:
                self.contact_index %= len(ids)
                m = g.diplomacy_panel(ids[self.contact_index])
                if m is not None:
                    g.run(Cmd.DIPLO, ids[self.contact_index], -1)
        self.model = m
        self.panel = name if m is not None else None
        if self.panel:
            self.audio.event('menu_open')
        return m is not None

    def close_panel(self):
        if self.panel:
            self.audio.event('menu_close')
        self.panel = None

    def refresh_panel(self):
        g, m = self.g, self.model
        if self.panel == 'production':
            g.production_panel(m.base, m)
        elif self.panel == 'research':
            g.research_panel(m)
        elif self.panel == 'social':
            g.social_panel(m)
        elif self.panel == 'design':
            g.design_panel(m)
        elif self.panel == 'diplo':
            g.diplomacy_panel(m.other, m)

    def panel_key(self, code):
        """play_panel_key of the native map window."""
        g, m, p = self.g, self.model, self.panel
        key = L.load().asmac_ui_key_from_char(code)
        if p == 'battle':
            a = panel_key('battle', g.battle_panel(), key)
            if a == Key.A_APPLY:
                self.audio.event('unit_attack')
                self.run(Cmd.ATTACK, *self.attack)
                self.panel = 'battle' if g.battle_panel().valid else None
            elif a == Key.A_CLOSE:
                self.close_panel()
                self.after_order()
            return True
        if p == 'production':
            a = panel_key('production', m, code)
            if a == BASE_EDIT_SET:
                self.run(Cmd.PRODUCE, m.base, m.items[m.selected].id)
            elif a == BASE_EDIT_QUEUE_ADD:
                self.run(Cmd.QUEUE_ADD, m.base, m.items[m.selected].id)
            elif a == BASE_EDIT_QUEUE_REMOVE:
                self.run(Cmd.QUEUE_REMOVE, m.base, m.queue_count - 1)
            elif a == BASE_EDIT_HURRY:
                self.run(Cmd.HURRY, m.base)
            elif a == BASE_EDIT_CLOSE:
                self.close_panel()
                return True
            if a in (BASE_EDIT_SET, BASE_EDIT_QUEUE_ADD, BASE_EDIT_QUEUE_REMOVE, BASE_EDIT_HURRY):
                self.audio.event('ui_click')
            self.refresh_panel()
            return True
        if p == 'research':
            a = panel_key('research', m, key)
            if a == Key.A_APPLY:
                self.audio.event('ui_click')
                r = self.run(Cmd.RESEARCH, m.ids[m.selected], 0, 1 if m.confirm else 0)
                if r == Status.NEEDS_CONFIRM:
                    m.confirm = 1
                    self.status = 'Changing research halves the progress. RETURN again confirms.'
                else:
                    m.confirm = 0
                    self.refresh_panel()
            elif a == Key.A_CLOSE:
                self.close_panel()
            return True
        if p == 'social':
            a = panel_key('social', m, key)
            if a == Key.A_APPLY:
                self.audio.event('ui_click')
                self.run(Cmd.SOCIAL, m.row, m.col)
                self.refresh_panel()
            elif a == Key.A_CLOSE:
                self.close_panel()
            return True
        if p == 'design':
            a = panel_key('design', m, key)
            if a == Key.A_MOVED:
                g.design_preview(m)
            elif a == Key.A_APPLY:
                self.audio.event('ui_click')
                self.run(Cmd.DESIGN, *Game.design_selection(m))
                self.refresh_panel()
            elif a == Key.A_CLOSE:
                self.close_panel()
            return True
        if p == 'diplo':
            if key in (Key.LEFT, Key.RIGHT):
                g.run(Cmd.DIPLO, m.other, -2)
                self.contact_index += -1 if key == Key.LEFT else 1
                self.open_panel('diplo')
                return True
            a = panel_key('diplo', m, key)
            if a == Key.A_APPLY:
                self.audio.event('ui_click')
                self.run(Cmd.DIPLO, m.other, m.proposals[m.selected])
                self.refresh_panel()
            elif a == Key.A_CLOSE:
                g.run(Cmd.DIPLO, m.other, -2)
                self.close_panel()
            self.log.extend(g.messages())
            return True
        if p == 'gameover':
            if code in (27, 13):
                self.panel = None
            return code not in (ord('q'), ord('Q'), ord('v'), ord('V'))
        return False

    # ---- mouse on panels ----
    def panel_rect(self):
        return BATTLE_RECT if self.panel == 'battle' else PANEL_RECT

    def panel_click(self, px, py, button):
        """A click while a panel is open: outside or right click closes; on an
        entry selects it; on the selected entry applies it."""
        x, y, w, h = self.panel_rect()
        if button == 3 or not (x <= px < x + w and y <= py < y + h):
            self.key(27)
            return
        if self.panel in ('battle', 'gameover'):
            self.key(13)
            return
        hit = None
        for ix, base, width, ink, heading, s in self.panel_items:
            if ix - 4 <= px < ix + max(width, 40) and base - 16 <= py < base + 5 and not heading:
                hit = s.strip().lstrip('>').strip()
                break
        if not hit:
            return
        m = self.model
        idx = None
        if self.panel == 'production':
            names = [cs(m.items[i].name) for i in range(m.count)]
            idx = next((i for i, n in enumerate(names) if n and hit.startswith(n)), None)
            cur = m.selected
            if idx is not None:
                if idx == cur:
                    self.key(13)
                else:
                    m.selected = idx
            return
        if self.panel == 'research':
            names = [cs(m.names[i]) for i in range(m.count)]
            idx = next((i for i, n in enumerate(names) if n and hit.startswith(n)), None)
            if idx is not None:
                if idx == m.selected:
                    self.key(13)
                else:
                    m.selected = idx
            return
        if self.panel == 'diplo':
            names = [cs(m.labels[i]) for i in range(m.count)]
            idx = next((i for i, n in enumerate(names) if n and hit.startswith(n)), None)
            if idx is not None:
                if idx == m.selected:
                    self.key(13)
                else:
                    m.selected = idx
            return
        if self.panel == 'social':
            for c in range(4):
                for k in range(4):
                    n = cs(m.models[c][k])
                    if n and hit.startswith(n):
                        if (m.row, m.col) == (c, k):
                            self.key(13)
                        else:
                            m.row, m.col = c, k
                        return
            return
        if self.panel == 'design':
            for r in range(5):
                for i in range(m.counts[r]):
                    n = cs(m.choices[r][i].name)
                    if n and n in hit:
                        if m.row == r and m.selected[r] == i:
                            self.key(13)
                        else:
                            m.row = r
                            m.selected[r] = i
                            self.g.design_preview(m)
                        return

    # ---- save / load ----
    def save(self):
        self.save_dir.mkdir(parents=True, exist_ok=True)
        for i in range(1, 1000):
            p = self.save_dir / f'AlphaPlay-{i:03d}.SAV'
            if not p.exists():
                break
        else:
            self.status = 'No free save slot (AlphaPlay-001..999.SAV).'
            return
        try:
            self.g.save(p)
        except Exception as e:  # noqa: BLE001
            self.status = f'Save failed: {e}'
            return
        self.last_save = p
        self.dirty = False
        self.status = f'Saved {p}'
        self.drain()

    def reload(self):
        if not self.last_save:
            self.status = 'Nothing saved in this session yet (V saves).'
            return
        r = self.g.load_bytes(Path(self.last_save).read_bytes())
        self.drain()
        if r == Status.OK:
            self.view.update(self.g, reopen=True)
            self.view.viewer = self.g.player
            self.refresh()
            self.panel = None
            self.dirty = False
            self.selected = -1
            self.next_unit()
            self.status = f'Reloaded {self.last_save}'
        else:
            self.status = 'Reload failed.'

    # ---- input ----
    def title_key(self, code):
        c = chr(code) if 32 <= code < 127 else ''
        if c in ('a', 'A'):
            if self.rules_available(False):
                self.expansion = False
            else:
                self.menu_status = 'Alpha Centauri rules (alpha.txt) are not in the data folder.'
        elif c in ('x', 'X'):
            if self.rules_available(True):
                self.expansion = True
            else:
                self.menu_status = 'Alien Crossfire rules (alphax.txt) are not in the data folder.'
        elif c in ('n', 'N'):
            self.start_new_game(self.newgame_seed)
        elif c in ('p', 'P'):
            self.open_load_list()
        elif c in ('m', 'M'):
            if self.music_menu:
                self.audio.menu_music(False)
                self.music_menu = False
            else:
                self.music_menu = self.audio.menu_music(True)
                if not self.music_menu:
                    self.menu_status = 'Menu music unavailable (opening menu.wav or an audio device).'
        elif c in ('q', 'Q'):
            self.running = False

    def loadlist_key(self, code):
        n = len(self.load_list)
        if code in (ord('8'), ord('w')) and n:
            self.load_index = (self.load_index - 1) % n
        elif code in (ord('2'), ord('s')) and n:
            self.load_index = (self.load_index + 1) % n
        elif code in (13, 10):
            self.load_selected()
        elif code in (27, 8):
            self.enter_title()

    def key(self, code, ctrl=False):
        """One key as the native window's character code (VANILLAKEY)."""
        if self.mode == 'title':
            return self.title_key(code)
        if self.mode == 'newgame':
            return self.newgame_key(code)
        if self.mode == 'loadlist':
            return self.loadlist_key(code)
        if ctrl and code in (ord('m'), ord('M')):
            self.status = 'Music on.' if self.audio.toggle_music() else 'Music off.'
            if self.audio.music_on:
                self.audio.tick()
            return
        if ctrl and code in (ord('n'), ord('N')):
            self.status = 'Sound effects on.' if self.audio.toggle_sfx() else 'Sound effects off.'
            return
        if code in (ord('q'), ord('Q')) and self.panel in (None, 'gameover'):
            if self.dirty and not self.quit_warned:
                self.quit_warned = True
                self.audio.event('quit_prompt')
                self.status = 'Unsaved turns. V saves; Q again quits.'
                return
            self.running = False
            return
        if ctrl and code in (ord('l'), ord('L'), 12):
            self.reload()
            return
        if code in (ord('v'), ord('V')) and self.panel in (None, 'gameover'):
            self.save()
            return
        if self.panel and self.panel_key(code):
            return
        if self.target_mode and code == 27:
            self.target_mode = None
            self.status = 'Target cancelled.'
            return
        if code == 27 and self.base_open:
            self.base_open = False
            self.audio.event('menu_close')
            return
        if ord('1') <= code <= ord('9') and code != ord('5'):
            self.base_open = False
            self.move(Dir.KEYPAD[code - ord('0')])
            return
        c = chr(code) if 32 <= code < 127 else ''
        if c in ('e', 'E'):
            self.end_turn()
        elif c in ('5', ' '):
            self.unit_order(Cmd.SKIP)
        elif code == 9 or c in ('n', 'N'):
            self.next_unit()
        elif c in ('g', 'G', 'R'):
            if self.own_selected():
                self.target_mode = 'g' if c != 'R' else 'R'
                self.status = ('Go to' if c != 'R' else 'Road to') + ': click the destination (ESC cancels).'
        elif c in ORDER_KEYS:
            kind, arg = ORDER_KEYS[c]
            self.unit_order(kind, arg)
        elif c == 'b':
            u = self.g.unit(self.selected) if self.selected >= 0 else None
            if u and u.own and u.colony:
                self.unit_order(Cmd.BUILD_BASE)
            else:
                self.cycle_base()
        elif c == 'B':
            self.cycle_base()
        elif c in ('i', 'I'):
            if self.selected_base < 0:
                self.cycle_base()
            self.base_open = not self.base_open and self.selected_base >= 0
            self.audio.event('menu_open' if self.base_open else 'menu_close')
        elif c in ('p', 'P'):
            self.open_panel('production' if self.base_open else 'social')
        elif c in ('t', 'T'):
            self.open_panel('research')
        elif c in ('k', 'K'):
            self.open_panel('design')
        elif c in ('x', 'X'):
            self.open_panel('diplo')
        elif c == 'M':
            self.messages_open = not self.messages_open
        elif c in ('o', 'O'):
            self.view.viewer = -1 if self.view.viewer >= 0 else self.player
        elif c in ('c', 'C'):
            u = self.g.unit(self.selected) if self.selected >= 0 else None
            if u:
                self.centre(u.x, u.y)
        elif c in ('+', '='):
            self.zoom = 2
        elif c == '-':
            self.zoom = 1

    def cycle_base(self):
        own = [b for b in self.bases if b.faction == self.player]
        if not own:
            self.status = 'You have no base.'
            return
        ids = [b.id for b in own]
        self.selected_base = next((i for i in ids if i > self.selected_base), ids[0])
        b = self.g.base(self.selected_base)
        self.centre(b.x, b.y)

    def scroll(self, dx, dy):
        self.cam[0] = (self.cam[0] + dx) % max(1, self.mw)
        self.cam[1] = max(0, min(self.mh - 1, self.cam[1] + dy))
        if (self.cam[0] + self.cam[1]) & 1:
            self.cam[1] += 1 if self.cam[1] + 1 < self.mh else -1

    def screen_tile(self, px, py):
        return self.view.screen_tile(self.cam, self.zoom, MAP_W, MAP_H, px, py - MAP_Y)

    def click(self, px, py, button=1):
        if self.mode == 'title':
            for k, (x, y, w, h) in TITLE_BUTTONS:
                if x <= px < x + w and y <= py < y + h:
                    self.audio.event('ui_click')
                    self.title_key(ord(k))
                    return
            return
        if self.mode == 'loadlist':
            row = (py - 150) // 27
            if 0 <= row < len(self.load_list) and 60 <= px < 730:
                if row == self.load_index:
                    self.load_selected()
                else:
                    self.load_index = row
            return
        if self.mode == 'newgame':
            if button == 3:
                self.newgame_key(27)
            return
        if self.panel and py < HUD_Y:
            self.panel_click(px, py, button)
            return
        if py >= HUD_Y:
            x, y, w, h = END_BUTTON
            if x <= px < x + w and y <= py < y + h:
                self.end_turn()
                return
            mx, my, mw, mh = MINIMAP
            t = self.view.minimap_click((mx, HUD_Y + my, mw, mh), px, py)
            if t:
                self.centre(*t)
            return
        if py < MAP_Y:
            return
        if self.base_open:
            x, y, w, h = BASEWIN_RECT
            if button == 3 or not (x <= px < x + w and y <= py < y + h):
                self.base_open = False
                self.audio.event('menu_close')
            return
        t = self.screen_tile(px, py)
        if not t:
            return
        tx, ty = t
        if button == 3:
            self.centre(tx, ty)
            return
        if self.target_mode:
            kind = Cmd.GOTO if self.target_mode == 'g' else Cmd.ROAD_TO
            self.target_mode = None
            self.run(kind, self.selected, tx, ty)
            return
        own = self.g.unit_at(tx, ty)
        if own >= 0:
            self.selected = own
            self.base_open = False
            self.status = 'Unit selected.'
            return
        b = self.g.base_at(tx, ty)
        if b >= 0:
            self.selected_base = b
            self.base_open = True
            self.audio.event('menu_open')
            return
        u = self.g.unit(self.selected) if self.selected >= 0 else None
        if self.g.hostile_at(tx, ty) and u and u.own:
            self.attack_preview(self.selected, tx, ty)

    # ---- drawing helpers ----
    def text(self, s, x, y, rgb=(200, 210, 200), font=None, width=None):
        f = font or self.font
        surf = f.render(s, True, rgb)
        if width is not None and surf.get_width() > width:
            surf = surf.subsurface((0, 0, max(1, width), surf.get_height()))
        self.screen.blit(surf, (x, y))
        return surf.get_height()

    def text_base(self, s, x, baseline, rgb, font=None, width=None):
        f = font or self.font
        return self.text(s, x, baseline - f.get_ascent(), rgb, f, width)

    def blit_frame(self, frame, dest, rect=None):
        surf = self.pg.image.frombuffer(frame.bytes(), (frame.width, frame.height), 'RGB')
        if rect:
            self.screen.blit(surf, dest, self.pg.Rect(rect))
        else:
            self.screen.blit(surf, dest)

    def draw_items(self, items, dx=0, dy=0):
        for ix, base, width, ink, heading, s in items:
            f = self.font_b if heading else self.font
            self.text_base(s, ix + dx, base + dy, INK_RGB[ink % len(INK_RGB)], f, width=max(1, width))

    # ---- screens ----
    def draw_title(self):
        img = self.title_image()
        if img:
            self.screen.blit(img, (0, 0))
        else:
            self.screen.fill((16, 9, 5))
            self.text('Alpha Centauri', 560, 300, MENU_INK, self.menu_font)
        labels = {'A': 'Alpha Centauri [A]', 'X': 'Alien Crossfire [X]' if self.rules_available(True) else 'Crossfire unavailable',
                  'M': ('Original menu music: ON [M]' if self.music_menu else 'Original menu music: OFF [M]') if self.audio.ok else 'Original menu music unavailable',
                  'Q': 'Exit [Q]', 'N': 'New game [N]', 'P': 'Play a saved game [P]'}
        for k, (x, y, w, h) in TITLE_BUTTONS:
            active = (k == 'A' and not self.expansion) or (k == 'X' and self.expansion)
            self.screen.fill((0x3b, 0x2c, 0x17) if active else (0x10, 0x09, 0x05), (x, y, w, h))
            self.pg.draw.rect(self.screen, (0xaa, 0x88, 0x44), (x, y, w, h), 1)
            self.text_base(labels[k], x + 18, y + 27, MENU_INK, self.menu_font)
        self.screen.fill((0x10, 0x09, 0x05), (45, 462, 600, 64))
        self.text_base(self.menu_status, 58, 486, MENU_INK, self.detail_font, 580)
        self.text_base('N starts a new game, P plays on from a save.', 58, 509, MENU_INK, self.detail_font, 580)

    def draw_newgame(self):
        self.draw_title()
        v = self.flow.view()
        self.screen.fill((0x10, 0x09, 0x05), (45, 70, 700, 690))
        self.text_base(v.title, 70, 112, MENU_INK, self.menu_font)
        y = 150
        for t, sel, heading in v.lines:
            if y > 730:
                break
            if sel:
                self.screen.fill((0x3b, 0x2c, 0x17), (60, y - 20, 670, 26))
            self.text_base(t, 70 if heading else 90, y, MENU_INK, self.menu_font if heading else self.detail_font, 640)
            y += 32 if heading else 27
        self.text_base('Cursor keys or 8/2/4/6 choose, RETURN accepts, ESC goes back.', 70, 752, MENU_INK, self.detail_font)

    def draw_loadlist(self):
        self.draw_title()
        self.screen.fill((0x10, 0x09, 0x05), (45, 70, 700, 690))
        self.text_base('Play a saved game', 70, 112, MENU_INK, self.menu_font)
        y = 150
        for i, p in enumerate(self.load_list[:21]):
            if i == self.load_index:
                self.screen.fill((0x3b, 0x2c, 0x17), (60, y - 20, 670, 26))
            self.text_base(p.name, 90, y, MENU_INK, self.detail_font, 620)
            y += 27
        if not self.load_list:
            self.text_base(self.menu_status, 90, y, MENU_INK, self.detail_font, 620)
        self.text_base('8/2 choose, RETURN loads, ESC goes back.', 70, 752, MENU_INK, self.detail_font)

    def draw_map_fallback(self):
        """No terrain art: flat-colour squares in the native projection."""
        pg, scr = self.pg, self.screen
        scr.fill((6, 15, 24), (0, MAP_Y, MAP_W, MAP_H))
        z = self.zoom
        hw, hh = 32 * z, 16 * z
        cx, cy = self.cam
        rows, cols = MAP_H // hh // 2 + 2, MAP_W // hw // 2 + 2
        viewer = self.view.viewer
        for y in range(max(0, cy - rows), min(self.mh, cy + rows + 1)):
            for raw in range(cx - cols, cx + cols + 1):
                x = raw % self.mw
                if (x + y) & 1:
                    continue
                t = self.tiles[y * self.mw + x]
                if not t.valid:
                    continue
                dx = raw - cx
                sx, sy = MAP_W // 2 + dx * hw, MAP_Y + MAP_H // 2 + (y - cy) * hh
                poly = [(sx, sy - hh), (sx + hw, sy), (sx, sy + hh), (sx - hw, sy)]
                if viewer > 0 and not t.visibility & (1 << viewer):
                    continue
                pg.draw.polygon(scr, tile_rgb(t), poly)
                it = t.items
                if it & TileBits.FOREST:
                    for ox in (-hw // 3, 0, hw // 3):
                        pg.draw.circle(scr, (22, 70, 30), (sx + ox, sy), max(2, hh // 4))
                if it & TileBits.FARM:
                    pg.draw.rect(scr, (210, 200, 90), (sx - hw // 4, sy - hh // 4, hw // 2, hh // 2), 1)
                if it & TileBits.MINE:
                    pg.draw.polygon(scr, (90, 90, 90), [(sx, sy - hh // 2), (sx + hw // 4, sy + hh // 3), (sx - hw // 4, sy + hh // 3)])
                if it & TileBits.RIVER and not t.ocean:
                    pg.draw.line(scr, (70, 130, 230), (sx - hw // 2, sy + hh // 3), (sx + hw // 2, sy - hh // 3), 2)
                if it & (TileBits.ROAD | TileBits.MAGTUBE):
                    pg.draw.line(scr, (120, 85, 50), (sx - hw // 2, sy), (sx + hw // 2, sy), 2)
                if t.owner > 0:
                    pg.draw.polygon(scr, FACTION_RGB[t.owner % 8], poly, 1)
                if t.base >= 0:
                    b = self.g.base(t.base)
                    s = 10 + 4 * z
                    pg.draw.rect(scr, FACTION_RGB[b.faction % 8], (sx - s // 2, sy - s // 2, s, s))
                    pg.draw.rect(scr, (20, 20, 20), (sx - s // 2, sy - s // 2, s, s), 1)
                    self.text_base(f'{b.name} ({b.pop_size})', sx - 24, sy + 24, INK_RGB[4])
                elif t.unit_count:
                    pg.draw.circle(scr, FACTION_RGB[t.unit_faction % 8], (sx, sy - 6), 5 + z)
                    pg.draw.circle(scr, (10, 10, 10), (sx, sy - 6), 5 + z, 1)
        u = self.g.unit(self.selected) if self.selected >= 0 else None
        if u:
            dx = (u.x - cx) % self.mw
            if dx >= self.mw // 2:
                dx -= self.mw
            sx, sy = MAP_W // 2 + dx * hw, MAP_Y + MAP_H // 2 + (u.y - cy) * hh
            pg.draw.polygon(scr, (255, 255, 255), [(sx, sy - 14), (sx + 29, sy), (sx, sy + 14), (sx - 29, sy)], 1)

    def draw_map(self):
        self.screen.set_clip(self.pg.Rect(0, MAP_Y, MAP_W, MAP_H))
        if self.view.has_art:
            labels = self.view.draw_map(self.map_frame, self.cam, self.zoom, self.selected)
            self.blit_frame(self.map_frame, (0, MAP_Y))
            if not self.panel and not self.base_open:
                self.draw_items(labels, 0, MAP_Y)
        else:
            self.draw_map_fallback()
        if self.hover and self.target_mode:
            p = self.view.tile_position(self.cam, self.zoom, MAP_W, MAP_H, *self.hover)
            if p:
                sx, sy = p[0], p[1] + MAP_Y
                hw, hh = 32 * self.zoom, 16 * self.zoom
                self.pg.draw.polygon(self.screen, (255, 255, 120), [(sx, sy - hh), (sx + hw, sy), (sx, sy + hh), (sx - hw, sy)], 1)
        self.screen.set_clip(None)

    def draw_top(self):
        g = self.g
        self.screen.fill((7, 19, 22), (0, 0, W, TOP_H))
        f = g.faction(self.player) if self.player > 0 else None
        line = f'{f.formal_name if f else "Planet"} | Turn {g.turn} | Year {g.year} | {self.mw} x {self.mh} | {g.vehicle_count} units, {g.base_count} bases'
        self.text_base(line, 12, 18, INK_RGB[2], self.font, W - 24)
        help_line = ('Click the destination square (ESC cancels)' if self.target_mode else
                     'Keypad move/attack | E end turn | I base, P production | T research | P social | K design | X comm link | V save | Q quit')
        self.text_base(self.status or help_line, 12, 38, (220, 214, 160) if self.status else INK_RGB[0], self.font, W - 24)

    def draw_hud(self):
        g, hf = self.g, self.hud_frame
        hf.clear((7, 19, 22))
        self.view.draw_console(hf, CONSOLE_X, 0)
        self.view.draw_minimap(hf, MINIMAP, self.cam, self.zoom, MAP_W, MAP_H)
        self.blit_frame(hf, (0, HUD_Y))
        ox, oy = CONSOLE_X, HUD_Y

        def hud(s, x, y, width=220, heading=False, ink=0):
            self.text_base(s, ox + x, oy + y, INK_RGB[ink], self.font_b if heading else self.font, width)
        u = g.unit(self.selected) if self.selected >= 0 else None
        if u:
            hud(u.name, 10, 86, 116, ink=2)
            hud(f'Position {u.x}, {u.y}', 10, 105, 116)
            owner = g.faction(u.faction)
            hud(owner.formal_name if owner and u.faction > 0 else 'Native life', 10, 123, 116)
            hud(f'Damage {u.damage}', 10, 141, 116)
            hud(f'Moves {u.moves_left}/{u.speed}' + ('' if u.own else ' (not yours)'), 10, 160, 116)
            hud('TAB: next unit', 10, 215, 116)
            hud('C: centre unit', 10, 234, 116)
            t = g.tile(u.x, u.y)
            if t:
                hud('TERRAIN', 146, 88, 98, ink=1)
                hud('Ocean' if t.ocean else 'Land', 146, 109, 98, ink=2)
                if not t.ocean:
                    hud('Rocky' if t.rocky >= 2 else 'Rolling' if t.rocky == 1 else 'Flat', 146, 128, 98)
                if t.items & TileBits.FUNGUS:
                    hud('Fungus', 146, 147, 98)
                if t.items & TileBits.RIVER:
                    hud('River', 146, 166, 98)
                if t.items & TileBits.FOREST:
                    hud('Forest', 146, 184, 98)
                hud('Engine game', 146, 215, 98)
                hud('Your orders', 146, 234, 98)
        hud(f'Turn {g.turn}   Year {g.year}   Energy {g.credits}', 274, 60, 260, ink=2)
        b = g.base(self.selected_base) if self.selected_base >= 0 else None
        if b:
            hud(b.name, 274, 89, 260, heading=True, ink=3)
            hud(f'Population {b.pop_size}', 274, 113)
            hud(f'Nutrients {b.nutrients}   Minerals {b.minerals}', 274, 137, 260)
            hud(f'Surplus  N {b.nutrient_surplus:+d}   M {b.mineral_surplus:+d}   E {b.energy_surplus:+d}', 274, 161, 260)
            hud('PRODUCTION', 274, 212, 260, ink=1)
            hud(f'{b.item_name} ({b.minerals}/{b.cost})', 274, 236, 260, ink=2)
        x, y, w, h = END_BUTTON
        self.screen.fill((0x40, 0x10, 0x10) if g.game_over else (0x30, 0x5a, 0x30), (x, y, w, h))
        self.text_base('GAME OVER' if g.game_over else 'END TURN  [E]', x + 37, y + 18, INK_RGB[3], self.font_b, 180)

    def draw_messages(self):
        if not self.messages_open:
            return
        x, y, w, h = MSG_RECT
        s = self.pg.Surface((w, h))
        s.set_alpha(215)
        s.fill((7, 19, 22))
        self.screen.blit(s, (x, y))
        self.pg.draw.rect(self.screen, (80, 110, 100), (x, y, w, h), 1)
        self.text_base('MESSAGES  (M closes)', x + 8, y + 16, INK_RGB[1], self.font, w - 16)
        lines = self.log[-8:]
        yy = y + 36
        for m in lines:
            self.text_base(f't{m.turn} {m.text}', x + 8, yy, MSG_RGB.get(m.kind, (180, 180, 180)), self.font_s, w - 16)
            yy += 17

    def draw_overlay(self):
        self.panel_items = []
        if self.base_open and self.selected_base >= 0 and not self.panel:
            self.full_frame.clear()
            items = self.view.draw_base_window(self.full_frame, BASEWIN_RECT, self.selected_base)
            if items is not None:
                self.blit_frame(self.full_frame, BASEWIN_RECT[:2], BASEWIN_RECT)
                self.draw_items(items)
                self.panel_items = items
        if not self.panel:
            return
        if self.panel == 'gameover':
            x, y = (W - 464) // 2, MAP_Y + 100
            self.screen.fill((30, 8, 8), (x, y, 464, 160))
            self.pg.draw.rect(self.screen, (170, 80, 60), (x, y, 464, 160), 1)
            self.text_base('GAME OVER', x + 24, y + 40, INK_RGB[3], self.font_b)
            self.text_base(f'The game ended on turn {self.g.turn} (mission year {self.g.year}).', x + 24, y + 72, INK_RGB[0])
            self.text_base('V saves the final state, Q quits. ESC shows the map.', x + 24, y + 100, INK_RGB[0])
            return
        self.full_frame.clear()
        model = self.g.battle_panel() if self.panel == 'battle' else self.model
        rect = self.panel_rect()
        items, _ = render_panel(self.panel, model, self.full_frame.buf, W, H, rect, 96)
        self.blit_frame(self.full_frame, rect[:2], rect)
        self.draw_items(items)
        self.panel_items = items

    def draw(self):
        if self.mode == 'title':
            self.draw_title()
        elif self.mode == 'newgame':
            self.draw_newgame()
        elif self.mode == 'loadlist':
            self.draw_loadlist()
        else:
            self.draw_map()
            self.draw_messages()
            self.draw_overlay()
            self.draw_top()
            self.draw_hud()
        self.frames += 1
        self.audio.tick()

    # ---- loop ----
    def handle(self, ev):
        pg = self.pg
        if ev.type == pg.QUIT:
            self.running = False
        elif ev.type == pg.KEYDOWN:
            ctrl = bool(ev.mod & pg.KMOD_CTRL)
            arrows = {pg.K_LEFT: (-2, 0), pg.K_RIGHT: (2, 0), pg.K_UP: (0, -2), pg.K_DOWN: (0, 2)}
            digit = {pg.K_LEFT: '4', pg.K_RIGHT: '6', pg.K_UP: '8', pg.K_DOWN: '2'}
            kp = {pg.K_KP1: '1', pg.K_KP2: '2', pg.K_KP3: '3', pg.K_KP4: '4', pg.K_KP5: '5', pg.K_KP6: '6', pg.K_KP7: '7',
                  pg.K_KP8: '8', pg.K_KP9: '9', pg.K_KP_ENTER: '\r', pg.K_RETURN: '\r', pg.K_ESCAPE: '\x1b', pg.K_TAB: '\t',
                  pg.K_BACKSPACE: '\x08'}
            if ev.key in arrows and (self.mode != 'map' or self.panel):
                self.key(ord(digit[ev.key]))
            elif ev.key in arrows:
                self.scroll(*arrows[ev.key])
            elif ev.key in kp:
                self.key(ord(kp[ev.key]))
            elif ctrl and ev.key in (pg.K_l, pg.K_m, pg.K_n):
                self.key(ord({pg.K_l: 'l', pg.K_m: 'm', pg.K_n: 'n'}[ev.key]), ctrl=True)
            elif ev.unicode:
                self.key(ord(ev.unicode[0]))
        elif ev.type == pg.MOUSEBUTTONDOWN:
            if ev.button in (1, 3):
                self.click(*ev.pos, button=ev.button)
            elif ev.button in (4, 5) and self.mode == 'map':
                self.scroll(0, -2 if ev.button == 4 else 2)
        elif ev.type == pg.MOUSEMOTION and self.mode == 'map':
            x, y = ev.pos
            self.hover = self.screen_tile(x, y) if MAP_Y <= y < HUD_Y else None

    def loop(self, max_frames=None, script=None, screenshot=None, fps=30, shots=None):
        """Run the window. script: steps fed one per frame (a key code or
        character, ('click', x, y[, button]) or (x, y), ('shot', path) saves a
        PNG of the current frame). Returns the number of frames drawn."""
        pg = self.pg
        clock = pg.time.Clock()
        script = list(script or [])
        while self.running:
            for ev in pg.event.get():
                self.handle(ev)
            step = script.pop(0) if script else None
            pending_shot = None
            if isinstance(step, tuple) and step and step[0] == 'shot':
                pending_shot = step[1]
            elif isinstance(step, tuple):
                args = step[1:] if step[0] == 'click' else step
                self.click(*args)
            elif step:
                self.key(ord(step) if isinstance(step, str) else step)
            self.draw()
            pg.display.flip()
            if pending_shot:
                Path(pending_shot).parent.mkdir(parents=True, exist_ok=True)
                pg.image.save(self.screen, str(pending_shot))
                if shots is not None:
                    shots.append(pending_shot)
            if max_frames is not None and self.frames >= max_frames:
                break
            if fps:
                clock.tick(fps)
        if screenshot:
            Path(screenshot).parent.mkdir(parents=True, exist_ok=True)
            pg.image.save(self.screen, str(screenshot))
        return self.frames

    def close(self):
        self.audio.close()
        self.view.close()
        if self.g is not None:
            self.g.close()
            self.g = None
        self.pg.quit()


ORDER_KEYS = {'r': (Cmd.TERRAFORM, 5), 'f': (Cmd.TERRAFORM, 0), 'F': (Cmd.TERRAFORM, 4), 'm': (Cmd.TERRAFORM, 2),
              's': (Cmd.TERRAFORM, 3), 'S': (Cmd.TERRAFORM, 9), 'l': (Cmd.SENTRY, 0), 'h': (Cmd.HOLD, 0), 'H': (Cmd.SET_HOME, 0),
              'j': (Cmd.RETURN_HOME, 0), 'w': (Cmd.WAKE, 0), 'D': (Cmd.DISBAND, 0), 'u': (Cmd.UPGRADE, -1), 'U': (Cmd.UPGRADE, -1)}


def parse_script(text):
    """--script: comma-separated steps; a step is one key character, a name
    (ENTER ESC TAB SPACE BACK), 'click:X:Y', 'shot:PATH' or empty (an idle frame)."""
    names = {'ENTER': 13, 'ESC': 27, 'TAB': 9, 'SPACE': 32, 'BACK': 8}
    out = []
    for part in text.split(','):
        p = part.strip()
        if p.startswith('click:'):
            _, x, y = p.split(':')
            out.append(('click', int(x), int(y)))
        elif p.startswith('shot:'):
            out.append(('shot', p[5:]))
        elif p in names:
            out.append(names[p])
        else:
            out.append(p[:1] if p else '')
    return out


def run_frontend(game=None, frames=None, script=None, screenshot=None, save_dir='.', fps=30, rules=None, data_dir=None, audio=True,
                 start_title=None):
    app = App(game, save_dir=save_dir, rules=rules, data_dir=data_dir, audio=audio, start_title=start_title)
    try:
        return app.loop(max_frames=frames, script=script, screenshot=screenshot, fps=fps), app
    finally:
        app.close()
