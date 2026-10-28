#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,random,shlex,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
names=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','retirement_history.cpp','prototype_retirement_queue.cpp','unused_prototype_retirement.cpp','unused_prototype_retirement_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True);rng=random.Random(0x57f1d0);cases=[];combined=bytearray()
for variant,version in [(0,7),(1,12)]:
 for mode in range(32):
  faction=1+mode%7;id=faction*64+(mode*11)%64;vehicles=[0,3,20,128][mode%4];bases=[0,1,3,32,512][mode%5];data=bytearray(valid_fixture(version,vehicles,bases));_,off,_=decode(data);fs=0x20bc if version<11 else 0x20cc;bs=304 if version<11 else 308;cursor=(mode*13)%64;flags=rng.randrange(65536);p=off['prototypes']+52*id;struct.pack_into('<H',data,p+48,flags);struct.pack_into('<i',data,off['factions']+fs*faction+(0x207c if version<11 else 0x208c),cursor)
  for i in range(vehicles):struct.pack_into('<h',data,off['vehicles']+52*i+10,(id+1+i)%512)
  live=vehicles>0 and bool(mode&8)
  if live:struct.pack_into('<h',data,off['vehicles']+10,id)
  queues=[]
  for i in range(bases):
   last=[-1,0,1,8,9][(mode+i)%5];items=[rng.choice([id,id,-38,-69,2]) for _ in range(10)];struct.pack_into('<11i',data,off['bases']+i*bs+76,last,*items);n=last;j=0
   while j<=n:
    if items[j]==id:
     if j==0:items[0]=-38 if variant==0 else -69
     else:
      for k in range(j,n):items[k]=items[k+1]
      n-=1
    j+=1
   queues+=struct.pack('<11i',n,*items)
  table=bytearray(data[off['bunglist']:off['bunglist']+4096]);table[(faction*64+cursor)*8:(faction*64+cursor+1)*8]=data[p+32:p+40]
  expected=struct.pack('<I',2) if live else struct.pack('<4I',0,(cursor+1)%64,flags&~1,bases)+table+bytes(queues)
  path=out/(str(len(cases))+'.SC');path.write_bytes(data);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(variant),str(faction),str(id)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(mode,variant,r.returncode,r.stderr);assert r.stdout==expected,(mode,variant,len(r.stdout),len(expected));combined+=r.stdout;cases.append(dict(variant=variant,version=version,faction=faction,id=id,liveRejected=live,vehicles=vehicles,bases=bases,cursor=cursor,sha256=hashlib.sha256(data).hexdigest()))
# Whole-world guard failures must not expose a partial proposal.
for variant,faction,id,cursor in [(0,1,64,None),(1,0,64,None),(1,1,0,None),(1,2,64,None),(1,1,64,-1),(1,1,64,64)]:
 data=bytearray(valid_fixture(12,3,2));_,off,_=decode(data)
 for i in range(3):struct.pack_into('<h',data,off['vehicles']+52*i+10,320+i)
 struct.pack_into('<i',data,off['factions']+0x20cc+0x208c,0 if cursor is None else cursor)
 path=out/(str(len(cases))+'.SC');path.write_bytes(data);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(variant),str(faction),str(id)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60)
 assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==struct.pack('<I',1);combined+=r.stdout;cases.append(dict(guard=True,variant=variant,faction=faction,id=id,cursor=cursor))
(out/'outputs.bin').write_bytes(combined);receipt=dict(cases=cases,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(len(cases),receipt['outputSHA256'])
