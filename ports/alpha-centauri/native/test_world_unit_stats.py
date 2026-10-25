#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned original unit dependencies, source bodies and independent raw-file model."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_game import decode,SRC,FORMATS
from test_original_faction import SCHEMA as FACTION_SCHEMA
from test_saved_speed import clamp,calculate
from test_units import reference,sections,fixture as units_fixture,material
from test_original_world import valid_fixture
from test_original_map import word

def scalar(blob,schema,name,index=0):
 field=next(f for f in schema['fields'] if f['name']==name);fmt=FORMATS[field['type']];return struct.unpack_from('<'+fmt,blob,field['offset']+struct.calcsize(fmt)*index)[0]

def original_model(rules,data,variant):
 info,offsets,spans=decode(data);version=int(info['version']);assert (version<11)==(variant=='base')
 table=reference(rules,variant);t=sections(rules);road=clamp(int(t['RULES'][0][0]),1,100)
 rows=t['TECHNOLOGY'];keys={row[1].lower():i for i,row in enumerate(rows)};keys.update(none=-1,disable=-2)
 flags=[int(row[8],2) for row in rows];prereqs=[(keys[row[6].lower()],keys[row[7].lower()]) for row in rows]
 pf=spans['preferences'] if version>=11 else spans['legacy'];shift=0 if version>=11 else 0x7c
 projects=list(struct.unpack_from('<'+('64i' if version>=11 else '33i'),pf,0x84));projects += [-1]*(64-len(projects));owners=pf[0x1e0-shift:0x1e0-shift+89]
 nbase=int(info['bases']);nv=int(info['vehicles']);bs=308 if version>=11 else 304;fs=0x20cc if version>=11 else 0x20bc;f_schema=FACTION_SCHEMA['Faction' if version>=11 else 'LegacyFactionV10']
 bases=[spans['bases'][i*bs:(i+1)*bs] for i in range(nbase)];factions=[spans['factions'][i*fs:(i+1)*fs] for i in range(8)];definitions=[spans['faction_definitions'][i*0x59c:(i+1)*0x59c] for i in range(8)]
 def project(index,faction):
  b=projects[index]
  if b<0:return False
  if b>=nbase:raise ValueError('unsafe project reference')
  return bases[b][4]==faction
 def technology(index,faction):
  if faction<=0:return False
  if index==-1:return True
  if index<0 or index>=88:return False
  a,b=prereqs[index]
  return a!=-2 and not(b==-2 and a!=-1) and bool(owners[index]&(1<<faction))
 def effective_flags(unit,p,triad):
  raw=struct.unpack_from('<I',p,32)[0]
  if unit<64:return raw
  requested=0x800420|(0x8000000 if variant=='expansion' else 0) if triad==2 else 0x800400
  if raw&requested==requested:return raw
  rules=definitions[unit//64];count=scalar(rules,FACTION_SCHEMA['MFaction'],'faction_bonus_count')
  if not(0<=count<=8):raise ValueError('invalid bonus count')
  for i in range(count):
   if scalar(rules,FACTION_SCHEMA['MFaction'],'faction_bonus_id',i)!=15:continue
   ability=scalar(rules,FACTION_SCHEMA['MFaction'],'faction_bonus_val1',i)
   if not(0<=ability<(24 if variant=='base' else 29)):raise ValueError('invalid FREEABIL')
   bit=1<<ability
   if requested&bit and not raw&bit and technology(int(table[f'abilities.{ability}.prerequisite']),unit//64):raw|=bit
  return raw
 def fac(b,index):return b>=0 and bool(bases[b][0x8c+index//8]&(1<<(index%8)))
 def morale(v,p,riots):
  unit=struct.unpack_from('<h',v,10)[0];faction=v[14];raw=v[36]
  if not faction:
   if version>=11 and unit==19:
    x,y=struct.unpack_from('<hh',v);w=int(info['width']);h=int(info['height']);flat=struct.unpack_from('<I',spans['metadata'],28)[0]!=0;n=-2
    for dx,dy in [(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]:
     tx=x+dx;ty=y+dy
     if not flat:tx=(tx+w)%w
     if not(0<=tx<w and 0<=ty<h):continue
     tile=(ty*w+tx)//2*44;items=struct.unpack_from('<I',spans['tiles'],tile+8)[0];n+=bool(items&0x20) and (spans['tiles'][tile]&0xe0)>=0x40
   else:
    turn=int(info['turn']);n=sum(turn>=limit for limit in [45,90,170,250,330,330])
    n+=bool(struct.unpack_from('<I',v,4)[0]&0x100)+(unit==10)+((struct.unpack_from('<H',v,8)[0]>>3)&3)
   return clamp(n,0,6)
  state=factions[faction];rules=definitions[faction]
  if p[42]==11:
   n=clamp(scalar(state,f_schema,'SE_probe'),0,3)+2*project(30,faction)+sum(bool(flags[i]&2) and technology(i,faction) for i in range(89))+raw
   return clamp(n,2,6)
  offense=int(table[f'weapons.{p[37]}.offense'])
  if unit<64 and offense<0:return clamp(raw,0,6)
  n=clamp(scalar(state,f_schema,'SE_morale'),-4,4);n+=1 if n<=-2 else -1 if n>=2 else 0
  rule=scalar(rules,FACTION_SCHEMA['MFaction'],'rule_morale');n+=min(0,rule)
  if not(-2147483648<=n<=2147483647):raise ValueError('morale overflow')
  home=struct.unpack_from('<h',v,46)[0]
  if fac(home,2) and n<0:n=-((-n)//2)
  if fac(home,35) and unit<64 and (offense<0 or (version>=11 and unit==15)) and n<0:n=-((-n)//2)
  n+=max(0,rule)
  if not(-2147483648<=n<=2147483647):raise ValueError('morale overflow')
  immune=bool(scalar(rules,FACTION_SCHEMA['MFaction'],'rule_flags')&0x8000)
  if immune:n=max(0,n)
  if riots and home>=0 and (struct.unpack_from('<I',bases[home],48)[0]&2) and not immune:n-=1
  n+=raw
  if not(-2147483648<=n<=2147483647):raise ValueError('morale overflow')
  return clamp(n,0,6)
 digest=2166136261;count=0
 for i in range(nv):
  v=spans['vehicles'][52*i:52*(i+1)];unit=struct.unpack_from('<h',v,10)[0];p=spans['prototypes'][52*unit:52*(unit+1)];m=[]
  for riots in [False,True]:
   n=morale(v,p,riots);m.append(n)
   for value in [i,int(riots),n]:digest=word(digest,value)
   count+=1
  triad=int(table[f'chassis.{p[36]}.triad']);tower=version>=11 and unit==19
  options=(int(project(35,unit//64)) if triad==2 and not tower else 0)+(2*int(project(10,v[14])) if triad==1 and not tower else 0)
  for skip in [False,True]:
   row=[int(variant=='expansion'),unit,p[36],p[37],p[39],effective_flags(unit,p,triad),struct.unpack_from('<b',p,42)[0],road,v[16],options+8*skip,m[1]]
   _,speed=calculate(row,table)
   for value in [int(skip),speed,2 if speed>255 else 0]:digest=word(digest,value)
   count+=1
 queries=0
 for faction in range(8):
  for index in range(64):digest=word(digest,project(index,faction));queries+=1
  for index in range(-2,90):digest=word(digest,technology(index,faction));queries+=1
 return dict(version=str(version),vehicles=str(nv),stats=str(count),queries=str(queries),digest=f'{digest:08x}',rc='0')

def rules_fixture(variant,road=3):
 t=units_fixture(variant);t['WEAPONS'][0][2:4]=['1','0'];t['WEAPONS'][12][2:4]=['-1','0']
 for i in [0,1,2,3,88]:t['TECHNOLOGY'][i][8]='000000010'
 t['TECHNOLOGY'][1][6:8]=['Disable','None'];t['TECHNOLOGY'][2][6:8]=['None','Disable'];t['TECHNOLOGY'][3][6:8]=['T0','Disable']
 basic=json.loads((SRC/'basic_fixture.json').read_text());rows=[r[:] for r in basic['rules_rows']];rows[0][0]=str(road)
 if variant=='base':rows=rows[:-2]
 t['RULES']=rows;t['WORLDBUILDER']=basic['world_rows'];return material(t)

def world_fixture(version,case,vehicles=9,bases=2,narrow=False):
 from test_original_map import fixture as map_fixture
 b=bytearray(valid_fixture(version,vehicles,bases));info,off,spans=decode(b)
 if narrow:
  start=off['metadata'];end=off['abstract']+len(spans['abstract']);b[start:end]=map_fixture(2,6)[15:];info,off,spans=decode(b)
 width=int(info['width']);pref=off['preferences'] if version>=11 else off['legacy'];shift=0 if version>=11 else 0x7c
 struct.pack_into('<i',b,pref+0x44,[0,44,45,89,90,169,170,249,250,329,330,999][case%12])
 projects=64 if version>=11 else 33
 for i in range(projects):struct.pack_into('<i',b,pref+0x84+4*i,-1)
 if bases:
  for i in [10,30,35]:
   if i<projects and case&32:struct.pack_into('<i',b,pref+0x84+4*i,0 if case&16 else min(1,bases-1))
 for i in range(89):b[pref+0x1e0-shift+i]=(1<<5) if case&1 else 0
 fs=0x20cc if version>=11 else 0x20bc;bs=308 if version>=11 else 304;f_schema=FACTION_SCHEMA['Faction' if version>=11 else 'LegacyFactionV10']
 def fset(blob,schema,name,value):
  field=next(f for f in schema['fields'] if f['name']==name);struct.pack_into('<'+FORMATS[field['type']],b,blob+field['offset'],value)
 for faction in range(8):fset(off['faction_definitions']+faction*0x59c,FACTION_SCHEMA['MFaction'],'faction_bonus_count',0)
 fset(off['factions']+5*fs,f_schema,'SE_morale',[-4,-3,-2,-1,0,1,2,3,4,50][case%10]);fset(off['factions']+5*fs,f_schema,'SE_probe',[-4,0,2,5][case//8%4])
 fset(off['faction_definitions']+5*0x59c,FACTION_SCHEMA['MFaction'],'rule_morale',[-7,-1,0,1,7][case%5]);fset(off['faction_definitions']+5*0x59c,FACTION_SCHEMA['MFaction'],'rule_flags',0x8000 if case&8 else 0)
 for i in range(bases):
  p=off['bases']+bs*i;b[p+4]=5 if i==0 else 6;struct.pack_into('<hh',b,p,(i*2)%width,2);struct.pack_into('<I',b,p+48,2 if case&4 else 0)
  b[p+0x8c:p+0x8c+(12 if version>=11 else 8)]=bytes(12 if version>=11 else 8)
  if case&1:b[p+0x8c]|=4
  if case&2:b[p+0x8c+4]|=8
 ids=[320,321,8,9,15,19,10,322,323]
 for i in range(vehicles):
  p=off['vehicles']+52*i;unit=ids[i];struct.pack_into('<hh',b,p,(i*2)%width,2);struct.pack_into('<h',b,p+10,unit);b[p+14]=0 if i in [2,5,6] else 5
  b[p+36]=[0,2,5,6,255][case//4%5];b[p+16]=[0,1,9,20,255][case%5];struct.pack_into('<h',b,p+46,0 if bases and b[p+14] else -1)
  struct.pack_into('<I',b,p+4,0x100 if case&8 else 0);struct.pack_into('<H',b,p+8,(case%4)<<3)
  u=off['prototypes']+52*unit;b[u:u+52]=bytes(52);b[u+36]=2 if i==7 else 1 if i==8 else 0;b[u+37]=12 if i in [2,3,6] else 0;b[u+39]=4 if i==7 else 1;b[u+42]=11 if i==1 else 3
  struct.pack_into('<I',b,u+32,(0x20 | (0x8000000 if version>=11 else 0)) if i==7 else 0)
 for index in range(int(info['width'])//2*int(info['height'])):
  p=off['tiles']+44*index;b[p]=0x60;struct.pack_into('<I',b,p+8,0x20 if index%(case%9+1)==0 else 0)
 struct.pack_into('<I',b,off['metadata']+28,0 if case&1 else 1)
 return bytes(b)

def controls():
 result=[]
 for variant,version in [('base',7),('expansion',12)]:
  rules=rules_fixture(variant)
  for case in range(64):result.append((f'{variant}-context{case}',variant,rules,world_fixture(version,case),True))
  for nbase in [0,2]:result.append((f'{variant}-empty{nbase}',variant,rules,world_fixture(version,0,0,nbase),True))
  result.append((variant+'-narrow-cylinder',variant,rules,world_fixture(version,41,narrow=True),True))
  result.append((variant+'-byte-speed-limit',variant,rules_fixture(variant,100),world_fixture(version,63),True))
  data=world_fixture(version,0);_,off,_=decode(data);pref=off['preferences'] if version>=11 else off['legacy'];fs=0x20cc if version>=11 else 0x20bc
  # Unsafe original arithmetic or references are rejected before a recovered
  # reference body would access/overflow them. These cases are not fidelity claims.
  for value in [2147483647,-2147483648]:
   b=bytearray(data);f=next(f for f in FACTION_SCHEMA['MFaction']['fields'] if f['name']=='rule_morale');struct.pack_into('<i',b,off['faction_definitions']+5*0x59c+f['offset'],value)
   # Force negative SE for MIN and positive/raw contribution for MAX.
   schema=FACTION_SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];sf=next(f for f in schema['fields'] if f['name']=='SE_morale');struct.pack_into('<i',b,off['factions']+5*fs+sf['offset'],-4 if value<0 else 4)
   result.append((variant+'-morale-overflow'+str(value),variant,rules,bytes(b),False))
  b=bytearray(data);struct.pack_into('<i',b,pref+0x84+10*4,512);result.append((variant+'-bad-project-reference',variant,rules,bytes(b),False))
  b=bytearray(data);b[off['prototypes']+320*52+37]=255;result.append((variant+'-bad-component',variant,rules,bytes(b),False))
 # Faction-granted abilities are owned by prototype_id/64, not vehicle faction.
 for variant,version in [('base',7),('expansion',12)]:
  available=[5,10,23]+([27] if variant=='expansion' else [])
  for triad in range(3):
   for bits in range(1<<len(available)):
    for ownership in range(4):
     b=bytearray(world_fixture(version,0));_,o,_=decode(b)
     fs=0x20cc if version>=11 else 0x20bc;f_schema=FACTION_SCHEMA['Faction' if version>=11 else 'LegacyFactionV10']
     def setfield(blob,schema,name,value,index=0):
      field=next(f for f in schema['fields'] if f['name']==name);struct.pack_into('<'+FORMATS[field['type']],b,blob+field['offset']+4*index,value)
     owner=o['faction_definitions']+5*0x59c;other=o['faction_definitions']+6*0x59c
     setfield(owner,FACTION_SCHEMA['MFaction'],'faction_bonus_count',len(available))
     setfield(other,FACTION_SCHEMA['MFaction'],'faction_bonus_count',len(available))
     for k,ability in enumerate(available):
      for where,enabled in [(owner,bool(bits&(1<<k))),(other,not bool(bits&(1<<k)))]:
       setfield(where,FACTION_SCHEMA['MFaction'],'faction_bonus_id',15 if enabled else 0,k)
       setfield(where,FACTION_SCHEMA['MFaction'],'faction_bonus_val1',ability,k)
     setfield(other,FACTION_SCHEMA['MFaction'],'rule_morale',0)
     setfield(other,FACTION_SCHEMA['MFaction'],'rule_flags',0)
     setfield(o['factions']+6*fs,f_schema,'SE_morale',0);setfield(o['factions']+6*fs,f_schema,'SE_probe',0)
     b[o['vehicles']+14]=6
     proto=o['prototypes']+320*52;b[proto+36]=triad
     struct.pack_into('<I',b,proto+32,[0,0,1<<10,1<<23][ownership])
     pref=o['preferences'] if version>=11 else o['legacy'];shift=0 if version>=11 else 0x7c
     b[pref+0x1e0-shift]=[0,1<<5,1<<6,(1<<5)|(1<<6)][ownership]
     result.append((f'{variant}-freeabil-triad{triad}-bits{bits}-owners{ownership}',variant,rules_fixture(variant),bytes(b),True))
  # None/Disable prerequisites, duplicates, malformed ids/count and short circuit.
  for prerequisite in ['None','Disable','T1','T2','T88']:
   for direct in [False,True]:
    b=bytearray(world_fixture(version,0));_,o,_=decode(b);owner=o['faction_definitions']+5*0x59c
    struct.pack_into('<i',b,owner+1292,2)
    for k in range(2):struct.pack_into('<i',b,owner+1296+4*k,15);struct.pack_into('<i',b,owner+1328+4*k,10)
    pref=o['preferences'] if version>=11 else o['legacy'];shift=0 if version>=11 else 0x7c
    b[pref+0x1e0-shift:pref+0x1e0-shift+89]=bytes([1<<5])*89
    if direct:struct.pack_into('<I',b,o['prototypes']+320*52+32,0x800400)
    t=sections(rules_fixture(variant));t['ABILITIES'][10][2]=prerequisite
    result.append((f'{variant}-prerequisite-{prerequisite}-raw{direct}',variant,material(t),bytes(b),True))
  b=bytearray(world_fixture(version,1));_,o,_=decode(b);owner=o['faction_definitions']+5*0x59c
  struct.pack_into('<i',b,owner+1292,2)
  for k,ability in enumerate([10,-1]):struct.pack_into('<i',b,owner+1296+4*k,15);struct.pack_into('<i',b,owner+1328+4*k,ability)
  result.append((variant+'-grant-then-invalid-transaction',variant,rules_fixture(variant),bytes(b),False))
  for count,ability in [(-1,10),(9,10),(1,-1),(1,24 if variant=='base' else 29)]:
   b=bytearray(world_fixture(version,0));_,o,_=decode(b);owner=o['faction_definitions']+5*0x59c
   struct.pack_into('<i',b,owner+1292,count);struct.pack_into('<i',b,owner+1296,15);struct.pack_into('<i',b,owner+1328,ability)
   result.append((f'{variant}-invalid-freeabil-count{count}-ability{ability}',variant,rules_fixture(variant),bytes(b),False))
 return result

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'world-stats-probe';files=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','world_unit_stats_probe.cpp'];cmd=[a.cc,*flags,*[str(SRC/n) for n in files],'-o',str(exe)];subprocess.run(cmd,check=True)
 records=[]
 for i,(name,variant,rules,data,valid) in enumerate(controls()):
  game=out/f'{i}.SC';text=out/f'{i}.txt';game.write_bytes(data);text.write_bytes(rules);r=subprocess.run([*runner,str(exe),variant,str(text),str(game)],capture_output=True,text=True,timeout=30,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  assert r.returncode==(0 if valid else 20),(name,r.returncode,r.stdout,r.stderr)
  expected=None
  if valid:
   expected=original_model(rules,data,variant);actual=dict(x.split('=') for x in r.stdout.split());assert actual==expected,(name,actual,expected)
  records.append(dict(name=name,valid=valid,passed=True,result=expected))
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,cases=records,binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(f'{len(records)} owned unit-stat cases passed')
if __name__=='__main__':main()
