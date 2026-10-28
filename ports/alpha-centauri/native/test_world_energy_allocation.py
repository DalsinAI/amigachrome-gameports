#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Allocation from current world sliders, citizens, facilities and project wire facts."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
import test_world_stable_intake as intake
from test_units import sections,material
from test_upkeep_energy_allocation import expected,SENTINEL
HERE=Path(__file__).resolve().parent
NAMES='rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats upkeep_energy_allocation world_energy_allocation world_energy_allocation_probe'.split()
def rules():
 table=sections(intake.rules('base'))
 r,c,_=intake.ROW['min_base_size_specialists'];table['RULES'][r][c]='5'
 table['CITIZENS']=[[f'Citizen{i}',f'Citizens{i}','None'if i<3 else f'T{i}','T8'if i<2 else'Disable',str(i%4),str((i*3)%5),str((i*5)%7)]for i in range(7)]+[['Drone','Drones'],['Worker','Workers'],['Talent','Talents']]
 return material(table)
def fixture(version,case):
 data=bytearray(intake.fixture('base'if version<11 else'expansion',version,case));_,offset,_=intake.decode(data);legacy=version<11;pf=offset['legacy'if legacy else'preferences'];fs=0x20bc if legacy else 0x20cc;schema=intake.FACTIONS['LegacyFactionV10'if legacy else'Faction'];rng=random.Random(0xA110+case+version*4096)
 def state(name,value,index=0):
  field=next(f for f in schema['fields']if f['name']==name);struct.pack_into('<'+intake.FORMATS[field['type']],data,offset['factions']+fs+field['offset']+index*struct.calcsize(intake.FORMATS[field['type']]),value)
 def pref(name,value,index=0):
  field=next(f for f in intake.SCHEMA['GamePrefs']['fields']if f['name']==name);delta=field['offset']-(0x7c if legacy and field['offset']>=0x184 else 0);struct.pack_into('<'+intake.FORMATS[field['type']],data,pf+delta+index*struct.calcsize(intake.FORMATS[field['type']]),value)
 labs=case%11;psych=(case//11)%(11-labs);state('SE_alloc_labs',labs);state('SE_alloc_psych',psych);state('SE_effic_pending',case%13-6);state('SE_Economics_pending',case%4)
 for i in range(89):pref('TechOwners',rng.choice([0,2]),i)
 pop=case%32;data[offset['bases']+6]=pop;struct.pack_into('<i',data,offset['bases']+0x7c,case%(pop+1));struct.pack_into('<2I',data,offset['bases']+0x84,rng.getrandbits(32),rng.getrandbits(32));data[offset['bases']+0x8c:offset['bases']+0x98]=bytes(12)
 for i in[7,8,9,11,13,14,15,16,17,18]:
  if rng.randrange(2):data[offset['bases']+0x8c+i//8]|=1<<(i%8)
 for i in range(33 if legacy else 64):pref('SecretProjects',-1,i)
 for i in[6,12,14,18,21,27]:pref('SecretProjects',0 if rng.randrange(2)else-1,i)
 if case==70:state('SE_alloc_labs',11)
 if case==71:state('SE_alloc_psych',11)
 if case==72:pref('FactionStatus',0)
 if case==73:struct.pack_into('<i',data,offset['bases']+0x7c,pop+1)
 if case==74:pref('SecretProjects',999,12)
 if case==75:pref('SecretProjects',-3,14)
 # Stale persisted resource/allocation caches cannot influence the proposal.
 for delta in[0xb8,0xc8,0xd8,0x100,0x104,0x108,0x10c]:struct.pack_into('<i',data,offset['bases']+delta-(4 if legacy and delta>=0x98 else 0),-7654321)
 return bytes(data)
def world_words(text,data,base,surplus,commerce):
 info,_,spans=intake.decode(data);version=int(info['version']);legacy=version<11;fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308;nb=int(info['bases']);schema=intake.FACTIONS['LegacyFactionV10'if legacy else'Faction'];prefs=spans['legacy'if legacy else'preferences'];table=sections(text)
 def pref(name,index=0):
  f=next(f for f in intake.SCHEMA['GamePrefs']['fields']if f['name']==name);delta=f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0);fmt=intake.FORMATS[f['type']];return struct.unpack_from('<'+fmt,prefs,delta+index*struct.calcsize(fmt))[0]
 def state(name):
  f=next(f for f in schema['fields']if f['name']==name);return struct.unpack_from('<'+intake.FORMATS[f['type']],spans['factions'],owner*fs+f['offset'])[0]
 if base>=nb:return None
 b=spans['bases'][base*bs:(base+1)*bs];owner=struct.unpack_from('<b',b,4)[0]
 if version>=11 or not 1<=owner<=7 or not pref('FactionStatus')&(1<<owner):return None
 pop=struct.unpack_from('<b',b,6)[0];count=struct.unpack_from('<i',b,0x7c)[0];types=struct.unpack_from('<2I',b,0x84);known=None
 def known(key):
  if key.lower()=='none':return True
  if key.lower()=='disable':return False
  i=next(i for i,r in enumerate(table['TECHNOLOGY'])if r[1].lower()==key.lower());row=table['TECHNOLOGY'][i]
  return i<88 and row[6].lower()!='disable'and not(row[7].lower()=='disable'and row[6].lower()!='none')and bool(pref('TechOwners',i)&(1<<owner))
 citizens=[];pre=[];obsolete=[];keys={r[1].lower():i for i,r in enumerate(table['TECHNOLOGY'])};keys.update(none=-1,disable=-2)
 for row in table['CITIZENS'][:7]:citizens.extend([keys[row[2].lower()],keys[row[3].lower()],*map(int,row[4:7])]);pre.append(int(known(row[2])));obsolete.append(int(known(row[3])))
 def built(record,id):return bool(record[0x8c+id//8]&(1<<(id%8)))
 flags=[built(b,i)for i in[9,7,8,11,17,18,13,14,15,16]];here=[];ours=[]
 for index in[6,12,14,18,21,27]:
  location=pref('SecretProjects',index)
  if location< -2 or location>=nb:return None
  here.append(location==base);ours.append(location>=0 and spans['bases'][location*bs+4]==owner)
 flags[3]=flags[3]or(ours[0]and flags[2]);flags.extend([here[1],here[3],here[5],here[2]and state('SE_Economics_pending')==1,here[4]])
 nodes=sum(built(spans['bases'][i*bs:(i+1)*bs],8)for i in range(nb));r,c,_=intake.ROW['min_base_size_specialists'];minimum=int(table['RULES'][r][c])
 return [surplus,state('SE_alloc_labs'),state('SE_alloc_psych'),state('SE_effic_pending'),pop,count,minimum,*types,*pre,*obsolete,7,*citizens,*map(int,flags),nodes,commerce]
def model(text,data,base,surplus,commerce):
 words=world_words(text,data,base,surplus,commerce);return expected(words)if words is not None else[1]+[SENTINEL]*13
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--binary',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.binary or a.out/'world-energy-allocation-probe';cmd=[]
 if not a.binary:
  cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(n+'.cpp'))for n in NAMES],'-o',str(exe)];subprocess.run(cmd,check=True)
 text=rules();rule=a.out/'base.txt';rule.write_bytes(text);records=[]
 for version in range(7,13):
  for case in range(80 if version<11 else 4):
   data=fixture(version,case);path=a.out/f'v{version}-{case}.SC';path.write_bytes(data);surplus=[-3,0,1,6,23,9999,2147483647][case%7];commerce=case%17;want=model(text,data,0,surplus,commerce);run=subprocess.run([str(exe),str(rule),str(path),'0',str(surplus),str(commerce)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30);assert run.returncode==0,(path,run.returncode,run.stderr);got=list(map(int,run.stdout.split()));assert got==want,(path,got,want);records.append(dict(name=path.name,sha256=hashlib.sha256(data).hexdigest(),result=got,passed=True))
 # Same parsed table semantics accept comments/CRLF; malformed counts/keys/numbers reject.
 sample=a.out/'v7-0.SC'
 for name,changed,valid in[('crlf',text.replace(b'\n',b'\r\n'),True),('unknown-key',text.replace(b'Citizen0,Citizens0,None,',b'Citizen0,Citizens0,UNKNOWN,'),False),('bad-number',text.replace(b'Citizen0,Citizens0,None,T8,0,',b'Citizen0,Citizens0,None,T8,x,'),False),('truncated',text[:text.index(b'Citizen6,')],False),('extra',text.replace(b'Talent,Talents\n',b'Talent,Talents\nExtra,Extras\n'),False)]:
  path=a.out/(name+'.txt');path.write_bytes(changed);run=subprocess.run([str(exe),str(path),str(sample),'0','6','0'],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30);assert run.returncode==(0 if valid else 20),(name,run.returncode,run.stderr)
 receipt=dict(cases=len(records),parserCases=5,command=cmd,casesDetail=records,native='UNRUN',policy='Thinker default energy allocation restoration');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(records)} world allocation cases and 5 parser cases passed')
if __name__=='__main__':main()
