#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile and check the native technology reader; original assets are optional."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess


def reference(data):
    rows = []
    inside = False
    for line in data.decode('cp1252').splitlines():
        line = line.split(';', 1)[0].strip()
        if line.upper() == '#TECHNOLOGY':
            inside = True
            continue
        if not inside:
            continue
        if line.startswith('#'):
            break
        if line:
            rows.append([part.strip() for part in line.split(',')])
    assert len(rows) == 89
    keys = {row[1].lower(): i for i, row in enumerate(rows)}
    digest = 2166136261
    active = edges = 0
    def number(s):
        if s.lower().startswith('0d'):
            return int(s[2:], 10)
        if s.lower().startswith(('0x', '0b')):
            return int(s, 0)
        return int(s, 10)
    for row in rows:
        prerequisites = [-1 if key.lower() == 'none' else -2 if key.lower() == 'disable'
                         else keys[key.lower()] for key in row[6:8]]
        if -2 not in prerequisites:
            active += 1
            edges += sum(key >= 0 for key in prerequisites)
        canonical = (row[0].encode('cp1252') + b'\0' + row[1].encode('cp1252') + b'\0'
                     + struct.pack('<6iI', *map(number, row[2:6]), *prerequisites, int(row[8], 2)))
        for byte in canonical:
            digest = ((digest ^ byte) * 16777619) & 0xffffffff
    return dict(rows=89, active=active, edges=edges, digest=f'{digest:08x}')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--cc', default='c++')
    ap.add_argument('--m68k-linux', action='store_true')
    ap.add_argument('--data', action='append', type=Path, default=[])
    args = ap.parse_args()
    source = Path(__file__).resolve().parent
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    executable = out / 'rules-probe'
    flags = ['-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror']
    runner = []
    if args.m68k_linux:
        flags += ['-m68040', '-static']
        runner = ['qemu-m68k', '-cpu', 'm68040']
    else:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    command = [args.cc, *flags, str(source/'rules_text.cpp'), str(source/'rules_probe.cpp'),
               '-o', str(executable)]
    subprocess.run(command, check=True)
    # LeakSanitizer cannot run under the local sandbox's ptrace. Memory access
    # checks and UBSan remain enabled; no leak-check claim is made.
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
    results = []
    def check(name, data, error=None):
        path = out/(name+'.txt')
        path.write_bytes(data)
        result = subprocess.run([*runner, str(executable), str(path)], env=env,
                                text=True, capture_output=True, timeout=20)
        if error:
            assert result.returncode == 20 and error in result.stderr, (name, result.returncode, result.stderr)
            results.append(dict(case=name, passed=True, expected_error=error))
        else:
            assert result.returncode == 0, (name, result.returncode, result.stderr)
            expected = reference(data)
            actual = dict(value.split('=') for value in result.stdout.split())
            assert actual == {k: str(v) for k, v in expected.items()}, (name, actual, expected)
            results.append(dict(case=name, passed=True, **expected))
    fixture = ('#TECHNOLOGY\n' + '\n'.join(
        f'Test {i},T{i},0,1,2,3,{"T88" if i == 0 else "None"},None,000000001'
        for i in range(89)) + '\n#CHASSIS\n').encode()
    for name, data in [
        ('lf', fixture), ('crlf', fixture.replace(b'\n', b'\r\n')),
        ('cr', fixture.replace(b'\n', b'\r')), ('no-final-newline', fixture.rstrip(b'\n')),
        ('lower-header', fixture.replace(b'#TECHNOLOGY', b'#technology')),
        ('blank-comments', fixture.replace(b'#TECHNOLOGY\n', b'#TECHNOLOGY\n\n; comment\n')),
        ('numeric-prefixes', fixture.replace(b',0,1,2,3,', b',0b0,0x1,0d2,03,', 1)),
        ('disabled', fixture.replace(b'Test 0,T0,0,1,2,3,T88,None',
                                    b'Test 0,T0,0,1,2,3,Disable,None')),
        ('signed-limits', fixture.replace(b',0,1,2,3,', b',-2147483648,2147483647,+2,-3,', 1)),
    ]:
        check(name, data)
    for name, data, error in [
        ('duplicate', fixture.replace(b'Test 1,T1,', b'Test 1,T0,'), 'duplicate key'),
        ('unknown', fixture.replace(b'T88,None', b'Missing,None', 1), 'unknown prerequisite'),
        ('overflow', fixture.replace(b'Test 0,T0,0,', b'Test 0,T0,2147483648,'), 'bad number'),
        ('bad-binary', fixture.replace(b'000000001', b'000000002', 1), 'bad number'),
        ('missing', fixture.replace(b'#TECHNOLOGY', b'#OTHER'), 'missing section'),
        ('long-line', b';'+b'x'*600+b'\n'+fixture, 'line too long'),
        ('empty', b'', 'empty file'),
        ('truncated', fixture[:fixture.index(b'Test 88')], 'wrong technology count'),
        ('hidden-field', fixture.replace(b'000000001', b'000000001,,unexpected', 1), 'bad row'),
        ('extra-field', fixture.replace(b'000000001', b'000000001,unexpected', 1), 'bad row'),
        ('nul', fixture.replace(b'Test 0', b'Test\x000', 1), 'bad row'),
        ('too-many', fixture.replace(b'#CHASSIS', b'Test 89,T89,0,1,2,3,None,None,000000001\n#CHASSIS'), 'wrong technology count'),
        ('long-key', fixture.replace(b'Test 0,T0,', b'Test 0,TooLongKey,'), 'bad key'),
    ]:
        check(name, data, error)
    for i, path in enumerate(args.data):
        check('owner-data-'+str(i), path.read_bytes())
    receipt = dict(command=command, runner=runner, cases=results,
                   executable_sha256=hashlib.file_digest(executable.open('rb'), 'sha256').hexdigest(),
                   scope='Technology parser control; no complete engine, AmigaOS runtime or original-executable fidelity claim')
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
