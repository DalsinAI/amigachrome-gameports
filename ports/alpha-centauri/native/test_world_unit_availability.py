#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_world_unit_stats import rules_fixture
from test_units import sections
from test_prototype import decode as unit_tables
from test_unit_availability import model as leaf
HERE=Path(__file__).resolve().parent
NAMES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','movement.cpp','prototype.cpp','unit_availability.cpp','world_unit_availability.cpp','world_unit_availability_probe.cpp']

def fixture(version,case):
 b=bytearray(valid_fixture(version,3,2));info,off,spans=decode(b);old=version<11;pref=off['legacy'] if old else off['preferences'];shift=0x7c if old else 0
 struct.pack_into('<I',b,pref+0xc,0x400000 if case&2 else 0)
 b[pref+0x1e0-shift:pref+0x1e0-shift+89]=bytes((i*17+case*31)&255 for i in range(89))
 struct.pack_into('<I',b,off['metadata']+28,case&1)
 for i in range(len(spans['tiles'])//44):b[off['tiles']+i*44]=[0xe0,0,0x40,0x60][(case+i)%4]
 for i in range(512):
  p=off['prototypes']+i*52;struct.pack_into('<I',b,p+32,[0,1,0x1000000,0x1f000000][(i+case)%4]);b[p+36]=(i+case)%9;b[p+37]=(i*7+case)%26;b[p+38]=(i+case)%14;b[p+42]=8 if (i+case)%7==0 else 0;b[p+44]=(i*3+case)&255;struct.pack_into('<H',b,p+48,1 if (i+case)%5 else 0);struct.pack_into('<h',b,p+50,[-2,-1,0,1,2,3,87,88][(i+case)%8])
 if case==3:b[off['prototypes']+30*52+36]=255
 return bytes(b)

def queries(data):
 info,_,_=decode(data);n=int(info['bases'])
 return [(exp,faction,base,unit) for exp in range(2) for faction in range(8) for base in [-1,*range(n)] for unit in range(512)]+[(exp,f,b,u) for exp,f,b,u in [(2,1,-1,0),(0,-1,-1,0),(0,8,-1,0),(0,1,-2,0),(0,1,n,0),(0,1,-1,-1),(0,1,-1,512)]]

def oracle(text,data,variant):
 info,_,spans=decode(data);old=int(info['version'])<11;bs=304 if old else 308;pref=spans['legacy'] if old else spans['preferences'];owners=pref[0x1e0-(0x7c if old else 0):][:89];rules=struct.unpack_from('<I',pref,0xc)[0];t=sections(text);tech=t['TECHNOLOGY'];keys={r[1].lower():i for i,r in enumerate(tech)};keys.update(none=-1,disable=-2);pr=[(keys[r[6].lower()],keys[r[7].lower()]) for r in tech];units=unit_tables(text,variant);width=int(info['width']);height=int(info['height']);flat=bool(struct.unpack_from('<I',spans['metadata'],28)[0]);n=int(info['bases'])
 def model(q):
  exp,faction,base,id=q
  if exp not in (0,1) or not 0<=faction<8 or not 0<=id<512 or not -1<=base<n:return 1,1
  p=spans['prototypes'][id*52:(id+1)*52];triad=0;coastal=False
  if base>=0:
   if p[36]>=9:return 1,1
   triad=units['chassis'][p[36]]['triad'];x,y=struct.unpack_from('<hh',spans['bases'],base*bs)
   for dx,dy in [(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]:
    tx=x+dx;ty=y+dy
    if not flat:tx%=width
    if 0<=tx<width and 0<=ty<height and spans['tiles'][(ty*width+tx)//2*44]&0xe0<0x60:coastal=True
  technology=struct.unpack_from('<h',p,50)[0];a,b=pr[technology] if 0<=technology<88 else (0,0);mask=owners[technology] if 0<=technology<88 else 0
  row=(int(not old),id,faction,struct.unpack_from('<H',p,48)[0],p[44],struct.unpack_from('<b',p,42)[0],p[38],p[37],struct.unpack_from('<I',p,32)[0],rules,exp,int(base>=0),triad,int(coastal),technology,a,b,mask)
  return leaf(row)
 return model

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--owner-dir',type=Path);ap.add_argument('--scenario-dir',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 exe=out/'world-unit-availability-probe';cmd=[a.cc,*flags,*[str(HERE/n) for n in NAMES],'-o',str(exe)];subprocess.run(cmd,check=True);cases=[]
 for version in range(7,13):
  variant='base' if version<11 else 'expansion'
  for case in range(4):cases.append((f'v{version}-{case}',variant,rules_fixture(variant),fixture(version,case)))
 for version in [7,12]:
  for edge,(x,y) in enumerate([(0,0),(7,5),(6,0),(0,4)]):
   for flat in range(2):
    variant='base' if version<11 else 'expansion';b=bytearray(fixture(version,edge));_,off,_=decode(b);struct.pack_into('<hh',b,off['bases'],x,y);struct.pack_into('<I',b,off['metadata']+28,flat);cases.append((f'edge-v{version}-{edge}-{flat}',variant,rules_fixture(variant),bytes(b)))
 for version in [7,12]:
  variant='base' if version<11 else 'expansion';cases.append((f'empty-v{version}',variant,rules_fixture(variant),valid_fixture(version,0,0)))
 if a.owner_dir or a.scenario_dir:
  assert a.owner_dir and a.scenario_dir
  for name in ['Explore.SC','Build.SC','conquer.SC']:cases.append(('original-'+name,'base',(a.owner_dir/'alpha.txt').read_bytes(),(a.scenario_dir/name).read_bytes()))
 digest=hashlib.sha256();records=[];accepted=rejected=available=0
 for name,variant,text,data in cases:
  fp=out/(name+'.SC');fp.write_bytes(data);rp=out/(name+'.txt');rp.write_bytes(text);qs=queries(data);qp=out/(name+'.queries');qp.write_bytes(b''.join(struct.pack('<4i',*q) for q in qs));model=oracle(text,data,variant);answers=[model(q) for q in qs];want=b''.join(struct.pack('<2i',*x) for x in answers);r=subprocess.run([*runner,str(exe),variant,str(rp),str(fp),str(qp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr,len(r.stdout));assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i in range(len(qs)) if r.stdout[i*8:(i+1)*8]!=want[i*8:(i+1)*8]),None));assert fp.read_bytes()==data;digest.update(want);accepted+=sum(e==0 for e,v in answers);rejected+=sum(e!=0 for e,v in answers);available+=sum(e==0 and v for e,v in answers);records.append(dict(name=name,queries=len(qs),inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest()))
 receipt=dict(worlds=len(cases),queries=accepted+rejected,accepted=accepted,rejected=rejected,available=available,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),records=records,command=cmd,scope='Read-only World unit admission; all decoded/opaque World and tables/RNG unchanged; aliases,unloaded,wrong edition,invalid args/unused chassis guards. No authority/orders/queue completion/turn/UI.');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ('records','command')}))
if __name__=='__main__':main()
