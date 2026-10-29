#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,itertools,json,os,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
S=0x31415926

def model(r):
 p,fac,clean,item,size,flags,gov,human,admitted=r
 fail=lambda e:(e,S,S,S,1,1)
 if p not in (0,1) or not 2<=fac<(34 if p==0 else 65) or not 0<=size<=9 or human not in (0,1) or admitted not in (0,1):return fail(1)
 if fac in (13,14,31,32):
  if clean==2147483647:return fail(2)
  clean=clean+1 if clean else 0;flags&=~0x4000
 manage=bool(gov&1 and gov&0x80000000);reset=not human or manage and not admitted;fallback=bool(human and not manage and item==-fac and size==0)
 return 0,clean,(-38 if p==0 else -69) if fallback else item,flags,int(reset),int(fallback)

def contexts():
 for p in range(2):
  for fac in range(2,34 if p==0 else 65):
   for clean,gov,human,admitted,size,item,flags in itertools.product([-2147483648,-1,0,1,2147483646,2147483647],[0,1,0x80000000,0x80000001],range(2),range(2),[0,1,9],[-fac,0,512],[0,0xffffffff]):yield p,fac,clean,item,size,flags,gov,human,admitted
 template=[0,13,0,-13,0,0,0,1,0]
 for i,values in {0:[-1,2],1:[-1,0,1,34,65],4:[-1,10,2147483647],7:[-1,2],8:[-1,2]}.items():
  for v in values:r=template.copy();r[i]=v;yield r

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);rows=list(contexts());data=b''.join(struct.pack('<9I',*[v&0xffffffff for v in r]) for r in rows);want=b''.join(struct.pack('<6I',*[v&0xffffffff for v in model(r)]) for r in rows);(out/'inputs.bin').write_bytes(data);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 exe=out/'completion-tail-probe';cmd=[a.cc,*flags,str(HERE/'completion_tail.cpp'),str(HERE/'completion_tail_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True);r=subprocess.run([*runner,str(exe),str(out/'inputs.bin')],capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want;answers=[model(r) for r in rows];receipt=dict(cases=len(rows),accepted=sum(x[0]==0 for x in answers),invalid=sum(x[0]==1 for x in answers),overflow=sum(x[0]==2 for x in answers),resetRequired=sum(x[0]==0 and x[4] for x in answers),fallback=sum(x[0]==0 and x[5] for x in answers),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd);(out/'outputs.bin').write_bytes(r.stdout);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
