#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Internal fresh upkeep labs joined to vanilla research, with atomic dual outputs."""
import argparse,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
SOURCES='rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats vehicle_stack tile_yields world_technology_effects world_upkeep_tile_yields world_tile_yields technology_effects world_fresh_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients upkeep_base_minerals world_base_minerals base_support world_base_support upkeep_base_energy upkeep_world_energy worker_allocation world_worker_upkeep upkeep_energy_allocation world_energy_allocation upkeep_base_psych world_base_psych cost_factor world_cost_factor world_worker_resolution world_upkeep_accounting research_progress world_research world_upkeep_research world_upkeep_research_probe'.split()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--source-root',type=Path,default=HERE);ap.add_argument('--cc',default='c++');ap.add_argument('--binary',type=Path);ap.add_argument('--original',type=Path);ap.add_argument('--original-rules',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);source=a.source_root.resolve();sys.path.insert(0,str(source))
 import test_world_worker_resolution as resolution
 import test_world_research as research
 import test_base_support as support
 from test_world_base_minerals import normalized
 intake=resolution.intake
 exe=a.binary.resolve()if a.binary else a.out/'world-upkeep-research-probe';command=[]
 if not a.binary:
  command=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(source/(n+'.cpp'))for n in SOURCES],'-o',str(exe)];subprocess.run(command,check=True)
 def prepared(data,case):
  data=bytearray(data);info,off,spans=intake.decode(data);legacy=int(info['version'])<11;bs=304 if legacy else 308;owner=spans['bases'][4];fs=0x20bc if legacy else 0x20cc;schema=intake.FACTIONS['LegacyFactionV10'if legacy else'Faction'];pf=off['legacy'if legacy else'preferences'];ds=intake.FACTIONS['MFaction']
  def pref(name,value,index=0):
   f=next(f for f in intake.SCHEMA['GamePrefs']['fields']if f['name']==name);fmt=intake.FORMATS[f['type']];delta=f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0);struct.pack_into('<'+fmt,data,pf+delta+index*struct.calcsize(fmt),value)
  def state(o,name,value):
   f=next(f for f in schema['fields']if f['name']==name);struct.pack_into('<'+intake.FORMATS[f['type']],data,off['factions']+o*fs+f['offset'],value)
  def definition(o,name,value):
   f=next(f for f in ds['fields']if f['name']==name);struct.pack_into('<'+intake.FORMATS[f['type']],data,off['faction_definitions']+o*0x59c+f['offset'],value)
  struct.pack_into('<i',data,off['player'],owner);pref('CurrentFaction',owner);pref('CurrentTurn',30);pref('GamePreferences',0);pref('TechOwners',pref_value(data,'TechOwners',4)&~(1<<owner),4)
  for o in range(1,8):state(o,'tech_ranking',10+o);state(o,'earned_techs_saved',0);state(o,'unk_27',0);definition(o,'rule_techcost',100)
  state(owner,'tech_research_id',4);state(owner,'SE_research_pending',case%11-5);state(owner,'SE_research_base',0);state(owner,'tech_achieved',7)
  for name in['tech_cost','labs_total','tech_accumulated','net_random_event']:state(owner,name,-987654321)
  if case==30:state(owner,'tech_research_id',-1)
  if case==31:definition(owner,'rule_techcost',2147483647)
  if case==32:pref('CurrentFaction',2 if owner!=2 else 1)
  if case==33:pref('GameRules',pref_value(data,'GameRules')|0x4000000);state(owner,'tech_research_id',-1)
  if case==34:pref('GamePreferences',0x20);pref('CurrentTurn',5)
  return bytes(data)
 def pref_value(data,name,index=0):
  info,_,spans=intake.decode(data);legacy=int(info['version'])<11;f=next(f for f in intake.SCHEMA['GamePrefs']['fields']if f['name']==name);fmt=intake.FORMATS[f['type']];at=f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0)+index*struct.calcsize(fmt);return struct.unpack_from('<'+fmt,spans['legacy'if legacy else'preferences'],at)[0]
 def model(text,data,base,commerce,grid,ledger,mode):
  info,_,spans=intake.decode(data)
  if base>=int(info['bases']):return{'BASE':[1]}
  mapped=normalized(data,base);r=resolution.model(text,mapped,commerce,grid,mode,base_index=base)
  mapping={1:1,2:2,3:4,4:8,5:1,6:7,7:3}
  if r['BASE']!=[0]:return{'BASE':[mapping[r['BASE'][0]]]}
  if r['ASSIGN'][-1]:return{'BASE':[6]}
  proposed=research.model(text,data,base,[r['ALLOCATION'][3],*ledger])
  if proposed['research'][0]:return{'BASE':[proposed['research'][0]]}
  r['INTAKE'][10]&=0xffffffff;r['INTAKE'][11]&=0xffffffff;supported=support.world_expected(mapped,text);assert supported[0]==0;r['SUPPORT']=supported[1:9];r['STATES']=[int(info['vehicles']),*supported[9:]];r['RESEARCH']=proposed['research'];computed=r['RESEARCH'][-8];r['COST']=[computed,*proposed['cost'][1:]]if computed else[0]*8
  cancel=[0]*64;cancel_count=0
  if r['ENERGY'][6]<0:
   bs=304 if int(info['version'])<11 else 308;owner=spans['bases'][base*bs+4];width=int(info['width'])
   for i in range(int(info['vehicles'])):
    v=spans['vehicles'][52*i:52*(i+1)];unit=struct.unpack_from('<h',v,10)[0];plan=struct.unpack_from('<b',spans['prototypes'],52*unit+42)[0]
    if v[14]!=owner or plan!=10 or v[17]!=3 or v[38]!=2:continue
    x,y=struct.unpack_from('<hh',v);tile=spans['tiles'][44*((y*width+x)//2):]
    if struct.unpack_from('<I',tile,8)[0]&1 and tile[2]&15<8:cancel[i//32]|=1<<(i%32);cancel_count+=1
  r['SHORTFALL']=[cancel_count,*cancel];return r
 tables=intake.sections(resolution.rules());row,col,_=intake.ROW['tech_discovery_rate_pct_std'];tables['RULES'][row][col]='100';text=intake.material(tables);cases=[]
 for case in range(1,35):cases.append((f'base-{case}',prepared(resolution.fixture(case%24+1,7+case%4),case),0,case%3,0,[17,99,case*3],None,text))
 seed=prepared(resolution.fixture(8,9),8)
 for mode in['inspection','editor','control']:cases.append(('mode-'+mode,seed,0,0,0,[17,99,0],mode,text))
 for name,ledger in[('bad-remainder',[17,100,0]),('negative-accumulation',[17,0,-1]),('late-ledger-overflow',[2147483647,99,2147483647])]:cases.append((name,seed,0,0,0,ledger,None,text))
 for name,commerce,grid in[('late-energy-overflow',2147483647,0),('negative-commerce',-1,0),('negative-grid',0,-1)]:cases.append((name,seed,0,commerce,grid,[17,99,0],None,text))
 for name,mask,pop,special in[('more-than16-specialists',1,45,0),('overassigned',0x1fffff,2,0),('empty-mask',0,6,0),('all-specialists',1,8,8)]:cases.append((name,prepared(resolution.worker.modified(seed,mask=mask,pop=pop,special=special),8),0,0,0,[17,99,0],None,text))
 if bool(a.original)!=bool(a.original_rules):raise ValueError('Both --original and --original-rules required')
 if a.original:
  original=a.original.read_bytes();actual=a.original_rules.read_bytes();info,_,spans=intake.decode(original);legacy=int(info['version'])<11;fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308;schema=intake.FACTIONS['LegacyFactionV10'if legacy else'Faction']
  for base in[9,10]:
   owner=spans['bases'][base*bs+4];f=spans['factions'][owner*fs:(owner+1)*fs];ledger=[0,intake.scalar(f,schema,'net_random_event'),intake.scalar(f,schema,'tech_accumulated')];cases.append((f'actual-{base}',original,base,0,0,ledger,None,actual))
 records=[]
 for index,(name,data,base,commerce,grid,ledger,mode,text)in enumerate(cases):
  path=a.out/f'{index}.SC';path.write_bytes(data);rule=a.out/f'{index}.txt';rule.write_bytes(text);want=model(text,data,base,commerce,grid,ledger,mode);argv=[str(exe),str(rule),str(path),str(base),str(commerce),str(grid),*map(str,ledger)]+([mode]if mode else[]);run=subprocess.run(argv,capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert run.returncode==0,(name,run.returncode,run.stdout,run.stderr);got={line.split()[0]:list(map(int,line.split()[1:]))for line in run.stdout.splitlines()};assert got==want,(name,got,want);records.append(dict(name=name,scenario=path.name,rules=rule.name,base=base,commerce=commerce,grid=grid,ledger=ledger,mode=mode,sha256=hashlib.sha256(data).hexdigest(),result=got,passed=True))
 receipt=dict(cases=len(records),accepted=sum(r['result']['BASE']==[0]for r in records),command=command,casesDetail=records,native='UNRUN',scope='Internally resolved fresh stage1 labs; unresolved riots fail atomically; acquisition/selection events remain required');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(records)} internally fresh upkeep research cases passed')
if __name__=='__main__':main()
