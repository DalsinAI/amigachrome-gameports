#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exact composer HQ scan vs independently decoded wrapped vector metric."""
import argparse,hashlib,itertools,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
HERE=Path(__file__).resolve().parent

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.out.resolve()/'world-energy-hq';flags=['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'];cmd=[a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror','-ffunction-sections','-Wl,--gc-sections',*flags,*[str(HERE/n) for n in ['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','world_energy_hq_probe.cpp']],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];digest=hashlib.sha256();records=[]
 for i,(v,flat,own,marked,pair) in enumerate(itertools.product([7,12],range(2),range(2),range(2),range(8))):
  b=bytearray(valid_fixture(v,0,2));_,o,_=decode(b);bs=304 if v<11 else 308;coords=[(0,0),(6,0),(2,0),(4,0),(0,4),(6,4),(2,4),(4,4)];x,y=coords[pair];hx,hy=coords[(pair+1)%8];struct.pack_into('<I',b,o['metadata']+28,flat)
  for j,(xx,yy) in enumerate([(x,y),(hx,hy)]):
   p=o['bases']+bs*j;struct.pack_into('<hh',b,p,xx,yy);b[p+4]=1 if j==0 or own else 2;b[p+0x8c:p+0x8c+8]=bytes(8)
  if marked:b[o['bases']+bs+0x8c]=2
  dx=abs(hx-x)
  if not flat:dx=min(dx,8-dx)
  dy=abs(hy-y);distance=max(dx,dy)-(((dx+dy)//2-min(dx,dy)+1)//2) if own and marked else 16
  p=a.out/(str(i)+'.SC');p.write_bytes(b);r=subprocess.run([*runner,str(exe),str(p)],capture_output=True,timeout=30,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));wanted=struct.pack('<I',distance);assert r.returncode==0 and r.stdout==wanted,(i,r.returncode,r.stderr,r.stdout.hex(),wanted.hex());assert p.read_bytes()==b;digest.update(r.stdout);records.append(distance)
 receipt=dict(worlds=len(records),outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),distances=sorted(set(records)),command=cmd);(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k!='command'}))
if __name__=='__main__':main()
