#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_game import decode
from test_world_actions import controls,fixture
from test_world_unit_stats import rules_fixture
from test_units import sections,material
from test_facility_definitions import tables
from test_local_production_session import ROW
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','world_movement_price.cpp','move_budget.cpp','vehicle_stack.cpp','world_actions.cpp','facility_availability.cpp','world_facility_options.cpp','production_retool.cpp','world_production_retool.cpp','local_production_session.cpp','local_session_actions.cpp','movement_snapshot.cpp','production_snapshot.cpp','local_actions_snapshot.cpp','local_actions_probe.cpp']
def rule_data():
 t=sections(rules_fixture('base'));t['CHASSIS'][1][9]='0';_,fac=tables('base')
 for row in fac:row[3]='None'
 t['FACILITIES']=fac;t['RULES'][ROW['retool_strictness']][0]='2';t['RULES'][ROW['retool_pct_pen_prod_chg']][0]='50';t['RULES'][ROW['retool_exemption']][0]='10'
 return material(t)
def local_world(data):
 b=bytearray(data);info,off,_=decode(b);pref=off['preferences'] if int(info['version'])>=11 else off['legacy'];shift=0 if int(info['version'])>=11 else 0x7c
 b[pref+0x58]=1<<5;struct.pack_into('<i',b,pref+0x390-shift,5);struct.pack_into('<i',b,pref+0x34,0)
 return bytes(b)
def expected(data,args,status,price):
 info,off,_=decode(data)
 if int(info['version'])>=11:return [8]
 id,_,_,x,y=args
 prep=0
 if id>=int(info['vehicles']):prep=1
 elif data[off['vehicles']+id*52+14]!=5:prep=6
 elif x<0 or x>=int(info['width']) or y<0 or y>=int(info['height']) or (x+y)&1:prep=1
 if prep:return [0,prep,-1,-1,0,0]
 return [0,0,0,['moved','exhausted','not_owner','unsupported','bad_state','out_of_memory'].index(status),price,int(status=='moved')]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-rules',type=Path);ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);binary=a.out/'local-actions-probe';cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(binary)];subprocess.run(cmd,check=True);records=[];digest=hashlib.sha256();text=rule_data()
 def check(name,data,rulebytes,args,mode,want,payload=b''):
  i=len(records);rp=a.out/(str(i)+'.txt');sp=a.out/(str(i)+'.SC');rp.write_bytes(rulebytes);sp.write_bytes(data);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(binary),str(rp),str(sp),str(args[0]),str(args[3]),str(args[4]),str(mode)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));head=r.stdout[:-len(payload)] if payload else r.stdout;got=list(struct.unpack('<'+'i'*(len(head)//4),head)) if len(head)%4==0 else []
  if r.returncode or (want is not None and got!=want) or (payload and not r.stdout.endswith(payload)):raise AssertionError((name,r.returncode,got,want,r.stderr.decode(errors='replace')))
  assert sp.read_bytes()==data;digest.update(r.stdout);(a.out/(str(i)+'.bin')).write_bytes(r.stdout);records.append({'name':name,'result':got,'mode':mode,'payloadSHA256':hashlib.sha256(payload).hexdigest() if payload else None,'passed':True})
 for name,data,args,status,price in controls():
  if name in ['wrong-player','native-player','bad-road']:continue # These caller-supplied authority/rate fields no longer exist.
  data=local_world(data);check(name,data,text,args,0,expected(data,args,status,price))
 # Same owned world: production plans/step commands invalidate each other;
 # exhaustion preserves a pending production confirmation, no turn refill.
 b=bytearray(local_world(fixture(bases=1)));info,o,_=decode(b);p=o['bases'];b[p+4]=5;struct.pack_into('<i',b,p+0x4c,0);struct.pack_into('<i',b,p+0x50,320);struct.pack_into('<i',b,p+0x44,0);struct.pack_into('<i',b,p+0x40,400);struct.pack_into('<i',b,p+0x98,111);b[p+0x8c:p+0x94]=bytes(8)
 # Independent complete-container oracle for the two moves and three facility
 # switches. Both moves return to the starting coordinates but retain spent6,
 # facing6, moved flag and the destination's low-nibble occupancy owner.
 from test_original_faction import SCHEMA
 expected_bytes=bytearray(b);vp=o['vehicles'];struct.pack_into('<I',expected_bytes,vp+4,struct.unpack_from('<I',b,vp+4)[0]|4);expected_bytes[vp+40]=6;expected_bytes[vp+41]=6
 target=o['tiles']+((3*int(info['width'])+3)//2)*44;expected_bytes[target+2]=(expected_bytes[target+2]&0xf0)|5
 struct.pack_into('<i',expected_bytes,p+0x50,-3);struct.pack_into('<i',expected_bytes,p+0x40,60)
 queue=next(f['offset'] for f in SCHEMA['LegacyFactionV10']['fields'] if f['name']=='units_queue');counter=o['factions']+5*0x20bc+queue+320;expected_bytes[counter]=(expected_bytes[counter]-1)%256
 check('shared-authority-interleaving',bytes(b),text,[0,5,3,3,3],1,[0,0,0,0,3,1,1,len(expected_bytes),-1412567295],bytes(expected_bytes))
 for path in a.original:
  assert a.original_rules;data=path.read_bytes();info,o,_=decode(data);width=int(info['width']);height=int(info['height']);rulebytes=a.original_rules.read_bytes()
  for id in range(int(info['vehicles'])):
   x,y=struct.unpack_from('<hh',data,o['vehicles']+id*52)
   for dx,dy in [(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]:
    nx=(x+dx)%width;ny=y+dy
    if ny<0 or ny>=height:continue
    check('original-'+path.name+'-'+str(id)+'-'+str(dx)+','+str(dy),data,rulebytes,[id,5,3,nx,ny],0,None)
  assert path.read_bytes()==data
 r={'cases':len(records),'outputSHA256':digest.hexdigest(),'records':records,'binarySHA256':hashlib.sha256(binary.read_bytes()).hexdigest(),'command':cmd,'nativeExecution':'UNRUN'};(a.out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({k:v for k,v in r.items() if k not in ['records','command']}))
if __name__=='__main__':main()
