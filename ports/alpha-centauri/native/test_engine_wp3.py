#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Engine unit orders, movement, path search and terraforming (WP3) controls.

Builds engine/tests/wp3_test.cpp with every engine source plus the pure
movement modules (move_budget.cpp; terrain_price.h is header-only), on the
host with ASan/UBSan or for m68k-linux under qemu (big-endian), then runs:
- unit: synthetic worlds with synthetic rules tables (no original assets):
  speeds, terraforming (farm, soil enricher, mine, solar, forest, road, mag
  tube, bunker, airbase, sensor, remove fungus, condenser, borehole, raise,
  lower, level) with helpers and contribution, base building with a colony
  pod, single steps, roads, zones of control, path search and go-to,
  transports (board, carry, unload, sentry), air units and fuel, supply pods,
  the monolith, visibility, hold/sentry/wake, home, disband, upgrade, convoy
  and supply crawler help;
- hexcost: mod_hex_cost on random adjacent tiles, designs, factions and
  projects against terrain_price.h (OpenSMACX hex_cost arithmetic);
- budget: one order_veh step with random terrain, spent moves, retry flag,
  owner and RNG seed against move_budget (Thinker order_veh expenditure):
  position, moves_spent, retry flag and RNG state must agree.
Several seeds are run for the random cases; a receipt is written to OUT.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'
SEEDS=[1,2,3,0x1234,0xC0FFEE]

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--count',type=int,default=4000);a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 sources=sorted(ENGINE.glob('*.cpp'))+sorted((ENGINE/'stubs').glob('*.cpp'))+[SRC/'move_budget.cpp',ENGINE/'tests'/'wp3_test.cpp']
 exe=out/'wp3-test';cmd=[a.cc,*flags,'-I',str(ENGINE),'-I',str(SRC),*map(str,sources),'-o',str(exe)];subprocess.run(cmd,check=True)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 def run(*args):
  r=subprocess.run([*runner,str(exe),*map(str,args)],capture_output=True,text=True,encoding='cp1252',timeout=1800,env=env)
  assert r.returncode==0,(args,r.returncode,r.stdout[-2000:],r.stderr[-4000:])
  return r.stdout.strip()
 records=[]
 unit=run('unit');print(unit);records.append(dict(case='unit',result=unit))
 hex_total=0;budget_total=0;fractional=0
 for seed in SEEDS:
  h=run('hexcost',seed,a.count);records.append(dict(case=f'hexcost-{seed}',result=h));hex_total+=int(h.split()[0])
  b=run('budget',seed,a.count//4);records.append(dict(case=f'budget-{seed}',result=b));budget_total+=int(b.split()[0])
  fractional+=int(b.split('(')[1].split()[0])
 assert fractional>0,'no fractional move failure was exercised'
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,cases=records,
   binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n')
 print(f'{hex_total} hex cost cases and {budget_total} move budget cases ({fractional} fractional failures) matched the oracles')
if __name__=='__main__':main()
