#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned convoy -> fresh food composition against independent wire model."""
import argparse,hashlib,itertools,json,os,struct,subprocess
from functools import lru_cache
from pathlib import Path
import test_world_stable_intake as intake
import test_world_fresh_nutrients as nutrients
from test_base_nutrients import expected as nutrient_expected
HERE=Path(__file__).resolve().parent
@lru_cache(maxsize=160)
def seed(variant,version,case):
 b=intake.fixture(variant,version,case);_,off,spans=intake.decode(b);return b,off,spans
def fixture(variant,version,case,actors=None):
 raw,off,spans=seed(variant,version,case);b=bytearray(raw);bs=304 if version<11 else 308;fs=0x20bc if version<11 else 0x20cc
 if actors is None:
  actors=[(1,0,3,k,0,10) for k in range(3)]+[(1,0,3,k,1,10) for k in range(3)]+[(1,1,3,case%3,1,10)]
 # at=0 is off-radius (0,0), at=1 selected base (8,6), at=2 another tile (1,1).
 vs=bytearray(52*len(actors));positions={}
 for i,(faction,home,order,channel,at,plan) in enumerate(actors):
  x,y=[(0,0),(8,6),(1,1)][at];v=i*52;struct.pack_into('<hhI',vs,v,x,y,0xdeadbeff);struct.pack_into('<h',vs,v+10,0 if plan==10 else 1);vs[v+14]=faction;vs[v+17]=order;vs[v+38]=channel;struct.pack_into('<hhh',vs,v+46,home,-1,-1);positions.setdefault((x,y),[]).append(i)
 for (x,y),ids in positions.items():
  for j,i in enumerate(ids):struct.pack_into('<hh',vs,52*i+48,ids[j+1] if j+1<len(ids) else -1,ids[j-1] if j else -1)
  tile=off['tiles']+44*(y*8+x//2);items=struct.unpack_from('<I',b,tile+8)[0];struct.pack_into('<I',b,tile+8,items|2)
  if not items&1:b[tile+2]=actors[ids[0]][0]
 bp=off['bases'];second=bytearray(bs);struct.pack_into('<hhb',second,0,14,10,1);second[6]=1;struct.pack_into('<I',second,0x78,1)
 b[bp+bs:bp+bs]=second;b[bp:bp]=vs
 pf=off['legacy'] if version<11 else off['preferences'];struct.pack_into('<ii',b,pf+0x38,len(actors),2)
 for name,value in [('base_count',2),('SE_growth_pending',0),('nutrient_surplus_total',0)]:
  f=next(f for f in intake.FACTIONS['LegacyFactionV10' if version<11 else 'Faction']['fields'] if f['name']==name);struct.pack_into('<i',b,off['factions']+fs+f['offset'],value)
 for proto,plan in [(0,10),(1,11)]:
  p=off['prototypes']+52*proto;b[p:p+52]=bytes(52);struct.pack_into('<b',b,p+42,plan)
 tile=off['tiles']+44*(10*8+7);b[tile+2]=1;struct.pack_into('<I',b,tile+8,1)
 return bytes(b)
def flow_model(variant,text,data,mode=0):
 info,off,spans=intake.decode(data);bs=304 if int(info['version'])<11 else 308;b=spans['bases'][:bs];faction=b[4];to=[0]*4;fr=[0]*4;harvesters=0;reduction=-2 if mode==2 else 2147483647 if mode==3 else 0;energy=-123456 if mode>=2 else 0
 tables=intake.sections(text);row_index,column,_=intake.ROW['limit_mineral_mine_sans_road'];mine_limit=int(tables['RULES'][row_index][column])
 for i in range(int(info['vehicles'])):
  v=spans['vehicles'][52*i:52*(i+1)];unit=struct.unpack_from('<h',v,10)[0];plan=struct.unpack_from('<b',spans['prototypes'],52*unit+42)[0]
  if v[14]!=faction or plan!=10 or v[17]!=3:continue
  channel=v[38]
  if channel>=4:return 1,None
  home=struct.unpack_from('<h',v,46)[0];x,y=struct.unpack_from('<hh',v);sq=spans['tiles'][44*(y*8+x//2):44*(y*8+x//2+1)];items=struct.unpack_from('<I',sq,8)[0]
  if home==0:
   if items&1 and sq[2]&15<8:fr[channel]+=1;continue
   harvesters+=1
   if channel==3:reduction=0;continue
   if items&0x20:return 3,None
   # Independently reconstruct the controlled plain/forest fixture leaf facts.
   row=[0]*59;row[0]=int(variant=='expansion');row[1]=sq[0];row[2]=sq[2];row[4]=items;row[11:14]=[1,1,1];row[14]=2;row[18],row[19]=struct.unpack_from('<II',b,0x8c);row[20]=15;row[25]=mode&1;row[28]=mine_limit;row[30]=reduction;row[31]=energy
   for k,resource in enumerate(tables['RESOURCEINFO']):row[32+3*k:35+3*k]=[0 if x=='*' else int(x) for x in resource[1:4]]
   result=intake.leaf_expected(row)
   if result[0]:return (7 if result[0]==2 else 1),None
   to[channel]+=result[1+channel];reduction+=result[6+channel]
   if channel==2:energy=result[5]
   if any(not -(1<<31)<=n<(1<<31) for n in [*to,reduction]):return 7,None
  elif home>=0 and (x,y)==struct.unpack_from('<hh',b):to[channel]+=1
 return 0,[*to,*fr,harvesters,reduction,energy]
def model(variant,text,data,mode=0):
 fail=lambda e:[e]+[0x5a5a5a5a]*95
 error,flows=flow_model(variant,text,data,mode)
 if error:return fail(error)
 fresh=intake.model(variant,text,data,mode)
 if fresh[0]:return fail(fresh[0])
 # Stable base resource wrapper resets TerraformReduce after each worked tile.
 info,off,spans=intake.decode(data);b=spans['bases'];pop=struct.unpack_from('<b',b,6)[0]
 result=nutrient_expected([pop,0,fresh[7],flows[0],flows[4],2,0,0,0,0])
 if result[0]:return fail(7 if result[0]==3 else 1)
 return [0,*flows,*fresh[1:],*result[1:]]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--binary',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 names=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','vehicle_stack.cpp','tile_yields.cpp','world_tile_yields.cpp','world_worker_radius.cpp','world_terraform_ecology.cpp','world_stable_intake.cpp','base_nutrients.cpp','world_fresh_nutrients.cpp','world_convoy_nutrients.cpp','world_convoy_nutrients_probe.cpp']
 exe=a.binary if a.binary else a.out/'world-convoy-nutrients';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[] if a.binary else [a.cc,*flags,*[str(HERE/n) for n in names],'-o',str(exe)]
 if cmd:subprocess.run(cmd,check=True)
 runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 cases=[]
 for variant in ['base','expansion']:
  for n in range(1,65):cases.append((f'{variant}-{n}',variant,fixture(variant,7+n%4 if variant=='base' else 11+n%2,n),0))
 for n,actor in enumerate(itertools.product([1,2],[-1,0,1],[0,3],range(4),[0,1,2],[10,11])):
  version=7+n%6;variant='base' if version<11 else 'expansion';cases.append((f'v{version}-'+ '-'.join(map(str,actor)),variant,fixture(variant,version,32,[actor]),0))
 for version in range(7,13):
  variant='base' if version<11 else 'expansion';cases.append((f'empty-v{version}',variant,fixture(variant,version,32,[]),0))
 seed=fixture('expansion',12,32);_,o,_=intake.decode(seed)
 for name,offset,fmt,value in [('bad-channel',o['vehicles']+38,'B',4),('bad-channel255',o['vehicles']+38,'B',255),('harvest-fungus',o['tiles']+8,'I',0x22),('bad-stack',o['vehicles']+48,'h',0),('specialists',o['bases']+0x7c,'i',1)]:
  b=bytearray(seed);struct.pack_into('<'+fmt,b,offset,value);cases.append((name,'expansion',bytes(b),0))
 for mode in range(4):cases.append((f'scratch-{mode}','expansion',fixture('expansion',12,32),mode))
 overrides={}
 for name,mode in [('scratch-add-overflow',3),('scratch-add-negative',2)]:
  b=bytearray(fixture('expansion',12,32,[(1,0,3,1,0,10)]));_,o,_=intake.decode(b);struct.pack_into('<I',b,o['tiles']+8,0x12);cases.append((name,'expansion',bytes(b),mode));t=intake.sections(nutrients.rules('expansion'));row,col,_=intake.ROW['limit_mineral_mine_sans_road'];t['RULES'][row][col]='0';overrides[name]=intake.material(t)
 for name,owner,marked in [('foreign-base-transfer',7,1),('invalid-base-owner-harvest',8,1),('resource3-reset',9,0)]:
  b=bytearray(fixture('expansion',12,32,[(1,0,3,3 if name=='resource3-reset' else 0,0,10)]));_,o,_=intake.decode(b);b[o['tiles']+2]=owner;struct.pack_into('<I',b,o['tiles']+8,marked|2);cases.append((name,'expansion',bytes(b),3 if name=='resource3-reset' else 0))
 digest=hashlib.sha256();accepted=rejected=0;records=[]
 for name,variant,data,mode in cases:
  text=overrides.get(name,nutrients.rules(variant));p=a.out/(name+'.SC');p.write_bytes(data);rp=a.out/(variant+'.txt');rp.write_bytes(text)
  want=model(variant,text,data,mode);r=subprocess.run([*runner,str(exe),variant,str(rp),str(p),str(mode)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr)
  encoded=struct.pack('<96I',*[x&0xffffffff for x in want]);assert r.stdout==encoded,(name,len(r.stdout),[(i,x,y) for i,(x,y) in enumerate(zip(struct.unpack('<96I',r.stdout),[x&0xffffffff for x in want])) if x!=y][:20]);assert p.read_bytes()==data;digest.update(encoded);accepted+=want[0]==0;rejected+=want[0]!=0;records.append({'name':name,'error':want[0],'inputSHA256':hashlib.sha256(data).hexdigest(),'outputSHA256':hashlib.sha256(encoded).hexdigest()})
 receipt={'worlds':len(cases),'accepted':accepted,'rejected':rejected,'outputSHA256':digest.hexdigest(),'allInputsAndDecodedWorldUnchanged':True,'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
