#!/usr/bin/env python3
"""Build the AC090 cache equivalence probe using the pinned upstream loaders."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'scripts'))
from build_sdl3_port import verify_stove,verify_source,digest
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--stove',required=True,type=Path);p.add_argument('--source',required=True,type=Path);p.add_argument('--out',required=True,type=Path)
a=p.parse_args();stove=a.stove.resolve();source=a.source.resolve();out=a.out.resolve()
verify_stove(stove)
verify_source(source,'2fa1c7a66de2ce8707d85cb9a304dbd6f96c8ab8')
if out.exists():raise ValueError('use a fresh output directory')
out.mkdir(parents=True)
port=Path(__file__).resolve().parents[1]
args=[str(stove/'prefix/bin/m68k-amigaos-g++'),'-noixemul','-pthread','-m68040','-m68881',
      '-fno-delete-null-pointer-checks','-O2','-std=gnu++98','-I'+str(source/'LIB386/H'),
      '-ffile-prefix-map='+str(ROOT)+'=/gameports',str(port/'tests/native_cache_probe.cpp'),str(port/'compat/native_cache.cpp')]
args += [str(source/n) for n in ['LIB386/SYSTEM/HQFILE.CPP','LIB386/SYSTEM/LZ.CPP','LIB386/ANIM/ANIM_LE.CPP','LIB386/OBJECT/BODY_LE.CPP']]
args += ['-o',str(out/'native-cache-probe')]
receipt={'command':args,'upstream_commit':'2fa1c7a66de2ce8707d85cb9a304dbd6f96c8ab8','runtime':'not tested'}
with (out/'build.log').open('w') as log:subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,check=True)
receipt['sha256']=digest(out/'native-cache-probe');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(out/'native-cache-probe')
