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
from technology_effects_reference import effects as fresh_effects
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
 field=next(f for f in FACTIONS['MFaction']['fields'] if f['name']=='rule_flags');struct.pack_into('<I',b,off['faction_definitions']+0x59c+field['offset'],0)
 p=off['bases'];b[p:p+bs]=bytes(bs);struct.pack_into('<hhb',b,p,8,6,1);b[p+6]=pop;struct.pack_into('<I',b,p+0x78,(1<<(pop+1))-1);struct.pack_into('<I',b,p+0x8c,fac);struct.pack_into('<I',b,p+0x48,0xdeadbeef)
 for delta in [0xb0,0xb4,0xb8,0xbc,0xc0,0xc4,0xc8,0xcc]:struct.pack_into('<I',b,p+delta,0xcafebabe)
 for i in range(96):
  p=off['tiles']+44*i;b[p:p+44]=bytes(44);b[p]=0x60;b[p+2]=1 if i==52 else 9;b[p+4]=2;b[p+7]=1;struct.pack_into('<I',b,p+8,1 if i==52 else terrain)
 struct.pack_into('<I',b,off['metadata']+8,0);struct.pack_into('<I',b,off['metadata']+28,0)
 return bytes(b)
def fresh_fungus_fixture(data,tile_index,overflow=False):
 b=bytearray(data);info,off,_=decode(b);legacy=int(info['version'])<11
 ds=FACTIONS['MFaction'];fs=FACTIONS['LegacyFactionV10' if legacy else 'Faction']
 def put(start,schema,name,value,index=0):
  field=next(f for f in schema['fields'] if f['name']==name);fmt=FORMATS[field['type']]
  struct.pack_into('<'+fmt,b,start+field['offset']+index*struct.calcsize(fmt),value)
 definition=off['faction_definitions']+0x59c;state=off['factions']+(0x20bc if legacy else 0x20cc)
 put(definition,ds,'faction_bonus_count',3);put(definition,ds,'rule_commerce',2147483647 if overflow else 0)
 for i,value in enumerate([2,-1,3]):
  put(definition,ds,'faction_bonus_id',7+i,i);put(definition,ds,'faction_bonus_val1',value,i)
 for name in ['tech_fungus_nutrient','tech_fungus_mineral','tech_fungus_energy']:put(state,fs,name,-123456789)
 put(state,fs,'SE_economy_pending',5 if overflow else 0)
 at=off['tiles']+44*tile_index;items=struct.unpack_from('<I',b,at+8)[0];struct.pack_into('<I',b,at+8,items|0x20)
 return bytes(b)
def model(variant,text,data,mode=0):
 fail=lambda code:[code]+[0x5a5a5a5a]*78
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
 if not pref('FactionStatus')&(1<<faction) or gov&0x80000040==0x80000040:return fail(3)
 if struct.unpack_from('<i',b,0x7c)[0] or not worked&1 or worked&~0x1fffff or worked.bit_count()!=pop+1:return fail(4)
 radius=radius_expected(data)
 if radius[0]:return fail(5 if radius[0]==3 else 1)
 if worked&radius[1]&~1:return fail(4)
 home=tile(x,y)
 if not struct.unpack_from('<I',home,8)[0]&1 or home[2]&15!=faction:return fail(3)
 try:full=bool(b[0x8c+29//8]&(1<<(29%8))) or project(27) or (variant=='expansion' and project(35))
 except ValueError:return fail(1)
 counts=[state(n) for n in ['satellites_nutrient','satellites_mineral','satellites_energy']]
 if any(n<0 for n in counts):return fail(1)
 if not full and 2147483647 in counts:return fail(7)
 satellites=[min(pop,n if full else (n+1)//2) for n in counts];totals=satellites+[0];tiles=[0,0,0];values=[[0,0,0] for _ in range(21)];energy=-123456 if mode>=2 else 0;flags=struct.unpack_from('<I',b,0x30)[0]&~0x8100;flat=struct.unpack_from('<I',spans['metadata'],28)[0]
 keys={r[1].lower():i for i,r in enumerate(tables['TECHNOLOGY'])};keys.update(none=-1,disable=-2)
 def basic(name):
  row,col,limits=ROW[name];v=tables['RULES'][row][col];return keys[v.lower()] if limits is None else max(limits[0],min(limits[1],int(v)))
 def technology(id):
  if faction<=0:return False
  if id==-1:return True
  if id<0 or id>=88:return False
  a,c=[keys[n.lower()] for n in tables['TECHNOLOGY'][id][6:8]];return a!=-2 and not(c==-2 and a!=-1) and bool(pref('TechOwners',id)&(1<<faction))
 for i,(dx,dy) in enumerate(zip(DX[:21],DY[:21])):
  if not worked&(1<<i):continue
  xx,yy=x+dx,y+dy
  if not flat:xx=xx+w if xx<0 else xx-w if xx>=w else xx
  if not(0<=xx<w and 0<=yy<h):return fail(4)
  sq=tile(xx,yy);items,lm=struct.unpack_from('<II',sq,8)
  fungus=None
  if items&0x20:
   effect_error,effect_values=fresh_effects(text,data,faction)
   if effect_error:return fail(7 if effect_error==3 else 1)
   fungus=effect_values[:3]
  if sq[0]&0xe0<0x60:flags|=0x8000
  row=[0]*59;row[0]=int(variant=='expansion');row[1:7]=[sq[0],sq[2],sq[5],items,lm,bonus_model(struct.unpack_from('<I',spans['metadata'],8)[0],xx,yy,pref('GameRules'),sq[0],items)];row[9]=state('SE_planet_pending');row[10]=state('SE_economy_pending');row[11:14]=fungus if fungus is not None else [state(n) for n in ['tech_fungus_nutrient','tech_fungus_mineral','tech_fungus_energy']];row[14]=state('base_count');row[15]=int(bool(struct.unpack_from('<I',b,0x30)[0]&4));row[18],row[19]=struct.unpack_from('<II',b,0x8c)
  if row[14]<0:return fail(1)
  try:row[16]=int(variant=='expansion' and project(33))
  except ValueError:return fail(1)
  flag=scalar(spans['faction_definitions'][faction*0x59c:(faction+1)*0x59c],FACTIONS['MFaction'],'rule_flags');row[17]=int(bool(flag&0x100))
  for j,n in enumerate(['tech_three_nutrients_sqr','tech_three_minerals_sqr','tech_three_energy_sqr','tech_mining_platform_bonus']):row[20]|=int(technology(basic(n)))<<j
  merchant=pref('SecretProjects',3)
  if merchant>=nb:return fail(1)
  row[21]=int(pref('GovernorFaction')==faction);row[22]=int(merchant==0);row[23]=int(pref('DustCloudDuration')!=0);row[24]=pref('SolarFlaresEvent')&0xffffffff;row[25]=mode&1;row[30]=(-2 if mode==2 else 2147483647 if mode==3 else 0) if i==0 else 0;row[27]=basic('tgl_nutrient_effect_with_mine');row[28]=basic('limit_mineral_mine_sans_road');row[29]=struct.unpack_from('<I',b,0x34)[0];row[31]=energy
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
  energy=result[5];values[i]=result[1:4]
  for k in range(3):
   tiles[k]+=values[i][k];totals[k]+=values[i][k]
   if not -(1<<31)<=tiles[k]<(1<<31) or not -(1<<31)<=totals[k]<(1<<31):return fail(7)
 eco=ecology_expected(data)
 if eco[0]:return fail(1)
 return [0,*satellites,*tiles,*totals,worked,flags,eco[1],0,energy,*[v for row in values for v in row]]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-control',type=Path);ap.add_argument('--original',type=Path,action='append',default=[]);ap.add_argument('--original-rules',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 sources=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','vehicle_stack.cpp','tile_yields.cpp','world_technology_effects.cpp','world_tile_yields.cpp','world_worker_radius.cpp','world_terraform_ecology.cpp','world_stable_intake.cpp','world_stable_intake_probe.cpp'];exe=a.out/'world-stable-intake-probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/n) for n in sources],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];cases=[];texts={v:rules(v) for v in ['base','expansion']}
 for variant in ['base','expansion']:
  for case in range(1,65):cases.append((f'{variant}-{case}',variant,fixture(variant,7+case%4 if variant=='base' else 11+case%2,case),case))
 for variant,version in [('base',7),('expansion',12)]:
  for overflow in [False,True]:cases.append((f'fresh-fungus-{variant}-{overflow}',variant,fresh_fungus_fixture(fixture(variant,version,1),44,overflow),None))
 for version in range(7,13):
  for variant in ['base','expansion']:cases.append((f'cross-v{version}-{variant}',variant,fixture(variant,version,31),None))
 seed=fixture('expansion',12,64);_,off,_=decode(seed)
 for name,offset,fmt,value in [('specialist',off['bases']+0x7c,'i',1),('mask0',off['bases']+0x78,'I',0),('maskhigh',off['bases']+0x78,'I',0x80000003),('governor',off['bases']+0x38,'I',0x80000040),('nonhuman',off['preferences']+0x58,'B',0),('no-centre',off['tiles']+52*44+8,'I',0),('fungus',off['tiles']+44*44+8,'I',0x20),('stack',off['tiles']+8,'I',2),('project',off['preferences']+0x84+27*4,'i',99)]:
  b=bytearray(seed);struct.pack_into('<'+fmt,b,offset,value);cases.append((name,'expansion',bytes(b),None))
 for name,value,case in [('satellite-negative',-1,64),('satellite-half-overflow',2147483647,64),('satellite-full-max',2147483647,1)]:
  b=bytearray(fixture('expansion',12,case));_,o,_=decode(b);f=next(f for f in FACTIONS['Faction']['fields'] if f['name']=='satellites_nutrient');struct.pack_into('<i',b,o['factions']+0x20cc+f['offset'],value);cases.append((name,'expansion',bytes(b),None))
 for variant in ['base','expansion']:cases.append(('empty-'+variant,variant,valid_fixture(12,0,0),None))
 overflow=sections(texts['expansion']);overflow['RESOURCEINFO'][3][3]='1073741824';overrides={'sum-overflow':material(overflow)};cases.append(('sum-overflow','expansion',fixture('expansion',12,3),None))
 case_modes={}
 for name,mode in [('solar-control-off',0),('solar-control-on',1),('scratch-negative',2),('scratch-max',3)]:
  b=bytearray(fixture('expansion',12,3));_,o,_=decode(b);struct.pack_into('<i',b,o['preferences']+next(f for f in SCHEMA['GamePrefs']['fields'] if f['name']=='SolarFlaresEvent')['offset'],2);cases.append((name,'expansion',bytes(b),None));case_modes[name]=mode
 b=bytearray(fixture('expansion',12,3));_,o,_=decode(b);p=o['tiles']+44*44;b[p]=0x40;struct.pack_into('<I',b,p+8,0);cases.append(('selected-ocean','expansion',bytes(b),None))
 for p in a.original:
  name='owner-'+p.name;cases.append((name,'base',p.read_bytes(),None))
  if a.original_rules:overrides[name]=a.original_rules.read_bytes()
 digest=hashlib.sha256();records=[];accepted=rejected=direct=0
 for name,variant,data,original_case in cases:
  text=overrides.get(name,texts[variant]);rule=a.out/(variant+'.txt');rule.write_bytes(text);path=a.out/(name+'.SC');path.write_bytes(data);r=subprocess.run([*runner,str(exe),variant,str(rule),str(path),str(case_modes.get(name,0))],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr);want=model(variant,text,data,case_modes.get(name,0));assert len(want)==79;encoded=struct.pack('<79I',*[n&0xffffffff for n in want]);assert r.stdout==encoded,(name,struct.unpack('<79I',r.stdout),want);assert path.read_bytes()==data;digest.update(encoded);accepted+=want[0]==0;rejected+=want[0]!=0
  if a.original_control and original_case is not None:
   original=json.loads((a.original_control/f'stable-yield-{variant}-case{original_case}'/'receipt.json').read_text());assert want[0]==0 and want[7:11]==original['intake2'] and want[11]==original['worked'] and want[13]==original['rawEco']&0xffffffff,(name,want,original);direct+=1
  records.append({'name':name,'error':want[0],'inputSHA256':hashlib.sha256(data).hexdigest(),'outputSHA256':hashlib.sha256(encoded).hexdigest()})
 receipt={'worlds':len(records),'accepted':accepted,'rejected':rejected,'actualFullOriginalComparisons':direct,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'allWorldInputsAndWireUnchanged':True,'command':cmd,'records':records,'native':'UNRUN'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
