#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from human_allocation_reference import expected
HERE=Path(__file__).resolve().parent
def cases():
 rows=[]
 for edition in [0,1]:
  for n in range(1200):
   rng=random.Random(0x574f524b+n);pop=[0,1,2,3,5,16,17,20,21,31,63,127][n%12];workers=rng.randrange(1,min(21,pop+1)+1);special=rng.randrange(pop+2-workers);worked=(1<<workers)-1;blocked=rng.getrandbits(21)&~1 if n%4==0 else 0
   citizens=[]
   for i in range(7):citizens += [rng.randrange(2),rng.randrange(-2,7),rng.randrange(-2,5),rng.randrange(-2,8)]
   tiles=[]
   for tile in range(21):tiles += [rng.randrange(5),rng.randrange(5),rng.randrange(5)]
   rows.append([edition,0,pop,2,rng.randrange(8),[-38,-39,-40,-69,-70,-71,0,511][n%8],special,worked,blocked,*[rng.randrange(pop+1) for _ in range(3)],rng.getrandbits(32),rng.getrandbits(32),rng.randrange(1,26),*citizens,*tiles])
 seed=rows[17][:]
 for index,value in [(0,2),(1,1),(2,-1),(2,128),(3,0),(3,100),(6,-1),(6,128),(7,0x80000001),(8,0x80000000),(9,-1),(9,128),(14,-1),(15,2)]:
  r=seed[:];r[index]=value;rows.append(r)
 for index in [4,43,44,45,46,47,48,49,50,51]:
  r=seed[:];r[index]=2147483647;rows.append(r)
 return rows

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.out/'human-allocation';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 command=[a.cc,*flags,str(HERE/'human_worker_allocation.cpp'),str(HERE/'human_worker_allocation_probe.cpp'),'-o',str(exe)];subprocess.run(command,check=True);rows=cases();answers=[expected(r) for r in rows];data=''.join(' '.join(map(str,r))+'\n' for r in rows).encode();(a.out/'input.txt').write_bytes(data);wanted=b''.join(struct.pack('<11I',*[v&0xffffffff for v in row]) for row in answers);r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(a.out/'actual.bin').write_bytes(r.stdout);(a.out/'expected.bin').write_bytes(wanted);(a.out/'stderr.txt').write_bytes(r.stderr)
 assert r.returncode==0 and r.stdout==wanted,(r.returncode,r.stderr,next((i for i,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None))
 report=dict(cases=len(rows),accepted=sum(x[0]==0 for x in answers),rejected=sum(x[0]!=0 for x in answers),exchanges=sum(x[0]==0 and x[9] for x in answers),foodDeficits=sum(x[0]==0 and x[10] for x in answers),outputSHA256=hashlib.sha256(wanted).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=command,native='UNRUN');(a.out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='command'}))
if __name__=='__main__':main()
