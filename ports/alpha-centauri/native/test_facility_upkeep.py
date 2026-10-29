#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent original maintenance ledger/scrapping/rounding oracle."""
import argparse,hashlib,itertools,json,os,random,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
SENTINEL=0x5a5a5a5a

def expected(v):
 fail=lambda e:[e]+[SENTINEL]*82
 project,diff,reactor,credits,total,factor,queue,flags,sac=v[:9];built=sum(v[9+i]<<(32*i)for i in range(3));free=sum(v[12+i]<<(32*i)for i in range(3));cost=v[15:85];maint=v[85:155]
 if project not in [39,70]or not 0<=diff<=5 or not 1<=reactor<=4 or min(credits,total,factor)<0:return fail(1)
 def fit(n):
  if not -(1<<31)<=n<(1<<31):raise OverflowError
  return n
 charges=[0]*70;scrap=short=0
 try:
  for id in range(1,34 if project==39 else 65):
   if not built>>id&1:continue
   if min(cost[id],maint[id])<0:return fail(1)
   charge=min(reactor,(diff+1)//2)if id==27 else 0 if free>>id&1 else maint[id]
   if sac:
    charge=fit(charge+int(bool(flags&32)));flags=(flags|32)if charge&1 else(flags&~32);charge//=2
   credits=fit(credits-charge);total=fit(total+charge);charges[id]=charge
   if credits<0:
    short|=1<<id
    if diff<=1 or queue==-id:credits=0
    else:built&=~(1<<id);scrap|=1<<id;credits=fit(cost[id]*factor)
  return [0,credits,total,flags,*[(b>>(32*i))&0xffffffff for b in [built,scrap,short]for i in range(3)],*charges]
 except OverflowError:return fail(3)

def cases():
 rows=[]
 for mask,sac,carry,diff,queue,credits in itertools.product(range(16),range(2),range(2),range(6),[130,-6,-27],[0,3,20]):
  built=(1<<65)|sum(1<<id for j,id in enumerate([6,8,27,33])if mask>>j&1);free=(1<<8)|(1<<27)
  rows.append([70,diff,4,credits,0,10,queue,0x80000100|carry*32,sac,*[(b>>(32*i))&0xffffffff for b in [built,free]for i in range(3)],*([8]*70),*([3]*70)])
 rng=random.Random(0x4f65f0);edges=[-(1<<31),-1,0,1,2,39,70,(1<<31)-1]
 for _ in range(1500):
  row=rows[rng.randrange(len(rows))][:];row[0]=rng.choice([39,70]);row[2]=rng.randrange(1,5);row[3]=rng.randrange(100);row[4]=rng.randrange(100);row[8]=rng.randrange(2)
  for j in range(15,155):row[j]=rng.randrange(25)
  for _ in range(rng.randrange(3)):row[rng.choice(list(range(7))+list(range(15,155)))]=rng.choice(edges)
  rows.append(row)
 return rows

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.out/'facility-upkeep-probe';cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',str(HERE/'facility_upkeep.cpp'),str(HERE/'facility_upkeep_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 rows=cases();data=b''.join(struct.pack('<155I',*[x&0xffffffff for x in row])for row in rows);(a.out/'controls.bin').write_bytes(data);r=subprocess.run([str(exe)],input=data,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),check=True,timeout=60);assert len(r.stdout)==len(rows)*332
 counts={0:0,1:0,3:0}
 for i,row in enumerate(rows):
  want=expected(row);got=list(struct.unpack_from('<83I',r.stdout,i*332));assert got==want,(i,row,got,want);counts[want[0]]+=1
 (a.out/'expected.bin').write_bytes(r.stdout);receipt=dict(cases=len(rows),accepted=counts[0],invalid=counts[1],overflow=counts[3],aliasControls='all aligned partial overlaps rejected without writes',outputSHA256=hashlib.sha256(r.stdout).hexdigest(),command=cmd,native='UNRUN',originalExecutable='UNRUN');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
