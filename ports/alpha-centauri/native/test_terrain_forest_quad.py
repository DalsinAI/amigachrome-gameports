# SPDX-License-Identifier: GPL-3.0-or-later
"""Original-source shaded keyed forest quad; portable no-owner fixtures."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path

def cases():
 shapes=[[(5,32),(32,5),(58,32),(32,58)],[(5,5),(51,5),(51,49),(5,49)],[(4,24),(20,4),(51,19),(45,57)]];uv=[(0,0),(55,0),(55,55),(0,55)]
 return [[key,*[z for k,(x,y)in enumerate(shape)for z in [x+dx,y+dy,*uv[(k+rotation)%4],lights[k]]]]for shape in shapes for rotation in range(4)for dx,dy in [(0,0),(-24,-8),(20,18),(-60,-60)]for key in [9,23,255]for lights in [(0,0,0,0),(-4,2,4,-1),(2,2,2,2),(256,-256,1,-1)]]
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(s/'terrain_forest_quad.cpp'),str(s/'terrain_forest_quad_probe.cpp'),'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];qs=cases();packed=b''.join(struct.pack('<21i',*q)for q in qs);env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run(runner+[str(exe)],input=packed,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(qs)*4096;h=hashlib.sha256(r.stdout).hexdigest();assert h=='c631f66518fd664e73c429077c531fac1562ed6fa4394618f0e242393cb5850f',h;assert subprocess.run(runner+[str(exe)],input=b'partial',capture_output=True,env=env).returncode==20;record=dict(cases=len(qs),pixels=len(r.stdout),rejections=9,partialRejected=True,bigEndian=a.m68k_linux,native=False,sha256=h);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
