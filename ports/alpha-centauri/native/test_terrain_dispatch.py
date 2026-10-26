# SPDX-License-Identifier: GPL-3.0-or-later
"""Explicit source rendering context/atlas/UV controls, without owner assets."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'];exe=out/'probe';subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'terrain_dispatch.cpp'),str(src/'terrain_dispatch_probe.cpp'),'-o',str(exe)],check=True)
 cases=[]
 for mask in range(256):
  for rock in range(3):
   for bits in range(8):cases.append(((0x23cb678+mask*44+rock*0x10000+bits*12345)&0xffffffff,rock,bits%3,mask,(bits>>2)&1,(bits>>1)&1,bits&1,(bits+mask)&1))
 data=b''.join(struct.pack('<8I',*c)for c in cases);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run(runner+[str(exe)],input=data,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(cases)*132;digest=hashlib.sha256(r.stdout).hexdigest();assert digest=='ca6b0406f0ebb1b17de479a6d87849c313ada707d8082bccc2d6c7d9817f4bf8',digest
 bad=subprocess.run(runner+[str(exe)],input=b'partial',capture_output=True,env=env);assert bad.returncode==20
 (out/'output.bin').write_bytes(r.stdout);record=dict(cases=len(cases),bytes=len(r.stdout),rejections=6,partialRejected=True,explicitNumericContext=True,originalRuntimeAllocationKnown=False,bigEndian=a.m68k_linux,sha256=digest);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
