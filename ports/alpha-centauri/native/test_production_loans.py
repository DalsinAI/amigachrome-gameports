#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
SRC=Path(__file__).resolve().parent

def model(h):
 phase,alive=h[:2];credits=h[2:10].copy();balances=h[10:18].copy();payment=h[18:26];diplo=h[26:34];sanctions=h[34:42]
 def result(e):return struct.pack('<I',e)+bytes([90])*68
 if not 0<=phase<8 or not 0<=alive<=255:return result(1)
 def check(n):
  if not -2**31<=n<2**31:raise OverflowError
  return n
 try:
  for other in range(1,8):
   if other==phase or not(alive&(1<<phase) and alive&(1<<other)) or not balances[other] or sanctions[other]:continue
   if diplo[other]&16:balances[other]=check(balances[other]+payment[other])
   else:
    paid=min(credits[phase],payment[other]);credits[other]=check(credits[other]+paid);balances[other]=check(balances[other]-paid);credits[phase]=check(credits[phase]-paid)
    if paid<payment[other]:balances[other]=check(balances[other]+check(payment[other]-paid))
 except OverflowError:return result(2)
 return struct.pack('<I17i',0,*credits,*balances,credits[phase])

def contexts():
 rng=random.Random(2026102614);out=[]
 for phase in range(8):
  for alive in range(256):
   out.append([phase,alive,*[rng.randrange(-20,101) for _ in range(8)],*[rng.randrange(-30,101) for _ in range(8)],*[rng.randrange(-20,31) for _ in range(8)],*[rng.getrandbits(32) for _ in range(8)],*[rng.choice([0,0,0,-1,1,100]) for _ in range(8)]])
 for phase in range(8):
  for reserves in [-2**31,-99,-1,0,1,99,2**31-1]:
   for balance in [-2**31,-1,0,1,20,2**31-1]:
    for scheduled in [-2**31,-30,-1,0,1,30,2**31-1]:
     for war in [0,16]:
      h=[phase,255,*([0]*40)];h[2+phase]=reserves;other=1 if phase!=1 else 2;h[10+other]=balance;h[18+other]=scheduled;h[26+other]=war;out.append(h)
 # Independent ordered multi-creditor cash exhaustion, recipient overflow,
 # creditor0/self exclusion, sanctioned lenders, and negative payment branches.
 for _ in range(4096):
  out.append([rng.randrange(8),255,*[rng.choice([-2**31,-1,0,1,2**31-1]) for _ in range(8)],*[rng.randrange(-100,101) for _ in range(8)],*[rng.randrange(-100,101) for _ in range(8)],*[rng.choice([0,16]) for _ in range(8)],*[rng.choice([0,0,-1,1]) for _ in range(8)]])
 good=[1,255,*([0]*40)]
 for field,values in [(0,[-1,8]),(1,[-1,256])]:
  for v in values:h=good.copy();h[field]=v;out.append(h)
 return out

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);rows=contexts();raw=b''.join(struct.pack('<42I',*[x&0xffffffff for x in h]) for h in rows);(out/'inputs.bin').write_bytes(raw);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);command=[a.cc,*flags,str(SRC/'production_loans.cpp'),str(SRC/'production_loans_probe.cpp'),'-o',str(out/'probe')];subprocess.run(command,check=True)
 r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(out/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=120);expected=b''.join(map(model,rows));(out/'outputs.bin').write_bytes(r.stdout);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==expected,next((i for i,(x,y) in enumerate(zip(r.stdout,expected)) if x!=y),('length',len(r.stdout),len(expected)));errors=[struct.unpack_from('<I',r.stdout,i*72)[0] for i in range(len(rows))];receipt=dict(cases=len(rows),accepted=errors.count(0),invalid=errors.count(1),overflow=errors.count(2),inputSHA256=hashlib.sha256(raw).hexdigest(),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=command);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
