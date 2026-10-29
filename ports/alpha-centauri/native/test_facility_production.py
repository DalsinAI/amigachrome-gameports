#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent material oracle, byte-exact completion persistence and world gates."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import struct
import subprocess
from test_original_game import decode
from test_local_actions import local_world, rule_data, SOURCES as ACTION_SOURCES
from test_world_actions import fixture as action_fixture
from test_units import sections, material
from test_local_actions_snapshot import moved_model

HERE = Path(__file__).resolve().parent
SOURCES = ACTION_SOURCES[:-1] + ['facility_production.cpp', 'facility_production_stage.cpp',
                                  'facility_production_snapshot.cpp', 'facility_production_probe.cpp'] + [name + '.cpp' for name in 'tile_yields world_technology_effects world_upkeep_tile_yields world_tile_yields technology_effects world_fresh_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients upkeep_base_minerals world_base_minerals base_support world_base_support cost_factor world_cost_factor'.split()]
SENTINEL = 0x5a5a5a5a


def oracle(row):
    acc, baseline, intake, surplus, units, factor, exempt, best, flags, human, starved, riots = row
    if min(acc, baseline, intake, units, best) < 0 or factor < 1 or not 0 <= exempt <= 1000:
        return [1] + [SENTINEL] * 7
    def fit(n):
        if not -(1 << 31) <= n < (1 << 31):
            raise OverflowError
        return n
    try:
        best = max(best, fit(intake + fit(2 * surplus)))
        flags &= ~0x800000
        cost = credited = completed = 0
        if not starved:
            if surplus >= 0 and not riots:
                credited = surplus
                acc = fit(acc + surplus)
            cost = fit(units * factor)
            if acc >= cost:
                completed = 1
                acc -= cost
                if human:
                    acc = min(acc, max(0, min(exempt, surplus)))
                baseline = acc
                flags = (flags | 0x800000) & ~0x80000000
        return [0, acc, baseline, cost, credited, best, flags, completed]
    except OverflowError:
        return [3] + [SENTINEL] * 7


def leaf_cases():
    rows = []
    for acc, surplus, human, starved, riots in itertools.product([0, 39, 40, 41, 100], [-3, 0, 1, 10, 20], [0, 1], [0, 1], [0, 1]):
        rows.append([acc, 11, 5, surplus, 4, 10, 10, 5, 0x80800000, human, starved, riots])
    rng = random.Random(0xFAC2026)
    edges = [-(1 << 31), -1, 0, 1, 10, 1000, (1 << 31) - 1]
    for _ in range(6000):
        row = [rng.choice(edges) if rng.randrange(5) == 0 else rng.randrange(200) for _ in range(9)]
        row[6] = rng.choice(edges) if rng.randrange(5) == 0 else rng.randrange(1001)
        row[8] = rng.getrandbits(32)
        rows.append(row + [rng.randrange(2) for _ in range(3)])
    return rows


def fixture(version=9, **changes):
    data = bytearray(local_world(action_fixture(version, 1, 1)))
    info, off, _ = decode(data)
    bp = off['bases']
    pref = off['preferences'] if version >= 11 else off['legacy']
    data[bp + 4] = changes.get('owner', 5)
    values = {'state': 0x80800000, 'governor': 0, 'nutrients': 20, 'acc': 32, 'queue_size': 0, 'queue': -3, 'baseline': 11}
    values.update(changes)
    for name, offset in [('state', 0x30), ('governor', 0x38), ('nutrients', 0x3c), ('acc', 0x40), ('queue_size', 0x4c), ('queue', 0x50), ('baseline', 0x9c if version >= 11 else 0x98)]:
        struct.pack_into('<I', data, bp + offset, values[name] & 0xffffffff)
    data[bp + 0x8c:bp + 0x8c + (12 if version >= 11 else 8)] = bytes(12 if version >= 11 else 8)
    data[bp + 0x8c] = changes.get('built', 0)
    if changes.get('inactive'):
        struct.pack_into('<i', data, pref + 0x390 - (0 if version >= 11 else 0x7c), 1)
    struct.pack_into('<i', data, off['factions'] + 5 * (0x20cc if version >= 11 else 0x20bc) + 0x3f8, changes.get('best', 5))
    return bytes(data)


def rules():
    table = sections(rule_data())
    table['FACILITIES'][1][1] = '5'
    table['FACILITIES'][2][1] = '4'
    return material(table)


def expected_world(data, fresh, error, move=False):
    if move:
        data = moved_model(data)
    if error:
        return data
    b = bytearray(data)
    info, off, _ = decode(b)
    p = off['bases']
    v = int(info['version'])
    baseline_offset = 0x9c if v >= 11 else 0x98
    acc, baseline = struct.unpack_from('<i', b, p + 0x40)[0], struct.unpack_from('<i', b, p + baseline_offset)[0]
    flags = struct.unpack_from('<I', b, p + 0x30)[0]
    facility = -struct.unpack_from('<i', b, p + 0x50)[0]
    owner = b[p + 4]
    fp = off['factions'] + owner * (0x20cc if v >= 11 else 0x20bc)
    best = struct.unpack_from('<i', b, fp + 0x3f8)[0]
    out = oracle([acc, baseline, fresh[0], fresh[1], {2: 5, 3: 4}[facility], fresh[2], 10, best, flags, 1,
                  int(struct.unpack_from('<i', b, p + 0x3c)[0] < 0), int(bool(flags & 2))])
    assert out[0] == 0
    _, acc, baseline, cost, credited, best, flags, complete = out
    struct.pack_into('<i', b, p + 0x40, acc)
    struct.pack_into('<i', b, p + baseline_offset, baseline)
    struct.pack_into('<I', b, p + 0x30, flags)
    struct.pack_into('<i', b, fp + 0x3f8, best)
    if complete:
        b[p + 0x8c + facility // 8] |= 1 << (facility % 8)
    return bytes(b)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True, type=Path)
    ap.add_argument('--cc', default='g++')
    ap.add_argument('--probe', type=Path)
    ap.add_argument('--original', type=Path)
    ap.add_argument('--original-rules', type=Path)
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=False)
    binary = a.probe or a.out / 'facility-production-probe'
    command = [a.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
               '-fno-sanitize-recover=all', '-no-pie', *[str(HERE / p) for p in SOURCES], '-o', str(binary)]
    if not a.probe:
        subprocess.run(command, check=True)
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
    rows = leaf_cases()
    corpus = a.out / 'leaf-input.bin'
    corpus.write_bytes(b''.join(struct.pack('<12I', *[n & 0xffffffff for n in row]) for row in rows))
    want = b''.join(struct.pack('<8I', *oracle(row)) for row in rows)
    dest = a.out / 'leaf-output.bin'
    r = subprocess.run([str(binary), '--leaf', str(corpus), str(dest)], env=env, capture_output=True)
    assert r.returncode == 0, (r.returncode, r.stderr.decode())
    assert dest.read_bytes() == want, 'Independent arithmetic oracle mismatch'
    text = rules()
    records = []
    specs = [('complete', {}, [20, 8, 10], 0), ('carryover', {'acc': 100}, [30, 20, 10], 0),
             ('waiting', {'acc': 5}, [20, 8, 10], 0), ('negative-surplus', {'acc': 50}, [5, -1, 10], 0),
             ('riots-funded', {'state': 0x80800002, 'acc': 45}, [20, 8, 10], 0),
             ('riots-waiting', {'state': 2}, [20, 8, 10], 0), ('starved', {'nutrients': -1}, [20, 8, 10], 0),
             ('overflow', {'acc': (1 << 31) - 1}, [20, 8, 10], 3),
             ('wrong-owner', {'owner': 1}, [20, 8, 10], 5), ('inactive', {'inactive': True}, [20, 8, 10], 4),
             ('governor', {'governor': 0x80000000}, [20, 8, 10], 6),
             ('queue', {'queue_size': 1}, [20, 8, 10], 6), ('unit', {'queue': 320}, [20, 8, 10], 6),
             ('other-facility', {'queue': -4}, [20, 8, 10], 6), ('built', {'built': 8}, [20, 8, 10], 7),
             ('bad-factor', {}, [20, 8, 0], 1), ('bad-baseline', {'baseline': -1}, [20, 8, 10], 1),
             ('inconsistent-fresh-minerals', {}, [5, 8, 10], 1),
             ('creche', {'queue': -2, 'acc': 42}, [20, 8, 10], 0)]
    for version in (7, 8, 9, 10):
        for name, changes, fresh, error in specs:
            data = fixture(version, **changes)
            rulepath, source = a.out / 'rules.txt', a.out / f'v{version}-{name}.SC'
            rulepath.write_bytes(text)
            source.write_bytes(data)
            output = a.out / f'v{version}-{name}.out'
            r = subprocess.run([str(binary), str(rulepath), str(source), '0', *map(str, fresh), '0', str(output)], env=env, capture_output=True, text=True)
            assert r.returncode == 0, (name, version, r.returncode, r.stdout, r.stderr)
            assert f'STAGE status={ {0:"ok",1:"bad_state",3:"overflow",4:"not_human",5:"not_owner",6:"unsupported",7:"unavailable"}[error]} ' in r.stdout, (name, r.stdout)
            assert output.read_bytes() == expected_world(data, fresh, error), (name, version, 'byte oracle')
            assert source.read_bytes() == data
            records.append(dict(version=version, name=name, passed=True, log=r.stdout, sha256=hashlib.sha256(output.read_bytes()).hexdigest()))
    data = fixture()
    rulepath, source = a.out / 'rules.txt', a.out / 'live-movement.SC'
    source.write_bytes(data)
    output = a.out / 'live-movement.out'
    r = subprocess.run([str(binary), str(rulepath), str(source), '0', '20', '8', '10', '1', str(output)], env=env, capture_output=True, text=True)
    assert r.returncode == 0, (r.returncode, r.stdout, r.stderr)
    assert output.read_bytes() == expected_world(data, [20, 8, 10], 0, move=True), 'Live movement reverted by completion persistence'
    records.append(dict(name='live-movement-and-completion', passed=True, log=r.stdout))
    for mode in range(2, 9):
        output = a.out / f'reject-{mode}.out'
        r = subprocess.run([str(binary), str(rulepath), str(source), '0', '20', '8', '10', str(mode), str(output)], env=env, capture_output=True, text=True)
        assert r.returncode == 0 and not output.exists() and 'unchanged=1' in r.stdout, (mode, r.returncode, r.stdout, r.stderr)
        records.append(dict(name=f'encoder-reject-{mode}', passed=True, log=r.stdout))
    if a.original:
        assert a.original_rules
        original = a.original.read_bytes()
        info, off, _ = decode(original)
        human = int(info['player'])
        stride = 308 if int(info['version']) >= 11 else 304
        for id in range(int(info['bases'])):
            if original[off['bases'] + id * stride + 4] != human:
                continue
            output = a.out / f'original-order-creche-{id}.out'
            r = subprocess.run([str(binary), str(a.original_rules), str(a.original), str(id), '100', '100', '10', '102', str(output)], env=env, capture_output=True, text=True)
            assert r.returncode in (0, 22), (id, r.returncode, r.stdout, r.stderr)
            records.append(dict(name=f'original-owned-base-{id}', passed=True, executed=r.returncode == 0,
                                explicitFreshTestFacts=[100, 100, 10], log=r.stdout,
                                sha256=hashlib.sha256(output.read_bytes()).hexdigest() if output.exists() else None))
        assert a.original.read_bytes() == original
    result = dict(policy='Pinned Thinker restoration subset; original parity unproven; no turn scheduler',
                  leafCases=len(rows), leafSHA256=hashlib.sha256(want).hexdigest(), worldCases=len(records), records=records,
                  binarySHA256=hashlib.sha256(binary.read_bytes()).hexdigest(), nativeExecution='UNRUN', command=command if not a.probe else None)
    (a.out / 'receipt.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: v for k, v in result.items() if k not in ('records', 'command')}))


if __name__ == '__main__':
    main()
