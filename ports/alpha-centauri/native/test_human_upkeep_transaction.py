#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Private population mutation, rollback and deterministic phase checkpoint oracle."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import zlib
import test_world_population as population
import test_world_worker_upkeep as worker

HERE = Path(__file__).resolve().parent
SOURCES = list(dict.fromkeys([s for s in population.SOURCES + worker.SOURCES if not s.endswith('_probe')] +
                            ['facility_production_snapshot', 'human_upkeep_snapshot', 'human_upkeep_transaction', 'human_upkeep_transaction_probe']))


def rules(food=5):
    rows = population.intake.sections(population.intake.rules('base'))
    for r in rows['RESOURCEINFO']:
        r[1] = str(food)
    for name, value in [('nutrient_req_citizen', 4 if food == 0 else 2), ('nutrient_cost_multi', 10), ('pop_limit_sans_hab_complex', 7), ('pop_limit_sans_hab_dome', 14)]:
        row, col, _ = population.intake.ROW[name]
        rows['RULES'][row][col] = str(value)
    return population.intake.material(rows)


def fixture(pop=2, food=0, growth=0, immune=False, invalid_psych=False, version=9, forest=False, starving=False):
    data = bytearray(worker.accounting.fixture(7, version))
    info, off, _ = population.intake.decode(data)
    struct.pack_into('<i', data, off['player'], 1)
    population.pref(data, 'CurrentFaction', 1)
    population.pref(data, 'FactionStatus', 0xfe)
    population.pref(data, 'GameRules', 0)
    population.pref(data, 'GameState', 0x10)
    bp = off['bases']
    struct.pack_into('<b', data, bp + 6, pop)
    struct.pack_into('<B', data, bp + 8, int(immune))
    struct.pack_into('<i', data, bp + 0x3c, food)
    struct.pack_into('<I', data, bp + 0x78, (1 << (pop + 1)) - 1)
    struct.pack_into('<i', data, bp + 0x7c, 0)
    struct.pack_into('<I', data, bp + 0x34, 0)
    schema = population.intake.FACTIONS['LegacyFactionV10']
    for name, value in [('SE_growth_pending', growth), ('SE_effic_pending', 0), ('SE_support_pending', 0), ('SE_police_pending', 0),
                        ('SE_industry_pending', 0), ('SE_alloc_psych', 99 if invalid_psych else 2)]:
        population.put(data, 'factions', schema, name, value)
    for project in (13, 24, 27, 30):
        population.pref(data, 'SecretProjects', -1, project)
    if forest:
        for tile in range(int(info['width']) * int(info['height']) // 2):
            at = off['tiles'] + tile * 44 + 8
            items = struct.unpack_from('<I', data, at)[0]
            if not items & 1:
                struct.pack_into('<I', data, at, items | 0x200000)
    if starving:
        population.put(data, 'factions', schema, 'satellites_nutrient', 0)
        for i in range(int(info['vehicles'])):
            data[off['vehicles'] + i * 52 + 17] = 0
    return bytes(data)


def envelope(original, after, text, phase, base=0, full=True):
    out = bytearray(64) + original + after
    out[:8] = b'ACHUPK01'
    struct.pack_into('<6I', out, 8, 64, len(original), 0xabcdef01, zlib.crc32(text), zlib.crc32(original), zlib.crc32(after))
    struct.pack_into('<3I', out, 36, base if phase else 0xffffffff, int(full) if phase else 0, phase)
    struct.pack_into('<I', out, 32, zlib.crc32(out))
    return bytes(out)


def model(original, text, full):
    proposed = population.model(original, text, full)
    error_map = {1: 4, 2: 2, 3: 6, 4: 6, 5: 4, 6: 5, 7: 7}
    failure = lambda e: (error_map.get(e, 4), envelope(original, original, text, 0), None)
    if proposed[0]:
        return failure(proposed[0])
    if proposed[16]:
        return 0, envelope(original, original, text, 1, full=full), dict(phase=1, food_checked=False, specialists_pending=False, riots_pending=False)
    after = bytearray(original)
    info, off, _ = population.intake.decode(original)
    bp = off['bases']
    struct.pack_into('<b', after, bp + 6, proposed[12])
    struct.pack_into('<i', after, bp + 0x3c, proposed[13])
    cancel = proposed[-64:]
    for i in range(int(info['vehicles'])):
        if cancel[i // 32] & (1 << (i % 32)):
            after[off['vehicles'] + i * 52 + 17] = 0
    selected = worker.model(text, bytes(after))
    if selected['BASE'][0]:
        return failure(selected['BASE'][0])
    assignment = selected['ALLOC']
    struct.pack_into('<I', after, bp + 0x78, assignment[0])
    struct.pack_into('<i', after, bp + 0x7c, assignment[1])
    specialist = bool(assignment[8] or assignment[1] or assignment[2])
    riots = bool(assignment[9])
    checked = not assignment[1] and not assignment[2]
    if checked and proposed[18]:
        finished = population.model(original, text, full, after=bytes(after))
        assert finished[0] == 0, finished
        struct.pack_into('<i', after, bp + 0x3c, finished[13])
    phase = 2 if specialist else 3 if riots else 4
    return 0, envelope(original, bytes(after), text, phase, full=full), dict(phase=phase, food_checked=checked, specialists_pending=specialist, riots_pending=riots)


def encoder_model(data, mode):
    if mode != 10:
        return bytes([0x5a]) * len(data)
    data = bytearray(data)
    info, off, spans = population.intake.decode(data)
    legacy = int(info['version']) < 11
    owner = data[off['bases'] + 4]
    def patch(schema, start, name, value, index=0, base=False):
        field = next(f for f in schema['fields'] if f['name'] == name)
        at = field['offset'] - (4 if base and legacy and field['offset'] >= 0x98 else 0)
        fmt = population.intake.FORMATS[field['type']]
        struct.pack_into('<' + fmt, data, start + at + index * struct.calcsize(fmt), value)
    schema = population.intake.SCHEMA['BASE']
    for name, value in [('pop_size', 3), ('nutrients_accumulated', -17), ('worked_tiles', 7), ('specialist_total', 1),
                        ('economy_total', 19), ('psych_total', 23), ('labs_total', 29)]:
        patch(schema, off['bases'], name, value, base=True)
    for index, value in enumerate((0x01234560, 0x06543210)):
        patch(schema, off['bases'], 'specialist_types', value, index, base=True)
    faction = population.intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction']
    for name, value in [('energy_credits', -31), ('labs_total', 37)]:
        patch(faction, off['factions'] + owner * (0x20bc if legacy else 0x20cc), name, value)
    return bytes(data)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--cc', default='g++')
    p.add_argument('--binary', type=Path)
    args = p.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.binary or args.out / 'human-upkeep-probe'
    command = [args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
               *[str(HERE / (s + '.cpp')) for s in SOURCES], '-o', str(exe)]
    if not args.binary:
        subprocess.run(command, check=True)
    cases = [('stable-checked', fixture(immune=True), True, 5), ('stable-riots-pending', fixture(), True, 5),
             ('growth-specialists-pending', fixture(food=100, immune=True, forest=True), True, 5),
             ('boom-pending', fixture(food=0, growth=6, forest=True), True, 5), ('starvation', fixture(food=-1, immune=True, starving=True, forest=True), True, 0),
             ('last-citizen-removal', fixture(pop=1, food=-1, starving=True), True, 0), ('partial-starvation', fixture(food=-1, starving=True), False, 0),
             ('late-worker-failure', fixture(invalid_psych=True), True, 5),
             ('legacy7', fixture(immune=True, version=7), True, 5), ('legacy10', fixture(immune=True, version=10), True, 5)]
    records = []
    for i, (name, data, full, food) in enumerate(cases):
        text = rules(food)
        rp, sp, out = (args.out / f'{i}{s}' for s in ('.txt', '.SC', '.checkpoint'))
        rp.write_bytes(text)
        sp.write_bytes(data)
        error, expected, flags = model(data, text, full)
        run = subprocess.run([str(exe), str(rp), str(sp), '0', str(int(full)), str(out)], capture_output=True, text=True,
                             env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        assert run.returncode == 0, (name, run.returncode, run.stdout, run.stderr)
        assert f'HUMAN status={error} ' in run.stdout, (name, run.stdout, error)
        assert out.read_bytes() == expected, (name, 'whole checkpoint mismatch', run.stdout)
        if flags:
            for key, value in flags.items():
                assert f'{key}={int(value)} ' in run.stdout, (name, key, value, run.stdout)
        _, offsets, _ = population.intake.decode(data)
        before_pop = data[offsets['bases'] + 6]
        after_pop = expected[64 + len(data) + offsets['bases'] + 6]
        if name in ('growth-specialists-pending', 'boom-pending'):
            assert after_pop == before_pop + 1 and flags['food_checked'] and flags['specialists_pending'], (name, run.stdout)
        if name == 'starvation':
            assert after_pop == before_pop - 1 and flags['food_checked'], run.stdout
        if name == 'last-citizen-removal':
            assert flags['phase'] == 1 and expected[64 + len(data):] == data, run.stdout
        assert sp.read_bytes() == data and rp.read_bytes() == text
        (args.out / f'{i}.expected').write_bytes(expected)
        records.append(dict(index=i, name=name, mode=int(full), base=0, status=error, flags=flags, log=run.stdout,
                            checkpointSHA256=hashlib.sha256(expected).hexdigest()))
    # Qualify the new transport union itself, including specialist and credit/
    # research fields which this population-only prefix does not yet mutate.
    for mode, version in [(m, 9) for m in range(10, 18)] + [(10, 7), (10, 10)]:
        i = len(records)
        name = f'encoder-v{version}-mode{mode}'
        data, text = fixture(version=version), rules()
        rp, sp, out = (args.out / f'{i}{s}' for s in ('.txt', '.SC', '.checkpoint'))
        rp.write_bytes(text)
        sp.write_bytes(data)
        expected = encoder_model(data, mode)
        run = subprocess.run([str(exe), str(rp), str(sp), '0', str(mode), str(out)], capture_output=True, text=True,
                             env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        assert run.returncode == 0 and out.read_bytes() == expected, (name, run.stdout, run.stderr)
        error = 0 if mode == 10 else 3
        assert f'ENCODE status={error} ' in run.stdout, (name, run.stdout)
        assert sp.read_bytes() == data
        (args.out / f'{i}.expected').write_bytes(expected)
        records.append(dict(index=i, name=name, mode=mode, base=0, status=error, flags=None, log=run.stdout,
                            checkpointSHA256=hashlib.sha256(expected).hexdigest()))
    receipt = dict(cases=len(records), records=records, command=command if not args.binary else None,
                   binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                   scope='Private population/worker prefix; explicit specialist/riot/base-removal events; no public commit or full turn.')
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({k: v for k, v in receipt.items() if k not in ('records', 'command')}))


if __name__ == '__main__':
    main()
