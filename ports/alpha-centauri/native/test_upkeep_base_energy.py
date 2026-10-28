#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent arithmetic oracle for original HQ loss/statistics and subtotal."""
import argparse,hashlib,itertools,json,os,random,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
SENTINEL=0x5a5a5a5a

def expected(v):
 fail=lambda e:[e]+[SENTINEL]*13
 fresh,to,commerce,grid,fr,dist,eff,stage,creche=v[:9];stats=v[9:]
 if min(fresh,to,commerce,grid,fr)<0 or dist < -1 or stage not in [0,1,2]:return fail(1)
 def fit(n):
  if not -(1<<31)<=n<(1<<31):raise OverflowError
  return n
 try:
  gross=fresh
  for n in [to,commerce,grid]:gross=fit(gross+n)
  preloss=fit(gross-fr);loss=0
  distance=dist if 0<=dist<999 else 16
  if preloss>0 and distance:
   product=None
   if stage==1:
    product=fit(preloss*distance)
    stats=[fit(n+(preloss if j-2*creche>=8 else product//(8*(8-j+2*creche)))) for j,n in enumerate(stats)]
   ineff=fit(fit(4-eff)-2*creche)
   if ineff>=8:loss=preloss
   else:
    denom=fit(fit(8-ineff)*8)
    if product is None:product=fit(preloss*distance)
    loss=min(preloss,max(0,product//denom))
  return [0,gross,fr,loss,fit(preloss-loss),*stats]
 except OverflowError:return fail(3)

def cases():
 rows=[]
 for fresh,dist,eff,stage,creche in itertools.product([0,1,7,16,31,99],[-1,0,1,8,16,32,998,999,1000],range(-6,7),range(3),range(2)):
  rows.append([fresh,1,2,0,3,dist,eff,stage,creche]+list(range(9)))
 rng=random.Random(0x4ea1f0);edges=[-(1<<31),-999,-2,-1,0,1,16,998,999,1000,(1<<31)-1]
 for _ in range(6000):
  row=[rng.randrange(1000),rng.randrange(10),rng.randrange(10),rng.randrange(10),rng.randrange(1000),rng.randrange(-1,1200),rng.randrange(-8,9),rng.randrange(3),rng.randrange(2)]+[rng.randrange(-999,999) for _ in range(9)]
  for _ in range(rng.randrange(3)):row[rng.choice(list(range(8))+list(range(9,18)))]=rng.choice(edges)
  rows.append(row)
 return rows

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 exe=a.out/'base-energy-probe';cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',str(HERE/'upkeep_base_energy.cpp'),str(HERE/'upkeep_base_energy_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 rows=cases();data=b''.join(struct.pack('<18i',*r)for r in rows);(a.out/'controls.bin').write_bytes(data)
 run=subprocess.run([str(exe)],input=data,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60,check=True);assert len(run.stdout)==len(rows)*56
 counts={0:0,1:0,3:0}
 for i,row in enumerate(rows):
  want=expected(row);got=list(struct.unpack_from('<14i',run.stdout,i*56));assert got==want,(i,row,got,want);counts[want[0]]+=1
 (a.out/'expected.bin').write_bytes(run.stdout)
 receipt=dict(cases=len(rows),accepted=counts[0],invalid=counts[1],overflow=counts[3],aliasControls='all aligned partial overlaps reject without writes',outputSHA256=hashlib.sha256(run.stdout).hexdigest(),command=cmd,native='UNRUN',originalExecutable='UNRUN');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
