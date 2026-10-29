#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Separate current monument ownership and fail-closed snapshot boundary."""
from pathlib import Path
import argparse,hashlib,json,os,random,subprocess
from test_local_workers_snapshot import fixture
from test_original_game import decode
from test_original_map import fnv
SRC=Path(__file__).resolve().parent
FILES=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','vehicle_stack.cpp','movement_snapshot.cpp','production_snapshot.cpp','local_actions_snapshot.cpp','local_accounting_snapshot.cpp','local_workers_snapshot.cpp','monument_storage_probe.cpp']
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='c++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--original',action='append',type=Path,default=[]);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(SRC/n) for n in FILES],'-o',str(out/'probe')];subprocess.run(cmd,check=True)
 rng=random.Random(20261026);corpus=[]
 for version in range(7,13):
  for mode in range(8):
   b=bytearray(fixture(version,0,0) if mode==7 else fixture(version));_,o,_=decode(b)
   b[o['monuments']:o['monuments']+0x27a0]=bytes(0x27a0) if mode==0 else bytes([255])*0x27a0 if mode==1 else rng.randbytes(0x27a0)
   corpus.append((f'v{version}-m{mode}',bytes(b)))
 corpus += [('original-'+p.name,p.read_bytes()) for p in a.original];results=[];combined=b''
 for i,(name,data) in enumerate(corpus):
  path=out/(str(i)+'.SC');path.write_bytes(data);info,o,_=decode(data)
  want=f"version={info['version']} controls=40 monuments={fnv(data[o['monuments']:o['monuments']+0x27a0]):08x} baseline={fnv(data):08x}\n".encode()
  r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stdout,r.stderr);assert r.stdout==want,(name,r.stdout,want);combined+=r.stdout
  results.append(dict(name=name,inputSHA256=hashlib.sha256(data).hexdigest(),result=r.stdout.decode().strip()))
 receipt=dict(worlds=len(results),controls=40*len(results),outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd,results=results);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k!='results'}))
if __name__=='__main__':main()
