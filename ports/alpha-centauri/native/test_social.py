#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""SOCIO/BONUSNAMES controls, with optional privately supplied original data."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def section(data, name):
    rows = []
    inside = False
    for line in data.decode('cp1252').splitlines():
        line = line.split(';', 1)[0].strip()
        if line.upper() == '#'+name:
            inside = True
            continue
        if not inside:
            continue
        if line.startswith('#'):
            break
        if line:
            tokens = [v.strip() for v in line.split(',')]
            if tokens[-1] == '':
                tokens.pop()  # one valid terminal separator
            assert all(tokens)
            rows.append(tokens)
    return rows


def reference(data, variant):
    technology = section(data, 'TECHNOLOGY')
    tech = {row[1].lower(): i for i, row in enumerate(technology)}
    tech.update(none=-1, disable=-2)
    rows = section(data, 'SOCIO')
    assert len(rows) == 19
    effects, labels, categories = rows[:3]
    assert len(effects) == len(labels) == 11 and len(categories) == 4
    result = {}
    for i, (key, label) in enumerate(zip(effects, labels)):
        result[f'effect.{i}.key'] = key
        result[f'effect.{i}.label'] = label
    for i, category in enumerate(categories):
        result[f'category.{i}'] = category
    for i, row in enumerate(rows[3:]):
        c, m = divmod(i, 4)
        result[f'model.{c}.{m}.name'] = row[0]
        result[f'model.{c}.{m}.tech'] = str(tech[row[1].lower()])
        modifiers = dict.fromkeys((k.lower() for k in effects), 0)
        for token in row[2:]:
            key = token.lstrip('+-')
            signs = token[:len(token)-len(key)]
            assert key.lower() in modifiers
            modifiers[key.lower()] = signs.count('+')-signs.count('-')
        for e, key in enumerate(effects):
            result[f'model.{c}.{m}.effect.{e}'] = str(modifiers[key.lower()])
    bonus_rows = section(data, 'BONUSNAMES')
    count = 29 if variant == 'base' else 41
    bonus = [key for row in bonus_rows for key in row]
    assert len(bonus) == count and all(len(row) == 8 for row in bonus_rows[:-1])
    result['bonus.count'] = str(count)
    for i, key in enumerate(bonus):
        result[f'bonus.{i}'] = key
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--cc', default='c++')
    ap.add_argument('--m68k-linux', action='store_true')
    ap.add_argument('--data', action='append', default=[], metavar='base|expansion=PATH')
    args = ap.parse_args()
    source = Path(__file__).resolve().parent
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    executable = out/'social-probe'
    flags = ['-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror']
    runner = []
    if args.m68k_linux:
        flags += ['-m68040', '-static']
        runner = ['qemu-m68k', '-cpu', 'm68040']
    else:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    command = [args.cc, *flags, str(source/'rules_text.cpp'), str(source/'social_probe.cpp'), '-o', str(executable)]
    subprocess.run(command, check=True)
    tech = '#TECHNOLOGY\n'+'\n'.join(f'Test {i},T{i},0,1,2,3,None,None,1' for i in range(89))+'\n'
    rows = [[f'E{i}' for i in range(11)], [f'Label{i}' for i in range(11)], [f'Category{i}' for i in range(4)]]
    rows += [[f'Model{c}{m}', 'T88' if m else 'None', '++E0', '---E1', '+-E2', 'E3', '+E0']
             for c in range(4) for m in range(4)]
    def material(variant='expansion', social=None, bonus=None):
        if social is None:
            social = rows
        if bonus is None:
            keys = [f'BONUS{i}' for i in range(29 if variant == 'base' else 41)]
            bonus = [keys[i:i+8] for i in range(0, len(keys), 8)]
        return (tech+'#SOCIO\n'+'\n'.join(','.join(row)+',' for row in social)+
                '\n#BONUSNAMES\n'+'\n'.join(','.join(row)+',' for row in bonus)+'\n#NEXT\n').encode()
    results = []
    def check(name, data, variant='expansion', error=None):
        path = out/(name+'.txt')
        path.write_bytes(data)
        proc = subprocess.run([*runner, str(executable), variant, str(path)], capture_output=True,
                              text=True, timeout=20, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        if error:
            assert proc.returncode == 20 and error in proc.stderr, (name, proc.returncode, proc.stderr)
        else:
            assert proc.returncode == 0, (name, proc.returncode, proc.stderr)
            actual = dict(line.split('=', 1) for line in proc.stdout.splitlines())
            expected = reference(data, variant)
            assert actual == expected, (name, actual, expected)
        results.append(dict(case=name, passed=True, expected_error=error))
    for variant in ['base', 'expansion']:
        data = material(variant)
        check(variant, data, variant)
        check(variant+'-crlf', data.replace(b'\n', b'\r\n'), variant)
        check(variant+'-cr', data.replace(b'\n', b'\r'), variant)
        check(variant+'-comments', data.replace(b'#SOCIO\n', b'#socio\n; comment\n\n'), variant)
        check(variant+'-no-final-newline', data.rstrip(b'\n'), variant)
    data = material()
    known = reference(data, 'expansion')
    assert known['model.0.0.effect.0'] == '1'  # repeated modifier: last wins
    assert known['model.0.0.effect.1'] == '-3'
    assert known['model.0.0.effect.2'] == known['model.0.0.effect.3'] == '0'
    for i in range(19):
        check(f'truncate-{i}', material(social=rows[:i]), error='bad row')
    for i, key in enumerate(['E0', 'Category0', 'Model00']):
        old = [b'E1,', b'Category1,', b'Model01,'][i]
        check('duplicate-'+str(i), data.replace(old, key.encode()+b',', 1), error='duplicate key')
    for name, changed, error in [
        ('unknown-effect', data.replace(b'++E0', b'++Absent', 1), 'bad key'),
        ('missing-model-tech', data.replace(b'Model00,None,', b'Model00,,', 1), 'bad key'),
        ('unknown-tech', data.replace(b'Model00,None,', b'Model00,Missing,', 1), 'unknown prerequisite'),
        ('empty-interior', data.replace(b'++E0,---E1', b'++E0,,---E1', 1), 'bad key'),
        ('extra-effect', data.replace(b'E10,\n', b'E10,,hidden\n', 1), 'bad row'),
        ('extra-bonus', data.replace(b'BONUS7,\n', b'BONUS7,,hidden\n', 1), 'bad row'),
        ('duplicate-bonus', data.replace(b'BONUS1,', b'BONUS0,', 1), 'duplicate key'),
        ('long-effect', data.replace(b'E0,', b'X'*24+b',', 1), 'bad key'),
        ('long-line', data.replace(b'#SOCIO\n', b'#SOCIO\n'+b'X'*600+b'\n'), 'line too long'),
        ('nul', data.replace(b'#SOCIO\n', b'#SOCIO\n\x00\n'), 'bad row'),
        ('extra-model', data.replace(b'#BONUSNAMES', b'Extra,None\n#BONUSNAMES'), 'bad row'),
        ('extra-bonus-row', data.replace(b'#NEXT', b'EXTRA\n#NEXT'), 'bad row'),
    ]:
        check(name, changed, error=error)
    for name in ['SOCIO', 'BONUSNAMES']:
        check('missing-'+name, data.replace(('#'+name).encode(), b'#OTHER'), error='missing section')
    check('wrong-base', data, 'base', error='bad row')
    check('wrong-expansion', material('base'), error='bad key')
    check('case-fold', data.replace(b'++E0', b'++e0').replace(b',T88,', b',t88,'))
    check('disabled-tech', data.replace(b'Model00,None,', b'Model00,Disable,', 1))
    for i, item in enumerate(args.data):
        variant, path = item.split('=', 1)
        assert variant in ['base', 'expansion']
        check('owner-data-'+str(i), Path(path).read_bytes(), variant)
    receipt = dict(command=command, runner=runner, cases=results,
                   executable_sha256=hashlib.file_digest(executable.open('rb'), 'sha256').hexdigest(),
                   scope='Social tables/bonus-name lookups only; not faction application or gameplay')
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(f'{len(results)} social/bonus cases passed')


if __name__ == '__main__':
    main()
