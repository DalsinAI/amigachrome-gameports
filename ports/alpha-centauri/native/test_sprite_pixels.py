# SPDX-License-Identifier: GPL-3.0-or-later
"""Explicit clipped sprite pixel-copy controls, no original art or OS calls."""
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess
from test_sprite_scale import reference

def fixtures(owner=None):
 for n,d in[(11,16),(21,16),(1,4),(2,1),(3,1),(4,1)]:
  for w,h in([(72,40)]if owner is not None else[(7,5),(13,11),(31,17),(72,40)]):
   sp=w+(0 if owner is not None else 3);dp=136 if (n+w)%2 else 128
   sprite=owner if owner is not None else bytes(9 if(x+3*y)%7==0 else(x*19+y*41)%256 for y in range(h)for x in range(sp))
   for left,top in[(-7,-5),(9,17),(108,113),(2,3)]:
    right=left+w*n//d;bottom=top+h*n//d
    for inset in[0,1,3]:
     cl=max(0,left)+inset;ct=max(0,top)+inset;cr=min(128,right)-inset;cb=min(128,bottom)-inset
     if cl>=cr or ct>=cb:continue
     q=(n,d,w,h,sp,dp,9,left,top,right,bottom,cl,ct,cr,cb)
     pixels=bytes((x*13+y*7+199)%256 for y in range(128)for x in range(dp))
     yield q,sprite,pixels

def axis(n,d,expand,begin,end,cl,cr):
 out=reference((n,d,0 if expand else 1,begin,end,cl,cr));v=struct.unpack('<1605i',out);header=v[800:805];steps=v[805:805+header[4]];return header,steps

def expected(q,sprite,initial):
 n,d,w,h,sp,dp,key,left,top,right,bottom,cl,ct,cr,cb=q;expand=n>d;(xh,xs),(yh,ys)=axis(n,d,expand,left,right,cl,cr),axis(n,d,expand,top,bottom,ct,cb);out=bytearray(initial)
 # Build source-coordinate lists independently from the C++ nested blitter.
 if expand:
  xindices=[xh[0]+i for i,step in enumerate(xs)for _ in range(step)];yindices=[yh[0]+i for i,step in enumerate(ys)for _ in range(step)]
 else:
  xindices=[xh[0]+sum(xs[:i])for i in range(len(xs))];yindices=[yh[0]+sum(ys[:i])for i in range(len(ys))]
 for dy,sy in enumerate(yindices):
  for dx,sx in enumerate(xindices):
   c=sprite[sy*sp+sx]
   if c!=key:out[(ct+dy)*dp+cl+dx]=c
 return bytes(4)+out

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');bins={}
 for name in['probe','control']:
  f=out/name;subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(s/'sprite_scale.cpp'),str(s/'sprite_pixels.cpp'),str(s/('sprite_pixels_'+name+'.cpp')),'-o',str(f)],check=True);bins[name]=hashlib.sha256(f.read_bytes()).hexdigest()
 cases=list(fixtures());packed=b''.join(struct.pack('<15i',*q)+src+dst for q,src,dst in cases);want=b''.join(expected(*case)for case in cases);r=subprocess.run(runner+[str(out/'probe')],input=packed,capture_output=True,env=env);(out/'stderr.txt').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want,(len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));h=hashlib.sha256(r.stdout).hexdigest();assert h=='9d45fa68c0b3671eef67b5009e980748fe4f43b9a7c487740e0e3f0d43146630',h;c=subprocess.run(runner+[str(out/'control')],capture_output=True,env=env);(out/'control.log').write_bytes(c.stdout+c.stderr);assert c.returncode==0,(c.returncode,c.stdout,c.stderr);(out/'input.bin').write_bytes(packed);(out/'output.bin').write_bytes(r.stdout);rec=dict(cases=len(cases),bytes=len(r.stdout),sha256=h,controls=c.stdout.decode().strip(),bigEndian=a.m68k_linux,native=False,binaries=bins);(out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n');print(rec)
if __name__=='__main__':main()
