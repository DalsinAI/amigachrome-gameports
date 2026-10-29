#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh vanilla research cost/contribution/acquisition proposals from owned wire facts."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
import test_world_stable_intake as intake
from test_units import sections,material
from test_research_progress import cost_expected,contribution_expected,SENTINEL
from technology_effects_reference import effects
HERE=Path(__file__).resolve().parent
NAMES='rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats research_progress world_research world_research_probe'.split()
def rules():
 t=sections(intake.rules('base'));r,c,_=intake.ROW['tech_discovery_rate_pct_std'];t['RULES'][r][c]='100';t['TECHNOLOGY'][4][8]=format(1|4|64|128|256,'b');return material(t)
def fixture(version,case):
 data=bytearray(intake.fixture('base'if version<11 else'expansion',version,case));_,off,_=intake.decode(data);legacy=version<11;pf=off['legacy'if legacy else'preferences'];fs=0x20bc if legacy else 0x20cc;schema=intake.FACTIONS['LegacyFactionV10'if legacy else'Faction'];ds=intake.FACTIONS['MFaction'];rng=random.Random(0x2E5EA2C4+case+version*8192)
 def state(owner,name,value,index=0):
  f=next(f for f in schema['fields']if f['name']==name);fmt=intake.FORMATS[f['type']];struct.pack_into('<'+fmt,data,off['factions']+owner*fs+f['offset']+index*struct.calcsize(fmt),value)
 def definition(owner,name,value,index=0):
  f=next(f for f in ds['fields']if f['name']==name);fmt=intake.FORMATS[f['type']];struct.pack_into('<'+fmt,data,off['faction_definitions']+owner*0x59c+f['offset']+index*struct.calcsize(fmt),value)
 def pref(name,value,index=0):
  f=next(f for f in intake.SCHEMA['GamePrefs']['fields']if f['name']==name);fmt=intake.FORMATS[f['type']];delta=f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0);struct.pack_into('<'+fmt,data,pf+delta+index*struct.calcsize(fmt),value)
 struct.pack_into('<i',data,off['player'],1);pref('CurrentFaction',1);pref('GamePreferences',0x20 if case%4==0 else 0);pref('CurrentTurn',case%40);pref('GameRules',0x20 if case%3==0 else 0);pref('FactionStatus',2)
 for i in range(89):pref('TechOwners',rng.choice([0,2,4,6]),i)
 pref('TechOwners',0 if case%2 else 4,4)
 for owner in range(1,8):
  for name,value in[('tech_ranking',rng.randrange(120)),('earned_techs_saved',rng.randrange(4)),('unk_27',rng.randrange(5)),('diff_level',case%6),('SE_research_base',rng.randrange(-4,5)),('SE_research_pending',case%13-6),('SE_economy_pending',case%9-3),('tech_research_id',4),('tech_cost',-7654321),('labs_total',-1234567),('tech_accumulated',-1234567),('net_random_event',-7654321),('tech_achieved',8),('player_flags',0)]:state(owner,name,value)
  definition(owner,'rule_techcost',[50,100,150][case%3]);definition(owner,'rule_commerce',case%4);definition(owner,'faction_bonus_count',3)
  for j in range(3):definition(owner,'faction_bonus_id',7+j,j);definition(owner,'faction_bonus_val1',-3+j,j)
 # Saved allocation/energy/cost/lab fields are deliberately not stage inputs.
 if case%5==0:data[off['bases']+0x8c+23//8]|=1<<(23%8)
 else:data[off['bases']+0x8c+23//8]&=~(1<<(23%8))
 if case==60:state(1,'tech_research_id',-1)
 if case==61:pref('TechOwners',2,4)
 if case==62:state(1,'tech_research_id',3)
 if case==63:state(1,'tech_research_id',78)
 if case==64:state(1,'tech_research_id',88)
 if case==65:pref('GameRules',0x200)
 if case==66:state(1,'player_flags',0x40)
 if case==67:pref('CurrentFaction',2)
 if case==68:pref('FactionStatus',0)
 if case==69:pref('GameRules',0x4000000)
 if case==70:state(2,'tech_ranking',-1)
 if case==71:state(1,'unk_27',-1)
 if case==72:state(1,'tech_achieved',2147483647)
 if case==73:state(1,'tech_ranking',2147483647)
 if case==74:definition(1,'rule_commerce',2147483647)
 if case==75:definition(1,'rule_techcost',2147483647)
 if case==76:state(1,'SE_research_pending',-2147483648)
 if case==77:pref('CurrentTurn',0)
 if case==78:pref('GamePreferences',0x20);pref('CurrentTurn',5)
 if case==80:pref('GameRules',0x200);state(1,'tech_research_id',-1)
 if case in[81,83,84]:pref('GameRules',0x200)
 if case in[82,83,85]:state(1,'player_flags',0x40)
 if case in[84,85]:pref('TechOwners',0,4)
 if case>=60 and case not in[77,78]:pref('CurrentTurn',30);pref('GamePreferences',0);state(1,'SE_research_pending',0)
 return bytes(data)
def stage(case):return[1+case%37,case%103,case%100,0 if case in[81,82,83,85]else 2147483000 if case>=60 else case*3]
def facts(text,data,owner):
 info,_,spans=intake.decode(data);legacy=int(info['version'])<11;fs=0x20bc if legacy else 0x20cc;schema=intake.FACTIONS['LegacyFactionV10'if legacy else'Faction'];pf=spans['legacy'if legacy else'preferences'];ds=intake.FACTIONS['MFaction']
 def pref(name,index=0):
  f=next(f for f in intake.SCHEMA['GamePrefs']['fields']if f['name']==name);fmt=intake.FORMATS[f['type']];delta=f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0);return struct.unpack_from('<'+fmt,pf,delta+index*struct.calcsize(fmt))[0]
 def state(o,name):return intake.scalar(spans['factions'][o*fs:(o+1)*fs],schema,name)
 def definition(o,name):return intake.scalar(spans['faction_definitions'][o*0x59c:(o+1)*0x59c],ds,name)
 t=sections(text);r,c,_=intake.ROW['tech_discovery_rate_pct_std'];rate=clamp(int(t['RULES'][r][c]),0,1000);cost=[owner,state(owner,'diff_level'),pref('CurrentTurn'),rate,definition(owner,'rule_techcost'),state(owner,'SE_research_base'),int(info['width'])//2*int(info['height']),*[state(o,'tech_ranking')for o in range(8)],*[state(o,'earned_techs_saved')for o in range(8)],state(owner,'unk_27'),int(bool(pref('GameRules')&0x20))]
 return info,spans,pref,state,definition,cost
def clamp(n,lo,hi):return max(lo,min(hi,n))
def model(text,data,base,s):
 info,_,spans=intake.decode(data);legacy=int(info['version'])<11;bs=304 if legacy else 308;owner=spans['bases'][base*bs+4];info,spans,pref,state,definition,cost=facts(text,data,owner);cost_failure=lambda e:[e]+[SENTINEL]*7;failure=lambda e:[e]+[SENTINEL]*26
 ce=cost_expected(cost)if legacy and pref('FactionStatus')&(1<<owner)else cost_failure(4)
 if not legacy or owner!=int(info['player'])or owner!=pref('CurrentFaction')or not pref('FactionStatus')&(1<<owner):return dict(cost=ce,research=failure(4))
 blind=int(bool(pref('GameRules')&0x200));target=state(owner,'tech_research_id');rank=state(owner,'tech_ranking');debt=state(owner,'unk_27');achieved=state(owner,'tech_achieved');sphere=bool(spans['bases'][base*bs+0x8c+23//8]&(1<<(23%8)));contribution=[s[0],state(owner,'SE_research_pending'),pref('CurrentTurn'),state(owner,'diff_level'),999999999,*s[1:],bool(pref('GameRules')&0x4000000),bool(pref('GamePreferences')&0x20),sphere];pre=contribution_expected(contribution,progress=False)
 # Conversion only never adds to accumulated and cannot complete a target.
 if pre[0]:return dict(cost=ce,research=failure(pre[0]))
 points=pre[5];pre[3]=s[3];pre[6]=0
 if not points:return dict(cost=ce,research=[*pre,target,target,rank,debt,achieved,0,0,0,0,0,0,0,0,0,0,blind,0,0,0])
 if target<0:return dict(cost=ce,research=failure(5))
 if 78<=target<=86 or target>=88:return dict(cost=ce,research=failure(4))
 table=sections(text)['TECHNOLOGY'];keys={r[1].lower():i for i,r in enumerate(table)};keys.update(none=-1,disable=-2)
 def known(i):
  if i==-1:return True
  if i<0 or i>=88:return False
  row=table[i];return row[6].lower()!='disable'and not(row[7].lower()=='disable'and row[6].lower()!='none')and bool(pref('TechOwners',i)&(1<<owner))
 row=table[target]
 if known(target)or any(keys[row[j].lower()]< -1 or not known(keys[row[j].lower()])for j in[6,7]):return dict(cost=ce,research=failure(1))
 if ce[0]:return dict(cost=ce,research=failure(ce[0]))
 contribution[4]=ce[1];result=contribution_expected(contribution)
 if result[0]:return dict(cost=ce,research=failure(result[0]))
 mask=pref('TechOwners',target);complete=result[6];pending=bool(state(owner,'player_flags')&0x40);acquire=int(bool(complete or pending));targetafter=target;effect=[0]*5;secrets=0;bonus_current=0;bonus_next=0
 if acquire:
  if achieved<0:return dict(cost=ce,research=failure(1))
  if max(achieved+1,rank+2)>2147483647:return dict(cost=ce,research=failure(3))
  targetafter=-1;achieved+=1;rank+=2
  if debt<0:return dict(cost=ce,research=failure(1))
  debt=max(0,debt-2);secrets=int(not mask and bool(int(row[8],2)&1));bonus_current=int(pending and not complete);bonus_next=int(bool(complete and(pending or secrets)));mask|=1<<owner;copy=bytearray(data);_,off,_=intake.decode(copy);field=next(f for f in intake.SCHEMA['GamePrefs']['fields']if f['name']=='TechOwners');at=off['legacy'if legacy else'preferences']+field['offset']-(0x7c if legacy and field['offset']>=0x184 else 0)+target;copy[at]=mask;error,effect=effects(text,bytes(copy),owner)
  if error:return dict(cost=ce,research=failure(3 if error==3 else 1))
 return dict(cost=ce,research=[*result,target,targetafter,rank,debt,achieved,mask,*effect,1,acquire,acquire,secrets,blind,int(bool(acquire and blind)),bonus_current,bonus_next])
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--binary',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.binary or a.out/'world-research-probe';cmd=[]
 if not a.binary:
  cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(n+'.cpp'))for n in NAMES],'-o',str(exe)];subprocess.run(cmd,check=True)
 text=rules();rule=a.out/'base.txt';rule.write_bytes(text);records=[]
 for version in range(7,13):
  for case in range(86 if version<11 else 3):
   data=fixture(version,case);path=a.out/f'v{version}-{case}.SC';path.write_bytes(data);s=stage(case);want=model(text,data,0,s);run=subprocess.run([str(exe),str(rule),str(path),'0',*map(str,s)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30);assert run.returncode==0,(path,run.returncode,run.stderr);got={line.split()[0]:list(map(int,line.split()[1:]))for line in run.stdout.splitlines()};assert got==want,(path,s,got,want);records.append(dict(name=path.name,stage=s,base=0,sha256=hashlib.sha256(data).hexdigest(),result=got,passed=True))
 receipt=dict(cases=len(records),command=cmd,casesDetail=records,native='UNRUN',scope='Explicit fresh scalar/ledger boundary; coupled upkeep helper required for freshness authority');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(records)} world original-cost and directed-research cases passed')
if __name__=='__main__':main()
