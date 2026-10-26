# SPDX-License-Identifier: GPL-3.0-or-later
"""Indexed terrain triangle pixel controls: source-derived fixed fixture digest.
No original executable bytes, images or platform FP-control changes required.
"""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
 exe=out/'probe';subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'terrain_texture.cpp'),str(src/'terrain_texture_probe.cpp'),'-o',str(exe)],check=True)
 cases=[]
 for x,y in [(0,0),(-10,0),(0,-10),(45,0),(0,45)]:
  for lights in [(0,0,0),(-4,4,1),(99,-99,3)]:cases.append(([(32+x,32+y),(x,32+y),(32+x,y)],[(27,28),(0,55),(0,1)],lights))
 cases += [([(40,55),(5,40),(25,3)],[(20,30),(1,50),(1,1)],(-3,2,4)), ([(4,50),(45,50),(20,2)],[(30,30),(2,50),(2,2)],(0,1,-2))]
 rng=random.Random(0x61b1f0)
 for n in range(96):cases.append(([(rng.randrange(-16,80),rng.randrange(-16,80))for _ in range(3)],[(rng.randrange(1,55),rng.randrange(1,55))for _ in range(3)],tuple(rng.randrange(-7,8)for _ in range(3))))
 packed=struct.pack('<i',len(cases))+b''.join(struct.pack('<5i',*xy,*uv,l)for v,u,ls in cases for xy,uv,l in zip(v,u,ls))
 runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[]
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run(runner+[str(exe)],input=packed,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(cases)*4097
 digest=hashlib.sha256(r.stdout).hexdigest();assert digest=='deff5030da7efbfc6e385da8b0d9400a24e125f872ec81e78667f053a3de4fd6',digest
 for n in range(len(cases)):assert r.stdout[n*4097]==1
 bad=subprocess.run(runner+[str(exe)],input=struct.pack('<i',1)+b'partial',capture_output=True,env=env);assert bad.returncode==20
 (out/'output.bin').write_bytes(r.stdout);record=dict(cases=len(cases),pixels=len(cases)*4096,aliasOrBoundaryRejects=8,partialRejected=True,bigEndian=a.m68k_linux,sha256=digest);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
