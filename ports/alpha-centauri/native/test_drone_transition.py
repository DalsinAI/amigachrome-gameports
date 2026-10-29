#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent flag/event/damage/CRT and fresh-psych world wire oracles."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import test_world_base_psych as psych
import test_world_facility_upkeep as maintenance
import test_world_base_minerals as minerals

HERE = Path(__file__).resolve().parent
SOURCES = [s for s in psych.SOURCES if s != 'world_base_psych_probe'] + ['combat_rounds', 'combat_crt', 'drone_transition', 'world_drone_transition', 'drone_transition_probe']
SENTINEL = 0x5a5a5a5a


def model(row):
    pop, talents, drones, superdrones, police, assimilation, bases, difficulty, rank, flags, governor, low, high, plow, phigh, nerve, matrix, seed = row
    original = flags
    built, protected = low | high << 32, plow | phigh << 32
    if not (0 <= pop <= 127 and 0 <= talents <= pop and 0 <= drones <= pop and talents + drones <= pop and
            0 <= superdrones <= drones and -100 <= police <= 100 and 0 <= assimilation <= 255 and
            1 <= bases <= 512 and 0 <= difficulty <= 5 and 0 <= rank <= 7):
        return [1, seed] + [SENTINEL] * 14
    draws = attempts = 0
    notices = []
    destroyed = -1
    repaint = False
    def draw():
        nonlocal seed, draws
        seed = (seed * 214013 + 2531011) & 0xffffffff
        draws += 1
        return (seed >> 16) & 32767
    golden = drones == 0 and pop > 2 and talents >= (pop + 1) // 2
    if golden:
        if not flags & 4:
            flags |= 4
            notices.append(1)
    elif flags & 4:
        flags &= ~4
        if drones <= talents:
            notices.append(2)
    manage = governor & 0x80000040 == 0x80000040
    if drones <= talents or nerve or matrix:
        if flags & 2:
            flags &= ~2
            repaint = True
            if not flags & 4:
                notices.append(6 if manage else 5)
    else:
        first = not flags & 2
        flags |= 2
        notices.append(4 if first and manage else 3)
        if first:
            repaint = True
        else:
            gate = police <= -6 or draw() % (police + 7) == 0
            if gate and not built & 2 and assimilation < 45 and bases > 1:
                skip = pop < 4 and draw() % 3 != 0
                if not skip:
                    for _ in range(pop + difficulty + rank + 9):
                        facility = draw() % 39
                        attempts += 1
                        if 1 < facility < 34 and built >> facility & 1 and not protected >> facility & 1:
                            built &= ~(1 << facility)
                            destroyed = facility
                            notices.append(7)
                            repaint = True
                            break
    assert len(notices) <= 2
    result = [0, seed, flags, draws, attempts, built & 0xffffffff, built >> 32, destroyed, len(notices),
              *notices, *([0] * (3 - len(notices))), int(golden), int(bool(flags & 2)), int(flags != original or destroyed >= 0), int(repaint)]
    return [x & 0xffffffff for x in result]


def leaf_cases():
    rows = []
    # State/notice transitions and the exact population rounding boundary.
    for pop in (0, 1, 2, 3, 4, 5, 126, 127):
        for talents in sorted({0, pop, max(0, (pop + 1) // 2 - 1), (pop + 1) // 2}):
            for flags in (0, 2, 4, 6, 0xabc00006):
                for governor in (0, 0x80000000, 0x40, 0x80000040):
                    rows.append([pop, talents, 0, 0, -6, 0, 2, 5, 7, flags, governor, 0xfffffffc, 0xffffffff, 0, 0, 0, 0, 0xffffffff])
    seed = [6, 0, 5, 2, -6, 0, 2, 5, 7, 2, 0, 0xfffffffc, 0xffffffff, 0, 0, 0, 0, 0]
    for police in (-100, -7, -6, -5, -1, 0, 3, 100):
        for state in (0, 2, 4, 6):
            for random_seed in (0, 1, 2, 0x7fffffff, 0x80000000, 0xffffffff):
                row = seed.copy()
                row[4], row[9], row[17] = police, state, random_seed
                rows.append(row)
    for index, value in ((5, 44), (5, 45), (5, 255), (6, 1), (6, 512), (15, 1), (16, 1), (11, 2), (12, 1 << 2), (13, 0xfffffffc)):
        row = seed.copy()
        row[index] = value
        rows.append(row)
    # No eligible built facilities, fully protected facilities, repeatable and
    # project bits, protected HQ, and off-by-one exhaustion.
    for built, protected in [(0, 0), (1 << 34, 0), (1 << 38, 0), (1 << 39, 0), (1 << 33, 0),
                             (1 << 6, 1 << 6), (0xfffffffffffffffc, 0xffffffffffffffff), (2, 0)]:
        row = seed.copy()
        row[11:15] = [built & 0xffffffff, built >> 32, protected & 0xffffffff, protected >> 32]
        rows.append(row)
    for pop in (1, 2, 3):
        for random_seed in range(12):
            row = seed.copy()
            row[:4] = [pop, 0, pop, 0]
            row[17] = random_seed
            rows.append(row)
    # Carry the same game CRT stream through successive ongoing-riot events.
    carry = seed.copy()
    carry[13], carry[14] = 0xffffffff, 0xffffffff
    for _ in range(128):
        rows.append(carry.copy())
        carry[17] = model(carry)[1]
    rng = random.Random(0x28652792)
    for _ in range(10000):
        pop = rng.randrange(128)
        talents = rng.randrange(pop + 1)
        drones = rng.randrange(pop - talents + 1)
        rows.append([pop, talents, drones, rng.randrange(drones + 1), rng.randrange(-100, 101), rng.randrange(256), rng.randrange(1, 513),
                     rng.randrange(6), rng.randrange(8), rng.getrandbits(32), rng.getrandbits(32), *[rng.getrandbits(32) for _ in range(4)],
                     rng.randrange(5) == 0, rng.randrange(5) == 0, rng.getrandbits(32)])
    for index, value in ((0, -1), (0, 128), (1, -1), (1, 7), (2, -1), (2, 7), (3, 6), (4, -101), (4, 101),
                         (5, -1), (5, 256), (6, 0), (6, 513), (7, -1), (7, 6), (8, -1), (8, 8), (8, 2147483647), (4, -2147483648)):
        row = seed.copy()
        row[index] = value
        rows.append(row)
    # A genuine count inconsistency, not an invalid individual bound.
    row = seed.copy()
    row[1], row[2] = 3, 4
    rows.append(row)
    return rows


def fixture(case, *, flags=None, nerve=False, matrix=False, hq=False, assimilation=0, free_known=True, version=9):
    data = bytearray(psych.fixture(7, version))
    info, off, spans = psych.intake.decode(data)
    legacy = version < 11
    fs, bs = (0x20bc, 304) if legacy else (0x20cc, 308)
    schema = psych.intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction']
    def put(schema, start, name, value, index=0):
        field = next(f for f in schema['fields'] if f['name'] == name)
        fmt = psych.intake.FORMATS[field['type']]
        struct.pack_into('<' + fmt, data, start + field['offset'] + index * struct.calcsize(fmt), value)
    for name, value in [('SE_talent_pending', 0), ('SE_police_pending', -6 if case % 3 else 0), ('SE_effic_pending', 0),
                        ('SE_support_pending', 0), ('SE_Economics_pending', 0), ('diff_level', 5), ('ranking', case % 8), ('base_count', -123456789)]:
        put(schema, off['factions'] + fs, name, value)
    definition = psych.intake.FACTIONS['MFaction']
    for name, value in [('rule_drone', 0), ('rule_talent', 0), ('faction_bonus_count', 2), ('rule_flags', 0)]:
        put(definition, off['faction_definitions'] + 0x59c, name, value)
    for i, (kind, facility) in enumerate(((2, 2), (12, 3))):
        put(definition, off['faction_definitions'] + 0x59c, 'faction_bonus_id', kind, i)
        put(definition, off['faction_definitions'] + 0x59c, 'faction_bonus_val1', facility, i)
    pref = off['legacy'] if legacy else off['preferences']
    def pset(name, value, index=0):
        f = next(f for f in psych.intake.SCHEMA['GamePrefs']['fields'] if f['name'] == name)
        fmt = psych.intake.FORMATS[f['type']]
        at = pref + f['offset'] - (0x7c if legacy and f['offset'] >= 0x184 else 0) + index * struct.calcsize(fmt)
        struct.pack_into('<' + fmt, data, at, value)
    pset('DiffLevel', 5)
    for i in range(33):
        pset('SecretProjects', 0 if matrix and i == 30 else -1, i)
    # Tech ownership, not the loader's overridden free-tech metadata, controls
    # RULE_FREEFAC protection. RULE_FACILITY remains unconditional.
    ownership = pref + 0x1e0 - (0x7c if legacy else 0)
    for i in range(4):
        data[ownership + i] = 2 if free_known else 0
    bp = off['bases']
    pop = 1 + case % 8
    data[bp + 6], data[bp + 7], data[bp + 8] = pop, assimilation, int(nerve)
    data[bp + bs + 4] = 1
    struct.pack_into('<I', data, bp + 0x30, flags if flags is not None else [0, 2, 4, 6][case % 4])
    struct.pack_into('<I', data, bp + 0x38, 0x80000040 if case & 1 else 0)
    built = sum(1 << f for f in (2, 3, 4, 5, 9, 10, 15, 16, 19, 20, 21, 24, 25, 27, 28, 29, 31, 32, 33, 34, 38, 39)) | (2 if hq else 0)
    data[bp + 0x8c:bp + 0x8c + (8 if legacy else 12)] = built.to_bytes(12, 'little')[:8 if legacy else 12]
    for delta in (0x118, 0x11c, 0x120):
        struct.pack_into('<i', data, bp + delta - (4 if legacy else 0), -987654321)
    return bytes(data)


def world_model(text, original, base, seed, energy, specialists, mask, types=(0, 0)):
    data = minerals.normalized(original, base)
    info, off, spans = psych.intake.decode(data)
    legacy = int(info['version']) < 11
    b = spans['bases'][:304 if legacy else 308]
    pop = struct.unpack_from('<b', b, 6)[0]
    fail = lambda e: f'WORLD {e} {seed}\n'.encode()
    if not 1 <= pop <= 127 or not 0 <= specialists <= pop or not mask & 1 or mask & ~0x1fffff or energy < 0 or mask.bit_count() + specialists != pop + 1:
        return fail(1)
    for i in range(min(specialists, 16)):
        if types[i // 8] >> (4 * (i % 8)) & 15 > 6:
            return fail(1)
    if not legacy:
        return fail(3)
    computed = psych.model(text, data, specialists, energy)
    if computed['BASE'][0]:
        return fail({1: 1, 2: 2, 3: 3, 4: 5, 5: 4}[computed['BASE'][0]])
    freshfacts = computed['FACTS'].copy()
    freshfacts[4] = base
    fresh = psych.leaf.expected(freshfacts)
    assert fresh[0] == 0
    talents, drones, superdrones = fresh[1:4]
    faction = b[4]
    schema = psych.intake.FACTIONS['LegacyFactionV10']
    f = spans['factions'][faction * 0x20bc:(faction + 1) * 0x20bc]
    state = lambda name: psych.intake.scalar(f, schema, name)
    definition = spans['faction_definitions'][faction * 0x59c:(faction + 1) * 0x59c]
    df = psych.intake.FACTIONS['MFaction']
    rule = lambda name, i=0: psych.intake.scalar(definition, df, name, i)
    prefs = spans['legacy']
    project = struct.unpack_from('<i', prefs, 0x84 + 30 * 4)[0]
    matrix = int(project >= 0 and spans['bases'][project * 304 + 4] == faction)
    tables = psych.intake.sections(text)
    tech = tables['TECHNOLOGY']
    keys = {r[1].lower(): i for i, r in enumerate(tech)}
    keys.update(none=-1, disable=-2)
    def known(token):
        tid = keys[token.lower()]
        if tid == -1:
            return True
        if not 0 <= tid < 88:
            return False
        a, bb = (keys[tech[tid][j].lower()] for j in (6, 7))
        return a != -2 and not (bb == -2 and a != -1) and bool(prefs[0x1e0 - 0x7c + tid] & (1 << faction))
    protected = 0
    for facility in range(2, 34):
        for i in range(rule('faction_bonus_count')):
            if rule('faction_bonus_val1', i) == facility and (rule('faction_bonus_id', i) == 2 or rule('faction_bonus_id', i) == 12 and known(tables['FACILITIES'][facility - 1][3])):
                protected |= 1 << facility
    built = int.from_bytes(b[0x8c:0x94], 'little')
    facts = [pop, talents, drones, superdrones, state('SE_police_pending'), b[7], freshfacts[5], state('diff_level'), state('ranking'),
             struct.unpack_from('<I', b, 0x30)[0], struct.unpack_from('<I', b, 0x38)[0], built & 0xffffffff, built >> 32,
             protected & 0xffffffff, protected >> 32, int(bool(b[8])), matrix, seed]
    result = model(facts)
    if result[0]:
        return fail(result[0])
    return (f'WORLD 0 {result[1]}\nFACTS ' + ' '.join(str(x & 0xffffffff) for x in facts) + '\nTRANS ' + ' '.join(map(str, result)) + '\n').encode()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--cc', default='g++')
    p.add_argument('--binary', type=Path)
    p.add_argument('--original', type=Path)
    p.add_argument('--original-rules', type=Path)
    args = p.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.binary or args.out / 'drone-transition-probe'
    command = [args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
               *[str(HERE / (s + '.cpp')) for s in SOURCES], '-o', str(exe)]
    if not args.binary:
        subprocess.run(command, check=True)
    rows = leaf_cases()
    payload = b''.join(struct.pack('<18I', *[int(x) & 0xffffffff for x in r]) for r in rows)
    want = b''.join(struct.pack('<16I', *model(r)) for r in rows)
    src, dest = args.out / 'leaf-input.bin', args.out / 'leaf-output.bin'
    src.write_bytes(payload)
    run = subprocess.run([str(exe), 'leaf', str(src), str(dest)], capture_output=True, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
    assert run.returncode == 0, (run.returncode, run.stdout, run.stderr)
    got = dest.read_bytes()
    for i, r in enumerate(rows):
        assert got[i*64:(i+1)*64] == want[i*64:(i+1)*64], (i, r, struct.unpack('<16I', got[i*64:(i+1)*64]), model(r))
        values = model(r)
        if not values[0]:
            assert not (values[2] ^ r[9]) & ~6
            before_built, after_built = r[11] | r[12] << 32, values[5] | values[6] << 32
            assert (before_built ^ after_built).bit_count() <= 1 and not after_built & ~before_built
            if values[7] != 0xffffffff:
                facility = values[7]
                assert 1 < facility < 34 and before_built >> facility & 1
                assert not (r[13] | r[14] << 32) >> facility & 1
    (args.out / 'leaf-expected.bin').write_bytes(want)
    # Independent branch assertions, beyond equality with the reference model.
    first = [6, 0, 5, 0, -6, 0, 2, 5, 7, 0, 0, 0xfffffffc, 0xffffffff, 0, 0, 0, 0, 0]
    assert model(first)[3:5] == [0, 0] and model(first)[7] == 0xffffffff
    exhausted = first.copy()
    exhausted[9], exhausted[13], exhausted[14] = 2, 0xffffffff, 0xffffffff
    assert model(exhausted)[3:5] == [27, 27]
    hq = first.copy()
    hq[9], hq[4], hq[11], hq[17] = 2, 0, 2, 12
    assert model(hq)[3:5] == [1, 0]
    low = first.copy()
    low[:4], low[9] = [3, 0, 3, 0], 2
    assert model(low)[3:5] == [1, 0]
    text = maintenance.rules('base')
    cases = []
    for case in range(1, 33):
        data = fixture(case, version=7 + case % 4)
        pop = 1 + case % 8
        count = case % (pop + 1)
        mask = (1 << (pop - count + 1)) - 1
        cases.append((f'world-{case}', text, data, 0, (case * 0x12345679) & 0xffffffff, [0, 2, 20, 1000][case % 4], count, mask, (0, 0)))
    for name, kwargs, energy in [('golden-start', {'flags': 2}, 1000), ('golden-end', {'flags': 4}, 0), ('riot-start', {'flags': 0}, 0),
                                 ('riot-ongoing', {'flags': 2}, 0), ('nerve-end', {'flags': 2, 'nerve': True}, 0),
                                 ('matrix-end', {'flags': 2, 'matrix': True}, 0), ('hq-auto-gate', {'flags': 2, 'hq': True}, 0),
                                 ('recent-capture', {'flags': 2, 'assimilation': 45}, 0), ('freefac-not-known', {'flags': 2, 'free_known': False}, 0)]:
        cases.append((name, text, fixture(5, **kwargs), 0, 0xffffffff, energy, 0, 127, (0, 0)))
    for name, flags in [('quiet-golden-ended', 4), ('managed-riot-ended', 2), ('golden-and-riot-ended', 6)]:
        cases.append((name, text, fixture(1, flags=flags), 0, 123, 2, 0, 7, (0, 0)))
    overflow = bytearray(fixture(5))
    _, overflow_offsets, _ = psych.intake.decode(overflow)
    talent = next(f for f in psych.intake.FACTIONS['MFaction']['fields'] if f['name'] == 'rule_talent')
    struct.pack_into('<i', overflow, overflow_offsets['faction_definitions'] + 0x59c + talent['offset'], 2147483647)
    cases.append(('late-psych-overflow', text, bytes(overflow), 0, 123, 0, 0, 127, (0, 0)))
    seed = fixture(5)
    cases.extend((name, text, seed, 0, 123, energy, count, mask, types) for name, energy, count, mask, types in [
        ('negative-psych', -1, 0, 127, (0, 0)), ('bad-specialists', 0, 7, 1, (0, 0)), ('missing-centre', 0, 0, 126, (0, 0)),
        ('outside-radius', 0, 0, 0x3fffff, (0, 0)), ('bad-specialist-type', 0, 1, 63, (7, 0))])
    cases.append(('SMAX-rejected', text, fixture(5, version=12), 0, 1, 0, 0, 127, (0, 0)))
    if args.original:
        assert args.original_rules
        data = args.original.read_bytes()
        info, _, spans = psych.intake.decode(data)
        stride = 304 if int(info['version']) < 11 else 308
        for i in range(int(info['bases'])):
            b = spans['bases'][i*stride:(i+1)*stride]
            if b[4] == int(info['player']):
                count = struct.unpack_from('<i', b, 0x7c)[0]
                mask = struct.unpack_from('<I', b, 0x78)[0]
                types = struct.unpack_from('<2I', b, 0x84)
                for energy in (0, 1000):
                    cases.append((f'actual-base-{i}-psych-{energy}', args.original_rules.read_bytes(), data, i, 0xffffffff, energy, count, mask, types))
    records = []
    digest = hashlib.sha256()
    for i, (name, rules, data, base, random_seed, energy, count, mask, types) in enumerate(cases):
        rp, sp, out = (args.out / f'{i}{ext}' for ext in ('.txt', '.SC', '.out'))
        rp.write_bytes(rules)
        sp.write_bytes(data)
        expected = world_model(rules, data, base, random_seed, energy, count, mask, types)
        argv = [str(exe), 'world', str(rp), str(sp), str(base), str(random_seed), str(energy), str(count), str(mask), str(types[0]), str(types[1]), str(out)]
        run = subprocess.run(argv, capture_output=True, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        assert run.returncode == 0, (name, run.returncode, run.stdout, run.stderr)
        assert out.read_bytes() == expected, (name, out.read_bytes(), expected)
        assert sp.read_bytes() == data and rp.read_bytes() == rules
        (args.out / f'{i}.expected').write_bytes(expected)
        digest.update(expected)
        records.append(dict(index=i, name=name, base=base, seed=random_seed, psych=energy, specialists=count, mask=mask, types=list(types),
                            expectedLog=expected.decode(), outputSHA256=hashlib.sha256(expected).hexdigest()))
    receipt = dict(leafCases=len(rows), worldCases=len(records), leafInputSHA256=hashlib.sha256(payload).hexdigest(),
                   leafOutputSHA256=hashlib.sha256(want).hexdigest(), leafOutputBytes=len(want), worldOutputSHA256=digest.hexdigest(),
                   binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(), records=records, command=command if not args.binary else None,
                   scope='Thinker default human base-game riot/GoldenAge policy; %39 base ordinal adaptation; separate CRT; no original revolt claim.')
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({k: v for k, v in receipt.items() if k not in ('records', 'command')}))


if __name__ == '__main__':
    main()
