#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent wire/rank/trade oracle; no saved base energy or commerce cache use."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
import test_world_stable_intake as intake
from test_original_world import valid_fixture
from test_units import sections,material
from test_base_commerce import expected as partner_expected,pack,SENTINEL
from technology_effects_reference import effects
HERE=Path(__file__).resolve().parent
NAMES='rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats world_technology_effects base_commerce world_base_commerce world_base_commerce_probe'.split()
def rules():
 t=sections(intake.rules('base'))
 for i,row in enumerate(t['TECHNOLOGY']):row[8]=format(4|(i%2),'b')if i%4==0 or i<4 else'0'
 return material(t)
def fixture(version,case):
 count=512 if case==40 else 2+case%19;data=bytearray(valid_fixture(version,0,count));_,off,_=intake.decode(data);legacy=version<11;pf=off['legacy'if legacy else'preferences'];fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308;schema=intake.FACTIONS['LegacyFactionV10'if legacy else'Faction'];rng=random.Random(0x7CADE+case+version*4096)
 def state(owner,name,value,index=0):
  f=next(f for f in schema['fields']if f['name']==name);fmt=intake.FORMATS[f['type']];struct.pack_into('<'+fmt,data,off['factions']+owner*fs+f['offset']+index*struct.calcsize(fmt),value)
 def pref(name,value,index=0):
  f=next(f for f in intake.SCHEMA['GamePrefs']['fields']if f['name']==name);fmt=intake.FORMATS[f['type']];delta=f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0);struct.pack_into('<'+fmt,data,pf+delta+index*struct.calcsize(fmt),value)
 pref('FactionStatus',0xfe);pref('GovernorFaction',case%9-1);pref('ProposalPassCount',case%3,3);pref('ProposalPassCount',(case//3)%3,4)
 for i in range(89):pref('TechOwners',rng.randrange(256),i)
 for i in range(count):
  data[off['bases']+i*bs+4]=1+(i%7);struct.pack_into('<i',data,off['bases']+i*bs+0xb8,-1234567)
 for owner in range(1,8):
  state(owner,'SE_economy_pending',rng.randrange(-2,7));state(owner,'sanction_turns',1 if rng.randrange(6)==0 else 0);state(owner,'base_count',-7654321);state(owner,'tech_commerce_bonus',-1234567)
  f=next(f for f in intake.FACTIONS['MFaction']['fields']if f['name']=='rule_commerce');struct.pack_into('<i',data,off['faction_definitions']+owner*0x59c+f['offset'],rng.randrange(4));f=next(f for f in intake.FACTIONS['MFaction']['fields']if f['name']=='faction_bonus_count');struct.pack_into('<i',data,off['faction_definitions']+owner*0x59c+f['offset'],0)
 for owner in range(1,8):
  for other in range(owner+1,8):
   flags=rng.choice([0,2,3]);state(owner,'diplo_status',flags,other);state(other,'diplo_status',flags,owner)
 energies=[rng.randrange(8)*3 for _ in range(count)]
 # Faults late enough to prove output/sources/energy vector remain unchanged.
 if case==34:energies[0]=-1
 if case==35:energies[-1]=4194304
 if case==36:pref('GovernorFaction',8)
 if case==37:energies.pop()
 if case==38:pref('FactionStatus',0)
 if case==39:
  for owner in range(1,8):state(owner,'sanction_turns',0)
  state(1,'diplo_status',2,2);state(2,'diplo_status',0,1)
 return bytes(data),energies
def model(text,data,base,energy):
 info,_,spans=intake.decode(data);legacy=int(info['version'])<11;fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308;n=int(info['bases']);pf=spans['legacy'if legacy else'preferences'];schema=intake.FACTIONS['LegacyFactionV10'if legacy else'Faction'];fail=lambda code:[code]+[SENTINEL]*35
 def pref(name,index=0):
  f=next(f for f in intake.SCHEMA['GamePrefs']['fields']if f['name']==name);fmt=intake.FORMATS[f['type']];delta=f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0);return struct.unpack_from('<'+fmt,pf,delta+index*struct.calcsize(fmt))[0]
 def state(owner,name,index=0):
  f=next(f for f in schema['fields']if f['name']==name);fmt=intake.FORMATS[f['type']];return struct.unpack_from('<'+fmt,spans['factions'],owner*fs+f['offset']+index*struct.calcsize(fmt))[0]
 if len(energy)!=n or not 0<=base<n:return fail(1)
 owners=[spans['bases'][i*bs+4]for i in range(n)];owner=owners[base]
 if not 1<=owner<=7:return fail(1)
 if not legacy or not pref('FactionStatus')&(1<<owner):return fail(4)
 governor=pref('GovernorFaction')
 if not -1<=governor<=7 or min(energy)<0:return fail(1)
 scores=[value*512+i for i,value in enumerate(energy)]
 if max(scores)>2147483647:return fail(3)
 table=sections(text);tech_count=sum(row[6].lower()!='disable'and row[7].lower()!='disable'and bool(int(row[8],2)&4)for row in table['TECHNOLOGY']);rank=sorted([i for i in range(n)if owners[i]==owner],key=lambda i:scores[i],reverse=True).index(base);partners=[-1]*8;imports=[0]*8;exports=[0]*8;bonuses=[0]*8;total=0;globaltrade=pref('ProposalPassCount',4)<pref('ProposalPassCount',3);ours=False
 if not state(owner,'sanction_turns'):
  for other in range(1,8):
   ranked=sorted([i for i in range(n)if owners[i]==other],key=lambda i:scores[i],reverse=True)
   if other==owner or state(other,'sanction_turns')or not state(owner,'diplo_status',other)&2 or rank>=len(ranked):continue
   if not state(other,'diplo_status',owner)&2:return fail(1)
   partner=ranked[rank]
   if not ours:
    effect=effects(text,data,owner)
    if effect[0]:return fail(3 if effect[0]==3 else 1)
    bonuses[owner]=effect[1][-1];ours=True
   effect=effects(text,data,other)
   if effect[0]:return fail(3 if effect[0]==3 else 1)
   bonuses[other]=effect[1][-1];r=partner_expected([energy[base],energy[partner],tech_count,bonuses[owner],bonuses[other],globaltrade,bool(state(owner,'diplo_status',other)&1),governor==owner,governor==other])
   if r[0]:return fail(r[0])
   partners[other]=partner;imports[other]=r[2];exports[other]=r[3];total+=r[2]
   if total>2147483647:return fail(3)
 return [0,rank,tech_count,total,*partners,*imports,*exports,*bonuses]
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--binary',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.binary or a.out/'world-base-commerce-probe';cmd=[]
 if not a.binary:
  cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(n+'.cpp'))for n in NAMES],'-o',str(exe)];subprocess.run(cmd,check=True)
 text=rules();rule=a.out/'base.txt';rule.write_bytes(text);records=[]
 for version in range(7,13):
  for case in range(41 if version<11 else 3):
   data,energy=fixture(version,case);path=a.out/f'v{version}-{case}.SC';path.write_bytes(data);energies=a.out/f'v{version}-{case}.bin';energies.write_bytes(pack([len(energy),*energy]));base=0;want=model(text,data,base,energy);run=subprocess.run([str(exe),str(rule),str(path),str(energies),str(base)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert run.returncode==0,(path,run.returncode,run.stderr);got=list(map(int,run.stdout.split()));assert got==want,(path,got,want);records.append(dict(name=path.name,energyFile=energies.name,base=base,sha256=hashlib.sha256(data).hexdigest(),result=got,passed=True))
 receipt=dict(cases=len(records),command=cmd,casesDetail=records,native='UNRUN',scope='Fresh all-base vector; actual world trade/ranks/technology, no saved energy use');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(records)} world commerce cases passed')
if __name__=='__main__':main()
