#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,shlex,struct,subprocess,ast
from pathlib import Path
from test_original_game import decode as game_decode
from test_prototype import decode,derive
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--fixtures',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);fixtures=Path(a.fixtures).resolve();src=Path(__file__).resolve().parent
node=next(x for x in ast.parse((src/'test_base_suffix.py').read_text()).body if isinstance(x,ast.FunctionDef) and x.name=='resolve');exec(compile(ast.Module(body=[node],type_ignores=[]),'<independent-suffix>','exec'))
module=ast.parse((src/'test_unreferenced_prototype_allocation.py').read_text());loop=next(x for x in module.body if isinstance(x,ast.For));oracle=next(x for x in loop.body if isinstance(x,ast.If) and isinstance(x.test,ast.UnaryOp) and isinstance(x.test.operand,ast.Name) and x.test.operand.id=='error');oracleCode=compile(ast.Module(body=oracle.body,type_ignores=[]),'<independent-proposal>','exec')
names=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','custom_prototype_construction.cpp','existing_prototype_reuse.cpp','proposer_record_stages.cpp','prototype_prefix_context.cpp','automatic_prototype_name.cpp','human_component_pruning.cpp','inactive_prototype_allocation.cpp','retirement_history.cpp','prototype_retirement_queue.cpp','unused_prototype_retirement.cpp','unreferenced_prototype_allocation.cpp','discovery_reactor_sequence.cpp','discovery_reactor_sequence_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True);combined=bytearray();records=[]
parent=json.loads((fixtures/'receipt.json').read_text())['cases']
def prune(data,off,d,f,selected):
 flags=[struct.unpack_from('<H',data,off['prototypes']+52*i+48)[0] for i in range(512)]
 if selected<64 or flags[selected]&4:
  for i in range(f*64,(f+1)*64):
   if not flags[i]&1 or flags[i]&4:continue
   ca,wp,ar=data[off['prototypes']+52*i+36:off['prototypes']+52*i+39];covered=set()
   for j in list(range(64))+list(range(f*64,(f+1)*64)):
    pp=off['prototypes']+52*j;pre=struct.unpack_from('<h',data,pp+50)[0]
    if not flags[j]&1 or (j>=64 and not flags[j]&4) or (j<64 and pre!=-1):continue
    cb,wb,ab=data[pp+36:pp+39]
    if wp==wb or 0<d['weapons'][wp]['offense']<=d['weapons'][wb]['offense']:covered.add('w')
    if ar==ab or 0<d['armor'][ar]['defense']<=d['armor'][ab]['defense']:covered.add('a')
    if ca==cb or d['chassis'][ca]['triad']==d['chassis'][cb]['triad'] and d['chassis'][ca]['speed']<=d['chassis'][cb]['speed']:covered.add('c')
   if len(covered)==3:flags[i]|=0x104
 for i,fl in enumerate(flags):struct.pack_into('<H',data,off['prototypes']+52*i+48,fl)
for mode in range(102):
 current=61<=mode<70;sourceMode=mode if mode<61 else [0,1,15,17,18,36,49,55,60][mode-61] if current else 0
 data=bytearray((fixtures/f'{sourceMode}.SC').read_bytes());facts=bytearray((fixtures/f'{sourceMode}.facts').read_bytes());raw=(fixtures/f'{sourceMode}.txt').read_bytes();_,off,_=game_decode(data);f=struct.unpack_from('<i',facts)[0];state=off['factions']+f*0x20bc;bit=1<<f;error=parent[sourceMode]['error'];error=0 if error==5 else error
 struct.pack_into('<i',data,state+0x207c,[0,1,57,63][mode%4])
 if 70<=mode<86:
  k=mode-70;raw=(fixtures/'0.txt').read_bytes();raw=raw.replace(b'Reactor2,R2,3,T88',b'Reactor2,R2,3,T0').replace(b'Reactor3,R3,4,T88',b'Reactor3,R3,4,T0')
  data[off['bunglist']:off['bunglist']+4096]=bytes(4096);data[state+0x854:state+0x854+1024]=bytes(1024)
  for i in range(64,128):
   pp=off['prototypes']+52*i;data[pp+44]=bit;struct.pack_into('<H',data,pp+48,1)
   if k>=12:
    data[pp+36:pp+40]=bytes([1,4,3,4]);data[pp+43]=9;data[pp+44]=0;data[state+0x854+i]=1
  sources=[72] if k>=12 else list(range(72,74+k%3))
  for i in sources:
   pp=off['prototypes']+52*i;data[pp:pp+32]=b'source'+bytes(26);data[pp+32:pp+40]=struct.pack('<I4B',8,0,3,1,1);data[pp+43]=9;data[pp+44]=0;struct.pack_into('<Hh',data,pp+48,5 if k%2 else 1,-1);data[state+0x854+i]=1 if k<12 else 0
  for i in range(2):struct.pack_into('<i10i',data,off['bases']+304*i+76,4,64,64,65,72,66,-1,-1,-1,-1,-1)
  struct.pack_into('<i',facts,16,-1 if k%2 else 119)
  if k in [4,5,6,7]:
   # Actual retirement of this tuple makes a LATER target2 request hit history.
   pp=off['prototypes']+52*64;data[pp+32:pp+40]=struct.pack('<I4B',8,0,3,1,2)
  error=0
 if 86<=mode<94 or 96<=mode<98:
  oldState=state;f=7;bit=128;state=off['factions']+f*0x20bc;struct.pack_into('<i',facts,0,f);data[off['legacy']+0x58]=bit;data[off['legacy']+0x164]|=bit
  data[state+0x854:state+0x854+1024]=bytes(1024)
  pp=off['prototypes']+52*73;data[pp:pp+52]=data[off['prototypes']+52*72:off['prototypes']+52*73]
  for i in range(64):
   a0=off['prototypes']+52*(64+i);b0=off['prototypes']+52*(448+i);data[b0:b0+52]=data[a0:a0+52]
   data[b0+44]=(data[b0+44]&~2)|(128 if data[b0+44]&2 else 0)
   data[state+0x854+448+i]=data[oldState+0x854+64+i];data[state+0x854+512+448+i]=data[oldState+0x854+512+64+i]
  for i in range(2):
   if data[off['bases']+304*i+4]==1:data[off['bases']+304*i+4]=7
   for j in range(10):
    at=off['bases']+304*i+80+j*4;item=struct.unpack_from('<i',data,at)[0]
    if 64<=item<128:struct.pack_into('<i',data,at,item+384)
  struct.pack_into('<i',data,state+0x207c,[0,1,57,63][mode%4])
  if mode%2:raw=raw.replace(b'Reactor2,R2,3,T88',b'Reactor2,R2,3,T0').replace(b'Reactor3,R3,4,T88',b'Reactor3,R3,4,T0')
  if mode==93:
   struct.pack_into('<i',data,state+0x207c,57);pp=off['prototypes']+52*448;data[pp+36:pp+40]=b'W3 I';data[off['bunglist']+4048:off['bunglist']+4056]=b'nvaders\0';pp=off['prototypes']+5*52;data[pp:pp+32]=b'W3 Marines'+bytes(22)
 if mode>=94:
  for i in range(f*64,(f+1)*64):
   pp=off['prototypes']+52*i;data[pp+36:pp+40]=bytes([1,4,3,4]);data[pp+43]=9;data[pp+44]=0;struct.pack_into('<Hh',data,pp+48,1,-1);data[state+0x854+i]=data[state+0x854+512+i]=1
  pp=off['prototypes']+52*(f*64+8);data[pp+32:pp+40]=struct.pack('<I4B',8,0,3,1,1)
  data[off['bunglist']:off['bunglist']+4096]=bytes(4096)
 if mode>=98:
  data=bytearray((out/'70.SC').read_bytes());facts=bytearray((out/'70.facts').read_bytes());raw=(out/'70.txt').read_bytes();_,off,_=game_decode(data);f=1;bit=2;state=off['factions']+0x20bc
  if mode==98:struct.pack_into('<h',data,off['vehicles']+10,65);error=8
  if mode==99:struct.pack_into('<i',data,state+0x207c,64);error=1
  if mode==100:struct.pack_into('<i',facts,40,f);error=6
  if mode==101:facts[48:176]=b'X'*128;error=1
 if current:
  # These unrelated CURRENT records differ from imported baseline too. They
  # must copy exactly and survive input destruction; no fake gameplay effect.
  data[off['tiles']]^=0x20;data[off['vehicles']+16]^=7;data[off['monuments']+5]^=0x2a;data[off['faction_definitions']+0x508]^=0x80
 rp=out/f'{mode}.txt';rp.write_bytes(raw);d=decode(raw,'base');original=bytes(data);backendFacts=facts;runtime,skip,owner,scratch=struct.unpack_from('<4i',facts,32);last,difficulty=struct.unpack_from('<2i',facts,16);events=[]
 if not error:
  for target in range(2,5):
   if f'Reactor{target-1},R{target-1},{target},T0'.encode() not in raw:continue
   for source in range(f*64,(f+1)*64):
    p=off['prototypes']+52*source;sf=struct.unpack_from('<H',data,p+48)[0]
    if not sf&1 or data[p+44]&bit or data[p+39]>=target:continue
    if owner==f:error=6;break
    abilities=struct.unpack_from('<I',data,p+32)[0];c,w,ar=data[p+36:p+39];q=[f,c,w,ar,target,-2 if sf&4 else -1,abilities,1,2];kind=None;returned=-1;retired=False
    history=data[off['bunglist']+f*512:off['bunglist']+(f+1)*512]
    if struct.pack('<I4B',abilities,c,w,ar,target) in [history[i:i+8] for i in range(0,512,8)]:kind=0
    elif d['chassis'][c]['speed']==d['weapons'][w]['offense']==d['armor'][ar]['defense']==1:kind=1;returned=2
    else:
     for i in list(range(64))+list(range(f*64,(f+1)*64)):
      pp=off['prototypes']+52*i;fl=struct.unpack_from('<H',data,pp+48)[0];ab=struct.unpack_from('<I',data,pp+32)[0]
      if not fl&1 or (i>=64 and data[pp+44]&bit and not data[state+0x854+i] and not data[state+0x854+512+i]) or (i<64 and struct.unpack_from('<h',data,pp+50)[0]!=-1):continue
      if data[pp+36:pp+40]==bytes([c,w,ar,target]):
       if ab==abilities:kind=2;break
       if not((ab^abilities)&0x44ae1b) and ab&abilities==ab:
        kind=3;returned=i;data[pp+44]&=~bit
        if q[5]==-2:struct.pack_into('<H',data,pp+48,fl|4)
        break
    if kind is None:
     assert (c,w,ar,abilities)==(0,3,1,8),(mode,source,target,q)
     active=state+0x854;best=0;selected=-1
     for i in range(f*64,(f+1)*64):
      pp=off['prototypes']+52*i;flags=struct.unpack_from('<H',data,pp+48)[0];av,qv=data[active+i],data[active+512+i]
      if flags&1 and data[pp+44]&bit and not av and not qv:score=0x2000 if data[pp+43]==q[5] else 0x1000
      elif not flags&1:score=0x800
      elif data[pp+44]&bit and not av:score=0x400
      elif data[pp+43]==q[5] or not av and not qv:score=(0x40 if av else 0x240)|(0x100 if data[pp+44]&bit else 0)|(0 if qv else 0x80);score-=min(63,data[pp+41]*av+qv)
      else:score=0
      if score>best:best=score;selected=i
     if selected<0:kind=5;returned=-1
     else:
      retired=bool(struct.unpack_from('<H',data,off['prototypes']+52*selected+48)[0]&1);cursor=struct.unpack_from('<i',data,state+0x207c)[0]
      if any(struct.unpack_from('<h',data,off['vehicles']+52*i+10)[0]==selected for i in range(3)):error=8;break
      backend=struct.pack('<6i3I',*q)+struct.pack('<2I2i',1,1,last,difficulty)+backendFacts[48:];env=dict(globals());env.update(data=data,off=off,q=q,f=f,selected=selected,cursor=cursor,retired=retired,d=d,facts=backend,expected=struct.pack('<I',0));exec(oracleCode,env);proposal=env['expected'];returned=selected;kind=4
      data[off['bunglist']:off['bunglist']+4096]=proposal[12:4108];struct.pack_into('<i',data,state+0x207c,struct.unpack_from('<i',proposal,8)[0]);data[off['prototypes']+52*selected:off['prototypes']+52*(selected+1)]=proposal[4116:4168]
      for i,fl in enumerate(struct.unpack_from('<512I',proposal,4172)):struct.pack_into('<H',data,off['prototypes']+52*i+48,fl)
      for i in range(2):data[off['bases']+304*i+76:off['bases']+304*i+120]=proposal[6220+44*i:6220+44*(i+1)]
      last=struct.unpack_from('<i',proposal,4112)[0]
    if returned!=source:prune(data,off,d,f,returned);scratch=0
    events.append((source,target,returned,kind,int(retired),struct.unpack_from('<i',data,state+0x207c)[0],last,sf))
   if error:break
 expected=struct.pack('<I',error)
 if not error:
  expected+=struct.pack('<5iI',len(events),-1,scratch,-1,-1,struct.unpack_from('<I',data,state+0x207c)[0])+b''.join(struct.pack('<8i',*e) for e in events)+data[off['prototypes']:off['prototypes']+512*52]+data[off['bunglist']:off['bunglist']+4096]+struct.pack('<I',2)
  for i in range(2):expected+=data[off['bases']+304*i+76:off['bases']+304*i+120]
  for i in range(8):expected+=data[off['factions']+0x20bc*i+0x207c:off['factions']+0x20bc*i+0x2080]+data[off['factions']+0x20bc*i+0x854:off['factions']+0x20bc*i+0x854+1024]
 path=out/f'{mode}.SC';path.write_bytes(original);qp=out/f'{mode}.facts';qp.write_bytes(facts);(out/f'{mode}.expected').write_bytes(expected);(out/f'{mode}.after.SC').write_bytes(data)
 baseline=fixtures/'30.SC' if current else path;run=(['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(baseline),str(rp),str(qp),'0',str(path) if current else '-'];r=subprocess.run(run,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(mode,r.returncode,r.stderr);assert r.stdout==expected,(mode,len(r.stdout),len(expected),next((i for i,(x,y) in enumerate(zip(r.stdout,expected)) if x!=y),None),r.stdout[:28].hex(),expected[:28].hex());combined+=r.stdout;records.append(dict(mode=mode,sourceMode=sourceMode,currentDecodedFixture=current,error=error,events=events))
(out/'outputs.bin').write_bytes(combined);receipt=dict(cases=records,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd,fixtures=str(fixtures));(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(len(records),receipt['outputSHA256'])
