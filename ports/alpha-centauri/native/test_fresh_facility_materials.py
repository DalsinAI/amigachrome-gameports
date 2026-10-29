#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh support/minerals/cost construction versus independent wire oracles."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import test_world_base_minerals as minerals
import test_world_stable_intake as intake
import test_base_support as support
import test_cost_factor as costs
import test_facility_production as production
from test_facility_definitions import tables
from test_units import sections, material
from test_world_facility_options import model as options_model

HERE = Path(__file__).resolve().parent
SOURCES = [p for p in production.SOURCES if p != 'facility_production_probe.cpp'] + ['fresh_facility_materials_probe.cpp']


def rules(multiplier=10, disabled=False):
    rows = sections(intake.rules('base'))
    _, fac = tables('base')
    for n, f in enumerate(fac):
        f[3] = 'Disable' if disabled else 'None'
        if n in (1, 2):
            f[1] = '5' if n == 1 else '4'
    rows['FACILITIES'] = fac
    rows['RULES'][costs.basic_row('mineral_cost_multi')][0] = str(multiplier)
    rows['RULES'][intake.ROW['retool_exemption'][0]][intake.ROW['retool_exemption'][1]] = '10'
    return material(rows)


def fixture(case=1, version=9, industry=2, support_rating=0, accumulated=30, queue=-3, bad_workers=False, tiny=False, clean=False):
    b = bytearray(minerals.fixture('base', version, case))
    info, off, _ = intake.decode(b)
    pf = off['legacy']
    struct.pack_into('<i', b, off['player'], 1)
    struct.pack_into('<i', b, pf + 0x390 - 0x7c, 1)
    schema = intake.FACTIONS['LegacyFactionV10']
    for name, value in [('SE_support_pending', support_rating), ('SE_police_pending', 0), ('SE_industry_pending', industry), ('best_mineral_output', 5)]:
        field = next(f for f in schema['fields'] if f['name'] == name)
        struct.pack_into('<i', b, off['factions'] + 0x20bc + field['offset'], value)
    for i in range(int(info['vehicles'])):
        b[off['vehicles'] + i * 52 + 17] = 0
    b[off['prototypes'] + 42] = 2
    struct.pack_into('<I', b, off['prototypes'] + 32, 0x10000 if clean else 0)
    p = off['bases']
    for delta, value in [(0x30, 0x80800000), (0x38, 0), (0x3c, 20), (0x40, accumulated), (0x4c, 0), (0x50, queue), (0x98, 11),
                         (0xc4, 0xcafebabe), (0xd4, 123456789), (0xf4, 0xdeadbeef)]:
        struct.pack_into('<I', b, p + delta, value & 0xffffffff)
    b[p + 0x8c] &= ~0x0c
    if bad_workers:
        struct.pack_into('<I', b, p + 0x78, 0)
    struct.pack_into('<I', b, off['metadata'] + 0xa30, 0 if tiny else 2)
    return bytes(b)


def model(original, text, base, mode):
    data = bytearray(original)
    info, off, _ = intake.decode(data)
    version = int(info['version'])
    bs = 304 if version < 11 else 308
    p = off['bases'] + base * bs
    if mode == 1:
        for i in range(4):
            struct.pack_into('<h', data, off['vehicles'] + i * 52 + 46, 1)
    current = bytes(data)
    if mode in (2, 3):
        return 6, current
    human = int(info['player'])
    if data[p + 4] != human:
        return 5, current
    queue = struct.unpack_from('<i', data, p + 0x50)[0]
    if queue not in (-2, -3):
        return 6, current
    normalized = minerals.normalized(current, base)
    supported = support.world_expected(normalized, text)
    if supported[0]:
        return {2: 2, 3: 6, 5: 3}.get(supported[0], 1), current
    budget = minerals.model('base', text, current, base, supported[3])
    if budget[1]:
        return {2: 2, 3: 6, 4: 6, 7: 3}.get(budget[1], 1), current
    report = costs.world_report(current, text).decode()
    matched = re.search(r'^FACTOR ' + str(base) + r' 1 (\d+) (\d+)$', report, re.MULTILINE)
    assert matched
    error, factor = map(int, matched.groups())
    if error or not factor:
        return {2: 2, 3: 6, 4: 3}.get(error, 6 if not factor else 1), current
    available = options_model(text, current, 'base', False)
    record = ((human - 1) * (int(info['bases']) + 1) * 3 + (base + 1) * 3) * 158
    if not available[record + 24 - queue]:
        return 7, current
    mineral_intake, consumed, surplus, final, ecology = budget[-5:]
    baseline = struct.unpack_from('<i', data, p + 0x98)[0]
    acc = struct.unpack_from('<i', data, p + 0x40)[0]
    flags = struct.unpack_from('<I', data, p + 0x30)[0]
    fp = off['factions'] + human * 0x20bc
    best = struct.unpack_from('<i', data, fp + 0x3f8)[0]
    out = production.oracle([acc, baseline, mineral_intake, surplus, 5 if queue == -2 else 4, factor, 10, best, flags, 1,
                             int(struct.unpack_from('<i', data, p + 0x3c)[0] < 0), int(bool(flags & 2))])
    if out[0]:
        return out[0], current
    _, acc, baseline, cost, credited, best, flags, complete = out
    struct.pack_into('<i', data, p + 0x40, acc)
    struct.pack_into('<i', data, p + 0x98, baseline)
    struct.pack_into('<I', data, p + 0x30, flags)
    struct.pack_into('<i', data, fp + 0x3f8, best)
    if complete:
        data[p + 0x8c] |= 1 << -queue
    return 0, bytes(data)


def corpus():
    result = []
    for version in (7, 9, 10):
        for case in (0, 1, 3, 5):
            for industry in (-8, -1, 0, 2, 5):
                result.append((f'v{version}-case{case}-industry{industry}', rules(), fixture(case, version, industry), 0, 0))
    for name, kwargs in [('paid', {}), ('free-clean', {'clean': True}), ('high-support-cost', {'support_rating': -4}),
                         ('creche', {'queue': -2}), ('workers-unavailable', {'bad_workers': True})]:
        result.append((name, rules(), fixture(**kwargs), 0, 0))
    result += [('live-rehome', rules(), fixture(), 0, 1), ('upkeep-stage', rules(), fixture(), 0, 2),
               ('editor-stage', rules(), fixture(), 0, 3), ('zero-cost-factor', rules(1), fixture(tiny=True), 0, 0),
               ('missing-tech', rules(disabled=True), fixture(), 0, 0)]
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', required=True, type=Path)
    ap.add_argument('--cc', default='g++')
    ap.add_argument('--probe', type=Path)
    ap.add_argument('--original', type=Path)
    ap.add_argument('--original-rules', type=Path)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.probe or args.out / 'fresh-materials-probe'
    command = [args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
               '-fno-sanitize-recover=all', '-no-pie', *[str(HERE / p) for p in SOURCES], '-o', str(exe)]
    if not args.probe:
        subprocess.run(command, check=True)
    cases = corpus()
    if args.original:
        assert args.original_rules
        for base in (9, 10):
            cases.append((f'actual-Borehole-{base}', args.original_rules.read_bytes(), args.original.read_bytes(), base, 0))
    records = []
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
    for index, (name, text, data, base, mode) in enumerate(cases):
        rp, sp, output = (args.out / f'{index}{ext}' for ext in ('.txt', '.SC', '.out'))
        rp.write_bytes(text)
        sp.write_bytes(data)
        error, want = model(data, text, base, mode)
        run = subprocess.run([str(exe), str(rp), str(sp), str(base), str(mode), str(output)], env=env, capture_output=True, text=True)
        assert run.returncode == 0, (name, run.returncode, run.stdout, run.stderr)
        assert output.read_bytes() == want, (name, 'whole-container mismatch', run.stdout)
        status = {0: 'ok', 1: 'bad_state', 2: 'bad_buffer', 3: 'overflow', 5: 'not_owner', 6: 'unsupported', 7: 'unavailable'}[error]
        assert f'FRESH status={status} ' in run.stdout, (name, run.stdout)
        assert sp.read_bytes() == data
        records.append(dict(index=index, name=name, base=base, mode=mode, status=status,
                            snapshotSHA256=hashlib.sha256(want).hexdigest(), log=run.stdout))
    receipt = dict(scope='Material freshness only; no starvation/riots/turn authority', cases=len(records),
                   records=records, binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(), command=command if not args.probe else None)
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({k: v for k, v in receipt.items() if k not in ('records', 'command')}))


if __name__ == '__main__':
    main()
