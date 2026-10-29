#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Vanilla fresh cost against independent integers and compiled complete original body."""
import argparse,hashlib,json,math,os,random,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
SENTINEL=0x5a5a5a5a
def pack(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))
def checked(value):
 if not -2147483648<=value<=2147483647:raise OverflowError
 return value
def clamp(n,lo,hi):return max(lo,min(hi,n))
def trunc(n,d):return n//d if n>=0 else-((-n)//d)
def cost_expected(w):
 faction,diff,turn,rate,custom,research,tiles=w[:7];rank=w[7:15];deferred=w[15:23];debt,stagnation=w[23:25];fail=lambda code:[code]+[SENTINEL]*7
 if not 1<=faction<=7 or not 0<=diff<=5 or turn<0 or not 0<=rate<=1000 or custom<0 or not 1<=tiles<=8388608 or debt<0:return fail(1)
 root=math.isqrt(tiles)
 if not rate:return[0,999999999,root,0,0,0,0,0]
 if min(rank[1:]+deferred[1:])<0:return fail(1)
 try:
  top=max(checked(checked(2*deferred[i])+rank[i])for i in range(1,8))//2;score=checked(checked(checked(2*deferred[faction])-debt)+rank[faction]);player=clamp(score,2,9999)//2;factor=clamp(4*(diff+(diff<3))+8,12-player,player+12);final=clamp(player-turn//(12 if stagnation else 8),0,factor*(3 if stagnation else 2)//2)+factor;reduction=clamp(trunc(checked(top-3-player+7),5),0,3*final//10+1);discovery=checked((final-reduction)*clamp(player-clamp(research,-1,1),1,99999))
  if rate!=100:discovery=checked(100*discovery)//rate
  if custom!=100:discovery=checked(discovery*custom)//100
  cost=checked(discovery*root)//56
  if stagnation:cost=checked(cost+cost//2)
  return[0,clamp(cost,1,99999999),root,player,top,factor,final,discovery]
 except OverflowError:return fail(3)
def contribution_expected(w,progress=True):
 labs,se,turn,diff,cost,total,remainder,accum,noadv,tutorial,sphere=w;fail=lambda code:[code]+[SENTINEL]*7
 if labs<0 or turn<0 or not 0<=diff<=5 or total<0 or not 0<=remainder<=99 or accum<0:return fail(1)
 if noadv:return[0,total,remainder,accum,0,0,0,2]
 if turn==0:return[0,total,remainder,accum,0,0,0,1]
 if tutorial and turn<=5:return[0,total,remainder,accum,0,0,0,3]
 try:
  if sphere:labs//=2
  threshold=checked(-5*se);reason=0
  if turn<threshold:labs=0;reason=4
  if turn<5 and diff<2:labs=0;reason=5
  total=checked(total+labs);scaled=checked(checked(10*labs)*(clamp(se,-5,5)+10));r=scaled%100+remainder
  if r>=100:scaled=checked(scaled+100);r-=100
  points=scaled//100;complete=0
  if points and progress:
   if not 1<=cost<=999999999:return fail(1)
   accum=checked(accum+points)
   if accum>=cost:accum=0;complete=1
  return[0,total,r,accum,labs,points,complete,reason]
 except OverflowError:return fail(3)
def fixture(rng):
 faction=rng.randrange(1,8);rank=[rng.randrange(160)for _ in range(8)];deferred=[rng.randrange(5)for _ in range(8)];cost=[faction,rng.randrange(6),rng.randrange(400),rng.choice([0,50,100,200,1000]),rng.choice([0,50,100,150]),rng.randrange(-5,6),rng.choice([1,2,3,95,96,1152,8192,16384,32768,8388608]),*rank,*deferred,rng.randrange(12),rng.randrange(2)];contribution=[rng.randrange(1000),rng.randrange(-8,9),rng.randrange(40),rng.randrange(6),rng.choice([1,10,100,1000,999999999]),rng.randrange(10000),rng.randrange(100),rng.randrange(1000),int(rng.randrange(7)==0),rng.randrange(2),rng.randrange(2)];return cost+contribution
def cases():
 rng=random.Random(0x2E5EA2C4);rows=[fixture(rng)for _ in range(6000)]
 for se in range(-5,6):
  for turn in[0,1,4,5,6,24,25,26]:
   for diff in range(6):
    for labs in[0,1,9,10]:
     row=fixture(rng);row[25:]=[labs,se,turn,diff,10,0,99,9,0,0,0];rows.append(row)
 for index,value in[(0,0),(0,8),(1,-1),(1,6),(2,-1),(3,-1),(3,1001),(4,-1),(6,0),(6,8388609),(8,-1),(16,-1),(23,-1),(8,2147483647),(16,2147483647),(4,2147483647),(25,-1),(27,-1),(28,6),(30,-1),(31,-1),(31,100),(32,-1),(26,-2147483648),(25,2147483647),(30,2147483647),(32,2147483647),(29,0),(29,1000000000)]:
  row=fixture(rng);row[3]=100;row[25:]=[100,0,30,3,1000,0,99,0,0,0,0];row[index]=value;rows.append(row)
 # Faction0 counters are not part of the original top-factor scan.
 row=fixture(rng);row[7]=row[15]=2147483647;rows.append(row)
 return rows
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--binary',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.binary or a.out/'research-progress-probe';cmd=[]
 if not a.binary:
  cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',str(HERE/'research_progress.cpp'),str(HERE/'research_cost_reference.cpp'),str(HERE/'research_progress_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 rows=cases();path=a.out/'cases.bin';path.write_bytes(pack([len(rows)])+b''.join(pack(row)for row in rows));wanted=b''.join(pack(cost_expected(row[:25])+contribution_expected(row[25:]))for row in rows);run=subprocess.run([str(exe),str(path)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=90);assert run.returncode==0,(run.returncode,run.stderr,len(run.stdout))
 if run.stdout!=wanted:
  for i,row in enumerate(rows):
   got=list(struct.unpack_from('<16I',run.stdout,64*i));want=[n&0xffffffff for n in cost_expected(row[:25])+contribution_expected(row[25:])];assert got==want,(i,row,got,want)
 (a.out/'results.bin').write_bytes(run.stdout);receipt=dict(cases=len(rows),bytes=len(wanted),sha256=hashlib.sha256(wanted).hexdigest(),command=cmd,compiledOriginalCostReference=True,native='UNRUN');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(rows)} original-cost and research contribution cases passed')
if __name__=='__main__':main()
