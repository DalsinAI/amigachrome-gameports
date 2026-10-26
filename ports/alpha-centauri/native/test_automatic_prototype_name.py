#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,shlex,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_units import fixture,material
LABELS=['Scout','Recon','Foil','Artillery','Battery','Marine','Marines']
def oracle(t,id,p,human,free):
 c,w,a,re=p[36:40];ab=struct.unpack_from('<I',p,32)[0];row=t['CHASSIS'][c];speed,triad,missile=int(row[8]),int(row[9]),int(row[11]);off,mode=int(t['WEAPONS'][w][2]),int(t['WEAPONS'][w][3]);defense=int(t['DEFENSES'][a][2]);n=[row[j] for j in [0,2,4,6,15,17]];ws=t['WEAPONS'][w][1];ars=t['DEFENSES'][a][1];rs=t['REACTORS'][re-1][1]
 prefix=''
 for i,ability in enumerate(t['ABILITIES']):
  if not(ab>>i&1) or not ability[3] or (i==5 and triad==2) or i==15 or(i==3 and speed==1 and off>=defense and not ab&0x8000):continue
  prefix=ability[3]+' ';break
 x=y=prefix;large=mode<3 and min(off,defense)>(6 if off<defense else 3)+(1 if triad==1 else (6 if off<defense else 3)//2 if triad==2 else 0) and bool(n[4] and n[5]);parity=(re-1^(id//64))&1;special=False
 if (p[43]==10 and not human and speed>1) or (off<=speed and defense<=speed and off>=0 and triad==0):
  if mode<3:
   if ab&0x400000 or free:x=x[:-1];y=y[:max(0,len(x)-1)]
   else:x+=LABELS[0 if off<2 else 1];y+=LABELS[0 if off<2 else 1]
   special=True;parity=int(speed>1)
 defends=off<defense and mode<3 and off>=0 and not ab&0x8000
 if defends or (triad==2 and ab&0x20 and mode<3 and off>=0):
  if not special:parity^=(defense if defends else off)&1;x+=ars if defends else ws;y+=ars if defends else ws
  x+=' '+n[2];y+=' '+(n[5] if large and (triad==1 if not special or not id&1 else triad!=1) else n[3] if not special or not id&1 else n[2])
 elif missile and off>98:x+=rs+' '+ws;y+=rs+' '+ws
 elif missile and w in [24,25]:x+=ws+' '+rs+' '+n[0];y+=ws+' '+rs+' '+n[0]
 else:
  if mode>2 and c!=0:
   if re>1:x+=rs+' ';y+=rs+' '
   if not(mode==7 and c==3) and not(mode==9 and triad==0 and ' ' in x):
    x+=(LABELS[2] if mode in[8,9] and c==3 else n[0])+' ';y+=(LABELS[2] if mode in[8,9] and c==3 else n[1])+' '
  if not special:parity^=off&1;x+=ws;y+=ws
  if mode<3:
   if ab&0x8000:u,v=LABELS[5:7]
   elif ab&8 and speed==1 and off>defense:u,v=LABELS[3:5]
   else:u=n[4] if large and triad!=1 else n[0];v=n[4] if large and triad==1 else n[1]
   x+=' '+u;y+=' '+v
 return (y if parity else x),x,y
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
names=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','automatic_prototype_name.cpp','automatic_prototype_name_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True);cases=[];combined=bytearray()
for v,variant,version in [(0,'base',7),(1,'expansion',12)]:
 t=fixture(variant,count=0)
 for i,row in enumerate(t['WEAPONS']):row[2]=str([-1,1,2,8,12,99][i%6]);row[3]=str(0 if i<7 or i>=20 else [7,8,9,11][i%4])
 for i,row in enumerate(t['DEFENSES']):row[2]=str([-1,1,2,8,12][i%5])
 for i,row in enumerate(t['ABILITIES']):row[3]=f'B{i}'
 rp=out/(variant+'.txt');rp.write_bytes(material(t));wc=len(t['WEAPONS']);ac=len(t['DEFENSES'])
 for mode in range(64):
  f=1+mode%7;id=f*64+(mode%2);c=mode%9;w=mode%wc;ar=(mode//3)%ac;rec=mode%4+1;flags=[0,1,8,0x20,0x8000,0x400000,0x8008,0x400001][mode%8]
  examples=[(0,3,1,1,8),(2,3,1,1,0x20),(0,3,4,1,0),(0,4,3,2,0),(0,1,1,1,0),(3,1,1,2,0),(3,1,1,1,0x400000),(3,1,1,1,0),(8,5,1,4,0),(8,24 if v else 20,1,3,0),(8,25 if v else 20,1,3,0),(3,7,1,2,1),(3,8,1,2,1),(3,9,1,2,1),(6,9,1,2,1),(0,1,1,1,0x8000)]
  if mode<16:c,w,ar,rec,flags=examples[mode]
  data=bytearray(valid_fixture(version,3,2));_,off,spans=decode(data);prefs=off['legacy'] if not v else off['preferences'];human=mode%3!=0;data[prefs+0x58]=(1<<f) if human else 0;owned=mode%2==0;data[prefs+(0x164 if not v else 0x1e0)]=(1<<f) if owned else 0
  for i in range(512):p=off['prototypes']+52*i;name=('unused'+str(i)).encode();data[p:p+32]=name+bytes(32-len(name));struct.pack_into('<H',data,p+48,0)
  fd=off['faction_definitions']+f*0x59c;struct.pack_into('<i',data,fd+0x50c,1);struct.pack_into('<i',data,fd+0x510,15);struct.pack_into('<i',data,fd+0x530,22)
  candidate=bytearray(52);struct.pack_into('<I',candidate,32,flags);candidate[36:40]=bytes([c,w,ar,rec]);candidate[43]=10 if mode%4==0 else 3;struct.pack_into('<Hh',candidate,48,1,-1)
  name,primary,alternate=oracle(t,id,candidate,human,bool(v and owned));error=2 if len(name)>=32 else 0
  if mode>=48:
   # Original collision resolution accesses adjacent runtime data; refuse it.
   p=off['prototypes'];data[p:p+32]=primary.encode()+bytes(32-len(primary));data[p+36:p+40]=bytes([0,0,0,0]);error=3
  labeldata=b''.join(s.encode()+bytes(128-len(s)) for s in LABELS);n=len(cases);path=out/(str(n)+'.SC');path.write_bytes(data);qp=out/(str(n)+'.facts');qp.write_bytes(candidate+struct.pack('<I',id)+labeldata)
  r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(rp),str(qp),str(v)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);expected=struct.pack('<I',error)+(name.encode()+bytes(32-len(name)) if not error else b'');assert r.returncode==0,(v,mode,r.returncode,r.stderr);assert r.stdout==expected,(v,mode,r.stdout,expected,name,primary,alternate);combined+=r.stdout;cases.append(dict(variant=v,mode=mode,faction=f,id=id,name=name,error=error,primary=primary,alternate=alternate))
# Transactional invalid IDs/components, unbounded labels and final storage overflow.
for v,variant in [(0,'base'),(1,'expansion')]:
 base=0 if not v else 64;scene=(out/(str(base)+'.SC')).read_bytes();facts=(out/(str(base)+'.facts')).read_bytes()
 for guard in range(10):
  q=bytearray(facts);want=1
  if guard in [0,1]:struct.pack_into('<I',q,52,63 if guard==0 else 512)
  if guard==2:q[36]=9
  if guard==3:q[37]=26
  if guard==4:q[38]=14
  if guard==5:q[39]=0
  if guard==6:struct.pack_into('<I',q,32,0x80000000)
  if guard==7:q[56:184]=b'X'*128
  if guard==8:
   for j in [3,4]:q[56+128*j:56+128*(j+1)]=b'Y'*127+b'\0'
   want=2
  if guard==9:struct.pack_into('<H',q,48,0)
  n=len(cases);path=out/(str(n)+'.SC');path.write_bytes(scene);qp=out/(str(n)+'.facts');qp.write_bytes(q);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(out/(variant+'.txt')),str(qp),str(v)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60)
  assert r.returncode==0,(v,guard,r.returncode,r.stderr);assert r.stdout==struct.pack('<I',want),(v,guard,r.stdout);combined+=r.stdout;cases.append(dict(variant=v,guard=guard,error=want))
(out/'outputs.bin').write_bytes(combined);receipt=dict(cases=cases,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(len(cases),receipt['outputSHA256'])
