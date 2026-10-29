# SPDX-License-Identifier: GPL-3.0-or-later
"""Shared fixtures: the asmac library (built by build_python.sh when missing
or older than its sources) and the synthetic rules directory of the native
play test. No original game files are used."""
import os
import sys
from pathlib import Path

import pytest

PYTHON_DIR = Path(__file__).resolve().parents[1]
if str(PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(PYTHON_DIR))

from alpha_smac import _lib  # noqa: E402


@pytest.fixture(scope='session')
def asmac_lib():
    path = _lib.lib_path()
    if 'ALPHA_SMAC_LIB' not in os.environ and _lib.stale(path):
        _lib.build(path, quiet=False)
    return _lib.load(path)


@pytest.fixture(scope='session')
def rules(tmp_path_factory, asmac_lib):
    from alpha_smac import synthetic_rules
    return synthetic_rules(tmp_path_factory.mktemp('rules') / 'rules')


@pytest.fixture(scope='session')
def artifacts_dir(tmp_path_factory):
    """ALPHA_SMAC_ARTIFACTS keeps outputs (logs, saves, screenshot) for a report."""
    d = os.environ.get('ALPHA_SMAC_ARTIFACTS')
    if d:
        p = Path(d)
        p.mkdir(parents=True, exist_ok=True)
        return p
    return tmp_path_factory.mktemp('artifacts')
