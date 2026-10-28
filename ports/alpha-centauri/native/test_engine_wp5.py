#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""WP5 engine controls: research, social engineering and unit design.

Builds engine/tests/wp5_test.cpp with every engine source, on the host with
ASan/UBSan or for m68k-linux under qemu (big-endian), then:
- runs the synthetic-rules unit checks (wp5_test unit);
- compares the engine's prototype cost and make_proto derivation (cost,
  carry, plan, role) with test_prototype.cost/derive, the independent Python
  model of the existing prototype module, over both fixture variants, every
  chassis/reactor and many component/ability combinations;
- compares the engine's original research cost (tech_rate) with
  test_research_progress.cost_expected, the independent model of the existing
  research_progress module, over its randomized human-cost domain.
"""
import argparse,hashlib,json,math,os,random,subprocess
from pathlib import Path
import test_prototype,test_research_progress

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'

def protocost_cases():
 lines=[];expect=[]
 for variant in (False,True):
  d=test_prototype.fixture(variant)
  for i,c in enumerate(d['chassis']):lines.append(f"C {i} {c['speed']} {c['triad']} {c['missile']} {c['cargo']} {c['cost']}")
  for i,w in enumerate(d['weapons']):lines.append(f"W {i} {w['offense']} {w['mode']} {w['cost']}")
  for i,a in enumerate(d['armor']):lines.append(f"A {i} {a['defense']} {a['cost']}")
  for i,b in enumerate(d['abilities']):lines.append(f"B {i} {b['cost_factor']} {b['flags']}")
  bc=len(d['abilities'])
  for c in range(9):
   for reactor in range(1,5):
    for combo in range(48):
     mask=(1<<(combo%bc))|(1<<((combo*3)%bc)) if combo else 0
     w=combo*7%len(d['weapons']);a=combo*3%len(d['armor'])
     lines.append(f'Q {c} {w} {a} {mask} {reactor}');expect.append(str(test_prototype.cost(d,c,w,a,mask,reactor)))
  for i in range(64):
   u=d['units'][i]
   row=test_prototype.derive(d,i,1)
   if row is None:continue
   lines.append(f"D {u['chassis']} {u['weapon']} {u['armor']} {u['abilities']} {u['reactor']}")
   price,carry,plan,role=row[6],row[7],row[8],row[9]
   expect.append(f'{price} {carry} {plan} {role}')
 return lines,expect

def techrate_cases():
 rng=random.Random(0x5EED5);rows=[];expect=[]
 while len(rows)<4000:
  row=test_research_progress.fixture(rng)[:25]
  row[4]=rng.choice([50,100,150]);row[3]=rng.choice([0,50,100,200,1000])
  r=test_research_progress.cost_expected(row)
  if r[0]!=0:continue
  rows.append(row);expect.append(r[1])
 return rows,expect

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 sources=sorted(ENGINE.glob('*.cpp'))+sorted((ENGINE/'stubs').glob('*.cpp'))+[ENGINE/'tests'/'wp5_test.cpp']
 exe=out/'wp5-test';cmd=[a.cc,*flags,'-I',str(ENGINE),*map(str,sources),'-o',str(exe)];subprocess.run(cmd,check=True)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 def run(*args):return subprocess.run([*runner,str(exe),*map(str,args)],capture_output=True,text=True,timeout=900,env=env)
 r=run('unit');assert r.returncode==0,(r.stdout,r.stderr[-4000:]);unit=r.stdout.strip();print(unit)
 lines,expect=protocost_cases();(out/'protocost.txt').write_text('\n'.join(lines)+'\n')
 r=run('protocost',out/'protocost.txt');assert r.returncode==0,(r.returncode,r.stderr[-2000:])
 got=r.stdout.split('\n')[:-1];assert len(got)==len(expect),(len(got),len(expect))
 bad=[(i,g,e) for i,(g,e) in enumerate(zip(got,expect)) if g!=e];assert not bad,bad[:10]
 print(f'{len(expect)} prototype cost/derivation cases match test_prototype')
 rows,rexp=techrate_cases();(out/'techrate.txt').write_text('\n'.join(' '.join(map(str,x)) for x in rows)+'\n')
 r=run('techrate',out/'techrate.txt');assert r.returncode==0,(r.returncode,r.stderr[-2000:])
 got=[tuple(map(int,l.split())) for l in r.stdout.split('\n') if l]
 assert len(got)==len(rexp),(len(got),len(rexp))
 bad=[(i,rows[i],g,e) for i,(g,e) in enumerate(zip(got,rexp)) if g[0]!=e or g[1]!=e];assert not bad,bad[:5]
 print(f'{len(rexp)} research cost cases match test_research_progress')
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,unit=unit,protocost=len(expect),techrate=len(rexp),
   binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n')
if __name__=='__main__':main()
