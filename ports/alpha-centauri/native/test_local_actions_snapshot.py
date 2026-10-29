#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Union movement/production container encoding, not original-game save proof."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_production_snapshot import expected as production_pattern
SRC=Path(__file__).resolve().parent

def fixture(version,vehicles=3,bases=2):
 b=bytearray(valid_fixture(version,vehicles,bases));info,o,_=decode(b);w=int(info['width'])
 for i in range(w*int(info['height'])//2):
  p=o['tiles']+44*i;items=struct.unpack_from('<I',b,p+8)[0];struct.pack_into('<I',b,p+8,items&~2)
 for i in range(vehicles):
  x,y=struct.unpack_from('<hh',b,o['vehicles']+52*i);p=o['tiles']+((y*w+x)//2)*44
  items=struct.unpack_from('<I',b,p+8)[0];struct.pack_into('<I',b,p+8,items|2)
 return bytes(b)

def moved_model(data):
 b=bytearray(data);info,o,_=decode(data);w=int(info['width']);nv=int(info['vehicles']);vp=o['vehicles']
 x,y=struct.unpack_from('<hh',b,vp);tx,ty=x+1,y+1
 prev,next_=struct.unpack_from('<hh',b,vp+48)[::-1]
 if prev>=0:struct.pack_into('<h',b,vp+52*prev+48,next_)
 if next_>=0:struct.pack_into('<h',b,vp+52*next_+50,prev)
 source=o['tiles']+((y*w+x)//2)*44;dest=o['tiles']+((ty*w+tx)//2)*44
 if prev<0 and next_<0:struct.pack_into('<I',b,source+8,struct.unpack_from('<I',b,source+8)[0]&~2)
 top=-1
 for i in range(1,nv):
  p=vp+52*i
  if struct.unpack_from('<hh',b,p)==(tx,ty) and struct.unpack_from('<h',b,p+50)[0]<0:top=i;break
 struct.pack_into('<hh',b,vp,tx,ty);struct.pack_into('<hh',b,vp+48,top,-1)
 if top>=0:struct.pack_into('<h',b,vp+52*top+50,0)
 items=struct.unpack_from('<I',b,dest+8)[0];owner=b[vp+14]
 if top<0 and not(items&1):b[dest+2]=(b[dest+2]&0xf0)|owner
 struct.pack_into('<I',b,dest+8,items|2|(0x100000 if owner else 0))
 b[vp+40]=3;b[vp+41]=2;struct.pack_into('<I',b,vp+4,struct.unpack_from('<I',b,vp+4)[0]|4)
 return bytes(b)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--probe',type=Path);ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',action='append',type=Path,default=[]);a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 files=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','vehicle_stack.cpp','local_actions_snapshot.cpp','local_actions_snapshot_probe.cpp']
 exe=a.probe.resolve() if a.probe else out/'snapshot-probe';cmd=[a.cc,*flags,*[str(SRC/f) for f in files],'-o',str(exe)]
 if not a.probe:subprocess.run(cmd,check=True)
 corpus=[(f'v{v}',fixture(v)) for v in range(7,13)]+[('empty',fixture(12,0,0))]+[('original-'+p.name,p.read_bytes()) for p in a.original]
 records=[]
 for name,data in corpus:
  path=out/(name+'.SC');path.write_bytes(data);info,_,_=decode(data)
  modes=range(16) if int(info['vehicles']) and int(info['bases']) else [0,3,4,5,7,8,9,12,14]
  for mode in modes:
   dest=out/f'{name}-{mode}.out';r=subprocess.run([*runner,str(exe),str(path),str(dest),str(mode)],capture_output=True,text=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
   assert r.returncode==0,(name,mode,r.returncode,r.stdout,r.stderr)
   if mode in [0,1,14,15]:
    expected=moved_model(data) if mode in [1,15] else data
    if mode in [14,15]:expected=production_pattern(1,b'',expected,'base',False,1)[4:]
    assert dest.read_bytes()==expected,(name,mode,'independent byte mismatch')
    assert r.stdout.endswith('seed=abcdef01 rc=0\n')
   else:assert not dest.exists() and 'unchanged=1 rc=0' in r.stdout,(name,mode,r.stdout)
   assert path.read_bytes()==data
   records.append(dict(name=name,mode=mode,passed=True,output=r.stdout))
  r=subprocess.run([*runner,str(exe),str(path),str(path),'0'],capture_output=True,text=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  assert r.returncode==20 and path.read_bytes()==data,(name,'existing source target',r.returncode,r.stderr)
  records.append(dict(name=name,mode='existing-target',passed=True))
 digest=hashlib.sha256();wire_bytes=0
 for record in records:
  if record['mode'] in [0,1,14,15]:
   payload=(out/f"{record['name']}-{record['mode']}.out").read_bytes();digest.update(payload);wire_bytes+=len(payload)
 receipt=dict(command=None if a.probe else cmd,suppliedProbe=str(exe) if a.probe else None,runner=runner,cases=records,binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),outputSHA256=digest.hexdigest(),outputBytes=wire_bytes)
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(cases=len(records),outputSHA256=digest.hexdigest(),outputBytes=wire_bytes)))
if __name__=='__main__':main()
