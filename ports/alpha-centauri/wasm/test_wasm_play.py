#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The native scripted play test (native/test_play_session.py) built with
emscripten and run as WebAssembly under node, against a plain host build of
the same sources.

Same sources (test_play_session.sources()), same synthetic rules directory
(no original assets), same seed and turn count. The WebAssembly program
reaches the real files through NODERAWFS. Checks:
- the WebAssembly game is played twice and must give identical output/saves;
- its stdout, play.SAV and final.SAV must be byte-identical to the host run;
- the same play assertions as test_play_session.py (TURNS End Turns with
  status 0, a base, production, attack, research, three equal CONTINUE
  lines, no FAIL);
- with --native-receipt, the hashes must equal those in a receipt.json that
  test_play_session.py wrote (host ASan/UBSan or m68k-linux).
Writes receipt.json to the output directory.

  test_wasm_play.py --out NEW_DIR [--emxx em++] [--node node] [--native-receipt R]
"""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path

HERE=Path(__file__).resolve().parent
NATIVE=HERE.parent/'native'
sys.path.insert(0,str(NATIVE))
import test_play_session as tps  # noqa: E402

# Emscripten link settings for a headless node program. NODERAWFS maps the
# real file system; the engine's recursion and frame buffers need more than
# the 64 KiB default stack.
# Clang (emscripten) warns on three things GCC does not: brace elision in the
# engine's Thinker tables, GCC-only warning pragmas and set-only globals.
# Everything else stays -Wall -Wextra -Werror as in the native build.
CLANG_QUIET=['-Wno-missing-braces','-Wno-unknown-warning-option','-Wno-unused-but-set-global']
# libc++ lacks libstdc++'s std::__gcd (engine/combat.cpp): see the header.
COMPAT=['-include',str(HERE/'compat'/'libstdcxx_compat.h')]
EM_FLAGS=[*CLANG_QUIET,*COMPAT,'-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sALLOW_MEMORY_GROWTH=1','-sINITIAL_MEMORY=64MB','-sSTACK_SIZE=8MB','-sENVIRONMENT=node']

def sha(b):return hashlib.sha256(b).hexdigest()

def main():
 ap=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
 ap.add_argument('--out',required=True,type=Path);ap.add_argument('--emxx',default='em++');ap.add_argument('--node',default='node')
 ap.add_argument('--cxx',default='c++');ap.add_argument('--turns',type=int,default=tps.TURNS);ap.add_argument('--seed',type=int,default=tps.SEED)
 ap.add_argument('--opt',default='-O2');ap.add_argument('--native-receipt',type=Path)
 a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 rules=out/'rules';tps.test_engine_wp0b.make_dir(rules,'expansion',rules=tps.playable_rules('expansion'))
 js=out/'play-control.js'
 cmd=[a.emxx,'-std=c++17',a.opt,'-Wall','-Wextra','-Werror',*EM_FLAGS,'-I',str(tps.ENGINE),'-I',str(tps.SRC),*map(str,tps.sources()),'-o',str(js)]
 subprocess.run(cmd,check=True)
 ref=out/'play-control-host';host_cmd=tps.build(a.cxx,[],ref)
 def play(pre,tag):
  d=out/tag;d.mkdir()
  r=subprocess.run([*pre,'play',str(rules),'1',str(a.seed),str(d),str(a.turns)],capture_output=True,text=True,timeout=3600)
  (d/'stdout.txt').write_text(r.stdout)
  assert r.returncode==0,(tag,r.returncode,r.stdout[-4000:],r.stderr[-4000:])
  return r.stdout,(d/'play.SAV').read_bytes(),(d/'final.SAV').read_bytes()
 runs=[play([a.node,str(js)],f'wasm{k}') for k in range(2)]
 assert runs[0]==runs[1],'second WebAssembly run differs'
 text,mid,final=runs[0];lines=text.split('\n')
 turn_lines=[l for l in lines if l.startswith('A TURN ')]
 assert len(turn_lines)>=a.turns,turn_lines
 assert all(' status=0 faction=1 ' in l for l in turn_lines[:a.turns]),turn_lines
 assert [l for l in lines if l.startswith('A CMD build ') and l.endswith('-> ok')],'no base founded'
 assert [l for l in lines if l.startswith('A CMD produce ') and l.endswith('-> ok')],'production not set'
 attack=[l for l in lines if l.startswith('A CMD attack ')];assert attack and attack[0].endswith('-> ok'),attack
 assert [l for l in lines if l.startswith('A CMD research ') and l.endswith('-> ok')],'research not changed'
 cont=[l for l in lines if l.startswith('CONTINUE ')];assert len(cont)==3 and all(l.split()[3][2:]==l.split()[4][2:] for l in cont),cont
 assert not [l for l in lines if l.startswith('FAIL')],[l for l in lines if l.startswith('FAIL')]
 summary=[l for l in lines if l.startswith('FINAL ')][0]
 htext,hmid,hfinal=play([str(ref)],'host')
 assert htext==text,'host output differs from WebAssembly'
 assert hmid==mid and hfinal==final,'host saves differ from WebAssembly'
 wasm=js.with_suffix('.wasm')
 rec=dict(command=cmd,host_command=host_cmd,turns=a.turns,seed=a.seed,summary=summary,turn_lines=turn_lines,attack=attack,continued=cont,
  output_sha256=sha(text.encode()),save_sha256=sha(mid),final_sha256=sha(final),host_equal=True,
  wasm_sha256=sha(wasm.read_bytes()),wasm_bytes=wasm.stat().st_size,js_bytes=js.stat().st_size,
  node=subprocess.run([a.node,'--version'],capture_output=True,text=True).stdout.strip(),
  emxx=subprocess.run([a.emxx,'--version'],capture_output=True,text=True).stdout.split('\n')[0])
 if a.native_receipt:
  nat=json.loads(a.native_receipt.read_text())
  for k in ('output_sha256','save_sha256','final_sha256','summary'):
   assert nat[k]==rec[k],(k,nat[k],rec[k])
  rec['native_receipt']=str(a.native_receipt);rec['native_equal']=True
 (out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n')
 print(turn_lines[-1]);print(attack[0]);print(summary)
 print(f'output {rec["output_sha256"]}\nplay.SAV {rec["save_sha256"]}\nfinal.SAV {rec["final_sha256"]}')
 print(f'{a.turns} End Turns in WebAssembly under node: deterministic, byte-identical output and saves to the host build'
  +(' and to '+str(a.native_receipt) if a.native_receipt else ''))
if __name__=='__main__':main()
