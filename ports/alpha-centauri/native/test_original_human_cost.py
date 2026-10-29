#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original human mineral/nutrient factors, independent integer model."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent

def contexts():
 for size in range(7):
  for growth in [-2147483648,-7,-2,-1,0,3,5,6,2147483643]:
   for industry in [-2147483648,-8,-7,-3,-2,-1,0,1,2,3,4,5,2147483647]:
    for flags in range(4):
     for n,m in [(1,1),(10,10),(100,100)]:yield n,m,size,growth,industry,flags
 for n,m,size,flags in [(0,10,2,0),(101,10,2,0),(10,0,2,0),(10,101,2,0),(10,10,-1,0),(10,10,7,0),(10,10,2,-1),(10,10,2,4)]:yield n,m,size,0,0,flags
 for growth,flags in [(2147483647,1),(2147483646,1),(2147483647,2),(2147483644,3),(2147483645,3)]:yield 10,10,2,growth,0,flags

def model(n,m,size,growth,industry,flags):
 sentinel=0x31415926
 if not(1<=n<=100 and 1<=m<=100 and 0<=size<=6 and 0<=flags<=3):return 1,sentinel,sentinel
 growth+=2*bool(flags&1)+2*bool(flags&2)
 if growth>2147483647:return 2,sentinel,sentinel
 scale=[8,9,10,10,10,10,10][size];n=n*scale//10;m=m*scale//10
 n=(n*(10-max(-2,min(5,growth)))+9)//10
 industry_ratio={-7:(13,9,10),-6:(13,9,10),-5:(13,9,10),-4:(13,9,10),-3:(13,9,10),-2:(6,4,5),-1:(11,9,10),0:(1,0,1),1:(9,9,10),2:(4,4,5),3:(7,9,10),4:(3,4,5)}
 a,b,c=industry_ratio.get(industry,(1,1,2));return 0,n,(a*m+b)//c

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 rows=list(contexts());data=b''.join(struct.pack('<6i',*r) for r in rows);expected=b''.join(struct.pack('<3i',*model(*r)) for r in rows);(out/'inputs.bin').write_bytes(data);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'human-cost-probe';cmd=[a.cc,*flags,str(SRC/'original_human_cost.cpp'),str(SRC/'original_human_cost_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 r=subprocess.run([*runner,str(exe),str(out/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=120);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(expected)
 for i,row in enumerate(rows):assert r.stdout[12*i:12*(i+1)]==expected[12*i:12*(i+1)],(row,struct.unpack_from('<3i',r.stdout,12*i),model(*row))
 (out/'outputs.bin').write_bytes(r.stdout);receipt=dict(cases=len(rows),accepted=sum(model(*r)[0]==0 for r in rows),invalid=sum(model(*r)[0]==1 for r in rows),overflow=sum(model(*r)[0]==2 for r in rows),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),inputSHA256=hashlib.sha256(data).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd)
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
