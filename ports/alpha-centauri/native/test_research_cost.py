# SPDX-License-Identifier: GPL-3.0-or-later
"""Original signed32 tech_rate cost, independent bounded-integer controls."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent
def trunc(a,b):return (abs(a)//abs(b))*(-1 if (a<0)!=(b<0) else 1)
def narrow(n):
 if not(-2**31<=n<2**31):raise OverflowError
 return n
def clamp(n,a,b):return min(max(n,a),b)
def calculate(c):
 cache,rate,root,turn,diff,flags,mask,faction=c[:8];ranking,earned,adjust,local,research,modifier=[c[x:x+8] for x in range(8,56,8)]
 if not 0<=faction<8:return (1,17,0)
 if cache>=0:return (0,cache,1)
 if not rate:return (0,999999999,2)
 human=bool(mask&(1<<faction));used_diff=local[faction] if human else diff
 if not(0<rate<=1000 and root>=0 and 0<=used_diff<=5):return (1,17,0)
 try:
  half=trunc(clamp(narrow(narrow(narrow(earned[faction]*2)-adjust[faction])+ranking[faction]),2,9999),2)
  top=max([0]+[narrow(narrow(earned[i]*2)+ranking[i]) for i in range(1,8)])//2
  d=used_diff+(used_diff<3);level=3 if human else diff
  factor=clamp(4*d+8 if human else 29-3*d,12-half,12+half)
  penalty=clamp(half-trunc(turn,12 if flags&0x20 else 8),0,trunc(factor*(3 if flags&0x20 else 2),2));adjusted=factor+penalty
  reduction=clamp(trunc(top-level-half+7,8-level),0,trunc(level*adjusted,10)+1)
  cost=narrow(clamp(half-clamp(research[faction],-1,1),1,99999)*(adjusted-reduction))
  if rate!=100:cost=trunc(narrow(cost*100),rate)
  if modifier[faction]!=100:cost=trunc(narrow(cost*modifier[faction]),100)
  cost=trunc(narrow(cost*root),56)
  if flags&0x20:cost=narrow(cost+trunc(cost,2))
  return (0,clamp(cost,1,99999999),0)
 except OverflowError:return (2,17,0)
def cases():
 rng=random.Random(0x5be6b0);rows=[]
 for mask in [0,255,0x55,0x80]:
  for diff in range(6):
   for stagnate in [0,0x20]:
    for faction in range(8):rows.append([-1,rng.choice([1,25,50,75,100,125,200,1000]),rng.choice([0,8,16,32,56,100]),rng.choice([-100,0,1,9,37,100,499,1000]),diff,stagnate,mask,faction,*[rng.randrange(-100,1000) for _ in range(8)],*[rng.randrange(0,500) for _ in range(8)],*[rng.randrange(-50,200) for _ in range(8)],*[rng.randrange(6) for _ in range(8)],*[rng.randrange(-5,6) for _ in range(8)],*[rng.choice([0,50,75,100,125,200]) for _ in range(8)]])
 base=[-1,100,56,0,3,0,255,1,*([10]*8),*([0]*8),*([0]*8),*([3]*8),*([0]*8),*([100]*8)]
 for offset,value in [(0,0),(0,123),(1,0),(1,-1),(1,1001),(2,-1),(4,8),(7,-1),(7,8),(16+1,2**31-1),(24+1,-2**31),(48+1,2**31-1),(48+1,-2**31),(2,2**31-1)]:
  b=base[:];b[offset]=value;rows.append(b)
 for rank in [0,1000000]:b=base[:];b[8]=rank;rows.append(b)
 return rows
FILES=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','research_cost.cpp','research_cost_probe.cpp']
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-contexts',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);rows=cases()
 if a.original_contexts:rows+=json.loads(a.original_contexts.read_text())
 data=b''.join(struct.pack('<56i',*r) for r in rows);(out/'inputs.bin').write_bytes(data);flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(SRC/f) for f in FILES],'-o',str(out/'probe')],check=True)
 expected=b''.join(struct.pack('<3i',*calculate(r)) for r in rows);p=subprocess.run(runner+[str(out/'probe'),str(out/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert p.returncode==0,(p.returncode,p.stderr);assert p.stdout==expected,[(i,struct.unpack_from('<3i',p.stdout,i*12),calculate(r)) for i,r in enumerate(rows) if p.stdout[i*12:i*12+12]!=expected[i*12:i*12+12]][:5]
 (out/'outputs.bin').write_bytes(p.stdout);receipt={'contexts':len(rows),'computed':sum(calculate(r)[0]==0 and calculate(r)[2]==0 for r in rows),'overflowRejected':sum(calculate(r)[0]==2 for r in rows),'invalidRejected':sum(calculate(r)[0]==1 for r in rows),'additionalAtomicShortcutControls':4,'outputSHA256':hashlib.sha256(p.stdout).hexdigest()};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
