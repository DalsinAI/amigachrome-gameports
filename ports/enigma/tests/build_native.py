#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
"""Build native XML regression programs from a completed soft-float engine build."""
from pathlib import Path
import argparse
import subprocess

ap = argparse.ArgumentParser(description=__doc__)
for name in ('stove', 'engine', 'out'):
    ap.add_argument('--' + name, type=Path, required=True)
a = ap.parse_args()
a.stove, a.engine, a.out = (p.resolve() for p in (a.stove, a.engine, a.out))
a.out.mkdir(parents=True, exist_ok=True)
cxx = a.stove / 'prefix/bin/m68k-amigaos-g++'
src, cm = a.engine / 'source', a.engine / 'cmake'
oxp = a.engine / 'oxp-source/oxp'
flags = ['-m68040', '-specs=' + str(oxp.parent / 'toolchain/m68k-soft-m68040.specs'),
         '-mcrt=nix20', '-pthread', '-std=c++17', '-O2',
         '-fno-delete-null-pointer-checks', '-fno-tree-loop-distribute-patterns',
         '-fno-late-combine-instructions', '-I' + str(oxp / 'include'),
         '-I' + str(src / 'src')]
checks = [
    ('adapter-check', Path(__file__).parent / 'adapter_test.cpp',
     [cm / 'src/CMakeFiles/enigma.dir/oxp_enigma.cc.obj']),
    ('schema-checks', oxp / 'tests/mutation_schema_test.cpp', []),
]
for name, source, objects in checks:
    command = [str(cxx), *flags, str(source), *map(str, objects),
               str(cm / 'src/CMakeFiles/enigma.dir/amiga_runtime.c.obj'),
               str(cm / 'liboxp-schema.a'), str(cm / 'liboxp.a'),
               '-latomic', '-lm', '-o', str(a.out / name)]
    subprocess.run(command, check=True)
    print(a.out / name, flush=True)
