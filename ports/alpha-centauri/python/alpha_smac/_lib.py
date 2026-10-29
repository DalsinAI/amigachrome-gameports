# SPDX-License-Identifier: GPL-3.0-or-later
"""ctypes binding of the asmac C API (../asmac.h).

The structs mirror asmac.h field by field; keep them in step with
ASMAC_ABI_VERSION. The library is found in this order: the ALPHA_SMAC_LIB
environment variable, libasmac.so next to this file. build() runs
build_python.sh to make it.
"""
import ctypes as C
import os
import subprocess
from pathlib import Path

ABI_VERSION = 1
HERE = Path(__file__).resolve().parent
PYTHON_DIR = HERE.parent
NATIVE_DIR = PYTHON_DIR.parent / 'native'
DEFAULT_LIB = HERE / 'libasmac.so'

i32 = C.c_int


def _s(fields):
    return [(n, t) for n, t in fields]


class Command(C.Structure):
    _fields_ = [('kind', i32), ('a', i32), ('b', i32), ('c', i32), ('d', i32), ('e', i32), ('text', C.c_char * 32)]


class Message(C.Structure):
    _fields_ = [('turn', i32), ('kind', i32), ('text', C.c_char * 120)]


class TurnInfo(C.Structure):
    _fields_ = [(n, i32) for n in ('status', 'faction', 'turn', 'year', 'events')]


class Unit(C.Structure):
    _fields_ = [(n, i32) for n in ('valid', 'id', 'unit_id', 'faction', 'x', 'y', 'damage', 'moves_left', 'speed', 'order',
                                   'home_base', 'colony', 'former', 'combat', 'own')] + [('name', C.c_char * 32)]


class Tile(C.Structure):
    _fields_ = [(n, i32) for n in ('valid', 'altitude', 'ocean', 'rainfall', 'temperature', 'rocky', 'owner', 'region')] + \
        [('visibility', C.c_uint), ('items', C.c_uint), ('landmarks', C.c_uint), ('base', i32), ('unit_count', i32), ('unit_faction', i32)]


class Faction(C.Structure):
    _fields_ = [(n, i32) for n in ('valid', 'id', 'alive', 'human', 'credits', 'bases', 'units', 'research_tech', 'labs',
                                   'techs_known')] + \
        [('social', i32 * 4), ('noun', C.c_char * 24), ('formal_name', C.c_char * 40), ('leader', C.c_char * 24),
         ('title', C.c_char * 24), ('adjective', C.c_char * 64)]


class Base(C.Structure):
    _fields_ = [(n, i32) for n in ('valid', 'id', 'x', 'y', 'faction', 'pop_size', 'nutrients', 'minerals', 'nutrient_surplus',
                                   'mineral_surplus', 'energy_surplus', 'current_item', 'queue_count', 'cost', 'turns')] + \
        [('name', C.c_char * 26), ('item_name', C.c_char * 32)]


class Frame(C.Structure):
    _fields_ = [('rgb', C.POINTER(C.c_uint8)), ('capacity', C.c_size_t), ('width', C.c_uint), ('height', C.c_uint)]


class Rect(C.Structure):
    _fields_ = [(n, i32) for n in ('x', 'y', 'w', 'h')]


class TextItem(C.Structure):
    _fields_ = [(n, i32) for n in ('x', 'baseline', 'width', 'ink', 'heading')] + [('text', C.c_char * 96)]


class ProductionItem(C.Structure):
    _fields_ = [('id', i32), ('kind', i32), ('name', C.c_char * 32), ('cost', i32), ('turns', i32)]


class Production(C.Structure):
    _fields_ = [('valid', i32), ('base', i32), ('name', C.c_char * 26), ('current', i32), ('current_name', C.c_char * 32),
                ('minerals', i32), ('cost', i32), ('surplus', i32), ('turns', i32), ('queue_count', i32), ('queue', i32 * 10),
                ('queue_names', (C.c_char * 32) * 10), ('hurry_cost', i32), ('credits', i32), ('hurried', i32), ('count', i32),
                ('items', ProductionItem * 96), ('selected', i32)]


class Research(C.Structure):
    _fields_ = [('valid', i32), ('faction', i32), ('current', i32), ('current_name', C.c_char * 40), ('accumulated', i32),
                ('cost', i32), ('labs', i32), ('known', i32), ('count', i32), ('ids', i32 * 32), ('names', (C.c_char * 40) * 32),
                ('categories', (C.c_char * 12) * 32), ('selected', i32), ('confirm', i32)]


class Social(C.Structure):
    _fields_ = [('valid', i32), ('faction', i32), ('categories', (C.c_char * 24) * 4), ('models', ((C.c_char * 24) * 4) * 4),
                ('available', (i32 * 4) * 4), ('cost', (i32 * 4) * 4), ('current', i32 * 4), ('credits', i32), ('row', i32), ('col', i32)]


class DesignChoice(C.Structure):
    _fields_ = [('id', i32), ('name', C.c_char * 32)]


class Design(C.Structure):
    _fields_ = [('valid', i32), ('faction', i32), ('counts', i32 * 5), ('choices', (DesignChoice * 32) * 5), ('selected', i32 * 5),
                ('row', i32), ('preview_ok', i32), ('preview_name', C.c_char * 40), ('preview_cost', i32), ('last_unit', i32)]


class Battle(C.Structure):
    _fields_ = [('valid', i32), ('preview', i32), ('x', i32), ('y', i32), ('attacker', C.c_char * 32), ('defender', C.c_char * 32),
                ('attacker_owner', C.c_char * 24), ('defender_owner', C.c_char * 24), ('odds', i32), ('outcome', i32),
                ('outcome_text', C.c_char * 48)] + \
        [(n, i32) for n in ('offense', 'defense', 'hp_atk_before', 'hp_def_before', 'hp_atk_after', 'hp_def_after', 'rounds',
                            'attacker_hits', 'defender_hits', 'killed', 'promoted_atk', 'promoted_def', 'base_captured',
                            'base_destroyed')] + \
        [('mod_count', i32 * 2), ('mods', ((C.c_char * 48) * 4) * 2)]


class Diplo(C.Structure):
    _fields_ = [('valid', i32), ('player', i32), ('other', i32), ('other_name', C.c_char * 40), ('leader', C.c_char * 48),
                ('mood', C.c_char * 24), ('status', C.c_char * 48), ('count', i32), ('proposals', i32 * 16),
                ('labels', (C.c_char * 48) * 16), ('selected', i32), ('last_answer', i32), ('reply', C.c_char * 160)]


class WorldInfo(C.Structure):
    _fields_ = [(n, i32) for n in ('vehicle_count', 'base_count', 'current_turn', 'player')]


P = C.POINTER
SESSION = C.c_void_p
WORLD = C.c_void_p
U8P = P(C.c_uint8)

_PROTOTYPES = {
    'asmac_abi_version': (i32, []),
    'asmac_status_name': (C.c_char_p, [i32]),
    'asmac_command_name': (C.c_char_p, [i32]),
    'asmac_parse_command': (i32, [C.c_char_p, P(Command)]),
    'asmac_new_game_available': (i32, []),
    'asmac_diplomacy_available': (i32, []),
    'asmac_fnv1a': (C.c_uint32, [C.c_void_p, C.c_size_t, C.c_uint32]),
    'asmac_session_new': (SESSION, []),
    'asmac_session_shared': (SESSION, []),
    'asmac_session_free': (None, [SESSION]),
    'asmac_load_rules': (i32, [SESSION, C.c_char_p, i32]),
    'asmac_rules_error': (C.c_char_p, [SESSION]),
    'asmac_new_game': (i32, [SESSION, C.c_uint32, i32, i32, i32]),
    'asmac_load_game': (i32, [SESSION, C.c_char_p, C.c_size_t]),
    'asmac_save_game': (i32, [SESSION, P(C.c_size_t)]),
    'asmac_view_bytes': (i32, [SESSION, P(C.c_size_t)]),
    'asmac_buffer': (i32, [SESSION, C.c_void_p, C.c_size_t]),
    'asmac_loaded': (i32, [SESSION]),
    'asmac_game_over': (i32, [SESSION]),
    'asmac_run': (i32, [SESSION, P(Command)]),
    'asmac_last_turn': (None, [SESSION, P(TurnInfo)]),
    'asmac_message_count': (i32, [SESSION]),
    'asmac_message': (i32, [SESSION, i32, P(Message)]),
    'asmac_take_messages': (None, [SESSION]),
    'asmac_player': (i32, [SESSION]),
    'asmac_turn': (i32, [SESSION]),
    'asmac_year': (i32, [SESSION]),
    'asmac_credits': (i32, [SESSION]),
    'asmac_vehicle_count': (i32, [SESSION]),
    'asmac_base_count': (i32, [SESSION]),
    'asmac_unit': (i32, [SESSION, i32, P(Unit)]),
    'asmac_next_unit': (i32, [SESSION, i32]),
    'asmac_unit_at': (i32, [SESSION, i32, i32]),
    'asmac_base_at': (i32, [SESSION, i32, i32]),
    'asmac_step_target': (i32, [SESSION, i32, i32, P(i32), P(i32)]),
    'asmac_hostile_at': (i32, [SESSION, i32, i32]),
    'asmac_upgrade_target': (i32, [SESSION, i32]),
    'asmac_contacts': (i32, [SESSION, P(i32), i32]),
    'asmac_digest': (C.c_uint32, [SESSION]),
    'asmac_map_size': (i32, [SESSION, P(i32), P(i32)]),
    'asmac_tile': (i32, [SESSION, i32, i32, P(Tile)]),
    'asmac_tiles': (i32, [SESSION, P(Tile), i32]),
    'asmac_faction': (i32, [SESSION, i32, P(Faction)]),
    'asmac_base': (i32, [SESSION, i32, P(Base)]),
    'asmac_item_name': (C.c_char_p, [SESSION, i32]),
    'asmac_set_human': (i32, [SESSION, i32, i32]),
    'asmac_base_production': (i32, [SESSION, i32, P(Production)]),
    'asmac_research': (i32, [SESSION, P(Research)]),
    'asmac_social': (i32, [SESSION, P(Social)]),
    'asmac_design': (i32, [SESSION, P(Design)]),
    'asmac_design_preview': (None, [SESSION, P(Design)]),
    'asmac_design_selection': (None, [P(Design), P(i32), P(i32), P(i32), P(i32), P(i32)]),
    'asmac_battle_model': (None, [SESSION, P(Battle)]),
    'asmac_diplomacy': (i32, [SESSION, i32, P(Diplo)]),
    'asmac_ui_key_from_char': (i32, [i32]),
    'asmac_base_production_key': (i32, [P(Production), i32]),
    'asmac_research_key': (i32, [P(Research), i32]),
    'asmac_social_key': (i32, [P(Social), i32]),
    'asmac_design_key': (i32, [P(Design), i32]),
    'asmac_battle_key': (i32, [P(Battle), i32]),
    'asmac_diplo_key': (i32, [P(Diplo), i32]),
    'asmac_render_base_production': (i32, [P(Production), P(Frame), P(Rect), P(TextItem), i32]),
    'asmac_render_research': (i32, [P(Research), P(Frame), P(Rect), P(TextItem), i32]),
    'asmac_render_social': (i32, [P(Social), P(Frame), P(Rect), P(TextItem), i32]),
    'asmac_render_design': (i32, [P(Design), P(Frame), P(Rect), P(TextItem), i32]),
    'asmac_render_battle': (i32, [P(Battle), P(Frame), P(Rect), P(TextItem), i32]),
    'asmac_render_diplo': (i32, [P(Diplo), P(Frame), P(Rect), P(TextItem), i32]),
    'asmac_world_load': (WORLD, [C.c_char_p, C.c_size_t, C.c_uint32, P(i32)]),
    'asmac_world_error_name': (C.c_char_p, [i32]),
    'asmac_world_free': (None, [WORLD]),
    'asmac_world_info_get': (None, [WORLD, P(WorldInfo)]),
    'asmac_world_vehicle': (i32, [WORLD, i32, P(i32), P(i32), P(i32)]),
    'asmac_world_base_faction': (i32, [WORLD, i32]),
    'asmac_world_render_base_window': (i32, [WORLD, i32, i32, P(Frame), P(Rect), P(TextItem), i32]),
}

_lib = None


def build(output=None, quiet=True):
    """Run build_python.sh (host g++). Returns the library path."""
    out = Path(output) if output else DEFAULT_LIB
    subprocess.run(['sh', str(PYTHON_DIR / 'build_python.sh'), str(out)], check=True,
                   stdout=subprocess.DEVNULL if quiet else None)
    return out


def lib_path():
    env = os.environ.get('ALPHA_SMAC_LIB')
    return Path(env) if env else DEFAULT_LIB


def load(path=None, auto_build=False):
    """Load (once) and return the ctypes library with prototypes set."""
    global _lib
    if _lib is not None:
        return _lib
    p = Path(path) if path else lib_path()
    if not p.exists():
        if not auto_build:
            raise FileNotFoundError(f'{p} not found: run {PYTHON_DIR / "build_python.sh"} first')
        build(p)
    lib = C.CDLL(str(p))
    for name, (res, args) in _PROTOTYPES.items():
        f = getattr(lib, name)
        f.restype = res
        f.argtypes = args
    v = lib.asmac_abi_version()
    if v != ABI_VERSION:
        raise RuntimeError(f'{p}: asmac ABI {v}, this package expects {ABI_VERSION}: rebuild it')
    _lib = lib
    return lib
