#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,shlex,struct,subprocess,ast
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode as game_decode
from test_units import fixture,material
from test_prototype import decode,derive
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
# Reuse the independent string/cycle oracle without running its CLI driver.
node=next(x for x in ast.parse((src/'test_base_suffix.py').read_text()).body if isinstance(x,ast.FunctionDef) and x.name=='resolve');exec(compile(ast.Module(body=[node],type_ignores=[]),'<independent-suffix>','exec'))
names=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','custom_prototype_construction.cpp','existing_prototype_reuse.cpp','proposer_record_stages.cpp','prototype_prefix_context.cpp','automatic_prototype_name.cpp','human_component_pruning.cpp','inactive_prototype_allocation.cpp','inactive_prototype_allocation_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True)
t=fixture('base',count=0);t['WEAPONS'][3][2]='8';t['WEAPONS'][3][3]='0';t['WEAPONS'][4][2]='10';t['WEAPONS'][4][3]='0';t['DEFENSES'][1][2]='1';t['DEFENSES'][3][2]='4';rules=material(t);rp=out/'base.txt';rp.write_bytes(rules);d=decode(rules,'base');labels=['Scout','Recon','Sea','Marines','Invaders','Artillery','Battery'];suffix='Mk';records=[];combined=bytearray()
for mode in range(48):
 data=bytearray(valid_fixture(7,3,2));_,off,spans=game_decode(data);prefs=off['legacy'];f=1;bit=2;selected=64+mode%4;group=[-2,-1,1,10][mode%4];difficulty=1 if mode&4 else 2;last=-1 if mode&8 else 47;rec=1+int(bool(mode&16));runtime=1;observed=1;error=0
 data[prefs+0x58]=bit;struct.pack_into('<I',data,prefs+4,0);state=off['factions']+0x20bc;data[state+0x854:state+0x854+1024]=bytes(1024)
 for i in range(512):
  p=off['prototypes']+52*i;data[p:p+52]=bytes(52);word=('unused'+str(i)).encode();data[p:p+32]=word+bytes(32-len(word));data[p+36:p+40]=bytes([0,0,0,1]);struct.pack_into('<h',data,p+50,-1)
 # Real known predefined construction coverage; ordered prune will use it.
 p=off['prototypes'];data[p+36:p+40]=bytes([0,4,3,1]);struct.pack_into('<H',data,p+48,0 if mode&8 else 5)
 for i in range(64,selected):
  p=off['prototypes']+52*i;data[p+36:p+40]=bytes([1,4,3,1]);struct.pack_into('<H',data,p+48,1);data[p+43]=9;data[state+0x854+i]=1
 # Same-faction unprototyped design to qualify virtual selected/pruning ordering.
 p=off['prototypes']+70*52;data[p+36:p+40]=bytes([0,1,0,1]);struct.pack_into('<H',data,p+48,1)
 p=off['prototypes']+71*52;data[p+36:p+40]=bytes([1,4,3,1]);struct.pack_into('<H',data,p+48,1);data[p+43]=10 if mode&2 else 1;data[p+44]=bit;data[state+0x854+71]=1;data[state+0x854+512+71]=1
 if mode&8:struct.pack_into('<H',data,off['prototypes']+127*52+48,0x10)
 collision={};primary='W3 Marines';alternate='W3 Invaders'
 for j,name in enumerate(([primary] if mode%4==1 else [alternate] if mode%4==2 else [primary,alternate] if mode%4==3 else [])):
  p=off['prototypes']+(5+j)*52;b=name.encode();data[p:p+32]=b+bytes(32-len(b));collision[5+j]=name
 for i in range(2):
  p=off['bases']+i*304;data[p+4]=1+i;struct.pack_into('<i10i',data,p+76,2,71,-38,71,71,-1,-1,-1,-1,-1,-1)
 if mode>=32:
  k=mode-32
  if k==0:runtime=0;error=2
  if k==1:observed=0;error=2
  if k==2:struct.pack_into('<I',data,prefs+4,0x100000);error=2
  if k==3:data[prefs+0x58]=0;error=2
  if k==4:p=off['prototypes']+72*52;struct.pack_into('<H',data,p+48,1);data[p+44]=bit;data[p+43]=group&255;error=6
  if k==5:struct.pack_into('<h',data,off['vehicles']+10,selected);error=7
  if k==6:struct.pack_into('<I4B',data,off['bunglist']+512,8,0,3,1,rec);error=3
  if k==7:
   p=off['prototypes']+72*52;data[p+36:p+40]=bytes([0,3,1,rec]);struct.pack_into('<I',data,p+32,8);struct.pack_into('<H',data,p+48,1);error=3
  if k==8:labels2=labels[:];labels2[3]=labels2[4]='X'*127
  else:labels2=labels[:]
  if k==9:runtime=2;error=2
  if k==10:observed=99;error=2
  if k==11:group=128;error=1
  if k==12:f=0;error=1
  if k==13:group=-3;error=1
  if k==14:
   for i in range(64,128):p=off['prototypes']+52*i;struct.pack_into('<H',data,p+48,1);data[p+43]=9;data[state+0x854+i]=1
   group=1;error=5
  if k==15:struct.pack_into('<i',data,off['bases']+80,512);error=1
 else:labels2=labels[:]
 q=struct.pack('<6i3I',f,0,3,1,rec,group,8,1,2)+struct.pack('<2I2i',runtime,observed,last,difficulty)+b''.join(x.encode()+bytes(128-len(x)) for x in labels2)+suffix.encode()+bytes(128-len(suffix));path=out/(str(mode)+'.SC');path.write_bytes(data);qp=out/(str(mode)+'.facts');qp.write_bytes(q)
 expected=struct.pack('<I',error)
 if not error:
  p=off['prototypes']+selected*52;wire=bytearray(data[p:p+52]);unit=dict(name='',chassis=0,weapon=3,armor=1,reactor=rec,plan=-1,prerequisite=-1,icon=-1,cost=0,carry=0,abilities=8);d['units']=[unit];z=derive(d,0,1);assert z is not None
  wire[32:48]=struct.pack('<I',8)+bytes([0,3,1,rec,z[7],z[6],3 if group==10 else z[8],group if group>=0 else z[9],0,bit,255,0]);flags=(1 if mode&8 else 0x105)|0x40|0x80|(4 if group==-2 or difficulty<2 else 0);struct.pack_into('<H',wire,48,flags)
  primary='W3 '+labels2[3];alternate='W3 '+labels2[4];prefix=(data[off['bunglist']+4044:off['bunglist']+4096]+data[off['prototypes']:off['prototypes']+32]).split(b'\0')[0].decode('latin1');ec,name=resolve(primary,alternate,((rec-1)^1)&1,collision,prefix,suffix,256);assert ec==0;wire[:32]=(name.encode()[:31]+bytes(32))[:32];flags&=~2;struct.pack_into('<H',wire,48,flags)
  protos=[bytearray(data[off['prototypes']+52*i:off['prototypes']+52*(i+1)]) for i in range(512)];protos[selected]=wire;prune=[struct.unpack_from('<H',p,48)[0] for p in protos]
  if flags&4:
   for i in range(64,128):
    if not prune[i]&1 or prune[i]&4:continue
    ca,wp,ar=protos[i][36:39];covered=set()
    for j in list(range(64))+list(range(64,128)):
     if not prune[j]&1 or (j>=64 and not prune[j]&4):continue
     cb,wb,ab=protos[j][36:39]
     if wp==wb or 0<d['weapons'][wp]['offense']<=d['weapons'][wb]['offense']:covered.add('w')
     if ar==ab or 0<d['armor'][ar]['defense']<=d['armor'][ab]['defense']:covered.add('a')
     if ca==cb or d['chassis'][ca]['triad']==d['chassis'][cb]['triad'] and d['chassis'][ca]['speed']<=d['chassis'][cb]['speed']:covered.add('c')
    if len(covered)==3:prune[i]|=0x104
  struct.pack_into('<H',wire,48,prune[selected]);expected+=struct.pack('<ii',selected,selected if last<0 else last)+wire+struct.pack('<I512I',2,*prune)
  for i in range(2):
   vals=list(struct.unpack_from('<i10i',data,off['bases']+304*i+76))
   if not i:
    for j in range(vals[0]+1):
     id=vals[j+1]
     if id>=0 and protos[id][44]&bit and protos[id][43]==wire[43]:vals[j+1]=selected
   expected+=struct.pack('<11i',*vals)
 r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(rp),str(qp),'0'],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(mode,r.returncode,r.stderr);assert r.stdout==expected,(mode,r.stdout.hex(),expected.hex());combined+=r.stdout;records.append(dict(mode=mode,error=error,id=selected,group=group,difficulty=difficulty,last=last,name=name if not error else None))
# Expansion runtime observations do not supply the original live name prefix.
expRules=material(fixture('expansion',count=0));er=out/'expansion.txt';er.write_bytes(expRules)
for expansion,error in [(2,8),(0,2)]:
 data=bytearray(valid_fixture(12,3,2));_,off,_=game_decode(data);data[off['preferences']+0x58]=2;struct.pack_into('<I',data,off['preferences']+4,0)
 for i in range(512):struct.pack_into('<H',data,off['prototypes']+52*i+48,0)
 path=out/('expansion-'+str(expansion)+'.SC');path.write_bytes(data);qp=out/('expansion-'+str(expansion)+'.facts');q=struct.pack('<6i3I',1,0,3,1,1,-1,8,1,expansion)+struct.pack('<2I2i',1,1,-1,2)+b''.join(x.encode()+bytes(128-len(x)) for x in labels)+suffix.encode()+bytes(128-len(suffix));qp.write_bytes(q)
 r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(er),str(qp),'1'],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(expansion,r.returncode,r.stderr);assert r.stdout==struct.pack('<I',error),(expansion,r.stdout.hex());combined+=r.stdout;records.append(dict(expansionObservation=expansion,error=error,edition='expansion'))
(out/'outputs.bin').write_bytes(combined);receipt=dict(cases=records,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(len(records),receipt['outputSHA256'])
