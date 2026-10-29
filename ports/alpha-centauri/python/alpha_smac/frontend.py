# SPDX-License-Identifier: GPL-3.0-or-later
"""pygame frontend: python -m alpha_smac [--data PATH | --synthetic].

The map, units, bases, side panel and message log are drawn here from the
engine's records (asmac_tiles / units / bases); the production, research,
social, design, battle and diplomacy panels and the base window are drawn by
the native ui_*.cpp renderers into an RGB frame and blitted, with their text
items drawn with a pygame font. Keys follow the native map window
(original_map_window.cpp play_key) where it has one:

  keypad 1-9 / digits    move the selected unit (into an enemy: battle odds, RETURN attacks)
  5 or SPACE  skip         N or TAB  next unit       E  end turn
  B  build base (colony pod), otherwise next own base   I  base window
  P  production (base window open) / social engineering
  T  research   K  design workshop   X  comm link (diplomacy)
  G  go to (click)  R road to (click)  r road  f farm  F forest  m mine  s solar  S sensor
  l sentry  h hold  H set home  j return home  w wake  D disband  U upgrade
  M  message log  O  all squares / player view   C  centre on unit
  V  save   Ctrl+L  reload the last save   Q  quit (twice with unsaved turns)
  arrows / mouse wheel scroll, +/- zoom, left click select / attack, right click centre
Panels: 8/2/4/6 (or arrows) move, RETURN applies, ESC closes (as natively).
"""
from pathlib import Path

from . import _lib as L
from .game import Cmd, Dir, Game, Key, MsgKind, Status, TileBits, World, panel_key, render_panel

W, H = 1024, 768
MAP_W, MAP_H = 744, 600
SIDE_X = MAP_W
LOG_Y = MAP_H

FACTION_RGB = [(170, 170, 170), (220, 200, 40), (60, 170, 60), (220, 80, 60), (90, 120, 230), (210, 120, 40),
               (170, 90, 200), (60, 190, 190)]
INK_RGB = [(150, 172, 153), (128, 148, 195), (184, 194, 88), (216, 205, 92), (232, 224, 112), (205, 164, 90)]
MSG_RGB = {MsgKind.INFO: (180, 190, 180), MsgKind.ACTION: (150, 180, 220), MsgKind.WARNING: (230, 160, 80),
           MsgKind.COMBAT: (235, 110, 100), MsgKind.BASE: (220, 210, 120), MsgKind.RESEARCH: (140, 210, 160)}
PANEL_RECT = (92, 50, 744, 398)
BATTLE_RECT = (192, 110, 544, 300)
BASEWIN_RECT = (40, 40, 560, 420)

BASE_EDIT_SET, BASE_EDIT_QUEUE_ADD, BASE_EDIT_QUEUE_REMOVE, BASE_EDIT_HURRY, BASE_EDIT_CLOSE = 2, 3, 4, 5, 6


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
    def __init__(self, game, title='Alpha Centauri (Python)', save_dir='.', pygame_mod=None):
        import pygame
        self.pg = pygame_mod or pygame
        pg = self.pg
        pg.display.init()
        pg.font.init()
        self.screen = pg.display.set_mode((W, H))
        pg.display.set_caption(title)
        self.font = pg.font.Font(None, 18)
        self.font_b = pg.font.Font(None, 22)
        self.font_s = pg.font.Font(None, 15)
        self.g = game
        self.save_dir = Path(save_dir)
        self.frame = bytearray(W * H * 3)
        self.zoom = 48
        self.selected = -1
        self.selected_base = -1
        self.base_open = False
        self.panel = None          # production research social design battle diplo gameover
        self.model = None
        self.attack = None
        self.target_mode = None
        self.all_tiles = False
        self.messages_open = True
        self.log = []
        self.status = ''
        self.dirty = False
        self.quit_warned = False
        self.last_save = None
        self.contact_index = 0
        self.running = True
        self.frames = 0
        self.hover = None
        self.refresh()
        self.drain()
        self.next_unit()
        if self.selected < 0:
            own = self.g.bases(self.g.player)
            if own:
                self.centre(own[0].x, own[0].y)

    # ---- state ----
    def refresh(self):
        self.mw, self.mh, self.tiles = self.g.tiles()
        self.units = self.g.units()
        self.bases = self.g.bases()
        self.player = self.g.player
        if not hasattr(self, 'cam'):
            self.cam = [0, 0]

    def drain(self):
        for m in self.g.messages():
            self.log.append(m)
        self.log = self.log[-400:]

    def run(self, kind, a=0, b=0, c=0, d=0, e=0):
        r = self.g.run(kind, a, b, c, d, e)
        if r == Status.OK:
            self.dirty = True
        self.quit_warned = False
        from .game import command_name, status_name
        msgs = self.g.messages()
        self.log.extend(msgs)
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
            u = self.g.unit(self.selected)
            if u:
                self.centre(u.x, u.y)
            if not u or not u.own or u.moves_left <= 0:
                self.next_unit()

    def attack_preview(self, veh, x, y):
        self.attack = (veh, x, y)
        self.g.run(Cmd.ODDS, veh, x, y)
        self.log.extend(self.g.messages())
        self.panel = 'battle'

    def end_turn(self):
        self.panel = None
        self.target_mode = None
        r = self.run(Cmd.END_TURN)
        self.messages_open = True
        if r == Status.OK:
            self.selected = -1
            self.next_unit()
            self.status = f'Turn {self.g.turn}, mission year {self.g.year}. Your move.'

    def open_panel(self, name):
        g = self.g
        m = None
        if name == 'production':
            if self.selected_base < 0:
                self.status = 'Choose one of your bases first (B or a click).'
            else:
                m = g.production_panel(self.selected_base)
                if m is None:
                    self.status = 'That base is not yours.'
        elif name == 'research':
            m = g.research_panel()
            if m is not None:
                m.confirm = 0
        elif name == 'social':
            m = g.social_panel()
        elif name == 'design':
            m = g.design_panel()
        elif name == 'diplo':
            from .game import diplomacy_available
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
        return m is not None

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
                self.run(Cmd.ATTACK, *self.attack)
                self.panel = 'battle' if g.battle_panel().valid else None
            elif a == Key.A_CLOSE:
                self.panel = None
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
                self.panel = None
                return True
            self.refresh_panel()
            return True
        if p == 'research':
            a = panel_key('research', m, key)
            if a == Key.A_APPLY:
                r = self.run(Cmd.RESEARCH, m.ids[m.selected], 0, 1 if m.confirm else 0)
                if r == Status.NEEDS_CONFIRM:
                    m.confirm = 1
                    self.status = 'Changing research halves the progress. RETURN again confirms.'
                else:
                    m.confirm = 0
                    self.refresh_panel()
            elif a == Key.A_CLOSE:
                self.panel = None
            return True
        if p == 'social':
            a = panel_key('social', m, key)
            if a == Key.A_APPLY:
                self.run(Cmd.SOCIAL, m.row, m.col)
                self.refresh_panel()
            elif a == Key.A_CLOSE:
                self.panel = None
            return True
        if p == 'design':
            a = panel_key('design', m, key)
            if a == Key.A_MOVED:
                g.design_preview(m)
            elif a == Key.A_APPLY:
                self.run(Cmd.DESIGN, *Game.design_selection(m))
                self.refresh_panel()
            elif a == Key.A_CLOSE:
                self.panel = None
            return True
        if p == 'diplo':
            if key in (Key.LEFT, Key.RIGHT):
                g.run(Cmd.DIPLO, m.other, -2)
                self.contact_index += -1 if key == Key.LEFT else 1
                self.open_panel('diplo')
                return True
            a = panel_key('diplo', m, key)
            if a == Key.A_APPLY:
                self.run(Cmd.DIPLO, m.other, m.proposals[m.selected])
                self.refresh_panel()
            elif a == Key.A_CLOSE:
                g.run(Cmd.DIPLO, m.other, -2)
                self.panel = None
            self.log.extend(g.messages())
            return True
        if p == 'gameover':
            if code in (27, 13):
                self.panel = None
            return code not in (ord('q'), ord('Q'), ord('v'), ord('V'))
        return False

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
        r = self.g.load_bytes(self.last_save.read_bytes())
        self.drain()
        if r == Status.OK:
            self.refresh()
            self.panel = None
            self.dirty = False
            self.selected = -1
            self.next_unit()
            self.status = f'Reloaded {self.last_save}'
        else:
            self.status = 'Reload failed.'

    # ---- input ----
    def key(self, code, ctrl=False):
        """One key as the native window's character code (VANILLAKEY)."""
        if code in (ord('q'), ord('Q')) and self.panel in (None, 'gameover'):
            if self.dirty and not self.quit_warned:
                self.quit_warned = True
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
            self.all_tiles = not self.all_tiles
        elif c in ('c', 'C'):
            u = self.g.unit(self.selected) if self.selected >= 0 else None
            if u:
                self.centre(u.x, u.y)
        elif c in ('+', '='):
            self.zoom = min(80, self.zoom + 16)
        elif c == '-':
            self.zoom = max(32, self.zoom - 16)

    def cycle_base(self):
        own = [b for b in self.bases if b.faction == self.player]
        if not own:
            self.status = 'You have no base.'
            return
        ids = [b.id for b in own]
        nxt = next((i for i in ids if i > self.selected_base), ids[0])
        self.selected_base = nxt
        b = self.g.base(nxt)
        self.centre(b.x, b.y)

    def scroll(self, dx, dy):
        self.cam[0] = (self.cam[0] + dx) % max(1, self.mw)
        self.cam[1] = max(0, min(self.mh - 1, self.cam[1] + dy))
        if (self.cam[0] + self.cam[1]) & 1:
            self.cam[1] += 1 if self.cam[1] + 1 < self.mh else -1

    def click(self, px, py, button=1):
        if px >= SIDE_X:
            if self.end_button.collidepoint(px, py):
                self.end_turn()
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
            return
        u = self.g.unit(self.selected) if self.selected >= 0 else None
        if self.g.hostile_at(tx, ty) and u and u.own:
            self.attack_preview(self.selected, tx, ty)

    # ---- projection ----
    def tile_pos(self, x, y):
        hw, hh = self.zoom // 2, self.zoom // 4
        dx = (x - self.cam[0]) % self.mw
        if dx >= self.mw // 2:
            dx -= self.mw
        return MAP_W // 2 + dx * hw, MAP_H // 2 + (y - self.cam[1]) * hh

    def screen_tile(self, px, py):
        hw, hh = self.zoom / 2, self.zoom / 4
        u, v = (px - MAP_W // 2) / hw, (py - MAP_H // 2) / hh
        a, b = round((u + v) / 2), round((v - u) / 2)
        x, y = (self.cam[0] + a - b) % self.mw, self.cam[1] + a + b
        return (x, y) if 0 <= y < self.mh else None

    def visible(self, t):
        return self.all_tiles or bool(t.visibility & (1 << self.player)) if self.player > 0 else True

    # ---- drawing ----
    def text(self, s, x, y, rgb=(200, 210, 200), font=None, width=None):
        f = font or self.font
        surf = f.render(s, True, rgb)
        if width is not None and surf.get_width() > width:
            surf = surf.subsurface((0, 0, max(1, width), surf.get_height()))
        self.screen.blit(surf, (x, y))
        return surf.get_height()

    def draw_map(self):
        pg, scr = self.pg, self.screen
        scr.fill((6, 15, 24), (0, 0, MAP_W, MAP_H))
        hw, hh = self.zoom // 2, self.zoom // 4
        cols = MAP_W // hw + 4
        rows = MAP_H // hh + 4
        cx, cy = self.cam
        for y in range(max(0, cy - rows // 2), min(self.mh, cy + rows // 2 + 1)):
            for raw in range(cx - cols // 2, cx + cols // 2 + 1):
                x = raw % self.mw
                if (x + y) & 1:
                    continue
                t = self.tiles[y * self.mw + x]
                if not t.valid:
                    continue
                sx, sy = self.tile_pos(x, y)
                if sx < -hw or sx > MAP_W + hw or sy < -hh or sy > MAP_H + hh:
                    continue
                poly = [(sx, sy - hh), (sx + hw, sy), (sx, sy + hh), (sx - hw, sy)]
                if not self.visible(t):
                    pg.draw.polygon(scr, (12, 14, 18), poly)
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
                if it & TileBits.SOLAR:
                    pg.draw.rect(scr, (60, 110, 200), (sx - hw // 5, sy - hh // 5, hw // 3, hh // 3))
                if it & TileBits.RIVER and not t.ocean:
                    pg.draw.line(scr, (70, 130, 230), (sx - hw // 2, sy + hh // 3), (sx + hw // 2, sy - hh // 3), 2)
                if it & (TileBits.ROAD | TileBits.MAGTUBE):
                    pg.draw.line(scr, (120, 85, 50) if not it & TileBits.MAGTUBE else (200, 200, 230), (sx - hw // 2, sy), (sx + hw // 2, sy), 2)
                if it & TileBits.SUPPLY_POD:
                    pg.draw.polygon(scr, (230, 200, 60), [(sx, sy - 4), (sx + 4, sy), (sx, sy + 4), (sx - 4, sy)])
                if it & TileBits.MONOLITH:
                    pg.draw.rect(scr, (240, 240, 240), (sx - 2, sy - hh // 2, 4, hh))
                if t.owner > 0:
                    pg.draw.polygon(scr, FACTION_RGB[t.owner % 8], poly, 1)
        # bases
        for b in self.bases:
            if not (0 <= b.x < self.mw and 0 <= b.y < self.mh):
                continue
            t = self.tiles[b.y * self.mw + b.x]
            if not self.visible(t):
                continue
            sx, sy = self.tile_pos(b.x, b.y)
            if not (-60 < sx < MAP_W + 60 and -30 < sy < MAP_H + 30):
                continue
            s = max(10, self.zoom // 3)
            rect = pg.Rect(sx - s // 2, sy - s // 2, s, s)
            pg.draw.rect(scr, FACTION_RGB[b.faction % 8], rect)
            pg.draw.rect(scr, (255, 255, 255) if b.id == self.selected_base else (20, 20, 20), rect, 2 if b.id == self.selected_base else 1)
            self.text(str(b.pop_size), sx - 4, sy - 6, (0, 0, 0), self.font_s)
            self.text(b.name, sx - s, sy + s // 2 + 1, (240, 235, 200), self.font_s)
        # units (one marker per square: the first unit, plus the stack size)
        shown = set()
        blink = (self.frames // 15) % 2 == 0
        for u in self.units:
            if (u.x, u.y) in shown and u.id != self.selected:
                continue
            t = self.tiles[u.y * self.mw + u.x] if 0 <= u.y < self.mh and 0 <= u.x < self.mw else None
            if t is None or not t.valid or not self.visible(t):
                continue
            shown.add((u.x, u.y))
            sx, sy = self.tile_pos(u.x, u.y)
            if not (-30 < sx < MAP_W + 30 and -30 < sy < MAP_H + 30):
                continue
            r = max(5, self.zoom // 7)
            ox = sx + (r if self.g.base_at(u.x, u.y) >= 0 else 0)
            pg.draw.circle(scr, FACTION_RGB[u.faction % 8], (ox, sy - r // 2), r)
            pg.draw.circle(scr, (10, 10, 10), (ox, sy - r // 2), r, 1)
            letter = 'C' if u.colony else 'F' if u.former else (u.name[:1] or '?')
            self.text(letter, ox - 4, sy - r // 2 - 6, (0, 0, 0), self.font_s)
            if u.id == self.selected and blink:
                pg.draw.circle(scr, (255, 255, 255), (ox, sy - r // 2), r + 3, 2)
        n_here = {}
        for u in self.units:
            n_here[(u.x, u.y)] = n_here.get((u.x, u.y), 0) + 1
        for (x, y), n in n_here.items():
            if n > 1 and (x, y) in shown:
                sx, sy = self.tile_pos(x, y)
                self.text(str(n), sx + 6, sy - 14, (255, 255, 255), self.font_s)
        if self.hover:
            sx, sy = self.tile_pos(*self.hover)
            pg.draw.polygon(scr, (255, 255, 255), [(sx, sy - hh), (sx + hw, sy), (sx, sy + hh), (sx - hw, sy)], 1)

    def draw_side(self):
        pg, scr, g = self.pg, self.screen, self.g
        scr.fill((14, 22, 30), (SIDE_X, 0, W - SIDE_X, H))
        pg.draw.line(scr, (80, 100, 90), (SIDE_X, 0), (SIDE_X, H))
        x, y = SIDE_X + 10, 8
        f = g.faction(self.player) if self.player > 0 else None
        y += self.text(f'{f.formal_name if f else "Planet"}', x, y, INK_RGB[3], self.font_b) + 2
        y += self.text(f'Turn {g.turn}  Mission year {g.year}', x, y, INK_RGB[2]) + 2
        y += self.text(f'Energy: {g.credits}', x, y) + 2
        if f:
            tech = g.research_panel()
            y += self.text(f'Research: {tech.current_name.decode("cp1252") if tech else "-"}', x, y, (160, 200, 170), width=260) + 2
            y += self.text(f'Bases {f.bases}  Units {f.units}  Techs {f.techs_known}', x, y) + 6
        u = g.unit(self.selected) if self.selected >= 0 else None
        y += self.text('UNIT', x, y, INK_RGB[1]) + 1
        if u:
            y += self.text(f'{u.name} (#{u.id})', x, y, FACTION_RGB[u.faction % 8]) + 1
            y += self.text(f'at {u.x},{u.y}  moves {u.moves_left}/{u.speed}', x, y) + 1
            y += self.text(f'damage {u.damage}  order {u.order}{"  colony" if u.colony else ""}{"  former" if u.former else ""}', x, y) + 6
        else:
            y += self.text('-', x, y) + 6
        y += self.text('BASE', x, y, INK_RGB[1]) + 1
        b = g.base(self.selected_base) if self.selected_base >= 0 else None
        if b:
            y += self.text(f'{b.name} (pop {b.pop_size})', x, y, FACTION_RGB[b.faction % 8]) + 1
            y += self.text(f'builds {b.item_name}: {b.minerals}/{b.cost}', x, y, width=260) + 1
            y += self.text(f'+{b.nutrient_surplus} food  +{b.mineral_surplus} min  +{b.energy_surplus} energy', x, y) + 6
        else:
            y += self.text('-', x, y) + 6
        if self.hover:
            t = self.tiles[self.hover[1] * self.mw + self.hover[0]]
            if t.valid:
                kind = 'ocean' if t.ocean else ['arid', 'moist', 'rainy'][min(t.rainfall, 2)]
                extra = [n for n, bit in (('fungus', TileBits.FUNGUS), ('forest', TileBits.FOREST), ('road', TileBits.ROAD),
                                          ('river', TileBits.RIVER), ('farm', TileBits.FARM), ('mine', TileBits.MINE)) if t.items & bit]
                y += self.text(f'Square {self.hover[0]},{self.hover[1]}: {kind} alt {t.altitude}' + (' rocky' if t.rocky >= 2 else ''),
                               x, y, (170, 180, 190), width=260) + 1
                y += self.text(' '.join(extra) or ' ', x, y, (170, 180, 190)) + 6
        y += self.text('FACTIONS', x, y, INK_RGB[1]) + 1
        for fi in g.factions():
            y += self.text(f'{fi.noun:<12} bases {fi.bases:<3} units {fi.units}', x, y, FACTION_RGB[fi.id % 8], self.font_s) + 1
        self.end_button = pg.Rect(x, H - 120, 250, 30)
        pg.draw.rect(scr, (40, 60, 40), self.end_button)
        pg.draw.rect(scr, (140, 180, 120), self.end_button, 1)
        self.text('END TURN (E)', x + 70, H - 112, (230, 240, 210))
        hy = H - 82
        for line in ('Keypad move  N next  E end turn  B/I base', 'P prod/social  T research  K design  X diplo',
                     'V save  ^L reload  O fog  M log  Q quit'):
            hy += self.text(line, x, hy, (120, 130, 140), self.font_s) + 2

    def draw_log(self):
        scr = self.screen
        scr.fill((8, 12, 16), (0, LOG_Y, MAP_W, H - LOG_Y))
        self.pg.draw.line(scr, (80, 100, 90), (0, LOG_Y), (MAP_W, LOG_Y))
        self.text(self.status, 8, LOG_Y + 4, (230, 220, 150), width=MAP_W - 16)
        if not self.messages_open:
            return
        lines = self.log[-9:]
        y = LOG_Y + 24
        for m in lines:
            y += self.text(f't{m.turn} {m.text}', 8, y, MSG_RGB.get(m.kind, (180, 180, 180)), self.font_s, width=MAP_W - 16) + 2

    def blit_native(self, rect, items):
        """Blit the rect of the native RGB frame and draw its text items."""
        pg = self.pg
        surf = pg.image.frombuffer(bytes(self.frame), (W, H), 'RGB')
        x, y, w, h = rect
        self.screen.blit(surf, (x, y), pg.Rect(x, y, w, h))
        for ix, base, width, ink, heading, s in items:
            f = self.font_b if heading else self.font
            self.text(s, ix, base - f.get_ascent(), INK_RGB[ink % len(INK_RGB)], f, width=max(1, width))

    def draw_overlay(self):
        if self.base_open and self.selected_base >= 0 and not self.panel:
            self.frame[:] = bytes(len(self.frame))
            w = self.g.world()
            n = w.render_base_window(self.selected_base, self.player, self.frame, W, H, BASEWIN_RECT)
            if n >= 0:
                self.blit_native(BASEWIN_RECT, w.last_items)
            w.close()
        if not self.panel:
            return
        if self.panel == 'gameover':
            r = self.pg.Rect(232, 150, 464, 160)
            self.screen.fill((30, 8, 8), r)
            self.pg.draw.rect(self.screen, (170, 80, 60), r, 1)
            self.text('GAME OVER', 256, 170, INK_RGB[3], self.font_b)
            self.text(f'The game ended on turn {self.g.turn} (mission year {self.g.year}).', 256, 210)
            self.text('V saves the final state, Q quits. ESC shows the map.', 256, 238)
            return
        self.frame[:] = bytes(len(self.frame))
        model = self.g.battle_panel() if self.panel == 'battle' else self.model
        rect = BATTLE_RECT if self.panel == 'battle' else PANEL_RECT
        items, _ = render_panel(self.panel, model, self.frame, W, H, rect, 96)
        self.blit_native(rect, items)

    def draw(self):
        self.draw_map()
        self.draw_side()
        self.draw_log()
        self.draw_overlay()
        self.frames += 1

    # ---- loop ----
    def handle(self, ev):
        pg = self.pg
        if ev.type == pg.QUIT:
            self.running = False
        elif ev.type == pg.KEYDOWN:
            ctrl = bool(ev.mod & pg.KMOD_CTRL)
            arrows = {pg.K_LEFT: (-2, 0), pg.K_RIGHT: (2, 0), pg.K_UP: (0, -2), pg.K_DOWN: (0, 2)}
            kp = {pg.K_KP1: '1', pg.K_KP2: '2', pg.K_KP3: '3', pg.K_KP4: '4', pg.K_KP5: '5', pg.K_KP6: '6', pg.K_KP7: '7',
                  pg.K_KP8: '8', pg.K_KP9: '9', pg.K_KP_ENTER: '\r', pg.K_RETURN: '\r', pg.K_ESCAPE: '\x1b', pg.K_TAB: '\t'}
            if self.panel and ev.key in arrows:
                self.key(ord({pg.K_LEFT: '4', pg.K_RIGHT: '6', pg.K_UP: '8', pg.K_DOWN: '2'}[ev.key]))
            elif ev.key in arrows:
                self.scroll(*arrows[ev.key])
            elif ev.key in kp:
                self.key(ord(kp[ev.key]))
            elif ctrl and ev.key == pg.K_l:
                self.key(ord('l'), ctrl=True)
            elif ev.unicode:
                self.key(ord(ev.unicode[0]))
        elif ev.type == pg.MOUSEBUTTONDOWN:
            if ev.button in (1, 3):
                self.click(*ev.pos, button=ev.button)
            elif ev.button == 4:
                self.scroll(0, -2)
            elif ev.button == 5:
                self.scroll(0, 2)
        elif ev.type == pg.MOUSEMOTION:
            self.hover = self.screen_tile(*ev.pos) if ev.pos[0] < MAP_W and ev.pos[1] < MAP_H else None

    def loop(self, max_frames=None, script=None, screenshot=None, fps=30):
        pg = self.pg
        clock = pg.time.Clock()
        script = list(script or [])
        while self.running:
            for ev in pg.event.get():
                self.handle(ev)
            if script:
                step = script.pop(0)
                if isinstance(step, tuple):
                    self.click(*step)
                elif step:
                    self.key(ord(step) if isinstance(step, str) else step)
            self.draw()
            pg.display.flip()
            if max_frames is not None and self.frames >= max_frames:
                break
            if fps:
                clock.tick(fps)
        if screenshot:
            Path(screenshot).parent.mkdir(parents=True, exist_ok=True)
            pg.image.save(self.screen, str(screenshot))
        return self.frames

    def close(self):
        self.pg.quit()


ORDER_KEYS = {'r': (Cmd.TERRAFORM, 5), 'f': (Cmd.TERRAFORM, 0), 'F': (Cmd.TERRAFORM, 4), 'm': (Cmd.TERRAFORM, 2),
              's': (Cmd.TERRAFORM, 3), 'S': (Cmd.TERRAFORM, 9), 'l': (Cmd.SENTRY, 0), 'h': (Cmd.HOLD, 0), 'H': (Cmd.SET_HOME, 0),
              'j': (Cmd.RETURN_HOME, 0), 'w': (Cmd.WAKE, 0), 'D': (Cmd.DISBAND, 0), 'u': (Cmd.UPGRADE, -1), 'U': (Cmd.UPGRADE, -1)}


def parse_script(text):
    """--script: comma-separated steps; a step is one key character, a name
    (ENTER ESC TAB SPACE), 'click:X:Y' or empty (an idle frame)."""
    names = {'ENTER': 13, 'ESC': 27, 'TAB': 9, 'SPACE': 32}
    out = []
    for part in text.split(','):
        p = part.strip()
        if p.startswith('click:'):
            _, x, y = p.split(':')
            out.append((int(x), int(y)))
        elif p in names:
            out.append(names[p])
        else:
            out.append(p[:1] if p else '')
    return out


def run_frontend(game, frames=None, script=None, screenshot=None, save_dir='.', fps=30):
    app = App(game, save_dir=save_dir)
    try:
        return app.loop(max_frames=frames, script=script, screenshot=screenshot, fps=fps), app
    finally:
        app.close()

