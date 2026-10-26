# SPDX-License-Identifier: GPL-3.0-or-later
"""Sprite numeric preparation controls; no owner assets or OS imports."""
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess

def vectors():
 for n,d in[(11,16),(21,16),(1,1),(1,4),(4,1),(16,17),(17,16),(7,3),(3,7),(2,9),(9,2),(13,8)]:
  for mode in range(4):
   for begin in[-93,0,71]:
    for offset,length in[(0,1),(0,13),(1,1),(1,17),(2,3),(7,23),(31,49),(79,5)]:yield(n,d,mode,begin,begin+100,begin+offset,begin+offset+length)
def reference(q):
 n,d,mode,begin,end,left,right=q;large,small=max(n,d),min(n,d);base,rem=divmod(large,small);err=rem-2*small+(large if base%2 else 0);table=[]
 for _ in range(800):
  err+=2*rem;step=base
  if err>0:err-=2*small;step+=1
  table.append(step)
 if small==1:table=[large]*800
 offset=left-begin;span=right-left
 if mode%2:steps=table[offset:offset+span];header=[sum(table[:offset]),offset,sum(steps),span,len(steps)]
 else:
  intervals=[];at=0
  for i,step in enumerate(table):
   overlap=max(0,min(at+step,offset+span)-max(at,offset))
   if overlap:intervals.append((i,overlap))
   at+=step
  steps=[v for _,v in intervals];header=[intervals[0][0],offset,len(steps),span,len(steps)]
 return struct.pack('<1605i',*table,*header,*steps,*([0]*(800-len(steps))))
def main():
 a=argparse.ArgumentParser();a.add_argument('--out',required=True);a.add_argument('--cc',default='g++');a.add_argument('--m68k-linux',action='store_true');a=a.parse_args();p=Path(a.out).resolve();p.mkdir(exist_ok=False,parents=True);s=Path(__file__).resolve().parent;flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');binaries={}
 for name in['probe','control']:
  binary=p/name;command=[a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(s/'sprite_scale.cpp'),str(s/('sprite_scale_'+name+'.cpp')),'-o',str(binary)];subprocess.run(command,check=True);binaries[name]=hashlib.sha256(binary.read_bytes()).hexdigest()
 cases=list(vectors());packed=b''.join(struct.pack('<7i',*q)for q in cases);expected=b''.join(reference(q)for q in cases);r=subprocess.run(runner+[str(p/'probe')],input=packed,capture_output=True,env=env);(p/'stderr.txt').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==expected,(len(r.stdout),len(expected),next((i for i,(x,y)in enumerate(zip(r.stdout,expected))if x!=y),None));h=hashlib.sha256(r.stdout).hexdigest();assert h=='1bbb9434fecc766e877ad5326506edfc7af556d960ef68c888d2c7dcc5eacc16',h
 for length in range(1,28):
  short=subprocess.run(runner+[str(p/'probe')],input=packed[:length],capture_output=True,env=env);assert short.returncode==20 and short.stdout==b'',(length,short.returncode,short.stderr)
 c=subprocess.run(runner+[str(p/'control')],capture_output=True,env=env);(p/'control.log').write_bytes(c.stdout+c.stderr);assert c.returncode==0,(c.returncode,c.stdout,c.stderr);(p/'input.bin').write_bytes(packed);(p/'output.bin').write_bytes(r.stdout);record=dict(cases=len(cases),bytes=len(r.stdout),sha256=h,controls=c.stdout.decode().strip(),partialInputRejects=27,bigEndian=a.m68k_linux,native=False,binaries=binaries);(p/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
