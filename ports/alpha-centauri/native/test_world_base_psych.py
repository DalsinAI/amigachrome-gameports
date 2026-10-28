#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh connected psych vs independently decoded current-world facts."""
import argparse,hashlib,json,math,os,struct,subprocess
from pathlib import Path
import test_world_worker_upkeep as worker
import test_world_stable_intake as intake
import test_base_support as support
import test_upkeep_base_psych as leaf
HERE=Path(__file__).resolve().parent
SOURCES='rules_text original_map original_game original_faction original_world base_support world_base_support prototype movement saved_speed world_unit_stats base_psych world_base_psych world_base_psych_probe'.split()
def fixture(case,version=9):
 b=bytearray(worker.fixture(case,version));_,off,_=intake.decode(b);legacy=version<11;schema=intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction'];fs=0x20bc if legacy else 0x20cc;definition=intake.FACTIONS['MFaction'];state_start=off['factions']+fs;definition_start=off['faction_definitions']+0x59c;pf=off['legacy'] if legacy else off['preferences']
 def put(start,schema,name,value,index=0):
  field=next(f for f in schema['fields'] if f['name']==name);fmt=intake.FORMATS[field['type']];struct.pack_into('<'+fmt,b,start+field['offset']+index*struct.calcsize(fmt),value)
 for name,value in [('SE_talent_pending',case%7-3),('SE_police_pending',case%11-5),('SE_Economics_pending',case%4),('diff_level',case%6),('base_count',-123456789)]:put(state_start,schema,name,value)
 for name,value in [('rule_drone',case%5),('rule_talent',case%7),('rule_flags',0x40 if case&1 else 0),('faction_bonus_count',2)]:put(definition_start,definition,name,value)
 for i,value in enumerate([14,14]):put(definition_start,definition,'faction_bonus_id',value,i);put(definition_start,definition,'faction_bonus_val1',case%4 if i==0 else 12345678,i)
 def pref(name,index=0):
  f=next(f for f in intake.SCHEMA['GamePrefs']['fields'] if f['name']==name);return pf+f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0)+index*struct.calcsize(intake.FORMATS[f['type']])
 struct.pack_into('<i',b,pref('DiffLevel'),case%6)
 for i,project in enumerate([0,6,7,14,25,26,30]):struct.pack_into('<i',b,pref('SecretProjects',project),0 if case&(1<<i) else -1)
 p=off['bases'];pop=case*7%32;b[p+6]=pop;b[p+7]=case*7%100;b[p+8]=4 if case%13==0 else 0
 for i,facility in enumerate([6,8,11,12,17,18,22,23]):
  at=p+0x8c+facility//8;bit=1<<(facility%8);b[at]=(b[at]|bit) if case&(1<<i) else (b[at]&~bit)
 for delta in [0x104,0x118,0x11c,0x120]:struct.pack_into('<i',b,p+delta-(4 if legacy else 0),-123456789)
 # Deliberately preserve malformed saved base_count and psych/drone aggregates:
 # the adapter must derive live owned base count and all fresh counts instead.
 return bytes(b)
def model(text,data,specialists,psych):
 fail=lambda e:{'BASE':[e]};info,off,spans=intake.decode(data);b=spans['bases'][:304];pop=struct.unpack_from('<b',b,6)[0];faction=b[4]
 if faction<1 or faction>7 or pop<0 or not 0<=specialists<=pop or psych<0:return fail(1)
 if int(info['version'])>=11:return fail(3)
 schema=intake.FACTIONS['LegacyFactionV10'];f=spans['factions'][faction*0x20bc:(faction+1)*0x20bc];definition=spans['faction_definitions'][faction*0x59c:(faction+1)*0x59c];pf=spans['legacy'];tables=intake.sections(text)
 state=lambda name:intake.scalar(f,schema,name);rule=lambda name:intake.scalar(definition,intake.FACTIONS['MFaction'],name)
 def pref(name,index=0):
  field=next(x for x in intake.SCHEMA['GamePrefs']['fields'] if x['name']==name);delta=field['offset']-(0x7c if field['offset']>=0x184 else 0);fmt=intake.FORMATS[field['type']];return struct.unpack_from('<'+fmt,pf,delta+index*struct.calcsize(fmt))[0]
 if not pref('FactionStatus')&(1<<faction):return fail(3)
 if not 0<=rule('faction_bonus_count')<=8 or not 0<=pref('DiffLevel')<=5 or not 0<=state('diff_level')<=5:return fail(1)
 def project(i):
  index=pref('SecretProjects',i)
  if index>=int(info['bases']):raise ValueError('project')
  return index>=0 and spans['bases'][304*index+4]==faction
 def fac(i):return int(bool(b[0x8c+i//8]&(1<<(i%8))))
 content=6-pref('DiffLevel');eff=max(4,4+state('SE_effic_pending'));product=(content+2)*eff*math.isqrt(int(info['width'])*int(info['height'])//2)
 if product>2147483647:return fail(5)
 count=sum(spans['bases'][304*i+4]==faction for i in range(int(info['bases'])));nodrone=0
 for i in range(rule('faction_bonus_count')):
  if intake.scalar(definition,intake.FACTIONS['MFaction'],'faction_bonus_id',i)==14:nodrone=intake.scalar(definition,intake.FACTIONS['MFaction'],'faction_bonus_val1',i);break
 supported=support.world_expected(data,text)
 if supported[0]:return fail({3:3,4:4,5:5}.get(supported[0],1))
 police=[];pending=state('SE_police_pending')
 if pending>=-1:
  value=1+int(pending>=3)
  if project(25):police.append(value)
  x,y=struct.unpack_from('<hh',b)
  for index in range(int(info['vehicles'])):
   v=spans['vehicles'][52*index:52*(index+1)]
   if struct.unpack_from('<hh',v)!=(x,y):continue
   proto=struct.unpack_from('<h',v,10)[0];p=spans['prototypes'][52*proto:52*(proto+1)];plan=struct.unpack_from('<b',p,42)[0]
   if p[36]>=9 or p[37]>=21 or not 0<=plan<=14:return fail(1)
   triad=int(tables['CHASSIS'][p[36]][9]);offense=int(tables['WEAPONS'][p[37]][2])
   if plan<=3 and triad!=1:police.append(value+int(bool(struct.unpack_from('<I',p,32)[0]&0x400000))+int(bool(rule('rule_flags')&0x40) and triad==0 and proto<64 and offense<0))
 police=sorted(police,reverse=True)[:3]+[0]*3;police=police[:3];row,col,limits=intake.ROW['drones_genejack_factory'];genejack=max(limits[0],min(limits[1],int(tables['RULES'][row][col])))
 facts=[pop,specialists,content,(product//56)//2,0,count,state('diff_level'),b[7],rule('rule_drone'),rule('rule_talent'),nodrone,state('SE_talent_pending'),pending,psych,genejack,supported[4],*police,fac(22),fac(6),int(bool(fac(11) or fac(8) and project(6))),fac(17),fac(18),fac(12),int(project(7)),int(bool(b[8])),fac(23),int(project(30)),int(project(14)),int(project(0)),int(project(26)),state('SE_Economics_pending')]
 result=leaf.expected(facts)
 if result[0]:return fail(result[0])
 return {'BASE':[0],'FACTS':facts,'PSYCH':result[1:]}
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,default=Path('/tmp/alpha-world-psych'));p.add_argument('--cc',default='g++');p.add_argument('--binary',type=Path);a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True);exe=a.binary or a.out/'probe';command=[]
 if not a.binary:
  command=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(name+'.cpp')) for name in SOURCES],'-o',str(exe)];subprocess.run(command,check=True)
 text=intake.rules('base');cases=[]
 for i in range(1,65):
  data=fixture(i,7+i%4);pop=struct.unpack_from('<b',intake.decode(data)[2]['bases'],6)[0];cases.append((f'base-{i}',data,i%(pop+1),i*3,text))
 seed=fixture(7,9);cases.extend([(f'psych-{value}',seed,0,value,text) for value in [0,1,2,3,4,20,99,2147483647,-1]]);cases.extend([('bad-special',seed,127,20,text),('SMAX',fixture(7,12),0,20,text)])
 police_seed=bytearray(fixture(8,9));_,police_off,_=intake.decode(police_seed);p=police_off['bases'];x,y=struct.unpack_from('<hh',police_seed,p)
 flag=next(f for f in intake.FACTIONS['MFaction']['fields'] if f['name']=='rule_flags');struct.pack_into('<I',police_seed,police_off['faction_definitions']+0x59c+flag['offset'],0x40)
 for i in range(3):
  v=police_off['vehicles']+52*i;struct.pack_into('<hh',police_seed,v,x,y);struct.pack_into('<h',police_seed,v+10,i+1);police_seed[v+14]=1 if i<2 else 2;struct.pack_into('<h',police_seed,v+46,0 if i<2 else -1)
  proto=police_off['prototypes']+52*(i+1);police_seed[proto+36]=0;police_seed[proto+37]=12 if i<2 else 0;police_seed[proto+42]=0;struct.pack_into('<I',police_seed,proto+32,0x400000 if i==0 else 0)
 cases.append(('strongest-police-including-foreign',bytes(police_seed),0,0,text))
 for name,chassis,plan in [('sea-excluded',1,0),('air-police',2,0),('recon-police',0,3),('noncombat-plan-excluded',0,4)]:
  changed=bytearray(police_seed);proto=police_off['prototypes']+52;changed[proto+36]=chassis;changed[proto+42]=plan;cases.append((name,bytes(changed),0,0,text))
 sphere=bytearray(police_seed);sphere[p+0x8c+23//8]|=1<<(23%8);cases.append(('punishment-sphere',bytes(sphere),0,0,text))
 different=bytearray(police_seed);pf=police_off['legacy'];field=next(f for f in intake.SCHEMA['GamePrefs']['fields'] if f['name']=='DiffLevel');struct.pack_into('<i',different,pf+field['offset']-(0x7c if field['offset']>=0x184 else 0),5);cases.append(('content-global-difficulty',bytes(different),0,0,text))
 digest=hashlib.sha256();records=[]
 for name,data,special,psych,rules in cases:
  path,rulepath=a.out/'input.SC',a.out/'rules.txt';path.write_bytes(data);rulepath.write_bytes(rules)
  run=subprocess.run([str(exe),str(rulepath),str(path),str(special),str(psych)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert run.returncode==0,(name,run.returncode,run.stderr)
  actual={line.split()[0]:list(map(int,line.split()[2:])) for line in run.stdout.decode().splitlines() if line.split()[1]=='0'};expected=model(rules,data,special,psych);assert actual==expected,(name,actual,expected)
  assert path.read_bytes()==data and rulepath.read_bytes()==rules;digest.update(run.stdout);(a.out/(name+'.SC')).write_bytes(data);(a.out/(name+'.rules.txt')).write_bytes(rules);(a.out/(name+'.expected.txt')).write_bytes(run.stdout);records.append(dict(name=name,specialists=special,psych=psych,accepted=actual['BASE']==[0],outputSHA256=hashlib.sha256(run.stdout).hexdigest()))
 receipt=dict(worlds=len(cases),accepted=sum(r['accepted'] for r in records),outputSHA256=digest.hexdigest(),allInputsAndOutputUnchangedOnFailure=True,allAliasRegionsChecked=True,command=command,records=records,native='UNRUN');assert receipt['accepted']>=60
 (a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ('command','records')}))
if __name__=='__main__':main()
