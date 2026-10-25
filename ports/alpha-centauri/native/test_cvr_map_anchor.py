# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned numeric placement controls; no game assets or original bytes required."""
import argparse,os,random,struct,subprocess,json,hashlib
from pathlib import Path

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');args=ap.parse_args();out=Path(args.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if args.m68k_linux else ['-g','-fsanitize=address,undefined'];subprocess.run([args.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'cvr_map_anchor.cpp'),str(src/'cvr_map_anchor_probe.cpp'),'-o',str(exe)],check=True)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');prefix=['qemu-m68k','-cpu','m68040']if args.m68k_linux else [];rng=random.Random(1025);cases=[];expected=[]
 for z in [-8,-5,0,5,8]:
  unit=max(1,(50*(z+16)//16+1)//4);hx,hy=unit*4,unit*2
  for flat in [0,1]:
   for _ in range(100):
    left=rng.randrange(-4,49);top=rng.randrange(-2,40);x=rng.randrange(48);y=rng.randrange(48);x=(x&~1)|(y&1);alt=rng.choice([-120,-16,-4,0,4,17,120]);px=rng.randrange(-100,600);py=rng.randrange(-100,400);ox=rng.randrange(1,16);ex=rng.randrange(1,16);oy=rng.randrange(1,12);ey=rng.randrange(1,12);heights=[alt]+[rng.choice([-120,-16,-4,0,4,17,120])for _ in range(3)];cases.append([flat,z,left,top,px,py,hx,hy,ox,ex,oy,ey,x,y]+heights)
    candidates=[x]if flat else [x+48 if x<left else x];representation=candidates[0]
    if not flat and representation>=left+ox+ex:representation-=48
    dx=x-left
    if top<=y<top+oy+ey and left<=representation<left+ox+ex:
     if not flat:dx=dx+48 if dx<0 else dx-48 if dx>=48 else dx
    else:
     if dx>24:dx-=48
     if dx< -24:dx+=48
    expected.append((px+(dx-1)*hx,py+(y-top-1)*hy-sum(heights)//4))
 inputs=out/'placement.bin';inputs.write_bytes(b''.join(struct.pack('<18i',*c)for c in cases));r=subprocess.run(prefix+[str(exe),str(inputs)],capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);want=b''.join(struct.pack('<2i',*p)for p in expected);assert r.stdout==want;(out/'placement-output.bin').write_bytes(r.stdout)
 metrics=[(z,0,16)for z in range(-8,9)]+[(0,n,d)for n,d in [(1,100),(8,16),(16,16),(21,16),(32,16)]];inp=out/'metrics.bin';inp.write_bytes(b''.join(struct.pack('<3i',*c)for c in metrics));r2=subprocess.run(prefix+[str(exe),'--metrics',str(inp)],capture_output=True,env=env);assert r2.returncode==0,(r2.returncode,r2.stderr);want=[]
 for z,n,d in metrics:
  size=n*50//d if n else (z+16)*50//16;u=max(1,(size+1)//4);want.extend([u*8,u*4,u*4,u*2,u*2,u])
 assert r2.stdout==struct.pack('<'+'i'*len(want),*want)
 receipt=dict(placementCases=len(cases),dimensionCases=len(metrics),builtInMetricAnchorInvalidAliasControls=True,originalCodeExecuted=False,placementSHA256=hashlib.sha256(r.stdout).hexdigest(),dimensionSHA256=hashlib.sha256(r2.stdout).hexdigest(),messages=[r.stderr.decode().strip(),r2.stderr.decode().strip()]);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
