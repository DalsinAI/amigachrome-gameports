#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,shlex,subprocess
p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default=os.environ.get('CXX','c++'));p.add_argument('--m68k-linux',action='store_true');a=p.parse_args()
out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;cxx=shlex.split(a.cc);cross=a.m68k_linux or 'm68k' in cxx[0]
cmd=cxx+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if cross else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/'facility_notice.cpp'),str(src/'notice_stages_probe.cpp'),'-o',str(out/'probe')]
subprocess.run(cmd,check=True);run=(['qemu-m68k','-cpu','m68040'] if cross else [])+[str(out/'probe')];r=subprocess.run(run,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=180)
assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==b'62714 stage controls passed\n',r.stdout
(out/'outputs.txt').write_bytes(r.stdout);receipt=dict(cases=62714,binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
