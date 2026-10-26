#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Connected accounting against independent support/wire/resource oracles."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import test_base_support as support
import test_world_base_minerals as mineral
import test_world_stable_intake as intake
import test_world_convoy_nutrients as convoy
from test_base_nutrients import expected as nutrient_expected
from test_base_energy import expected as energy_expected

HERE = Path(__file__).resolve().parent
SOURCES = list(dict.fromkeys([name for name in mineral.SOURCES if name != 'world_base_minerals_probe'] + 'base_support world_base_support base_energy world_energy world_technology_effects world_base_accounting world_base_accounting_probe'.split()))


def model(variant, text, original, base_id, commerce, grid, mode=None):
    only = lambda error: {('BASE', base_id): [error]}
    data = mineral.normalized(original, base_id)
    supported = support.world_expected(data, text)
    if supported[0]:
        return only({4: 6, 5: 7}.get(supported[0], supported[0]))
    minerals = mineral.model(variant, text, original, base_id, supported[3], mode)
    if minerals[1]:
        return only(minerals[1])
    info, _, spans = intake.decode(data)
    legacy = int(info['version']) < 11
    bs, fs = (304, 0x20bc) if legacy else (308, 0x20cc)
    b = spans['bases'][:bs]
    faction = b[4]
    schema = intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction']
    f = spans['factions'][faction * fs:(faction + 1) * fs]
    state = lambda name: intake.scalar(f, schema, name)
    fresh = intake.model(variant, text, data)
    flow_error, flows = convoy.flow_model(variant, text, data)
    assert fresh[0] == flow_error == 0
    signed = lambda value: value - (1 << 32) if value >= (1 << 31) else value
    row, column, limits = intake.ROW['nutrient_req_citizen']
    requirement = max(limits[0], min(limits[1], int(intake.sections(text)['RULES'][row][column])))
    flags = int(bool(b[0x8c] & 4)) | (int(bool(struct.unpack_from('<I', b, 0x30)[0] & 4)) << 1)
    food = nutrient_expected([struct.unpack_from('<b', b, 6)[0], state('SE_growth_pending'), signed(fresh[7]), flows[0], flows[4], requirement, struct.unpack_from('<i', b, 0x3c)[0], state('nutrient_surplus_total'), 0, flags])
    if food[0]:
        return only(7 if food[0] == 3 else 1)
    width = int(info['width'])
    flat = struct.unpack_from('<I', spans['metadata'], 28)[0]
    x, y = struct.unpack_from('<hh', b)
    distance = 999
    for index in range(int(info['bases'])):
        other = spans['bases'][index * bs:(index + 1) * bs]
        if other[4] != faction or not other[0x8c] & 2:
            continue
        xx, yy = struct.unpack_from('<hh', other)
        dx, dy = abs(x - xx), abs(y - yy)
        if not flat and dx > width // 2:
            dx = width - dx
        distance = min(distance, max(dx, dy) - ((dx + dy) // 2 - min(dx, dy) + 1) // 2)
    distance = -1 if distance == 999 else distance
    efficiency = state('SE_effic_pending')
    field = next(v['offset'] for v in schema['fields'] if v['name'] == 'social_effic')
    statistics = list(struct.unpack_from('<9i', f, field))
    creche = int(bool(b[0x8c] & 4))
    energy = energy_expected([signed(fresh[9]), flows[2], commerce, grid, flows[6], distance, efficiency, 0, creche, *statistics])
    if energy[0]:
        return only(7 if energy[0] == 3 else 1)
    cancellation = [0] * 64
    cancellations = 0
    if energy[4] < 0:
        for index in range(int(info['vehicles'])):
            vehicle = spans['vehicles'][index * 52:(index + 1) * 52]
            prototype = struct.unpack_from('<h', vehicle, 10)[0]
            plan = struct.unpack_from('<b', spans['prototypes'], prototype * 52 + 42)[0]
            if vehicle[14] != faction or plan != 10 or vehicle[17] != 3 or vehicle[38] != 2:
                continue
            xx, yy = struct.unpack_from('<hh', vehicle)
            tile = spans['tiles'][44 * ((yy * width + xx) // 2):]
            if struct.unpack_from('<I', tile, 8)[0] & 1 and tile[2] & 15 < 8:
                cancellation[index // 32] |= 1 << (index % 32)
                cancellations += 1
    return {('BASE', base_id): [0], ('SUPPORT', base_id): supported[1:9], ('MINERAL', base_id): minerals[2:], ('NUTRIENT', base_id): food[1:], ('ENERGY', base_id): [distance, efficiency, creche, *energy[1:]], ('SHORTFALL', base_id): [cancellations, *cancellation], ('STATES', base_id): [int(info['vehicles']), *supported[9:]]}


def fixture(case, version=10):
    variant = 'base' if version < 11 else 'expansion'
    data = bytearray(mineral.fixture(variant, version, case))
    _, off, _ = intake.decode(data)
    fs = 0x20bc if version < 11 else 0x20cc
    schema = intake.FACTIONS['LegacyFactionV10' if version < 11 else 'Faction']
    for name, value in [('SE_support_pending', case % 8 - 4), ('SE_police_pending', [-4, -3, -2, 0][case % 4]), ('SE_effic_pending', case % 9 - 4), ('SE_growth_pending', case % 7 - 2), ('nutrient_surplus_total', 17)]:
        field = next(v['offset'] for v in schema['fields'] if v['name'] == name)
        struct.pack_into('<i', data, off['factions'] + fs + field, value)
    field = next(v['offset'] for v in schema['fields'] if v['name'] == 'social_effic')
    struct.pack_into('<9i', data, off['factions'] + fs + field, *range(9))
    # Three genuine actors become supported land units; other actors remain
    # actual supply convoys, so the connected support/flow paths both matter.
    struct.pack_into('<b', data, off['prototypes'] + 52 + 42, 0)
    for index in range(3):
        at = off['vehicles'] + index * 52
        struct.pack_into('<h', data, at + 10, 1)
        data[at + 17] = 0
    bs = 304 if version < 11 else 308
    if case % 3 < 2:
        data[off['bases'] + (case % 3) * bs + 0x8c] |= 2
    if case & 1:
        data[off['bases'] + 0x8c] |= 4
    # These saved outputs must not influence any newly computed resource.
    for delta in [0xb0, 0xb4, 0xb8, 0xbc, 0xc0, 0xc4, 0xc8, 0xcc, 0xd8, 0xe8, 0xf8]:
        struct.pack_into('<i', data, off['bases'] + delta - (4 if version < 11 and delta >= 0x98 else 0), -987654321)
    return bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--cc', default='c++')
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--original', type=Path)
    parser.add_argument('--original-rules', type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.binary.resolve() if args.binary else args.out / 'world-base-accounting-probe'
    command = []
    if not args.binary:
        command = [args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie', *[str(HERE / (name + '.cpp')) for name in SOURCES], '-o', str(exe)]
        subprocess.run(command, check=True)
    text = intake.rules('base')
    tables = intake.sections(text)
    row, column, _ = intake.ROW['nutrient_req_citizen']
    tables['RULES'][row][column] = '2'
    text = intake.material(tables)
    cases = [(f'base-{case}', 'base', text, fixture(case, 7 + case % 4), case % 5, case % 3, None, [0]) for case in range(1, 65)]
    seed = fixture(1)
    _, offsets, _ = intake.decode(seed)
    cases += [('late-energy-overflow', 'base', text, seed, 2147483647, 0, None, [0]), ('negative-commerce', 'base', text, seed, -1, 0, None, [0]), ('negative-grid', 'base', text, seed, 0, -1, None, [0]), ('upkeep1', 'base', text, seed, 0, 0, 'upkeep', [0]), ('editor', 'base', text, seed, 0, 0, 'editor', [0])]
    for name, offset, fmt, value in [('needs-allocation', offsets['bases'] + 0x78, 'I', 1), ('bad-support-plan', offsets['prototypes'] + 52 + 42, 'b', -1), ('bad-support-weapon', offsets['prototypes'] + 52 + 37, 'B', 255)]:
        changed = bytearray(seed)
        struct.pack_into('<' + fmt, changed, offset, value)
        cases.append((name, 'base', text, bytes(changed), 0, 0, None, [0]))
    zero = intake.sections(text)
    for row in zero['RESOURCEINFO']:
        row[3] = '0'
    shortfall = bytearray(fixture(1))
    _, shortfall_offsets, _ = intake.decode(shortfall)
    shortfall[shortfall_offsets['vehicles'] + 3 * 52 + 38] = 2
    cases.append(('energy-shortfall', 'base', intake.material(zero), bytes(shortfall), 0, 0, None, [0]))
    cases.append(('expansion-reject', 'expansion', intake.rules('expansion'), fixture(7, 12), 0, 0, None, [0]))
    if args.original:
        if not args.original_rules:
            parser.error('--original-rules is required with --original')
        raw = args.original.read_bytes()
        info, _, _ = intake.decode(raw)
        bases = list(range(int(info['bases'])))
        cases += [('Boreholes', 'base', args.original_rules.read_bytes(), raw, 0, 0, None, bases), ('Boreholes-late-overflow', 'base', args.original_rules.read_bytes(), raw, 2147483647, 0, None, bases), ('Boreholes-upkeep-reject', 'base', args.original_rules.read_bytes(), raw, 0, 0, 'upkeep', bases)]
    records = []
    digest = hashlib.sha256()
    for name, variant, rules, data, commerce, grid, mode, bases in cases:
        path, rule = args.out / 'input.SC', args.out / 'rules.txt'
        path.write_bytes(data)
        rule.write_bytes(rules)
        run = subprocess.run([str(exe), variant, str(rule), str(path), str(commerce), str(grid), *([mode] if mode else [])], capture_output=True, timeout=60, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        assert run.returncode == 0, (name, run.returncode, run.stdout, run.stderr)
        parsed = {(parts[0], int(parts[1])): [int(value) for value in parts[2:]] for parts in [line.split() for line in run.stdout.decode().splitlines()]}
        for base in bases:
            want = model(variant, rules, data, base, commerce, grid, mode)
            got = {key: value for key, value in parsed.items() if key[1] == base}
            assert got == want, (name, base, got, want)
        if name == 'energy-shortfall':
            assert parsed['ENERGY', 0][6] < 0 and parsed['SHORTFALL', 0][0] == 2
        if name == 'late-energy-overflow':
            assert parsed['BASE', 0] == [7]
        assert path.read_bytes() == data and rule.read_bytes() == rules
        digest.update(run.stdout)
        records.append(dict(name=name, bases=len(bases), accepted=sum(parsed['BASE', base] == [0] for base in bases), inputSHA256=hashlib.sha256(data).hexdigest(), outputSHA256=hashlib.sha256(run.stdout).hexdigest()))
        if name == 'Boreholes':
            (args.out / 'Boreholes-accounting.txt').write_bytes(run.stdout)
    receipt = dict(worlds=len(cases), queriedBases=sum(row['bases'] for row in records), accepted=sum(row['accepted'] for row in records), outputSHA256=digest.hexdigest(), allWorldRulesStagesAndBothOutputsTransactional=True, aliasShortCapacityAndTailGuards=True, native='UNRUN', command=command, records=records)
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({key: value for key, value in receipt.items() if key not in ['records', 'command']}))


if __name__ == '__main__':
    main()
