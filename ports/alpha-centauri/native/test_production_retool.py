#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original production retool classification/loss, explicit input records."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent

def contexts():
 for profile in [0,1]:
  repeat,project=(34,39) if not profile else (65,70)
  items=[-project-1,-project,-project+1,-repeat,-repeat+1,-10,-1,0,511]
  for strict in range(4):
   for human in range(2):
    for mode in range(4):
     for previous in items:
      for queue in items:
       mask=(1<<10) if mode&1 else 0
       if mode&2 and -repeat<queue<0:mask|=1<<(-queue)
       for accumulated in [-1,10,11,111,21474836,2147483647]:
        for penalty in [0,50,100]:yield profile,strict,penalty,10,human,previous,queue,accumulated,mask&0xffffffff,(mask>>32)&0xffffffff,(mask>>64)&0xffffffff
 base=[0,2,50,10,1,0,-1,111,0,0,0]
 for field,value in [(0,2),(1,-1),(1,4),(2,-1),(2,101),(3,-1),(3,1001),(4,2),(6,-2147483648)]:
  row=base.copy();row[field]=value;yield tuple(row)
 for exemption in [0,1,1000]:
  for accumulated in [exemption-1,exemption,exemption+1]:
   for penalty in [1,99]:
    row=base.copy();row[2]=penalty;row[3]=exemption;row[7]=accumulated;yield tuple(row)

def classify(row,item):
 profile,strict,penalty,exemption,human,previous,queue,accumulated,*words=row;repeat,project=(34,39) if not profile else (65,70);mask=sum(w<<(32*i) for i,w in enumerate(words))
 if mask&(1<<10) and strict:strict=1
 if item<0 and -repeat<queue<0 and mask&(1<<(-queue)):return -1
 if strict==0:return 0
 if strict==1:return 0 if item>=0 else 1 if item<=-project else 2 if item<=-repeat else 3
 return -project if strict==2 and item<=-project else item

def model(row):
 profile,strict,penalty,exemption,human,previous,queue,accumulated,*_=row;s=0x31415926
 if profile not in [0,1] or not 0<=strict<=3 or not 0<=penalty<=100 or not 0<=exemption<=1000 or human not in [0,1] or queue==-2147483648:return 1,s,s,s
 a,b=classify(row,previous),classify(row,queue);loss=0
 if penalty and human and a!=b and accumulated>exemption:
  difference=accumulated-exemption;product=difference*(100-penalty)
  if product>2147483647:return 2,s,s,s
  loss=difference-product//100
 return 0,a,b,loss

def wire(row):return struct.pack('<8i3I',*row)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);rows=list(contexts());data=b''.join(wire(r) for r in rows);expected=b''.join(struct.pack('<4i',*model(r)) for r in rows);(out/'inputs.bin').write_bytes(data);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'retool-probe';cmd=[a.cc,*flags,str(SRC/'production_retool.cpp'),str(SRC/'production_retool_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True);r=subprocess.run([*runner,str(exe),str(out/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=120);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(expected)
 for i,row in enumerate(rows):assert r.stdout[16*i:16*(i+1)]==expected[16*i:16*(i+1)],(row,struct.unpack_from('<4i',r.stdout,16*i),model(row))
 (out/'outputs.bin').write_bytes(r.stdout);receipt=dict(cases=len(rows),accepted=sum(model(r)[0]==0 for r in rows),invalid=sum(model(r)[0]==1 for r in rows),overflow=sum(model(r)[0]==2 for r in rows),inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
