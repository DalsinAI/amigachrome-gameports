#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generated trade through fresh allocations and maintenance, independent wire oracles."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import test_world_base_budget as budget
import test_world_base_commerce as commerce
from test_units import sections, material

HERE = Path(__file__).resolve().parent
def rules():
    rows = sections(budget.rules())
    rows['TECHNOLOGY'] = sections(commerce.rules())['TECHNOLOGY']
    return material(rows)

def fixture(case, text):
    data = bytearray(budget.fixture(case))
    info, off, _ = budget.accounting.intake.decode(data)
    schema = budget.accounting.intake.FACTIONS['LegacyFactionV10']
    def state(owner, name, value, index=0):
        field = next(f for f in schema['fields'] if f['name'] == name)
        struct.pack_into('<i', data, off['factions'] + owner * 0x20bc + field['offset'] + index * 4, value)
    def pref(name, value, index=0):
        field = next(f for f in budget.accounting.intake.SCHEMA['GamePrefs']['fields'] if f['name'] == name)
        delta = field['offset'] - (0x7c if field['offset'] >= 0x184 else 0)
        struct.pack_into('<i', data, off['legacy'] + delta + index * 4, value)
    assert int(info['bases']) >= 2
    data[off['bases'] + 304 + 4] = 2
    pref('GovernorFaction', case % 3)
    pref('ProposalPassCount', case % 3, 3)
    pref('ProposalPassCount', (case // 3) % 3, 4)
    for owner in range(1, 8):
        fd = off['faction_definitions'] + owner * 0x59c
        struct.pack_into('<i', data, fd + 0x50c, 0)
        field = next(f for f in budget.accounting.intake.FACTIONS['MFaction']['fields'] if f['name'] == 'rule_commerce')
        struct.pack_into('<i', data, fd + field['offset'], case % 3)
        state(owner, 'SE_economy_pending', case % 6)
        state(owner, 'sanction_turns', 0)
        for other in range(1, 8):
            state(owner, 'diplo_status', 0, other)
    state(1, 'diplo_status', 2 + case % 2, 2)
    state(2, 'diplo_status', 2 + case % 2, 1)
    if case == 13:
        state(2, 'sanction_turns', 1)
    if case == 14:
        state(2, 'diplo_status', 0, 1)
    original = bytes(data)
    energy = [10 + case * 3] * int(info['bases'])
    intake = budget.accounting.intake.model('base', text, original)
    assert intake[0] == 0
    energy[0] = intake[9] if intake[9] < 0x80000000 else intake[9] - 0x100000000
    return original, energy

def model(text, data, energy, credits, maintenance, flags, mode):
    info, _, _ = budget.accounting.intake.decode(data)
    if len(energy) != int(info['bases']) or mode == 2:
        return [1]
    if mode == 1:
        return [3]
    fresh = budget.accounting.model('base', text, data, 0, 0, 0)
    if fresh['BASE', 0][0]:
        return [{1: 1, 2: 2, 3: 3, 4: 4, 6: 6, 7: 5}.get(fresh['BASE', 0][0], 1)]
    intake = budget.accounting.intake.model('base', text, data)
    if energy[0] != intake[9]:
        return [1]
    trade = commerce.model(text, data, 0, energy)
    if trade[0]:
        return [{1: 1, 2: 2, 3: 5, 4: 3}[trade[0]]]
    result = budget.model(text, data, 0, credits, maintenance, flags, mode, commerce=trade[3])
    return [0, trade[3], trade[1], trade[2], *result[1:]] if result[0] == 0 else result

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--cc', default='c++')
    ap.add_argument('--binary', type=Path)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.binary.resolve() if args.binary else args.out / 'world-traded-budget-probe'
    command = []
    if not args.binary:
        command = [args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
            *[str(HERE / (n + '.cpp')) for n in budget.NAMES], '-o', str(exe)]
        subprocess.run(command, check=True)
    text = rules()
    rp = args.out / 'base.txt'
    rp.write_bytes(text)
    cases = [(i, 100 if i % 2 else 0, 7, 512 | (32 if i % 3 else 0), 0, 0) for i in range(16)]
    cases += [(7, 100, 7, 512, 0, fault) for fault in range(1, 5)]
    cases += [(7, 100, 7, 512, mode, 0) for mode in [1, 2]]
    cases += [(7, credits, maintenance, 512, 0, 0) for credits, maintenance in [(2147483647, 7), (1, 2147483647), (-1, 7)]]
    records, aggregate = [], hashlib.sha256()
    for index, (case, credits, maintenance, flags, mode, fault) in enumerate(cases):
        data, energies = fixture(case, text)
        if fault == 1:
            energies[0] += 1
        if fault == 2:
            energies[-1] = -1
        if fault == 3:
            energies[-1] = 4194304
        if fault == 4:
            energies.pop()
        path, ep = args.out / f'case-{index}.SC', args.out / f'energy-{index}.bin'
        path.write_bytes(data)
        ep.write_bytes(struct.pack('<' + 'I' * (len(energies) + 1), len(energies), *[v & 0xffffffff for v in energies]))
        want = [v & 0xffffffff for v in model(text, data, energies, credits, maintenance, flags, mode)]
        arguments = [0, credits, maintenance, flags, mode]
        run = subprocess.run([str(exe), str(rp), str(path), *map(str, arguments), str(ep)], capture_output=True,
            env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'), timeout=30)
        assert run.returncode == 0, (index, run.returncode, run.stderr)
        got = list(struct.unpack('<' + 'I' * (len(run.stdout) // 4), run.stdout))
        assert got == want, (index, got, want)
        assert path.read_bytes() == data
        aggregate.update(run.stdout)
        (args.out / f'{index}.expected').write_bytes(run.stdout)
        records.append(dict(name=path.name, energyFile=ep.name, args=arguments, status=got[0],
            commerce=got[1] if got[0] == 0 else None, passed=True, outputSHA256=hashlib.sha256(run.stdout).hexdigest()))
    receipt = dict(cases=len(records), accepted=sum(x['status'] == 0 for x in records),
        positiveTrade=sum(x['commerce'] is not None and x['commerce'] > 0 for x in records),
        command=command, aggregateSHA256=aggregate.hexdigest(), casesDetail=records, native='UNRUN')
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({k: v for k, v in receipt.items() if k not in ['command', 'casesDetail']}))
if __name__ == '__main__':
    main()
