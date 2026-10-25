#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,json,os,struct,subprocess,hashlib
from pathlib import Path
SRC=Path(__file__).resolve().parent

def fnv(data):
 h=2166136261
 for v in data:h=((h^v)*16777619)&0xffffffff
 return f'{h:08x}'

def fixture(w,h):
 stride=(w+1)&~1;header=bytearray(128);header[0:4]=bytes([10,5,1,8]);struct.pack_into('<4H',header,4,0,0,w-1,h-1);header[65]=1;struct.pack_into('<H',header,66,stride)
 pixels=bytes((x*7+y*41)&255 for y in range(h) for x in range(w));body=bytearray()
 for y in range(h):
  for x in range(stride):
   v=pixels[y*w+x] if x<w else 0
   body.extend(bytes([0xc1,v]) if v>=192 else bytes([v]))
 palette=bytes((i*29)&255 for i in range(768));return bytes(header+body+b'\x0c'+palette),dict(width=str(w),height=str(h),pixels=fnv(pixels),palette=fnv(palette),rc='0')

def cases():
 out=[]
 for w,h in [(1,1),(2,3),(55,57),(56,56),(101,7),(1024,768)]:
  data,expected=fixture(w,h);out.append((f'valid-{w}-{h}',data,expected))
 data,_=fixture(7,5)
 for offset,value in [(0,0),(1,4),(2,0),(3,4),(65,3),(66,0),(67,255),(len(data)-769,0)]:
  b=bytearray(data);b[offset]=value;out.append((f'bad-field-{offset}',bytes(b),None))
 for n in [0,1,127,128,768,769,len(data)-1]:out.append((f'truncated-{n}',data[:n],None))
 for name,body in [('zero-run',bytes([0xc0,0])),('overrun',bytes([0xff,0]))]:out.append((name,data[:128]+body+data[-769:],None))
 return out

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='c++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 exe=a.out/'pcx-probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];run=[]
 if a.m68k_linux:flags+=['-m68040','-static'];run=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 cmd=[a.cc,*flags,str(SRC/'pcx_image.cpp'),str(SRC/'pcx_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True);records=[]
 for i,(name,data,expected) in enumerate(cases()):
  f=a.out/f'{i}.pcx';f.write_bytes(data);r=subprocess.run([*run,str(exe),str(f)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  assert r.returncode==(0 if expected else 20),(name,r.returncode,r.stdout,r.stderr)
  if expected:assert dict(x.split('=') for x in r.stdout.split())==expected,(name,r.stdout,expected)
  records.append(dict(name=name,passed=True))
 (a.out/'receipt.json').write_text(json.dumps(dict(command=cmd,cases=records,binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(f'{len(records)} PCX import cases passed')
if __name__=='__main__':main()
