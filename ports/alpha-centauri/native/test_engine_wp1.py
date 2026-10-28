#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""WP1 turn scheduler controls (engine/turn.cpp, engine/turn_faction.cpp).

Builds engine/tests/wp1_test.cpp with every engine/*.cpp and engine/stubs/*.cpp
source, on the host with ASan/UBSan or for m68k-linux under qemu (big-endian), then:
- runs the synthetic-world unit checks (turn order, human/AI/hotseat stops, year
  and turn counter, movement refill, repair, loans, territory, rankings, resume
  after load, same-seed determinism);
- loads every synthetic save of test_engine.valid_cases() (patterned bytes from
  test_original_game.fixture, no original assets), runs several End Turn calls and
  saves; each case runs twice and must give identical bytes and event digests;
- with --m68k-linux, also builds a plain host binary and requires the m68k run to
  write byte-identical saves and identical per-call output (host vs big-endian).
Writes receipt.json to the output directory.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
from test_engine import valid_cases

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'
TURNS=6

def build(cc,flags,exe):
 sources=sorted(ENGINE.glob('*.cpp'))+sorted((ENGINE/'stubs').glob('*.cpp'))+[ENGINE/'tests'/'wp1_test.cpp']
 cmd=[cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',*flags,'-I',str(ENGINE),*map(str,sources),'-o',str(exe)]
 subprocess.run(cmd,check=True);return cmd

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--turns',type=int,default=TURNS);a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 exe=out/'wp1-test'
 if a.m68k_linux:
  cmd=build(a.cc,['-m68040','-static'],exe);runner=['qemu-m68k','-cpu','m68040']
  ref=out/'wp1-test-host';build('c++',[],ref)
 else:
  cmd=build(a.cc,['-fsanitize=address,undefined','-fno-sanitize-recover=all'],exe);runner=[];ref=None
 def run(binary,pre,*args):
  return subprocess.run([*pre,str(binary),*map(str,args)],capture_output=True,text=True,timeout=1800,env=env)
 r=run(exe,runner,'unit');assert r.returncode==0,(r.stdout,r.stderr);unit=r.stdout.strip();print(unit)
 records=[]
 cases=[(n,d,0) for n,d in valid_cases()]
 cases+=[(n+'-human3',d,3) for n,d in valid_cases() if n in ('v7-scenario0','v12-scenario1','legacy-actors','max-actors')]
 for i,(name,data,human) in enumerate(cases):
  src=out/f'{i}.SAV';src.write_bytes(data);outs=[]
  for k in range(2):
   dst=out/f'{i}.turn{k}.SAV';r=run(exe,runner,'run',src,dst,a.turns,human)
   assert r.returncode==0,(name,k,r.returncode,r.stdout,r.stderr[-4000:])
   outs.append((r.stdout,dst.read_bytes()))
  assert outs[0]==outs[1],(name,'second run differs')
  lines=outs[0][0].split('\n');calls=[l for l in lines if l.startswith('call=')]
  assert len(calls)==a.turns,(name,outs[0][0])
  if human:assert all(' status=0 faction=%d '%human in c for c in calls),(name,calls)
  rec=dict(name=name,human=human,input_sha256=hashlib.sha256(data).hexdigest(),output_sha256=hashlib.sha256(outs[0][1]).hexdigest(),
   calls=calls,digest=[l for l in lines if l.startswith('events_digest=')][0])
  if ref:
   dst=out/f'{i}.host.SAV';r=run(ref,[],'run',src,dst,a.turns,human);assert r.returncode==0,(name,'host',r.stdout,r.stderr[-4000:])
   assert r.stdout==outs[0][0],(name,'host output differs',r.stdout,outs[0][0])
   assert dst.read_bytes()==outs[0][1],(name,'host save bytes differ from m68k')
   rec['host_equal']=True
  records.append(rec);print(name,calls[-1],rec['digest'])
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,unit=unit,turns=a.turns,cases=records,
   binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n')
 print(f'{len(records)} synthetic save runs made {a.turns} End Turn calls deterministically'
  +(' with byte-identical host and m68k saves' if ref else ''))
if __name__=='__main__':main()
