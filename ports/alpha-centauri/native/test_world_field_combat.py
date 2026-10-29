#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent raw-container infantry strength and complete round integration."""
import argparse, hashlib, json, os, re, struct, subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode, FORMATS
from test_original_faction import SCHEMA
from test_units import fixture as unit_fixture,material
from test_combat_rounds import expected as round_model
from test_combat_crt import draws as crt_draws
HERE=Path(__file__).resolve().parent

def rules_fixture(case):
 tables=unit_fixture('base');basic=json.loads((HERE/'basic_fixture.json').read_text())
 tables['RULES']=[r[:]for r in basic['rules_rows'][:-2]];tables['WORLDBUILDER']=basic['world_rows']
 fields=re.findall(r'^RULE_ROW\(([^,]+),', (HERE/'basic_fields.inc').read_text(),re.M)
 params=dict(move_rate_roads=3,combat_pct_atk_road=25,combat_pen_pct_atk_lwr_elev=50,combat_pct_atk_higher_elev=25,combat_pct_fanatic_atk_bonus=25,combat_pct_def_range_sensor=25)
 for name,val in params.items():tables['RULES'][fields.index(name)][0]=str(val)
 tables['CHASSIS'][0][8:12]=['1','0','0','0'];tables['WEAPONS'][0][2:4]=[str(1+case%8),'0'];tables['DEFENSES'][0][2]=str(1+case%5)
 return material(tables)

def put_field(data,start,schema,name,value,index=0):
 f=next(f for f in schema['fields']if f['name']==name);fmt=FORMATS[f['type']]
 struct.pack_into('<'+fmt,data,start+f['offset']+index*struct.calcsize(fmt),value)

def fixture(case,bases=0):
 data=bytearray(valid_fixture(9,2,bases));_,off,spans=decode(data);pref=off['legacy']
 data[pref+0x58]=1<<5|((1<<6)if case%3==0 else 0)
 for i in range(33):struct.pack_into('<i',data,pref+0x84+4*i,-1)
 fstate=SCHEMA['LegacyFactionV10'];fdef=SCHEMA['MFaction'];fs=0x20bc
 meta=dict(case=case,ar=1+case%4,dr=1+(case//4)%4,ad=case%6,dd=(case//2)%6,am=case%6,dm=(case//6)%6,sea=case%3==0,se_attack=[-3,0,3][case%3],se_def=[-4,0,4][case//3%3],fanatic=bool(case&4),ob=150 if case&8 else 100,db=125 if case&16 else 100,roads=bool(case&1),aa=3+(case%3==1),ba=3+(case%3==2),rock=case%3==1,forest=case%3==2,spent=case%3,difficulty=case%6)
 for faction in range(8):
  start=off['factions']+faction*fs;data[start:start+fs]=bytes(fs)
  start=off['faction_definitions']+faction*0x59c;data[start:start+0x59c]=bytes(0x59c)
 for faction,other,morale in [(5,6,meta['se_attack']),(6,5,meta['se_def'])]:
  p=off['factions']+faction*fs;put_field(data,p,fstate,'SE_morale',morale);put_field(data,p,fstate,'diff_level',meta['difficulty']);put_field(data,p,fstate,'diplo_status',0x10,other)
  p=off['faction_definitions']+faction*0x59c;put_field(data,p,fdef,'faction_bonus_count',1);put_field(data,p,fdef,'faction_bonus_id',18 if faction==5 else 17);put_field(data,p,fdef,'faction_bonus_val1',meta['ob']if faction==5 else meta['db'])
  if faction==5:put_field(data,p,fdef,'rule_flags',0x400 if meta['fanatic']else 0)
 for i in range(2):
  p=off['vehicles']+52*i;data[p:p+52]=bytes(52);struct.pack_into('<hh',data,p,2+2*i,2);struct.pack_into('<h',data,p+10,320+i);data[p+14]=5+i
  data[p+16]=meta['dd'if i else'ad'];data[p+36]=meta['dm'if i else'am'];data[p+39]=255;data[p+40]=meta['spent']if not i else 0;struct.pack_into('<hhh',data,p+46,-1,-1,-1)
  p=off['prototypes']+52*(320+i);data[p:p+52]=bytes(52);data[p+39]=meta['dr'if i else'ar'];data[p+42]=3
 for i in range(24):
  p=off['tiles']+44*i;data[p:p+44]=bytes(44);data[p]=0x60;data[p+2]=15;data[p+3]=1;data[p+4]=255;data[p+7]=255
 for x,y,alt in [(2,2,meta['aa']),(4,2,meta['ba'])]:
  p=off['tiles']+44*((8*y+x)//2);data[p]=alt<<5;struct.pack_into('<I',data,p+8,2|(4 if meta['roads']else 0)|(0x200000 if x==4 and meta['forest']else 0));data[p+2]=(5 if x==2 else 6)
  if x==4:data[p+5]=0x80 if meta['rock']else 0
 for i in range(bases):
  p=off['bases']+304*i;data[p:p+304]=bytes(304);struct.pack_into('<hh',data,p,6 if i else 0,4);data[p+4]=6
 return bytes(data),meta

def morale(raw,se):
 modifier=max(-4,min(4,se));modifier+=1 if modifier<=-2 else -1 if modifier>=2 else 0
 return max(0,min(6,raw+modifier))

def model(m):
 am=morale(m['am'],m['se_attack']);dm=morale(m['dm'],m['se_def'])
 speed=3+(3 if am==6 else 0)
 if m['ad']:
  hp=10*m['ar'];speed=max(1,(speed//3*(hp-m['ad'])+hp-1)//hp)*3
 left=speed-m['spent'];o=(1+m['case']%8)*8*(am+6)*4;d=(1+m['case']%5)*8*(max(1,dm)+6)
 if m['fanatic']:o=o*125//100
 o=o*m['ob']//100;d=d*m['db']//100
 if m['roads']:o=o*125//100
 if m['ba']>m['aa']:d=d*150//100
 if m['aa']>m['ba']:o=o*125//100
 terrain=3 if m['rock']or m['forest']else 2;d*=terrain*2
 if m.get('sensor_mask',0)&1:d=d*125//100
 if m.get('sensor_mask',0)&2:d=d*125//100
 o=o*min(left,3)//3
 if not m['sea']and m['difficulty']<2:o=o*(4-m['difficulty'])//2
 result=round_model([o,d,m['ar'],m['dr'],m['ad'],m['dd'],160,0],[(i*7919+17)&32767 for i in range(160)])
 assert result[0]==0
 return dict(error='ok',cursor=str(result[1]),offense=str(o),defense=str(d),ar=str(m['ar']),dr=str(m['dr']),ad=str(m['ad']),dd=str(m['dd']),am=str(am),dm=str(dm),terrain=str(terrain),left=str(left),after_ad=str(result[2]),after_dd=str(result[3]),ahp=str(result[4]),dhp=str(result[5]),rounds=str(result[6]),ah=str(result[7]),dh=str(result[8]),draws=str(result[9]),win=str(result[10]),unchanged='1',rc='0')

def controls():
 rows=[]
 for case in range(96):
  data,m=fixture(case);rows.append((f'field-{case}',data,rules_fixture(case),model(m)))
 data,m=fixture(0);_,o,_=decode(data);base_rules=rules_fixture(0)
 def bad(name,offset,fmt,value,error='unsupported'):
  b=bytearray(data);struct.pack_into('<'+fmt,b,offset,value);rows.append((name,bytes(b),base_rules,dict(error=error,cursor='0',unchanged='1',rc='0')))
 vp=o['vehicles'];up=o['prototypes']+52*320;tp=o['tiles']+44*((2*8+4)//2)
 for name,offset,fmt,value in [('ability',up+32,'I',1),('mobile',up+36,'B',1),('native-defender',vp+52+14,'B',0),('sleeping-attacker',vp+17,'B',3),('dead-attacker',vp+16,'B',10),('exhausted',vp+40,'B',3),('fungus',tp+8,'I',0x22),('bunker',tp+8,'I',0x802),('sea',tp,'B',0x20),('wrong-active-owner',vp+14,'B',6),('invisible-defender',vp+52+39,'B',0)]:
  bad(name,offset,fmt,value,'bad_state'if name in ['native-defender','dead-attacker']else'unsupported')
 # Native faction0 is intentionally unqualified, represented as bad state by
 # the current conventional field API rather than guessed psi mechanics.
 for side,other in [(5,6),(6,5)]:
  field=next(f for f in SCHEMA['LegacyFactionV10']['fields']if f['name']=='diplo_status')
  bad(f'truce-{side}',o['factions']+side*0x20bc+field['offset']+4*other,'I',0x14)
 # A far sensor no longer globally rejects an unrelated engagement.
 far=bytearray(data);struct.pack_into('<I',far,o['tiles']+8,0x80000000);rows.append(('sensor-outside-radius',bytes(far),base_rules,model(m)))
 for owner,alt,mask in [(-1,0x60,1),(6,0x60,1),(5,0x60,0),(-1,0x20,0)]:
  b=bytearray(data);p=o['tiles']+44*((0*8+4)//2);b[p]=alt;b[p+7]=owner&255;struct.pack_into('<I',b,p+8,0x80000000)
  rows.append((f'near-sensor-{owner}-{alt}',bytes(b),base_rules,model(dict(m,sensor_mask=mask))))
 for nearest_faction in [5,6]:
  b,meta=fixture(0,2);b=bytearray(b);_,bo,_=decode(b)
  for i,(x,faction)in enumerate([(0,5),(4,nearest_faction)]):
   p=bo['bases']+304*i;struct.pack_into('<hh',b,p,x,0);b[p+4]=faction
  p=bo['tiles']+44*(2//2);b[p]=0x20;b[p+7]=255;struct.pack_into('<I',b,p+8,0x80000000)
  rows.append((f'sea-sensor-nearest-tie-{nearest_faction}',bytes(b),base_rules,model(dict(meta,sensor_mask=1 if nearest_faction==6 else 0))))
 for raw_all in [False,True]:
  b,meta=fixture(0,1);b=bytearray(b);_,bo,_=decode(b);b[bo['bases']+0x8c+5]|=2
  if raw_all:
   for i in range(24):
    p=bo['tiles']+44*i+8;struct.pack_into('<I',b,p,struct.unpack_from('<I',b,p)[0]|0x80000000)
  rows.append((f'gsp-raw-precedence-{raw_all}',bytes(b),base_rules,model(dict(meta,sensor_mask=1 if raw_all else 3))))
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='c++');p.add_argument('--seeded',action='store_true');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 sources='rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats combat_rounds combat_crt world_combat_sensors world_field_combat world_combat_crt world_field_combat_probe'.split()
 exe=a.out/'world-field-combat-probe';cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(n+'.cpp'))for n in sources],'-o',str(exe)];subprocess.run(cmd,check=True)
 records=[];aggregate=b''
 for i,(name,data,rules,want)in enumerate(controls()):
  if a.seeded:
   seed=0x12345678;want=dict(want,seed=str(seed))
   if want['error']=='ok':
    _,values=crt_draws(seed,160);duel=[int(want[key])for key in ['offense','defense','ar','dr','ad','dd']];r=round_model([*duel,160,0],values)
    want.update(cursor=str(r[1]),seed=str(crt_draws(seed,r[9])[0]),after_ad=str(r[2]),after_dd=str(r[3]),ahp=str(r[4]),dhp=str(r[5]),rounds=str(r[6]),ah=str(r[7]),dh=str(r[8]),draws=str(r[9]),win=str(r[10]))
  wp=a.out/f'{i}.SC';rp=a.out/f'{i}.txt';wp.write_bytes(data);rp.write_bytes(rules)
  run=subprocess.run([str(exe),str(rp),str(wp)]+(['seeded']if a.seeded else[]),capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),check=True,timeout=30)
  got=dict(x.split('=')for x in run.stdout.split());assert got==want,(i,name,got,want)
  records.append(dict(case=i,name=name,worldSHA256=hashlib.sha256(data).hexdigest(),result=got));aggregate+=run.stdout.encode();(a.out/f'{i}.expected').write_text(run.stdout)
 report=dict(cases=len(records),accepted=sum(r['result']['error']=='ok'for r in records),outputSHA256=hashlib.sha256(aggregate).hexdigest(),casesDetail=records,native='UNRUN',originalExecutable='UNRUN',rng='Separate explicit CRT state'if a.seeded else'Recorded CRT draws');(a.out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='casesDetail'}))
if __name__=='__main__':main()
