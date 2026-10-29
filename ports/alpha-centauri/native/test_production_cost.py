#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Explicit original unit production pricing and checked mineral threshold."""
import argparse,hashlib,itertools,json,os,random,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent
S=0x31415926

def model(r):
 v,idx,c,bc,t,o,p,d,free,done,base,pit,skunk,land,sea,air,req,factor=r
 e=0;out=[S,S,1]
 if v not in (0,1) or not 0<=idx<512 or not 0<=c<=255 or bc<0 or not 0<=t<=2 or not -128<=o<=127 or not 0<=p<=255 or not 0<=d<=5 or any(x not in (0,1) for x in (free,done,base,pit,skunk)) or any(not 0<=x<=1000 for x in (land,sea,air)):e=1
 elif v and idx<64 and (o<0 or idx==15) and not base:e=1
 else:
  if v and idx<64 and (o<0 or idx==15) and pit:c=c*3//4
  if p==8 and base and c==0:c=1
  surcharge=0
  if idx>=64 and not done:
   percent=(land,sea,air)[t] if not free and d>=2 else 0
   product=bc*percent+50
   if product>2147483647:e=2
   else:
    surcharge=0 if base and skunk else product//100
    if c+surcharge>2147483647:e=2
  if not e:out=[c+surcharge,surcharge,int(surcharge!=0)]
 q=1 if req<0 or factor<0 else 2 if req*factor>2147483647 else 0
 return (e,*out,q,S if q else req*factor)

def contexts():
 for v,idx,c,t,d,bits in itertools.product(range(2),[0,15,63,64,65,320,511],[0,1,3,4,255],range(3),[0,1,2,5],range(32)):
  free,done,base,pit,skunk=((bits>>i)&1 for i in range(5))
  yield (v,idx,c,37,t,-1 if idx%2 else 1,8 if c%2 else 0,d,free,done,base,pit,skunk,50,100,150,c,10)
 r=[0,64,255,37,0,1,0,2,0,0,1,0,0,50,100,150,255,10]
 for i,values in {0:[-1,2],1:[-1,512],2:[-1,256],3:[-1,2147483647],4:[-1,3],5:[-129,128],6:[-1,256],7:[-1,6],8:[-1,2],9:[-1,2],10:[-1,2],11:[-1,2],12:[-1,2],13:[-1,1001],14:[-1,1001],15:[-1,1001],16:[-1,2147483647],17:[-1,2147483647]}.items():
  for x in values:
   q=r.copy();q[i]=x;yield tuple(q)
 for bc,pct,skunk,done in itertools.product([0,1,2147483,2147484,21474836,21474837,2147483597,2147483647],[0,1,50,100,1000],range(2),range(2)):
  q=r.copy();q[3]=bc;q[13]=pct;q[12]=skunk;q[9]=done;yield tuple(q)
 rng=random.Random(173810)
 for _ in range(2000):
  q=r.copy();q[0]=rng.randrange(2);q[1]=rng.randrange(512);q[2]=rng.randrange(256);q[3]=rng.randrange(2147483648);q[4]=rng.randrange(3);q[5]=rng.randrange(-128,128);q[6]=rng.randrange(256);q[7]=rng.randrange(6)
  q[8:13]=[rng.randrange(2) for _ in range(5)];q[13:16]=[rng.randrange(1001) for _ in range(3)];q[16]=rng.choice([0,1,255,2147483647]);q[17]=rng.choice([0,1,10,1000]);yield tuple(q)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 rows=list(contexts());data=b''.join(struct.pack('<18i',*r) for r in rows);expected=b''.join(struct.pack('<6i',*model(r)) for r in rows);(out/'inputs.bin').write_bytes(data)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'production-cost-probe';cmd=[a.cc,*flags,str(SRC/'production_cost.cpp'),str(SRC/'production_cost_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 r=subprocess.run([*runner,str(exe),str(out/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=180);assert r.returncode==0,(r.returncode,r.stderr)
 assert len(r.stdout)==len(expected)
 for i,row in enumerate(rows):assert r.stdout[i*24:(i+1)*24]==expected[i*24:(i+1)*24],(row,model(row),struct.unpack_from('<6i',r.stdout,i*24))
 (out/'outputs.bin').write_bytes(r.stdout);results=[model(r) for r in rows];receipt=dict(cases=len(rows),accepted=sum(r[0]==0 for r in results),invalid=sum(r[0]==1 for r in results),overflow=sum(r[0]==2 for r in results),thresholdAccepted=sum(r[4]==0 for r in results),thresholdInvalid=sum(r[4]==1 for r in results),thresholdOverflow=sum(r[4]==2 for r in results),inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
