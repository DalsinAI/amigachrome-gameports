# SPDX-License-Identifier: GPL-3.0-or-later
"""Where the game rules come from.

The original rules and art (alpha.txt / alphax.txt, labels.txt, the faction
.txt files, ...) are the user's own SMAC/SMAX installation and are never
part of this repository: pass the folder with --data PATH (find_rules).

Without them, synthetic_rules() writes the synthetic rules directory the
native play test uses (native/test_play_session.py: test_engine_wp0b.make_dir
with playable #RULES rows and invented unit tables, names like "Noun1"). It
contains no original text and lets the engine start and play a new game;
it is a test world, not the original game's content.
"""
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

from ._lib import NATIVE_DIR


@dataclass
class RulesDir:
    path: Path
    expansion: bool
    synthetic: bool = False

    def describe(self):
        kind = 'synthetic test rules (no original files)' if self.synthetic else ('SMAX alphax.txt' if self.expansion else 'SMAC alpha.txt')
        return f'{self.path} [{kind}]'


def _find(folder, name):
    for cand in (name, name.lower(), name.upper()):
        p = folder / cand
        if p.is_file():
            return p
    return None


def find_rules(path, expansion=None):
    """The user's SMAC/SMAX folder. expansion None: alphax.txt if present."""
    folder = Path(path).expanduser()
    if not folder.is_dir():
        raise FileNotFoundError(f'--data {folder}: not a directory')
    has_x = _find(folder, 'alphax.txt') is not None
    has_base = _find(folder, 'alpha.txt') is not None
    if expansion is None:
        expansion = has_x
    if expansion and not has_x:
        raise FileNotFoundError(f'{folder}: no alphax.txt (Alien Crossfire rules)')
    if not expansion and not has_base:
        raise FileNotFoundError(f'{folder}: no alpha.txt (needs the original game files)')
    if _find(folder, 'labels.txt') is None:
        raise FileNotFoundError(f'{folder}: no labels.txt (needs the original game files)')
    return RulesDir(folder, bool(expansion))


def synthetic_rules(directory=None):
    """Write the native play test's synthetic rules directory and return it."""
    if str(NATIVE_DIR) not in sys.path:
        sys.path.insert(0, str(NATIVE_DIR))
    import test_engine_wp0b
    import test_play_session
    out = Path(directory) if directory else Path(tempfile.mkdtemp(prefix='alpha-smac-rules-')) / 'rules'
    if (out / 'alphax.txt').exists():
        return RulesDir(out, True, True)
    if out.is_dir() and not any(out.iterdir()):
        out.rmdir()
    test_engine_wp0b.make_dir(out, 'expansion', rules=test_play_session.playable_rules('expansion'))
    return RulesDir(out, True, True)
