#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Current saved-world human production costs versus independent wire model."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from functools import lru_cache
from test_original_world import valid_fixture
from test_original_game import decode
from test_original_faction import SCHEMA as FACTIONS
from test_world_unit_stats import rules_fixture,scalar
from test_world_tile_yields import ROW
from test_units import sections,material
from test_facility_definitions import tables as facility_tables,sections as facility_sections
from test_prototype import decode as unit_tables,cost as component_cost
from test_production_cost import model as raw_price
from test_original_human_cost import model as factor_model
HERE=Path(__file__).resolve().parent
NAMES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','original_human_cost.cpp','production_cost.cpp','world_production_cost.cpp','world_production_cost_probe.cpp']
S=0x31415926
cached_world=lru_cache(maxsize=4)(decode)
cached_units=lru_cache(maxsize=4)(unit_tables)
cached_sections=lru_cache(maxsize=4)(sections)
cached_facilities=lru_cache(maxsize=4)(facility_sections)

def rules(variant,case):
 t=sections(rules_fixture(variant));_,fac=facility_tables(variant);t['FACILITIES']=fac
 # Valid zero/negative facility costs are retained by the parser; pricing rejects
 # negative costs instead of treating them as completed production.
 for name,value in [('mineral_cost_multi',[1,10,100][case%3]),('extra_pct_cost_proto_land',[0,50,500][case%3]),('extra_pct_cost_proto_sea',[0,100,500][(case+1)%3]),('extra_pct_cost_proto_air',[0,150,500][(case+2)%3])]:
  row,col,_=ROW[name];t['RULES'][row][col]=str(value)
 return material(t)

def fixture(version,case):
 b=bytearray(valid_fixture(version,3,2));_,off,_=decode(b);old=version<11;fs=0x20bc if old else 0x20cc;bs=304 if old else 308;pf=off['legacy'] if old else off['preferences'];schema=FACTIONS['LegacyFactionV10' if old else 'Faction']
 b[pf+0x58]=6
 for i in range(2):
  bp=off['bases']+i*bs;b[bp+4]=i+1;b[bp+0x8c:bp+0x8c+(8 if old else 12)]=bytes(8 if old else 12)
  for bit in [10,35]:
   if case&(1<<((bit==35)+i)):b[bp+0x8c+bit//8]|=1<<(bit%8)
 for owner in range(8):
  fp=off['factions']+owner*fs
  for name,value in [('diff_level',(case+owner)%6),('SE_growth_pending',[-2147483648,0,2147483647][(case+owner)%3]),('SE_industry_pending',[-2147483648,-8,-7,-3,-2,-1,0,1,2,3,4,5,2147483647][(case+owner)%13])]:
   f=next(f for f in schema['fields'] if f['name']==name);struct.pack_into('<i',b,fp+f['offset'],value)
  struct.pack_into('<I',b,off['faction_definitions']+owner*0x59c+0x508,0x200 if case&(1<<(owner%5)) else 0)
 struct.pack_into('<i',b,off['metadata']+0xa30,case%7)
 wc,ac=(21,10) if old else (26,14)
 for idx in range(512):
  p=off['prototypes']+idx*52;b[p:p+52]=bytes(52);b[p+36]=idx%9;b[p+37]=idx%wc;b[p+38]=idx%ac;b[p+39]=1+idx%4;b[p+41]=(case*17+idx)%256;b[p+42]=[0,8,10,255][(case+idx)%4]
  struct.pack_into('<I',b,p+32,0xffffffff) # component base cost MUST ignore abilities
  struct.pack_into('<H',b,p+48,4 if (case+idx)%3==0 else 0)
 return bytes(b)

def queries(data):
 info,_,_=decode(data);n=int(info['bases']);version=int(info['version']);last=71 if version<11 else 133;repeat,project=(34,39) if version<11 else (65,70)
 ids=[0,1,15,63,64,65,127,128,191,255,256,319,320,383,448,511,-1,-2,-10,-35,-repeat,-project,-last,-2147483648,512,-last-1]
 return [(i,item) for i in range(n) for item in ids]+[(-1,0),(n,0)]

def model(text,data,variant,base,item):
 fail=lambda e:(e,S,S,S,S,S)
 info,_,spans=cached_world(data);version=int(info['version']);old=version<11
 if old!=(variant=='base') or not 0<=base<int(info['bases']):return fail(1)
 fs=0x20bc if old else 0x20cc;bs=304 if old else 308;schema=FACTIONS['LegacyFactionV10' if old else 'Faction'];b=spans['bases'][base*bs:(base+1)*bs];faction=b[4];prefs=spans['legacy'] if old else spans['preferences']
 if not 1<=faction<=7 or not prefs[0x58]&(1<<faction):return fail(1)
 t=cached_sections(text)
 def rule(name):
  r,c,bounds=ROW[name];return max(bounds[0],min(bounds[1],int(t['RULES'][r][c])))
 def state(f,name):return scalar(spans['factions'][f*fs:(f+1)*fs],schema,name)
 sizes=struct.unpack_from('<i',spans['metadata'],0xa30)[0]
 result=factor_model(rule('nutrient_cost_multi'),rule('mineral_cost_multi'),sizes,state(faction,'SE_growth_pending'),state(faction,'SE_industry_pending'),0)
 if result[0]:return fail(result[0])
 factor=result[2];bc=surcharge=0
 if item<0:
  rows=cached_facilities(text)['facilities']
  if not 1<=-item<=len(rows):return fail(1)
  price=int(rows[-item-1][1]);q=1 if price<0 else 2 if price*factor>2147483647 else 0
  return fail(q) if q else (0,factor,price,0,0,price*factor)
 if item>=512:return fail(1)
 p=spans['prototypes'][item*52:(item+1)*52];units=cached_units(text,variant);done=bool(struct.unpack_from('<H',p,48)[0]&4);offense=triad=diff=0;free=False
 if not old and item<64:
  if p[37]>=len(units['weapons']):return fail(1)
  offense=units['weapons'][p[37]]['offense']
 if item>=64 and not done:
  if p[36]>=9 or p[37]>=len(units['weapons']) or p[38]>=len(units['armor']) or not 1<=p[39]<=4:return fail(1)
  bc=component_cost(units,p[36],p[37],p[38],0,p[39]);triad=units['chassis'][p[36]]['triad'];diff=state(item//64,'diff_level');free=bool(struct.unpack_from('<I',spans['faction_definitions'],item//64*0x59c+0x508)[0]&0x200)
 fac=lambda bit:bool(b[0x8c+bit//8]&(1<<(bit%8)))
 row=(int(not old),item,p[41],bc,triad,offense,p[42],diff,int(free),int(done),1,int(fac(35)),int(fac(10)),rule('extra_pct_cost_proto_land'),rule('extra_pct_cost_proto_sea'),rule('extra_pct_cost_proto_air'),0,0)
 out=raw_price(row)
 if out[0]:return fail(out[0])
 price,surcharge=out[1:3]
 if price*factor>2147483647:return fail(2)
 return 0,factor,price,bc,surcharge,price*factor

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--binary',type=Path);ap.add_argument('--owner-dir',type=Path);ap.add_argument('--scenario-dir',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 exe=a.binary or out/'world-production-cost-probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[] if a.binary else [a.cc,*flags,*[str(HERE/n) for n in NAMES],'-o',str(exe)]
 if cmd:subprocess.run(cmd,check=True)
 runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];cases=[]
 for version in range(7,13):
  variant='base' if version<11 else 'expansion'
  for case in range(32):cases.append((f'v{version}-{case}',variant,rules(variant,case),fixture(version,case)))
 for version in [7,12]:
  variant='base' if version<11 else 'expansion';base=fixture(version,0);_,off,_=decode(base);old=version<11;fs=0x20bc if old else 0x20cc;schema=FACTIONS['LegacyFactionV10' if old else 'Faction'];diff=next(f['offset'] for f in schema['fields'] if f['name']=='diff_level')
  changes=[('AI-base',off['legacy' if old else 'preferences']+0x58,'B',0),('map-invalid',off['metadata']+0xa30,'i',7),('owner-diff-invalid',off['factions']+5*fs+diff,'i',6)]
  for idx in [0,64,320]:
   changes.extend([(f'bad-chassis-{idx}',off['prototypes']+52*idx+36,'B',255),(f'bad-weapon-{idx}',off['prototypes']+52*idx+37,'B',255),(f'bad-reactor-{idx}',off['prototypes']+52*idx+39,'B',0)])
  for name,pos,fmt,value in changes:
   b=bytearray(base);struct.pack_into('<'+fmt,b,pos,value);cases.append((f'{variant}-{name}',variant,rules(variant,0),bytes(b)))
  text=rules(variant,0);t=sections(text);t['FACILITIES'][9][1]='2147483647'
  row,col,_=ROW['mineral_cost_multi'];t['RULES'][row][col]='10'
  b=bytearray(base);struct.pack_into('<i',b,off['metadata']+0xa30,2)
  industry=next(f['offset'] for f in schema['fields'] if f['name']=='SE_industry_pending')
  for faction in [1,2]:struct.pack_into('<i',b,off['factions']+faction*fs+industry,0)
  cases.append((variant+'-threshold-overflow',variant,material(t),bytes(b)))
  b=bytearray(base);p=off['prototypes']+64*52;b[p+36:p+40]=bytes([255,255,255,0]);struct.pack_into('<H',b,p+48,4)
  struct.pack_into('<i',b,off['factions']+fs+diff,6)
  cases.append((variant+'-typed-short-circuit',variant,rules(variant,0),bytes(b)))
  cases.append((variant+'-no-bases',variant,rules(variant,0),valid_fixture(version,0,0)))
 if a.owner_dir or a.scenario_dir:
  assert a.owner_dir and a.scenario_dir
  for name in ['Explore.SC','Build.SC','conquer.SC']:cases.append(('original-'+name,'base',(a.owner_dir/'alpha.txt').read_bytes(),(a.scenario_dir/name).read_bytes()))
 digest=hashlib.sha256();records=[];accepted=invalid=overflow=0
 for name,variant,text,data in cases:
  fp=out/(name+'.SC');fp.write_bytes(data);rp=out/(name+'.txt');rp.write_bytes(text);qs=queries(data);qp=out/(name+'.queries');qp.write_bytes(b''.join(struct.pack('<2i',*q) for q in qs));expected=[model(text,data,variant,*q) for q in qs];want=b''.join(struct.pack('<6i',*r) for r in expected)
  r=subprocess.run([*runner,str(exe),variant,str(rp),str(fp),str(qp)],capture_output=True,timeout=90,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr,len(r.stdout));assert len(r.stdout)==len(want),(name,len(r.stdout),len(want))
  for i,q in enumerate(qs):assert r.stdout[i*24:(i+1)*24]==want[i*24:(i+1)*24],(name,q,expected[i],struct.unpack_from('<6i',r.stdout,i*24))
  assert fp.read_bytes()==data;digest.update(want);accepted+=sum(x[0]==0 for x in expected);invalid+=sum(x[0]==1 for x in expected);overflow+=sum(x[0]==2 for x in expected);(out/(name+'.bin')).write_bytes(r.stdout);records.append(dict(name=name,queries=len(qs),inputSHA256=hashlib.sha256(data).hexdigest(),rulesSHA256=hashlib.sha256(text).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest()))
 receipt=dict(worlds=len(cases),queries=accepted+invalid+overflow,accepted=accepted,invalid=invalid,overflow=overflow,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),records=records,command=cmd,native='UNRUN',scope='Read-only human production costs from current owned World. Snapshot all decoded/opaque world/table bytes and runtime RNG; output preserved on errors, aliases/wrong edition/unloaded/index guards. No mutation/completion/turn/UI.');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
