#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Ordinary install PREFIX only: independent arithmetic/bitmap oracle."""
import argparse,hashlib,itertools,json,os,random,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent
S=0x31415926

def model(row):
 profile,id,stock,cost,surplus,exemption,human,flags,*words=row;fail=(1,S,S,S,0x5a5a5a5a,0x5a5a5a5a,0x5a5a5a5a,1)
 if profile not in (0,1) or not 2<=id<(34 if profile==0 else 65) or cost<0 or stock<cost or not 0<=exemption<=1000 or human not in (0,1):return fail
 mask=sum(w<<(32*i) for i,w in enumerate(words))
 if mask&(1<<id):return fail
 remaining=stock-cost;cap=min(exemption,surplus)
 if human and remaining>cap:remaining=max(0,cap)
 mask|=1<<id
 return 0,(flags|0x800000)&0x7fffffff,remaining,remaining,*[(mask>>(32*i))&0xffffffff for i in range(3)],int(id==8)

def contexts():
 rng=random.Random(928152)
 for profile in range(2):
  for id in range(2,34 if profile==0 else 65):
   for human,stock,cost,surplus,exemption,flags in itertools.product(range(2),[0,11,111,2147483647],[0,10],[-1,0,9,11],[0,10,1000],[0,0x800000,0xffffffff]):
    mask=rng.randrange(1<<96)&~(1<<id);yield profile,id,stock,cost,surplus,exemption,human,flags,*[(mask>>(32*i))&0xffffffff for i in range(3)]
 base=[0,3,111,100,13,10,1,0,0,0,0]
 for i,values in {0:[-1,2],1:[-1,0,1,34,65,2147483647],2:[-2147483648,-1,99],3:[-1,112,2147483647],5:[-1,1001],6:[-1,2]}.items():
  for v in values:r=base.copy();r[i]=v;yield tuple(r)
 for profile in [0,1]:
  for id in range(2,34 if profile==0 else 65):
   r=base.copy();r[0]=profile;r[1]=id;mask=1<<id;r[8:11]=[(mask>>(32*i))&0xffffffff for i in range(3)];yield tuple(r)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);rows=list(contexts());data=b''.join(struct.pack('<7i4I',*r) for r in rows);want=b''.join(struct.pack('<8I',*[x&0xffffffff for x in model(r)]) for r in rows);(out/'inputs.bin').write_bytes(data);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 exe=out/'facility-install-prefix-probe';cmd=[a.cc,*flags,str(SRC/'facility_install_prefix.cpp'),str(SRC/'facility_install_prefix_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True);r=subprocess.run([*runner,str(exe),str(out/'inputs.bin')],capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(want)
 for i,row in enumerate(rows):assert r.stdout[i*32:(i+1)*32]==want[i*32:(i+1)*32],(row,model(row),struct.unpack_from('<8I',r.stdout,i*32))
 (out/'outputs.bin').write_bytes(r.stdout);results=[model(r) for r in rows];receipt=dict(cases=len(rows),accepted=sum(r[0]==0 for r in results),invalid=sum(r[0]!=0 for r in results),networkNodeTailRequired=sum(r[0]==0 and r[-1] for r in results),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),inputSHA256=hashlib.sha256(data).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
