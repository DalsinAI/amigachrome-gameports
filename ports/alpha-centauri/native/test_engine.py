#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Engine core (engine/) controls: unit checks, SAV round trips, rejections.

Builds engine/tests/engine_test.cpp with every engine/*.cpp and
engine/stubs/*.cpp source, on the host with ASan/UBSan or for m68k-linux under
qemu (big-endian). Save fixtures come from test_original_game.fixture()
(synthetic patterned bytes, no original assets), adjusted where the Thinker
loader derives or validates a value, then:
- read and written back by the engine and compared byte for byte;
- the engine's decoded summary is compared with test_original_game.decode(),
  an independent Python reader of the same container;
- loaded through load_daemon_bytes (with load-time fixups) and saved twice,
  requiring a stable second save;
- corrupted, truncated and unsupported variants must be rejected.
Optional --original FILE adds real saves the tester owns (never committed).
"""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_game import fixture,decode,SCHEMA

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'

def field(kind,name):
 for f in SCHEMA[kind]['fields']:
  if f['name']==name:return f['offset']
 raise KeyError(name)

def quick_root(n):
 # OpenSMACX quick_root, which Thinker's map_init uses for MapAreaSqRoot.
 if n<=1:return n
 t=n>>1
 while True:
  r=t;t=(t+n//t)>>1
  if not t<r:return r

def sanitize(data):
 """Make a patterned fixture acceptable to the Thinker loader without
 changing its layout: values the loader validates or recomputes are set to
 what an original save holds."""
 _,offsets,_=decode(data);m=bytearray(data)
 marker=struct.unpack_from('<i',m,12+0x304)[0];expanded=marker>=11
 p=offsets['preferences'] if expanded else offsets['legacy']
 def poff(off):return p+(off if expanded or off<0x184 else off-0x7c)
 version=struct.unpack_from('<i',m,poff(0x380))[0]
 for i in range(64 if expanded else 33):struct.pack_into('<i',m,poff(field('GamePrefs','SecretProjects'))+4*i,-1)
 m[poff(field('GamePrefs','FactionStatus')):poff(field('GamePrefs','FactionStatus'))+2]=b'\0\0' # tamper check
 meta=offsets['metadata'];w,h=struct.unpack_from('<ii',m,meta);struct.pack_into('<i',m,meta+24,quick_root(w//2*h))
 ui=offsets['ui'];struct.pack_into('<i',m,ui+16,0) # field_23D20, written as 0
 vehicles=struct.unpack_from('<i',m,poff(0x38))[0]
 for v in range(vehicles):
  o=offsets['vehicles']+v*52;struct.pack_into('<h',m,o+field('VEH','unit_id'),v%512)
 units=offsets['prototypes']
 for i in range(512):
  u=units+i*52
  if version<11 and m[u+field('UNIT','weapon_id')]>=100:m[u+field('UNIT','weapon_id')]&=0x3f
 if version<=7:m[units+8*52+field('UNIT','icon_offset')]=3;m[units+7*52+field('UNIT','icon_offset')]=2
 if version<11:
  q=field('BASE','queue_items')
  q=q-4 if q>=0x98 else q
  bases=struct.unpack_from('<i',m,poff(0x3c))[0]
  for b in range(bases):
   for k in range(10):
    o=offsets['bases']+b*304+q+4*k
    if struct.unpack_from('<i',m,o)[0]<-1000000:struct.pack_into('<i',m,o,-40)
 return bytes(m)

def valid_cases():
 r=[]
 for version in range(7,13):
  for scenario in [False,True]:r.append((f'v{version}-scenario{int(scenario)}',fixture(version,scenario)))
 r+=[('empty-actors',fixture(12,vehicles=0,bases=0,replay=0)),('max-replay',fixture(11,replay=8192)),
     ('max-actors',fixture(12,vehicles=2048,bases=512)),('expanded-marker-old-data',fixture(7,marker=12)),
     ('legacy-actors',fixture(10,vehicles=40,bases=30)),('v12-no-replay',fixture(12,replay=0))]
 return [(n,sanitize(d)) for n,d in r]

def invalid_cases():
 data=sanitize(fixture(12));_,offsets,_=decode(data);P=offsets['preferences'];r=[]
 def mutate(name,off,value):
  m=bytearray(data);struct.pack_into('<I',m,off,value&0xffffffff);r.append((name,bytes(m)))
 for n in [0,5,11,40]:r.append((f'truncated{n}',data[:n]))
 for i in range(8):
  m=bytearray(data);m[i]^=0x80;r.append((f'header-corrupt{i}',bytes(m)))
 for n in [0,0x55]:mutate(f'outer-version{n}',8,n)
 for n in [0,6,13,0x7fffffff]:mutate(f'data-version{n}',P+0x380,n)
 mutate('encrypted',P+0x30,0x10000010);mutate('pbem',P+0x38c,0x20)
 for name,off,values in [('vehicles',P+0x38,[-1,2049]),('bases',P+0x3c,[-1,513]),('player',offsets['player'],[-1,8]),
   ('replay',offsets['replay_size'],[-1,8193]),('width',offsets['metadata'],[0,3,4097]),('height',offsets['metadata']+4,[0,4097])]:
  for n in values:mutate(name+str(n),off,n)
 m=bytearray(data);fs=P+field('GamePrefs','FactionStatus');m[fs:fs+2]=b'\x06\x06';r.append(('tamper-check',bytes(m)))
 mutate('project-base',P+field('GamePrefs','SecretProjects'),512)
 for name,n in offsets.items():
  if n>=12 and name!='tick':r.append(('truncated-before-'+name,data[:n])) # the seed footer is optional
 r+=[('last-byte-missing',data[:-5]),('trailing-byte',data+b'\0')]
 return r

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 subprocess.run(['python3',str(ENGINE/'tools'/'gen_save_fields.py'),'--check'],check=True)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 sources=sorted(ENGINE.glob('*.cpp'))+sorted((ENGINE/'stubs').glob('*.cpp'))+[ENGINE/'tests'/'engine_test.cpp']
 exe=out/'engine-test';cmd=[a.cc,*flags,'-I',str(ENGINE),*map(str,sources),'-o',str(exe)];subprocess.run(cmd,check=True)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 def run(*args):return subprocess.run([*runner,str(exe),*map(str,args)],capture_output=True,text=True,timeout=600,env=env)
 r=run('unit');assert r.returncode==0,(r.stdout,r.stderr);unit=r.stdout.strip();print(unit)
 records=[]
 for i,(name,data) in enumerate(valid_cases()+[('original-'+p.name,p.read_bytes()) for p in a.original]):
  src=out/f'{i}.SAV';dst=out/f'{i}.out';dmn=out/f'{i}.daemon';src.write_bytes(data)
  r=run('roundtrip',src,dst);assert r.returncode==0,(name,r.returncode,r.stdout,r.stderr)
  got=dict(s.split('=') for s in r.stdout.split())
  assert dst.read_bytes()==data,(name,'round trip differs')
  if not name.startswith('original-'):
   exp,_,_=decode(data)
   for k in ['version','marker','player','turn','year','vehicles','bases','width','height','replay']:assert got[k]==exp[k],(name,k,got[k],exp[k])
   assert got['tick']==exp['tick'],(name,got['tick'],exp['tick'])
  r=run('daemon',src,dmn);assert r.returncode==0,(name,'daemon',r.returncode,r.stdout,r.stderr)
  records.append(dict(name=name,valid=True,sha256=hashlib.sha256(data).hexdigest(),summary=got,daemon=r.stdout.strip()))
 for i,(name,data) in enumerate(invalid_cases()):
  src=out/f'bad{i}.SAV';src.write_bytes(data);r=run('roundtrip',src,out/f'bad{i}.out')
  assert r.returncode==20,(name,r.returncode,r.stdout,r.stderr)
  records.append(dict(name=name,valid=False,sha256=hashlib.sha256(data).hexdigest(),error=r.stdout.strip()))
 # A file without the optional seed footer loads and is written back with seed 0.
 data=valid_cases()[-1][1];src=out/'noseed.SAV';src.write_bytes(data[:-4]);r=run('roundtrip',src,out/'noseed.out')
 assert r.returncode==0 and (out/'noseed.out').read_bytes()==data[:-4]+bytes(4),(r.stdout,r.stderr)
 records.append(dict(name='no-seed-footer',valid=True,sha256=hashlib.sha256(data[:-4]).hexdigest(),summary=r.stdout.strip()))
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,unit=unit,cases=records,
   binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n')
 nv=sum(c['valid'] for c in records);print(f'{nv} round-trip cases and {len(records)-nv} rejection cases passed')
if __name__=='__main__':main()
