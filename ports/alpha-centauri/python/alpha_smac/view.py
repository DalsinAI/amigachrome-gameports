# SPDX-License-Identifier: GPL-3.0-or-later
"""The native map window's drawing with the user's own art (asmac_view.cpp),
the native new-game screens (ui_newgame) and PCX images, for the frontend."""
import ctypes as C
from dataclasses import dataclass

from . import _lib as L
from .game import text

ART_TERRAIN, ART_UNITS, ART_CONSOLE, ART_PALETTE, ART_INFANTRY, ART_SPEEDER, ART_RULES = 1, 2, 4, 8, 16, 32, 64


def lib():
    return L.load()


class Frame:
    """An RGB24 frame the native renderers draw into."""

    def __init__(self, width, height):
        self.width, self.height = width, height
        self.buf = (C.c_uint8 * (width * height * 3))()
        self.c = L.Frame(C.cast(self.buf, C.POINTER(C.c_uint8)), width * height * 3, width, height)

    def clear(self, rgb=(0, 0, 0)):
        if rgb == (0, 0, 0):
            C.memset(self.buf, 0, len(self.buf))
        else:
            self.buf[:] = bytes(rgb) * (self.width * self.height)

    def bytes(self):
        return bytes(self.buf)


def _items(arr, n, cap):
    return [(it.x, it.baseline, it.width, it.ink, bool(it.heading), text(it.text)) for it in arr[:max(0, min(n, cap))]]


class MapView:
    """asmac_view: texture.pcx terrain, overlays, faction base art, Units.pcx
    sprites and CVR vehicles, minimap, base window and console.pcx."""

    def __init__(self, data_dir, expansion=True):
        self._h = lib().asmac_view_open(str(data_dir).encode(), int(bool(expansion)))
        if not self._h:
            raise MemoryError('asmac_view_open')
        self.data_dir = data_dir

    @property
    def assets(self):
        return lib().asmac_view_assets(self._h)

    @property
    def has_art(self):
        """Terrain and unit sheets present: the original look is available."""
        return (self.assets & (ART_TERRAIN | ART_UNITS)) == (ART_TERRAIN | ART_UNITS)

    def describe(self):
        a = self.assets
        names = [n for n, b in (('texture.pcx', ART_TERRAIN), ('Units.pcx', ART_UNITS), ('console.pcx', ART_CONSOLE),
                                ('palette.pcx', ART_PALETTE), ('infantry CVR', ART_INFANTRY), ('speeder CVR', ART_SPEEDER),
                                ('rules tables', ART_RULES)) if a & b]
        mask = a >> 16
        return names + [f'faction art {bin(mask).count("1")}/7']

    def update(self, game, reopen=False):
        return lib().asmac_view_update(self._h, game._h, int(bool(reopen)))

    @property
    def viewer(self):
        return lib().asmac_view_viewer(self._h)

    @viewer.setter
    def viewer(self, v):
        lib().asmac_view_set_viewer(self._h, v)

    def draw_map(self, frame, camera, zoom, selected=-1, cap=256):
        arr = (L.TextItem * cap)()
        n = lib().asmac_view_draw_map(self._h, C.byref(frame.c), camera[0], camera[1], zoom, selected, arr, cap)
        return _items(arr, n, cap)

    def draw_minimap(self, frame, rect, camera, zoom, view_w, view_h):
        return bool(lib().asmac_view_draw_minimap(self._h, C.byref(frame.c), C.byref(L.Rect(*rect)), camera[0], camera[1], zoom, view_w, view_h))

    def minimap_click(self, rect, px, py):
        x, y = C.c_int(), C.c_int()
        ok = lib().asmac_view_minimap_click(self._h, C.byref(L.Rect(*rect)), px, py, C.byref(x), C.byref(y))
        return (x.value, y.value) if ok else None

    def draw_base_window(self, frame, rect, base, cap=128):
        arr = (L.TextItem * cap)()
        n = lib().asmac_view_draw_base_window(self._h, C.byref(frame.c), C.byref(L.Rect(*rect)), base, arr, cap)
        return _items(arr, n, cap) if n >= 0 else None

    def draw_console(self, frame, x, y):
        return bool(lib().asmac_view_draw_console(self._h, C.byref(frame.c), x, y))

    def screen_tile(self, camera, zoom, width, height, px, py):
        x, y = C.c_int(), C.c_int()
        ok = lib().asmac_view_screen_tile(self._h, camera[0], camera[1], zoom, width, height, px, py, C.byref(x), C.byref(y))
        return (x.value, y.value) if ok else None

    def tile_position(self, camera, zoom, width, height, x, y):
        sx, sy = C.c_int(), C.c_int()
        ok = lib().asmac_view_tile_position(self._h, camera[0], camera[1], zoom, width, height, x, y, C.byref(sx), C.byref(sy))
        return (sx.value, sy.value) if ok else None

    def tile_visible(self, x, y):
        return bool(lib().asmac_view_tile_visible(self._h, x, y))

    def log(self):
        buf = C.create_string_buffer(8192)
        lib().asmac_view_messages(self._h, buf, 8192)
        return [l for l in buf.value.decode('cp1252', 'replace').split('\n') if l]

    def faction_stem(self, f):
        buf = C.create_string_buffer(32)
        return buf.value.decode() if lib().asmac_view_faction_stem(self._h, f, buf, 32) else ''

    def close(self):
        if self._h:
            lib().asmac_view_free(self._h)
            self._h = None

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass


def load_pcx(data_dir, name):
    """(width, height, RGB bytes) of a PCX in the data folder, or None."""
    h = lib().asmac_pcx_open(str(data_dir).encode(), name.encode())
    if not h:
        return None
    try:
        w, hh = C.c_int(), C.c_int()
        lib().asmac_pcx_size(h, C.byref(w), C.byref(hh))
        buf = (C.c_uint8 * (w.value * hh.value * 3))()
        lib().asmac_pcx_rgb(h, buf, len(buf))
        return w.value, hh.value, bytes(buf)
    finally:
        lib().asmac_pcx_free(h)


def find_file(data_dir, name):
    buf = C.create_string_buffer(1024)
    return buf.value.decode() if lib().asmac_find_file(str(data_dir).encode(), name.encode(), buf, 1024) else None


# ---- new game screens ----
NG_FACTION, NG_SETTINGS, NG_PLANETFALL, NG_DONE, NG_CANCELLED = range(5)
NGK_UP, NGK_DOWN, NGK_LEFT, NGK_RIGHT, NGK_ENTER, NGK_BACK = range(6)


@dataclass
class NewGameView:
    screen: int
    row: int
    title: str
    lines: list  # (text, selected, heading)


class NewGameFlow:
    """ui_newgame's faction / settings / planetfall screens on a Game whose
    rules are loaded; finish() starts turn 1 (Session::adopt_new_game)."""

    def __init__(self, game, seed):
        self.game = game
        r = lib().asmac_newgame_open(game._h, seed & 0xFFFFFFFF)
        if r != 0:
            raise RuntimeError(f'new game screens unavailable ({r})')

    def key(self, k):
        return bool(lib().asmac_newgame_key(self.game._h, k))

    @property
    def screen(self):
        return lib().asmac_newgame_screen(self.game._h)

    @property
    def done(self):
        return self.screen in (NG_DONE, NG_CANCELLED)

    @property
    def start_status(self):
        return lib().asmac_newgame_start_status(self.game._h)

    def view(self):
        title = C.create_string_buffer(160)
        arr = (L.Line * 64)()
        n = lib().asmac_newgame_view(self.game._h, title, 160, arr, 64)
        lines = [(text(l.text), bool(l.selected), bool(l.heading)) for l in arr[:max(0, min(n, 64))]]
        return NewGameView(self.screen, lib().asmac_newgame_row(self.game._h), title.value.decode('cp1252', 'replace'), lines)

    def finish(self):
        return lib().asmac_adopt_new_game(self.game._h)
