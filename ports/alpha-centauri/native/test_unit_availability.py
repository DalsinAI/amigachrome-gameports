#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,random,struct,subprocess,itertools
from pathlib import Path
SRC=Path(__file__).resolve().parent

def model(r):
 v,id,fac,flags,obsolete,plan,armor,weapon,ability,rules,exp,base,triad,coast,tech,p1,p2,owners=r
 if v not in (0,1) or not 0<=id<512 or not 0<=fac<8 or not 0<=flags<=65535 or not 0<=obsolete<=255 or not -128<=plan<=127 or not 0<=armor<=255 or not 0<=weapon<=255 or exp not in (0,1) or base not in (0,1) or not 0<=triad<=2 or coast not in (0,1) or not -32768<=tech<=32767 or not 0<=owners<=255:return 1,1
 tech_ok=fac>0 and (tech==-1 or 0<=tech<88 and p1>-2 and (p2>-2 or p1==-1) and owners&(1<<fac))
 ok=bool(flags&1) and not obsolete&(1<<fac) and (id>=64 or tech_ok) and not (plan==8 and rules&0x400000) and not (base and triad==1 and not coast) and (id<64 or id//64==fac)
 if v==1 and not exp and (armor>9 or weapon in (12,13,14,24,25) or ability&0x1f000000 or 14<=id<=20):ok=False
 return 0,int(ok)

def contexts():
 rng=random.Random(68194);base=[0,63,1,1,0,0,0,0,0,0,1,0,0,0,-1,-1,-1,2]
 for v,id,fac,exp in itertools.product(range(2),range(512),range(8),range(2)):
  r=base.copy();r[0]=v;r[1]=id;r[2]=fac;r[10]=exp;yield r
 for v in range(2):
  for i,values in {1:[-1,0,13,14,15,20,21,63,64,127,128,511,512],2:[-1,0,1,7,8],3:[0,1,2,65535,65536],4:[0,1,2,255,256],5:[-129,-128,0,8,127,128],6:[0,9,10,255,256],7:[0,11,12,13,14,15,23,24,25,26,255,256],8:[0,1,0x1000000,0x2000000,0x4000000,0x8000000,0x10000000,0x1f000000,0xffffffff],9:[0,0x400000,0xffffffff],10:[0,1,2],11:[0,1,2],12:[0,1,2,3],13:[0,1,2],14:[-32769,-32768,-3,-2,-1,0,86,87,88,89,32767,32768],15:[-2147483648,-3,-2,-1,0,87,2147483647],16:[-2147483648,-3,-2,-1,0,87,2147483647],17:[0,1,2,255,256]}.items():
   for value,exp,base_present,triad,coast in itertools.product(values,range(2),range(2),range(3),range(2)):
    r=base.copy();r[0]=v;r[10]=exp;r[11]=base_present;r[12]=triad;r[13]=coast;r[i]=value;yield r
 for _ in range(10000):
  r=base.copy();r[0]=rng.randrange(2);r[1]=rng.randrange(512);r[2]=rng.randrange(8);r[3]=rng.randrange(8);r[4]=rng.randrange(256);r[5]=rng.choice([0,8,12]);r[6]=rng.randrange(12);r[7]=rng.randrange(26);r[8]=rng.randrange(1<<32);r[9]=rng.choice([0,0x400000]);r[10]=rng.randrange(2);r[11]=rng.randrange(2);r[12]=rng.randrange(3);r[13]=rng.randrange(2);r[14]=rng.choice([-3,-2,-1,0,87,88]);r[15]=rng.randrange(-3,2);r[16]=rng.randrange(-3,2);r[17]=rng.randrange(256);yield r

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);rows=list(contexts());data=b''.join(struct.pack('<18I',*[x&0xffffffff for x in r]) for r in rows);want=b''.join(struct.pack('<2I',*model(r)) for r in rows);(out/'inputs.bin').write_bytes(data);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 exe=out/'unit-availability-probe';cmd=[a.cc,*flags,str(SRC/'unit_availability.cpp'),str(SRC/'unit_availability_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True);r=subprocess.run([*runner,str(exe),str(out/'inputs.bin')],capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want;answers=[model(r) for r in rows];receipt=dict(cases=len(rows),accepted=sum(e==0 for e,v in answers),rejected=sum(e!=0 for e,v in answers),available=sum(e==0 and v for e,v in answers),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd);(out/'outputs.bin').write_bytes(r.stdout);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
