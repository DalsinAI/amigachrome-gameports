#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,shlex,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
names=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype_prefix_context.cpp','prototype_prefix_context_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True);cases=[];combined=bytearray();vectors=bytearray()
# Synthetic raw context uses proper Ability27/28 field positions, not a UNIT[-1].
runtime=bytearray([0xa5])*52;struct.pack_into('<I',runtime,0,0x009d021e);struct.pack_into('<h',runtime,16,0);struct.pack_into('<I',runtime,28,0x009d0222);struct.pack_into('<h',runtime,44,0)
for v,version in [(0,7),(1,12)]:
 for mode in range(16):
  data=bytearray(valid_fixture(version,3,2));_,off,spans=decode(data);name=b'origin0\0'+bytes(24);data[off['prototypes']:off['prototypes']+32]=name
  # Opaque synthetic history uses nonzero component bytes and29-bit flags.
  # High flags are not asserted valid base abilities; this exercises preserved
  # modded/corrupt raw bytes. Offset0xfcc starts tuple57 components.
  row=struct.pack('<I4B',0x1fffffff,1,1,1,1)
  for i in range(57,64):at=off['bunglist']+7*512+i*8;data[at:at+8]=row
  supplied=bytes(runtime);prefix=(data[off['bunglist']+4044:off['bunglist']+4096] if not v else supplied)+name;value=bytes(prefix).split(b'\0')[0];term=value+b'\0';observation=1;variant=v;error=0
  if mode==1:term=bytes([min(255,value[0]+1)])+value[1:]+b'\0'
  if mode==2:term=b'\0'
  if mode==3:term=b'\0'
  if mode==4:term=b'W3 Battery\0'
  if mode==5:term=value[:len(value)//2]+b'\0'
  if mode==6:term=value[:min(20,len(value))]+b'\0'
  if mode==7:term=b'origin0\0'
  if mode==8:term=b'X'*256+b'\0';error=1
  if mode==9:term=b'';error=1
  if mode==10:term=b'A\0B\0';error=1
  if mode==11:observation=0;error=2 if v else 0
  if mode==12:data[off['prototypes']:off['prototypes']+32]=b'X'*32;error=1
  if mode==13:variant=99;error=1
  if mode==14:observation=99;error=2 if v else 0
  if mode==15:term=value+b'G\0'
  sign=0;count=0
  if not error:
   for x,y in zip(term,prefix):
    count+=1
    if x!=y:sign=-1 if x<y else 1;break
    if not x:break
  expected=struct.pack('<I',error)+(struct.pack('<iII',sign,count,int(not v)) if not error else b'');n=len(cases);scene=out/(str(n)+'.SC');scene.write_bytes(data);facts=out/(str(n)+'.facts');facts.write_bytes(supplied+struct.pack('<II',observation,len(term))+term);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(scene),str(facts),str(variant)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(v,mode,r.returncode,r.stderr);assert r.stdout==expected,(v,mode,r.stdout.hex(),expected.hex());combined+=r.stdout;cases.append(dict(variant=v,mode=mode,error=error,sign=sign,count=count))
  if not error:
   # Private original-fragment reference vectors: supplied bytes are data, not
   # claims of a live owner's observation or a playable automatic name.
   vectors+=struct.pack('<II',v,len(term))+bytes(prefix)+term+bytes(256-len(term))+struct.pack('<i',sign)
(out/'vectors.bin').write_bytes(vectors);(out/'outputs.bin').write_bytes(combined);r=dict(cases=cases,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),vectorsSHA256=hashlib.sha256(vectors).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(len(cases),r['outputSHA256'])
