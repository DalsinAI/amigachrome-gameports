# SPDX-License-Identifier: GPL-3.0-or-later
"""Quad primary/fog and signed upper-threshold original pixel controls."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path

def cases():
 shapes=[([(8,8),(45,8),(45,45),(8,45)],[(1,1),(54,1),(54,54),(1,54)]), ([(32,3),(60,31),(32,59),(4,31)],[(28,1),(54,28),(28,54),(1,28)]), ([(4,3),(52,7),(56,58),(7,52)],[(1,1),(54,1),(54,54),(1,54)])]
 return [[mode,fog,16,*[a for(x,y),(u,v),l in zip(xy,uv,lights)for a in [x+dx,y+dy,u,v,l]]]for xy,uv in shapes for dx,dy in [(0,0),(-15,0),(0,-10),(0,40)]for lights in [(0,0,0,0),(-8,5,-7,10),(16,16,16,16),(0,16,32,37)]for fog in [-1,65]for mode in [0,1]]
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'terrain_water_texture.cpp'),str(src/'terrain_water_texture_probe.cpp'),'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run(runner+[str(exe)],input=b''.join(struct.pack('<23i',*c)for c in cases()),capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==192*4096;digest=hashlib.sha256(r.stdout).hexdigest();assert digest=='742c4e5bf6ecf8bd5127dd8b3fa94486486836db53bf5d0424faa0ef7ac00dd5',digest;bad=subprocess.run(runner+[str(exe)],input=b'partial',capture_output=True,env=env);assert bad.returncode==20;(out/'output.bin').write_bytes(r.stdout);record=dict(cases=192,pixels=len(r.stdout),rejections=10,partialRejected=True,primaryClampControl=[-4,4],bigEndian=a.m68k_linux,sha256=digest);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
