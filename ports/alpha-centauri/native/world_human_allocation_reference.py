#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh stable intake vs independent wire composition and private full originals."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_game import decode,SCHEMA,FORMATS
from test_original_faction import SCHEMA as FACTIONS
from test_original_world import valid_fixture
from test_original_map import fixture as map_fixture
from test_world_unit_stats import rules_fixture,scalar
from test_units import sections,material
from test_world_tile_yields import ROW
from test_world_worker_radius import wire_expected as radius_expected
from test_world_terraform_ecology import wire_expected as ecology_expected
from worker_radius_reference import DX,DY
from test_tile_yields import expected as leaf_expected
from tile_bonus_reference import bonus_model
from test_technology_effects_world import facts as effect_facts
from test_technology_effects import model as effect_model
HERE=Path(__file__).resolve().parent
def rules(variant):
 t=sections(rules_fixture(variant));t['RESOURCEINFO']=[[f'R{i}','2','1','1','0'] for i in range(9)]
 for name,value in [('tech_three_nutrients_sqr','None'),('tech_three_minerals_sqr','None'),('tech_three_energy_sqr','None'),('tech_mining_platform_bonus','None'),('limit_mineral_mine_sans_road','1'),('tgl_nutrient_effect_with_mine','0')]:
  r,c,_=ROW[name];t['RULES'][r][c]=value
 return material(t)
def fixture(variant,version,case):
 b=bytearray(valid_fixture(version,0,1));_,off,spans=decode(b);b[off['metadata']:off['abstract']+len(spans['abstract'])]=map_fixture(16,12)[15:];_,off,_=decode(b);legacy=version<11;pf=off['legacy'] if legacy else off['preferences'];fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308
 def pref(name,index=0):
  f=next(f for f in SCHEMA['GamePrefs']['fields'] if f['name']==name);return pf+f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0)+index*struct.calcsize(FORMATS[f['type']])
 def fset(name,value):
  f=next(f for f in FACTIONS['LegacyFactionV10' if legacy else 'Faction']['fields'] if f['name']==name);struct.pack_into('<'+FORMATS[f['type']],b,off['factions']+fs+f['offset'],value)
 for i in range(33 if legacy else 64):struct.pack_into('<i',b,pref('SecretProjects',i),-1)
 for name,value in [('GovernorFaction',-1),('GameRules',0),('GameState',0x10),('DustCloudDuration',0),('SolarFlaresEvent',0)]:struct.pack_into('<i',b,pref(name),value)
 b[pref('FactionStatus')]=2
 flags=(case*13)&63;pop=case%6;fac=((flags&1)<<13)|(((flags>>1)&1)<<14)|(((flags>>2)&1)<<29);sat=[case%9,(case*3)%11,(case*7)%13];terrain=0x200000 if flags&32 else 0
 if flags&8:struct.pack_into('<i',b,pref('SecretProjects',27),0)
 if flags&16 and not legacy:struct.pack_into('<i',b,pref('SecretProjects',35),0)
 for name,value in [('player_flags',0x200),('SE_planet_pending',0),('SE_economy_pending',0),('tech_fungus_nutrient',1),('tech_fungus_mineral',1),('tech_fungus_energy',1),('base_count',1),*zip(['satellites_nutrient','satellites_mineral','satellites_energy'],sat)]:fset(name,value)
 # These definition facts were previously unused opaque fixture bytes. The fresh stage
 # now consumes them, so seed a genuine empty bonus list and zero rule commerce.
 struct.pack_into('<i',b,off['faction_definitions']+0x59c+0x504,0);struct.pack_into('<i',b,off['faction_definitions']+0x59c+0x50c,0)
 field=next(f for f in FACTIONS['MFaction']['fields'] if f['name']=='rule_flags');struct.pack_into('<I',b,off['faction_definitions']+0x59c+field['offset'],0)
 p=off['bases'];b[p:p+bs]=bytes(bs);struct.pack_into('<hhb',b,p,8,6,1);b[p+6]=pop;struct.pack_into('<I',b,p+0x78,(1<<(pop+1))-1);struct.pack_into('<I',b,p+0x8c,fac);struct.pack_into('<I',b,p+0x48,0xdeadbeef)
 for delta in [0xb0,0xb4,0xb8,0xbc,0xc0,0xc4,0xc8,0xcc]:struct.pack_into('<I',b,p+delta,0xcafebabe)
 for i in range(96):
  p=off['tiles']+44*i;b[p:p+44]=bytes(44);b[p]=0x60;b[p+2]=1 if i==52 else 9;b[p+4]=2;b[p+7]=1;struct.pack_into('<I',b,p+8,1 if i==52 else terrain)
 struct.pack_into('<I',b,off['metadata']+8,0);struct.pack_into('<I',b,off['metadata']+28,0)
 return bytes(b)
def model(variant,text,data,mode=0,expansion_enabled=False,reduction_before=None,energy_before=None):
 fail=lambda code:[code]
 info,off,spans=decode(data);legacy=int(info['version'])<11;pf=spans['legacy'] if legacy else spans['preferences'];fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308;nb=int(info['bases']);w,h=int(info['width']),int(info['height']);tables=sections(text)
 def pref(name,index=0):
  if legacy and name=='SecretProjects' and index>=33:return -1
  f=next(f for f in SCHEMA['GamePrefs']['fields'] if f['name']==name);o=f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0);fmt=FORMATS[f['type']];return struct.unpack_from('<'+fmt,pf,o+index*struct.calcsize(fmt))[0]
 def state(name):return scalar(spans['factions'][faction*fs:(faction+1)*fs],FACTIONS['LegacyFactionV10' if legacy else 'Faction'],name)
 def owner(i):return struct.unpack_from('<b',spans['bases'],i*bs+4)[0]
 def project(i):
  p=pref('SecretProjects',i)
  if p>=nb:raise ValueError('project')
  return p>=0 and owner(p)==faction
 def tile(x,y):return spans['tiles'][44*(y*(w//2)+x//2):44*(y*(w//2)+x//2+1)]
 if not nb:return fail(1)
 b=spans['bases'][:bs];x,y=struct.unpack_from('<hh',b);faction=owner(0);pop=struct.unpack_from('<b',b,6)[0];worked=struct.unpack_from('<I',b,0x78)[0];gov=struct.unpack_from('<I',b,0x38)[0]
 if not pref('FactionStatus')&(1<<faction) or gov&0x80000040==0x80000040 or faction==0:return fail(3)
 specialists,adjust=struct.unpack_from('<ii',b,0x7c)
 if adjust:return fail(3)
 radius=radius_expected(data)
 if radius[0]:return fail(5 if radius[0]==3 else 1)
 if gov&0x80000000 and (not 0<=specialists<=pop or not worked&1 or worked&~0x1fffff or worked&radius[1]&~1 or worked.bit_count()+specialists!=pop+1):return fail(3)
 if (int(info["version"])<11)!=(variant=="base"):return fail(1)
 home=tile(x,y)
 if not struct.unpack_from('<I',home,8)[0]&1 or home[2]&15!=faction:return fail(3)
 try:full=bool(b[0x8c+29//8]&(1<<(29%8))) or project(27) or (variant=='expansion' and project(35))
 except ValueError:return fail(1)
 counts=[state(n) for n in ['satellites_nutrient','satellites_mineral','satellites_energy']]
 if any(n<0 for n in counts):return fail(1)
 if not full and 2147483647 in counts:return fail(7)
 satellites=[min(pop,n if full else (n+1)//2) for n in counts];totals=satellites+[0];tiles=[0,0,0];tile_rows={};values=[[0,0,0] for _ in range(21)];energy=(energy_before if energy_before is not None else -123456 if mode>=2 else 0);flags=struct.unpack_from('<I',b,0x30)[0]&~0x8100;flat=struct.unpack_from('<I',spans['metadata'],28)[0]
 keys={r[1].lower():i for i,r in enumerate(tables['TECHNOLOGY'])};keys.update(none=-1,disable=-2)
 def basic(name):
  row,col,limits=ROW[name];v=tables['RULES'][row][col];return keys[v.lower()] if limits is None else max(limits[0],min(limits[1],int(v)))
 def technology(id):
  if faction<=0:return False
  if id==-1:return True
  if id<0 or id>=88:return False
  a,c=[keys[n.lower()] for n in tables['TECHNOLOGY'][id][6:8]];return a!=-2 and not(c==-2 and a!=-1) and bool(pref('TechOwners',id)&(1<<faction))
 effect_error,fresh_effects=effect_model(effect_facts(text,data)[faction])
 if effect_error:return fail(7 if effect_error==2 else 1)
 for i,(dx,dy) in enumerate(zip(DX[:21],DY[:21])):
  if i and radius[1]&(1<<i):continue
  xx,yy=x+dx,y+dy
  if not flat:xx=xx+w if xx<0 else xx-w if xx>=w else xx
  if not(0<=xx<w and 0<=yy<h):return fail(4)
  sq=tile(xx,yy);items,lm=struct.unpack_from('<II',sq,8)
  
  row=[0]*59;row[0]=int(variant=='expansion');row[7]=int(expansion_enabled);row[1:7]=[sq[0],sq[2],sq[5],items,lm,bonus_model(struct.unpack_from('<I',spans['metadata'],8)[0],xx,yy,pref('GameRules'),sq[0],items)];row[9]=state('SE_planet_pending');row[10]=state('SE_economy_pending');row[11:14]=fresh_effects[1:4];row[14]=state('base_count');row[15]=int(bool(struct.unpack_from('<I',b,0x30)[0]&4));row[18],row[19]=struct.unpack_from('<II',b,0x8c)
  if row[14]<0:return fail(1)
  try:row[16]=int(variant=='expansion' and project(33))
  except ValueError:return fail(1)
  flag=scalar(spans['faction_definitions'][faction*0x59c:(faction+1)*0x59c],FACTIONS['MFaction'],'rule_flags');row[17]=int(bool(flag&0x100))
  for j,n in enumerate(['tech_three_nutrients_sqr','tech_three_minerals_sqr','tech_three_energy_sqr','tech_mining_platform_bonus']):row[20]|=int(technology(basic(n)))<<j
  merchant=pref('SecretProjects',3)
  if merchant>=nb:return fail(1)
  row[21]=int(pref('GovernorFaction')==faction);row[22]=int(merchant==0);row[23]=int(pref('DustCloudDuration')!=0);row[24]=pref('SolarFlaresEvent')&0xffffffff;row[25]=mode&1;row[30]=0;row[27]=basic('tgl_nutrient_effect_with_mine');row[28]=basic('limit_mineral_mine_sans_road');row[29]=struct.unpack_from('<I',b,0x34)[0];row[31]=0
  for j,res in enumerate(tables['RESOURCEINFO']):row[32+3*j:35+3*j]=[0 if v=='*' else int(v) for v in res[1:4]]
  mirrors=not(items&1 and sq[2]&15<8) and not items&(0x2000|0x1000000) and not(items&0x20 and sq[0]&0xe0>0x3f) and sq[0]&0xe0>=0x60 and not items&0x200000 and bool(items&0x40)
  if mirrors:
   for mx,my in [(0,-2),(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1)]:
    nx,ny=xx+mx,yy+my
    if not flat:nx=nx+w if nx<0 else nx-w if nx>=w else nx
    if 0<=nx<w and 0<=ny<h:
     near=tile(nx,ny);row[26]+=int(bool(struct.unpack_from('<I',near,8)[0]&0x800000) and struct.unpack_from('<b',near,7)[0]==faction)
  result=leaf_expected(row)
  if result[0]:return fail(7 if result[0]==2 else 1)
  values[i]=result[1:4];tile_rows[i]=row
 from test_world_unit_support import original_model as support_model
 from test_citizen_definitions import reference as definition_model
 from human_allocation_reference import expected as select
 try:maintenance=support_model(text,data,variant)[1][0][-1]
 except OverflowError:return fail(7)
 except ValueError:return fail(1)
 definitions=definition_model(text,variant);citizens=[]
 for i in range(7):
  pre,obs,econ,psych,labs=struct.unpack_from('<5i',definitions,8+276*i+256);citizens += [int(technology(pre)),econ,psych,labs]
 packed=list(struct.unpack_from('<2I',b,0x84));current=struct.unpack_from('<i',b,0x50)[0]
 facts=[int(variant=='expansion'),0,pop,basic('nutrient_req_citizen'),maintenance,current,specialists,worked,radius[1],*satellites,*packed,basic('min_base_size_specialists'),*citizens,*[v for row in values for v in row]]
 visits=[];answer=select(facts,visits)
 if answer[0]:return fail(7 if answer[0]==2 else 4)
 scratch=reduction_before if reduction_before is not None else -2 if mode==2 else 2147483647 if mode==3 else 0
 for kind,i in visits:
  row=tile_rows[i][:];row[30]=scratch;row[31]=energy;result=leaf_expected(row)
  if result[0]:return fail(7 if result[0]==2 else 1)
  assert result[1:4]==values[i]
  scratch,energy=result[4:6]
  if kind:
   if row[1]&0xe0<0x60:flags|=0x8000
   for k in range(3):
    totals[k]+=values[i][k]*(-1 if kind==2 else 1)
    if not -(1<<31)<=totals[k]<(1<<31):return fail(7)
   scratch=0
 assert totals[:3]==answer[4:7]
 tiles=[totals[i]-satellites[i] for i in range(3)]
 if any(not -(1<<31)<=v<(1<<31) for v in tiles):return fail(7)
 eco=ecology_expected(data)
 if eco[0]:return fail(1)
 selected=answer[1];selected_values=[values[i] if selected&(1<<i) else [0,0,0] for i in range(21)]
 intake=[*satellites,*tiles,*totals,selected,flags,eco[1],scratch,energy,*[v for row in selected_values for v in row]]
 assert len(facts)==106 and len(intake)==78
 return [0,*facts,*answer[1:],*intake,len(visits),*[v for event in visits for v in event]]
