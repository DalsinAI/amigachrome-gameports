#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""WP8 diplomacy, Planetary Council, victory and score controls.

Builds engine/tests/wp8_test.cpp with every engine/*.cpp and engine/stubs/*.cpp
source and ui_diplo.cpp, on the host with ASan/UBSan or for m68k-linux under qemu
(big-endian), and runs the synthetic-world unit checks: treaty state changes,
mood and friction, atrocities, comm-link proposals and the dialog model,
pact_ends, Governor and Supreme Leader votes, conquest, diplomatic, economic and
transcendence victories, the score, End Turn with the WP8 turn hooks and
same-seed determinism. With --m68k-linux it also builds a plain host binary and
requires identical output (digests of the diplomacy state, scores and runs).
Writes receipt.json to the output directory. No original assets are used.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'

def build(cc,flags,exe):
 sources=sorted(ENGINE.glob('*.cpp'))+sorted((ENGINE/'stubs').glob('*.cpp'))+[SRC/'ui_diplo.cpp',ENGINE/'tests'/'wp8_test.cpp']
 cmd=[cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',*flags,'-I',str(ENGINE),*map(str,sources),'-o',str(exe)]
 subprocess.run(cmd,check=True);return cmd

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 exe=out/'wp8-test'
 if a.m68k_linux:
  cmd=build(a.cc,['-m68040','-static'],exe);runner=['qemu-m68k','-cpu','m68040']
  ref=out/'wp8-test-host';build('c++',[],ref)
 else:
  cmd=build(a.cc,['-fsanitize=address,undefined','-fno-sanitize-recover=all'],exe);runner=[];ref=None
 def run(binary,pre):
  return subprocess.run([*pre,str(binary),'unit'],capture_output=True,text=True,timeout=1800,env=env)
 r=run(exe,runner);assert r.returncode==0,(r.stdout,r.stderr[-8000:]);unit=r.stdout.strip();print(unit)
 r2=run(exe,runner);assert r2.returncode==0 and r2.stdout==r.stdout,'second run differs'
 rec=dict(command=cmd,runner=runner,unit=unit,binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest())
 if ref:
  h=run(ref,[]);assert h.returncode==0,(h.stdout,h.stderr[-8000:])
  assert h.stdout==r.stdout,('host output differs',h.stdout,r.stdout);rec['host_equal']=True
 (out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n')
 print('wp8 unit checks deterministic'+(' with identical host and m68k output' if ref else ''))
if __name__=='__main__':main()
