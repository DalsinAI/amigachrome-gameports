#!/usr/bin/env python3
"""Validate owner-supplied JJ1 assets with the real, separately built engine probe."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


def cases_for(data):
    names = [p.name for p in data.iterdir() if p.is_file()]
    if len({n.casefold() for n in names}) != len(names):
        raise ValueError('Case-colliding data names')
    return ([('menu', 'MENU.000')]
            + [('scene', n) for n in sorted(names) if re.fullmatch(r'.*\.0SC', n)]
            + [('level', n) for n in sorted(names) if re.fullmatch(r'LEVEL[0-9]\.[0-9]{3}', n)])


def audit_sound_bank(path):
    """Check the original SFX directory/ranges without changing sample bytes."""
    data = path.read_bytes()
    if len(data) < 8 or data[:4] != b'sfx\x1a':
        raise ValueError('Invalid SOUNDS bank header')
    offset = int.from_bytes(data[-4:], 'little')
    if not 4 <= offset <= len(data)-4 or (len(data)-4-offset) % 18:
        raise ValueError('Invalid SOUNDS directory bounds')
    names = []
    for i in range((len(data)-4-offset)//18):
        entry = data[offset+i*18:offset+(i+1)*18]
        names.append(entry[:12].split(b'\0', 1)[0].decode('ascii'))
        start = int.from_bytes(entry[12:16], 'little')
        length = int.from_bytes(entry[16:18], 'little')
        if start+length > offset:
            raise ValueError('SOUNDS sample exceeds data range')
    return dict(file=path.name, bytes=len(data), directory_offset=offset,
                clips=len(names), all_sample_ranges_in_bounds=True,
                sample_names=names, playback_qualified=False)


def amiga_script(cases, guest_root):
    """Produce an explicit, non-autostart Shell script for an isolated lab."""
    if not re.fullmatch(r'[A-Za-z0-9_:/.-]+', guest_root) or ':' not in guest_root:
        raise ValueError('Use an absolute, simple Amiga guest path')
    lines = ['Stack 2097152', 'FailAt 21', f'CD {guest_root}/User']
    for i, (kind, name) in enumerate(cases):
        if kind not in {'menu', 'scene', 'level'} or not re.fullmatch(r'[A-Z0-9_.]+', name):
            raise ValueError('Unsafe test case')
        command = f'{guest_root}/asset-probe {guest_root}/data {kind} {name} >{guest_root}/Results/{i:03d}.txt'
        if len(command) >= 255:
            raise ValueError('Amiga Shell command too long')
        lines.extend([command, f'Echo "CASE {i:03d} RC $RC"'])
    lines.extend([f'{guest_root}/asset-probe {guest_root}/Empty menu MENU.000 >{guest_root}/Results/missing.txt',
                  'Echo "MISSING RC $RC"'])
    if any(len(line) >= 255 for line in lines):
        raise ValueError('Amiga Shell command too long')
    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--timeout', type=int, default=30)
    parser.add_argument('--guest-root', default='DH1:Games/OpenJazz-AssetValidation')
    args = parser.parse_args()
    probe, data = args.probe.resolve(), args.data.resolve()
    args.out.mkdir(parents=True, exist_ok=False)
    cases = cases_for(data)
    sound_bank = audit_sound_bank(data/'SOUNDS.000')
    (args.out/'ac090-assets.script').write_text(amiga_script(cases, args.guest_root))
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SDL_RENDER_DRIVER='software')
    def run(case):
        kind, name = case
        with tempfile.TemporaryDirectory(prefix='openjazz-assets-') as user:
            try:
                r = subprocess.run([str(probe), str(data), kind, name], cwd=user,
                                   env=env, text=True, errors='replace', capture_output=True, timeout=args.timeout)
                result = dict(kind=kind, name=name, exit=r.returncode, stdout=r.stdout, stderr=r.stderr)
                result['passed'] = r.returncode == 0 and f'PASS {kind} {name}' in r.stdout
            except subprocess.TimeoutExpired as e:
                result = dict(kind=kind, name=name, passed=False, timeout_seconds=args.timeout)
        return result
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(run, cases))
    # A required-file failure must fail rather than claim the menu was loaded.
    with tempfile.TemporaryDirectory(prefix='openjazz-empty-') as empty:
        r = subprocess.run([str(probe), empty, 'menu', 'MENU.000'], cwd=empty, env=env,
                           text=True, capture_output=True, timeout=args.timeout)
        negative = dict(exit=r.returncode, passed=r.returncode == 4 and 'FAIL menu MENU.000' in r.stderr,
                        stdout=r.stdout, stderr=r.stderr)
    report = dict(schema=1, upstream_commit='a1626f4edd4a7af72c54103021790c56d8ecfced',
                  probe_sha256=hashlib.sha256(probe.read_bytes()).hexdigest(),
                  method='Real startup, menu/scene/level constructors and level collision digest; no gameplay loop',
                  audio='Muted output, dummy host device; sample decoding may occur, playback not qualified',
                  results=results, missing_required_data=negative, sound_bank=sound_bank,
                  gameplay_qualified=False, release_qualified=False)
    (args.out/'results.json').write_text(json.dumps(report, indent=2)+'\n')
    for kind in ['menu', 'scene', 'level']:
        rows = [r for r in results if r['kind'] == kind]
        print(kind, sum(r['passed'] for r in rows), '/', len(rows))
    for r in results:
        if not r['passed']: print('FAIL', r)
    print('missing required data rejected:', negative['passed'])
    return 0 if all(r['passed'] for r in results) and negative['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
