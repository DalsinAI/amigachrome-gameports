#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Engine base upkeep (WP2) controls.

Builds engine/tests/wp2_test.cpp with every engine source and the pure base
modules used as differential oracles (cost_factor, base_psych,
base_nutrients, base_minerals, base_energy, base_support, base_commerce), on
the host with ASan/UBSan or for m68k-linux under qemu (big-endian), and runs
it on synthetic states with synthetic rules (no original assets):
- randomized cost factor, psych, support, nutrient, mineral, energy and
  commerce states are computed by the engine and compared with the oracle;
- unit births, prototypes, facility completion and the production queue,
  stockpile energy, retooling, secret projects, growth and starvation, drone
  riots and golden ages, hurry and maintenance, base capture, monuments and
  base names are checked against the expected rules behaviour;
- 30 upkeep turns over eight bases keep the worker/citizen invariants.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'
ORACLES=['cost_factor','upkeep_base_psych','base_nutrients','upkeep_base_minerals','upkeep_base_energy','base_support','base_commerce']

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 sources=sorted(ENGINE.glob('*.cpp'))+sorted((ENGINE/'stubs').glob('*.cpp'))+[SRC/(m+'.cpp') for m in ORACLES]+[ENGINE/'tests'/'wp2_test.cpp']
 exe=out/'wp2-test';cmd=[a.cc,*flags,'-I',str(ENGINE),*map(str,sources),'-o',str(exe)];subprocess.run(cmd,check=True)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 r=subprocess.run([*runner,str(exe),'unit'],capture_output=True,text=True,encoding='cp1252',timeout=1800,env=env)
 assert r.returncode==0,(r.returncode,r.stdout[-2000:],r.stderr[-6000:])
 unit=r.stdout.strip();print(unit)
 # Usage errors are rejected.
 u=subprocess.run([*runner,str(exe)],capture_output=True,text=True,timeout=60,env=env)
 assert u.returncode==2 and 'usage' in u.stderr,(u.returncode,u.stderr)
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,unit=unit,
   binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n')
if __name__=='__main__':main()
