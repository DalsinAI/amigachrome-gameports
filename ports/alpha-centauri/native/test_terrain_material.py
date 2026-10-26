# SPDX-License-Identifier: GPL-3.0-or-later
"""Original terrain index offset and palette placement; no art or x86 required."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'];exe=out/'probe'
 subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'terrain_material.cpp'),str(src/'terrain_material_probe.cpp'),'-o',str(exe)],check=True)
 runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];r=subprocess.run(runner+[str(exe)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stderr)
 want=bytes((x+10)&255 for x in range(256))+bytes([177]*30)+bytes((i*37+13)&255 for i in range(708))+bytes([177]*30);assert r.stdout==want
 (out/'output.bin').write_bytes(r.stdout);record=dict(indexCases=256,paletteBytes=768,rejections=9,bigEndian=a.m68k_linux,sha256=hashlib.sha256(want).hexdigest());(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
