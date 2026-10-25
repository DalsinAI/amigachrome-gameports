#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Saved-world edge prices: recovered source and independent raw-file model."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_game import decode,SRC,FORMATS
from test_original_faction import SCHEMA
from test_saved_speed import clamp,calculate
from test_units import reference,sections
from test_world_unit_stats import world_fixture,rules_fixture,scalar
from test_original_map import word
OFFSETS=[(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]

def original_model(rules,data,variant):
 info,_,s=decode(data);version=int(info['version']);assert (version<11)==(variant=='base')
 table=reference(rules,variant);road=clamp(int(sections(rules)['RULES'][0][0]),1,100)
 width=int(info['width']);height=int(info['height']);flat=bool(struct.unpack_from('<I',s['metadata'],28)[0]);nv=int(info['vehicles'])
 pf=s['preferences'] if version>=11 else s['legacy'];baseid=struct.unpack_from('<i',pf,0x84+8*4)[0]
 bs=308 if version>=11 else 304;fs=0x20cc if version>=11 else 0x20bc;schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10']
 def tile(x,y):
  p=(y*width+x)//2*44
  return s['tiles'][p]&0xe0,s['tiles'][p+5]&0xc0,struct.unpack_from('<I',s['tiles'],p+8)[0]
 digest=2166136261;cases=0;rejected=0
 for i in range(nv):
  v=s['vehicles'][52*i:52*(i+1)];x,y=struct.unpack_from('<hh',v);unit=struct.unpack_from('<h',v,10)[0];faction=v[14];p=s['prototypes'][unit*52:(unit+1)*52]
  ch=p[36];weapon=p[37];triad=int(table[f'chassis.{ch}.triad']);offense=int(table[f'weapons.{weapon}.offense']);plan=struct.unpack_from('<b',p,42)[0];abilities=struct.unpack_from('<I',p,32)[0]
  xeno=baseid>=0 and s['bases'][baseid*bs+4]==faction
  planet=scalar(s['factions'][faction*fs:(faction+1)*fs],schema,'SE_planet')
  if triad==0 and unit>=64:
   owner=unit//64;definition=s['faction_definitions'][owner*0x59c:(owner+1)*0x59c]
   count=scalar(definition,SCHEMA['MFaction'],'faction_bonus_count');assert 0<=count<=8
   techrows=sections(rules)['TECHNOLOGY'];keys={r[1].lower():n for n,r in enumerate(techrows)};keys.update(none=-1,disable=-2)
   shift=0 if version>=11 else 0x7c
   def known(tech):
    if tech==-1:return True
    if not(0<=tech<88):return False
    a,b=keys[techrows[tech][6].lower()],keys[techrows[tech][7].lower()]
    return a!=-2 and not(b==-2 and a!=-1) and bool(pf[0x1e0-shift+tech]&(1<<owner))
   for slot in range(count):
    if scalar(definition,SCHEMA['MFaction'],'faction_bonus_id',slot)!=15:continue
    a=scalar(definition,SCHEMA['MFaction'],'faction_bonus_val1',slot);assert 0<=a<(24 if version<11 else 29)
    bit=1<<a
    if bit&0x800400 and known(int(table[f'abilities.{a}.prerequisite'])):abilities|=bit
  speed=calculate([int(version>=11),unit,ch,weapon,p[39],abilities,plan,road,0,8,0],table)[0]
  fa,_,fi=tile(x,y)
  for edge,(dx,dy) in enumerate(OFFSETS):
   tx=x+dx;ty=y+dy
   if not flat:
    if tx>=width:tx-=width
    elif tx<0:tx+=width
   if not(0<=tx<width and 0<=ty<height) or (tx+ty)&1:continue
   for rough in range(2):
    if (tx,ty)==(x,y):rejected+=1;continue
    ta,rock,ti=tile(tx,ty);dist=abs(x-tx);dist=min(dist,width-dist) if not flat else dist
    sea_native=unit==9 or (version>=11 and unit==14);land_native=unit==8 or (version>=11 and unit==15)
    if ta<0x60:value=road*(3 if ti&0x20 and ta==0x40 and triad==1 and not sea_native and not xeno else 1)
    elif fa<0x60 or triad!=0:value=road
    elif faction and fi&9 and ti&9:value=0
    elif faction and ti&5 and (fi&5 or fi&0x20 and xeno):value=1
    elif ti&0x20 and (xeno or faction==0 or land_native):value=1
    elif faction and fi&0x80 and ti&0x80 and dist==1 and abs(y-ty)==1:value=1
    elif ch==2 or abilities&0x400:value=road
    else:
     value=road*(1+int(rock>0x40 and not rough)+int(bool(ti&0x200000) and not rough))
     if faction and ti&0x20 and (unit>=64 or offense>=0):
      if plan not in (9,12) and planet<=0:value+=2*road
      else:value=max(value,speed)
    for value_ in (i,edge,rough,value):digest=word(digest,value_)
    cases+=1
 return dict(version=str(version),vehicles=str(nv),prices=str(cases),rejected=str(rejected),digest=f'{digest:08x}',rc='0')

def controls():
 result=[]
 items=[0,4,8,1,0x20,0x80,0x200000,0x200020,0x24,0x81,0x80000000,0x2000ac]
 for variant,version in [('base',7),('expansion',12)]:
  for case in range(96):
   b=bytearray(world_fixture(version,case%64,narrow=case>=88));info,o,_=decode(b);fs=0x20cc if version>=11 else 0x20bc;schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];sf=next(f for f in schema['fields'] if f['name']=='SE_planet');pref=o['preferences'] if version>=11 else o['legacy']
   struct.pack_into('<i',b,o['factions']+5*fs+sf['offset'],[-3,0,1,4][case//3%4])
   struct.pack_into('<i',b,pref+0x84+8*4,0 if case&1 else -1)
   for index in range(int(info['width'])*int(info['height'])//2):
    p=o['tiles']+44*index;b[p]=[0x20,0x40,0x60,0x80][(index+case//12)%4];b[p+5]=[0,0x40,0x80,0xc0][(index//3+case//4)%4]
    struct.pack_into('<I',b,p+8,items[(index+case)%len(items)])
   # Faction-specific PSI and terraforming/artifact paths; explicit native IDs
   # 14/15 exercise variant gates, and real saved designs need no name/flags.
   for i in range(9):
    vp=o['vehicles']+52*i;unit=[320,321,8,9,15,19,10,322,323][i]
    if i==3 and case&4:unit=14;struct.pack_into('<h',b,vp+10,unit)
    p=o['prototypes']+52*unit;b[p:p+52]=bytes(52);b[p+36]=[0,1,2,3,5][(i+case//8)%5];b[p+37]=12 if i%3==0 else 7 if i%3==1 else 0;b[p+39]=1;b[p+42]=[3,9,12][(i+case//4)%3]
    struct.pack_into('<I',b,p+32,[0,0x400,0x800000][case//16%3])
   result.append((f'{variant}-terrain{case}',variant,rules_fixture(variant,[1,3,100][case//32]),bytes(b),True))
  named=[('custom-psi-fungus',320,12,3,0x60,0x20,0,0,False),
         ('predefined-psi-fungus',63,12,3,0x60,0x20,0,0,False),
         ('mindworm-fungus',8,12,3,0x60,0x20,0,0,False),
         ('spore-profile',15,0,3,0x60,0x20,0,0,False),
         ('former-fungus',320,12,9,0x60,0x20,0,3,False),
         ('artifact-fungus',320,0,12,0x60,0x20,0,3,False),
         ('xeno-fungus',320,0,3,0x60,0x20,0,0,True),
         ('sea-fungus',320,0,3,0x40,0x20,0,1,False),
         ('isle-sea',9,12,3,0x40,0x20,0,1,False),
         ('sealurk-profile',14,0,3,0x40,0x20,0,1,False),
         ('raw-rock-both-bits',320,0,3,0x60,0,0xc0,0,False),
         ('rock-forest',320,0,3,0x60,0x200000,0x80,0,False),
         ('unknown-items',320,0,3,0x60,0x80000000,0,0,False),
         ('mag-tube',320,0,3,0x60,8,0,0,False),
         ('road',320,0,3,0x60,4,0,0,False),
         ('river',320,0,3,0x60,0x80,0,0,False)]
  for name,unit,weapon,plan,alt,items_,rock,chassis,xeno in named:
   b=bytearray(world_fixture(version,0,1,1));info,o,_=decode(b);vp=o['vehicles'];struct.pack_into('<h',b,vp+10,unit)
   p=o['prototypes']+52*unit;b[p:p+52]=bytes(52);b[p+36]=chassis;b[p+37]=weapon;b[p+39]=1;b[p+42]=plan
   sf=next(f for f in schema['fields'] if f['name']=='SE_planet');struct.pack_into('<i',b,o['factions']+5*fs+sf['offset'],0)
   pref=o['preferences'] if version>=11 else o['legacy'];struct.pack_into('<i',b,pref+0x84+8*4,0 if xeno else -1)
   for index in range(int(info['width'])*int(info['height'])//2):
    p=o['tiles']+44*index;b[p]=alt;b[p+5]=rock;struct.pack_into('<I',b,p+8,items_)
   result.append((variant+'-'+name,variant,rules_fixture(variant),bytes(b),True))
  for nb in (0,2):result.append((f'{variant}-empty{nb}',variant,rules_fixture(variant),world_fixture(version,0,0,nb),True))
  b=bytearray(world_fixture(version,0));_,o,_=decode(b);pref=o['preferences'] if version>=11 else o['legacy'];struct.pack_into('<i',b,pref+0x84+8*4,512)
  result.append((variant+'-bad-xeno-reference',variant,rules_fixture(variant),bytes(b),False))
  b=bytearray(world_fixture(version,0));_,o,_=decode(b);b[o['prototypes']+320*52+37]=255
  result.append((variant+'-bad-component',variant,rules_fixture(variant),bytes(b),False))
  result.append((variant+'-wrong-profile',variant,rules_fixture(variant),world_fixture(12 if version==7 else 7,0),False))
 # Unused fields in the generic wire fixture are arbitrary bytes. Explicitly
 # initialise the owned definitions before exercising ability callbacks.
 cleaned=[]
 bonus_schema=SCHEMA['MFaction'];countoff=next(f for f in bonus_schema['fields'] if f['name']=='faction_bonus_count')['offset']
 valoff=next(f for f in bonus_schema['fields'] if f['name']=='faction_bonus_val1')['offset']
 for name,variant,rules,data,valid in result:
  b=bytearray(data);_,o,_=decode(b)
  for faction in range(8):struct.pack_into('<i',b,o['faction_definitions']+faction*0x59c+countoff,0)
  cleaned.append((name,variant,rules,bytes(b),valid))
 for variant in ['base','expansion']:
  # Grants use DESIGN owner5; vehicle owner6 is deliberately different.
  template=next(c for c in cleaned if c[0]==variant+'-former-fungus')
  for kind,ability,known_,owner in [('antigrav-known',10,True,5),('antigrav-locked',10,False,5),('antigrav-wrong-owner',10,True,6),('slow-known',23,True,5),('slow-locked',23,False,5),('bad-count',10,True,5),('bad-ability',99,True,5)]:
   b=bytearray(template[3]);_,o,_=decode(b);pref=o['preferences'] if variant=='expansion' else o['legacy'];shift=0 if variant=='expansion' else 0x7c
   b[o['vehicles']+14]=6;b[pref+0x1e0-shift]=(1<<owner) if known_ else 0
   definition=o['faction_definitions']+5*0x59c;struct.pack_into('<iii',b,definition+countoff,9 if kind=='bad-count' else 1,15,0);struct.pack_into('<i',b,definition+valoff,ability)
   cleaned.append((variant+'-free-'+kind,variant,template[2],bytes(b),not kind.startswith('bad-')))
 return cleaned

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'world-price-probe';files=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','world_movement_price.cpp','world_movement_price_probe.cpp'];cmd=[a.cc,*flags,*[str(SRC/n) for n in files],'-o',str(exe)];subprocess.run(cmd,check=True)
 records=[]
 for i,(name,variant,rules,data,valid) in enumerate(controls()):
  game=out/f'{i}.SC';text=out/f'{i}.txt';game.write_bytes(data);text.write_bytes(rules);r=subprocess.run([*runner,str(exe),variant,str(text),str(game)],capture_output=True,text=True,timeout=30,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  assert r.returncode==(0 if valid else 20),(name,r.returncode,r.stdout,r.stderr)
  expected=None
  if valid:
   expected=original_model(rules,data,variant);actual=dict(x.split('=') for x in r.stdout.split());assert actual==expected,(name,actual,expected)
  records.append(dict(name=name,valid=valid,passed=True,result=expected))
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,cases=records,binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(f'{len(records)} owned world-price cases passed')
if __name__=='__main__':main()
