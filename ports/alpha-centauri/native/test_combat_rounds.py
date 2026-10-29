#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent finite combat model against complete deterministic draw streams."""
import argparse, hashlib, itertools, json, os, random, struct, subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
SENTINEL=0x5a5a5a5a

def expected(v,rolls):
 offense,defense,ar,dr,ad,dd,count,cursor=v
 initial_cursor=cursor;fail=lambda e:[e,initial_cursor]+[SENTINEL]*9
 if cursor>count:return fail(2)
 if offense<0 or defense<0 or not(1<=ar<=4 and 1<=dr<=4 and 0<=ad<10*ar and 0<=dd<10*dr):return fail(1)
 start=cursor;ah=dh=0
 while ad<10*ar and dd<10*dr:
  draws=[]
  for strength in [offense,defense]:
   if strength<=1:draws.append(0)
   else:
    if cursor>=count or rolls[cursor]>32767:return fail(5)
    draws.append(rolls[cursor]%strength);cursor+=1
  if draws[0]>draws[1]:dd+=1;ah+=1
  else:ad+=1;dh+=1
 return [0,cursor,ad,dd,10*ar-ad,10*dr-dd,ah+dh,ah,dh,cursor-start,int(dd==10*dr)]

def cases():
 rng=random.Random(0x506af0);rows=[]
 for ar,dr,o,d in itertools.product(range(1,5),range(1,5),[0,1,2,256,32768,2147483647],[0,1,2,256,32768,2147483647]):
  rolls=[rng.randrange(32768)for _ in range(160)];rows.append(([o,d,ar,dr,0,0,160,0],rolls))
 for _ in range(6000):
  ar=rng.randrange(1,5);dr=rng.randrange(1,5)
  v=[rng.randrange(200000),rng.randrange(200000),ar,dr,rng.randrange(10*ar),rng.randrange(10*dr),160,0]
  rolls=[rng.randrange(32768)for _ in range(160)]
  if rng.randrange(3)==0:
   which=rng.randrange(8)
   v[which]=rng.choice([-1,0,1,4,5,9,10,39,40,159,160,161,2147483647])
   v[6]=max(0,min(160,v[6]));v[7]=max(0,v[7])
  if rng.randrange(20)==0:rolls[rng.randrange(160)]=65535
  rows.append((v,rolls))
 # Equal draws: defender wins every tie; no-draw domains leave cursor unchanged.
 for o,d in [(0,0),(1,1),(2,2),(100,100)]:
  rows.append(([o,d,4,4,39,0,160,7],[0]*160))
 # Deliberately short streams and partial round must not advance the public cursor.
 for count in range(25):rows.append(([5,5,1,1,0,0,count,0],[0]*160))
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='c++');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 exe=a.out/'combat-rounds-probe';cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',str(HERE/'combat_rounds.cpp'),str(HERE/'combat_rounds_probe.cpp'),'-o',str(exe)]
 subprocess.run(cmd,check=True);rows=cases();controls=b''.join(struct.pack('<8i160H',*v,*rolls)for v,rolls in rows);(a.out/'controls.bin').write_bytes(controls)
 run=subprocess.run([str(exe)],input=controls,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),check=True,timeout=60)
 expected_bytes=b''.join(struct.pack('<11I',*expected(v,rolls))for v,rolls in rows)
 assert run.stdout==expected_bytes, next((i,v,struct.unpack_from('<11I',run.stdout,44*i),expected(v,rolls))for i,(v,rolls)in enumerate(rows)if run.stdout[44*i:44*(i+1)]!=expected_bytes[44*i:44*(i+1)])
 (a.out/'expected.bin').write_bytes(expected_bytes)
 counts={};
 for v,rolls in rows:
  e=expected(v,rolls)[0];counts[e]=counts.get(e,0)+1
 report=dict(cases=len(rows),outcomes=counts,outputSHA256=hashlib.sha256(expected_bytes).hexdigest(),aliasControls='stream, result and roll input overlaps reject; cursor/output preserve on failures',native='UNRUN',originalExecutable='UNRUN',rng='explicit recorded 15-bit CRT draws; engine seed generator not claimed')
 (a.out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
