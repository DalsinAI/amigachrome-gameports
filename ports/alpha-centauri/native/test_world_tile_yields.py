#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,re,struct,subprocess
from pathlib import Path
from test_original_game import decode,FORMATS,SCHEMA
from test_original_faction import SCHEMA as FACTIONS
from test_original_world import valid_fixture
from test_world_unit_stats import rules_fixture,scalar
from test_units import sections,material
from test_tile_yields import expected as leaf_expected
from tile_bonus_reference import bonus_model
HERE=Path(__file__).resolve().parent
ROW={};row=-1;column=0
for line in (HERE/'basic_fields.inc').read_text().splitlines():
 m=re.match(r'RULE_(ROW|ITEM|TECH)\((\w+)(?:,\s*(-?\d+),\s*(-?\d+))?\)',line)
 if not m:continue
 if m[1]!='ITEM':row+=1;column=0
 else:column+=1
 ROW[m[2]]=(row,column,None if m[1]=='TECH' else (int(m[3]),int(m[4])))
def rules(variant,case):
 t=sections(rules_fixture(variant));t['RESOURCEINFO']=[[f'R{i}',str((i+case)%4),str((i+case+1)%5),str((i+case+2)%6),'0'] for i in range(9)]
 if case&1:t['RESOURCEINFO'][5][2:4]=['*','*']
 for name,value in [('tech_three_nutrients_sqr','T0'),('tech_three_minerals_sqr','T1'),('tech_three_energy_sqr','T2'),('tech_mining_platform_bonus','T3'),('limit_mineral_mine_sans_road',str(case%3)),('tgl_nutrient_effect_with_mine',str(-(case&1)))]:
  r,c,_=ROW[name];t['RULES'][r][c]=value
 return material(t)
def fixture(version,case,empty=False):
 b=bytearray(valid_fixture(version,0,0 if empty else 2));info,off,spans=decode(b);legacy=version<11;pf=off['legacy'] if legacy else off['preferences'];fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308
 def pref(name,index=0):
  f=next(f for f in SCHEMA['GamePrefs']['fields'] if f['name']==name);o=f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0);return pf+o+index*struct.calcsize(FORMATS[f['type']])
 def fset(faction,name,value):
  f=next(f for f in FACTIONS['LegacyFactionV10' if legacy else 'Faction']['fields'] if f['name']==name);struct.pack_into('<'+FORMATS[f['type']],b,off['factions']+faction*fs+f['offset'],value)
 for i in range(33 if legacy else 64):struct.pack_into('<i',b,pref('SecretProjects',i),-1)
 if not empty:
  struct.pack_into('<i',b,pref('SecretProjects',3),0 if case&1 else 1)
  if not legacy:struct.pack_into('<i',b,pref('SecretProjects',33),1 if case&2 else -1)
 for i in range(89):b[pref('TechOwners',i)]=(i*37+case*11)&255
 struct.pack_into('<i',b,pref('GovernorFaction'),case%8);struct.pack_into('<i',b,pref('DustCloudDuration'),case&1);struct.pack_into('<i',b,pref('SolarFlaresEvent'),case%4);struct.pack_into('<i',b,pref('GameRules'),0x2000 if case&4 else 0)
 for f in range(8):
  for name,value in [('SE_planet_pending',(f+case)%9-4),('SE_economy_pending',(f+case*2)%10-4),('tech_fungus_nutrient',(f+case)%4),('tech_fungus_mineral',(f+case+1)%4),('tech_fungus_energy',(f+case+2)%4),('base_count',1 if f==5 else 2)]:fset(f,name,value)
  flag=next(x for x in FACTIONS['MFaction']['fields'] if x['name']=='rule_flags');struct.pack_into('<I',b,off['faction_definitions']+f*0x59c+flag['offset'],0x100 if (case+f)&1 else 0)
 for i in range(0 if empty else 2):
  p=off['bases']+i*bs;b[p+4]=5+i;struct.pack_into('<I',b,p+0x30,4 if (case+i)&1 else 0);struct.pack_into('<I',b,p+0x34,(case*0x600+i*0x800)&0x7e00);b[p+0x8c:p+0x8c+(8 if legacy else 12)]=bytes(8 if legacy else 12)
  for bit in [1,3,13,14,26,36,37,38]:
   if (case+i+bit)%3:b[p+0x8c+bit//8]|=1<<(bit%8)
 width,height=int(info['width']),int(info['height']);items=[0,1,0x2000,0x1000000,0x20,0x200000,0x40,0x800040,0x800000,0x8000,0x10,0x14,0x88000,0x400000]
 for i in range(width//2*height):
  p=off['tiles']+44*i;b[p]=[0x20,0x40,0x60,0x80,0xa0][(i+case)%5]|((i%3)*8);b[p+2]=(i+case)%16;b[p+5]=((i+case)%4)<<6;b[p+7]=(i%9-1)&255
  struct.pack_into('<I',b,p+8,items[(i+case)%len(items)]|[0,0x400,0x20000400,0x20020400,0x20010400][(i+case)%5]);struct.pack_into('<I',b,p+12,[0,1,2,4,8,0x80,0x200,0x400,0x800,0x8000,0x80000004][(i+case)%11])
 struct.pack_into('<I',b,off['metadata']+8,[0,1,0x80000001,0xffffffff][case%4]);struct.pack_into('<I',b,off['metadata']+28,case&1)
 return bytes(b)
def mirror_fixture(version,flat):
 b=bytearray(fixture(version,flat));info,off,_=decode(b);width,height=int(info['width']),int(info['height'])
 for i in range(width//2*height):
  p=off['tiles']+44*i;b[p]=0x60;b[p+2]=15;b[p+5]=0;b[p+7]=5 if i%2 else 6
  struct.pack_into('<II',b,p+8,0x800000,0)
 for x,y in [(0,0),(width-2,0),(1,1)]:
  p=off['tiles']+44*((y*width+x)//2);struct.pack_into('<I',b,p+8,0x40)
 struct.pack_into('<I',b,off['metadata']+28,flat)
 return bytes(b)
def expected(variant,text,data):
 info,off,spans=decode(data);legacy=int(info['version'])<11;pf=spans['legacy'] if legacy else spans['preferences'];fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308;nbase=int(info['bases']);width,height=int(info['width']),int(info['height']);t=sections(text)
 def pref(name,index=0):
  f=next(f for f in SCHEMA['GamePrefs']['fields'] if f['name']==name)
  if legacy and name=='SecretProjects' and index>=33:return -1
  o=f['offset']-(0x7c if legacy and f['offset']>=0x184 else 0);fmt=FORMATS[f['type']];return struct.unpack_from('<'+fmt,pf,o+index*struct.calcsize(fmt))[0]
 keys={r[1].lower():i for i,r in enumerate(t['TECHNOLOGY'])};keys.update(none=-1,disable=-2)
 def basic(name):
  r,c,limits=ROW[name];v=t['RULES'][r][c]
  if limits is None:return keys[v.lower()]
  return max(limits[0],min(limits[1],int(v)))
 def technology(id,faction):
  if faction<=0:return False
  if id==-1:return True
  if id<0 or id>=88:return False
  a,b=[keys[k.lower()] for k in t['TECHNOLOGY'][id][6:8]];return a!=-2 and not(b==-2 and a!=-1) and bool(pref('TechOwners',id)&(1<<faction))
 def state(faction,name):return scalar(spans['factions'][faction*fs:(faction+1)*fs],FACTIONS['LegacyFactionV10' if legacy else 'Faction'],name)
 def base_owner(index):return struct.unpack_from('<b',spans['bases'],index*bs+4)[0]
 def tile(x,y):return spans['tiles'][((y*width+x)//2)*44:((y*width+x)//2+1)*44]
 xs=[0,1,width-2,width-1,-1,1,width];ys=[0,1,0,(height-1)|1,0,0,0];rows=[]
 for b in range(nbase+1):
  for f in range(-1,9):
   for x,y in zip(xs,ys):
    for mode in range(4):
     failure=[1]+[0x5a5a5a5a]*59+[1]+[0x5a5a5a5a]*9
     if b>=nbase or f<0 or f>=8 or x<0 or y<0 or x>=width or y>=height or (x+y)&1:rows.append(failure);continue
     p=spans['bases'][b*bs:(b+1)*bs];sq=tile(x,y);it,lm=struct.unpack_from('<II',sq,8);bonus=bonus_model(struct.unpack_from('<I',spans['metadata'],8)[0],x,y,pref('GameRules'),sq[0],it)
     r=[0]*59;r[0]=int(variant=='expansion');r[1:7]=[sq[0],sq[2],sq[5],it,lm,bonus];r[7]=int(variant=='expansion' and mode&1);r[8]=int(mode>=2);r[9]=state(f,'SE_planet_pending');r[10]=state(f,'SE_economy_pending')
     for j,name in enumerate(['tech_fungus_nutrient','tech_fungus_mineral','tech_fungus_energy']):r[11+j]=state(f,name)
     r[14]=state(f,'base_count');r[15]=int(bool(struct.unpack_from('<I',p,0x30)[0]&4));project=pref('SecretProjects',33)
     if variant=='expansion' and project>=0:
      if project>=nbase:rows.append(failure);continue
      r[16]=int(base_owner(project)==f)
     own=base_owner(b);flag=scalar(spans['faction_definitions'][own*0x59c:(own+1)*0x59c],FACTIONS['MFaction'],'rule_flags');r[17]=int(bool(flag&0x100));r[18],r[19]=struct.unpack_from('<II',p,0x8c)
     for j,name in enumerate(['tech_three_nutrients_sqr','tech_three_minerals_sqr','tech_three_energy_sqr','tech_mining_platform_bonus']):r[20]|=int(technology(basic(name),f))<<j
     merchant=pref('SecretProjects',3)
     if merchant>=nbase:rows.append(failure);continue
     r[21]=int(pref('GovernorFaction')==f);r[22]=int(merchant==b);r[23]=int(pref('DustCloudDuration')!=0);r[24]=pref('SolarFlaresEvent')&0xffffffff;r[25]=mode&1;r[27]=basic('tgl_nutrient_effect_with_mine');r[28]=basic('limit_mineral_mine_sans_road');r[29]=struct.unpack_from('<I',p,0x34)[0];r[30]=mode-2;r[31]=-123456
     for i,row in enumerate(t['RESOURCEINFO']):
      for j,value in enumerate(row[1:4]):r[32+i*3+j]=0 if value=='*' else int(value)
     base_square=bool(it&1 and sq[2]&15<8);mirrors=not base_square and not it&(0x2000|0x1000000) and not(it&0x20 and sq[0]&0xe0>0x3f) and sq[0]&0xe0>=0x60 and not it&0x200000 and bool(it&0x40)
     if mirrors:
      current=b if mode==0 else 1-b if mode==1 else -1 if mode==2 else 999
      if current<0 or current>=nbase:rows.append(failure);continue
      owner=base_owner(current);flat=struct.unpack_from('<I',spans['metadata'],28)[0]!=0
      for dx,dy in [(0,-2),(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1)]:
       nx,ny=x+dx,y+dy
       if not flat:nx%=width
       if nx<0 or nx>=width or ny<0 or ny>=height:continue
       near=tile(nx,ny);r[26]+=bool(struct.unpack_from('<I',near,8)[0]&0x800000) and struct.unpack_from('<b',near,7)[0]==owner
     result=leaf_expected(r);rows.append([0,*r,*result])
 return rows
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',type=Path,action='append',default=[]);ap.add_argument('--original-rules',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 names=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','tile_yields.cpp','world_tile_yields.cpp','world_tile_yields_probe.cpp'];exe=a.out/'world-tile-yields-probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/n) for n in names],'-o',str(exe)];subprocess.run(cmd,check=True)
 cases=[]
 for version in range(7,13):
  for case in range(8):
   for variant in ['base','expansion']:cases.append((f'v{version}-{case}-{variant}',variant,rules(variant,case),fixture(version,case)))
 for version in range(7,13):
  for flat in range(2):
   for variant in ['base','expansion']:cases.append((f'mirrors-v{version}-flat{flat}-{variant}',variant,rules(variant,flat),mirror_fixture(version,flat)))
 for variant in ['base','expansion']:cases.append(('empty-'+variant,variant,rules(variant,0),fixture(12,0,True)))
 for path in a.original:
  assert a.original_rules;cases.append(('original-'+path.name,'base',a.original_rules.read_bytes(),path.read_bytes()))
 digest=hashlib.sha256();records=[];queries=accepted=rejected=0;runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 for i,(name,variant,text,data) in enumerate(cases):
  rp=a.out/f'{i}.txt';sp=a.out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);run=subprocess.run([*runner,str(exe),variant,str(rp),str(sp)],capture_output=True,timeout=90,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert run.returncode==0,(name,run.returncode,run.stderr)
  want=expected(variant,text,data);encoded=b''.join(struct.pack('<70I',*(v&0xffffffff for v in row)) for row in want);assert run.stdout==encoded,(name,len(run.stdout),len(encoded),next((j for j in range(min(len(encoded),len(run.stdout))) if encoded[j]!=run.stdout[j]),None))
  assert sp.read_bytes()==data and rp.read_bytes()==text;digest.update(encoded);queries+=len(want);accepted+=sum(row[60]==0 for row in want);rejected+=sum(row[60]!=0 for row in want);records.append({'name':name,'queries':len(want),'passed':True})
 receipt=dict(worlds=len(cases),queries=queries,accepted=accepted,rejected=rejected,outputSHA256=digest.hexdigest(),records=records,binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd,nativeExecution='UNRUN');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
