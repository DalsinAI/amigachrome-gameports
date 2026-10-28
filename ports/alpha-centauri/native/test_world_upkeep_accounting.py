#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Stage1 accounting join vs independent resolved-worker and actor-state models."""
import argparse,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
SOURCES='rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats vehicle_stack tile_yields world_technology_effects world_upkeep_tile_yields world_tile_yields technology_effects world_fresh_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients upkeep_base_minerals world_base_minerals base_support world_base_support upkeep_base_energy upkeep_world_energy worker_allocation world_worker_upkeep upkeep_energy_allocation world_energy_allocation upkeep_base_psych world_base_psych cost_factor world_cost_factor world_worker_resolution'.split()

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--out',type=Path,required=True);parser.add_argument('--source-root',type=Path,default=HERE);parser.add_argument('--cc',default='c++');parser.add_argument('--binary',type=Path);parser.add_argument('--original',type=Path);parser.add_argument('--original-rules',type=Path);args=parser.parse_args();args.out.mkdir(parents=True,exist_ok=False)
 sys.path.insert(0,str(args.source_root.resolve()))
 import test_world_worker_resolution as resolution
 import test_base_support as support
 from test_world_base_minerals import normalized
 intake=resolution.intake
 exe=args.binary.resolve() if args.binary else args.out/'world-upkeep-accounting-probe';command=[]
 if not args.binary:
  command=[args.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie','-I',str(args.source_root),*[str(args.source_root/(n+'.cpp')) for n in SOURCES],str(HERE/'world_upkeep_accounting.cpp'),str(HERE/'world_upkeep_accounting_probe.cpp'),'-o',str(exe)];subprocess.run(command,check=True)
 def model(text,data,base,commerce,grid,mode):
  info,_,spans=intake.decode(data)
  if base>=int(info['bases']):return {'BASE':[1]}
  renumbered=normalized(data,base)
  result=resolution.model(text,renumbered,commerce,grid,mode,base_index=base)
  if result['BASE']!=[0]:return result
  result['INTAKE'][10]&=0xffffffff;result['INTAKE'][11]&=0xffffffff
  supported=support.world_expected(renumbered,text);assert supported[0]==0
  result['SUPPORT']=supported[1:9]
  result['STATES']=[int(info['vehicles']),*supported[9:]]
  cancellation=[0]*64;count=0
  if result['ENERGY'][6]<0:
   bs=304 if int(info['version'])<11 else 308;owner=spans['bases'][base*bs+4];width=int(info['width'])
   for index in range(int(info['vehicles'])):
    v=spans['vehicles'][52*index:52*(index+1)];unit=struct.unpack_from('<h',v,10)[0];plan=struct.unpack_from('<b',spans['prototypes'],52*unit+42)[0]
    if v[14]!=owner or plan!=10 or v[17]!=3 or v[38]!=2:continue
    x,y=struct.unpack_from('<hh',v);tile=spans['tiles'][44*((y*width+x)//2):]
    if struct.unpack_from('<I',tile,8)[0]&1 and tile[2]&15<8:cancellation[index//32]|=1<<(index%32);count+=1
  result['SHORTFALL']=[count,*cancellation]
  return result
 text=resolution.rules();cases=[(f'base-{i}',resolution.fixture(i,7+i%4),0,i%3,i%2,None,text) for i in range(1,25)]
 seed=resolution.fixture(8,9)
 for mode in ['inspection','editor','control']:cases.append(('mode-'+mode,seed,0,0,0,mode,text))
 cases.extend([('late-energy-overflow',seed,0,2147483647,0,None,text),('negative-commerce',seed,0,-1,0,None,text),('negative-grid',seed,0,0,-1,None,text),('SMAX',resolution.fixture(8,12),0,0,0,None,text),('bad-base',seed,999,0,0,None,text)])
 for name,mask,pop,special in [('more-than16-specialists',1,45,0),('overassigned',0x1fffff,2,0),('empty-mask',0,6,0),('post-growth-population',1,9,0),('post-loss-population',1,1,0),('all-specialists',1,8,8)]:cases.append((name,resolution.worker.modified(seed,mask=mask,pop=pop,special=special),0,0,0,None,text))
 # A directed base with existing Technician nibbles needs iterative worker
 # trimming; unsolved unrest must remain an explicit event requirement.
 riots=bytearray(resolution.worker.modified(seed,mask=1,pop=8,special=0));_,off,_=intake.decode(riots);base=off['bases'];riots[base+0x8c:base+0x98]=bytes(12);riots[base+7:base+9]=bytes(2);struct.pack_into('<2I',riots,base+0x84,0,0);schema=intake.FACTIONS['LegacyFactionV10']
 for name,value in [('SE_talent_pending',0),('SE_police_pending',-2),('SE_alloc_psych',0),('SE_alloc_labs',5),('diff_level',5)]:
  field=next(f for f in schema['fields'] if f['name']==name);struct.pack_into('<i',riots,off['factions']+0x20bc+field['offset'],value)
 for name,value in [('rule_drone',0),('rule_talent',0),('faction_bonus_count',0)]:
  field=next(f for f in intake.FACTIONS['MFaction']['fields'] if f['name']==name);struct.pack_into('<i',riots,off['faction_definitions']+0x59c+field['offset'],value)
 for name,value,index in [('DiffLevel',5,0),*[('SecretProjects',-1,i) for i in range(33)]]:
  field=next(f for f in intake.SCHEMA['GamePrefs']['fields'] if f['name']==name);delta=field['offset']-(0x7c if field['offset']>=0x184 else 0)+index*struct.calcsize(intake.FORMATS[field['type']]);struct.pack_into('<i',riots,off['legacy']+delta,value)
 cases.append(('iterative-riot-trim',bytes(riots),0,0,0,None,text))
 # Cache poison includes displayed intake/surplus, ecology, psych/allocation and
 # drone/talent totals. Stored food, live population, assignment and faction
 # aggregates are real inputs and are deliberately left intact.
 for name,value in [('old-displays-positive',123456789),('old-displays-negative',-123456789)]:
  changed=bytearray(seed);_,off,_=intake.decode(changed);base=off['bases']
  for delta in [0x48,0x98,0xb0,0xb4,0xb8,0xbc,0xc0,0xc4,0xc8,0xcc,0xd0,0xd4,0xd8,0xdc,0xe0,0xe4,0xe8,0xec,0xf0,0xf4,0xf8,0xfc,0x100,0x104,0x108,0x10c,0x118,0x11c,0x120]:struct.pack_into('<i',changed,base+delta-(4 if delta>=0x98 else 0),value)
  cases.append((name,bytes(changed),0,0,0,None,text))
 for name,field,value in [('late-mineral-overflow','clean_minerals_modifier',2147483647),('late-food-aggregate-overflow','nutrient_surplus_total',2147483647),('late-psych-invalid','SE_talent_pending',101)]:
  changed=bytearray(seed);_,off,_=intake.decode(changed);schema=intake.FACTIONS['LegacyFactionV10'];at=next(f['offset'] for f in schema['fields'] if f['name']==field);struct.pack_into('<i',changed,off['factions']+0x20bc+at,value)
  case_rules=text
  if field=='nutrient_surplus_total':
   tables=intake.sections(text)
   for row in tables['RESOURCEINFO']:row[1]='4'
   case_rules=intake.material(tables)
  cases.append((name,bytes(changed),0,0,0,None,case_rules))
 # Real energy-export orders produce a negative freshly computed surplus.
 # Specialist output may still exist; original shortfall cancellation uses the
 # pre-allocation energy result and must not be replaced by economy income.
 shortfall=bytearray(seed);info,off,_=intake.decode(shortfall);schema=intake.FACTIONS['LegacyFactionV10'];field=next(f['offset'] for f in schema['fields'] if f['name']=='satellites_energy');struct.pack_into('<i',shortfall,off['factions']+0x20bc+field,0);shortfall[off['bases']+0x8c]&=~2
 if int(info['vehicles'])>6:shortfall[off['vehicles']+6*52+17]=0
 tables=intake.sections(text)
 for row in tables['RESOURCEINFO']:row[3]='0'
 cases.append(('energy-shortfall',bytes(shortfall),0,0,0,None,intake.material(tables)))
 # No actors qualifies a null/zero support-state output as well as a tail-only
 # caller buffer; physical stack markers are removed together with actors.
 no_actors=bytearray(seed);info,off,_=intake.decode(no_actors);count=int(info['vehicles']);del no_actors[off['vehicles']:off['vehicles']+count*52];struct.pack_into('<i',no_actors,off['legacy']+0x38,0)
 for i in range(int(info['width'])*int(info['height'])//2):
  at=off['tiles']+44*i+8;items=struct.unpack_from('<I',no_actors,at)[0];struct.pack_into('<I',no_actors,at,items&~2)
 cases.append(('no-actors',bytes(no_actors),0,0,0,None,text))
 if args.original:
  if not args.original_rules:parser.error('--original-rules required')
  data=args.original.read_bytes();rules=args.original_rules.read_bytes();info,_,spans=intake.decode(data);bs=304 if int(info['version'])<11 else 308
  for base in range(int(info['bases'])):cases.append(('Boreholes-base-'+str(base),data,base,0,0,None,rules))
 records=[];digest=hashlib.sha256();cache_outputs={}
 for index,(name,data,base,commerce,grid,mode,rules) in enumerate(cases):
  src,rule=args.out/'input.SC',args.out/'rules.txt';src.write_bytes(data);rule.write_bytes(rules)
  run=subprocess.run([str(exe),str(rule),str(src),str(base),str(commerce),str(grid),*([mode] if mode else [])],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  assert run.returncode==0,(name,run.returncode,run.stdout,run.stderr)
  actual={parts[0]:list(map(int,parts[1:])) for parts in [line.split() for line in run.stdout.decode().splitlines()]};expected=model(rules,data,base,commerce,grid,mode)
  assert actual==expected,(name,actual,expected)
  if name in ('late-mineral-overflow','late-food-aggregate-overflow','late-energy-overflow'):assert actual['BASE']==[7],(name,actual)
  assert src.read_bytes()==data and rule.read_bytes()==rules
  digest.update(run.stdout);(args.out/(name+'.SC')).write_bytes(data);(args.out/(name+'.rules.txt')).write_bytes(rules);(args.out/(name+'.expected.txt')).write_bytes(run.stdout)
  records.append(dict(name=name,base=base,commerce=commerce,grid=grid,mode=mode,accepted=actual['BASE']==[0],actorCount=len(actual.get('STATES',[]))-1 if actual['BASE']==[0] else 0,trimmedWorkers=actual.get('ASSIGN',[0]*7)[6],requiresRiotEvent=bool(actual.get('ASSIGN',[0]*8)[7]),outputSHA256=hashlib.sha256(run.stdout).hexdigest()))
  if name=='more-than16-specialists':assert actual['ASSIGN'][1]>16,(name,actual)
  if name=='iterative-riot-trim':assert actual['ASSIGN'][6]>0,(name,actual)
  if name=='energy-shortfall':assert actual['ENERGY'][6]<0 and actual['SHORTFALL'][0]>0,(name,actual)
  if name=='Boreholes-base-9':assert actual['BASE']==[0] and actual['MINERAL'][2]==4 and actual['NUTRIENT'][3]==2 and actual['ALLOCATION'][1]==5 and actual['ALLOCATION'][3]==5
  if name=='Boreholes-base-10':assert actual['BASE']==[0]
  if name=='post-growth-population':assert actual['NUTRIENT'][2]>=18
  if name=='post-loss-population':assert actual['NUTRIENT'][2]>=2
  if name.startswith('old-displays-'):cache_outputs[name]=run.stdout
 assert cache_outputs['old-displays-positive']==cache_outputs['old-displays-negative']
 receipt=dict(worlds=len(cases),accepted=sum(r['accepted'] for r in records),trimmedWorkers=sum(r['trimmedWorkers'] for r in records),unresolvedRiotEvents=sum(r['requiresRiotEvent'] for r in records),outputSHA256=digest.hexdigest(),allWorldRulesStagesAndBothOutputsTransactional=True,allSourceRegionsAliasRejected=True,nullShortCapacityCrossAliasAndBufferTailGuards=True,cachePoisonInvariant=True,actualStage1Only=True,currentPopulationRecomputed=True,command=command,records=records,native='UNRUN')
 assert receipt['accepted']>=25 and receipt['trimmedWorkers']>0,receipt
 (args.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
