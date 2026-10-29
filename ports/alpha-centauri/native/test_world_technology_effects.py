#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh technology effects, independent wire oracle and immutable inputs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
from test_original_world import valid_fixture
from test_original_game import decode, SCHEMA, FORMATS
from test_original_faction import SCHEMA as FACTIONS
from test_world_unit_stats import rules_fixture
from test_units import sections, material
from technology_effects_reference import effects

HERE = Path(__file__).resolve().parent


def fixture(version, case):
    data = bytearray(valid_fixture(version))
    _, offsets, _ = decode(data)
    legacy = version < 11
    stride = 0x20bc if legacy else 0x20cc
    fs = FACTIONS['LegacyFactionV10' if legacy else 'Faction']
    ds = FACTIONS['MFaction']
    def write(start, schema, name, value, index=0):
        field = next(f for f in schema['fields'] if f['name'] == name)
        fmt = FORMATS[field['type']]
        struct.pack_into('<' + fmt, data, start + field['offset'] + index * struct.calcsize(fmt), value)
    pf = offsets['legacy'] if legacy else offsets['preferences']
    field = next(f for f in SCHEMA['GamePrefs']['fields'] if f['name'] == 'TechOwners')
    owner_offset = pf + field['offset'] - (0x7c if legacy and field['offset'] >= 0x184 else 0)
    for index in range(89):
        data[owner_offset + index] = (case * 37 + index * 19) & 255
    for faction in range(8):
        st = offsets['factions'] + faction * stride
        df = offsets['faction_definitions'] + faction * 0x59c
        write(st, fs, 'SE_economy_pending', [-(1 << 31), -1, 0, 2, 3, 4, 5, (1 << 31)-1][(case + faction) % 8])
        for name in ['tech_fungus_nutrient', 'tech_fungus_mineral', 'tech_fungus_energy', 'tech_commerce_bonus']:
            write(st, fs, name, -2147483648 if case & 1 else 2147483647)
        write(df, ds, 'rule_commerce', [-100, 0, 5, 2147483647][(case + faction) % 4])
        count = [0, 1, 3, 8, -1, 9][(case + faction) % 6]
        write(df, ds, 'faction_bonus_count', count)
        for index in range(8):
            write(df, ds, 'faction_bonus_id', [7, 8, 9, 2][(case + index) % 4], index)
            write(df, ds, 'faction_bonus_val1', [-2147483648, -90, -1, 0, 1, 200, 2147483647][(case + faction + index) % 7], index)
    return bytes(data)


def text(case):
    table = sections(rules_fixture('base'))
    for index, row in enumerate(table['TECHNOLOGY']):
        flags = ((case + index) % 16)
        mask = sum(bit for slot, bit in enumerate([4, 64, 128, 256]) if flags & (1 << slot))
        row[8] = format(mask, '09b')
        if index % 13 == case % 13:
            row[6] = 'Disable'
        elif index % 11 == case % 11:
            row[6:8] = ['None', 'Disable']
    return material(table)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--cc', default='c++')
    parser.add_argument('--rules', type=Path)
    parser.add_argument('--original', type=Path, action='append', default=[])
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.out / 'technology-effects'
    sources = ['rules_text.cpp', 'original_map.cpp', 'original_game.cpp', 'original_faction.cpp', 'original_world.cpp',
               'prototype.cpp', 'movement.cpp', 'saved_speed.cpp', 'world_unit_stats.cpp',
               'world_technology_effects.cpp', 'world_technology_effects_probe.cpp']
    command = [args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
               '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
               *[str(HERE / name) for name in sources], '-o', str(exe)]
    subprocess.run(command, check=True)
    cases = [(f'v{v}-{c}', text(c), fixture(v, c), False) for v in range(7, 13) for c in range(16)]
    cases += [('invalid-rule-count', text(0), fixture(12, 0), True)]
    if args.original:
        if not args.rules:
            parser.error('--rules is required for --original')
        cases += [('original-' + p.name, args.rules.read_bytes(), p.read_bytes(), False) for p in args.original]
    total = hashlib.sha256()
    counts = {}
    for index, (name, rules, data, bad_count) in enumerate(cases):
        rule = args.out / f'{index}.txt'
        save = args.out / f'{index}.SC'
        rule.write_bytes(rules)
        save.write_bytes(data)
        want = []
        for faction in range(-1, 9):
            error, values = (1, None) if bad_count else effects(rules, data, faction)
            counts[error] = counts.get(error, 0) + 1
            want.extend([error, *(values if error == 0 else [0x5a5a5a5a] * 5)])
        encoded = struct.pack('<60I', *[value & 0xffffffff for value in want])
        result = subprocess.run([str(exe), str(rule), str(save), *(['bad-count'] if bad_count else [])],
                                capture_output=True, timeout=30, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        assert result.returncode == 0, (name, result.returncode, result.stderr)
        assert result.stdout == encoded, (name, struct.unpack('<60I', result.stdout), want)
        assert rule.read_bytes() == rules and save.read_bytes() == data
        total.update(encoded)
    receipt = dict(worlds=len(cases), factionQueries=10 * len(cases), errorCounts=counts,
                   allWorldInputsAndWireUnchanged=True, outputSHA256=total.hexdigest(),
                   binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(), native='UNRUN')
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))


if __name__ == '__main__':
    main()
