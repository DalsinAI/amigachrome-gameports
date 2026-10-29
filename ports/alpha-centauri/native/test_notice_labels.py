#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,itertools,json,os,random,shlex,struct,subprocess

def model(h):
 v,fac,first,admitted,size,gov,warn,flags,slot=h
 if v not in (0,1) or not 2<=fac<=(33 if v==0 else 41) or (v and fac==40) or first not in (0,1) or admitted not in (0,1) or not 0<=size<=9:return struct.pack('<5I',1,0x12345678,0x12345678,1,1)+b'Z'*10
 managing=gov&0x80000001==0x80000001;label=('PRODUCEX' if first else 'PRODUCE')+('Q' if admitted else 'G' if managing else '');mask=int(bool(first or (not admitted and not managing) or warn&0x2000));return struct.pack('<5I',0,mask,(size+1 if admitted else slot)&0xffffffff,int(bool(v and flags&128)),admitted)+label.encode().ljust(10,b'\0')

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;rng=random.Random(719424);rows=[];answers=[]
 for v,first,admitted,size,gov,warn,flags,slot in itertools.product(range(2),range(2),range(2),range(10),[0,1,0x80000000,0x80000001,0xffffffff],[0,1,0x2000,0xffffffff],[0,128],[-2147483648,0,2147483647]):
  h=[v,2,first,admitted,size,gov,warn,flags,slot];rows.append(struct.pack('<9I',*[x&0xffffffff for x in h]));answers.append(model(h))
 for v in range(2):
  for fac in range(2,34 if not v else 42):
   for i in range(16):
    h=[v,fac,i&1,(i>>1)&1,rng.randrange(10),rng.choice([0,1,0x80000000,0x80000001,0xffffffff]),rng.choice([0,0x2000,0xffffffff]),rng.getrandbits(32),rng.randrange(-(1<<31),1<<31)];rows.append(struct.pack('<9I',*[x&0xffffffff for x in h]));answers.append(model(h))
 good=[0,2,1,1,0,0,0,0,0]
 for field,values in [(0,[2,0xffffffff]),(1,[-1,0,1,34,42]),(2,[2,0xffffffff]),(3,[2,0xffffffff]),(4,[-1,10,2147483647])]:
  for value in values:
   h=good.copy();h[field]=value;rows.append(struct.pack('<9I',*[x&0xffffffff for x in h]));answers.append(model(h))
 raw=b''.join(rows);want=b''.join(answers);(out/'inputs.bin').write_bytes(raw);cxx=shlex.split(a.cc);cross=a.m68k_linux or 'm68k' in cxx[0];cmd=cxx+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if cross else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/'notice_labels.cpp'),str(src/'notice_labels_probe.cpp'),'-o',str(out/'probe')];subprocess.run(cmd,check=True);run=(['qemu-m68k','-cpu','m68040'] if cross else [])+[str(out/'probe'),str(out/'inputs.bin')];r=subprocess.run(run,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=180);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want;(out/'outputs.bin').write_bytes(r.stdout);receipt=dict(cases=len(rows),rejected=sum(struct.unpack_from('<I',r)[0]!=0 for r in answers),numericSlotWrites=sum(struct.unpack_from('<I',r)[0]==0 and struct.unpack_from('<I',r,16)[0] for r in answers),inputSHA256=hashlib.sha256(raw).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
