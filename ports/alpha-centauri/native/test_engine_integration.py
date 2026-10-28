#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Combined engine control: End Turn (WP1), base upkeep (WP2) and combat (WP4)
on one state, through a saved game.

Builds engine/tests/integration_test.cpp with every engine source, on the host
with ASan/UBSan or for m68k-linux under qemu (big-endian), then:
- "make" writes a synthetic saved game (synthetic rules and world, no original
  assets): three factions, bases producing units and facilities, opposing
  units on a front line;
- "run" loads it with load_daemon_bytes and makes TURNS End Turn calls, with a
  scripted human attack before every second call; it prints the battles, the
  turn results and the base state, and saves the final game;
- every run is made twice and must give identical output and saved bytes;
- with --m68k-linux, a plain host build must make the same synthetic save and
  give the same output and saved bytes as the big-endian run.
While WP3 is a stub, the speed functions come from the combat test's Thinker
test double (test_engine_wp4.wp3_double), as in the combat package test.
Writes receipt.json to the output directory.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
from test_engine_wp4 import wp3_double

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'
TURNS=8

def sources(out):
 stubs=sorted((ENGINE/'stubs').glob('*.cpp'));double=wp3_double(out)
 if double:stubs=[p for p in stubs if p.name!='wp3.cpp']+[double]
 return sorted(ENGINE.glob('*.cpp'))+stubs+[ENGINE/'tests'/'integration_test.cpp']

def build(cc,flags,exe,srcs):
 cmd=[cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',*flags,'-I',str(ENGINE),*map(str,srcs),'-o',str(exe)]
 subprocess.run(cmd,check=True);return cmd

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--turns',type=int,default=TURNS);a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 srcs=sources(out);exe=out/'integration-test'
 if a.m68k_linux:
  cmd=build(a.cc,['-m68040','-static'],exe,srcs);runner=['qemu-m68k','-cpu','m68040']
  ref=out/'integration-test-host';build('c++',[],ref,srcs)
 else:
  cmd=build(a.cc,['-fsanitize=address,undefined','-fno-sanitize-recover=all'],exe,srcs);runner=[];ref=None
 def run(binary,pre,*args):
  r=subprocess.run([*pre,str(binary),*map(str,args)],capture_output=True,text=True,timeout=1800,env=env)
  assert r.returncode==0,(args,r.returncode,r.stdout[-3000:],r.stderr[-6000:]);return r.stdout
 save=out/'synthetic.SAV';made=run(exe,runner,'make',save);print(made.strip())
 outs=[]
 for k in range(2):
  dst=out/f'turns{k}.SAV';text=run(exe,runner,'run',save,dst,a.turns);outs.append((text,dst.read_bytes()))
 assert outs[0]==outs[1],'second run differs'
 text,final=outs[0];lines=text.split('\n')
 calls=[l for l in lines if l.startswith('call=')];attacks=[l for l in lines if l.startswith('attack ')]
 summary=[l for l in lines if l.startswith('battles=')][0]
 assert len(calls)==a.turns,text
 assert all(' status=0 faction=1 ' in c for c in calls),calls
 assert attacks,text
 turns=[int(c.split(' turn=')[1].split()[0]) for c in calls]
 assert turns==list(range(turns[0],turns[0]+a.turns)),turns
 battles=int(summary.split('battles=')[1].split()[0]);assert battles>=1,summary
 # Bases produced units: more vehicles at the end than the battles left.
 v0,v1=map(int,summary.split('vehicles=')[1].split()[0].split('->'));assert v1>v0-battles,summary
 # Base 1 (faction 1) builds Recycling Tanks (facility 3, bit 0x08 of the first byte).
 last_base1=[l for l in lines if l.startswith('  base=1 ')][-1]
 assert int(last_base1.split('facilities=')[1],16)&0x08,last_base1
 rec=dict(command=cmd,runner=runner,turns=a.turns,made=made.strip(),synthetic_sha256=hashlib.sha256(save.read_bytes()).hexdigest(),
  output_sha256=hashlib.sha256(final).hexdigest(),attacks=attacks,calls=calls,summary=summary,
  binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest())
 if ref:
  hsave=out/'synthetic.host.SAV';run(ref,[],'make',hsave)
  assert hsave.read_bytes()==save.read_bytes(),'host synthetic save differs from m68k'
  hdst=out/'turns.host.SAV';htext=run(ref,[],'run',save,hdst,a.turns)
  assert htext==text,('host output differs',htext,text)
  assert hdst.read_bytes()==final,'host saved bytes differ from m68k'
  rec['host_equal']=True
 (out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n')
 for l in attacks:print(l)
 print(calls[-1]);print(summary)
 print(f'{a.turns} End Turn calls with production and {len(attacks)} scripted attacks ran deterministically'
  +(' with byte-identical host and m68k output and saves' if ref else ''))
if __name__=='__main__':main()
