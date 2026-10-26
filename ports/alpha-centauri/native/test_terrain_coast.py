# SPDX-License-Identifier: GPL-3.0-or-later
"""Coast quad mask/lookup/light/fog controls: source-derived fixture digest.
No original executable bytes, images or platform FP-control changes required.
"""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
 exe=out/'probe';subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'terrain_coast.cpp'),str(src/'terrain_coast_probe.cpp'),'-o',str(exe)],check=True)
 cases=[]
 shapes=[([(8,8),(45,8),(45,45),(8,45)],[(1,1),(54,1),(54,54),(1,54)]), ([(32,3),(60,31),(32,59),(4,31)],[(28,1),(54,28),(28,54),(1,28)]), ([(4,3),(52,7),(56,58),(7,52)],[(1,1),(54,1),(54,54),(1,54)])]
 for xy,uv in shapes:
  for dx,dy in [(0,0),(-15,0),(0,-10),(40,0),(0,40)]:
   for light in [(0,0,0,0),(-8,5,-7,10),(7,-9,11,-5)]:
    for fog in [-1,65]:cases.append((fog,[(x+dx,y+dy,u,v,l)for (x,y),(u,v),l in zip(xy,uv,light)]))
 packed=struct.pack('<i',len(cases))+b''.join(struct.pack('<i',f)+b''.join(struct.pack('<5i',*v)for v in vs)for f,vs in cases)
 runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[]
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run(runner+[str(exe)],input=packed,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(cases)*4096
 digest=hashlib.sha256(r.stdout).hexdigest();assert digest=='625dbefa6752d69459cd4dc441d04bcc55f8457d37a3aa45e49b20f0f5494a67',digest
 bad=subprocess.run(runner+[str(exe)],input=struct.pack('<i',1)+b'partial',capture_output=True,env=env);assert bad.returncode==20
 (out/'output.bin').write_bytes(r.stdout);record=dict(cases=len(cases),pixels=len(cases)*4096,aliasOrBoundaryRejects=13,partialRejected=True,bigEndian=a.m68k_linux,sha256=digest);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
