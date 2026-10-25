# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent fixed-depth raster controls; no game bytes/assets required."""
import argparse,json,os,random,struct,subprocess,hashlib
from pathlib import Path

def fixtures():
 r=random.Random(0xAC090)
 shades=bytes((i*7+i//89)%24 for i in range(2048))
 palette=bytes((c*11+s*13)&255 for s in range(24) for c in range(256))
 cases=[]
 # Deliberately include fractional negative coordinates and both clip profiles.
 for i in range(256):
  l,t=r.randrange(4),r.randrange(4);right,bottom=r.randrange(8,16),r.randrange(8,16)
  points=[]
  for j in range(1+i%64):
   x=r.randrange((l-3)*256,(right+3)*256);y=r.randrange((t-3)*256,(bottom+3)*256)
   points.append((x,y,r.getrandbits(24)<<8,r.randrange(2048),r.randrange(256)))
  cases.append((f'random-{i}',(l,t,right,bottom),bool(i%2),bytes(r.randrange(256) for _ in range(320)),[r.randrange(65536) for _ in range(320)],points))
 # Complete corner/edge/fraction controls with near/far/tied depths at each pixel.
 for quad in (False,True):
  for x in (-513,-257,-256,-255,-1,0,255,256,1791,1792,2047,2048,2303):
   for y in (-257,-256,-1,0,255,1791,1792,2048):
    points=[(x,y,z<<16,normal,color) for z,normal,color in [(100,0,7),(200,2047,8),(50,89,9),(50,175,10),(65535,2046,11),(0,1,12)]]
    cases.append((f'edge-{int(quad)}-{x}-{y}',(0,0,7,7),quad,bytes(320),[65535]*320,points))
 return shades,palette,cases

def input_bytes(shades,palette,cases):
 out=bytearray(b'CVRP'+struct.pack('<I',len(cases))+shades+palette)
 for _,clip,quad,pixels,depths,points in cases:
  out+=struct.pack('<6I',*clip,int(quad),len(points))+pixels+struct.pack('<320H',*depths)
  for p in points:out+=struct.pack('<iiIHB',*p)
 return out

def reference(shades,palette,cases):
 out=bytearray()
 for _,(left,top,right,bottom),quad,initial,initial_depth,points in cases:
  pixels=bytearray(initial);depths=list(initial_depth)
  for xf,yf,z,normal,color in points:
   x,y=xf//256,yf//256
   if quad:
    if not(left-1<=x<=right and top-1<=y<=bottom):continue
    x=max(left,min(right-1,x));y=max(top,min(bottom-1,y))
   elif not(left<=x<right and top<=y<bottom):continue
   for dy in range(2 if quad else 1):
    for dx in range(2 if quad else 1):
     at=(y+dy)*20+x+dx
     if z>>16<depths[at]:
      depths[at]=z>>16;pixels[at]=palette[shades[normal]*256+color]
  out+=pixels+struct.pack('<320H',*depths)
 return bytes(out)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 shades,palette,cases=fixtures();data=input_bytes(shades,palette,cases);(out/'input.bin').write_bytes(data);wanted=reference(shades,palette,cases)
 flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined']
 subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'cvr_raster.cpp'),str(src/'cvr_raster_probe.cpp'),'-o',str(out/'probe')],check=True)
 cmd=(['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(out/'input.bin')]
 p=subprocess.run(cmd,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert p.returncode==0,(p.returncode,p.stderr)
 assert p.stdout==wanted, next((cases[i][0] for i in range(len(cases)) if p.stdout[i*960:(i+1)*960]!=wanted[i*960:(i+1)*960]),'length')
 (out/'output.bin').write_bytes(p.stdout)
 subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'cvr_raster.cpp'),str(src/'cvr_raster_control.cpp'),'-o',str(out/'control')],check=True)
 c=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'control')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
 assert c.returncode==0 and c.stdout==b'CVR RASTER CONTROL cases=26 RC 0\n',(c.returncode,c.stdout,c.stderr)
 receipt=dict(batches=len(cases),points=sum(len(c[-1]) for c in cases),invalidAndBoundaryControls=26,bytes=len(wanted),sha256=hashlib.sha256(wanted).hexdigest())
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
