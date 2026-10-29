#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original unit mineral upkeep from owned wire records, no turn mutation."""
import argparse,hashlib,json,os,struct,subprocess
from functools import lru_cache
from pathlib import Path
from test_world_unit_stats import world_fixture,rules_fixture,scalar
from test_original_game import decode,SRC,FORMATS
from test_original_faction import SCHEMA
from test_original_map import word
from test_original_world import valid_fixture
from test_units import reference,sections,material

@lru_cache(maxsize=16)
def tables(rules,variant):
 t=sections(rules);keys={r[1].lower():i for i,r in enumerate(t['TECHNOLOGY'])};keys.update(none=-1,disable=-2)
 return reference(rules,variant),t,keys

def original_model(rules,data,variant):
 info,_,s=decode(data);version=int(info['version']);table,t,keys=tables(rules,variant)
 assert (version<11)==(variant=='base')
 fs=0x20bc if version<11 else 0x20cc;bs=304 if version<11 else 308;schema=SCHEMA['LegacyFactionV10' if version<11 else 'Faction']
 pref=s['legacy'] if version<11 else s['preferences'];shift=0x7c if version<11 else 0;owners=pref[0x1e0-shift:0x1e0-shift+89]
 width=int(info['width']);digest=2166136261;results=[]
 def known(id,faction):
  if not faction:return False
  if id==-1:return True
  if not 0<=id<88:return False
  row=t['TECHNOLOGY'][id];a,b=keys[row[6].lower()],keys[row[7].lower()]
  return a!=-2 and not(b==-2 and a!=-1) and bool(owners[id]&(1<<faction))
 for base_id in range(int(info['bases'])):
  base=s['bases'][base_id*bs:(base_id+1)*bs];faction=base[4];population=struct.unpack_from('<b',base,6)[0]
  if population<0:raise ValueError('negative population')
  rating=scalar(s['factions'][faction*fs:(faction+1)*fs],schema,'SE_support_pending')
  if rating>2147483643:raise OverflowError('support+4')
  free={-4:0,-3:0,-2:1,-1:1,0:2,1:3,2:4,3:max(4,population)}[max(-4,min(3,rating))]
  supported=clean_exempt=native_exempt=0
  for i in range(int(info['vehicles'])):
   v=s['vehicles'][i*52:(i+1)*52]
   if v[14]!=faction or struct.unpack_from('<h',v,46)[0]!=base_id:continue
   id=struct.unpack_from('<h',v,10)[0];p=s['prototypes'][id*52:(id+1)*52]
   if p[42]>=10:continue
   # Qualified component profile, including its original flag-width domain.
   abilities=struct.unpack_from('<I',p,32)[0]
   if p[36]>=9 or p[37]>=int(table['weapon_count']) or p[38]>=int(table['armor_count']) or not 1<=p[39]<=4 or abilities&~((1<<int(table['ability_count']))-1):raise ValueError('bad design')
   clean=bool(abilities&(1<<16));owner=id//64
   if variant=='expansion' and owner and not clean:
    f=s['faction_definitions'][owner*0x59c:(owner+1)*0x59c];count=scalar(f,SCHEMA['MFaction'],'faction_bonus_count')
    if not 0<=count<=8:raise ValueError('bad bonus count')
    for j in range(count):
     if scalar(f,SCHEMA['MFaction'],'faction_bonus_id',j)!=15:continue
     ability=scalar(f,SCHEMA['MFaction'],'faction_bonus_val1',j)
     if not 0<=ability<int(table['ability_count']):raise ValueError('bad ability')
     if ability==16 and known(int(table['abilities.16.prerequisite']),owner):clean=True
   if clean:clean_exempt+=1;continue
   native=id<64 and (int(table[f'weapons.{p[37]}.offense'])<0 or (variant=='expansion' and id==15))
   x,y=struct.unpack_from('<hh',v);tile=(y*width+x)//2*44
   fungus=struct.unpack_from('<I',s['tiles'],tile+8)[0]&0x20
   if native and fungus and s['tiles'][tile]&0xe0>=0x40:native_exempt+=1;continue
   supported+=1
  maintained=max(0,supported-free);per=2 if rating< -3 else 1
  row=[rating,free,per,supported,maintained,clean_exempt,native_exempt,maintained*per]
  if row[-1]>2147483647:raise OverflowError('cost')
  results.append(row)
  for n in [base_id,*row]:digest=word(digest,n)
 return dict(bases=str(info['bases']),digest=f'{digest:08x}',unchanged='1',seed='abcdef01',rc='0'),results

def fixture(version,rating=0,population=7,altitude=0x60,fungus=False,clean_mode=0,vehicles=9):
 b=bytearray(world_fixture(version,0,vehicles=vehicles));info,o,s=decode(b)
 fs=0x20bc if version<11 else 0x20cc;bs=304 if version<11 else 308;schema=SCHEMA['LegacyFactionV10' if version<11 else 'Faction']
 field=next(f for f in schema['fields'] if f['name']=='SE_support_pending')['offset']
 for f in range(8):struct.pack_into('<i',b,o['factions']+f*fs+field,rating if f==5 else 0)
 b[o['bases']+6]=population&255;b[o['bases']+bs+6]=7
 for i in range(vehicles):
  v=o['vehicles']+52*i;b[v+14]=5;b[v+17]=0;struct.pack_into('<h',b,v+46,0)
  id=struct.unpack_from('<h',b,v+10)[0];p=o['prototypes']+52*id;b[p+42]=0
  struct.pack_into('<I',b,p+32,(1<<16) if clean_mode==1 and id>=64 else 0)
 for i in range(int(info['width'])//2*int(info['height'])):
  p=o['tiles']+44*i;b[p]=altitude;b[p+2]=5;struct.pack_into('<I',b,p+8,1|(0x20 if fungus else 0))
 if clean_mode>=2:
  f=o['faction_definitions']+5*0x59c;struct.pack_into('<iii',b,f+1292,1,15,0);struct.pack_into('<i',b,f+1328,16)
  pref=o['legacy'] if version<11 else o['preferences'];shift=0x7c if version<11 else 0;b[pref+0x1e0-shift]=(1<<5) if clean_mode==2 else 0
 return bytes(b)

def controls():
 result=[]
 for variant,version in [('base',7),('expansion',12)]:
  rules=rules_fixture(variant)
  for rating in range(-4,4):
   for population in [0,7]:
    for altitude,fungus in [(0x20,True),(0x40,True),(0x60,True),(0x60,False)]:
     for clean in range(4):result.append((f'{variant}-se{rating}-pop{population}-alt{altitude}-fungus{fungus}-clean{clean}',variant,rules,fixture(version,rating,population,altitude,fungus,clean),True))
  for rating in [-2147483648,-8,6,2147483643,2147483644,2147483647]:result.append((f'{variant}-support-boundary{rating}',variant,rules,fixture(version,rating),rating<=2147483643))
  for n in [0,1,2,3,4,5,7,9]:result.append((f'{variant}-units{n}',variant,rules,fixture(version,0,vehicles=n),True))
  for count,a,raw in [(9,16,False),(1,99,False),(9,16,True)]:
   b=bytearray(fixture(version,clean_mode=1 if raw else 0));_,o,_=decode(b);f=o['faction_definitions']+5*0x59c;struct.pack_into('<iii',b,f+1292,count,15,0);struct.pack_into('<i',b,f+1328,a)
   result.append((f'{variant}-bonus{count}-ability{a}-raw{raw}',variant,rules,bytes(b),variant=='base' or raw))
  b=bytearray(fixture(version));_,o,_=decode(b);b[o['bases']+6]=255;result.append((variant+'-negative-population',variant,rules,bytes(b),False))
  for state in [0,0x600010]:
   b=bytearray(fixture(version));_,o,_=decode(b)
   for i in range(9):struct.pack_into('<I',b,o['vehicles']+52*i+4,state)
   result.append((variant+'-old-state'+str(state),variant,rules,bytes(b),True))
  for kind in ['no-home','foreign-faction']:
   b=bytearray(fixture(version));_,o,_=decode(b)
   for i in range(9):
    if kind=='no-home':struct.pack_into('<h',b,o['vehicles']+52*i+46,-1)
    else:b[o['vehicles']+52*i+14]=6
   result.append((variant+'-'+kind,variant,rules,bytes(b),True))
  # Every possible plan byte in one world, to check unsigned comparison incl -1/-2.
  b=bytearray(valid_fixture(version,256,2));_,o,_=decode(b);fs=0x20bc if version<11 else 0x20cc;bs=304 if version<11 else 308
  schema=SCHEMA['LegacyFactionV10' if version<11 else 'Faction'];field=next(f for f in schema['fields'] if f['name']=='SE_support_pending')['offset']
  for f in range(8):struct.pack_into('<i',b,o['factions']+f*fs+field,0);struct.pack_into('<i',b,o['faction_definitions']+f*0x59c+1292,0)
  for i in range(2):b[o['bases']+i*bs+4]=5;b[o['bases']+i*bs+6]=7
  for i in range(256):
   v=o['vehicles']+52*i;struct.pack_into('<h',b,v+10,i);b[v+14]=5;b[v+17]=0;struct.pack_into('<h',b,v+46,0);p=o['prototypes']+52*i;b[p:p+52]=bytes(52);b[p+39]=1;b[p+42]=i
  result.append((variant+'-all-plan-bytes',variant,rules,bytes(b),True))
 return result

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'unit-support-probe';files=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','world_unit_support.cpp','world_unit_support_probe.cpp'];cmd=[a.cc,*flags,*[str(SRC/n) for n in files],'-o',str(exe)];subprocess.run(cmd,check=True);records=[]
 for i,(name,variant,rules,data,valid) in enumerate(controls()):
  rule=out/f'{i}.txt';game=out/f'{i}.SC';rule.write_bytes(rules);game.write_bytes(data);r=subprocess.run([*runner,str(exe),variant,str(rule),str(game)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30)
  assert r.returncode==(0 if valid else 20),(name,r.returncode,r.stdout,r.stderr)
  expected=None;rows=None
  if valid:expected,rows=original_model(rules,data,variant);assert dict(x.split('=') for x in r.stdout.split())==expected,(name,r.stdout,expected)
  records.append(dict(name=name,valid=valid,passed=True,result=expected,rows=rows))
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,cases=records,binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(f'{len(records)} owned unit-support contexts passed')
if __name__=='__main__':main()
