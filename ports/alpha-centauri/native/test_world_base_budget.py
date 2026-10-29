#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh connected income/maintenance against independent wire and integer models."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import test_world_base_accounting as accounting
import test_world_energy_allocation as allocation
import test_world_facility_upkeep as upkeep
import test_cost_factor as costs
from test_units import sections, material
from test_facility_definitions import tables

HERE = Path(__file__).resolve().parent
NAMES = list(dict.fromkeys([name for name in accounting.SOURCES if name != 'world_base_accounting_probe'] +
    'upkeep_energy_allocation world_energy_allocation facility_upkeep world_facility_upkeep cost_factor world_cost_factor world_base_budget base_commerce world_base_commerce world_traded_budget world_base_budget_probe'.split()))

def rules():
    rows = sections(allocation.rules())
    _, fac = tables('base')
    for i, row in enumerate(fac, 1):
        row[1:5] = [str(i + 2), str(i % 5), 'None', 'Disable']
    rows['FACILITIES'] = fac
    return material(rows)

def fixture(case, version=10):
    data = bytearray(accounting.fixture(case, version))
    _, offsets, _ = accounting.intake.decode(data)
    pref = offsets['legacy'] if version < 11 else offsets['preferences']
    fs = 0x20bc if version < 11 else 0x20cc
    struct.pack_into('<i', data, offsets['faction_definitions'] + 0x59c + 0x50c, 0)
    schema = accounting.intake.FACTIONS['LegacyFactionV10' if version < 11 else 'Faction']
    def state(name, value):
        field = next(f for f in schema['fields'] if f['name'] == name)
        struct.pack_into('<i', data, offsets['factions'] + fs + field['offset'], value)
    state('SE_alloc_labs', case % 11)
    state('SE_alloc_psych', (case // 11) % (11 - case % 11))
    state('SE_industry_pending', case % 11 - 5)
    state('SE_Economics_pending', case % 4)
    struct.pack_into('<i', data, pref + 0x44, case % 7)
    p = offsets['bases']
    for id in [6, 7, 8, 9, 11, 13, 14, 15, 16, 17, 18, 27, 33]:
        if (case * 17 + id) % 3:
            data[p + 0x8c + id // 8] |= 1 << (id % 8)
    for project in range(33 if version < 11 else 64):
        struct.pack_into('<i', data, pref + 0x84 + project * 4, -1)
    for project in [6, 12, 14, 18, 21, 25, 27]:
        if (case + project) % 4 == 0:
            struct.pack_into('<i', data, pref + 0x84 + project * 4, 0)
    return bytes(data)

def model(text, data, base, credits, maintenance, flags, mode, commerce=3):
    if min(credits, maintenance) < 0:
        return [1]
    info, offsets, spans = accounting.intake.decode(data)
    if int(info['version']) >= 11 or mode == 2:
        return [3]
    if mode == 1:
        return [3]
    fresh = accounting.model('base', text, data, base, 2147483647 if mode == 3 else commerce, 0,
                             1 if mode == 1 else None)
    status = fresh['BASE', base][0]
    if status:
        return [{1: 1, 2: 2, 3: 3, 4: 4, 6: 6, 7: 5}.get(status, 1)]
    surplus = fresh['ENERGY', base][6]
    energy = allocation.model(text, data, base, surplus, commerce)
    if energy[0]:
        return [{1: 1, 2: 2, 3: 5}[energy[0]]]
    after_income = credits + energy[2]
    if after_income > 2147483647:
        return [5]
    report = costs.world_report(data, text).decode()
    match = re.search(r'^FACTOR ' + str(base) + r' 1 (\d+) (\d+)$', report, re.MULTILINE)
    assert match
    error, factor = map(int, match.groups())
    if error or factor <= 0:
        return [5 if error == 4 else 3 if error == 3 or factor <= 0 else 1]
    turn = struct.unpack_from('<i', spans['legacy'], 0x44)[0]
    if turn > 1:
        # Independent maintenance oracle consumes the newly computed ECONOMY.
        result = upkeep.model('base', text, data, 0, credits_before=after_income,
            maintenance_before=maintenance, mineral_cost_factor=factor, player_flags_before=flags)
        if result[0]:
            return [{1: 1, 2: 2, 3: 5}[result[0]]]
    else:
        built = int.from_bytes(spans['bases'][base * 304 + 0x8c:base * 304 + 0x94], 'little')
        result = [0, after_income, maintenance, flags, *[(built >> (32 * i)) & 0xffffffff for i in range(3)], *([0] * 76)]
    arrays = [byte for word in result[4:13] for byte in struct.pack('<I', word)]
    return [0, surplus, *energy[2:5], after_income, factor, int(turn > 1), *result[1:4], *arrays, *result[13:],
            *fresh['STATES', base][1:]]

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--cc', default='c++')
    ap.add_argument('--binary', type=Path)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.binary.resolve() if args.binary else args.out / 'world-base-budget-probe'
    command = []
    if not args.binary:
        command = [args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
            *[str(HERE / (n + '.cpp')) for n in NAMES], '-o', str(exe)]
        subprocess.run(command, check=True)
    text = rules()
    rp = args.out / 'base.txt'
    rp.write_bytes(text)
    cases = [(i, 10, [0, 1, 2, 100][i % 4], 7, 512 | (32 if i % 3 else 0), 0) for i in range(44)]
    cases += [(7, v, 100, 0, 0, 0) for v in [7, 8, 9, 11, 12]]
    cases += [(7, 10, 100, 7, 512, mode) for mode in [1, 2, 3]]
    cases += [(7, 10, c, m, 512, 0) for c, m in [(2147483647, 7), (1, 2147483647), (-1, 7), (100, -1)]]
    records = []
    aggregate = hashlib.sha256()
    for index, (case, version, credits, maintenance, flags, mode) in enumerate(cases):
        data = fixture(case, version)
        path = args.out / f'case-{index}.SC'
        path.write_bytes(data)
        expected = [v & 0xffffffff for v in model(text, data, 0, credits, maintenance, flags, mode)]
        run = subprocess.run([str(exe), str(rp), str(path), '0', str(credits), str(maintenance), str(flags), str(mode)],
            capture_output=True, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'), timeout=30)
        assert run.returncode == 0, (index, run.returncode, run.stderr)
        actual = list(struct.unpack('<' + 'I' * (len(run.stdout) // 4), run.stdout))
        assert actual == expected, (index, actual, expected)
        assert path.read_bytes() == data
        aggregate.update(run.stdout)
        records.append(dict(name=path.name, args=[0, credits, maintenance, flags, mode],
            status=actual[0], passed=True, outputSHA256=hashlib.sha256(run.stdout).hexdigest()))
    receipt = dict(cases=len(records), accepted=sum(x['status'] == 0 for x in records),
        command=command, aggregateSHA256=aggregate.hexdigest(), casesDetail=records,
        native='UNRUN', scope='Atomic fresh accounting/allocation/maintenance projection; no turn mutation')
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({k: v for k, v in receipt.items() if k not in ['command', 'casesDetail']}))
if __name__ == '__main__':
    main()
