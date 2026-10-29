#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent arithmetic oracle for one eligible fresh commerce partner."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
SENTINEL=0x5a5a5a5a
def pack(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))
def expected(words):
 ours,theirs,technologies,ourbonus,theirbonus,globalpact,pact,ourgov,theirgov=words
 fail=lambda code:[code]+[SENTINEL]*3
 if min(ours,theirs)<0 or not 0<=technologies<=89 or min(ourbonus,theirbonus)<-1:return fail(1)
 def check(n):
  if not -2147483648<=n<=2147483647:raise OverflowError
  return n
 try:
  value=check(check(ours+theirs)+7)//8
  if globalpact:value=check(2*value)
  divisor=technologies+1;im=check(divisor//2+check(value*check(ourbonus+1)))//divisor;ex=check(divisor//2+check(value*check(theirbonus+1)))//divisor
  if not pact:im//=2;ex//=2
  im=check(im+bool(ourgov));ex=check(ex+bool(theirgov));return[0,value,im,ex]
 except OverflowError:return fail(3)
def cases():
 rng=random.Random(0xC011E2CE);rows=[]
 for ours in[0,1,6,7,8,9,31,9999]:
  for theirs in[0,1,6,7,8,9,31,9999]:
   for tech in[0,1,3,7,89]:
    for flags in range(16):rows.append([ours,theirs,tech,rng.randrange(-1,12),rng.randrange(-1,12),flags&1,(flags>>1)&1,(flags>>2)&1,(flags>>3)&1])
 for _ in range(2000):rows.append([rng.randrange(100000),rng.randrange(100000),rng.randrange(90),rng.randrange(-1,100),rng.randrange(-1,100),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2)])
 for index,value in[(0,-1),(1,-1),(2,-1),(2,90),(3,-2),(4,-2),(0,2147483647),(1,2147483647),(3,2147483647),(4,2147483647),(3,1000000000),(4,1000000000)]:
  row=[23,31,7,4,3,1,1,1,0];row[index]=value;rows.append(row)
 return rows
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--binary',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.binary or a.out/'base-commerce-probe';cmd=[]
 if not a.binary:
  cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',str(HERE/'base_commerce.cpp'),str(HERE/'base_commerce_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 rows=cases();path=a.out/'cases.bin';path.write_bytes(pack([len(rows)])+b''.join(pack(row)for row in rows));wanted=b''.join(pack(expected(row))for row in rows);run=subprocess.run([str(exe),str(path)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert run.returncode==0,(run.returncode,run.stderr);assert run.stdout==wanted;(a.out/'results.bin').write_bytes(run.stdout);receipt=dict(cases=len(rows),bytes=len(wanted),sha256=hashlib.sha256(wanted).hexdigest(),command=cmd,native='UNRUN');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(rows)} commerce partner cases passed')
if __name__=='__main__':main()
