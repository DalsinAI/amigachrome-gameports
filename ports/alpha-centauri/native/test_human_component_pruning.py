#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,random,shlex,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;names=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','existing_prototype_reuse.cpp','human_component_pruning.cpp','human_component_pruning_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True);rng=random.Random(0x57f4b0);cases=[];combined=bytearray()
for v,version in [(0,7),(1,12)]:
 for mode in range(32):
  f=1+mode%7;wc=21 if not v else 26;ac=10 if not v else 14;data=bytearray(valid_fixture(version,3,2));_,off,_=decode(data);prefs=off['legacy'] if not v else off['preferences'];data[prefs+0x58]=0 if mode%8==7 else 1<<f;struct.pack_into('<I',data,prefs+4,0x100000 if mode%8==6 else 0);runtime=mode%3 if mode%8 in [4,5] else 1
  pre=[(rng.choice([-2,-1,0,1,88]),rng.choice([-2,-1,0,1,88])) for _ in range(89)];speed=[rng.choice([1,2,4]) for _ in range(9)];triad=[i%3 for i in range(9)];weapon=[rng.choice([-1,0,1,2,4]) for _ in range(26)];armor=[rng.choice([-1,0,1,2,4]) for _ in range(14)];owners=bytes(rng.randrange(256) for _ in range(89));ownerOffset=prefs+(0x164 if not v else 0x1e0);data[ownerOffset:ownerOffset+89]=owners
  for i in range(512):
   p=off['prototypes']+52*i;data[p+36]=rng.randrange(9);data[p+37]=rng.randrange(wc);data[p+38]=rng.randrange(ac);struct.pack_into('<Hh',data,p+48,rng.choice([0,1,1,5,0x81,0x104]),rng.choice([-2,-1,0,1,88]))
  selected=0 if mode&1 else f*64+7
  if mode&2:struct.pack_into('<H',data,off['prototypes']+52*selected+48,5)
  before=[struct.unpack_from('<H',data,off['prototypes']+52*i+48)[0] for i in range(512)];flags=before.copy();blocked=runtime!=1 or mode%8 in [6,7]
  def known(id):
   if id==-1:return True
   if not 0<=id<88:return False
   x,y=pre[id];return x>=-1 and (y>=-1 or x==-1) and bool(owners[id]&(1<<f))
  if not blocked and (selected<64 or flags[selected]&4):
   for id in range(f*64,(f+1)*64):
    if not flags[id]&1 or flags[id]&4:continue
    p=off['prototypes']+52*id;ch,w,ar=data[p+36:p+39];got=set()
    for other in list(range(64))+list(range(f*64,(f+1)*64)):
     q=off['prototypes']+52*other;t=struct.unpack_from('<h',data,q+50)[0]
     if not flags[other]&1 or (not known(t) if other<64 else not flags[other]&4):continue
     bch,bw,bar=data[q+36:q+39]
     if w==bw or 0<weapon[w]<=weapon[bw]:got.add('w')
     if ar==bar or 0<armor[ar]<=armor[bar]:got.add('a')
     if ch==bch or triad[ch]==triad[bch] and speed[ch]<=speed[bch]:got.add('c')
    if len(got)==3:flags[id]|=0x104
  expected=struct.pack('<I',2) if blocked else struct.pack('<2I',0,sum(x!=y for x,y in zip(before,flags)))+struct.pack('<512I',*flags)
  facts=struct.pack('<4I',f,selected,runtime,v)+b''.join(struct.pack('<2i',*x) for x in pre)+b''.join(struct.pack('<2i',*x) for x in zip(speed,triad))+struct.pack('<26i',*weapon)+struct.pack('<14i',*armor);assert len(facts)==960
  num=len(cases);path=out/(str(num)+'.SC');path.write_bytes(data);fp=out/(str(num)+'.facts');fp.write_bytes(facts);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(fp)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(v,mode,r.returncode,r.stderr);assert r.stdout==expected,(v,mode,len(r.stdout),len(expected));combined+=r.stdout;cases.append(dict(variant=v,faction=f,selected=selected,blocked=blocked,changed=sum(x!=y for x,y in zip(before,flags)) if not blocked else 0))
# Invalid structural facts reject transactionally; no original unsafe reads.
for guard in range(7):
 data=bytearray(valid_fixture(12,3,2));_,off,_=decode(data);prefs=off['preferences'];data[prefs+0x58]=2;struct.pack_into('<I',data,prefs+4,0)
 for i in range(512):struct.pack_into('<H',data,off['prototypes']+52*i+48,0)
 q=off['prototypes']+52*64;data[q+36:q+39]=bytes(3);struct.pack_into('<Hh',data,q+48,1,-1)
 facts=bytearray(struct.pack('<4I',1,0,1,1)+bytes(944))
 if guard==0:struct.pack_into('<i',facts,16,-3)
 if guard==1:struct.pack_into('<i',facts,728,-1)
 if guard==2:struct.pack_into('<i',facts,800,128)
 if guard==3:data[q+37]=26
 if guard==4:struct.pack_into('<I',facts,12,0)
 if guard==5:struct.pack_into('<I',facts,0,0)
 if guard==6:struct.pack_into('<I',facts,4,512)
 num=len(cases);path=out/(str(num)+'.SC');path.write_bytes(data);fp=out/(str(num)+'.facts');fp.write_bytes(facts)
 r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(fp)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(guard,r.returncode,r.stderr);assert r.stdout==struct.pack('<I',1);combined+=r.stdout;cases.append(dict(guard=guard,blocked=True,variant=1))
(out/'outputs.bin').write_bytes(combined);receipt=dict(cases=cases,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(len(cases),receipt['outputSHA256'])
