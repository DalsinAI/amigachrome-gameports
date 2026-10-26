#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original cost-factor port against exact source body and independent integer/wire oracle."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import struct
import subprocess
from test_world_stable_intake import rules, fixture, decode, FACTIONS, SCHEMA
from test_units import sections, material

HERE = Path(__file__).resolve().parent
SENTINEL = 0x5A5A5A5A

def expected(row):
    resource, multiplier, size, difficulty, industry, growth, flags = row
    human, threat, leader, network, base, creche, golden = [bool(flags & (1 << i)) for i in range(7)]
    if not 1 <= multiplier <= 100 or (not human and difficulty > 5):
        return [1, SENTINEL]
    factor = 10 if human else [13, 12, 11, 10, 8, 7][difficulty] - threat - (leader and not network)
    factor = factor * multiplier // 10
    factor = factor * {0: 8, 1: 9}.get(size, 10) // 10
    if resource == 1:
        rates = {**dict.fromkeys(range(-7, -2), (13, 10)), -2: (6, 5), -1: (11, 10), 0: (1, 1), 1: (9, 10), 2: (4, 5), 3: (7, 10), 4: (3, 5)}
        numerator, denominator = rates.get(industry, (1, 2))
        factor = (factor * numerator + denominator - 1) // denominator
    elif resource == 0:
        growth += (2 * creche + 2 * golden) if base else 0
        if not -(1 << 31) <= growth < (1 << 31):
            return [4, SENTINEL]
        growth = min(5, max(-2, growth))
        factor = (factor * (10 - growth) + 9) // 10
    return [0, factor]

def cases():
    result = []
    industries = [-(1 << 31), *range(-10, 9), (1 << 31) - 1]
    growths = [-(1 << 31), *range(-8, 9), (1 << 31) - 1]
    multipliers = [1, 5, 9, 10, 11, 100]
    sizes = [0, 1, 2, 4, 0xFFFFFFFF]
    for industry, multiplier, size in itertools.product(industries, multipliers, sizes):
        result.append([1, multiplier, size, 0xFFFFFFFF, industry, 0, 1])
    for growth, multiplier, size, flags in itertools.product(growths, multipliers, sizes, range(8)):
        result.append([0, multiplier, size, 0, 0, growth, 1 | (flags << 4)])
    for difficulty, flags, multiplier, size, industry in itertools.product(range(6), range(8), multipliers, sizes, industries):
        result.append([1, multiplier, size, difficulty, industry, 0, flags << 1])
    for resource, flags, multiplier, size in itertools.product([2, 3, 17, 0xFFFFFFFF], range(16), multipliers, sizes):
        result.append([resource, multiplier, size, 4, -8, 2147483647, flags | 112])
    rng = random.Random(0x4E4430)
    for _ in range(3000):
        result.append([rng.choice([0, 1, 2, 3]), rng.choice([0, 1, 10, 100, 101, 0xFFFFFFFF]), rng.choice(sizes), rng.choice([0, 5, 6, 0xFFFFFFFF]), rng.choice(industries), rng.choice(growths), rng.randrange(128)])
    return result

def packet(rows):
    return struct.pack('<I', len(rows)) + b''.join(struct.pack('<IIIIiiI', *row) for row in rows)

def basic_row(name):
    index = -1
    for line in (HERE / 'basic_fields.inc').read_text().splitlines():
        if line.startswith('RULE_ROW('): index += 1
        if line.startswith('RULE_ROW(' + name + ','): return index
    raise KeyError(name)

def world_report(data, text):
    info, _, spans = decode(data)
    legacy = int(info['version']) < 11
    bs, fs = (304, 0x20BC) if legacy else (308, 0x20CC)
    schema = FACTIONS['LegacyFactionV10' if legacy else 'Faction']
    prefs = spans['legacy' if legacy else 'preferences']
    status = next(f['offset'] for f in SCHEMA['GamePrefs']['fields'] if f['name'] == 'FactionStatus')
    status -= 0x7C if legacy and status >= 0x184 else 0
    human_flags = prefs[status]
    size = struct.unpack_from('<I', spans['metadata'], 0xA30)[0]
    table = sections(text)['RULES']
    multipliers = [int(table[basic_row(n)][0]) for n in ['nutrient_cost_multi', 'mineral_cost_multi']]
    multipliers = [min(100, max(1, n)) for n in multipliers]
    def evaluate(faction, resource, base=None):
        if not 1 <= faction <= 7: return [1, SENTINEL]
        if not human_flags & (1 << faction): return [3, SENTINEL]
        def state(name):
            offset = next(f['offset'] for f in schema['fields'] if f['name'] == name)
            return struct.unpack_from('<i', spans['factions'], fs * faction + offset)[0]
        flags = 1
        if base is not None:
            flags |= 16 | (32 if base[0x8C] & 4 else 0) | (64 if struct.unpack_from('<I', base, 0x30)[0] & 4 else 0)
        return expected([resource, multipliers[bool(resource)], size, 0, state('SE_industry_pending'), state('SE_growth_pending'), flags])
    lines = []
    for base_id in range(int(info['bases'])):
        b = spans['bases'][bs * base_id:bs * (base_id + 1)]
        faction = struct.unpack_from('<b', b, 4)[0]
        for resource in range(4):
            result = evaluate(faction, resource, b)
            lines.append(f'FACTOR {base_id} {resource} {result[0]} {result[1]}\n')
    for faction in range(1, 8):
        for resource in range(2):
            result = evaluate(faction, resource)
            lines.append(f'FACTION {faction} {resource} {result[0]} {result[1]}\n')
    return ''.join(lines).encode()

def world_fixture(case):
    version = 7 + case % 6
    data = bytearray(fixture('base' if version < 11 else 'expansion', version, case))
    _, off, _ = decode(data)
    legacy = version < 11
    schema, fs = (FACTIONS['LegacyFactionV10'], 0x20BC) if legacy else (FACTIONS['Faction'], 0x20CC)
    for name, value in [('SE_industry_pending', case % 21 - 10), ('SE_growth_pending', case % 12 - 4)]:
        offset = next(f['offset'] for f in schema['fields'] if f['name'] == name)
        struct.pack_into('<i', data, off['factions'] + fs + offset, value)
    struct.pack_into('<I', data, off['metadata'] + 0xA30, case % 5)
    data[off['bases'] + 0x8C] |= 4 if case & 1 else 0
    # Original state_flags is at0x30, not the governor flags at0x38.
    state = struct.unpack_from('<I', data, off['bases'] + 0x30)[0]
    struct.pack_into('<I', data, off['bases'] + 0x30, (state & ~4) | (4 if case & 2 else 0))
    return bytes(data)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--cc', default='c++')
    ap.add_argument('--binary', type=Path)
    ap.add_argument('--original', type=Path)
    ap.add_argument('--original-rules', type=Path)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.binary.resolve() if args.binary else args.out / 'cost-factor'
    if not args.binary:
        names = ['rules_text', 'original_map', 'original_game', 'original_faction', 'original_world', 'cost_factor', 'world_cost_factor', 'cost_factor_probe']
        subprocess.run([args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie', *[str(HERE / (n + '.cpp')) for n in names], '-o', str(exe)], check=True)
    env = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'}
    rows = cases()
    source, output = args.out / 'cost-cases.bin', args.out / 'cost-results.bin'
    source.write_bytes(packet(rows))
    subprocess.run([str(exe), 'leaf', str(source), str(output)], check=True, env=env)
    want = b''.join(struct.pack('<II', *expected(row)) for row in rows)
    assert output.read_bytes() == want, 'leaf mismatch'
    records = []
    for case in range(1, 49):
        text = rules('base')
        t = sections(text)
        for name, value in [('nutrient_cost_multi', [1, 10, 13][case % 3]), ('mineral_cost_multi', [5, 10, 17][case % 3])]: t['RULES'][basic_row(name)][0] = str(value)
        records.append((f'synthetic-{case}', material(t), world_fixture(case)))
    if args.original:
        if not args.original_rules: ap.error('--original-rules required with --original')
        records.append(('original', args.original_rules.read_bytes(), args.original.read_bytes()))
    world_receipts = []
    for name, text, data in records:
        rulefile, save, report = args.out / 'rules.txt', args.out / 'world.SC', args.out / (name + '.log')
        rulefile.write_bytes(text); save.write_bytes(data)
        subprocess.run([str(exe), 'world', str(rulefile), str(save), str(report)], check=True, env=env)
        expected_report = world_report(data, text)
        assert report.read_bytes() == expected_report, f'world mismatch {name}'
        world_receipts.append({'case': name, 'sha256': hashlib.sha256(expected_report).hexdigest()})
    receipt = {'leaf_cases': len(rows), 'world_cases': len(records), 'leaf_sha256': hashlib.sha256(want).hexdigest(), 'source_body_comparison': 'exact pinned OpenSMACX body, every successful leaf case', 'worlds': world_receipts}
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({k: v for k, v in receipt.items() if k != 'worlds'}))

if __name__ == '__main__': main()
