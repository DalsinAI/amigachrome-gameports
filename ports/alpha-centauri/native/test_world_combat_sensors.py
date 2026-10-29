#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent sensor/GSP radius and nearest-global-base raw-file model."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_original_map import fixture as map_fixture
HERE=Path(__file__).resolve().parent
OFFSETS=[(0,0),(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2),(2,-2),(2,2),(-2,2),(-2,-2),(1,-3),(3,-1),(3,1),(1,3),(-1,3),(-3,1),(-3,-1),(-1,-3),(4,0),(-4,0),(0,4),(0,-4)]

def model(data):
 info,_,spans=decode(data);w=int(info['width']);h=int(info['height']);flat=struct.unpack_from('<I',spans['metadata'],28)[0]!=0
 bs=308 if int(info['version'])>=11 else 304;bases=[]
 for i in range(int(info['bases'])):
  b=spans['bases'][i*bs:(i+1)*bs];bases.append((*struct.unpack_from('<hh',b),b[4],bool(b[0x8c+5]&2)))
 def xdist(a,b):
  d=abs(a-b);return w-d if not flat and d>w//2 else d
 def nearest(x,y):
  def key(b):
   a=xdist(x,b[0]);c=abs(y-b[1]);return max(a,c)-(((a+c)//2-min(a,c)+1)//2)
  return min(enumerate(bases),key=lambda ib:(key(ib[1]),-ib[0]))[1]if bases else None
 result=[]
 for faction in range(1,8):
  for index in range(w//2*h):
   y=index//(w//2);x=2*(index%(w//2))+(y&1);mask=0;error=0
   for ox,oy in OFFSETS:
    sx=x+ox;sy=y+oy
    if not flat:sx%=w
    if not(0<=sx<w and 0<=sy<h):continue
    t=(sy*w+sx)//2*44;b=spans['tiles'][t:t+44];owner=struct.unpack_from('<b',b,7)[0]
    if owner>7:error=1;break
    near=nearest(sx,sy);status=0
    if struct.unpack_from('<I',b,8)[0]&0x80000000:status=1
    elif near and near[3] and xdist(sx,near[0])in[0,2] and abs(sy-near[1])in[0,2]:status=2
    if not status:continue
    qualify=(near is not None and near[2]==faction)if b[0]&0xe0<0x60 else(owner<0 or owner==faction)
    if qualify:
     mask|=1
     if status==2:mask|=2;break
   result.extend([error,0x5a5a5a5a if error else mask])
 return struct.pack('<'+'I'*len(result),*result)

def fixtures():
 rows=[];rng=random.Random(0x501da0)
 for case in range(40):
  version=[7,9,12][case%3];nb=[0,1,2,4][case%4];data=bytearray(valid_fixture(version,0,nb));_,off,spans=decode(data)
  width,height=(16,12)if case>=32 else(8,6)
  if case>=32:
   data[off['metadata']:off['abstract']+len(spans['abstract'])]=map_fixture(width,height)[15:];_,off,spans=decode(data)
  struct.pack_into('<I',data,off['metadata']+28,case%2)
  for i in range(width//2*height):
   p=off['tiles']+44*i;data[p:p+44]=bytes(44);data[p]=[0x20,0x40,0x60,0x80][rng.randrange(4)];data[p+2]=15;data[p+3]=1;data[p+4]=255;data[p+7]=rng.choice([-1,1,5,6,7])&255
   struct.pack_into('<I',data,p+8,0x80000000 if rng.randrange(8)==0 else 0)
  bs=308 if version>=11 else 304
  for i in range(nb):
   p=off['bases']+bs*i;data[p:p+bs]=bytes(bs);x=[0,2,6,4][i];y=2 if case&4 else 4;struct.pack_into('<hh',data,p,x,y);data[p+4]=[5,6,7,1][i]
   if case&8 or i%2:data[p+0x8c+5]|=2
  # Equal-distance bases can have distinct factions and GSP states. Last wins.
  rows.append((f'random-{case}',bytes(data)))
 # Deterministic raw-sensor/GSP precedence, land ownership and nearest sea owner.
 for name,flat,sensor,owner,alt,bases in [
  ('raw-only',1,True,-1,0x60,[]),('raw-foreign',1,True,5,0x60,[]),('ocean-no-base',1,True,-1,0x20,[]),
  ('gsp-neutral-land',1,False,-1,0x60,[(0,2,5,True)]),('raw-overrides-gsp',1,True,-1,0x60,[(0,2,5,True)]),
  ('tie-last-gsp',1,False,-1,0x60,[(0,2,5,False),(4,2,6,True)]),('tie-last-no-gsp',1,False,-1,0x60,[(0,2,5,True),(4,2,6,False)]),
  ('sea-last-base-owner',1,True,-1,0x20,[(0,2,5,False),(4,2,6,False)]),('cylinder-seam-gsp',0,False,-1,0x60,[(0,2,5,True)]),
 ]:
  data=bytearray(valid_fixture(9,0,len(bases)));_,o,_=decode(data);struct.pack_into('<I',data,o['metadata']+28,flat)
  for i in range(24):
   p=o['tiles']+44*i;data[p:p+44]=bytes(44);data[p]=alt;data[p+2]=15;data[p+3]=1;data[p+4]=255;data[p+7]=owner&255
   if sensor:struct.pack_into('<I',data,p+8,0x80000000)
  for i,(x,y,faction,gsp)in enumerate(bases):
   p=o['bases']+304*i;data[p:p+304]=bytes(304);struct.pack_into('<hh',data,p,x,y);data[p+4]=faction;data[p+0x8c+5]=2 if gsp else 0
  rows.append((name,bytes(data)))
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='c++');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 names='original_map original_game original_faction original_world prototype movement combat_rounds world_combat_sensors world_combat_sensors_probe'.split();exe=a.out/'world-combat-sensors-probe';cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(n+'.cpp'))for n in names],'-o',str(exe)];subprocess.run(cmd,check=True)
 records=[];all_bytes=b''
 for i,(name,data)in enumerate(fixtures()):
  path=a.out/f'{i}.SC';path.write_bytes(data);want=model(data);(a.out/f'{i}.expected').write_bytes(want)
  run=subprocess.run([str(exe),str(path)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),check=True,timeout=30);assert run.stdout==want,(i,name)
  records.append(dict(case=i,name=name,queries=len(want)//8,bytes=len(want),sha256=hashlib.sha256(want).hexdigest()));all_bytes+=want
 report=dict(worlds=len(records),queries=sum(r['queries']for r in records),outputSHA256=hashlib.sha256(all_bytes).hexdigest(),cases=records,native='UNRUN',originalExecutable='UNRUN');(a.out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'}))
if __name__=='__main__':main()
