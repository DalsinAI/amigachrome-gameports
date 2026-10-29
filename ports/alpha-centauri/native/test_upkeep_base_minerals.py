#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent signed integer oracle for the explicit-fact mineral leaf."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import struct
import subprocess

HERE = Path(__file__).resolve().parent
SENTINEL = 0x5A5A5A5A


def expected(v):
    intake, to, support, fr, raw, ecology, satellites, pop, atrocities, tectonics, sea, rises, fdiff, diff, tech, transcendent, planet, life = v[:18]
    quantum, robot, gene, nano, bulk, inductor, preserve, temple, pholus, human, ascending, perihelion, orbital = v[18:]
    if min(intake, to, support, fr, atrocities, tectonics, tech, transcendent) < 0 or not 0 <= pop <= 127 or not 0 <= fdiff <= 5 or not 0 <= diff <= 5 or not 0 <= life <= 2 or transcendent > tech:
        return [1] + [SENTINEL] * 5
    def fit(n):
        if not -(1 << 31) <= n < (1 << 31):
            raise OverflowError
        return n
    def div(n, d):
        return (1 if n >= 0 else -1) * (abs(n) // d)
    def clamp(n, low, high):
        return min(max(n, low), high)
    try:
        output = fit(fit(intake + to) * (2 + bool(quantum or inductor) + robot + gene + nano + bulk)) // 2
        consumed = fit(support + fr)
        surplus = fit(output - consumed)
        final = surplus
        damage = div(raw, 8)
        clean = fit(ecology + 16)
        if damage > 0:
            absorbed = min(damage, clean)
            damage = fit(damage - absorbed)
            clean = fit(clean - absorbed)
        reduction = 1 + bool(nano or inductor) + preserve + temple + pholus
        damage = fit(damage + div(fit(fit(output - clean) - clamp(satellites, 0, pop)), reduction))
        if human:
            damage = fit(damage + fit(fit(atrocities + tectonics) * 5) // (clamp(sea, 0, 100) // clamp(rises, 1, 100) + reduction))
        damage = max(0, damage)
        for active in [ascending, perihelion]:
            if active:
                damage = fit(damage * 2)
        factor = (2 if fdiff == 0 else 3 if fdiff <= 3 else 5) if human else clamp(6 - diff, 1, 3)
        numerator = tech - transcendent
        for n in [3 - clamp(planet, -3, 2), life + 1, damage, factor]:
            numerator = fit(numerator * n)
        damage = fit(numerator // 6 + 50) // 100
        if orbital:
            output = fit(output * 2)
            surplus = fit(output - consumed)
        return [0, output, consumed, surplus, final, damage]
    except OverflowError:
        return [3] + [SENTINEL] * 5


def cases():
    seed = [7, 1, 2, 1, 80, 0, 3, 4, 2, 1, 50, 10, 3, 3, 50, 0, 0, 1] + [0] * 13
    rows = []
    # Exhaust facility/project combinations independently from event/actor flags.
    for flags in itertools.product([0, 1], repeat=9):
        for human, orbital in itertools.product([0, 1], repeat=2):
            row = seed[:]
            row[18:27] = flags
            row[27], row[30] = human, orbital
            rows.append(row)
    rng = random.Random(0x4E9CB0)
    edges = [-(1 << 31), -100, -17, -1, 0, 1, 7, 8, 15, 16, 17, 99, 100, (1 << 31) - 1]
    for _ in range(8000):
        row = seed[:]
        for i in range(18):
            row[i] = rng.choice(edges) if rng.randrange(4) == 0 else rng.randrange(100)
        row[7], row[12], row[13], row[17] = rng.randrange(128), rng.randrange(6), rng.randrange(6), rng.randrange(3)
        row[15] = rng.randrange(max(0, row[14]) + 1) if row[14] >= 0 else 0
        row[18:] = [rng.randrange(2) for _ in range(13)]
        rows.append(row)
    for index in range(18):
        for value in edges:
            row = seed[:]
            row[index] = value
            rows.append(row)
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--cc', default='c++')
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    exe = args.out / 'base-minerals-probe'
    command = [args.cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie', str(HERE / 'upkeep_base_minerals.cpp'), str(HERE / 'upkeep_base_minerals_probe.cpp'), '-o', str(exe)]
    subprocess.run(command, check=True)
    rows = cases()
    data = b''.join(struct.pack('<31i', *r) for r in rows)
    result = subprocess.run([str(exe)], input=data, capture_output=True, check=True, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'), timeout=60)
    assert len(result.stdout) == len(rows) * 24
    counts = {0: 0, 1: 0, 3: 0}
    for i, row in enumerate(rows):
        want = expected(row)
        got = list(struct.unpack_from('<6i', result.stdout, i * 24))
        assert got == want, (i, row, got, want)
        counts[want[0]] += 1
    receipt = dict(cases=len(rows), accepted=counts[0], invalid=counts[1], overflow=counts[3], alias_controls='all aligned partial offsets rejected without writes', outputSHA256=hashlib.sha256(result.stdout).hexdigest(), command=command, native='UNRUN', originalExecutable='UNRUN')
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))


if __name__ == '__main__':
    main()
