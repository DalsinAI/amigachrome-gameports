#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,shlex,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_units import fixture,material
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
names=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','existing_prototype_reuse.cpp','existing_prototype_reuse_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True);cases=[];combined=bytearray()
for v,variant,version in [(0,'base',7),(1,'expansion',12)]:
 rows=fixture(variant,count=0)
 for i,row in enumerate(rows['WEAPONS']):row[2]=str(i%5+1)
 for i,row in enumerate(rows['DEFENSES']):row[2]=str(i%4+1)
 rules=material(rows);rp=out/(variant+'.txt');rp.write_bytes(rules)
 for mode in range(64):
  k=mode%16;f=1+mode%7;bit=1<<f;c,w,ar,rec,plan,flags=1,3,0,1,-1,0x40;runtime=1;exp=2;target=6 if k in [2,3,6,15] else f*64
  data=bytearray(valid_fixture(version,3,2));_,off,spans=decode(data);prefs=off['legacy'] if not v else off['preferences'];data[prefs+0x58]=bit;data[prefs+(0x164 if not v else 0x1e0)]=0;fs=0x20bc if not v else 0x20cc;active=0x854 if not v else 0x858;fp=off['factions']+fs*f;data[fp+active:fp+active+1024]=bytes(1024)
  for i in range(512):struct.pack_into('<H',data,off['prototypes']+52*i+48,0)
  p=off['prototypes']+52*target;struct.pack_into('<I',data,p+32,0);data[p+36:p+40]=bytes([c,w,ar,rec]);struct.pack_into('<Hh',data,p+48,1,-1);data[p+44]=0xa5
  if k==0:struct.pack_into('<I4B',data,off['bunglist']+f*512+(mode%64)*8,flags,c,w,ar,rec)
  if k==1:c=w=ar=0
  if k==2:flags=0
  if k==4:data[p+44]=bit;data[fp+active+target]=1
  if k==5:
   data[p+44]=bit;q=p+52;data[q:q+52]=data[p:p+52];data[q+44]=0
  if k==6:struct.pack_into('<h',data,p+50,0);q=off['prototypes']+52*(f*64+7);data[q:q+52]=data[p:p+52];data[q+44]=0
  if k==7:struct.pack_into('<H',data,p+48,0)
  if k==8:runtime=0
  if k==9:data[prefs+0x58]=0
  if k==10:
   # Negative conventional request is outside this callback-free checked domain.
   rr=fixture(variant,count=0);rr['WEAPONS'][3][2]='-1';rp2=out/(variant+'-psi.txt');rp2.write_bytes(material(rr));rpuse=rp2
  else:rpuse=rp
  if k==11:plan=-2;data[p+44]=bit;data[fp+active+target]=1
  if k==12:struct.pack_into('<I',data,p+32,0x40);flags=0
  if k==13:struct.pack_into('<I',data,p+32,0x20)
  if k==14 and v:exp=0
  if k==15:struct.pack_into('<h',data,p+50,88)
  # Independent read-only wire oracle for the supported original early returns.
  error=0;kind=0;selected=-1;record=bytes(52)
  if runtime!=1 or not data[prefs+0x58]&bit or (v and exp not in [1,2]):error=2
  elif k==0:kind=0
  elif k==1:kind=1;selected=2
  elif k==10:error=3
  else:
   error=4
   for i in list(range(64))+list(range(f*64,(f+1)*64)):
    q=off['prototypes']+52*i;fl=struct.unpack_from('<H',data,q+48)[0];obsolete=data[q+44]
    if not fl&1 or (i>=64 and obsolete&bit and not data[fp+active+i] and not data[fp+active+512+i]):continue
    pre=struct.unpack_from('<h',data,q+50)[0]
    if i<64 and pre!=-1:continue
    same=data[q+36:q+40]==bytes([c,w,ar,rec]);ab=struct.unpack_from('<I',data,q+32)[0]
    if same and ab==flags:error=0;kind=2;break
    if same and not ((ab^flags)&0x44ae1b) and ab&flags==ab:
     error=0;kind=3;selected=i;wire=bytearray(data[q:q+52]);wire[44]&=~bit
     if plan==-2:struct.pack_into('<H',wire,48,fl|4)
     record=bytes(wire);break
  expected=struct.pack('<I',error)+(struct.pack('<Ii',kind,selected)+record if not error else b'')
  n=len(cases);path=out/(str(n)+'.SC');path.write_bytes(data);qp=out/(str(n)+'.request');qp.write_bytes(struct.pack('<6i3I',f,c,w,ar,rec,plan,flags,runtime,exp));result=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(rpuse),str(qp),str(v)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60)
  assert result.returncode==0,(v,mode,result.returncode,result.stderr);assert result.stdout==expected,(v,mode,result.stdout.hex(),expected.hex());combined+=result.stdout;cases.append(dict(variant=v,mode=mode,faction=f,error=error,kind=kind,selected=selected,rules=rpuse.name))
# Structural guards run before any supported return/write on both profiles.
for v,variant in [(0,'base'),(1,'expansion')]:
 baseline=(out/('0.SC' if not v else '64.SC')).read_bytes()
 for guard in range(9):
  q=[1,1,3,0,1,-1,0x40,1,2]
  at,value=[(0,0),(0,8),(1,9),(2,26),(3,14),(4,0),(4,5),(6,0x80000000),(5,128)][guard];q[at]=value
  n=len(cases);path=out/(str(n)+'.SC');path.write_bytes(baseline);qp=out/(str(n)+'.request');qp.write_bytes(struct.pack('<6i3I',*q));r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(out/(variant+'.txt')),str(qp),str(v)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60)
  assert r.returncode==0,(v,guard,r.returncode,r.stderr);assert r.stdout==struct.pack('<I',1),(v,guard,r.stdout.hex());combined+=r.stdout;cases.append(dict(variant=v,guard=guard,error=1))
(out/'outputs.bin').write_bytes(combined);r=dict(cases=cases,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(len(cases),r['outputSHA256'])
