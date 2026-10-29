#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh stage1 resolution against independent wire and component models."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_world_worker_upkeep as worker
import test_world_base_psych as psych_world
import test_world_energy_allocation as energy_world
import test_proposed_energy_allocation as proposed
import test_world_base_minerals as minerals
import test_upkeep_base_minerals as mineral_leaf
import test_world_convoy_nutrients as convoy
from test_base_nutrients import expected as nutrient_expected
from test_upkeep_base_energy import expected as energy_expected
intake=worker.intake
HERE=Path(__file__).resolve().parent
SOURCES=list(dict.fromkeys([n for n in worker.SOURCES if n!='world_worker_upkeep_probe']+'upkeep_energy_allocation world_energy_allocation upkeep_base_psych world_base_psych world_worker_resolution world_worker_resolution_probe'.split()))
def rules():
 t=intake.sections(energy_world.rules());row,col,_=intake.ROW['nutrient_req_citizen'];t['RULES'][row][col]='2';t['CITIZENS']=[['Technician','Technicians','None','Disable','3','0','0'],['Doctor','Doctors','None','Disable','0','2','0'],['Librarian','Librarians','None','Disable','0','0','3'],['Empath','Empaths','T4','Disable','2','2','0'],['Engineer','Engineers','T5','Disable','3','0','3'],['Transcend','Transcends','T6','Disable','4','2','4'],['Other','Others','T7','Disable','3','3','3'],['Drone','Drones'],['Worker','Workers'],['Talent','Talents']];return intake.material(t)
def fixture(case,version=9):
 b=bytearray(psych_world.fixture(case,version));_,off,_=intake.decode(b);legacy=version<11;schema=intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction'];fs=0x20bc if legacy else 0x20cc
 def state(name,value):
  f=next(f for f in schema['fields'] if f['name']==name);struct.pack_into('<i',b,off['factions']+fs+f['offset'],value)
 state('base_count',1);state('SE_alloc_labs',case%5);state('SE_alloc_psych',case%3);state('nutrient_surplus_total',17)
 p=off['bases'];b[p+6]=case%10;struct.pack_into('<I',b,p+0x78,1 if case%3 else (1<<(b[p+6]+1))-1);struct.pack_into('<i',b,p+0x7c,0);struct.pack_into('<2I',b,p+0x84,0x11111111,0x11111111)
 # Forests supply alternatives; selected improvement damage must follow the
 # proposed mask instead of silently following the source worked mask.
 for i in range(96):
  at=off['tiles']+44*i
  if i!=52:struct.pack_into('<I',b,at+8,(struct.unpack_from('<I',b,at+8)[0]&3)|0x200000|0x40)
 return bytes(b)
def model(text,data,commerce=0,grid=0,mode=None,base_index=0):
 fail=lambda e:{'BASE':[e]}
 if commerce<0 or grid<0:return fail(1)
 candidate=worker.model(text,data,mode)
 if candidate['BASE']!=[0]:return {'BASE':candidate['BASE']}
 info,off,spans=intake.decode(data);b=spans['bases'][:304];faction=b[4];schema=intake.FACTIONS['LegacyFactionV10'];f=spans['factions'][faction*0x20bc:(faction+1)*0x20bc];tables=intake.sections(text)
 state=lambda name:intake.scalar(f,schema,name)
 def pref(name,index=0):
  field=next(x for x in intake.SCHEMA['GamePrefs']['fields'] if x['name']==name);delta=field['offset']-(0x7c if field['offset']>=0x184 else 0);fmt=intake.FORMATS[field['type']];return struct.unpack_from('<'+fmt,spans['legacy'],delta+index*struct.calcsize(fmt))[0]
 def basic(name):
  row,col,limits=intake.ROW[name];return max(limits[0],min(limits[1],int(tables['RULES'][row][col])))
 facts=candidate['FACTS'];available,original,pop,original_special,Nbefore,Mbefore,Ebefore,eff,psych_slider,threshold,grow,canriot=facts
 labs_slider=state('SE_alloc_labs');econ_slider=10-labs_slider-psych_slider
 if not 0<=labs_slider<=10 or not 0<=psych_slider<=10-labs_slider:return fail(1)
 foodcache=state('nutrient_surplus_total');socialfield=next(x for x in schema['fields'] if x['name']=='social_effic');social=list(struct.unpack_from('<9i',f,socialfield['offset']))
 flowerror,flows=convoy.flow_model('base',text,data)
 if flowerror:return fail(flowerror)
 support=worker.support.world_expected(data,text)
 def known(key):
  if key.lower()=='none':return True
  if key.lower()=='disable':return False
  i=next(i for i,row in enumerate(tables['TECHNOLOGY']) if row[1].lower()==key.lower());r=tables['TECHNOLOGY'][i];return i<88 and r[6].lower()!='disable' and not(r[7].lower()=='disable' and r[6].lower()!='none') and bool(pref('TechOwners',i)&(1<<faction))
 citizens=[list(map(int,row[4:7])) for row in tables['CITIZENS'][:7]]
 eligible=[known(row[2]) and not known(row[3]) and (citizens[i][1]>=2 or pop>=basic('min_base_size_specialists')) for i,row in enumerate(tables['CITIZENS'][:7])]
 if not any(eligible):return fail(4)
 needlabs=not pref('GameRules')&0x4000000 and not b[0x8c+23//8]&(1<<(23%8));ew=4+int(econ_slider>labs_slider);lw=2+2*int(needlabs and labs_slider>0)+int(econ_slider<=labs_slider);pw=2+2*canriot+4*int(canriot and state('SE_police_pending')<=-3 and support[4]>0)
 scores=[ew*c[0]+pw*c[1]+lw*c[2] if eligible[i] else -1 for i,c in enumerate(citizens)]
 if max(scores)>2147483647:return fail(7)
 best=max(range(7),key=lambda i:scores[i]);types=list(struct.unpack_from('<2I',b,0x84))
 for i in range(16):
  kind=(types[i//8]>>(4*(i%8)))&15
  if not original or kind>6 or not eligible[kind]:types[i//8]=(types[i//8]&~(15<<(4*(i%8))))|(best<<(4*(i%8)))
 allocation=candidate['ALLOC'];mask,special,unallocated=allocation[:3];added=allocation[10:10+allocation[6]];mask&=~sum(1<<i for i in added);remaining=len(added)+unallocated;N,M,E=Nbefore,Mbefore,Ebefore;yields=[candidate['YIELDS'][i:i+3] for i in range(0,63,3)]
 for i in range(21):
  if mask>>i&1:N+=yields[i][0];M+=yields[i][1];E+=yields[i][2]
 choices=[];Wenergy=1+int(eff>=6);gov=struct.unpack_from('<I',b,0x38)[0];Wmineral=3+int(bool(gov&0x4000000))+int(bool(gov&0x8000000));Wmineral+=int(Wmineral==5)
 for i in added:
  yn,ym,ye=yields[i];Wnutrient=1+int(grow or N<0 or M<2)+int(pop<basic('min_base_size_specialists')+2)
  if 2*(citizens[best][2]+citizens[best][0])+(2 if canriot else 1)*citizens[best][1]>Wnutrient*yn+Wmineral*ym+Wenergy*ye and N>=(threshold if grow else 0) and M>=2 and (M+support[3]>=(pop*Wmineral+5)//2 or Wmineral<5 and special+remaining<pop//2 and yn+ym+ye<=2):break
  mask|=1<<i;choices.append(i);remaining-=1;N+=yn;M+=ym;E+=ye
 special+=remaining
 def evaluate(mask,special,types):
  patched=worker.modified(data,mask=mask,pop=mask.bit_count()-1,special=0);fresh=intake.model('base',text,patched,1 if mode=='control' else 0)
  if fresh[0]:return fail(fresh[0])
  fresh=list(map(worker.signed,fresh[1:]));fresh[10]&=0xffffffff;fresh[11]&=0xffffffff;fresh[:3]=candidate['INTAKE'][:3];fresh[6:9]=[fresh[3+i]+fresh[i] for i in range(3)]
  mint=minerals.model('base',text,patched,0,support[3])
  if mint[1]:return fail(mint[1])
  mineralfacts=mint[2:33];mineralfacts[0]=fresh[7];mineralfacts[4]=fresh[12];mineralfacts[7]=pop;m=mineral_leaf.expected(mineralfacts)
  if m[0]:return fail(7 if m[0]==3 else 1)
  nf=nutrient_expected([pop,state('SE_growth_pending'),fresh[6],flows[0],flows[4],basic('nutrient_req_citizen'),struct.unpack_from('<i',b,0x3c)[0],foodcache,1,int(bool(b[0x8c]&4))|(2*int(bool(struct.unpack_from('<I',b,0x30)[0]&4)))])
  if nf[0]:return fail(7 if nf[0]==3 else 1)
  x,y=struct.unpack_from('<hh',b);distance=999;width=int(info['width']);flat=struct.unpack_from('<I',spans['metadata'],28)[0]
  for index in range(int(info['bases'])):
   other=spans['bases'][304*index:304*(index+1)]
   if other[4]!=faction or not other[0x8c]&2:continue
   xx,yy=struct.unpack_from('<hh',other);dx,dy=abs(x-xx),abs(y-yy)
   if not flat and dx>width//2:dx=width-dx
   distance=min(distance,max(dx,dy)-((dx+dy)//2-min(dx,dy)+1)//2)
  distance=-1 if distance==999 else distance;creche=int(bool(b[0x8c]&4));er=energy_expected([fresh[8],flows[2],commerce,grid,flows[6],distance,state('SE_effic_pending'),1,creche,*social])
  if er[0]:return fail(7 if er[0]==3 else 1)
  ar=proposed.model(text,data,0,er[4],commerce,[pop,mask,special,*types])
  if ar[0]:return fail(7 if ar[0]==3 else 1)
  pr=psych_world.model(text,data,special,ar[3])
  if pr['BASE']!=[0]:return fail(7 if pr['BASE']==[5] else 1)
  if base_index:
   pr['FACTS'][4]=base_index;psych=psych_world.leaf.expected(pr['FACTS'])
   if psych[0]:return fail(7 if psych[0]==5 else 1)
   pr['PSYCH']=psych[1:]
  return {'BASE':[0],'INTAKE':fresh,'MINERAL':m[1:],'NUTRIENT':nf[1:],'ENERGY':[distance,state('SE_effic_pending'),creche,*er[1:]],'ALLOCATION':ar[1:],'PSYCH':pr['PSYCH']}
 trimmed=0
 while True:
  result=evaluate(mask,special,types)
  if result['BASE']!=[0]:return result
  types=result['ALLOCATION'][8:10]
  if not canriot or not result['PSYCH'][-1] or not choices or result['MINERAL'][2]-yields[choices[-1]][1]<0:break
  mask&=~(1<<choices.pop());special+=1;trimmed+=1
 result['ASSIGN']=[pop,special,mask,*types,trimmed,trimmed,result['PSYCH'][-1]];return result
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,default=Path('/tmp/alpha-worker-resolution'));p.add_argument('--cc',default='g++');p.add_argument('--binary',type=Path);a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True);exe=a.binary or a.out/'probe';command=[]
 if not a.binary:
  command=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(n+'.cpp')) for n in SOURCES],'-o',str(exe)];subprocess.run(command,check=True)
 text=rules();cases=[(f'base-{i}',fixture(i,7+i%4),i%3,i%2,None,text) for i in range(1,17)]
 seed=fixture(8,9);cases.extend([(name,seed,0,0,name,text) for name in ['inspection','editor','control']]);cases.extend([('late-commerce-overflow',seed,2147483647,0,None,text),('negative-commerce',seed,-1,0,None,text),('SMAX',fixture(8,12),0,0,None,text)])
 high=worker.modified(seed,mask=1,pop=35,special=0);cases.append(('more-than16-specialists',high,0,0,None,text));over=worker.modified(seed,mask=0x1fffff,pop=2,special=0);cases.append(('overassigned-doctors',over,0,0,None,text))
 empty=worker.modified(seed,mask=0,pop=6,special=0);cases.append(('empty-mask-assign',empty,0,0,None,text))
 riots=bytearray(worker.modified(seed,mask=1,pop=8,special=0));_,riot_off,_=intake.decode(riots);p=riot_off['bases'];riots[p+0x8c:p+0x98]=bytes(12);riots[p+7:p+9]=bytes(2);struct.pack_into('<2I',riots,p+0x84,0,0);schema=intake.FACTIONS['LegacyFactionV10']
 for name,value in [('SE_talent_pending',0),('SE_police_pending',-2),('SE_alloc_psych',0),('SE_alloc_labs',5),('diff_level',5)]:
  field=next(f for f in schema['fields'] if f['name']==name);struct.pack_into('<i',riots,riot_off['factions']+0x20bc+field['offset'],value)
 for name,value in [('rule_drone',0),('rule_talent',0),('faction_bonus_count',0)]:
  field=next(f for f in intake.FACTIONS['MFaction']['fields'] if f['name']==name);struct.pack_into('<i',riots,riot_off['faction_definitions']+0x59c+field['offset'],value)
 for name,value,index in [('DiffLevel',5,0),*[('SecretProjects',-1,i) for i in range(33)]]:
  field=next(f for f in intake.SCHEMA['GamePrefs']['fields'] if f['name']==name);delta=field['offset']-(0x7c if field['offset']>=0x184 else 0)+index*struct.calcsize(intake.FORMATS[field['type']]);struct.pack_into('<i',riots,riot_off['legacy']+delta,value)
 cases.append(('iterative-riot-trim-preserves-technicians',bytes(riots),0,0,None,text))
 doctors=bytearray(riots);struct.pack_into('<2I',doctors,p+0x84,0x11111111,0x11111111);field=next(f for f in schema['fields'] if f['name']=='SE_support_pending');struct.pack_into('<i',doctors,riot_off['factions']+0x20bc+field['offset'],3);cases.append(('iterative-riot-trim-with-doctors',bytes(doctors),0,0,None,text))
 digest=hashlib.sha256();records=[]
 for name,data,commerce,grid,mode,case_rules in cases:
  path,rule=a.out/'input.SC',a.out/'rules.txt';path.write_bytes(data);rule.write_bytes(case_rules);run=subprocess.run([str(exe),str(rule),str(path),'0',str(commerce),str(grid),*([mode] if mode else [])],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert run.returncode==0,(name,run.returncode,run.stderr)
  actual={line.split()[0]:list(map(int,line.split()[1:])) for line in run.stdout.decode().splitlines()};expected=model(case_rules,data,commerce,grid,mode);assert actual==expected,(name,actual,expected)
  assert path.read_bytes()==data and rule.read_bytes()==case_rules;digest.update(run.stdout);(a.out/(name+'.SC')).write_bytes(data);(a.out/(name+'.rules.txt')).write_bytes(case_rules);(a.out/(name+'.expected.txt')).write_bytes(run.stdout);records.append(dict(name=name,commerce=commerce,grid=grid,mode=mode,accepted=actual['BASE']==[0],trimmed=actual.get('ASSIGN',[0]*7)[6],outputSHA256=hashlib.sha256(run.stdout).hexdigest()))
 receipt=dict(worlds=len(cases),accepted=sum(r['accepted'] for r in records),trimmedWorkers=sum(r['trimmed'] for r in records),outputSHA256=digest.hexdigest(),allInputsAndOutputsTransactional=True,command=command,records=records,native='UNRUN');assert receipt['accepted']>=15 and receipt['trimmedWorkers']>0
 (a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ('command','records')}))
if __name__=='__main__':main()
