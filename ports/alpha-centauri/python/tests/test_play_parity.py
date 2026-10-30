# SPDX-License-Identifier: GPL-3.0-or-later
"""The 20-turn scripted play test of native/test_play_session.py, played once
by the native host binary (play_control.cpp, built exactly as
test_play_session.py builds its plain host reference) and once through the
Python API (alpha_smac.playtest, a line-for-line mirror of play_game()).
The printed output and both saves must be byte-identical.

ALPHA_SMAC_NATIVE_RUN=DIR reuses a finished native run (DIR/stdout.txt,
play.SAV, final.SAV), e.g. run0/ of test_play_session.py --out.
"""
import hashlib
import json
import os
import subprocess
import sys

import pytest

from alpha_smac._lib import NATIVE_DIR, PYTHON_DIR

TURNS = 20
SEED = 777


def sha(b):
    return hashlib.sha256(b).hexdigest()


@pytest.fixture(scope='module')
def native_run(tmp_path_factory, rules):
    reuse = os.environ.get('ALPHA_SMAC_NATIVE_RUN')
    if reuse:
        from pathlib import Path
        d = Path(reuse)
        return d, None
    sys.path.insert(0, str(NATIVE_DIR))
    import test_play_session
    work = tmp_path_factory.mktemp('native')
    exe = work / 'play-control-host'
    cmd = test_play_session.build('c++', [], exe)
    out = work / 'run'
    out.mkdir()
    r = subprocess.run([str(exe), 'play', str(rules.path), '1', str(SEED), str(out), str(TURNS)], capture_output=True, timeout=3600)
    assert r.returncode == 0, r.stdout[-3000:]
    (out / 'stdout.txt').write_bytes(r.stdout)
    return out, cmd


def test_python_play_matches_native(native_run, rules, artifacts_dir):
    nat_dir, build_cmd = native_run
    py_dir = artifacts_dir / 'python-play'
    py_dir.mkdir(exist_ok=True)
    env = dict(os.environ, PYTHONPATH=str(PYTHON_DIR))
    r = subprocess.run([sys.executable, '-m', 'alpha_smac.playtest', str(rules.path), '1', str(SEED), str(py_dir), str(TURNS)],
                       capture_output=True, timeout=3600, env=env, cwd=str(PYTHON_DIR))
    assert r.returncode == 0, (r.stdout[-3000:], r.stderr[-3000:])
    nat_text = (nat_dir / 'stdout.txt').read_bytes()
    py_text = (py_dir / 'stdout.txt').read_bytes()
    assert r.stdout == py_text
    nat_lines, py_lines = nat_text.decode('cp1252').split('\n'), py_text.decode('cp1252').split('\n')
    first_diff = next((i for i, (a, b) in enumerate(zip(nat_lines, py_lines)) if a != b), None)
    assert first_diff is None, (first_diff, nat_lines[first_diff], py_lines[first_diff])
    assert py_text == nat_text
    files = {}
    for name in ('play.SAV', 'final.SAV'):
        a, b = (nat_dir / name).read_bytes(), (py_dir / name).read_bytes()
        assert a == b, name
        files[name] = sha(a)
    turns = [l for l in py_lines if l.startswith('A TURN ')]
    assert len(turns) >= TURNS and all(' status=0 faction=1 ' in l for l in turns[:TURNS])
    assert not [l for l in py_lines if l.startswith('FAIL')]
    final = [l for l in py_lines if l.startswith('FINAL ')][0]
    receipt = dict(turns=TURNS, seed=SEED, native_build=build_cmd and ' '.join(map(str, build_cmd))[:400], native_dir=str(nat_dir),
                   lines=len(py_lines), output_sha256=sha(py_text), native_output_sha256=sha(nat_text), play_sav_sha256=files['play.SAV'],
                   final_sav_sha256=files['final.SAV'], final=final, last_turn=turns[-1])
    (artifacts_dir / 'parity-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))
