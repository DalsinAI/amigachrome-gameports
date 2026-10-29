#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,shlex,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode as game_decode
from test_units import fixture,material
from test_prototype import decode,derive
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;names=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','custom_prototype_construction.cpp','custom_prototype_construction_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True);cases=[];combined=bytearray()
for v,variant,version in [(0,'base',7),(1,'expansion',12)]:
 t=fixture(variant,count=0)
 for i,row in enumerate(t['WEAPONS']):row[2]=str(i%5+1);row[3]='7' if i==7 else '11' if i==8 else '8' if i==9 else '0';row[4]=str(i%4+1)
 t['WEAPONS'][20][2]='99'
 for i,row in enumerate(t['DEFENSES']):row[2]=str(i%4+1);row[4]=str(i%4+1)
 for i,row in enumerate(t['CHASSIS']):row[13]=str(i%4+1)
 for i,row in enumerate(t['ABILITIES']):row[1]=str(-i-1 if i<7 else 1);row[4]=format(0x400 if i%2 else 5,'b')
 rules=material(t);rp=out/(variant+'.txt');rp.write_bytes(rules);d=decode(rules,variant);wc=len(d['weapons']);ac=len(d['armor'])
 for mode in range(48):
  f=1 if mode&16 else 1+mode%7;id=f*64+(0 if mode&16 else 7);c=0 if mode&16 else mode%9;w=0 if mode&16 else mode%wc;ar=0 if mode&16 else mode%ac;r=mode%4+1;flags=[0,1,0x20,0x400000,0x8018,0x800000,0x80000,0][mode%8]
  if mode>=32:
   samples=[(8,20,0,1,0),(8,24 if v else 8,0,2,0),(8,25 if v else 9,0,3,0),(1,7,0,4,0x880000),(2,7,0,4,0x880000),(0,0,0,1,0),(0,0,0,1,0x400000),(2,0,0,1,0),(2,0,0,1,0x20),(3,1,2,2,0),(6,4,0,3,0x8018),(4,8,0,2,0),(0,4,3,4,127),(8,20,ac-1,4,0),(1,7,0,1,0x800000),(2,7,0,1,0x80000)]
   c,w,ar,r,flags=samples[mode-32]
  data=bytearray(valid_fixture(version,3,2));_,off,spans=game_decode(data);prefs=off['legacy'] if not v else off['preferences'];data[prefs+(0x164 if not v else 0x1e0)]=1<<f if mode&1 else 0
  for i in range(512):
   p=off['prototypes']+52*i;data[p+36:p+39]=bytes(3);struct.pack_into('<Hh',data,p+48,0,-1)
  if not mode&4:
   for i in range(9):p=off['prototypes']+52*i;data[p+36:p+39]=bytes([i,4,3]);struct.pack_into('<H',data,p+48,5)
  p=off['prototypes']+52*id;data[p+36:p+39]=bytes(3);struct.pack_into('<Hh',data,p+48,5 if mode&16 else 0,-2)
  if mode&8:struct.pack_into('<H',data,off['prototypes']+52*((f+1)*64-1)+48,0x10)
  fd=off['faction_definitions']+0x59c*f;struct.pack_into('<i',data,fd+0x50c,2);struct.pack_into('<2i',data,fd+0x510,15,15);struct.pack_into('<2i',data,fd+0x530,5,22)
  q=dict(name='',chassis=c,weapon=w,armor=ar,reactor=r,plan=-1,prerequisite=-2,icon=-1,cost=0,carry=0,abilities=flags);d['units']=[q];z=derive(d,0,1)
  expected=struct.pack('<I',2) if z is None else None
  if z is not None:
   cover=set()
   for i in list(range(64))+list(range(f*64,(f+1)*64)):
    o=off['prototypes']+52*i;ff=struct.unpack_from('<H',data,o+48)[0]
    if not ff&1 or (i>=64 and not ff&4):continue
    ch,wp,ap=data[o+36:o+39]
    if wp==w or 0<d['weapons'][w]['offense']<=d['weapons'][wp]['offense']:cover.add('w')
    if ap==ar or 0<d['armor'][ar]['defense']<=d['armor'][ap]['defense']:cover.add('a')
    if ch==c or d['chassis'][ch]['triad']==d['chassis'][c]['triad'] and d['chassis'][ch]['speed']>=d['chassis'][c]['speed']:cover.add('c')
   if v and mode&1 and not d['chassis'][c]['missile'] and d['weapons'][w]['mode']<7:
    if d['chassis'][c]['triad']==2:z[8:10]=[4,9]
    elif d['chassis'][c]['triad']==0 and d['weapons'][w]['offense']==d['armor'][ar]['defense']==1:z[8]=2
   wire=bytearray(data[p:p+52]);struct.pack_into('<I',wire,32,flags);wire[36:44]=bytes([c,w,ar,r,z[7],z[6],z[8],z[9]]);wire[44]=0;wire[45]=1<<f;wire[46]=255;struct.pack_into('<H',wire,48,(0x115 if mode&8 else 0x105) if len(cover)==3 else 1);expected=struct.pack('<I',0)+wire
  num=len(cases);path=out/(str(num)+'.SC');path.write_bytes(data);qp=out/(str(num)+'.request');qp.write_bytes(struct.pack('<6I',id,c,w,ar,r,flags));result=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(rp),str(qp),str(v)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert result.returncode==0,(v,mode,result.returncode,result.stderr);assert result.stdout==expected,(v,mode,result.stdout.hex(),expected.hex());combined+=result.stdout;cases.append(dict(variant=v,mode=mode,id=id,chassis=c,weapon=w,armor=ar,reactor=r,abilities=flags,rejected=z is None))
# Transactional structural/storage guards on both real record profiles.
for v,variant,version in [(0,'base',7),(1,'expansion',12)]:
 baseline=(out/('0.SC' if not v else '48.SC')).read_bytes();rules=(out/(variant+'.txt')).read_bytes()
 for guard in range(9):
  id,c,w,ar,r,flags=64,0,0,0,1,0;rp=out/(variant+'.txt');want=1
  if guard==0:id=63
  if guard==1:id=512
  if guard==2:c=9
  if guard==3:w=26
  if guard==4:ar=14
  if guard==5:r=0
  if guard==6:flags=0x80000000
  if guard>=7:
   rows=fixture(variant,count=0)
   # High byte-sized pricing inputs overflow final original byte storage.
   if guard==7:rows['CHASSIS'][0][13]='255';rows['WEAPONS'][0][4]='255';rows['DEFENSES'][0][4]='255'
   else:c=1;w=7;r=4;rows['CHASSIS'][1][12]='255'
   # Keep permitted input components/ability factors identical to normal rules.
   rows['WEAPONS'][0][2]='1';rows['DEFENSES'][0][2]='1'
   rp=out/(variant+'-guard'+str(guard)+'.txt');rp.write_bytes(material(rows));want=2
  num=len(cases);path=out/(str(num)+'.SC');path.write_bytes(baseline);qp=out/(str(num)+'.request');qp.write_bytes(struct.pack('<6I',id,c,w,ar,r,flags))
  result=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(rp),str(qp),str(v)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60)
  assert result.returncode==0,(v,guard,result.returncode,result.stderr);assert result.stdout==struct.pack('<I',want),(v,guard,result.stdout.hex(),want);combined+=result.stdout;cases.append(dict(variant=v,guard=guard,rejected=True,expected=want))
(out/'outputs.bin').write_bytes(combined);receipt=dict(cases=cases,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(len(cases),receipt['outputSHA256'])
