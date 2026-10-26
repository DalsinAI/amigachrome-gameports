# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned sprite geometry/intersection/composition, synthetic indexed pixels."""
from pathlib import Path
import argparse,json,hashlib,os,struct,subprocess
from test_sprite_pixels import expected as pixel_expected

def vectors():
 for n,d in[(11,16),(21,16),(1,1),(1,3),(3,2),(4,1)]:
  for w,h,ox,oy,fw,fh in[(7,5,2,1,11,9),(13,11,1,3,17,18),(72,40,14,4,100,62)]:
   for x,y in[(-17,-13),(-10,-2),(0,0),(17,29),(111,117),(200,200)]:
    for minimum in[(0,0),(8,9)]:
     for clip in[(0,0,128,128),(3,5,117,121)]:yield(n,d,w,h,ox,oy,fw,fh,x,y,*clip,*minimum)
def fixture(q,owner=None):
 w,h=q[2:4];src=owner if owner is not None else bytes(9 if(x+y*3)%7==0 else(x*19+y*37)%256 for y in range(h)for x in range(w));dst=bytes((x*11+y*17+199)%256 for y in range(128)for x in range(128));return src,dst

def reference(q,src,dst):
 n,d,w,h,ox,oy,fw,fh,x,y,cl,ct,cr,cb,mw,mh=q;ratio=struct.unpack('<f',struct.pack('<f',n/d))[0];left=int(x+ox*ratio);top=int(y+oy*ratio);right=left+int(w*ratio);bottom=top+int(h*ratio);full=(left,top,right,bottom);inter=(max(left,cl),max(top,ct),min(right,cr),min(bottom,cb));status=0
 if inter[0]>=inter[2]or inter[1]>=inter[3]:inter=(0,0,0,0);status=1
 elif n<d and(right-left<mw or bottom-top<mh):status=2
 output=dst if status else pixel_expected((n,d,w,h,w,128,9,*full,*inter),src,dst)[4:]
 return struct.pack('<I8i',status,*full,*inter)+output

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');bins={}
 for name in['probe','control']:
  f=out/name;subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(s/'sprite_scale.cpp'),str(s/'sprite_pixels.cpp'),str(s/'sprite_draw.cpp'),str(s/('sprite_draw_'+name+'.cpp')),'-o',str(f)],check=True);bins[name]=hashlib.sha256(f.read_bytes()).hexdigest()
 cases=[(q,*fixture(q))for q in vectors()];packed=b''.join(struct.pack('<16i',*q)+src+dst for q,src,dst in cases);expected=b''.join(reference(*case)for case in cases);r=subprocess.run(runner+[str(out/'probe')],input=packed,capture_output=True,env=env);(out/'stderr.txt').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==expected,(len(r.stdout),len(expected),next((i for i,(x,y)in enumerate(zip(r.stdout,expected))if x!=y),None));h=hashlib.sha256(r.stdout).hexdigest();assert h=='bf4a4d96c35dfd053d1f1726b0f605594046d570dbc69b2a74564231a4f3c007',h;c=subprocess.run(runner+[str(out/'control')],capture_output=True,env=env);(out/'control.log').write_bytes(c.stdout+c.stderr);assert c.returncode==0,(c.returncode,c.stdout,c.stderr);(out/'input.bin').write_bytes(packed);(out/'output.bin').write_bytes(r.stdout);rec=dict(cases=len(cases),bytes=len(r.stdout),sha256=h,controls=c.stdout.decode().strip(),bigEndian=a.m68k_linux,native=False,binaries=bins);(out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n');print(rec)
if __name__=='__main__':main()
