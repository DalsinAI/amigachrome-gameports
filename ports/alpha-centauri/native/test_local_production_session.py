#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,re,struct,subprocess
from pathlib import Path
from test_original_game import decode
from test_original_faction import SCHEMA
from test_facility_definitions import tables,material
from test_world_facility_options import fixture as world_fixture,model as options_model,SOURCES as WORLD_SOURCES
from test_production_retool import model as retool_model
HERE=Path(__file__).resolve().parent
TARGETS=[1,2,3,4,5,7,10,11,12,14,20,21,23,25,27,28,29,30,32,34,39,65,70,71,101,102,133]
SOURCES=WORLD_SOURCES[:-1]+['production_retool.cpp','world_production_retool.cpp','local_production_session.cpp','local_production_probe.cpp']

def rule_rows():
 index=-1;mapping={}
 for line in (HERE/'basic_fields.inc').read_text().splitlines():
  m=re.match(r'RULE_(ROW|ITEM|TECH)\((\w+)',line)
  if not m:continue
  if m[1]!='ITEM':index+=1
  mapping[m[2]]=index
 return mapping
ROW=rule_rows()
def rules(profile,case):
 tech,fac=tables(profile)
 if case%3:
  for row in fac:row[3]='Disable' if case%5==4 else 'None'
 tech[1][6:8]=['Disable','None'];tech[2][6:8]=['None','Disable'];tech[3][6:8]=['T0','Disable']
 a=json.loads((HERE/'basic_fixture.json').read_text());rows=[r[:] for r in a['rules_rows']]
 for key,value in [('retool_strictness',3 if case==16 else case%4),('retool_pct_pen_prod_chg',50 if case==16 else [0,1,50,99,100][case%5]),('retool_exemption',10 if case==16 else [0,10,1000][case%3])]:rows[ROW[key]][0]=str(value)
 if profile=='base':rows=rows[:-2]
 return material(tech,fac)+('#RULES\n'+'\n'.join(','.join(r) for r in rows)+'\n#WORLDBUILDER\n'+'\n'.join(','.join(r) for r in a['world_rows'])+'\n#NEXT\n').encode()

def fixture(version,case):
 b=bytearray(world_fixture(version,case));info,off,spans=decode(b);pref=off['preferences'] if version>=11 else off['legacy'];human=5 if case%2 else 1;struct.pack_into('<i',b,off['player'],human);b[pref+0x58]=1<<human
 if case==15:b[pref+0x58]=0
 struct.pack_into('<i',b,pref+0x390-(0 if version>=11 else 0x7c),({36:human%7+1,37:0,38:-1}.get(case,human)))
 # Every normal load has qualified numeric difficulty/project metadata; the
 # earlier fixture's other fields exercise known tech, filename and coast.
 struct.pack_into('<i',b,pref+0x34,6 if case==32 else case%6)
 bs=308 if version>=11 else 304;first=70 if version>=11 else 39
 previous=-first if case==16 else [0,-1,-34,-first,511,-10][case%6];old=0 if case==16 else [0,511,-1,-3,-first,-(first+32)][case%6]
 if case==16:struct.pack_into('<i',b,pref+0xc,2048)
 if case==14:old=512
 for base in range(int(info['bases'])):
  bp=off['bases']+base*bs;b[bp+4]=human if base==0 or case%4==0 else (human%7+1);struct.pack_into('<i',b,bp+0x30,0x10000000 if case==10 else 0);struct.pack_into('<i',b,bp+0x4c,1 if case==3 else -1 if case==9 else 0);struct.pack_into('<i',b,bp+0x44,previous);struct.pack_into('<i',b,bp+0x50,old);struct.pack_into('<i',b,bp+0x40,[-101,0,17,400][case%4]);struct.pack_into('<i',b,bp+(0x9c if version>=11 else 0x98),(111 if case==16 else [-1,10,11,111,21474836,2147483647][case%6]))
 fs=0x20cc if version>=11 else 0x20bc;schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];field=next(f for f in schema['fields'] if f['name']=='units_queue');b[off['factions']+human*fs+field['offset']:off['factions']+human*fs+field['offset']+512]=bytes([(case*17)%256])*512
 return bytes(b),human

def oracle(text,data,profile,enabled,human):
 info,off,spans=decode(data);version=int(info['version']);pref=off['preferences'] if version>=11 else off['legacy'];baseprofile=profile=='base';nbase=int(info['bases']);bs=308 if version>=11 else 304;count=72 if baseprofile else 134
 if human<1 or human>7 or (baseprofile and enabled):return struct.pack('<i',1)
 if (version<11)!=baseprofile:return struct.pack('<i',8)
 if int(info['player'])!=human or not data[pref+0x58]&(1<<human):return struct.pack('<i',5)
 if struct.unpack_from('<i',data,pref+0x390-(0 if version>=11 else 0x7c))[0]!=human:return struct.pack('<i',8)
 options=options_model(text,data,profile,enabled);global_index=(human-1)*(nbase+1)*3
 if struct.unpack_from('<i',options,global_index*158+12)[0]:return struct.pack('<i',1)
 from test_facility_definitions import sections
 rows=sections(text)['rules']
 strict=min(2,max(0,min(3,int(rows[ROW['retool_strictness']][0]))))
 penalty=max(0,min(100,int(rows[ROW['retool_pct_pen_prod_chg']][0])))
 exemption=max(0,min(1000,int(rows[ROW['retool_exemption']][0])))
 fs=0x20cc if version>=11 else 0x20bc;schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];queue_offset=next(f for f in schema['fields'] if f['name']=='units_queue')['offset'];out=bytearray(struct.pack('<i',0));accepted=0;sequences=0
 def bounded(n):return 0<=n<512 or -count<n<0
 for base in range(nbase):
  bp=off['bases']+base*bs;faction=data[bp+4];state=struct.unpack_from('<I',data,bp+0x30)[0];queue_size=struct.unpack_from('<i',data,bp+0x4c)[0];old=struct.unpack_from('<i',data,bp+0x50)[0];prior=struct.unpack_from('<i',data,bp+0x44)[0];before=struct.unpack_from('<i',data,bp+0x40)[0];baseline=struct.unpack_from('<i',data,bp+(0x9c if version>=11 else 0x98))[0];bits=data[bp+0x8c:bp+0x8c+(12 if version>=11 else 8)]+bytes(0 if version>=11 else 4);words=struct.unpack('<3I',bits);old_count=data[off['factions']+human*fs+queue_offset+old] if 0<=old<512 else 0
  changing=0
  for facility in TARGETS:
   if facility>=count:continue
   error=0;commit=-1;changed=loss=0;after=before;new_item=old;new_count=old_count
   if faction!=human:error=6
   elif state&0x10000000 or queue_size!=0:error=8
   elif not bounded(old) or not bounded(prior):error=1
   elif old!=-facility:
    option_index=(human-1)*(nbase+1)*3+(base+1)*3
    if not options[option_index*158+24+facility]:error=7
    else:
     result=retool_model((int(not baseprofile),strict,penalty,exemption,1,prior,-facility,baseline,*words))
     if result[0]:error=11 if result[0]==2 else 1
     else:changed=1;loss=result[3];after=baseline-loss;new_item=-facility;new_count=(old_count-1)%256 if old>=0 else old_count
   if not error:accepted+=1;commit=0;changing+=changed
   out+=struct.pack('<13i',base,facility,error,commit,changed,old,new_item,before,baseline,loss,after,old_count,new_count)
  if changing>=2:sequences+=changing
 out+=struct.pack('<2i',accepted,sequences);return bytes(out)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-rules',type=Path);ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);binary=a.out/'local-production-probe';flags=['-std=c++17','-O1','-Wall','-Wextra','-Werror'];flags+=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'];cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(binary)];subprocess.run(cmd,check=True);records=[];digest=hashlib.sha256();accepted=attempts=sequences=0
 def check(name,text,data,profile,enabled,human):
  nonlocal accepted,attempts,sequences
  i=len(records);rp=a.out/(str(i)+'.txt');sp=a.out/(str(i)+'.SC');rp.write_bytes(text);sp.write_bytes(data);wanted=oracle(text,data,profile,enabled,human);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(binary),profile,str(int(enabled)),str(human),str(rp),str(sp)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=120)
  if r.returncode or r.stdout!=wanted:
   print('FAILED',name,'RC',r.returncode,'STDERR',r.stderr.decode(errors='replace'),'BYTES',len(r.stdout),len(wanted))
   for k in range(min(len(r.stdout),len(wanted))//4):
    g,w=struct.unpack_from('<i',r.stdout,k*4)[0],struct.unpack_from('<i',wanted,k*4)[0]
    if g!=w:print('FIRST WORD',k,g,w);break
   raise AssertionError(name)
  assert sp.read_bytes()==data;digest.update(r.stdout);(a.out/(str(i)+'.bin')).write_bytes(r.stdout);n=(len(wanted)-12)//52 if len(wanted)>4 else 0;ok,chain=struct.unpack_from('<2i',wanted,len(wanted)-8) if len(wanted)>4 else (0,0);attempts+=n;accepted+=ok;sequences+=chain;records.append({'name':name,'profile':profile,'expansion':enabled,'human':human,'attempts':n,'accepted':ok,'sequences':chain,'loadError':struct.unpack_from('<i',wanted)[0],'passed':True,'outputSHA256':hashlib.sha256(r.stdout).hexdigest()})
 for version in range(7,13):
  profile='base' if version<11 else 'expansion'
  for case in list(range(17))+[31,32,33,34,35,36,37,38]:
   text=rules(profile,case);data,human=fixture(version,case);check('v'+str(version)+'-'+str(case),text,data,profile,version>=11 and bool(case%2),human)
 for p in a.original:
  assert a.original_rules;data=p.read_bytes();info,_,_=decode(data);check('original-'+p.name,a.original_rules.read_bytes(),data,'base',False,int(info['player']));assert p.read_bytes()==data
 receipt={'worldContexts':len(records),'orderAttempts':attempts,'acceptedPlans':accepted,'rejectedPlans':attempts-accepted,'sequenceContinuations':sequences,'outputSHA256':digest.hexdigest(),'cases':records,'binarySHA256':hashlib.sha256(binary.read_bytes()).hexdigest(),'command':cmd,'nativeExecution':'UNRUN'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['cases','command']}))
if __name__=='__main__':main()
