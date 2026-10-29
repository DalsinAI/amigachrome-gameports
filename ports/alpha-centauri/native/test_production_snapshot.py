#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_local_production_session import fixture,rules,oracle,TARGETS,SOURCES as SESSION_SOURCES
from test_original_game import decode
from test_original_faction import SCHEMA
from test_original_world import valid_fixture
HERE=Path(__file__).resolve().parent
SOURCES=SESSION_SOURCES[:-1]+['production_snapshot.cpp','production_snapshot_probe.cpp']
def patch_order(text,data,profile,enabled,human,target):
 result=oracle(text,data,profile,enabled,human)
 if len(result)==4:return struct.unpack('<i',result)[0],data
 count=72 if profile=='base' else 134;targets=[n for n in TARGETS if n<count]
 row=struct.unpack_from('<13i',result,4+targets.index(target)*52)
 if row[2]:return row[2],data
 b=bytearray(data);info,off,_=decode(b);version=int(info['version']);base=off['bases'];struct.pack_into('<i',b,base+0x50,row[6]);struct.pack_into('<i',b,base+0x40,row[10]);old=row[5]
 if row[4] and old>=0:
  rec=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];offset=next(x['offset'] for x in rec['fields'] if x['name']=='units_queue');stride=0x20cc if version>=11 else 0x20bc;b[off['factions']+human*stride+offset+old]=row[12]
 return 0,bytes(b)
def expected(mode,text,data,profile,enabled,human):
 if mode>=100:
  load=struct.unpack_from('<i',oracle(text,data,profile,enabled,human))[0];head=struct.pack('<i',load)
  if load:return head
  error,data=patch_order(text,data,profile,enabled,human,1);head+=struct.pack('<i',error)
  if mode==101:error,data=patch_order(text,data,profile,enabled,human,2);head+=struct.pack('<i',error)
  _,data=patch_order(text,data,profile,enabled,human,3)
  return head+struct.pack('<i',0)+data
 if 2<=mode<=13:return struct.pack('<i',3)
 if mode==1:
  b=bytearray(data);info,off,_=decode(b);version=int(info['version']);stride=308 if version>=11 else 304
  for i in range(int(info['bases'])):struct.pack_into('<i',b,off['bases']+i*stride+0x40,2147483647 if i&1 else -2147483648);struct.pack_into('<i',b,off['bases']+i*stride+0x50,-1-i%20)
  rec=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];offset=next(x['offset'] for x in rec['fields'] if x['name']=='units_queue');stride=0x20cc if version>=11 else 0x20bc
  for i in range(8):start=off['factions']+i*stride+offset;b[start:start+512]=bytes((i*41+j*17)%256 for j in range(512))
  data=bytes(b)
 return struct.pack('<i',0)+data
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-rules',type=Path);ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);binary=a.out/'production-snapshot-probe';command=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(binary)];subprocess.run(command,check=True);records=[];digest=hashlib.sha256()
 def check(name,mode,text,data,profile,enabled,human):
  i=len(records);rp=a.out/(str(i)+'.txt');sp=a.out/(str(i)+'.SC');rp.write_bytes(text);sp.write_bytes(data);want=expected(mode,text,data,profile,enabled,human);cmd=(['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(binary),str(mode),profile,str(int(enabled)),str(human),str(rp),str(sp)];r=subprocess.run(cmd,capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  if r.returncode or r.stdout!=want:
   print('FAILED',name,mode,'RC',r.returncode,'STDERR',r.stderr.decode(errors='replace'),'BYTES',len(r.stdout),len(want))
   for j,(x,y) in enumerate(zip(r.stdout,want)):
    if x!=y:print('FIRSTBYTE',j,x,y);break
   raise AssertionError(name)
  assert sp.read_bytes()==data;digest.update(r.stdout);(a.out/(str(i)+'.bin')).write_bytes(r.stdout);records.append({'name':name,'mode':mode,'profile':profile,'expansion':enabled,'human':human,'bytes':len(want),'passed':True,'outputSHA256':hashlib.sha256(want).hexdigest()})
 for version in range(7,13):
  profile='base' if version<11 else 'expansion'
  for case in [0,1,2,4,8,12,16,32,36]:
   data,human=fixture(version,case);text=rules(profile,case);enabled=version>=11 and bool(case%2)
   if case==0:
    # Include an actual actor so the vehicle-field seal is exercised.
    info,off,_=decode(data);b=bytearray(data);actor=valid_fixture(version,1,0);_,ao,_=decode(actor);b[off['vehicles']:off['vehicles']]=actor[ao['vehicles']:ao['vehicles']+52];pref=off['preferences'] if version>=11 else off['legacy'];struct.pack_into('<i',b,pref+0x38,1);data=bytes(b)
    for mode in range(14):check(f'v{version}-{case}',mode,text,data,profile,enabled,human)
   for mode in [100,101]:check(f'v{version}-{case}',mode,text,data,profile,enabled,human)
 for p in a.original:
  assert a.original_rules;data=p.read_bytes();info,_,_=decode(data)
  for mode in [0,1,100,101]:check('original-'+p.name,mode,a.original_rules.read_bytes(),data,'base',False,int(info['player']))
  assert p.read_bytes()==data
 receipt={'cases':len(records),'outputSHA256':digest.hexdigest(),'records':records,'command':command,'binarySHA256':hashlib.sha256(binary.read_bytes()).hexdigest(),'nativeExecution':'UNRUN'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
