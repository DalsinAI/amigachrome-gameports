#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,shlex,struct,subprocess,ast
from pathlib import Path
from test_original_game import decode as game_decode
from test_prototype import decode
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--fixtures',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);fixtures=Path(a.fixtures).resolve();src=Path(__file__).resolve().parent
node=next(x for x in ast.parse((src/'test_base_suffix.py').read_text()).body if isinstance(x,ast.FunctionDef) and x.name=='resolve');exec(compile(ast.Module(body=[node],type_ignores=[]),'<independent-suffix>','exec'))
# Reuse only the independently Python-modelled construction/name/prune/queue
# oracle body, not a C++ query result or the other script's CLI.
module=ast.parse((src/'test_unreferenced_prototype_allocation.py').read_text());loop=next(x for x in module.body if isinstance(x,ast.For));oracle=next(x for x in loop.body if isinstance(x,ast.If) and isinstance(x.test,ast.UnaryOp) and isinstance(x.test.operand,ast.Name) and x.test.operand.id=='error');oracleCode=compile(ast.Module(body=oracle.body,type_ignores=[]),'<independent-proposal>','exec')
from test_prototype import derive
names=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','custom_prototype_construction.cpp','existing_prototype_reuse.cpp','proposer_record_stages.cpp','prototype_prefix_context.cpp','automatic_prototype_name.cpp','human_component_pruning.cpp','inactive_prototype_allocation.cpp','retirement_history.cpp','prototype_retirement_queue.cpp','unused_prototype_retirement.cpp','unreferenced_prototype_allocation.cpp','discovery_reactor_stage.cpp','discovery_reactor_stage_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True);combined=bytearray();records=[]
for mode in range(65):
 baseMode=mode%16;data=bytearray((fixtures/(str(baseMode)+'.SC')).read_bytes());backendFacts=bytearray((fixtures/(str(baseMode)+'.facts')).read_bytes());_,off,_=game_decode(data);f=1;bit=2;state=off['factions']+0x20bc;source=72;target=2+mode%3;techId=0
 # Post-award context is actual owned data, not an invented grant in this API.
 data[off['legacy']+0x164]|=bit;data[off['legacy']:off['legacy']+4]=struct.pack('<I',struct.unpack_from('<I',data,off['legacy'])[0]&~0x4000)
 for i in range(64,128):
  p=off['prototypes']+52*i
  if struct.unpack_from('<H',data,p+48)[0]&1:data[p+44]|=bit
 # One current eligible source; all request components/flags derive from it.
 p=off['prototypes']+52*source;data[p:p+32]=b'source'+bytes(26);data[p+32:p+40]=struct.pack('<I4B',8,0,3,1,1);data[p+44]=0;struct.pack_into('<Hh',data,p+48,5 if mode%2 else 1,-1);data[state+0x854+source]=1;data[state+0x854+512+source]=0
 if mode==15:
  # Force genuine same-source replacement: other active reactors are current,
  # all have live saved counts/group mismatch, so source is exact best slot.
  for i in range(64,128):
   pp=off['prototypes']+52*i;data[pp+36:pp+40]=bytes([1,4,3,target]);data[pp+43]=9;data[pp+44]=0;struct.pack_into('<H',data,pp+48,1);data[state+0x854+i]=1
  data[p+36:p+40]=bytes([0,3,1,1]);struct.pack_into('<H',data,p+48,5);data[state+0x854+source]=data[state+0x854+512+source]=0
 raw=(fixtures/'base.txt').read_text();raw=raw.replace(f'Reactor{target-1},R{target-1},{target},T88',f'Reactor{target-1},R{target-1},{target},T0');rp=out/(str(mode)+'.txt');rp.write_text(raw);d=decode(raw.encode(),'base')
 runtime=1;skip=1;owner=2;scratch=12345;mp=1;cursorObs=1;proposalMode=1;error=0;has=True
 if 16<=mode<33:
  k=mode-16
  if k==0:owner=1;error=6
  if k==1:pp=p+52;data[pp:pp+52]=data[p:p+52];error=5
  if k==2:raw=raw.replace('Reactor1,R1,2,T88','Reactor1,R1,2,T0').replace('Reactor2,R2,3,T88','Reactor2,R2,3,T0');error=5
  if k==3:data[off['legacy']+0x164]&=~bit;error=4
  if k==4:runtime=0;error=2
  if k==5:skip=0;error=2
  if k==6:skip=2;error=3
  if k==7:struct.pack_into('<i',data,off['legacy']+0x44,0);error=3
  if k==8:mp=0;error=2
  if k==9:struct.pack_into('<I',data,off['legacy'],0x4000);error=2
  if k==10:struct.pack_into('<I',data,off['legacy']+4,0x100000);error=2
  if k==11:struct.pack_into('<H',data,p+48,0);has=False
  if k==12:data[p+44]=bit;has=False
  if k==13:data[p+39]=target;has=False;owner=1
  if k==14:raw=(fixtures/'base.txt').read_text();has=False
  if k==15:proposalMode=0;error=2
  if k==16:pp=p+52;data[pp:pp+52]=data[p:p+52];data[pp+39]=target;error=0
  rp.write_text(raw);d=decode(raw.encode(),'base')
 early=None
 if mode==32 or 33<=mode<49:
  # Keep the one lower-reactor source but route the real proposer to its
  # unchanged history/scout/duplicate or exact allowed reuse return.
  if mode>=33:
   kind=(mode-33)%4;predefined=(mode-33)//4%2;desired=-1
   for i in range(512):
    if i!=source:struct.pack_into('<H',data,off['prototypes']+52*i+48,0)
   data[state+0x854:state+0x854+1024]=bytes(1024)
   if kind==0:
    struct.pack_into('<I4B',data,off['bunglist']+f*512+8*(mode%64),8,0,3,1,target)
    early=(0,-1,bytes(52))
   elif kind==1:
    # Explicit original scout shortcut: component strengths/speed all one.
    from test_units import fixture,material
    rows=fixture('base',count=0);rows['CHASSIS'][0][6]='1';rows['WEAPONS'][3][2]='1';rows['DEFENSES'][1][2]='1'
    raw=material(rows).decode();raw=raw.replace(f'Reactor{target-1},R{target-1},{target},T88',f'Reactor{target-1},R{target-1},{target},T0');rp.write_text(raw);d=decode(raw.encode(),'base');early=(1,2,bytes(52))
   else:
    desired=6 if predefined else 75;pp=off['prototypes']+52*desired;data[pp:pp+52]=data[p:p+52];data[pp+39]=target;struct.pack_into('<h',data,pp+50,-1)
    if kind==2:early=(2,-1,bytes(52))
    else:
     struct.pack_into('<I',data,p+32,0x40);struct.pack_into('<I',data,pp+32,0);data[pp+44]=bit;data[state+0x854+desired]=1
     if (mode-33)//8:struct.pack_into('<H',data,p+48,5)
     record=bytearray(data[pp:pp+52]);record[44]&=~bit
     if struct.unpack_from('<H',data,p+48)[0]&4:struct.pack_into('<H',record,48,struct.unpack_from('<H',record,48)[0]|4)
     early=(3,desired,record)
  else:early=(2,-1,bytes(52))
 multi=[]
 if mode>=49:
  num=2+mode%3;numReactors=1+(mode//3)%3;kind=mode%3
  from test_units import fixture,material
  rows=fixture('base',count=0)
  if kind==1:rows['CHASSIS'][0][6]='1';rows['WEAPONS'][3][2]='1';rows['DEFENSES'][1][2]='1'
  raw=material(rows).decode()
  for reactor in range(2,2+numReactors):raw=raw.replace(f'Reactor{reactor-1},R{reactor-1},{reactor},T88',f'Reactor{reactor-1},R{reactor-1},{reactor},T0')
  rp.write_text(raw);d=decode(raw.encode(),'base')
  for i in range(512):struct.pack_into('<H',data,off['prototypes']+52*i+48,0)
  data[state+0x854:state+0x854+1024]=bytes(1024)
  for i in range(source,source+num):
   pp=off['prototypes']+52*i;data[pp:pp+52]=data[p:p+52];data[pp+44]=0;data[pp+39]=1;struct.pack_into('<Hh',data,pp+48,5 if i%2 else 1,-1)
  for reactor in range(2,2+numReactors):
   if kind==0:struct.pack_into('<I4B',data,off['bunglist']+f*512+8*(reactor-2),8,0,3,1,reactor)
   elif kind==2:
    pp=off['prototypes']+52*(reactor+4);data[pp:pp+52]=data[p:p+52];data[pp+39]=reactor;struct.pack_into('<Hh',data,pp+48,1,-1)
   for i in range(source,source+num):multi.append((i,reactor,0 if kind==0 else 1 if kind==1 else 2,2 if kind==1 else -1))
  source,target,kind,returned=multi[-1];early=(kind,returned,bytes(52))
 if mode>=61:
  # First call genuinely returns without changing a record; a later changing
  # result must reject the WHOLE stage rather than publish partial flags.
  source=72;target=2;multi=[];early=None;error=5
  from test_units import fixture,material
  raw=material(fixture('base',count=0)).decode().replace('Reactor1,R1,2,T88','Reactor1,R1,2,T0');rp.write_text(raw);d=decode(raw.encode(),'base')
  for i in range(512):struct.pack_into('<H',data,off['prototypes']+52*i+48,0)
  for i in [72,73]:
   pp=off['prototypes']+52*i;data[pp:pp+52]=data[p:p+52];data[pp+36:pp+40]=bytes([0,3,1,1]);data[pp+44]=0;struct.pack_into('<Hh',data,pp+48,1,-1)
  struct.pack_into('<I',data,p+32,8);struct.pack_into('<I',data,p+52+32,0x40);struct.pack_into('<I4B',data,off['bunglist']+f*512,8,0,3,1,2)
  if mode in [62,63]:
   pp=off['prototypes']+52*75;data[pp:pp+52]=data[p:p+52];struct.pack_into('<I',data,pp+32,0);data[pp+39]=2;data[pp+44]=0
  if mode==63:owner=f;error=6
  if mode==64:
   rows=fixture('base',count=0);rows['WEAPONS'][3][2]='-1';raw=material(rows).decode().replace('Reactor1,R1,2,T88','Reactor1,R1,2,T0');rp.write_text(raw);d=decode(raw.encode(),'base')
 last,difficulty=struct.unpack_from('<2i',backendFacts,44);facts=struct.pack('<2i2I2i4I2i',f,techId,mp,cursorObs,last,difficulty,proposalMode,2,runtime,skip,owner,scratch)+backendFacts[52:];assert len(facts)==1072
 expected=struct.pack('<I',error)
 if not error:
  if has and early is not None:
   kind,returned,record=early;prune=[struct.unpack_from('<H',data,off['prototypes']+52*i+48)[0] for i in range(512)];protos=[data[off['prototypes']+52*i:off['prototypes']+52*(i+1)] for i in range(512)]
   if kind==3:protos[returned]=record;prune[returned]=struct.unpack_from('<H',record,48)[0]
   for event in range(len(multi) or 1):
    if returned<64 or prune[returned]&4:
     for i in range(64,128):
      if not prune[i]&1 or prune[i]&4:continue
      ca,wp,ar=protos[i][36:39];covered=set()
      for j in list(range(64))+list(range(64,128)):
       if not prune[j]&1 or j>=64 and not prune[j]&4:continue
       pre=struct.unpack_from('<h',protos[j],50)[0]
       if j<64 and pre!=-1:continue
       cb,wb,ab=protos[j][36:39]
       if wp==wb or 0<d['weapons'][wp]['offense']<=d['weapons'][wb]['offense']:covered.add('w')
       if ar==ab or 0<d['armor'][ar]['defense']<=d['armor'][ab]['defense']:covered.add('a')
       if ca==cb or d['chassis'][ca]['triad']==d['chassis'][cb]['triad'] and d['chassis'][ca]['speed']<=d['chassis'][cb]['speed']:covered.add('c')
      if len(covered)==3:prune[i]|=0x104
   selected=returned;wire=bytearray(record)
   if kind==3:struct.pack_into('<H',wire,48,prune[returned])
   expected+=struct.pack('<7i',3 if multi else 2,source,target,-1,0,-1,-1)+struct.pack('<512I',*prune)+struct.pack('<Ii',kind,returned)+wire+(struct.pack('<I',len(multi)) if multi else b'')
  elif has:
   q=[f,0,3,1,target,-2 if struct.unpack_from('<H',data,p+48)[0]&4 else -1,8,1,2]
   active=state+0x854;best=0;selected=-1
   for i in range(64,128):
    pp=off['prototypes']+52*i;flags=struct.unpack_from('<H',data,pp+48)[0];av,qv=data[active+i],data[active+512+i]
    if flags&1 and data[pp+44]&bit and not av and not qv:score=0x2000 if data[pp+43]==q[5] else 0x1000
    elif not flags&1:score=0x800
    elif data[pp+44]&bit and not av:score=0x400
    elif data[pp+43]==q[5] or not av and not qv:score=(0x40 if av else 0x240)|(0x100 if data[pp+44]&bit else 0)|(0 if qv else 0x80);score-=min(63,data[pp+41]*av+qv)
    else:score=0
    if score>best:best=score;selected=i
   assert selected>=0;retired=bool(struct.unpack_from('<H',data,off['prototypes']+52*selected+48)[0]&1);cursor=struct.unpack_from('<i',data,state+0x207c)[0]
   # The parent fixture's random cursor is irrelevant to inactive replacement,
   # but actual active retirement requires the real bounded ring cursor.
   if retired and not 0<=cursor<64:cursor=mode%64;struct.pack_into('<i',data,state+0x207c,cursor)
   backend=struct.pack('<6i3I',*q)+struct.pack('<2I2i',mp,cursorObs,last,difficulty)+backendFacts[52:];env=dict(globals());env.update(data=data,off=off,q=q,f=f,selected=selected,cursor=cursor,retired=retired,d=d,facts=backend,expected=struct.pack('<I',0));exec(oracleCode,env);proposal=env['expected'];prune=list(env['prune']);protos=env['protos'];wire=bytearray(proposal[4116:4168])
   if selected!=source:
    for i in range(64,128):
     if not prune[i]&1 or prune[i]&4:continue
     ca,wp,ar=protos[i][36:39];covered=set()
     for j in list(range(64))+list(range(64,128)):
      if not prune[j]&1 or j>=64 and not prune[j]&4:continue
      cb,wb,ab=protos[j][36:39]
      if wp==wb or 0<d['weapons'][wp]['offense']<=d['weapons'][wb]['offense']:covered.add('w')
      if ar==ab or 0<d['armor'][ar]['defense']<=d['armor'][ab]['defense']:covered.add('a')
      if ca==cb or d['chassis'][ca]['triad']==d['chassis'][cb]['triad'] and d['chassis'][ca]['speed']<=d['chassis'][cb]['speed']:covered.add('c')
     if len(covered)==3:prune[i]|=0x104
   child=bytearray(proposal[4:4172]);struct.pack_into('<H',child,4160,prune[selected]);expected+=struct.pack('<7i',1,source,target,-1,0 if selected!=source else scratch,-1,-1)+struct.pack('<512I',*prune)+child+proposal[6220:6308]
  else:
   flags=[struct.unpack_from('<H',data,off['prototypes']+52*i+48)[0] for i in range(512)];expected+=struct.pack('<7i',0,-1,0,-1,scratch,-1,-1)+struct.pack('<512I',*flags)
 path=out/(str(mode)+'.SC');path.write_bytes(data);qp=out/(str(mode)+'.facts');qp.write_bytes(facts);(out/(str(mode)+'.expected')).write_bytes(expected);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(path),str(rp),str(qp),'0'],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(mode,r.returncode,r.stderr);assert r.stdout==expected,(mode,r.stdout[:100].hex(),expected[:100].hex(),len(r.stdout),len(expected));combined+=r.stdout;records.append(dict(mode=mode,error=error,hasProposal=has,source=source,target=target,selected=selected if not error and has else None,sequence=multi))
(out/'outputs.bin').write_bytes(combined);receipt=dict(cases=records,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd,fixtures=str(fixtures));(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(len(records),receipt['outputSHA256'])
