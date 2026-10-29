#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Engine computer players (engine/ai_*.cpp, WP6) controls.

Builds engine/tests/wp6_test.cpp with every engine source, on the host with
ASan/UBSan or for m68k-linux under qemu (big-endian, -m68040), writes the
synthetic rules directories of test_engine_wp0b.py (no original assets) and
then:
- runs the AI helper checks ("unit");
- plays several seeded synthetic worlds for a number of AI turns and checks
  that every run finishes, that a second run of the same seed prints exactly
  the same per-turn digests (unit orders, positions, production, goals,
  plans, research and social choices), and, with --compare RECEIPT, that the
  digests equal those of another build's receipt (host against m68k);
- records per-turn work counters and wall time for the 68040 budget.
Optional --rules DIR plays the same worlds with owner-provided rules tables
(never committed) instead of the test tables, and only requires that the run
completes deterministically.
"""
import argparse,hashlib,json,os,shutil,subprocess
from pathlib import Path
import test_engine_wp0b

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'
SEEDS=[1,2,3,17,4242]

def parse(stdout):
 turns=[];final=None;stubs={};total_ms=None
 for line in stdout.splitlines():
  w=line.split()
  if not w:continue
  if w[0]=='turn':
   rec={'turn':int(w[1])}
   for kv in w[2:]:
    k,v=kv.split('=');rec[k]=v if k=='digest' else float(v) if k=='ms' else int(v)
   turns.append(rec)
  elif w[0].startswith('digest='):final=w[0].split('=')[1]
  elif w[0].startswith('total_ms='):total_ms=float(w[0].split('=')[1])
  elif w[0]=='stub':stubs[w[1]]=int(w[2])
 return turns,final,stubs,total_ms

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--turns',type=int,default=12)
 ap.add_argument('--seeds',type=int,nargs='*',default=SEEDS);ap.add_argument('--compare',type=Path)
 ap.add_argument('--rules',type=Path,help='owner-provided rules directory (alpha.txt/alphax.txt), local only');a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 sources=sorted(ENGINE.glob('*.cpp'))+sorted((ENGINE/'stubs').glob('*.cpp'))+[ENGINE/'tests'/'wp6_test.cpp']
 exe=out/'wp6-test';cmd=[a.cc,*flags,'-I',str(ENGINE),*map(str,sources),'-o',str(exe)];subprocess.run(cmd,check=True)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 def run(*args):return subprocess.run([*runner,str(exe),*map(str,args)],capture_output=True,text=True,encoding='cp1252',timeout=3600,env=env)
 dirs={}
 for variant in ['base','expansion']:
  d=out/('rules-'+variant);test_engine_wp0b.make_dir(d,variant);dirs[variant]=d
 r=run('unit',dirs['expansion'],'expansion');assert r.returncode==0,(r.returncode,r.stdout[-2000:],r.stderr[-4000:])
 unit=r.stdout.strip();print(unit)
 cases=[]
 def play(name,d,variant,seed,keep=False):
  extra=['keep'] if keep else []
  runs=[]
  for _ in range(2):
   r=run('turns',d,variant,seed,a.turns,*extra)
   assert r.returncode==0,(name,seed,r.returncode,r.stdout[-2000:],r.stderr[-4000:])
   runs.append(parse(r.stdout))
  (t1,f1,stubs,ms1),(t2,f2,_,_)=runs
  assert len(t1)==a.turns and f1,(name,seed,len(t1))
  d1=[t['digest'] for t in t1];d2=[t['digest'] for t in t2]
  assert d1==d2,(name,seed,'rerun differs',next(i for i,(x,y) in enumerate(zip(d1,d2)) if x!=y)+1)
  assert len(set(d1))>1,(name,seed,'state never changes')
  rec=dict(case=name,variant=variant,seed=seed,turns=a.turns,digest=f1,turn_digests=d1,
   counters=[{k:v for k,v in t.items() if k not in('digest',)} for t in t1],total_ms=ms1,stubs=stubs)
  cases.append(rec)
  last=t1[-1]
  print(f'{name} seed {seed}: {a.turns} turns digest {f1} vehs {last["vehs"]} bases {last["bases"]} '
        f'max tile_steps/turn {max(t["tile_steps"] for t in t1)} total {ms1:.0f} ms')
 for variant in ['expansion','base']:
  for seed in a.seeds:play('synthetic-'+variant,dirs[variant],variant,seed)
 if a.rules:
  rd=out/'rules-owner';rd.mkdir()
  # labels.txt and faction files come from the synthetic set when the owner's directory lacks them.
  src=dirs['expansion']
  for f in src.iterdir():shutil.copy(f,rd/f.name)
  for f in a.rules.iterdir():
   if f.is_file():shutil.copy(f,rd/f.name)
  variant='expansion' if any((rd/n).exists() for n in ['alphax.txt','ALPHAX.TXT']) else 'base'
  if variant=='expansion':
   for n in ['alpha.txt','ALPHA.TXT']:
    if (rd/n).exists() and not (a.rules/n).exists():(rd/n).unlink()
  for seed in a.seeds[:3]:play('owner-rules',rd,variant,seed,keep=True)
 compared=None
 if a.compare:
  other=json.loads(a.compare.read_text())
  theirs={(c['case'],c['seed']):c for c in other['cases'] if c['turns']==a.turns}
  compared=0
  for c in cases:
   o=theirs.get((c['case'],c['seed']))
   if o is None:continue
   assert o['turn_digests']==c['turn_digests'],(c['case'],c['seed'],'differs from',str(a.compare),
    next(i for i,(x,y) in enumerate(zip(o['turn_digests'],c['turn_digests'])) if x!=y)+1)
   compared+=1
  assert compared>0,'nothing to compare'
  print(f'{compared} runs match {a.compare}')
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,unit=unit,cases=cases,compared=compared,
   binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n')
 print(f'{len(cases)} AI runs of {a.turns} turns passed (deterministic on rerun)')
if __name__=='__main__':main()
