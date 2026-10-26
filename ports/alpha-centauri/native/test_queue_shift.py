#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Structural queue iteration, never availability/completion."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent

def model(data):
 size,*items=struct.unpack_from('<11i',data);counts=bytearray(data[44:]);shifted=0
 if size<0 or size>9 or size>0 and items[0]>=512:return struct.pack('<12I',1,*([0x31415926]*11))+bytes([0x5a])*512+struct.pack('<I',1)
 if size:
  if items[0]>=0:counts[items[0]]=(counts[items[0]]-1)&255
  items[:size]=items[1:size+1];size-=1;shifted=1
 return struct.pack('<2i10i',0,size,*items)+counts+struct.pack('<I',shifted)

def contexts():
 rng=random.Random(61583)
 for size in range(10):
  for old in [-2147483648,-133,-71,-1,*range(512),512,2147483647]:
   for count in [0,1,127,255]:
    items=[old]+[rng.randrange(-133,512) for _ in range(9)];counts=bytearray(rng.randbytes(512))
    if 0<=old<512:counts[old]=count
    yield struct.pack('<11i',size,*items)+counts
 for size in [-2147483648,-1,10,2147483647]:yield struct.pack('<11i',size,*([0]*10))+bytes(512)
 for _ in range(1000):yield struct.pack('<11i',rng.randrange(10),*[rng.randrange(-2147483648,2147483648) for _ in range(10)])+rng.randbytes(512)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);rows=list(contexts());data=b''.join(rows);want=b''.join(model(r) for r in rows);(out/'inputs.bin').write_bytes(data);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 exe=out/'queue-shift-probe';cmd=[a.cc,*flags,str(SRC/'queue_shift.cpp'),str(SRC/'queue_shift_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True);r=subprocess.run([*runner,str(exe),str(out/'inputs.bin')],capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want;results=[model(row) for row in rows];receipt=dict(cases=len(rows),accepted=sum(struct.unpack_from('<I',x)[0]==0 for x in results),rejected=sum(struct.unpack_from('<I',x)[0]!=0 for x in results),shifted=sum(struct.unpack_from('<I',x)[0]==0 and struct.unpack_from('<I',x,560)[0]==1 for x in results),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd);(out/'outputs.bin').write_bytes(r.stdout);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
