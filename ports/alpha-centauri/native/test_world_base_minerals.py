#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned-world fresh minerals against independent wire/intake/integer models."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import test_world_stable_intake as intake
import test_world_convoy_nutrients as convoy
from test_upkeep_base_minerals import expected as mineral_expected
from test_original_game import decode, SCHEMA, FORMATS

HERE = Path(__file__).resolve().parent
SOURCES = 'rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats vehicle_stack tile_yields world_technology_effects world_upkeep_tile_yields world_tile_yields technology_effects world_fresh_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients upkeep_base_minerals world_base_minerals world_base_minerals_probe'.split()


def normalized(data, target):
    """Only the oracle's copy renumbers the requested base to existing model0."""
    if target == 0:
        return data
    b = bytearray(data)
    info, off, _ = decode(b)
    legacy = int(info['version']) < 11
    bs = 304 if legacy else 308
    start = off['bases']
    first = b[start:start + bs]
    b[start:start + bs] = b[start + target * bs:start + (target + 1) * bs]
    b[start + target * bs:start + (target + 1) * bs] = first
    pf = off['legacy'] if legacy else off['preferences']
    for index in range(33 if legacy else 64):
        at = pf + 0x84 + index * 4
        value = struct.unpack_from('<i', b, at)[0]
        if value == 0:
            struct.pack_into('<i', b, at, target)
        elif value == target:
            struct.pack_into('<i', b, at, 0)
    for index in range(int(info['vehicles'])):
        at = off['vehicles'] + index * 52 + 46
        value = struct.unpack_from('<h', b, at)[0]
        if value == 0:
            struct.pack_into('<h', b, at, target)
        elif value == target:
            struct.pack_into('<h', b, at, 0)
    return bytes(b)


def model(variant, text, original, base, maintenance, mode=None):
    data = normalized(original, base)
    info, _, spans = decode(data)
    version = int(info['version'])
    if maintenance < 0 or (version < 11) != (variant == 'base') or mode == 'profile':
        return [base, 1]
    if mode in ['editor', 'upkeep']:
        return [base, 3]
    legacy = version < 11
    prefs = spans['legacy'] if legacy else spans['preferences']
    b = spans['bases'][:304 if legacy else 308]
    faction = b[4]
    fs = 0x20bc if legacy else 0x20cc
    f = spans['factions'][faction * fs:(faction + 1) * fs]

    def pref(name, index=0):
        field = next(x for x in SCHEMA['GamePrefs']['fields'] if x['name'] == name)
        offset = field['offset'] - (0x7c if legacy and field['offset'] >= 0x184 else 0)
        return struct.unpack_from('<' + FORMATS[field['type']], prefs, offset + index * struct.calcsize(FORMATS[field['type']]))[0]

    def state(name):
        return intake.scalar(f, intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction'], name)

    def project(index):
        location = pref('SecretProjects', index)
        if location < -2 or location >= int(info['bases']):
            raise ValueError('invalid project location')
        return location >= 0 and spans['bases'][location * (304 if legacy else 308) + 4] == faction

    flow_error, flows = convoy.flow_model(variant, text, data)
    if flow_error:
        return [base, flow_error]
    fresh = intake.model(variant, text, data)
    if fresh[0]:
        return [base, fresh[0]]
    tables = intake.sections(text)
    row = -1
    for line in (HERE / 'world_fields.inc').read_text().splitlines():
        if line.startswith('RULE_ROW('):
            row += 1
        if line.startswith('RULE_ROW(sea_level_rises,'):
            rises = max(1, min(100, int(tables['WORLDBUILDER'][row][0])))
            break
    physical = lambda index: int(bool(b[0x8c + index // 8] & (1 << (index % 8))))
    try:
        effects = [physical(21), physical(19), physical(22), physical(20), int(project(29)), int(project(28)), physical(31), physical(32), int(project(16))]
        voice = pref('SecretProjects', 31)
        if voice < -2 or voice >= int(info['bases']):
            return [base, 1]
        first = 34 if variant == 'base' else 65
        orbital = int(project(27) and -(first + 3) <= struct.unpack_from('<i', b, 0x50)[0] <= -first)
    except ValueError:
        return [base, 1]
    tectonic = struct.unpack_from('<I', spans['tectonic'], faction * 4)[0] if version >= 12 else 0
    if tectonic > 0x7fffffff:
        return [base, 7]
    signed = lambda value: value - (1 << 32) if value >= (1 << 31) else value
    facts = [signed(fresh[8]), flows[1], maintenance, flows[5], signed(fresh[13]), state('clean_minerals_modifier'), state('satellites_mineral'), struct.unpack_from('<b', b, 6)[0], state('major_atrocities'), tectonic, struct.unpack_from('<i', spans['metadata'], 12)[0], rises, state('diff_level'), pref('DiffLevel'), state('tech_ranking'), state('tech_count_transcendent'), state('SE_planet_pending'), struct.unpack_from('<i', spans['metadata'], 0xa48)[0], *effects, int(bool(pref('FactionStatus') & (1 << faction))), int(voice != -1 and bool(pref('GameRules') & 0x800)), int(bool(pref('GameState') & 0x100000)), orbital]
    result = mineral_expected(facts)
    if result[0]:
        return [base, 7 if result[0] == 3 else 1]
    return [base, 0, *facts, *result[1:]]


def fixture(variant, version, case):
    data = bytearray(convoy.fixture(variant, version, case))
    _, off, spans = decode(data)
    legacy = version < 11
    pf = off['legacy'] if legacy else off['preferences']
    fs = 0x20bc if legacy else 0x20cc
    for name, value in [('diff_level', case % 6), ('clean_minerals_modifier', [-17, 0, 10][case % 3]), ('major_atrocities', case % 4), ('tech_ranking', 30), ('tech_count_transcendent', case % 6), ('SE_planet_pending', [-3, 0, 2][case % 3])]:
        field = next(x for x in intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction']['fields'] if x['name'] == name)
        struct.pack_into('<i', data, off['factions'] + fs + field['offset'], value)
    struct.pack_into('<i', data, pf + 0x34, case % 6)
    struct.pack_into('<i', data, off['metadata'] + 0xa48, case % 3)
    struct.pack_into('<i', data, off['metadata'] + 12, case * 3 - 20)
    if version >= 12:
        struct.pack_into('<8I', data, off['tectonic'], *[case % 3] * 8)
    for bit, facility in enumerate([19, 20, 21, 22, 31, 32]):
        if case & (1 << bit):
            data[off['bases'] + 0x8c + facility // 8] |= 1 << (facility % 8)
    for bit, project in enumerate([16, 27, 28, 29]):
        if case & (1 << bit):
            struct.pack_into('<i', data, pf + 0x84 + project * 4, 0)
    struct.pack_into('<i', data, pf + 0x84 + 31 * 4, [-1, -2, 0][case % 3])
    struct.pack_into('<i', data, pf + 0xc, 0x800 if case & 8 else 0)
    struct.pack_into('<i', data, pf + 0x30, 0x10 | (0x100000 if case & 16 else 0))
    first = 34 if variant == 'base' else 65
    struct.pack_into('<i', data, off['bases'] + 0x50, -(first + case % 4))
    return bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--cc', default='c++')
    parser.add_argument('--original', type=Path)
    parser.add_argument('--original-rules', type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.out / 'world-base-minerals-probe'
    command = [args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie', *[str(HERE / (name + '.cpp')) for name in SOURCES], '-o', str(exe)]
    subprocess.run(command, check=True)
    digest = hashlib.sha256()
    records = []
    for variant in ['base', 'expansion']:
        text = intake.rules(variant)
        for case in range(1, 65):
            version = 7 + case % 4 if variant == 'base' else 11 + case % 2
            data = fixture(variant, version, case)
            records.append((f'{variant}-{case}', variant, text, data, case % 7, None, [0]))
    seed = fixture('expansion', 12, 47)
    for mode, maintenance in [('upkeep', 0), ('editor', 0), ('profile', 0), (None, -1), (None, 2147483647)]:
        records.append((f'reject-{mode}-{maintenance}', 'expansion', intake.rules('expansion'), seed, maintenance, mode, [0]))
    _, offsets, _ = decode(seed)
    field = lambda name: next(x for x in intake.FACTIONS['Faction']['fields'] if x['name'] == name)['offset']
    boundaries = [
        ('tectonic-overflow', offsets['tectonic'] + 4, 'I', 0x80000000),
        ('native-life', offsets['metadata'] + 0xa48, 'i', 3),
        ('difficulty', offsets['preferences'] + 0x34, 'i', 6),
        ('owner-difficulty', offsets['factions'] + 0x20cc + field('diff_level'), 'i', -1),
        ('project-location', offsets['preferences'] + 0x84 + 28 * 4, 'i', -3),
        ('nonhuman', offsets['preferences'] + 0x58, 'B', 0),
        ('specialist', offsets['bases'] + 0x7c, 'i', 1),
        ('technology-boundary', offsets['factions'] + 0x20cc + field('tech_ranking'), 'i', 2147483647),
    ]
    for name, offset, fmt, value in boundaries:
        data = bytearray(seed)
        struct.pack_into('<' + fmt, data, offset, value)
        records.append((name, 'expansion', intake.rules('expansion'), bytes(data), 3, None, [0]))
    if args.original:
        if not args.original_rules:
            parser.error('--original-rules is required with --original')
        data = args.original.read_bytes()
        info, _, _ = decode(data)
        for maintenance in [0, 3, 7]:
            records.append((f'original-support{maintenance}', 'base' if int(info['version']) < 11 else 'expansion', args.original_rules.read_bytes(), data, maintenance, None, list(range(int(info['bases'])))))
    receipt = []
    for index, (name, variant, text, data, maintenance, mode, bases) in enumerate(records):
        rule = args.out / 'rules.txt'
        path = args.out / 'input.SC'
        rule.write_bytes(text)
        path.write_bytes(data)
        run = subprocess.run([str(exe), variant, str(rule), str(path), str(maintenance), *([mode] if mode else [])], capture_output=True, timeout=60, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        assert run.returncode == 0, (name, run.returncode, run.stderr)
        lines = [[int(x) for x in line.split()[1:]] for line in run.stdout.decode().splitlines()]
        for base in bases:
            want = model(variant, text, data, base, maintenance, mode)
            assert lines[base] == want, (name, base, lines[base], want)
        assert path.read_bytes() == data and rule.read_bytes() == text
        digest.update(run.stdout)
        receipt.append(dict(name=name, bases=len(bases), accepted=sum(lines[i][1] == 0 for i in bases), inputSHA256=hashlib.sha256(data).hexdigest(), outputSHA256=hashlib.sha256(run.stdout).hexdigest()))
    result = dict(worlds=len(records), queriedBases=sum(x['bases'] for x in receipt), accepted=sum(x['accepted'] for x in receipt), outputSHA256=digest.hexdigest(), allWorldInputsAndWireUnchanged=True, native='UNRUN', originalExecutable='UNRUN', command=command, records=receipt)
    (args.out / 'receipt.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: value for key, value in result.items() if key not in ['records', 'command']}))


if __name__ == '__main__':
    main()
