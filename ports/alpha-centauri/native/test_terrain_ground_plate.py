# SPDX-License-Identifier: GPL-3.0-or-later
"""Coherent small indexed ground plates; synthetic numeric fixture, not game art."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'];exe=out/'probe';names=['terrain_mesh','terrain_dispatch','terrain_texture','terrain_ground_plate','terrain_ground_plate_probe'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in names],'-o',str(exe)],check=True)
 heights=[[0,0,0,0,0],[8,4,12,8,8],[12,16,4,20,8],[4,0,8,0,8]];data=bytearray()
 for i in range(48):
  meta=[(0x23cb678+i*44)&0xffffffff,i%3,(i//3)%3,(i*37)&255,(i//9)&1,(i//2)&1,(i//7)&1,i&1];mesh=[-12+i%3,20-i%5,*heights[i%4],1+i%2];light=[((i+k*3)%9)-4 for k in range(5)];data+=struct.pack('<8I8i5i',*meta,*mesh,*light)
 runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run(runner+[str(exe)],input=data,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==48*16384;digest=hashlib.sha256(r.stdout).hexdigest();assert digest=='4a966c7042e90f1ccdedf05b5ffdd89fcdd1cf7c6b6d271c9fb0c562e2e8e374',digest
 bad=subprocess.run(runner+[str(exe)],input=b'partial',capture_output=True,env=env);assert bad.returncode==20
 (out/'output.bin').write_bytes(r.stdout);record=dict(cases=48,pixels=786432,transactionalRejections=12,partialRejected=True,bigEndian=a.m68k_linux,syntheticFixture=True,sha256=digest);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
