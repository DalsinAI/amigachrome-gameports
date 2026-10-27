#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""World stage1 proposal against independent wire/radius/tile/support models."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_world_base_accounting as accounting
import test_world_stable_intake as intake
import test_world_convoy_nutrients as convoy
import test_base_support as support
import test_worker_allocation as allocation
from test_base_energy import expected as energy_expected
HERE=Path(__file__).resolve().parent
SOURCES=list(dict.fromkeys([name for name in accounting.SOURCES if name!='world_base_accounting_probe']+'worker_allocation world_worker_upkeep world_worker_upkeep_probe'.split()))
def fixture(case,version):
 b=bytearray(accounting.fixture(case,version));_,off,_=intake.decode(b);legacy=version<11;schema=intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction'];fs=0x20bc if legacy else 0x20cc
 for name,value in [('SE_alloc_psych',case%4),('SE_alloc_labs',case%5)]:
  field=next(f for f in schema['fields'] if f['name']==name);struct.pack_into('<i',b,off['factions']+fs+field['offset'],value)
 return bytes(b)
def modified(data,mask=None,pop=None,special=None):
 b=bytearray(data);_,off,_=intake.decode(data);p=off['bases']
 if mask is not None:struct.pack_into('<I',b,p+0x78,mask)
 if pop is not None:struct.pack_into('<b',b,p+6,pop)
 if special is not None:struct.pack_into('<i',b,p+0x7c,special)
 return bytes(b)
def signed(x):return x-(1<<32) if x>=1<<31 else x
def model(text,data,mode=None):
 info,off,spans=intake.decode(data);b=spans['bases'][:304];faction=b[4];pop=struct.unpack_from('<b',b,6)[0];special=struct.unpack_from('<i',b,0x7c)[0]
 fail=lambda error:{'BASE':[error]}
 if mode in ('inspection','editor') or int(info['version'])>=11:return fail(3)
 if faction<1 or faction>7 or pop<0 or special<0 or special>pop:return fail(1)
 schema=intake.FACTIONS['LegacyFactionV10'];f=spans['factions'][faction*0x20bc:(faction+1)*0x20bc];definition=spans['faction_definitions'][faction*0x59c:(faction+1)*0x59c]
 state=lambda name:intake.scalar(f,schema,name)
 tables=intake.sections(text)
 def basic(name):
  row,col,limits=intake.ROW[name];return max(limits[0],min(limits[1],int(tables['RULES'][row][col])))
 pf=spans['legacy']
 def pref(name,index=0):
  field=next(x for x in intake.SCHEMA['GamePrefs']['fields'] if x['name']==name);delta=field['offset']-(0x7c if field['offset']>=0x184 else 0);fmt=intake.FORMATS[field['type']];return struct.unpack_from('<'+fmt,pf,delta+index*struct.calcsize(fmt))[0]
 if not pref('FactionStatus')&(1<<faction) or struct.unpack_from('<I',b,0x38)[0]&0x80000040==0x80000040:return fail(3)
 supported=support.world_expected(data,text)
 if supported[0]:return fail({3:3,4:6,5:7}.get(supported[0],1))
 radius=intake.radius_expected(data)
 if radius[0]:return fail(5 if radius[0]==3 else 1)
 flow_error,flows=convoy.flow_model('base',text,data)
 if flow_error:return fail(flow_error)
 available=(~radius[1]&0x1fffff)|1
 def project(index):
  bid=pref('SecretProjects',index) if index<33 else -1
  if bid>=int(info['bases']):raise ValueError('project')
  return bid>=0 and spans['bases'][304*bid+4]==faction
 def fac(index):return bool(b[0x8c+index//8]&(1<<(index%8)))
 full=fac(29) or project(27);counts=[state(name) for name in ['satellites_nutrient','satellites_mineral','satellites_energy']]
 if min(counts)<0:return fail(1)
 if not full and max(counts)==2147483647:return fail(7)
 satellites=[min(pop,c if full else (c+1)//2) for c in counts]
 values=[]
 for i in range(21):
  if not available>>i&1:values.extend([0,0,0]);continue
  temp=modified(data,mask=1|(1<<i),pop=int(i!=0),special=0);fresh=intake.model('base',text,temp,1 if mode=='control' else 0)
  if fresh[0]:return fail(fresh[0])
  values.extend(map(signed,fresh[16+3*i:19+3*i]))
 width=int(info['width']);flat=struct.unpack_from('<I',spans['metadata'],28)[0];x,y=struct.unpack_from('<hh',b);distance=999
 for index in range(int(info['bases'])):
  other=spans['bases'][304*index:304*(index+1)]
  if other[4]!=faction or not other[0x8c]&2:continue
  xx,yy=struct.unpack_from('<hh',other);dx,dy=abs(x-xx),abs(y-yy)
  if not flat and dx>width//2:dx=width-dx
  distance=min(distance,(dx+dy)//2)
 energy=energy_expected([16,0,0,0,0,distance,state('SE_effic_pending'),0,int(fac(2)),*([0]*9)])
 assert energy[0]==0
 cap=127 if fac(25) else basic('pop_limit_sans_hab_dome' if fac(24) else 'pop_limit_sans_hab_complex')+(2 if project(13) else 0)-intake.scalar(definition,intake.FACTIONS['MFaction'],'rule_population')
 facts=[available,struct.unpack_from('<I',b,0x78)[0],pop,special,satellites[0]+flows[0]-flows[4]-pop*basic('nutrient_req_citizen'),satellites[1]+flows[1]-flows[5]-supported[3],satellites[2]+flows[2]-flows[6],16-energy[3],state('SE_alloc_psych'),2*basic('nutrient_req_citizen'),int(cap>pop),int(not b[8] and not project(30) and not fac(23))]
 selected=allocation.model(facts+values)
 if selected[0]:return fail(7 if selected[0]==3 else 1)
 mask=selected[1];temp=modified(data,mask=mask,pop=mask.bit_count()-1,special=0);fresh=intake.model('base',text,temp,1 if mode=='control' else 0)
 if fresh[0]:return fail(fresh[0])
 fresh=list(map(signed,fresh[1:]));fresh[10]&=0xffffffff;fresh[11]&=0xffffffff;fresh[0:3]=satellites
 fresh[6:9]=[fresh[3+k]+satellites[k] for k in range(3)]
 return {'BASE':[0],'FACTS':facts,'ALLOC':selected[1:],'YIELDS':values,'INTAKE':fresh}
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,default=Path('/tmp/alpha-world-worker'));p.add_argument('--cc',default='g++');p.add_argument('--binary',type=Path);p.add_argument('--only');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True);exe=a.binary or a.out/'probe';command=[]
 if not a.binary:
  command=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(name+'.cpp')) for name in SOURCES],'-o',str(exe)];subprocess.run(command,check=True)
 text=intake.rules('base');tables=intake.sections(text);row,col,_=intake.ROW['nutrient_req_citizen'];tables['RULES'][row][col]='2';text=intake.material(tables)
 cases=[(f'directed-{i}',fixture(i,7+i%4),None,text) for i in range(1,41)]
 seed=fixture(7,9)
 cases.extend((name,modified(seed,mask=mask,pop=pop,special=special),None,text) for name,mask,pop,special in [('new-worker',1,5,0),('overassigned',0x1fffff,2,0),('high-bits',0xffffffff,5,0),('specialists',1,5,5),('large-pop',1,30,0),('zero-pop',1,0,0),('empty-mask',0,5,2)])
 cases.extend([(mode,seed,mode,text) for mode in ['inspection','editor','control']])
 _,off,_=intake.decode(seed)
 for name,delta,fmt,value in [('bad-special',0x7c,'i',99),('governor',0x38,'I',0x80000040),('riot-immune',0x8,'B',1)]:
  changed=bytearray(seed);struct.pack_into('<'+fmt,changed,off['bases']+delta,value);cases.append((name,bytes(changed),None,text))
 for name,variant in [('SMAX',12)]:cases.append((name,fixture(7,variant),None,text))
 for name,source in [('fungus',intake.fresh_fungus_fixture(seed,52)),('fresh-tech-overflow',intake.fresh_fungus_fixture(seed,52,True))]:cases.append((name,source,None,text))
 changed=bytearray(modified(fixture(32,9),mask=1,pop=3,special=0));_,eco_off,_=intake.decode(changed)
 for facility in [13,14]:changed[eco_off['bases']+0x8c+facility//8]&=~(1<<(facility%8))
 for index in range(96):
  if index!=52:
   at=eco_off['tiles']+44*index+8;struct.pack_into('<I',changed,at,(struct.unpack_from('<I',changed,at)[0]&3)|0x200000|0x40)
 cases.append(('proposed-mask-ecology',bytes(changed),None,text))
 if a.only:cases=[case for case in cases if case[0]==a.only]
 assert cases
 digest=hashlib.sha256();records=[]
 for name,data,mode,rules in cases:
  path,rule=a.out/'input.SC',a.out/'rules.txt';path.write_bytes(data);rule.write_bytes(rules)
  run=subprocess.run([str(exe),str(rule),str(path),*([mode] if mode else [])],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert run.returncode==0,(name,run.returncode,run.stderr)
  actual={line.split()[0]:list(map(int,line.split()[2:])) for line in run.stdout.decode().splitlines() if line.split()[1]=='0'};expected=model(rules,data,mode)
  assert actual==expected,(name,actual,expected)
  if name=='proposed-mask-ecology':assert actual['BASE']==[0] and actual['INTAKE'][12]!=intake.ecology_expected(data)[1],actual
  (a.out/(name+'.SC')).write_bytes(data);(a.out/(name+'.rules.txt')).write_bytes(rules);(a.out/(name+'.expected.txt')).write_bytes(run.stdout)
  assert data==path.read_bytes() and rules==rule.read_bytes();digest.update(run.stdout);records.append(dict(name=name,accepted=actual['BASE']==[0],mode=mode,outputSHA256=hashlib.sha256(run.stdout).hexdigest()))
 receipt=dict(worlds=len(cases),accepted=sum(r['accepted'] for r in records),outputSHA256=digest.hexdigest(),command=command,records=records,native='UNRUN',allWorldAndRulesAndOutputsTransactional=True,upkeep1PreSpecialistProposalOnly=True)
 assert receipt['accepted']>=(1 if a.only else 40),receipt
 (a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ('command','records')}))
if __name__=='__main__':main()
