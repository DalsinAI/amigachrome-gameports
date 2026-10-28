#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh population proposal against independent wire, nutrient and growth models."""
import argparse, hashlib, json, os, struct, subprocess
from pathlib import Path
import test_world_stable_intake as intake
import test_world_convoy_nutrients as convoy
from test_base_nutrients import expected as nutrient
from test_cost_factor import expected as cost
from test_world_base_minerals import normalized
HERE=Path(__file__).resolve().parent
SOURCES='rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats vehicle_stack tile_yields world_technology_effects world_upkeep_tile_yields world_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients game_logic colony cost_factor world_cost_factor world_population world_population_probe'.split()

def decode(data):
 info,off,spans=intake.decode(data);legacy=int(info['version'])<11
 return info,off,spans,304 if legacy else 308,0x20bc if legacy else 0x20cc

def put(data,region,schema,name,value,index=0):
 info,off,_,_,fs=decode(data);field=next(f for f in schema['fields'] if f['name']==name);fmt=intake.FORMATS[field['type']]
 start=off[region]+(fs if region=='factions' else 0x59c if region=='faction_definitions' else 0)
 if region=='legacy' and field['offset']>=0x184:start-=0x7c
 struct.pack_into('<'+fmt,data,start+field['offset']+index*struct.calcsize(fmt),value)

def pref(data,name,value,index=0):
 put(data,'legacy' if int(decode(data)[0]['version'])<11 else 'preferences',intake.SCHEMA['GamePrefs'],name,value,index)

def fixture(case):
 data=bytearray(intake.fixture('base',7+case%4,1));info,off,_,_,_=decode(data);p=off['bases'];pop=1+case%5
 data[p+6]=pop;struct.pack_into('<I',data,p+0x78,(1<<(pop+1))-1);struct.pack_into('<i',data,p+0x3c,[-5,0,15,40,200][case//5%5]);struct.pack_into('<I',data,p+0x8c,0)
 state=intake.FACTIONS['LegacyFactionV10'];definition=intake.FACTIONS['MFaction']
 for name,value in [('SE_growth_pending',[-3,0,6][case//25%3]),('satellites_nutrient',0),('nutrient_surplus_total',17)]:put(data,'factions',state,name,value)
 put(data,'faction_definitions',definition,'rule_population',[-2,0,2][case//75%3]);pref(data,'SecretProjects',-1,27)
 if case%7==0:data[p+0x8c]|=4
 if case%11==0:data[p+0x8c+3]|=1 # HabComplex24
 if case%13==0:data[p+0x8c+3]|=2 # HabDome25
 if case%17==0:pref(data,'SecretProjects',0,13)
 if case%19==0:pref(data,'SecretProjects',0,24)
 for delta in [0xb0,0xc0,0xd0,0xe0,0xf0]:struct.pack_into('<i',data,p+delta-4,-87654321)
 return bytes(data)

def basic(text,name):
 row,col,limits=intake.ROW[name];value=int(intake.sections(text)['RULES'][row][col]);return max(limits[0],min(limits[1],value))

def readfacts(data,text):
 info,off,spans,bs,fs=decode(data);b=spans['bases'][:bs];owner=b[4];legacy=int(info['version'])<11
 schema=intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction'];f=spans['factions'][owner*fs:(owner+1)*fs]
 state=lambda name:intake.scalar(f,schema,name)
 prefs=spans['legacy' if legacy else 'preferences']
 def pref(name,index=0):
  field=next(x for x in intake.SCHEMA['GamePrefs']['fields'] if x['name']==name);offset=field['offset']-(0x7c if legacy and field['offset']>=0x184 else 0);fmt=intake.FORMATS[field['type']];return struct.unpack_from('<'+fmt,prefs,offset+index*struct.calcsize(fmt))[0]
 def project(i):
  p=pref('SecretProjects',i)
  return p>=0 and spans['bases'][p*bs+4]==owner
 built=lambda n:bool(b[0x8c+n//8]&(1<<(n%8)))
 fresh=intake.model('base',text,data)
 if fresh[0]:return fresh[0],None
 flowerror,flows=convoy.flow_model('base',text,data)
 if flowerror:return flowerror,None
 flags=int(built(2))|int(bool(struct.unpack_from('<I',b,0x30)[0]&4))<<1
 food=nutrient([b[6],state('SE_growth_pending'),fresh[7],flows[0],flows[4],basic(text,'nutrient_req_citizen'),struct.unpack_from('<i',b,0x3c)[0],state('nutrient_surplus_total'),0,flags])
 if food[0]:return 7 if food[0]==3 else 1,None
 growth=food[1];factor=cost([0,basic(text,'nutrient_cost_multi'),struct.unpack_from('<I',spans['metadata'],0xa30)[0],pref('DiffLevel'),state('SE_industry_pending'),state('SE_growth_pending'),1|16|int(built(2))*32|int(bool(flags&2))*64])
 if factor[0]:return 7 if factor[0]==4 else 3,None
 if factor[1]<1:return 3,None
 definition=spans['faction_definitions'][owner*0x59c:(owner+1)*0x59c];modifier=2*project(13)-intake.scalar(definition,intake.FACTIONS['MFaction'],'rule_population')
 limit=basic(text,'pop_limit_sans_hab_dome' if built(24) else 'pop_limit_sans_hab_complex')+modifier
 if not -(1<<31)<=limit<(1<<31):return 7,None
 event=struct.unpack_from('<I',b,0x34)[0];obj=bool(pref('GameRules')&0x20000000 or event&0x8000)
 if not obj and pref('GameRules')&0x40000000:obj=any(pref('SecretProjects',i)==0 for i in range(33))
 if not obj and pref('GameState')&0x4000000:obj=0<=pref('ScnVictFacilityObj')<34 and built(pref('ScnVictFacilityObj'))
 return 0,([b[6],food[5],food[4],factor[1],growth,min(255,max(-127,limit)),basic(text,'nutrient_req_citizen'),int(built(25)),int(project(24)),0,int(obj)],food[1:],flows)

def model(data,text,full,after=None,mode=None):
 if mode:return [3]
 if int(decode(data)[0]['version'])>=11:return [3]
 _,_,spans,_,_=decode(data)
 if not spans['bases'] or not 1<=spans['bases'][6]<=127 or not 1<=spans['bases'][4]<=7:return [1]
 error,row=readfacts(data,text)
 if error:return [error]
 facts,food,flows=row;facts[9]=int(full);pop,acc,surplus,factor,growth,limit,requirement,dome,clone,_,obj=facts
 p=pop;a=acc;recompute=append=removed=False;ceiling=-1;costbox=(pop+1)*factor;allowed=pop<limit or dome
 if acc>=0:
  if (growth>=6 or clone) and requirement and surplus>=requirement and allowed:p=min(127,pop+1);recompute=True;ceiling=costbox
  elif acc>=costbox and growth<=-3:a=costbox
  elif acc>=costbox and allowed:p=min(127,pop+1);a=0;recompute=append=True
  else:a=(costbox//2 if acc>=costbox else acc)+surplus
 else:
  if full and (pop>1 or not obj):p-=1;removed=p<=0
  if not removed:a=surplus
 if not -(1<<31)<=a<(1<<31):return [7]
 required=not removed and (recompute or p!=pop);starve=acc<0;cancellations=[0]*64;count=0
 info,_,spans,_,_=decode(data)
 if starve and full and flows[4]:
  for i in range(int(info['vehicles'])):
   v=spans['vehicles'][i*52:(i+1)*52];unit=struct.unpack_from('<h',v,10)[0];plan=struct.unpack_from('<b',spans['prototypes'],unit*52+42)[0];x,y=struct.unpack_from('<hh',v);items=struct.unpack_from('<I',spans['tiles'],44*((y*int(info['width'])+x)//2)+8)[0]
   if struct.unpack_from('<h',v,46)[0]==0 and plan==10 and v[17]==3 and not v[40] and items&1:cancellations[i//32]|=1<<(i%32);count+=1
 required=not removed and (required or count>0)
 if after is not None:
  if not required:return [1]
  _,off,_,_,_=decode(after)
  if after[off['bases']+6]!=p or struct.unpack_from('<i',after,off['bases']+0x3c)[0]!=a:return [1]
  normalized=bytearray(after)
  _,oldoff,_,_,_=decode(data)
  for delta,length in [(6,1),(0x3c,4),(0x78,4)]:normalized[off['bases']+delta:off['bases']+delta+length]=data[oldoff['bases']+delta:oldoff['bases']+delta+length]
  for i in range(int(info['vehicles'])):
   if cancellations[i//32]&(1<<(i%32)):
    if after[off['vehicles']+i*52+17]!=0:return [1]
    normalized[off['vehicles']+i*52+17]=data[oldoff['vehicles']+i*52+17]
  if bytes(normalized)!=data:return [1]
  error,row=readfacts(after,text)
  if error:return [error]
  _,food,_=row
  if recompute:
   a=food[4]
   if ceiling>=0:a=min(a,ceiling)
   if append:a+=food[3]
   if not -(1<<31)<=a<(1<<31):return [7]
   recompute=append=False;ceiling=-1
  required=False
 return [0,*facts,p,a,int(recompute),int(append),int(removed),ceiling,int(required),int(starve),count,*food,*cancellations]

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--out',type=Path,required=True);parser.add_argument('--cc',default='c++');parser.add_argument('--binary',type=Path);parser.add_argument('--original',type=Path);parser.add_argument('--original-rules',type=Path);args=parser.parse_args();args.out.mkdir(parents=True,exist_ok=False)
 exe=args.binary.resolve() if args.binary else args.out/'world-population-probe';cmd=[]
 if not args.binary:
  cmd=[args.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(s+'.cpp')) for s in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True)
 texts={}
 for foodvalue in [0,5]:
  t=intake.sections(intake.rules('base'))
  for r in t['RESOURCEINFO']:r[1]=str(foodvalue)
  for name,value in [('nutrient_req_citizen',2),('nutrient_cost_multi',10),('pop_limit_sans_hab_complex',4),('pop_limit_sans_hab_dome',7)]:r,c,_=intake.ROW[name];t['RULES'][r][c]=str(value)
  texts[foodvalue]=intake.material(t)
 records=[];cases=[]
 for case in range(225):cases.append((f'controlled-{case}',fixture(case),texts[0 if case&1 else 5],bool(case%3),None))
 for objective in ['all','event','projects','facility']:
  data=bytearray(fixture(0));_,off,_,_,_=decode(data);p=off['bases'];struct.pack_into('<i',data,p+0x3c,-1)
  if objective=='all':pref(data,'GameRules',0x20000000)
  if objective=='event':struct.pack_into('<I',data,p+0x34,0x8000)
  if objective=='projects':pref(data,'GameRules',0x40000000);pref(data,'SecretProjects',0,0)
  if objective=='facility':pref(data,'GameState',0x4000010);pref(data,'ScnVictFacilityObj',2);data[p+0x8c]|=4
  cases.append(('objective-'+objective,bytes(data),texts[0],True,None))
 for moved in [False,True]:
  data=bytearray(convoy.fixture('base',9,32,[(1,0,3,0,1,10),(1,0,3,2,1,10),(1,0,3,1,0,10)]));_,off,_,_,_=decode(data);struct.pack_into('<i',data,off['bases']+0x3c,-1)
  if moved:data[off['vehicles']+40]=1
  cases.append(('starvation-convoy-'+str(moved),bytes(data),texts[0],True,None))
 seed=fixture(25)
 cases.extend((name,seed,texts[5],True,mode) for name,mode in [('reject-upkeep','upkeep'),('reject-editor','editor')])
 if args.original:
  if not args.original_rules:parser.error('--original-rules required')
  # Query every owned current player base using the normal API; arbitrary IDs
  # are intentionally not normalized to base0 in a real original scenario.
  data=args.original.read_bytes();info,_,spans,bs,_=decode(data)
  for base in range(int(info['bases'])):cases.append(('original-base-'+str(base),data,args.original_rules.read_bytes(),True,None,base))
 digest=hashlib.sha256();accepted=finished=refused_stale=0
 def run(name,data,text,full,after=None,mode=None,base_id=0):
  src=args.out/'before.SC';rules=args.out/'rules.txt';src.write_bytes(data);rules.write_bytes(text);argv=[str(exe),str(rules),str(src),str(base_id),str(int(full))]
  if after is not None:(args.out/'after.SC').write_bytes(after);argv.append(str(args.out/'after.SC'))
  elif mode:argv.append('-')
  if mode:argv.append(mode)
  result=subprocess.run(argv,capture_output=True,timeout=30,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert result.returncode==0,(name,result.returncode,result.stdout,result.stderr)
  got=list(map(int,result.stdout.split()));want=model(normalized(data,base_id),text,full,normalized(after,base_id) if after is not None else None,mode);assert got==want,(name,got,want)
  assert src.read_bytes()==data and rules.read_bytes()==text
  digest.update(result.stdout);records.append(dict(name=name,accepted=got[0]==0,outputSHA256=hashlib.sha256(result.stdout).hexdigest()));return got
 for entry in cases:
  name,data,text,full,mode,*extra=entry;base_id=extra[0] if extra else 0
  got=run(name,data,text,full,mode=mode,base_id=base_id)
  if got[0]:continue
  accepted+=1
  if not got[18]:continue
  old=run(name+'-old-world-refused',data,text,full,after=data,base_id=base_id);assert old==[1];refused_stale+=1
  after=bytearray(data);_,off,_,bs,_=decode(after);p=off['bases']+base_id*bs;after[p+6]=got[12];struct.pack_into('<i',after,p+0x3c,got[13]);struct.pack_into('<I',after,p+0x78,(1<<(got[12]+1))-1)
  info,off,_,_,_=decode(after)
  for i in range(int(info['vehicles'])):
   if got[27+i//32]&(1<<(i%32)):after[off['vehicles']+i*52+17]=0
  complete=run(name+'-recompute',data,text,full,after=bytes(after),base_id=base_id);assert complete[0]==0 and not complete[18];finished+=1
  stale_mask=bytearray(after);stale_mask[p+0x78:p+0x7c]=data[p+0x78:p+0x7c];assert run(name+'-old-workers-refused',data,text,full,after=bytes(stale_mask),base_id=base_id)==[4]
  unrelated=bytearray(after);unrelated[p+0x40]^=1;assert run(name+'-unrelated-change-refused',data,text,full,after=bytes(unrelated),base_id=base_id)==[1]
 receipt=dict(worlds=len(cases),queries=len(records),acceptedBegin=accepted,recomputed=finished,oldPopulationWorldsRefused=refused_stale,outputSHA256=digest.hexdigest(),sourceUnchanged=True,hostSanitizers=True,native='UNRUN',command=cmd,records=records);(args.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
