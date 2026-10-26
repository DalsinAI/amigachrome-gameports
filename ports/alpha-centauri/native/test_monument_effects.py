#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,random,shlex,struct,subprocess
from pathlib import Path
SIZE=0x27a0;STRIDE=0x4f4

def model(header,name,table):
 v,f,local,event,year,flags,size=header
 if v not in (0,1) or not 0<=f<8 or not 0<=local<8 or event not in (4,12) or size!=SIZE:return struct.pack('<III',1,1,1)+bytes([90])*STRIDE
 r=bytearray(table[f*STRIDE:(f+1)*STRIDE]);off=event*56
 def word(i,n):return struct.unpack_from('<I',table,i*STRIDE+off+n)[0]
 changed=not word(f,16) and not (v==1 and flags&128)
 if changed:
  others=[i for i in range(8) if i!=f and word(i,16)]
  firsts=[i for i in others if word(i,20)]
  struct.pack_into('<I',r,off,year&0xffffffff);struct.pack_into('<III',r,off+12,firsts[-1] if firsts else f,1,int(not others));struct.pack_into('<I',r,0x498,event)
  if event==4:r[0x4c4:0x4dc]=name.split(b'\0')[0].ljust(24,b'\0')
 return struct.pack('<III',0,int(changed),int(changed and f==local))+r

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;rng=random.Random(562724);rows=[];want=[];stats=dict(changed=0,presentation=0,rejected=0)
 for v in range(2):
  for event in (4,12):
   for f in range(8):
    for mode in range(64):
     table=bytearray(rng.randbytes(SIZE));name=bytearray(rng.randbytes(24))
     if mode%25<24:name[mode%25]=0
     for i in range(8):
      # Include negative and noncanonical nonzero markers, multiple or absent firsts.
      achieved=0 if (i==f and mode%4) or (mode&8) else rng.choice([0,1,0xffffffff,0x80000000])
      first=rng.choice([0,1,0xffffffff]);struct.pack_into('<II',table,i*STRIDE+event*56+16,achieved,first)
     h=[v,f,(f if mode&1 else (f+1)%8),event,rng.choice([-2147483648,-1,0,2100,2147483647]),128 if mode&2 else 0,SIZE]
     ans=model(h,name,table);stats['changed']+=struct.unpack_from('<I',ans,4)[0];stats['presentation']+=struct.unpack_from('<I',ans,8)[0];rows.append(struct.pack('<7I',*[x&0xffffffff for x in h])+name+table);want.append(ans)
 good=[0,1,1,4,2100,0,SIZE];table=bytes(SIZE);name=bytes(24)
 for field,values in [(0,[2,0xffffffff]),(1,[-1,8]),(2,[-1,8]),(3,[0,3,5,11,13,-1]),(6,[0,SIZE-1,SIZE+1,0xffffffff])]:
  for value in values:
   h=good.copy();h[field]=value;rows.append(struct.pack('<7I',*[x&0xffffffff for x in h])+name+table);want.append(model(h,name,table));stats['rejected']+=1
 inputs=b''.join(rows);expected=b''.join(want);(out/'inputs.bin').write_bytes(inputs)
 cxx=shlex.split(a.cc);cross=a.m68k_linux or 'm68k' in cxx[0];cmd=cxx+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if cross else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/'monument_effects.cpp'),str(src/'monument_effects_probe.cpp'),'-o',str(out/'probe')];subprocess.run(cmd,check=True)
 run=(['qemu-m68k','-cpu','m68040'] if cross else [])+[str(out/'probe'),str(out/'inputs.bin')];r=subprocess.run(run,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=180);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==expected,(len(r.stdout),len(expected),next((i for i,(x,y) in enumerate(zip(r.stdout,expected)) if x!=y),None));(out/'outputs.bin').write_bytes(r.stdout)
 receipt=dict(cases=len(rows),**stats,inputSHA256=hashlib.sha256(inputs).hexdigest(),outputSHA256=hashlib.sha256(expected).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
