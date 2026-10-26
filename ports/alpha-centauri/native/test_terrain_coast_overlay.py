# SPDX-License-Identifier: GPL-3.0-or-later
"""Coherent coast overlays: original source-kernel controlled pixel digest."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path

def cases():
 heights=[[0]*5,[8,4,12,8,8],[12,16,4,20,8],[4,0,8,0,8]]
 return [[-12+i%3,20-i%5,*heights[i%4],1+i%2,i%12,i*3%14,-1 if i%2 else 65,*[(i+k*3)%8 for k in range(4)],*[(i*3+k*5+j*7)%51-9 for k in range(4)for j in range(4)]]for i in range(48)]
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];names=['terrain_mesh','terrain_coast_resources','terrain_coast','terrain_coast_overlay','terrain_coast_overlay_probe'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in names],'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');packed=b''.join(struct.pack('<31i',*c)for c in cases());r=subprocess.run(runner+[str(exe)],input=packed,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==48*16384;digest=hashlib.sha256(r.stdout).hexdigest();assert digest=='8a69ce892b08251b1a59d0258646c7c55f1c24269bbaa9385959c29f84f0f4b3',digest;bad=subprocess.run(runner+[str(exe)],input=b'partial',capture_output=True,env=env);assert bad.returncode==20;(out/'output.bin').write_bytes(r.stdout);record=dict(patches=48,pixels=len(r.stdout),rejections=16,partialRejected=True,bigEndian=a.m68k_linux,sha256=digest);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
