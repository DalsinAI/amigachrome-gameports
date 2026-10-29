#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess
from test_original_game import decode
from test_local_workers_snapshot import fixture as union_fixture,moved_model,accounting_pattern,worker_pattern
from test_production_snapshot import expected as production_pattern
SRC=Path(__file__).resolve().parent

def fixture(v,vehicles=3,bases=2):
 b=bytearray(union_fixture(v,vehicles,bases));_,o,_=decode(b);fs=0x20bc if v<11 else 0x20cc
 for f in range(8):
  p=o['factions']+f*fs;struct.pack_into('<I',b,p,(struct.unpack_from('<I',b,p)[0]&~32)|(32 if f%2==0 else 0))
 return bytes(b)

def phase_pattern(data):
 # Independent wire-offset allowlist, not read/generated from the C++ include.
 b=bytearray(data);info,o,_=decode(data);old=int(info['version'])<11;fs=0x20bc if old else 0x20cc
 offsets=[*range(0xc4,0xe4,4),0x220,*range(0x238,0x2d0,4),0x2fc,0x300,0x304,0x308,0x314,0x318,0x31c,0x320,0x364,0x3f8,*range(0x690,0x808,4)]
 assert len(offsets)==151
 for f in range(8):
  p=o['factions']+f*fs;struct.pack_into('<I',b,p,struct.unpack_from('<I',b,p)[0]&~32)
  for cell,offset in enumerate(offsets,1):struct.pack_into('<I',b,p+offset-(4 if old and offset>=0x464 else 0),((cell*0x01020304)^(f*0x10203040))&0xffffffff)
 return bytes(b)

def credit(data,increment=False):
 b=bytearray(data);info,o,_=decode(data);fs=0x20bc if int(info['version'])<11 else 0x20cc;p=o['factions']+fs+0x220;old=struct.unpack_from('<I',b,p)[0];struct.pack_into('<I',b,p,old+1 if increment else old^1);return bytes(b)

def expected(data,mode):
 if mode==4:return credit(data,True)
 effective=19 if mode==26 else mode
 b=moved_model(data) if effective in [1,15,17,19] else data
 if effective in [14,15,17,19]:b=production_pattern(1,b'',b,'base',False,1)[4:]
 if effective in [16,17,19]:b=accounting_pattern(b)
 if effective in [18,19]:b=worker_pattern(b)
 if mode in [25,26]:b=phase_pattern(b)
 return b

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);names=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','vehicle_stack.cpp','local_workers_snapshot.cpp','phase_snapshot.cpp','phase_snapshot_probe.cpp'];cmd=[a.cc,*flags,*[str(SRC/name) for name in names],'-o',str(out/'probe')];subprocess.run(cmd,check=True);corpus=[(f'v{v}',fixture(v)) for v in range(7,13)]+[('empty7',fixture(7,0,0)),('empty12',fixture(12,0,0))]+[('owner-'+p.name,p.read_bytes()) for p in a.original];runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];digest=hashlib.sha256();records=[];success=fail=continuations=0
 good=[0,1,4,14,15,16,17,18,19,25,26]
 for name,data in corpus:
  path=out/(name+'.SC');path.write_bytes(data);info,o,_=decode(data);modes=list(range(38)) if int(info['vehicles']) and int(info['bases']) else [0,3,4,5,7,8,9,12,14,25,27,28,29,30,31,32,33,34,35,36,37];fs=0x20bc if int(info['version'])<11 else 0x20cc
  if struct.unpack_from('<I',data,o['factions']+fs)[0]&32:modes.remove(37)
  for mode in modes:
   dest=out/f'{name}-{mode}.out';r=subprocess.run([*runner,str(out/'probe'),str(path),str(dest),str(mode)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(name,mode,r.returncode,r.stdout,r.stderr)
   if mode in good:
    want=expected(data,mode);assert dest.read_bytes()==want,(name,mode,'wire mismatch',next((i for i,(x,y) in enumerate(zip(dest.read_bytes(),want)) if x!=y),None));digest.update(want);success+=1
    if mode in [4,25,26]:actual=Path(str(dest)+'.continued').read_bytes();assert actual==credit(want),(name,mode,'continued mismatch');digest.update(actual);continuations+=1
   else:assert not dest.exists() and 'unchanged=1 rc=0' in r.stdout,(name,mode,r.stdout);fail+=1
   assert path.read_bytes()==data;records.append(dict(name=name,mode=mode,passed=True,output=r.stdout))
  r=subprocess.run([*runner,str(out/'probe'),str(path),str(path),'0'],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==20 and path.read_bytes()==data;records.append(dict(name=name,mode='existing-target',passed=True));fail+=1
 receipt=dict(worlds=len(corpus),controls=len(records),exports=success,rejected=fail,continuations=continuations,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd,records=records);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
