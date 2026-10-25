#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Source-flow stack relocation controls, including rejected no-write cases."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'vehicle-stack-control';command=[a.cc,*flags,str(SRC/'vehicle_stack.cpp'),str(SRC/'vehicle_stack_control.cpp'),'-o',str(exe)]
 subprocess.run(command,check=True)
 r=subprocess.run([*runner,str(exe)],capture_output=True,text=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
 expected='source_cases=18432 rejections=20 digest=61fa6357 rc=0\n'
 assert r.returncode==0 and r.stdout==expected,(r.returncode,r.stdout,r.stderr)
 (out/'receipt.json').write_text(json.dumps(dict(command=command,runner=runner,output=r.stdout,binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n')
 print(r.stdout,end='')
if __name__=='__main__':main()
