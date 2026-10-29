#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exact world oracle plus durable once-per-base ledger and malformed resume."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import zlib
import test_fresh_facility_materials as fresh

HERE = Path(__file__).resolve().parent
SOURCES = [p for p in fresh.SOURCES if p != 'fresh_facility_materials_probe.cpp'] + ['facility_upkeep_session.cpp', 'facility_upkeep_session_probe.cpp']


def seal(data):
    data = bytearray(data)
    struct.pack_into('<I', data, 24, zlib.crc32(data[256:]))
    data[28:32] = bytes(4)
    struct.pack_into('<I', data, 28, zlib.crc32(data))
    return bytes(data)


def expected(original, rules, base, stage=0):
    error, payload = fresh.model(original, rules, base, 2 if stage else 0)
    b = bytearray(256) + payload
    b[:8] = b'ACFUPK01'
    info, _, _ = fresh.intake.decode(original)
    struct.pack_into('<6I', b, 8, 256, len(payload), 0xabcdef01, zlib.crc32(rules), 0, 0)
    struct.pack_into('<3I', b, 32, int(info['bases']), int(info['player']), 1)
    complete = 0
    if error == 0:
        b[64 + base // 8] |= 1 << (base % 8)
        _, off, _ = fresh.intake.decode(payload)
        stride = 304 if int(info['version']) < 11 else 308
        complete = bool(struct.unpack_from('<I', payload, off['bases'] + base * stride + 0x30)[0] & 0x800000)
        if complete:
            for offset in (128, 192):
                b[offset + base // 8] |= 1 << (base % 8)
    return error, seal(b), complete


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--cc', default='g++')
    ap.add_argument('--probe', type=Path)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.probe or args.out / 'upkeep-session-probe'
    command = [args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
               *[str(HERE / p) for p in SOURCES], '-o', str(exe)]
    if not args.probe:
        subprocess.run(command, check=True)
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
    cases = [(f'v{v}-i{i}', fresh.rules(), fresh.fixture(version=v, industry=i), 0, 0) for v in (7, 9, 10) for i in (-8, 0, 2, 5)]
    cases += [('waiting', fresh.rules(), fresh.fixture(accumulated=0), 0, 0),
              ('upkeep1-rejected', fresh.rules(), fresh.fixture(), 0, 1),
              ('bad-workers', fresh.rules(), fresh.fixture(bad_workers=True), 0, 0),
              ('bad-facility', fresh.rules(), fresh.fixture(queue=130), 0, 0),
              ('disabled-tech', fresh.rules(disabled=True), fresh.fixture(), 0, 0)]
    records = []
    def run_case(name, rules, data, base, mode, wanted, log=None):
        index = len(records)
        rp, sp, out = (args.out / f'{index}{suffix}' for suffix in ('.txt', '.SC', '.checkpoint'))
        rp.write_bytes(rules)
        sp.write_bytes(data)
        run = subprocess.run([str(exe), str(rp), str(sp), str(base), str(mode), str(out)], env=env, capture_output=True, text=True)
        assert run.returncode == 0, (name, run.stdout, run.stderr)
        if wanted is None:
            assert not out.exists(), (name, run.stdout)
        else:
            assert out.read_bytes() == wanted, (name, 'whole checkpoint mismatch', run.stdout)
        if log:
            assert log in run.stdout, (name, run.stdout, log)
        assert sp.read_bytes() == data
        records.append(dict(name=name, log=run.stdout, sha256=hashlib.sha256(wanted).hexdigest() if wanted else None))
    for name, rules, original, base, mode in cases:
        error, want, complete = expected(original, rules, base, mode)
        status = {0: 'ok', 1: 'bad_state', 2: 'bad_buffer', 3: 'overflow', 5: 'not_owner', 6: 'unsupported', 7: 'unavailable'}[error]
        run_case(name, rules, original, base, 4 if not records else mode, want, f'SESSION status={status} ')
        assert bool(want[64] & 1) == (error == 0)
        assert bool(want[128] & 1) == bool(complete)
        if error == 0:
            run_case(name + '-resumed-again', rules, want, base, 2, want, 'SESSION status=already_processed repeat=already_processed')
    rules = fresh.rules()
    _, valid, complete = expected(fresh.fixture(clean=True), rules, 0)
    assert complete
    invalid = []
    for label, offset, value in [('missing-processed', 64, 0), ('choice-mismatch', 192, 0), ('unowned-base', 65, 1),
                                 ('outside-base-count', 127, 128), ('reserved-byte', 44, 1), ('wrong-phase', 40, 2)]:
        b = bytearray(valid)
        b[offset] = value
        invalid.append((label, seal(b)))
    for label, offset in [('wrong-base-count', 32), ('wrong-human', 36)]:
        b = bytearray(valid)
        struct.pack_into('<I', b, offset, 123)
        invalid.append((label, seal(b)))
    _, off, _ = fresh.intake.decode(valid[256:])
    b = bytearray(valid)
    b[256 + off['bases'] + 0x8c] &= ~0x0c
    invalid.append(('completion-without-built-facility', seal(b)))
    for label, data in invalid:
        run_case(label, rules, data, 0, 2, None, 'OPEN 4')
    receipt = dict(cases=len(records), records=records, binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(), command=command if not args.probe else None)
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({k: v for k, v in receipt.items() if k not in ('records', 'command')}))


if __name__ == '__main__':
    main()
