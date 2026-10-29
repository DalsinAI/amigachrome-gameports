#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,random,shlex,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
names=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','retirement_history.cpp','prototype_retirement_queue.cpp','direct_prototype_retirement.cpp','direct_prototype_retirement_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True);rng=random.Random(0x57f1d1);cases=[];combined=bytearray()
for variant,version in [(0,7),(1,12)]:
 for mode in range(64):
  f=1+mode%7;id=f*64+7;nv=0 if mode%8==0 else 8;nb=1+mode%4;data=bytearray(valid_fixture(version,nv,nb));_,off,spans=decode(data);fs=0x20bc if variant==0 else 0x20cc;bs=304 if variant==0 else 308;cursor=mode*13%64;fp=off['factions']+f*fs;co=0x207c if variant==0 else 0x208c;active=0x854 if variant==0 else 0x858;struct.pack_into('<i',data,fp+co,cursor);humans=1|(1<<f if mode%8==6 else 0);data[off['preferences']+0x58 if variant else off['legacy']+0x58]=humans
  for i in range(f*64,(f+1)*64):struct.pack_into('<H',data,off['prototypes']+52*i+48,0)
  p=off['prototypes']+52*id;flags=rng.randrange(65536);struct.pack_into('<H',data,p+48,flags);data[p+43]=9
  missing=bool(mode&8)
  for i in [f*64+2,f*64+3,f*64+4]:
   q=off['prototypes']+52*i;struct.pack_into('<H',data,q+48,0 if missing else 1);data[q+43]=10 if i%64==4 else 9;data[q+44]=(1<<f) if mode&16 and i%64==2 else 0
  candidates=[i for i in [f*64+2,f*64+3] if not missing];replacement=(f*64+3 if mode&16 else f*64+2) if candidates else -1
  data[fp+active+id]=0 if mode&32 else 1
  if replacement>=0:data[fp+active+replacement]=255
  for i in range(nv):
   q=off['vehicles']+52*i;unit=id if i%2==0 else f*64+2;struct.pack_into('<h',data,q+10,unit);data[q+14]=((f+1)%8 if mode%8==7 else f);data[q+39]=1 if mode%8 in [2,3,4,5] else 0
   ti=off['tiles']+((i%4)*2//2)*44;struct.pack_into('<I',data,ti+8,1 if mode%8!=3 else 0);data[ti+2]=0 if mode%8==4 else 8 if mode%8==5 else 2
  rejected=bool(nv and (missing or mode%8 in [3,4,5,6,7]));table=bytearray(data[off['bunglist']:off['bunglist']+4096]);table[(f*64+cursor)*8:(f*64+cursor+1)*8]=data[p+32:p+40];counts=bytearray(data[fp+active:fp+active+512]);units=[];conversions=0
  for i in reversed(range(nv)):
   unit=struct.unpack_from('<h',data,off['vehicles']+52*i+10)[0]
   if unit==id and not rejected:counts[id]=(counts[id]-1)&255;counts[replacement]=(counts[replacement]+1)&255;conversions+=1
  units=[replacement if i%2==0 and not rejected else struct.unpack_from('<h',data,off['vehicles']+52*i+10)[0] for i in range(nv)];queues=[]
  for i in range(nb):
   items=[id,id,f*64+2,id]+[0]*6;n=3;struct.pack_into('<11i',data,off['bases']+bs*i+76,n,*items);j=0
   while j<=n:
    if items[j]==id:
     if j==0:items[0]=-38 if not variant else -69
     else:
      for k in range(j,n):items[k]=items[k+1]
      n-=1
    j+=1
   queues+=struct.pack('<11i',n,*items)
  expected=struct.pack('<I',2) if rejected else struct.pack('<7I',0,(cursor+1)%64,flags&~1,nv,nb,replacement&0xffffffff,conversions)+table+counts+struct.pack('<'+str(nv)+'i',*units)+bytes(queues)
  path=out/(str(len(cases))+'.SC');path.write_bytes(data);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(variant),str(f),str(id)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(variant,mode,r.returncode,r.stderr);assert r.stdout==expected,(variant,mode,len(r.stdout),len(expected),next((i for i,(x,y) in enumerate(zip(r.stdout,expected)) if x!=y),None));combined+=r.stdout;cases.append(dict(variant=variant,faction=f,id=id,mode=mode,rejected=rejected,replacement=replacement,vehicles=nv,bases=nb,cursor=cursor,humanMask=humans,sha256=hashlib.sha256(data).hexdigest()))
(out/'outputs.bin').write_bytes(combined);rec=dict(cases=cases,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n');print(len(cases),rec['outputSHA256'])
