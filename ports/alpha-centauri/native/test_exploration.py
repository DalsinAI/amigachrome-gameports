#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
src=Path(__file__).resolve().parent;out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
flags=['-std=c++17','-O1','-Wall','-Wextra','-Werror'];runner=[]
if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
exe=out/'exploration-control';command=[a.cc,*flags,*[str(src/(n+'.cpp')) for n in ['prototype','movement','move_budget','exploration','exploration_control']],'-o',str(exe)]
subprocess.run(command,check=True);r=subprocess.run([*runner,str(exe)],capture_output=True,text=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
assert r.returncode==0,(r.returncode,r.stdout,r.stderr)
expected='scenario=128 road=3 wrap=1 ocean=1 rough=1 exhausted=2 invalid=3 fractional_failed=112 fractional_moved=16 rc=0'
assert r.stdout.strip()==expected,r.stdout
(out/'receipt.json').write_text(json.dumps(dict(command=command,runner=runner,results=r.stdout.strip(),binarySHA256=hashlib.file_digest(exe.open('rb'),'sha256').hexdigest()),indent=2)+'\n');print(r.stdout,end='')
