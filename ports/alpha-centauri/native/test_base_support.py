#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent arithmetic/byte-level oracle for the explicit restoration policy."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import struct
import subprocess
from test_world_stable_intake import fixture as world_fixture, rules, decode, SCHEMA, FACTIONS, FORMATS
from test_units import sections, material

HERE = Path(__file__).resolve().parent
SENTINEL = 0x5A5A5A5A
MASK = 0x600010

def expected(facts, units):
    support, police, pop = facts
    if not 0 <= pop <= 127 or any(not 0 <= p <= 14 or not 0 <= t <= 2 for _, p, _, t, _ in units):
        return [1] + [SENTINEL] * (8 + len(units))
    free = [0, 0, 1, 1, 2, 3, 4, max(4, pop)][min(7, max(0, support + 4))]
    cost = 2 if support <= -4 else 1
    # Compute totals from index sets, separately from the C++ streaming loop.
    charged = [i for i, (_, p, _, _, flags) in enumerate(units) if flags & 1 and p <= 9 and not flags & 6]
    paid = set(charged[free:])
    away = [i for i, (_, p, o, t, flags) in enumerate(units) if flags & 1 and p <= 9 and o and (flags & 8 or (t == 2 and p != 4))]
    skipped = {away[0]} if away and police == -3 else set()
    drones = set(away) - skipped if police <= -3 else set()
    states = []
    for i, (state, _, _, _, flags) in enumerate(units):
        if flags & 1:
            state &= ~MASK
            state |= (0x10 if i in paid else 0) | (0x200000 if i in drones else 0) | (0x400000 if i in skipped else 0)
        states.append(state)
    return [0, len(charged), len(paid), len(paid) * cost, len(away), len(drones), len(skipped), free, cost, *states]

def leaf_cases():
    result = []
    # Exercise every plan, triad and exemption independently, with stale flags.
    for support, police, pop in itertools.product(range(-5, 5), [-4, -3, -2, 0], [0, 2, 4, 9, 127]):
        units = [(0xFFFFFFFF, plan, offense, triad, flags) for plan, offense, triad, flags in
                 itertools.product(range(15), [-1, 0, 1], range(3), range(16))]
        for start in range(0, len(units), 200):
            result.append(((support, police, pop), units[start:start + 200]))
    rng = random.Random(0xE8FC)
    for _ in range(1200):
        units = [(rng.getrandbits(32), rng.randrange(-1, 16), rng.choice([-1, 0, 1]), rng.randrange(-1, 4), rng.randrange(16)) for _ in range(rng.randrange(40))]
        result.append(((rng.choice([-(1 << 31), -4, -3, -2, -1, 0, 2, 3, (1 << 31) - 1]), rng.choice([-4, -3, -2, 0]), rng.choice([-1, 0, 127, 128])), units))
    result += [((0, -3, 4), []), ((0, -4, 4), []), ((0, 0, -1), [])]
    return result

def packet(cases):
    b = bytearray(struct.pack('<I', len(cases)))
    for facts, units in cases:
        b += struct.pack('<iiiI', *facts, len(units))
        for state, plan, offense, triad, flags in units:
            b += struct.pack('<IiiiI', state, plan, offense, triad, flags)
    return bytes(b)

def world_data(seed, version=10, count=24):
    rng = random.Random(seed)
    b = bytearray(world_fixture('base' if version < 11 else 'expansion', version, 3))
    _, off, spans = decode(b)
    legacy = version < 11
    pf = off['legacy' if legacy else 'preferences']
    field = next(f for f in SCHEMA['GamePrefs']['fields'] if f['name'] == 'VehCount')
    struct.pack_into('<i', b, pf + field['offset'] - (0x7C if legacy and field['offset'] >= 0x184 else 0), count)
    fs = 0x20BC if legacy else 0x20CC
    for name, value in [('SE_support_pending', seed % 10 - 5), ('SE_police_pending', [-4, -3, -2, 0][seed % 4])]:
        f = next(f for f in FACTIONS['LegacyFactionV10' if legacy else 'Faction']['fields'] if f['name'] == name)
        struct.pack_into('<i', b, off['factions'] + fs + f['offset'], value)
    # Every vehicle occupies a different valid map coordinate, no stack shortcuts.
    vehicles = bytearray(52 * count)
    for i in range(count):
        x, y = (i % 8) * 2 + (i // 8) % 2, i // 8
        v = 52 * i
        proto = i + 1 if i < 20 else 64 + i
        struct.pack_into('<hhI', vehicles, v, x, y, 0xFFFFFFFF)
        struct.pack_into('<h', vehicles, v + 10, proto)
        vehicles[v + 14] = 2 if i % 7 == 0 else 1
        struct.pack_into('<hhh', vehicles, v + 46, -1 if i % 11 == 0 else 0, -1, -1)
        p = off['prototypes'] + 52 * proto
        b[p:p + 52] = bytes(52)
        struct.pack_into('<I', b, p + 32, 0x10000 if rng.randrange(4) == 0 else 0)
        b[p + 36], b[p + 37], b[p + 39] = rng.randrange(9), rng.randrange(3), 1
        struct.pack_into('<b', b, p + 42, rng.randrange(15))
        tile = off['tiles'] + 44 * ((y * 16 + x) // 2)
        b[tile], b[tile + 2], b[tile + 7] = rng.choice([0, 0x20, 0x40, 0x60]), 1, rng.choice([1, 2, 255])
        struct.pack_into('<I', b, tile + 8, 2 | (0x20 if rng.randrange(2) else 0) | (1 if rng.randrange(5) == 0 else 0))
    b[off['bases']:off['bases']] = vehicles
    return bytes(b)

def world_expected(data, text):
    info, _, spans = decode(data)
    count = int(info['vehicles'])
    version = int(info['version'])
    if version >= 11:
        return [3] + [SENTINEL] * (8 + count)
    base = spans['bases'][:304]
    faction, pop = base[4], struct.unpack_from('<b', base, 6)[0]
    if not 1 <= faction <= 7 or pop < 0:
        return [1] + [SENTINEL] * (8 + count)
    f = next(f for f in SCHEMA['GamePrefs']['fields'] if f['name'] == 'FactionStatus')
    if not spans['legacy'][f['offset'] - (0x7C if f['offset'] >= 0x184 else 0)] & (1 << faction):
        return [3] + [SENTINEL] * (8 + count)
    schema = FACTIONS['LegacyFactionV10']
    facts = []
    for name in ['SE_support_pending', 'SE_police_pending']:
        f = next(f for f in schema['fields'] if f['name'] == name)
        facts.append(struct.unpack_from('<i', spans['factions'], faction * 0x20BC + f['offset'])[0])
    facts.append(pop)
    tables = sections(text)
    units = []
    for i in range(count):
        v = spans['vehicles'][52 * i:52 * (i + 1)]
        state = struct.unpack_from('<I', v, 4)[0]
        if v[14] != faction or struct.unpack_from('<h', v, 46)[0] != 0:
            units.append((state, 0, 0, 0, 0))
            continue
        proto = struct.unpack_from('<h', v, 10)[0]
        p = spans['prototypes'][52 * proto:52 * (proto + 1)]
        plan = struct.unpack_from('<b', p, 42)[0]
        if p[36] >= 9 or p[37] >= 21 or not 0 <= plan <= 14:
            return [1] + [SENTINEL] * (8 + count)
        # Component file rows contain names/abbreviations before numeric offense.
        offense = int(tables['WEAPONS'][p[37]][2])
        triad = int(tables['CHASSIS'][p[36]][9])
        x, y = struct.unpack_from('<hh', v)
        sq = spans['tiles'][44 * ((y * int(info['width']) + x) // 2):]
        owner = struct.unpack_from('<b', sq, 7)[0]
        if owner not in range(-1, 8):
            return [1] + [SENTINEL] * (8 + count)
        items = struct.unpack_from('<I', sq, 8)[0]
        clean = bool(struct.unpack_from('<I', p, 32)[0] & 0x10000)
        native = proto < 64 and offense < 0 and bool(items & 0x20) and sq[0] & 0xE0 >= 0x40
        away = not(items & 1 and sq[2] & 15 < 8) and owner != faction
        units.append((state, plan, offense, triad, 1 | (2 if clean else 0) | (4 if native else 0) | (8 if away else 0)))
    return expected(facts, units)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--cc', default='c++')
    ap.add_argument('--binary', type=Path)
    ap.add_argument('--m68k-linux', action='store_true')
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.binary.resolve() if args.binary else args.out / 'support-probe'
    if not args.binary:
        flags = ['-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror'] + (['-m68040', '-static'] if args.m68k_linux else ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie'])
        names = ['rules_text', 'original_map', 'original_game', 'original_faction', 'original_world', 'base_support', 'world_base_support', 'base_support_probe']
        subprocess.run([args.cc, *flags, *[str(HERE / (n + '.cpp')) for n in names], '-o', str(exe)], check=True)
    runner = ['qemu-m68k', '-cpu', 'm68040'] if args.m68k_linux else []
    env = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'}
    cases = leaf_cases()
    source, target = args.out / 'support-cases.bin', args.out / 'support-results.bin'
    source.write_bytes(packet(cases))
    subprocess.run([*runner, str(exe), 'leaf', str(source), str(target)], check=True, env=env)
    expected_bytes = b''.join(struct.pack('<' + 'I' * len(v), *v) for v in [expected(*case) for case in cases])
    assert target.read_bytes() == expected_bytes, 'leaf oracle mismatch'
    text = rules('base')
    # Force known offense and triad values rather than rely on fixture names.
    tables = sections(text)
    for i in range(3): tables['WEAPONS'][i][2] = str([-1, 0, 1][i])
    for i in range(9): tables['CHASSIS'][i][9] = str(i % 3)
    text = material(tables)
    rulefile = args.out / 'rules.txt'
    rulefile.write_bytes(text)
    worlds = [(str(seed), world_data(seed, 7 + seed % 4, seed % 25)) for seed in range(80)]
    worlds += [('expansion', world_data(17, 12, 24))]
    seed = world_data(17, 10, 24)
    _, offsets, _ = decode(seed)
    status = next(f for f in SCHEMA['GamePrefs']['fields'] if f['name'] == 'FactionStatus')['offset']
    for name, offset, fmt, value in [('ai', offsets['legacy'] + status - (0x7C if status >= 0x184 else 0), 'B', 0), ('planet-base', offsets['bases'] + 4, 'B', 0), ('bad-pop', offsets['bases'] + 6, 'b', -1), ('bad-weapon', offsets['prototypes'] + 52 * 2 + 37, 'B', 255), ('bad-plan', offsets['prototypes'] + 52 * 2 + 42, 'b', -1), ('bad-territory', offsets['tiles'] + 44 + 7, 'b', 8)]:
        changed = bytearray(seed)
        struct.pack_into('<' + fmt, changed, offset, value)
        worlds.append((name, bytes(changed)))
    for name, data in worlds:
        save, output = args.out / 'world.SC', args.out / 'world-result.bin'
        save.write_bytes(data)
        subprocess.run([*runner, str(exe), 'world', str(rulefile), str(save), str(output)], check=True, env=env)
        want = world_expected(data, text)
        assert output.read_bytes() == struct.pack('<' + 'I' * len(want), *want), f'world oracle mismatch {name}'
    # Keep a rich synthetic case for the actual Amiga HUNK lane.
    data = world_data(17, 10, 24)
    (args.out / 'world.SC').write_bytes(data)
    want = world_expected(data, text)
    (args.out / 'world-expected.bin').write_bytes(struct.pack('<' + 'I' * len(want), *want))
    receipt = {'leaf_cases': len(cases), 'world_cases': len(worlds), 'source_policy': 'Thinker default adaptation; original support routine parity unproven', 'leaf_sha256': hashlib.sha256(expected_bytes).hexdigest(), 'world_sha256': hashlib.sha256((args.out / 'world-expected.bin').read_bytes()).hexdigest()}
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))

if __name__ == '__main__': main()
