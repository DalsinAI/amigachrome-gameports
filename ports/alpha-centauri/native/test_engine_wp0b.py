#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Engine rules loader (engine/rules.cpp) controls.

Builds engine/tests/wp0b_test.cpp with every engine source, on the host with
ASan/UBSan or for m68k-linux under qemu (big-endian), then loads synthetic
rules directories (alpha.txt for the base game, alphax.txt for Alien
Crossfire, labels.txt and eight faction files; no original assets) with
engine_load_rules and compares every table with independent references:
- TECHNOLOGY/SOCIO/BONUSNAMES and the faction files come from
  test_faction.synthetic_rules/material and are checked with
  test_social.reference and test_faction.reference;
- RULES/WORLDBUILDER rows and values come from basic_fixture.json;
- CHASSIS..UNITS come from test_units.fixture and test_units.reference;
- FACILITIES come from test_facility_definitions.tables;
- RESOURCEINFO comes from test_resources.fixture;
- the remaining name tables are generated here with their expected values.
Rejections (unknown keys, duplicate techs, missing files or sections) must
exit 20 with an error. Optional --original DIR loads owner-provided rules
(never committed) and only requires a successful load.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import test_faction,test_social,test_units,test_facility_definitions,test_resources

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'
WPN_BASE=[*range(12),*range(14,23)] # base weapon rows -> Alien Crossfire slots

def fac_slot(variant,row):
 return row if variant=='expansion' or row<=33 else row+31

def facility_rows(variant):
 return test_facility_definitions.tables(variant)[1]

def terrain_rows():
 return [[f'Terr{i}',f'T{i}',f'Sea{i}','None' if i%2 else f'T{i+1}',str(i+1),'Build $STR0',f'k{i}',f'S{i}'] for i in range(20)]

def simple_sections():
 s={}
 s['TIMECONTROLS']=[[f'Time{i}',*[str(i*10+j) for j in range(7)]] for i in range(6)]
 s['MORALE']=[[f'Morale{i}',f'Life{i}'] for i in range(7)]
 s['DEFENSEMODES']=[[f'Def{i}',f'De-f{i}',f'D{i}',f'd{i}'] for i in range(3)]
 s['OFFENSEMODES']=[[f'Off{i}',f'Of-f{i}',f'O{i}',f'o{i}'] for i in range(3)]
 s['ORDERS']=[[f'Order{i}',f'L{i}'] for i in [*range(4),*range(24,30)]]
 s['COMPASS']=[[f'Dir{i}'] for i in range(8)]
 s['PLANS']=[[f'P{i}',f'Plan {i}'] for i in range(15)]
 s['TRIAD']=[[f'Triad{i}'] for i in range(3)]
 s['RESOURCES']=[[f'Res{i}',f'Res{i}s'] for i in range(4)]
 s['ENERGY']=[[f'E{i}',f'Energy{i}'] for i in range(3)]
 s['CITIZENS']=[[f'Cit{i}',f'Cits{i}',*([f'T{i}','None' if i%2 else 'Disable',str(i),str(i+1),str(i+2)] if i<7 else [])] for i in range(10)]
 s['DIFF']=[[f'Diff{i}'] for i in range(6)]
 s['MANDATE']=[[f'Mandate{i}',f'MANDATE{i}'] for i in range(4)]
 s['MOOD']=[[f'Mood{i}'] for i in range(9)]
 s['REPUTE']=[[f'Repute{i}'] for i in range(8)]
 s['MIGHT']=[[f'Start{i}',f'Mighty{i}'] for i in range(7)]
 s['PROPOSALS']=[[f'Proposal{i}',f'T{i}' if i%3 else 'None',f'Text {i}'] for i in range(11)]
 s['NATURAL']=[[f'Natural{i}',f'N{i}'] for i in range(16)]
 return s

FACTION_FILES=[f'base{i}' for i in range(1,8)]
NEW_FACTION_FILES=[f'new{i}' for i in range(1,8)]
BONUS_LINES=['TECH,T88','MORALE,0,RESEARCH,2,SOCIAL,++E2,IMPUNITY,M12','TECH,1,HURRY,2000,FACILITY,7,INTEREST,3',
 'ROBUST,E4,IMMUNITY,-E5,PENALTY,M03,ALIEN,0','FUNGNUTRIENT,1,FUNGMINERALS,2,FUNGENERGY,3,VOTES,4',
 'FREEPROTO,0,AQUATIC,0,FREEFAC,9,REVOLT,10','NODRONE,2,WORMPOLICE,0,FREEABIL,3,PROBECOST,50,DEFENSE,1,OFFENSE,2']

def bonus_line(i,variant):
 lines=BONUS_LINES if variant=='expansion' else [l for l in BONUS_LINES if all(k in test_faction.BONUS[:29] for k in l.split(',')[::2])]
 return lines[(i-1)%len(lines)]

def faction_text(i,file,variant='expansion'):
 header=f'Faction {file},Desc {i},Noun{i},{"FMN"[i%3]},{1+i%2},Leader{i},{"FM"[i%2]},{i%3-1},1,0,1,0,'
 return test_faction.material(bonuses=bonus_line(i,variant),header=header,
  tail=f'Adj{i},Abbr{i}\nAssistant{i},Scientist{i},City{i}\nTitle{i},a{i},b{i},c{i},d{i},\nInsult{i}').replace(b'#TEST',('#'+file.upper()).encode())

def rules_text(variant,units_count=20):
 head=test_faction.synthetic_rules(variant).decode().replace('#END\n','')
 fixture=json.loads((SRC/'basic_fixture.json').read_text())
 rules=fixture['rules_rows'] if variant=='expansion' else fixture['rules_rows'][:-2]
 parts=[head,'#RULES\n'+'\n'.join(','.join(r) for r in rules)+'\n',
        '#WORLDBUILDER\n'+'\n'.join(','.join(r) for r in fixture['world_rows'])+'\n',
        '#TERRAIN\n'+'\n'.join(','.join(r) for r in terrain_rows())+'\n',
        test_resources.fixture().replace('#END\n','')]
 units=test_units.fixture(variant,units_count);del units['TECHNOLOGY']
 parts.append(test_units.material(units).decode('cp1252').replace('#END\n',''))
 parts.append('#FACILITIES\n'+'\n'.join(','.join(r)+',' for r in facility_rows(variant))+'\n')
 for name,rows in simple_sections().items():parts.append('#'+name+'\n'+'\n'.join(','.join(r) for r in rows)+'\n')
 parts.append('#FACTIONS\n'+'\n'.join(f'{f},{f}' for f in FACTION_FILES)+'\n')
 if variant=='expansion':parts.append('#NEWFACTIONS\n'+'\n'.join(f'{f},{f}' for f in NEW_FACTION_FILES)+'\n')
 return (''.join(parts)+'#END\n').encode('cp1252')

LABELS=['first label','Second, with comma','$WORD0 third','']

def make_dir(path,variant,rules=None,labels=True,skip_faction=None,crlf=False):
 path.mkdir(parents=True)
 data=rules_text(variant) if rules is None else rules
 conv=(lambda b:b.replace(b'\n',b'\r\n')) if crlf else (lambda b:b)
 (path/('alphax.txt' if variant=='expansion' else 'alpha.txt')).write_bytes(conv(data))
 if labels:(path/'labels.txt').write_bytes(conv(('#LABELS\n'+str(len(LABELS))+'\n'+'\n'.join(LABELS)+'\n').encode()))
 files=NEW_FACTION_FILES if variant=='expansion' else FACTION_FILES
 for i,f in enumerate(files,1):
  if f!=skip_faction:(path/(f.upper()+'.TXT' if i==3 else f+'.txt')).write_bytes(conv(faction_text(i,f,variant)))
 return data

def expected(variant,data):
 e={}
 e['expansion']=str(int(variant=='expansion'))
 e['labels.count']=str(len(LABELS))
 for i,l in enumerate(LABELS):e[f'labels.{i}']=l
 techs=test_social.section(data,'TECHNOLOGY')
 for i,r in enumerate(techs):
  e.update({f'tech.{i}.name':r[0],f'tech.{i}.short':r[1],f'tech.{i}.ai_power':r[2],f'tech.{i}.ai_tech':r[3],
            f'tech.{i}.ai_wealth':r[4],f'tech.{i}.ai_growth':r[5],f'tech.{i}.preq1':'-1',f'tech.{i}.preq2':'-1',f'tech.{i}.flags':r[8]})
 e['tech_valid']='89';e['tech_commerce']='0'
 fx=json.loads((SRC/'basic_fixture.json').read_text())['expected']
 basic=[v for k,v in fx.items() if k.startswith('basic.')];world=[v for k,v in fx.items() if k.startswith('world.')]
 if variant=='base':basic[-2:]=[0,0]
 for i,v in enumerate(basic):e[f'rules.{i}']=str(v)
 for i,v in enumerate(world):e[f'world.{i}']=str(v)
 for i,r in enumerate(terrain_rows()):
  e.update({f'terrain.{i}.name':r[0],f'terrain.{i}.preq':str(i),f'terrain.{i}.name_sea':r[2],
            f'terrain.{i}.preq_sea':'-1' if i%2 else str(i+1),f'terrain.{i}.rate':r[4],f'terrain.{i}.shortcuts':r[7],
            f'order.{i+4}.order':'Build '+r[0],f'order.{i+4}.order_sea':'Build '+r[2],f'order.{i+4}.letter':r[6]})
 for n,i in enumerate([*range(4),*range(24,30)]):
  e.update({f'order.{i}.order':f'Order{i}',f'order.{i}.order_sea':f'Order{i}',f'order.{i}.letter':f'L{i}'})
 for i in range(9):
  for k,v in zip(['nutrient','mineral','energy','unused'],[i,i+1,i+2,0]):e[f'resinfo.{i}.{k}']=str(v)
 s=simple_sections()
 for i,r in enumerate(s['TIMECONTROLS']):
  e[f'time.{i}.name']=r[0]
  for k,v in zip(['turn','base','unit','event','extra','refresh','accumulated'],r[1:]):e[f'time.{i}.{k}']=v
 # Component tables: the oracle uses file rows, the engine Alien Crossfire slots.
 ref=test_units.reference(data,variant)
 wslot=(lambda r:r) if variant=='expansion' else (lambda r:WPN_BASE[r])
 for k,v in ref.items():
  part=k.split('.')
  if part[0] in ('chassis','reactors','armor','abilities'):e[k]=v
  elif part[0]=='weapons':e['weapon.'+str(wslot(int(part[1])))+'.'+'.'.join(part[2:])]=v
  elif part[0]=='units' and part[2] not in ('plan','cost','carry'):
   e[k]=str(wslot(int(v))) if part[2]=='weapon' else v
 for i,r in enumerate(s['MORALE']):e[f'morale.{i}.name'],e[f'morale.{i}.lifecycle']=r
 for key,sec in [('defmode','DEFENSEMODES'),('offmode','OFFENSEMODES')]:
  for i,r in enumerate(s[sec]):
   for k,v in zip(['name','hyphened','abbrev','letter'],r):e[f'{key}.{i}.{k}']=v
 for row,r in enumerate(facility_rows(variant),1):
  i=fac_slot(variant,row);tech=lambda t:{'none':'-1','disable':'-2'}.get(t.lower(),t[1:])
  e.update({f'facility.{i}.name':r[0],f'facility.{i}.cost':r[1],f'facility.{i}.maint':r[2],f'facility.{i}.preq':tech(r[3]),
            f'facility.{i}.free':tech(r[4]),f'facility.{i}.effect':r[5]})
  ai=r[6:11] if i>=70 else ['0']*5
  for k,v in zip(['fight','power','tech','wealth','growth'],ai):e[f'facility.{i}.ai_{k}']=v
 for i,r in enumerate(s['COMPASS']):e[f'compass.{i}']=r[0]
 for i,r in enumerate(s['PLANS']):e[f'plan.{i}.short'],e[f'plan.{i}.full']=r
 for i,r in enumerate(s['TRIAD']):e[f'triad.{i}']=r[0]
 for i,r in enumerate(s['RESOURCES']):e[f'resource.{i}.singular'],e[f'resource.{i}.plural']=r
 for i,r in enumerate(s['ENERGY']):e[f'energy.{i}.abbrev'],e[f'energy.{i}.name']=r
 for i,r in enumerate(s['CITIZENS']):
  e[f'citizen.{i}.singular'],e[f'citizen.{i}.plural']=r[:2]
  if i<7:vals=[str(i),'-1' if i%2 else '-2',r[4],r[5],r[6]]
  else:vals=['-2','-2','0','0','0']
  for k,v in zip(['preq','obsol','econ','psych','labs'],vals):e[f'citizen.{i}.{k}']=v
 soc=test_social.reference(data,variant)
 for k,v in soc.items():
  if k!='bonus.count':e[k]=v
 for i in range(int(soc['bonus.count']),41):e[f'bonus.{i}']=''
 for i,r in enumerate(s['DIFF']):e[f'diff.{i}']=r[0]
 for i,r in enumerate(s['MANDATE']):e[f'mandate.{i}.name'],e[f'mandate.{i}.caps']=r
 for i,r in enumerate(s['MOOD']):e[f'mood.{i}']=r[0]
 for i,r in enumerate(s['REPUTE']):e[f'repute.{i}']=r[0]
 for i,r in enumerate(s['MIGHT']):e[f'might.{i}.adj_start'],e[f'might.{i}.adj']=r
 for i,r in enumerate(s['PROPOSALS']):
  e[f'proposal.{i}.name']=r[0];e[f'proposal.{i}.preq']=str(i) if i%3 else '-1';e[f'proposal.{i}.description']=r[2]
 for i,r in enumerate(s['NATURAL']):e[f'natural.{i}.name'],e[f'natural.{i}.short']=r
 files=NEW_FACTION_FILES if variant=='expansion' else FACTION_FILES
 e['faction_pool']=str(14 if variant=='expansion' else 7);e['missing_factions']='0'
 for i,f in enumerate(files,1):
  r=test_faction.reference(data,faction_text(i,f,variant),f.upper(),f.upper())
  r.pop('abbreviation')
  # Thinker bounds the AI priorities (fight -1..1, others 0..1).
  r['ai_fight']=str(max(-1,min(1,int(r['ai_fight']))))
  for k in ['ai_power','ai_tech','ai_wealth','ai_growth']:r[k]=str(max(0,min(1,int(r[k]))))
  r['filename']=f;r['search_key']=f
  for k,v in r.items():e[f'faction.{i}.{k}']=v
 return e

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 sources=sorted(ENGINE.glob('*.cpp'))+sorted((ENGINE/'stubs').glob('*.cpp'))+[ENGINE/'tests'/'wp0b_test.cpp']
 exe=out/'wp0b-test';cmd=[a.cc,*flags,'-I',str(ENGINE),*map(str,sources),'-o',str(exe)];subprocess.run(cmd,check=True)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 def run(*args):return subprocess.run([*runner,str(exe),*map(str,args)],capture_output=True,text=True,encoding='cp1252',timeout=600,env=env)
 r=run('unit');assert r.returncode==0,(r.stdout,r.stderr);unit=r.stdout.strip();print(unit)
 records=[]
 def load(name,d,variant,*extra):
  r=run('load',d,variant,*extra);assert r.returncode==0,(name,r.returncode,r.stdout[-2000:],r.stderr[-4000:])
  return dict(line.split('=',1) for line in r.stdout.splitlines())
 for variant in ['base','expansion']:
  for crlf in [False,True]:
   name=f'{variant}{"-crlf" if crlf else ""}';d=out/name;data=make_dir(d,variant,crlf=crlf)
   got=load(name,d,variant);exp=expected(variant,data)
   bad=[(k,exp[k],got.get(k)) for k in exp if got.get(k)!=exp[k]]
   assert not bad,(name,len(bad),bad[:20])
   # Slots the base game does not define stay disabled with empty names.
   if variant=='base':
    for w in [12,13,23,24,25]:assert got[f'weapon.{w}.name']=='' and got[f'weapon.{w}.prerequisite']=='-2',(name,w)
    for f in [*range(34,65),*range(103,134)]:assert got[f'facility.{f}.name']=='' and got[f'facility.{f}.preq']=='-2',(name,f)
   assert '(null)' not in got.values(),(name,[k for k,v in got.items() if v=='(null)'][:10])
   twice=load(name+'-twice',d,variant,'twice');assert twice==got,(name,'reload differs')
   records.append(dict(case=name,valid=True,checked=len(exp),sha256=hashlib.sha256(data).hexdigest()))
 # A missing faction file is reported (PLAYERFILE) but the load succeeds.
 d=out/'missing-faction';make_dir(d,'expansion',skip_faction='new5');got=load('missing-faction',d,'expansion')
 assert got['missing_factions']=='1' and got['faction.5.formal_name_faction']=='',got['missing_factions']
 records.append(dict(case='missing-faction',valid=True))
 # Rejections.
 good=rules_text('expansion')
 def reject(name,variant='expansion',rules=None,labels=True,fragment=''):
  d=out/name;make_dir(d,variant,rules=rules,labels=labels);r=run('load',d,variant)
  assert r.returncode==20 and 'error=' in r.stdout and fragment in r.stdout,(name,r.returncode,r.stdout[-500:],r.stderr[-2000:])
  records.append(dict(case=name,valid=False,error=r.stdout.strip()))
 reject('bad-tech-key',rules=good.replace(b'Weapon3,W3,2,3,4,2,None',b'Weapon3,W3,2,3,4,2,T999'),fragment='BADTECHKEY')
 reject('bad-weapon-key',rules=good.replace(b'Unit1,C1,W1,',b'Unit1,C1,Wx,'),fragment='BADWEAPKEY')
 reject('bad-chassis-key',rules=good.replace(b'Unit2,C2,',b'Unit2,Cx,'),fragment='BADCHASKEY')
 reject('bad-armor-key',rules=good.replace(b'Unit4,C4,W4,A4,',b'Unit4,C4,W4,Ax,'),fragment='BADARMKEY')
 reject('duplicate-tech',rules=good.replace(b'Test 5,T5,',b'Test 5,T4,'),fragment='DUPLICATETECH')
 reject('missing-labels',labels=False,fragment='labels')
 for sec in [b'RULES',b'WORLDBUILDER',b'TERRAIN',b'CHASSIS',b'UNITS',b'FACILITIES',b'SOCIO',b'NEWFACTIONS',b'NATURAL']:
  reject('missing-'+sec.decode().lower(),rules=good.replace(b'#'+sec+b'\n',b'#X'+sec+b'\n'),fragment='#'+sec.decode())
 reject('base-rules-as-expansion',variant='expansion',rules=rules_text('base'),fragment='')
 # The base directory has no alphax.txt: loading it as Alien Crossfire fails.
 r=run('load',out/'base','expansion');assert r.returncode==20 and 'alphax' in r.stdout,(r.stdout,r.stderr[-500:])
 records.append(dict(case='base-dir-as-expansion',valid=False,error=r.stdout.strip()))
 for p in a.original:
  for variant in ['base','expansion']:
   if (p/('alphax.txt' if variant=='expansion' else 'alpha.txt')).exists() or (p/('ALPHAX.TXT' if variant=='expansion' else 'ALPHA.TXT')).exists():
    got=load('original-'+variant,p,variant);records.append(dict(case='original-'+variant,valid=True,missing_factions=got['missing_factions']))
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,unit=unit,cases=records,
   binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n')
 nv=sum(c['valid'] for c in records);print(f'{nv} load cases and {len(records)-nv} rejection cases passed')
if __name__=='__main__':main()
