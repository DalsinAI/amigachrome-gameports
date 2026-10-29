#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Connected support->fresh-mineral proof against independent original-byte models."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
from test_base_support import world_expected as support_model
from test_world_base_minerals import SOURCES, fixture, model as mineral_model, normalized
from test_world_stable_intake import rules, decode, FACTIONS

HERE = Path(__file__).resolve().parent

def supported_fixture(case):
    data = bytearray(fixture('base', 7 + case % 4, case))
    _, off, _ = decode(data)
    # Convert one source-supported prototype to a former in half the cases;
    # this turns an actual convoy actor into an ordinary supported unit.
    data[off['prototypes'] + 52 + 42] = 9
    if case % 2: data[off['prototypes'] + 42] = 9
    for name, value in [('SE_support_pending', case % 9 - 5), ('SE_police_pending', case % 5 - 4)]:
        field = next(f for f in FACTIONS['LegacyFactionV10']['fields'] if f['name'] == name)
        struct.pack_into('<i', data, off['factions'] + 0x20BC + field['offset'], value)
    return bytes(data)

def check_report(data, text, report):
    info, _, spans = decode(data)
    records = [line.split() for line in report.decode().splitlines()]
    at = 0
    observed = []
    for base in range(int(info['bases'])):
        want = support_model(normalized(data, base), text)
        row = records[at]; at += 1
        assert row[:3] == ['SUPPORT', str(base), str(want[0])], row
        if want[0]:
            assert len(row) == 3
            continue
        fields = dict(x.split('=') for x in row[3:])
        assert [int(fields[k]) for k in ['supported', 'paid', 'cost', 'pacifism', 'drones', 'skips', 'free', 'unit_cost']] == want[1:9]
        for i in range(int(info['vehicles'])):
            v = spans['vehicles'][52 * i:52 * (i + 1)]
            if struct.unpack_from('<h', v, 46)[0] == base and v[14] == spans['bases'][304 * base + 4]:
                row = records[at]; at += 1
                assert row == ['STATE', str(base), str(i), f'{struct.unpack_from("<I", v, 4)[0]:08x}', f'{want[9+i]:08x}']
        minerals = mineral_model('base', text, data, base, want[3])
        row = records[at]; at += 1
        assert row[:3] == ['MINERALS', str(base), str(minerals[1])], (base, row, minerals)
        if not minerals[1]:
            fields = dict(x.split('=') for x in row[3:])
            assert [int(fields[k]) for k in ['intake', 'consumption', 'surplus', 'final', 'eco']] == minerals[-5:]
            assert [int(fields[k]) for k in ['support', 'convoy_to', 'convoy_from']] == [want[3], minerals[3], minerals[5]]
            observed.append({'base': base, 'supported': want[1], 'paid': want[2], 'support_minerals': want[3], 'pacifism': want[4], 'mineral_intake': minerals[-5], 'mineral_consumption': minerals[-4], 'mineral_surplus': minerals[-3]})
    assert at == len(records)
    return observed

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--cc', default='c++')
    ap.add_argument('--binary', type=Path)
    ap.add_argument('--original', type=Path)
    ap.add_argument('--original-rules', type=Path)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.binary.resolve() if args.binary else args.out / 'world-supported-minerals'
    if not args.binary:
        names = [n for n in SOURCES if n != 'world_base_minerals_probe'] + ['base_support', 'world_base_support', 'world_supported_minerals_probe']
        subprocess.run([args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie', *[str(HERE / (n + '.cpp')) for n in names], '-o', str(exe)], check=True)
    cases = [(f'base-{n}', rules('base'), supported_fixture(n)) for n in range(1, 33)]
    if args.original:
        if not args.original_rules: ap.error('--original-rules required with --original')
        cases.append(('original', args.original_rules.read_bytes(), args.original.read_bytes()))
    results = []
    for name, text, data in cases:
        rulefile, save = args.out / 'rules.txt', args.out / 'world.SC'
        rulefile.write_bytes(text); save.write_bytes(data)
        report = subprocess.check_output([str(exe), str(rulefile), str(save)], env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
        values = check_report(data, text, report)
        (args.out / (name + '.log')).write_bytes(report)
        results.append({'case': name, 'report_sha256': hashlib.sha256(report).hexdigest(), 'accepted_mineral_bases': values})
    (args.out / 'receipt.json').write_text(json.dumps(results, indent=2) + '\n')
    print(json.dumps({'cases': len(cases), 'last': results[-1]}))

if __name__ == '__main__': main()
