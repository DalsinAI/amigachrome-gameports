# SPDX-License-Identifier: GPL-3.0-or-later
"""Indexed terrain triangle pixel controls: source-derived fixed fixture digest.
No original executable bytes, images or platform FP-control changes required.
"""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path

MODES=[(-1, 65, '24eab9e1c28e820ea8edd20aab6ba9fe4e05a399f7d12e15ef306fd4bfb3ea21'), (9, -1, '8531d7d449849cbb771fa76783a1e429154543eb0ca58197f5fdffd7aad8af7a'), (9, 65, 'c222b3a32353756a2ce04c789f24d1a591625dda4032d05a1050f8d2961425d4')]

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
 exe=out/'probe';subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'terrain_texture.cpp'),str(src/'terrain_composition_probe.cpp'),'-o',str(exe)],check=True)
 cases=[]
 for x,y in [(0,0),(-10,0),(0,-10),(45,0),(0,45)]:
  for lights in [(0,0,0),(-4,4,1),(99,-99,3)]:cases.append(([(32+x,32+y),(x,32+y),(32+x,y)],[(27,28),(0,55),(0,1)],lights))
 cases += [([(40,55),(5,40),(25,3)],[(20,30),(1,50),(1,1)],(-3,2,4)), ([(4,50),(45,50),(20,2)],[(30,30),(2,50),(2,2)],(0,1,-2))]
 rng=random.Random(0x61b1f0)
 for n in range(96):cases.append(([(rng.randrange(-16,80),rng.randrange(-16,80))for _ in range(3)],[(rng.randrange(1,55),rng.randrange(1,55))for _ in range(3)],tuple(rng.randrange(-7,8)for _ in range(3))))
 packed=struct.pack('<i',len(cases))+b''.join(struct.pack('<5i',*xy,*uv,l)for v,u,ls in cases for xy,uv,l in zip(v,u,ls))
 runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[]
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');records=[]
 for key,fog,digest in MODES:
  data=struct.pack('<2i',key,fog)+packed
  r=subprocess.run(runner+[str(exe)],input=data,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(cases)*4097
  got=hashlib.sha256(r.stdout).hexdigest();assert got==digest,(key,fog,got)
  for n in range(len(cases)):assert r.stdout[n*4097]==1
  (out/('output-%d-%d.bin'%(key,fog))).write_bytes(r.stdout);records.append(dict(key=key,fog=fog,sha256=got))
 bad=subprocess.run(runner+[str(exe)],input=struct.pack('<3i',-1,65,1)+b'partial',capture_output=True,env=env);assert bad.returncode==20
 record=dict(cases=len(cases)*len(MODES),pixels=len(cases)*4096*len(MODES),aliasOrBoundaryRejects=12,partialRejected=True,bigEndian=a.m68k_linux,modes=records);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
