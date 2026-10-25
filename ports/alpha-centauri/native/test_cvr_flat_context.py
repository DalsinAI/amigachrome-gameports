# SPDX-License-Identifier: GPL-3.0-or-later
"""Conservative terrain-context guards, wholly synthetic original-world input."""
import argparse,json,struct,subprocess,os
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'];runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else [];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/n)for n in ['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','cvr_flat_context.cpp','cvr_flat_context_probe.cpp']],'-o',str(exe)],check=True)
 b=bytearray(valid_fixture());_,offsets,_=decode(b);start=offsets['tiles'];
 def tile(x,y):return start+(y*4+x//2)*44
 for y in range(6):
  for x in range(y&1,8,2):b[tile(x,y)]=3<<5;struct.pack_into('<I',b,tile(x,y)+12,0)
 cases=[]
 for code in range(1,8):
  A,B=next((aa,bb)for aa in range(3,7)for bb in range(3,7)if aa+2*bb-8==code);v=bytearray(b);v[tile(4,2)]=4<<5
  for x,y in [(2,2),(4,0),(6,2),(4,4)]:v[tile(x,y)]=A<<5
  for x,y in [(3,1),(5,1),(3,3),(5,3)]:v[tile(x,y)]=B<<5
  cases.append((f'uniform-corner{code}',v,4,2,[(1,1,code,code*17//4),(2,1,code,code*32//4)]))
 good=bytearray(cases[3][1]);reject=[(1,0,0,0),(2,0,0,0)]
 for bit in [1,2,0x100,0x200,0x800]:
  for x,y in [(4,2),(3,1)]:
   v=bytearray(good);struct.pack_into('<I',v,tile(x,y)+12,bit);cases.append((f'feature-{bit}-{x}-{y}',v,4,2,reject))
 for name,x,y,level in [('slope',4,0,5),('coast',3,1,2),('clipped-height',4,2,7)]:v=bytearray(good);v[tile(x,y)]=level<<5;cases.append((name,v,4,2,reject))
 for x,y in [(-1,1),(8,0),(0,-2),(0,6),(1,0),(0,0)]:cases.append((f'edge-or-invalid-{x}-{y}',good,x,y,reject))
 sea=bytearray(good);sea[tile(4,2)]=2<<5;cases.append(('sea',sea,4,2,[(1,1,4,0),(2,1,4,0)]));records=[]
 for i,(name,data,x,y,want)in enumerate(cases):
  path=out/f'{i}.SC';path.write_bytes(data);r=subprocess.run(runner+[str(exe),str(path),str(x),str(y)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr);got=[tuple(map(int,line.split()))for line in r.stdout.splitlines()];assert got==want,(name,got,want);records.append(dict(name=name,result=got))
 (out/'receipt.json').write_text(json.dumps(dict(cases=records,originalCodeExecuted=False,bigEndian=a.m68k_linux),indent=2)+'\n');print('Known flat context',len(cases),'synthetic cases PASS')
if __name__=='__main__':main()
