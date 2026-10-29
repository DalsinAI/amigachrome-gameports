# SPDX-License-Identifier: GPL-3.0-or-later
"""High-level Python API over the asmac C library.

Every game rule is the C++ engine's; a Game is one alpha::play::Session.
Typical use:

    from alpha_smac import Game
    g = Game.new_game(rules_dir, expansion=True, seed=7, faction=1)
    for u in g.units(own=True): ...
    g.move(unit_id, Dir.N); g.set_production(base_id, item); g.end_turn()
    g.save('my.SAV')
"""
import ctypes as C
from dataclasses import dataclass, field
from pathlib import Path

from . import _lib as L

FNV_SEED = 2166136261


class Status:
    OK, REFUSED, BAD_ARGS, NO_GAME, UNAVAILABLE, IO, GAME_OVER, ATTACK, NEEDS_CONFIRM = range(9)
    SHORT_BUFFER = 100


class Cmd:
    """alpha::play::CommandKind."""
    (NONE, MOVE, GOTO, ROAD_TO, TERRAFORM, BUILD_BASE, SENTRY, HOLD, SKIP, WAKE, DISBAND, SET_HOME, RETURN_HOME, UPGRADE, ODDS,
     ATTACK, PRODUCE, QUEUE_ADD, QUEUE_REMOVE, QUEUE_MOVE, HURRY, RESEARCH, SOCIAL, DESIGN, DIPLO, END_TURN, EDIT_UNIT,
     COUNT) = range(28)


class Dir:
    """Step directions (engine BaseOffset order) and the keypad digits of the native map window."""
    NE, E, SE, S, SW, W, NW, N = range(8)
    KEYPAD = {1: 4, 2: 3, 3: 2, 4: 5, 6: 1, 7: 6, 8: 7, 9: 0}


class Key:
    """alpha::UiKey and UiPanelAction."""
    NONE, UP, DOWN, LEFT, RIGHT, ENTER, BACK = range(7)
    A_NONE, A_MOVED, A_APPLY, A_CLOSE, A_EXTRA1, A_EXTRA2, A_EXTRA3 = range(7)


class Item:
    """alpha::BaseItemKind of production items."""
    FACILITY, PROJECT, COMBAT, COLONY, FORMER, OTHER_UNIT = range(6)


class Terraform:
    """FormerItem order numbers used by the native keys (f r m s F S)."""
    FARM, SOIL_ENRICHER, MINE, SOLAR, FOREST, ROAD, MAGTUBE, BUNKER, AIRBASE, SENSOR = range(10)


class MsgKind:
    INFO, ACTION, WARNING, COMBAT, BASE, RESEARCH = range(6)


class TileBits:
    BASE, UNIT, ROAD, MAGTUBE, MINE, FUNGUS, SOLAR, RIVER = 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
    BUNKER, MONOLITH, FARM, FOREST, CONDENSER, SUPPLY_POD, SENSOR = 0x800, 0x2000, 0x8000, 0x200000, 0x400000, 0x10000000, 0x80000000


class GameError(RuntimeError):
    def __init__(self, status, what):
        super().__init__(f'{what}: {status_name(status)}')
        self.status = status


def lib():
    return L.load()


def text(b):
    """C char array -> str (the rules text is cp1252)."""
    return bytes(b).split(b'\0', 1)[0].decode('cp1252')


def status_name(s):
    return lib().asmac_status_name(s).decode()


def command_name(kind):
    return lib().asmac_command_name(kind).decode()


def parse_command(line):
    """Text form of play scripts ("move 3 2", "endturn"): returns a Command or None."""
    c = L.Command()
    return c if lib().asmac_parse_command(line.encode('cp1252'), C.byref(c)) else None


def fnv1a(data, h=FNV_SEED):
    buf = (C.c_uint8 * len(data)).from_buffer_copy(data) if not isinstance(data, C.Array) else data
    return lib().asmac_fnv1a(buf, len(data), h)


def new_game_available():
    return bool(lib().asmac_new_game_available())


def diplomacy_available():
    return bool(lib().asmac_diplomacy_available())


@dataclass
class UnitInfo:
    id: int
    unit_id: int
    faction: int
    x: int
    y: int
    damage: int
    moves_left: int
    speed: int
    order: int
    home_base: int
    colony: bool
    former: bool
    combat: bool
    own: bool
    name: str


@dataclass
class FactionInfo:
    id: int
    alive: bool
    human: bool
    credits: int
    bases: int
    units: int
    research_tech: int
    labs: int
    techs_known: int
    social: tuple
    noun: str
    formal_name: str
    leader: str
    title: str
    adjective: str


@dataclass
class BaseInfo:
    id: int
    x: int
    y: int
    faction: int
    pop_size: int
    nutrients: int
    minerals: int
    nutrient_surplus: int
    mineral_surplus: int
    energy_surplus: int
    current_item: int
    item_name: str
    cost: int
    turns: int
    name: str


@dataclass
class Event:
    turn: int
    kind: int
    text: str


@dataclass
class TurnResult:
    status: int
    faction: int
    turn: int
    year: int
    events: int
    messages: list = field(default_factory=list)


class Panel:
    """A panel model (production, research, social, design, battle, diplomacy)
    kept on the Python side and passed in/out of the C calls like the native
    window keeps its models."""
    RENDER = {'production': 'asmac_render_base_production', 'research': 'asmac_render_research', 'social': 'asmac_render_social',
              'design': 'asmac_render_design', 'battle': 'asmac_render_battle', 'diplo': 'asmac_render_diplo'}
    KEY = {'production': 'asmac_base_production_key', 'research': 'asmac_research_key', 'social': 'asmac_social_key',
           'design': 'asmac_design_key', 'battle': 'asmac_battle_key', 'diplo': 'asmac_diplo_key'}


def render_panel(kind, model, frame_buf, width, height, rect, capacity=256):
    """Draw a panel model into frame_buf (bytearray/ctypes array, RGB24) with
    the native ui_*.cpp renderer. Returns the text items as a list of
    (x, baseline, width, ink, heading, text)."""
    if isinstance(frame_buf, bytearray):
        arr = (C.c_uint8 * len(frame_buf)).from_buffer(frame_buf)
    else:
        arr = frame_buf
    fr = L.Frame(C.cast(arr, C.POINTER(C.c_uint8)), len(frame_buf), width, height)
    rc = L.Rect(*rect)
    items = (L.TextItem * capacity)()
    n = getattr(lib(), Panel.RENDER[kind])(C.byref(model), C.byref(fr), C.byref(rc), items, capacity)
    return [(it.x, it.baseline, it.width, it.ink, bool(it.heading), text(it.text)) for it in items[:max(0, min(n, capacity))]], n


def panel_key(kind, model, key):
    return getattr(lib(), Panel.KEY[kind])(C.byref(model), key)


class World:
    """alpha::OriginalWorld: the native window's view rebuilt from SAV bytes."""

    def __init__(self, data, seed=0x12345678):
        err = C.c_int(0)
        self._h = lib().asmac_world_load(bytes(data), len(data), seed, C.byref(err))
        self.error = err.value
        self.error_name = lib().asmac_world_error_name(err.value).decode()

    @property
    def ok(self):
        return bool(self._h)

    def info(self):
        i = L.WorldInfo()
        lib().asmac_world_info_get(self._h, C.byref(i))
        return i

    def vehicle(self, i):
        x, y, u = C.c_int(), C.c_int(), C.c_int()
        if not lib().asmac_world_vehicle(self._h, i, C.byref(x), C.byref(y), C.byref(u)):
            return None
        return x.value, y.value, u.value

    def base_faction(self, i):
        return lib().asmac_world_base_faction(self._h, i)

    def render_base_window(self, base, viewer, frame_buf, width, height, rect, capacity=256):
        arr = (C.c_uint8 * len(frame_buf)).from_buffer(frame_buf)
        fr = L.Frame(C.cast(arr, C.POINTER(C.c_uint8)), len(frame_buf), width, height)
        items = (L.TextItem * capacity)()
        n = lib().asmac_world_render_base_window(self._h, base, viewer, C.byref(fr), C.byref(L.Rect(*rect)), items, capacity)
        return n

    def close(self):
        if self._h:
            lib().asmac_world_free(self._h)
            self._h = None

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass


class Game:
    """One play session (alpha::play::Session) and the engine state it owns."""

    def __init__(self, shared=False):
        self._lib = lib()
        self._h = self._lib.asmac_session_shared() if shared else self._lib.asmac_session_new()
        if not self._h:
            raise MemoryError('asmac_session_new')
        self._owned = not shared
        self.rules_dir = None
        self.expansion = None

    # ---- lifetime ----
    def close(self):
        if self._h and self._owned:
            self._lib.asmac_session_free(self._h)
        self._h = None

    def __enter__(self):
        return self

    def __exit__(self, *a):
        self.close()

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    # ---- rules, new game, load, save ----
    def load_rules(self, rules_dir, expansion=True):
        r = self._lib.asmac_load_rules(self._h, str(rules_dir).encode(), int(bool(expansion)))
        self.rules_dir, self.expansion = Path(rules_dir), bool(expansion)
        return r

    def load_rules_checked(self, rules_dir, expansion=True):
        r = self.load_rules(rules_dir, expansion)
        if r != Status.OK:
            raise GameError(r, f'rules from {rules_dir}: {self.rules_error}')
        return r

    @property
    def rules_error(self):
        return self._lib.asmac_rules_error(self._h).decode('cp1252', 'replace')

    @classmethod
    def new_game(cls, rules_dir, expansion=True, seed=1, faction=1, difficulty=2, map_size=2):
        g = cls()
        g.load_rules_checked(rules_dir, expansion)
        r = g.start(seed, faction, difficulty, map_size)
        if r != Status.OK:
            raise GameError(r, 'new game')
        return g

    @classmethod
    def load(cls, rules_dir, path, expansion=True):
        g = cls()
        g.load_rules_checked(rules_dir, expansion)
        r = g.load_bytes(Path(path).read_bytes())
        if r != Status.OK:
            raise GameError(r, f'load {path}')
        return g

    def start(self, seed=1, faction=1, difficulty=2, map_size=2):
        """engine_new_game on the loaded rules and the start of turn 1."""
        return self._lib.asmac_new_game(self._h, seed & 0xFFFFFFFF, faction, difficulty, map_size)

    def load_bytes(self, data):
        return self._lib.asmac_load_game(self._h, bytes(data), len(data))

    def _take_buffer(self, fn):
        n = C.c_size_t(0)
        r = fn(self._h, C.byref(n))
        if r != Status.OK:
            return r, b''
        buf = (C.c_uint8 * n.value)()
        r2 = self._lib.asmac_buffer(self._h, buf, n.value)
        return (r if r2 == Status.OK else r2), bytes(buf)

    def save_bytes(self):
        """(status, original SAV container bytes)."""
        return self._take_buffer(self._lib.asmac_save_game)

    def view_bytes(self):
        """(status, the engine's SAV bytes without a seed change) for OriginalWorld."""
        return self._take_buffer(self._lib.asmac_view_bytes)

    def save(self, path):
        r, data = self.save_bytes()
        if r != Status.OK:
            raise GameError(r, 'save')
        Path(path).write_bytes(data)
        return len(data)

    # ---- state ----
    loaded = property(lambda s: bool(s._lib.asmac_loaded(s._h)))
    game_over = property(lambda s: bool(s._lib.asmac_game_over(s._h)))
    player = property(lambda s: s._lib.asmac_player(s._h))
    turn = property(lambda s: s._lib.asmac_turn(s._h))
    year = property(lambda s: s._lib.asmac_year(s._h))
    credits = property(lambda s: s._lib.asmac_credits(s._h))
    vehicle_count = property(lambda s: s._lib.asmac_vehicle_count(s._h))
    base_count = property(lambda s: s._lib.asmac_base_count(s._h))

    def digest(self):
        return self._lib.asmac_digest(self._h)

    def last_turn(self):
        t = L.TurnInfo()
        self._lib.asmac_last_turn(self._h, C.byref(t))
        return t

    # ---- messages ----
    def messages(self, take=True):
        """Message-log lines since the last take, oldest first."""
        out = []
        m = L.Message()
        for i in range(self._lib.asmac_message_count(self._h)):
            if self._lib.asmac_message(self._h, i, C.byref(m)):
                out.append(Event(m.turn, m.kind, text(m.text)))
        if take:
            self._lib.asmac_take_messages(self._h)
        return out

    # ---- commands ----
    def run(self, kind, a=0, b=0, c=0, d=0, e=0, text_arg=''):
        cmd = L.Command(kind, a, b, c, d, e, text_arg.encode('cp1252')[:31])
        return self._lib.asmac_run(self._h, C.byref(cmd))

    def run_command(self, cmd):
        return self._lib.asmac_run(self._h, C.byref(cmd))

    def run_text(self, line):
        c = parse_command(line)
        if c is None:
            return Status.BAD_ARGS
        return self.run_command(c)

    def move(self, veh, direction):
        return self.run(Cmd.MOVE, veh, direction)

    def go_to(self, veh, x, y):
        return self.run(Cmd.GOTO, veh, x, y)

    def attack(self, veh, x, y):
        return self.run(Cmd.ATTACK, veh, x, y)

    def order(self, veh, kind, arg=0):
        """Unit orders: Cmd.SENTRY, HOLD, SKIP, WAKE, DISBAND, SET_HOME, RETURN_HOME, UPGRADE, TERRAFORM (arg), BUILD_BASE."""
        return self.run(kind, veh, arg)

    def terraform(self, veh, item):
        return self.run(Cmd.TERRAFORM, veh, item)

    def build_base(self, veh, name=''):
        return self.run(Cmd.BUILD_BASE, veh, text_arg=name)

    def set_production(self, base, item):
        """item: unit prototype id >= 0, facility -id."""
        return self.run(Cmd.PRODUCE, base, item)

    def queue_add(self, base, item):
        return self.run(Cmd.QUEUE_ADD, base, item)

    def hurry(self, base):
        return self.run(Cmd.HURRY, base)

    def research(self, tech, confirm=False):
        r = self.run(Cmd.RESEARCH, tech, 0, int(confirm))
        return r

    def social(self, category, model):
        return self.run(Cmd.SOCIAL, category, model)

    def design(self, chassis, weapon, armor, reactor, abilities=0):
        return self.run(Cmd.DESIGN, chassis, weapon, armor, reactor, abilities)

    def diplomacy_propose(self, other, proposal):
        return self.run(Cmd.DIPLO, other, proposal)

    def end_turn(self):
        """End the human turn; the computer factions play. Returns a TurnResult with the new messages."""
        r = self.run(Cmd.END_TURN)
        t = self.last_turn()
        return TurnResult(r, t.faction, t.turn, t.year, t.events, self.messages())

    def set_human(self, faction, human):
        return self._lib.asmac_set_human(self._h, faction, int(bool(human)))

    # ---- units, bases, factions, map ----
    def unit_raw(self, veh):
        u = L.Unit()
        ok = self._lib.asmac_unit(self._h, veh, C.byref(u))
        return u if ok else None

    def unit(self, veh):
        u = self.unit_raw(veh)
        if u is None:
            return None
        return UnitInfo(u.id, u.unit_id, u.faction, u.x, u.y, u.damage, u.moves_left, u.speed, u.order, u.home_base,
                        bool(u.colony), bool(u.former), bool(u.combat), bool(u.own), text(u.name))

    def units(self, own=None):
        out = [self.unit(v) for v in range(self.vehicle_count)]
        return [u for u in out if u and (own is None or u.own == own)]

    def next_unit(self, veh=-1):
        return self._lib.asmac_next_unit(self._h, veh)

    def unit_at(self, x, y):
        return self._lib.asmac_unit_at(self._h, x, y)

    def base_at(self, x, y):
        return self._lib.asmac_base_at(self._h, x, y)

    def step_target(self, veh, direction):
        x, y = C.c_int(), C.c_int()
        ok = self._lib.asmac_step_target(self._h, veh, direction, C.byref(x), C.byref(y))
        return bool(ok), x.value, y.value

    def hostile_at(self, x, y):
        return bool(self._lib.asmac_hostile_at(self._h, x, y))

    def upgrade_target(self, veh):
        return self._lib.asmac_upgrade_target(self._h, veh)

    def contacts(self):
        ids = (C.c_int * 8)()
        n = self._lib.asmac_contacts(self._h, ids, 8)
        return list(ids[:n])

    def faction(self, f):
        o = L.Faction()
        if not self._lib.asmac_faction(self._h, f, C.byref(o)):
            return None
        return FactionInfo(o.id, bool(o.alive), bool(o.human), o.credits, o.bases, o.units, o.research_tech, o.labs,
                           o.techs_known, tuple(o.social), text(o.noun), text(o.formal_name), text(o.leader), text(o.title),
                           text(o.adjective))

    def factions(self, alive_only=True):
        out = [self.faction(f) for f in range(1, 8)]
        return [f for f in out if f and (f.alive or not alive_only)]

    def base(self, b):
        o = L.Base()
        if not self._lib.asmac_base(self._h, b, C.byref(o)):
            return None
        return BaseInfo(o.id, o.x, o.y, o.faction, o.pop_size, o.nutrients, o.minerals, o.nutrient_surplus, o.mineral_surplus,
                        o.energy_surplus, o.current_item, text(o.item_name), o.cost, o.turns, text(o.name))

    def bases(self, faction=None):
        out = [self.base(b) for b in range(self.base_count)]
        return [b for b in out if b and (faction is None or b.faction == faction)]

    def item_name(self, item):
        return self._lib.asmac_item_name(self._h, item).decode('cp1252')

    def map_size(self):
        w, h = C.c_int(), C.c_int()
        self._lib.asmac_map_size(self._h, C.byref(w), C.byref(h))
        return w.value, h.value

    def tile(self, x, y):
        t = L.Tile()
        return t if self._lib.asmac_tile(self._h, x, y, C.byref(t)) == Status.OK else None

    def tiles(self):
        """All squares, row-major (index y*width+x); invalid squares have valid=0."""
        w, h = self.map_size()
        arr = (L.Tile * max(1, w * h))()
        r = self._lib.asmac_tiles(self._h, arr, w * h)
        if r != Status.OK:
            raise GameError(r, 'tiles')
        return w, h, arr

    # ---- panels ----
    def production_panel(self, base, model=None):
        m = model if model is not None else L.Production()
        return m if self._lib.asmac_base_production(self._h, base, C.byref(m)) else None

    def production_panel_into(self, base, model):
        return bool(self._lib.asmac_base_production(self._h, base, C.byref(model)))

    def research_panel(self, model=None):
        m = model if model is not None else L.Research()
        return m if self._lib.asmac_research(self._h, C.byref(m)) else None

    def social_panel(self, model=None):
        m = model if model is not None else L.Social()
        return m if self._lib.asmac_social(self._h, C.byref(m)) else None

    def design_panel(self, model=None):
        m = model if model is not None else L.Design()
        return m if self._lib.asmac_design(self._h, C.byref(m)) else None

    def design_preview(self, model):
        self._lib.asmac_design_preview(self._h, C.byref(model))

    @staticmethod
    def design_selection(model):
        v = [C.c_int() for _ in range(5)]
        lib().asmac_design_selection(C.byref(model), *[C.byref(x) for x in v])
        return tuple(x.value for x in v)

    def battle_panel(self):
        m = L.Battle()
        self._lib.asmac_battle_model(self._h, C.byref(m))
        return m

    def diplomacy_panel(self, other, model=None):
        m = model if model is not None else L.Diplo()
        return m if self._lib.asmac_diplomacy(self._h, other, C.byref(m)) else None

    def world(self, seed=0x12345678):
        r, data = self.view_bytes()
        if r != Status.OK:
            raise GameError(r, 'view bytes')
        return World(data, seed)
