#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""World generation and new game controls (engine/mapgen*.cpp, setup.cpp,
newgame.cpp; WP7).

Builds engine/tests/wp7_test.cpp with every engine source, on the host with
ASan/UBSan or for m68k-linux under qemu (big-endian, 68040 FPU), and runs it
against synthetic rules directories (test_engine_wp0b.make_dir: alpha.txt for
the base game, alphax.txt for Alien Crossfire, no original assets):
- unit checks (FastNoiseLite samples, Path_continents on a drawn map,
  fixup_landmarks, setup_game resets, rejected settings);
- seeded maps for every planet size and a spread of ocean, erosion and native
  settings: the same seed must give the same bytes on a second run, other
  seeds different bytes;
- the new game screens (ui_newgame.cpp) driven by keys from faction pick to
  planetfall;
- new games for several seeds and player factions: valid start positions for
  every faction, a planetfall notice, and a save that round-trips through
  save_daemon_bytes/load_daemon_bytes (only PFLAG_MAP_REVEALED, which
  mod_load_daemon recomputes, may change; a second round trip is exact).
With --m68k-linux a host reference build (no sanitizers) runs the same cases
and every map dump and save must be byte-identical to the m68k output.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import test_engine_wp0b

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'

MAP_CASES=[(seed,size,ocean,erosion,native) for seed,size,ocean,erosion,native in [
 (1,0,0,0,0),(2,1,1,1,1),(12345,2,1,1,1),(0xDEADBEEF,2,2,2,2),(77,3,0,2,1),(4242,4,1,0,2),(9,2,-1,-1,-1),
 (0x7FFFFFFF,1,2,1,0)]]
NEWGAME_CASES=[(777,2,3),(1,0,1),(31337,1,5),(2024,3,7),(0xCAFEBABE,2,2)]

def build(cc,flags,exe):
 sources=sorted(ENGINE.glob('*.cpp'))+sorted((ENGINE/'stubs').glob('*.cpp'))+[SRC/'ui_newgame.cpp',ENGINE/'tests'/'wp7_test.cpp']
 cmd=[cc,*flags,'-I',str(ENGINE),*map(str,sources),'-o',str(exe)];subprocess.run(cmd,check=True);return cmd

def runner_for(exe,runner):
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 def run(*args):return subprocess.run([*runner,str(exe),*map(str,args)],capture_output=True,text=True,timeout=1200,env=env)
 return run

def check(r,name):
 assert r.returncode==0,(name,r.returncode,r.stdout[-3000:],r.stderr[-3000:])
 return r.stdout

def run_cases(run,out,rules,tag):
 res={'maps':{},'games':{}}
 for variant in ['base','expansion']:
  for seed,size,ocean,erosion,native in MAP_CASES:
   name=f'{variant}-map-{seed:x}-{size}-{ocean}{erosion}{native}'
   a=out/f'{name}.{tag}.bin';b=out/f'{name}.{tag}.again.bin'
   line=check(run('map',rules[variant],variant,seed,size,ocean,erosion,native,a),name).strip()
   check(run('map',rules[variant],variant,seed,size,ocean,erosion,native,b),name+'-again')
   assert a.read_bytes()==b.read_bytes(),(name,'second run differs')
   res['maps'][name]=dict(summary=line,path=str(a),sha256=hashlib.sha256(a.read_bytes()).hexdigest())
  # Different seeds give different worlds.
  hashes=[v['sha256'] for k,v in res['maps'].items() if k.startswith(variant+'-')]
  assert len(set(hashes))==len(hashes),(variant,'seed collision')
  for seed,size,player in NEWGAME_CASES:
   name=f'{variant}-game-{seed:x}-{size}-p{player}'
   sav=out/f'{name}.{tag}.sav'
   text=check(run('newgame',rules[variant],variant,seed,size,player,sav),name)
   starts=[l for l in text.splitlines() if l.startswith('start ')]
   last=text.strip().splitlines()[-1]
   assert 'reload=identical' in last and len(starts)>=2,(name,text[-2000:])
   res['games'][name]=dict(starts=starts,summary=last,path=str(sav),sha256=hashlib.sha256(sav.read_bytes()).hexdigest())
 return res

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--host-cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'wp7-test';cmd=build(a.cc,flags,exe);run=runner_for(exe,runner)
 rules={}
 for variant in ['base','expansion']:
  rules[variant]=out/f'rules-{variant}';test_engine_wp0b.make_dir(rules[variant],variant)
 unit=check(run('unit'),'unit').strip();print(unit)
 ui={}
 for variant in ['base','expansion']:
  ui[variant]=check(run('ui',rules[variant],variant,4321),'ui-'+variant);print(ui[variant].strip().splitlines()[-1])
 res=run_cases(run,out,rules,'m68k' if a.m68k_linux else 'host')
 compared=0
 if a.m68k_linux:
  ref=out/'wp7-host-reference';build(a.host_cc,['-std=c++17','-O1','-Wall','-Wextra','-Werror'],ref)
  hrun=runner_for(ref,[])
  hunit=check(hrun('unit'),'host-unit').strip()
  for variant in ['base','expansion']:
   assert check(hrun('ui',rules[variant],variant,4321),'host-ui')==ui[variant],('ui screens differ',variant)
   compared+=1
  assert hunit.splitlines()[0]==unit.splitlines()[0],('noise samples differ',hunit,unit)
  href=run_cases(hrun,out,rules,'hostref')
  for kind in ['maps','games']:
   for k,v in res[kind].items():
    h=href[kind][k]
    assert Path(v['path']).read_bytes()==Path(h['path']).read_bytes(),(k,'m68k and host bytes differ')
    if kind=='maps':assert v['summary']==h['summary'],(k,v['summary'],h['summary'])
    else:assert v['starts']==h['starts'],(k,v['starts'],h['starts'])
    compared+=1
 for kind in ['maps','games']:
  for k,v in res[kind].items():print(f'{k}: {v["summary"]}')
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,unit=unit,ui=ui,results=res,
   cross_compared=compared,binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n')
 print(f'{len(res["maps"])} map cases and {len(res["games"])} new game cases passed'+
  (f'; {compared} outputs byte-identical to the host reference' if a.m68k_linux else ''))
if __name__=='__main__':main()
