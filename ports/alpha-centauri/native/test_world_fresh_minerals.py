#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh complete food/mineral proposal versus independent wire composition."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_world_convoy_nutrients as convoy
import test_world_unit_support as support
import test_base_minerals as mineral
HERE=Path(__file__).resolve().parent
def fixture(variant,version,case):
 b=bytearray(convoy.fixture(variant,version,case));info,o,s=convoy.intake.decode(b);legacy=version<11;fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308;schema=convoy.intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction'];pf=o['legacy'] if legacy else o['preferences']
 # Add controlled ordinary units to the existing reciprocal base stack.
 count=case%8;n=int(info['vehicles']);extra=bytearray(count*52)
 for i in range(count):
  p=52*i;struct.pack_into('<hhI',extra,p,8,6,0xdeadbeff);struct.pack_into('<h',extra,p+10,2);extra[p+14]=1;struct.pack_into('<hhh',extra,p+46,0,n+i+1 if i+1<count else -1,n+i-1 if i else 6)
 if count:struct.pack_into('<h',b,o['vehicles']+6*52+48,n)
 b[o['bases']:o['bases']]=extra;struct.pack_into('<i',b,pf+0x38,n+count);_,o,s=convoy.intake.decode(b)
 p=o['prototypes']+2*52;b[p:p+52]=bytes(52);b[p+39]=1;struct.pack_into('<I',b,p+32,0x10000 if case&8 else 0)
 for faction in [1,2]:
  for name,value in [('SE_support_pending',case%8-4 if faction==1 else 0),('clean_minerals_modifier',case%12),('major_atrocities',case%4),('tech_ranking',case+10),('tech_count_transcendent',case%5),('diff_level',case%6),('SE_planet_pending',case%7-3)]:
   f=next(f for f in schema['fields'] if f['name']==name);struct.pack_into('<i',b,o['factions']+faction*fs+f['offset'],value)
 b[o['bases']+bs+4]=2
 for j,bit in enumerate([21,19,22,20,31,32]):
  if case&(1<<j):b[o['bases']+0x8c+bit//8]|=1<<(bit%8)
 for j,project in enumerate([28,29,16,27]):struct.pack_into('<i',b,pf+0x84+project*4,[-1,0,1,-2][(case>>(2*j))&3])
 struct.pack_into('<i',b,pf+0x84+31*4,[-1,-2,0,1][case%4]);struct.pack_into('<i',b,pf+0xc,0x800 if case&1 else 0);struct.pack_into('<i',b,pf+0x30,0x10|(0x100000 if case&2 else 0));struct.pack_into('<i',b,pf+0x34,case%6)
 struct.pack_into('<i',b,o['bases']+0x50,[-34,-35,-65,-66,-68,320][case%6]);struct.pack_into('<i',b,o['metadata']+0xa48,case%3)
 if version>=12:struct.pack_into('<i',b,o['tectonic']+4,case%4)
 return bytes(b)
def model(variant,text,data,mode=0):
 fail=lambda e:[e]+[0x5a5a5a5a]*130
 food=convoy.model(variant,text,data)
 if food[0]:return fail(food[0])
 info,o,s=convoy.intake.decode(data);version=int(info['version']);legacy=version<11;fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308;b=s['bases'];owner=b[4];schema=convoy.intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction'];pref=s['legacy'] if legacy else s['preferences'];tables=convoy.intake.sections(text)
 def state(name):return convoy.intake.scalar(s['factions'][owner*fs:(owner+1)*fs],schema,name)
 def project(index):
  base=struct.unpack_from('<i',pref,0x84+index*4)[0]
  if base>=int(info['bases']):raise ValueError('project')
  return base>=0 and b[bs*base+4]==owner
 try:_,units=support.original_model(text,data,variant)
 except OverflowError:return fail(7)
 except ValueError:return fail(1)
 unit=units[0]
 if mode:
  tectonic=3 if mode==2 else 2147483647 if mode==3 else 0
  if variant=='base' and tectonic:return fail(1)
 elif variant=='expansion':
  if version<12:return fail(3)
  tectonic=struct.unpack_from('<i',s['tectonic'],4*owner)[0]
  if tectonic<0:return fail(1)
 else:tectonic=0
 # Incoming/outgoing mineral channels are food result words2/6; fresh intake
 # and raw ecology are words19/24. No saved accounting field is reused.
 signed=lambda n:(n+(1<<31))%(1<<32)-(1<<31)
 values=[food[19],food[2],food[6],unit[7],signed(food[24]),struct.unpack_from('<b',b,6)[0],state('clean_minerals_modifier'),state('satellites_mineral'),state('major_atrocities'),tectonic,struct.unpack_from('<i',s['metadata'],12)[0],5,state('tech_ranking'),state('tech_count_transcendent'),state('SE_planet_pending'),struct.unpack_from('<i',s['metadata'],0xa48)[0],state('diff_level'),struct.unpack_from('<i',pref,0x34)[0],struct.unpack_from('<i',b,0x50)[0]]
 # Rules WorldBuilder row sea_level_rises follows14 numeric rows.
 values[11]=int(tables['WORLDBUILDER'][14][0])
 bits=[bool(pref[0x58]&(1<<owner)),struct.unpack_from('<i',pref,0x84+31*4)[0]!=-1,bool(struct.unpack_from('<I',pref,0xc)[0]&0x800),bool(struct.unpack_from('<I',pref,0x30)[0]&0x100000)]
 fac=lambda bit:bool(b[0x8c+bit//8]&(1<<(bit%8)))
 try:bits += [fac(21),fac(19),fac(22),fac(20),project(28),project(29),fac(31),fac(32),project(16),project(27),variant=='expansion']
 except ValueError:return fail(1)
 flags=sum(int(value)<<i for i,value in enumerate(bits));result=mineral.model(values,flags)
 if result[0]:return fail(7 if result[0]==2 else 1)
 return [*food,*unit,*values,flags,*result[1:]]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--binary',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 names=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','world_unit_support.cpp','vehicle_stack.cpp','tile_yields.cpp','world_tile_yields.cpp','world_worker_radius.cpp','world_terraform_ecology.cpp','world_stable_intake.cpp','base_nutrients.cpp','world_fresh_nutrients.cpp','world_convoy_nutrients.cpp','base_minerals.cpp','world_fresh_minerals.cpp','world_fresh_minerals_probe.cpp'];exe=a.binary if a.binary else a.out/'world-fresh-minerals';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[] if a.binary else [a.cc,*flags,*[str(HERE/n) for n in names],'-o',str(exe)]
 if cmd:subprocess.run(cmd,check=True)
 runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 cases=[]
 for version in range(7,13):
  variant='base' if version<11 else 'expansion'
  for case in range(1,17):
   data=fixture(variant,version,case)
   for mode in [0,1,2]:cases.append((f'v{version}-c{case}-m{mode}',variant,data,mode))
 seed=fixture('expansion',12,16);_,o,_=convoy.intake.decode(seed)
 for name,offset,value in [('negative-tectonic',o['tectonic']+4,-1),('tectonic-max',o['tectonic']+4,2147483647),('native-invalid',o['metadata']+0xa48,3),('foreign-quantum-project',o['preferences']+0x84+28*4,2),('voice-destroyed',o['preferences']+0x84+31*4,-2)]:
  b=bytearray(seed);struct.pack_into('<i',b,offset,value);cases.append((name,'expansion',bytes(b),0))
 digest=hashlib.sha256();records=[];accepted=rejected=0
 for name,variant,data,mode in cases:
  text=convoy.nutrients.rules(variant);p=a.out/(name+'.SC');p.write_bytes(data);rp=a.out/(variant+'.txt');rp.write_bytes(text);want=model(variant,text,data,mode)
  r=subprocess.run([*runner,str(exe),variant,str(rp),str(p),str(mode)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr);encoded=struct.pack('<131I',*[x&0xffffffff for x in want]);assert r.stdout==encoded,(name,len(r.stdout),[(i,x,y) for i,(x,y) in enumerate(zip(struct.unpack('<131I',r.stdout),[v&0xffffffff for v in want])) if x!=y][:20]);assert p.read_bytes()==data;digest.update(encoded);accepted+=want[0]==0;rejected+=want[0]!=0;records.append({'name':name,'error':want[0],'inputSHA256':hashlib.sha256(data).hexdigest(),'outputSHA256':hashlib.sha256(encoded).hexdigest()})
 receipt={'worlds':len(cases),'accepted':accepted,'rejected':rejected,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'records':records,'command':cmd,'native':'UNRUN','scope':'Fresh read-only food/mineral proposal with actual convoy/support/project/map/faction facts; no accounting/actor/faction mutation/upkeep/turn/UI.'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
