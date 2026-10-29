#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,shlex,struct,subprocess

def model(h,announced):
 v,fac,f,local,state,warn,after,pref,closed,humans=h
 if v not in (0,1) or not 2<=fac<=(33 if v==0 else 41) or not 0<=f<8 or not 0<=local<8:return struct.pack('<5I',1,4,1,1,1)+bytes([90])*12
 ann=bytearray(announced);blurb=voice=fade=0
 if v and fac==40:route=4
 elif f!=local:route=int(fac>=(33 if v==0 else 40))
 else:
  first=not (ann[fac//8]&(1<<(fac%8))) and not(state&1) and warn&1
  route=3 if first else 2
  if first:
   blurb=int(bool(after&1));voice=int(bool(pref&0x4000 and humans&(1<<f)));fade=int(closed&0x24000==0x24000)
   if blurb:ann[fac//8]|=1<<(fac%8)
 return struct.pack('<5I',0,route,blurb,voice,fade)+ann

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;rng=random.Random(624718);rows=[];answers=[]
 for v in range(2):
  for fac in range(2,34 if not v else 42):
   for f in range(8):
    for mode in range(64):
     ann=bytearray(rng.randbytes(12));mask=1<<(fac%8);ann[fac//8]=(ann[fac//8]|mask) if mode&2 else (ann[fac//8]&~mask)
     h=[v,fac,f,f if mode&1 else (f+1)%8,0x100 if mode&4==0 else 0x101,(0x2000 if mode&8==0 else 0x2001),(0x400 if mode&16==0 else 0x401),[0,0x4000,0x20000,0x24000][(mode//2+f)%4],[0,0x4000,0x20000,0x24000][(mode//3+fac)%4],(1<<f) if mode&32 else 0]
     rows.append(struct.pack('<10I',*h)+ann);answers.append(model(h,ann))
 h0=[0,2,1,1,0,1,1,0x4000,0x24000,2];ann=bytes(12)
 for field,values in [(0,[2,0xffffffff]),(1,[-1,0,1,34,41,42,2147483647]),(2,[-1,8]),(3,[-1,8])]:
  for value in values:
   h=h0.copy();h[field]=value;rows.append(struct.pack('<10I',*[x&0xffffffff for x in h])+ann);answers.append(model(h,ann))
 raw=b''.join(rows);want=b''.join(answers);(out/'inputs.bin').write_bytes(raw);cxx=shlex.split(a.cc);cross=a.m68k_linux or 'm68k' in cxx[0];cmd=cxx+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if cross else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/'facility_notice.cpp'),str(src/'facility_notice_probe.cpp'),'-o',str(out/'probe')];subprocess.run(cmd,check=True);run=(['qemu-m68k','-cpu','m68040'] if cross else [])+[str(out/'probe'),str(out/'inputs.bin')];r=subprocess.run(run,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=180);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want;(out/'outputs.bin').write_bytes(r.stdout);decoded=[struct.unpack_from('<5I',x) for x in answers];receipt=dict(cases=len(rows),rejected=sum(x[0]!=0 for x in decoded),routes={str(i):sum(not x[0] and x[1]==i for x in decoded) for i in range(5)},blurbs=sum(not x[0] and x[2] for x in decoded),voiceLoads=sum(not x[0] and x[3] for x in decoded),voiceFades=sum(not x[0] and x[4] for x in decoded),inputSHA256=hashlib.sha256(raw).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
