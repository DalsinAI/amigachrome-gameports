#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,itertools,json,os,random,shlex,struct,subprocess
SIZE=0x4f4

def model(h,record):
 v,flags,prefs,state,multi,new,size=h
 if v not in (0,1) or new not in (0,1) or size!=SIZE:return (1,0x12345678,1,1)
 blocked=bool(multi) or bool(v and flags&128)
 if new:blocked|=bool(prefs&0x1000000) or bool(state&16) or state&9==9
 if blocked:return (0,0,0,0)
 count=sum(struct.unpack_from('<I',record,i*56+16)[0]!=0 for i in range(15))
 return (0,count,1,int(count!=0))

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;rng=random.Random(561839);rows=[];answers=[]
 for v,flags,prefs,state,multi,new,pattern in itertools.product(range(2),[0,128],[0,0x1000000],[0,1,8,9,16,17,24,25,0xffffffff],[0,1,-1,-2147483648],range(2),range(7)):
  rec=bytearray(rng.randbytes(SIZE))
  for i in range(21):
   yes=pattern==1 and i==0 or pattern==2 and i==14 or pattern==3 and i<7 or pattern==4 and i<15 or pattern==5 and i>=15 or pattern==6
   struct.pack_into('<I',rec,i*56+16,rng.choice([1,0xffffffff,0x80000000]) if yes else 0)
  h=[v,flags,prefs,state,multi,new,SIZE];rows.append(struct.pack('<7I',*[x&0xffffffff for x in h])+rec);answers.append(model(h,rec))
 good=[0,0,0,0,0,1,SIZE];rec=bytes(SIZE)
 for field,values in [(0,[2,0xffffffff]),(5,[2,0xffffffff]),(6,[0,SIZE-1,SIZE+1,0xffffffff])]:
  for n in values:
   h=good.copy();h[field]=n;rows.append(struct.pack('<7I',*[x&0xffffffff for x in h])+rec);answers.append(model(h,rec))
 raw=b''.join(rows);want=b''.join(struct.pack('<4I',*x) for x in answers);(out/'inputs.bin').write_bytes(raw);cxx=shlex.split(a.cc);cross=a.m68k_linux or 'm68k' in cxx[0];cmd=cxx+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if cross else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/'monument_presentation.cpp'),str(src/'monument_presentation_probe.cpp'),'-o',str(out/'probe')];subprocess.run(cmd,check=True);run=(['qemu-m68k','-cpu','m68040'] if cross else [])+[str(out/'probe'),str(out/'inputs.bin')];r=subprocess.run(run,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=180);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want;(out/'outputs.bin').write_bytes(r.stdout);receipt=dict(cases=len(rows),rejected=sum(x[0]!=0 for x in answers),countWritten=sum(not x[0] and x[2] for x in answers),initializationRequired=sum(not x[0] and x[3] for x in answers),directGuards=3,inputSHA256=hashlib.sha256(raw).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
