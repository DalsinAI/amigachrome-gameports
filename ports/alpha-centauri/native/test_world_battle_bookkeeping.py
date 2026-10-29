# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,struct,subprocess,json,hashlib,os,ast
from test_original_world import valid_fixture
from test_original_game import decode
P=argparse.ArgumentParser();P.add_argument('--out',type=Path,required=True);P.add_argument('--cc',default='g++');P.add_argument('--m68k-linux',action='store_true');P.add_argument('--original',action='append',type=Path,default=[]);a=P.parse_args();D=a.out.resolve();D.mkdir(parents=True,exist_ok=False);S=Path(__file__).resolve().parent
sources=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','battle_bookkeeping.cpp','world_battle_bookkeeping.cpp','world_battle_bookkeeping_probe.cpp'];flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all']);subprocess.run([a.cc,*flags,*[str(S/x)for x in sources],'-o',str(D/'probe')],check=True)
# Reuse the independent numeric model, not the C++ implementation.
tree=ast.parse((S/'test_battle_bookkeeping.py').read_text());model=ast.Module(body=[x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name in ['signed','expected']],type_ignores=[]);env={'U':0xffffffff};exec(compile(model,'independent_model','exec'),env);numeric=env['expected'];U=0xffffffff;seeds=[0,1,19,0x7ffffffe,0x7fffffff,0x80000000,0xfffffffe,U]
worlds=[valid_fixture(v)for v in range(7,13)]+[valid_fixture(7,0,0),valid_fixture(12,2048,512)]+[p.read_bytes()for p in a.original]
rows=[(at,(at+1)%8,kind,var)for at in range(8)for kind in range(2)for var in range(8)];inputs=bytearray(struct.pack('<I',len(worlds)));want=bytearray();guards=awards=0
pack=lambda r:struct.pack('<%dI'%len(r),*r)
fields=[0x150,0x158,0x15c,0x160,0x164,0x498]
for data in worlds:
 info,off,sp=decode(data);ed=int(info['version'])>=11;turn=struct.unpack_from('<I',sp['preferences']if ed else sp['legacy'],0x44)[0]
 inputs+=struct.pack('<II',len(data),len(rows))+data+b''.join(struct.pack('<4I',*r)for r in rows)
 for at,de,kind,var in rows:
  wins=[seeds[(var+i)%8]for i in range(8)];losses=[seeds[(var+i+3)%8]for i in range(8)];total=[(seeds[var]+i*71+j*13)&U for i in range(8)for j in range(8)];count=[(seeds[(var+1)%8]+i*17+j*31)&U for i in range(8)for j in range(8)];records=[];mon=bytearray(sp['monuments']);mon[7]^=0xa5
  for i in range(8):
   rec=[0xa1a1a1a1,0xb2b2b2b2,0xc3c3c3c3,((var*37)>>i)&1,1 if(var+i)%4 else 0,0xd4d4d4d4];records+=rec
   for f,v in zip(fields,rec):struct.pack_into('<I',mon,i*0x4f4+f,v)
  flags=[struct.unpack_from('<I',sp['faction_definitions'],i*0x59c+0x508)[0]for i in range(8)]
  ctx=[int(ed),at,de,kind,U,0,0,1,turn,seeds[(var+2)%8]]+flags;state=wins+losses+total+count+records;r=numeric(ctx+state);assert r[0]==0
  after=r[193:385];g=11+bool(r[-1]);guards+=g;awards+=bool(r[-1]);want+=pack([g])+pack(r[1:])
  for i in range(8):
   for f,v in zip(fields,after[144+i*6:150+i*6]):struct.pack_into('<I',mon,i*0x4f4+f,v)
  want+=mon
  ctx[3]=1-kind;second=numeric(ctx+after);assert second[0]==0;want+=pack(second[1:]);final=second[193:385]
  for i in range(8):
   for f,v in zip(fields,final[144+i*6:150+i*6]):struct.pack_into('<I',mon,i*0x4f4+f,v)
  want+=mon
(D/'inputs.bin').write_bytes(inputs);(D/'expected.bin').write_bytes(want)
cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(D/'probe'),str(D/'inputs.bin')];r=subprocess.run(cmd,capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(D/'actual.bin').write_bytes(r.stdout);(D/'stderr').write_bytes(r.stderr);assert not r.returncode,(r.returncode,len(r.stdout),r.stderr[:1800]);assert r.stdout==want,(len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));receipt={'worlds':len(worlds),'cases':len(worlds)*len(rows),'continuations':len(worlds)*len(rows),'noWriteGuards':guards,'activePlayerMonumentHolds':awards,'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest(),'native':'UNRUN'};(D/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
