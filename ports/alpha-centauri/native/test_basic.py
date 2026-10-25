#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded RULES/WORLDBUILDER controls; original files are local-only optional inputs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--cc', default='c++')
    ap.add_argument('--m68k-linux', action='store_true')
    args = ap.parse_args()
    source = Path(__file__).resolve().parent
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    fixture = json.loads((source/'basic_fixture.json').read_text())
    executable = out/'basic-probe'
    flags = ['-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror']
    runner = []
    if args.m68k_linux:
        flags += ['-m68040', '-static']
        runner = ['qemu-m68k', '-cpu', 'm68040']
    else:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    command = [args.cc, *flags, str(source/'rules_text.cpp'), str(source/'basic_probe.cpp'), '-o', str(executable)]
    subprocess.run(command, check=True)
    tech = '#TECHNOLOGY\n' + '\n'.join(
        f'Test {i},T{i},0,1,2,3,None,None,1' for i in range(89)) + '\n'
    results = []
    def material(variant, rules=None, world=None):
        if rules is None:
            rules = fixture['rules_rows'] if variant == 'expansion' else fixture['rules_rows'][:-2]
        if world is None:
            world = fixture['world_rows']
        return (tech+'#RULES\n'+'\n'.join(','.join(row) for row in rules)+
                '\n#WORLDBUILDER\n'+'\n'.join(','.join(row) for row in world)+'\n#NEXT\n').encode()
    def expected(variant):
        values = fixture['expected'].copy()
        if variant == 'base':
            values['basic.size_base_subspace_gen'] = 0
            values['basic.subspace_generators_needed'] = 0
        return values
    def check(name, data, variant='expansion', values=None, error=None):
        path = out/(name+'.txt')
        path.write_bytes(data)
        proc = subprocess.run([*runner, str(executable), variant, str(path)], text=True,
                              capture_output=True, timeout=20,
                              env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        if error:
            assert proc.returncode == 20 and error in proc.stderr, (name, proc.returncode, proc.stderr)
        else:
            assert proc.returncode == 0, (name, proc.returncode, proc.stderr)
            actual = {key: int(value) for key, value in (line.split('=') for line in proc.stdout.splitlines())}
            assert actual == (values if values is not None else expected(variant)), (name, actual, values)
        results.append(dict(case=name, passed=True, expected_error=error))
    for variant in ['base', 'expansion']:
        data = material(variant)
        check(variant, data, variant)
        check(variant+'-crlf', data.replace(b'\n', b'\r\n'), variant)
        check(variant+'-comments', data.replace(b'#RULES\n', b'#rules\n; comment\n\n'), variant)
    data = material('expansion')
    check('base-rejects-expansion', data, 'base', error='bad row')
    check('expansion-rejects-base', material('base'), error='bad row')
    for marker in [b'#RULES', b'#WORLDBUILDER']:
        check('missing-'+marker.decode()[1:], data.replace(marker, b'#OTHER'), error='missing section')
    # Truncate at every required row boundary, including inside grouped pairs.
    for section, key in [('rules', 'rules_rows'), ('world', 'world_rows')]:
        rows = fixture[key]
        for index, row in enumerate(rows):
            kwargs = {section: rows[:index]}
            check(f'{section}-truncated-{index}', material('expansion', **kwargs), error='bad row')
            if len(row) > 1:
                short = [r[:] for r in rows]
                short[index] = row[:-1]
                check(f'{section}-short-group-{index}', material('expansion', **{section: short}), error='bad number')
        for suffix in ['12garbage', '2147483648', '', '0x', '1,2', '1,,hidden']:
            changed = [r[:] for r in rows]
            changed[0] = [suffix]
            check(section+'-bad-'+str(len(results)), material('expansion', **{section: changed}),
                  error='bad row' if ',' in suffix or not suffix else 'bad number')
        check(section+'-extra-row', material('expansion', **{section: rows+[['7']]}), error='bad row')
    for replacement, number in [('none', -1), ('DISABLE', -2), ('t1', 1)]:
        values = expected('expansion')
        for key in values:
            if key.startswith('basic.tech_') and key != 'basic.tech_discovery_rate_pct_std':
                values[key] = number
        check('technology-'+replacement, data.replace(b'T88\n', (replacement+'\n').encode()), values=values)
    check('unknown-tech', data.replace(b'\nT88\n', b'\nMissing\n', 1), error='unknown prerequisite')
    check('long-tech', data.replace(b'\nT88\n', b'\nTooLongKey\n', 1), error='bad key')
    check('embedded-nul', data.replace(b'#RULES\n', b'#RULES\n\x00\n'), error='bad row')
    check('overlong-rule', data.replace(b'#RULES\n', b'#RULES\n'+b'8'*600+b'\n'), error='line too long')
    check('invalid-variant', data, 'guess', error='usage:')
    receipt = dict(command=command, runner=runner, cases=results,
                   executable_sha256=hashlib.file_digest(executable.open('rb'), 'sha256').hexdigest(),
                   scope='Basic rules and world-generation parameter parser, not map generation or gameplay')
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(f'{len(results)} basic/world cases passed')


if __name__ == '__main__':
    main()
