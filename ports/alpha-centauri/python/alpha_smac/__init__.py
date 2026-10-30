# SPDX-License-Identifier: GPL-3.0-or-later
"""alpha_smac: play the Alpha Centauri port from Python.

The game logic is the native C++ engine (ports/alpha-centauri/native/engine)
and play layer (play_session.cpp), reached through the small C API in
../asmac.h (libasmac.so, built by ../build_python.sh) with ctypes.

    import alpha_smac as ac
    rules = ac.find_rules('/path/to/SMAC')          # your own game folder
    g = ac.new_game(rules, seed=7)
    print(g.factions(), g.bases(), g.units(own=True))
    g.move(g.next_unit(), ac.Dir.N); g.end_turn(); g.save('game.SAV')
"""
from .game import (Cmd, Dir, Event, FactionInfo, BaseInfo, Game, GameError, Item, Key, MsgKind, Status, Terraform, TileBits,
                   TurnResult, UnitInfo, World, command_name, diplomacy_available, fnv1a, new_game_available, panel_key,
                   parse_command, render_panel, status_name)
from .rules import RulesDir, find_rules, synthetic_rules

__all__ = ['Game', 'GameError', 'Status', 'Cmd', 'Dir', 'Key', 'Item', 'Terraform', 'MsgKind', 'TileBits', 'World', 'Event',
           'FactionInfo', 'BaseInfo', 'UnitInfo', 'TurnResult', 'RulesDir', 'find_rules', 'synthetic_rules', 'new_game', 'load',
           'command_name', 'status_name', 'parse_command', 'render_panel', 'panel_key', 'fnv1a', 'new_game_available',
           'diplomacy_available']


def new_game(rules, seed=1, faction=1, difficulty=2, map_size=2):
    """Start a new game. rules: a RulesDir (find_rules / synthetic_rules)."""
    return Game.new_game(rules.path, rules.expansion, seed, faction, difficulty, map_size)


def load(rules, path):
    """Load an original SAV (or one saved by this port)."""
    return Game.load(rules.path, path, rules.expansion)
