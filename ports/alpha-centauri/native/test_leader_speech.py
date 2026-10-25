# SPDX-License-Identifier: GPL-3.0-or-later
"""No original assets required: catalog, explicit LE PCM and rejection controls."""
import argparse,json,os,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args()
out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined']
subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'leader_speech.cpp'),str(src/'leader_speech_control.cpp'),'-o',str(out/'control')],check=True)
prefix=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
r=subprocess.run(prefix+[str(out/'control')],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True)
assert r.returncode==0 and r.stdout==b'LEADER SPEECH 105 checks PASS\n',(r.returncode,r.stdout,r.stderr)
(out/'receipt.json').write_text(json.dumps({'checks':105,'architecture':'big-endian-68040' if a.m68k_linux else 'host-asan-ubsan','result':'PASS'})+'\n');print(r.stdout.decode(),end='')
