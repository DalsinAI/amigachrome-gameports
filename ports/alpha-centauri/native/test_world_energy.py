#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned world/fresh energy against independent wire, convoy and tile models."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_world_convoy_nutrients as convoy
import test_world_stable_intake as intake
from test_base_energy import expected as energy_expected
HERE=Path(__file__).resolve().parent
NAMES='base_energy rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats vehicle_stack tile_yields world_technology_effects world_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients world_energy world_fresh_energy world_energy_probe'.split()
def fixture(variant,version,case):
 # Mix at-base/exterior/foreign/otherhome/wrongplan/channel/order convoys.
 actors=[(1,0,3,2,1,10),(1,1,3,2,1,10),(1,0,3,2,0,10),(2,0,3,2,1,10),(1,0,0,2,1,10),(1,0,3,1,1,10),(1,0,3,2,1,11)]
 b=bytearray(convoy.fixture(variant,version,32+case%4,actors));_,off,_=intake.decode(b);fs=0x20bc if version<11 else 0x20cc;bs=304 if version<11 else 308;schema=intake.FACTIONS['LegacyFactionV10' if version<11 else 'Faction']
 def at(name):return off['factions']+fs+next(f['offset']for f in schema['fields']if f['name']==name)
 struct.pack_into('<i',b,at('SE_effic_pending'),case%13-6);struct.pack_into('<9i',b,at('social_effic'),*range(9))
 # physicalHQ at currentbase/secondbase/missing; queuedHQ never qualifies.
 for i in range(2):b[off['bases']+i*bs+0x8c]&=~6
 if case%3<2:b[off['bases']+(case%3)*bs+0x8c]|=2
 if case&1:b[off['bases']+0x8c]|=4
 struct.pack_into('<i',b,off['bases']+0x50,-1)
 # Force one marked convoy destination to foreign ownership without changing records.
 if case&2:b[off['tiles']+44*52+2]=2
 # Saved energy values deliberately absurd: not used by either accounting route.
 for delta in [0xb8,0xc8,0xd8,0xe8,0xf8]:struct.pack_into('<i',b,off['bases']+delta-(4 if version<11 and delta>=0x98 else 0),-987654321)
 return bytes(b)
def model(variant,text,data):
 info,off,spans=intake.decode(data);v=int(info['version']);fs=0x20bc if v<11 else 0x20cc;bs=304 if v<11 else 308;schema=intake.FACTIONS['LegacyFactionV10'if v<11 else 'Faction'];base=spans['bases'][:bs];owner=base[4];f=spans['factions'][owner*fs:(owner+1)*fs];w=int(info['width']);flat=struct.unpack_from('<I',spans['metadata'],28)[0]
 field=lambda name:next(x['offset']for x in schema['fields']if x['name']==name)
 eff=struct.unpack_from('<i',f,field('SE_effic_pending'))[0];stats=list(struct.unpack_from('<9i',f,field('social_effic')));creche=int(bool(base[0x8c]&4));x,y=struct.unpack_from('<hh',base);dist=999
 for i in range(int(info['bases'])):
  b=spans['bases'][i*bs:(i+1)*bs]
  if b[4]!=owner or not b[0x8c]&2:continue
  xx,yy=struct.unpack_from('<hh',b);dx=abs(x-xx);dy=abs(y-yy)
  if not flat and dx>w//2:dx=w-dx
  radius=max(dx,dy)-((dx+dy)//2-min(dx,dy)+1)//2;dist=min(dist,radius)
 dist=-1 if dist==999 else dist
 def account(fresh,to,fr,commerce=0,grid=0,stage=0):
  leaf=energy_expected([fresh,to,commerce,grid,fr,dist,eff,stage,creche,*stats]);return [leaf[0],dist,eff,creche,*leaf[1:]]if leaf[0]==0 else [leaf[0]]
 direct=account(23,2,7,3,0,1);bits=[0]*64;count=0
 for i in range(int(info['vehicles'])):
  a=spans['vehicles'][52*i:52*(i+1)];proto=struct.unpack_from('<h',a,10)[0];plan=struct.unpack_from('<b',spans['prototypes'],proto*52+42)[0]
  if a[14]!=owner or plan!=10 or a[17]!=3 or a[38]!=2:continue
  xx,yy=struct.unpack_from('<hh',a);tile=spans['tiles'][44*(yy*(w//2)+xx//2):44*(yy*(w//2)+xx//2+1)]
  if struct.unpack_from('<I',tile,8)[0]&1 and tile[2]&15<8:bits[i//32]|=1<<(i%32);count+=1
 short=[0,count,*bits]
 e,flows=convoy.flow_model(variant,text,data);fresh=intake.model(variant,text,data)
 if e:actualfresh=[e]
 elif fresh[0]:actualfresh=[fresh[0]]
 else:
  a=account(fresh[9],flows[2],flows[6]);actualfresh=[a[0],*a[1:],*flows[:8],count if a[7]<0 else 0]
 return dict(account=direct,shortfall=short,fresh=actualfresh)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--binary',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.binary or a.out/'world-energy-probe';cmd=[]
 if not a.binary:
  cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(n+'.cpp'))for n in NAMES],'-o',str(exe)];subprocess.run(cmd,check=True)
 records=[]
 for version in range(7,13):
  variant='base'if version<11 else 'expansion';text=intake.rules(variant);rule=a.out/(variant+'.txt');rule.write_bytes(text)
  for case in range(18):
   data=fixture(variant,version,case);path=a.out/f'v{version}-{case}.SC';path.write_bytes(data);args=[str(exe),variant,str(rule),str(path)];run=subprocess.run(args,capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30);assert run.returncode==0,(args,run.returncode,run.stdout,run.stderr)
   got={line.split()[0]:list(map(int,line.split()[1:]))for line in run.stdout.splitlines()};want=model(variant,text,data);assert got==want,(version,case,got,want)
   records.append(dict(name=path.name,sha256=hashlib.sha256(data).hexdigest(),passed=True,result=got))
 receipt=dict(cases=len(records),command=cmd,casesDetail=records,native='UNRUN',source='Original black_market/base_energy_costs + fresh stable/convoy adapters; full allocation remains excluded');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(records)} connected energy cases passed')
if __name__=='__main__':main()
