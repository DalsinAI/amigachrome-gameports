#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Player adapter, confirmation and checkpoint continuation controls."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import zlib
from test_local_actions import SOURCES, rule_data, local_world, fixture, decode
from test_original_faction import SCHEMA
HERE = Path(__file__).resolve().parent

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--cc', default='g++')
    ap.add_argument('--m68k-linux', action='store_true')
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=False)
    b = bytearray(local_world(fixture(bases=1)))
    info, o, _ = decode(b)
    p = o['bases']
    b[p+4] = 5
    for offset, value in [(0x4c, 0), (0x50, 320), (0x44, 0), (0x40, 400), (0x98, 111)]:
        struct.pack_into('<i', b, p+offset, value)
    b[p+0x8c:p+0x94] = bytes(8)
    rules, save, checkpoint = [a.out/x for x in ['rules.txt', 'original.SC', 'checkpoint.aclocal']]
    rules.write_bytes(rule_data())
    save.write_bytes(b)
    binary = a.out/'local-player-control'
    flags = ['-std=c++17', '-O1', '-Wall', '-Wextra', '-Werror']
    flags += ['-m68040', '-static'] if a.m68k_linux else ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie']
    sources = [x for x in SOURCES if x != 'local_actions_probe.cpp'] + ['local_player.cpp', 'local_player_control.cpp']
    subprocess.run([a.cc, *flags, *[str(HERE/x) for x in sources], '-o', str(binary)], check=True)
    cmd = (['qemu-m68k', '-cpu', 'm68040'] if a.m68k_linux else []) + [str(binary), str(rules), str(save), str(checkpoint)]
    result = subprocess.run(cmd, check=True, capture_output=True, timeout=120,
                            env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
    data = checkpoint.read_bytes()
    assert data[:8] == b'ACLOCAL1'
    assert struct.unpack_from('<I', data, 24)[0] == zlib.crc32(rules.read_bytes())
    check, length, payload_crc, seed = struct.unpack_from('<IIII', data, 8)
    assert check == zlib.crc32(data[12:]) and length == len(b) and seed == 0xabcdef01
    assert payload_crc == zlib.crc32(data[28:])
    expected = bytearray(b)
    vp = o['vehicles']
    struct.pack_into('<I', expected, vp+4, struct.unpack_from('<I', b, vp+4)[0] | 4)
    expected[vp+40] = 6
    expected[vp+41] = 6
    target = o['tiles']+((3*int(info['width'])+3)//2)*44
    expected[target+2] = (expected[target+2] & 0xf0) | 5
    struct.pack_into('<i', expected, p+0x50, -2)
    struct.pack_into('<i', expected, p+0x40, 60)
    queue = next(f['offset'] for f in SCHEMA['LegacyFactionV10']['fields'] if f['name'] == 'units_queue')
    counter = o['factions']+5*0x20bc+queue+320
    expected[counter] = (expected[counter]-1) % 256
    assert data[28:] == expected, 'Independent complete-container action patches differ'
    assert save.read_bytes() == b
    receipt = dict(control=result.stdout.decode().strip(), checkpointSHA256=hashlib.sha256(data).hexdigest(),
                   nativeAmigaOS='UNRUN', independentContainerBytes=len(expected), passed=True)
    (a.out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(receipt))

if __name__ == '__main__':
    main()
