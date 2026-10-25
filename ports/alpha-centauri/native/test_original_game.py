#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original plaintext SC/SAV container, endian and legacy record controls.

Wire schema transcribed from pinned MIT source; no original game assets.
Independent Python cursor and scalar interpretation, not native struct images.
"""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_map import fixture as map_fixture,fnv,word,model as map_model
SRC=Path(__file__).resolve().parent
SCHEMA=json.loads((SRC/'original_game_wire.json').read_text())['records']
FORMATS={'int8_t':'b','uint8_t':'B','int16_t':'h','uint16_t':'H','int32_t':'i','uint32_t':'I'}

def record_hash(data,kind,legacy=False):
 h=2166136261
 for f in SCHEMA[kind]['fields']:
  off=f['offset'];count=f['count'];typ=f['type']
  if legacy and kind=='GamePrefs' and off>=0x184:off-=0x7c
  if legacy and kind=='BASE' and off>=0x98:off-=4
  if typ=='char':h=fnv(data[off:off+count],h);continue
  fmt=FORMATS[typ];width=struct.calcsize(fmt)
  for i in range(count):
   if legacy and kind=='GamePrefs' and f['name']=='SecretProjects' and i>=33:value=-1
   elif legacy and kind=='BASE' and f['name']=='facilities_built' and i>=8:value=0
   else:value=struct.unpack_from('<'+fmt,data,off+i*width)[0]
   h=word(h,value)
 return h

def fixture(version=7,scenario=True,vehicles=3,bases=2,replay=16,marker=None):
 # Distinctive data exercises every byte, signed widths and full-length strings.
 def pattern(n,salt):return bytearray((i*73+salt)&255 for i in range(n))
 expanded=(version>=11) if marker is None else marker>=11
 marker=version if marker is None else marker
 legacy=pattern(796,11);struct.pack_into('<i',legacy,0x304,marker)
 prefs=pattern(920 if expanded else 796,83)
 def pref(off,value):struct.pack_into('<i',prefs,off if expanded or off<0x184 else off-0x7c,value)
 for off,value in [(0x30,0x10 if scenario else 0),(0x38,vehicles),(0x3c,bases),(0x44,37),(0x48,2138),(0x380,version),(0x38c,0)]:pref(off,value)
 if not expanded:legacy=prefs
 result=bytearray(b'TERRAN\0\x1a'+struct.pack('<I',0x56))+legacy
 if expanded:result+=prefs
 result+=struct.pack('<i',5)+pattern(0x10660 if version>=11 else 0x105e0,17)+pattern(0x2ce0,29)+pattern(0x20,33)
 result+=map_fixture()[15:]+pattern(0x3e80,45)+struct.pack('<i',replay)+pattern(replay,57)+pattern(0xe00,69)
 result+=pattern(0x6800,71)+pattern(0x1000,83)+pattern(vehicles*52,97)+pattern(bases*(308 if version>=11 else 304),103)
 if scenario:result+=pattern(0x100,113)
 result+=pattern(0x100,127)+pattern(0x98,139)+pattern(0x27a0,151)
 if version>=12:result+=pattern(0x20,163)
 result+=pattern(20+216+8,173)+struct.pack('<I',0xfedcba98)
 return bytes(result)

def decode(data):
 assert data[:12]==b'TERRAN\0\x1a'+struct.pack('<I',0x56)
 pos=12;spans={};offsets={}
 def take(name,n):
  nonlocal pos
  assert n>=0 and pos+n<=len(data),(name,pos,n,len(data))
  offsets[name]=pos;spans[name]=data[pos:pos+n];pos+=n;return spans[name]
 legacy=take('legacy',796);marker=struct.unpack_from('<i',legacy,0x304)[0];expanded=marker>=11
 p=take('preferences',920) if expanded else legacy
 def pref(off):return struct.unpack_from('<i',p,off if expanded or off<0x184 else off-0x7c)[0]
 version=pref(0x380);vehicles=pref(0x38);bases=pref(0x3c)
 player=struct.unpack('<i',take('player',4))[0]
 take('factions',0x10660 if version>=11 else 0x105e0);take('faction_definitions',0x2ce0);take('rankings',0x20)
 m=take('metadata',0xaa4);w,h=struct.unpack_from('<ii',m);nt=w//2*h;ac=((h+4)//5)*((((w+4)//5)+1)//2)
 take('tiles',nt*44);take('abstract',ac);take('faction_turn_might',0x3e80)
 replay=struct.unpack('<i',take('replay_size',4))[0];take('replay',replay);take('continents',0xe00);take('prototypes',0x6800);take('bunglist',0x1000)
 take('vehicles',vehicles*52);take('bases',bases*(308 if version>=11 else 304))
 if pref(0x30)&0x10:take('scenario',0x100)
 else:spans['scenario']=b''
 take('scenario_name',0x100);take('objectives',0x98);take('monuments',0x27a0)
 if version>=12:take('tectonic',0x20)
 else:spans['tectonic']=b''
 ui=take('ui',244);tick=struct.unpack('<I',take('tick',4))[0]
 assert pos==len(data),(pos,len(data))
 def records(name,kind,count,size):
  digest=2166136261
  for i in range(count):digest=word(digest,record_hash(spans[name][i*size:(i+1)*size],kind,version<11))
  return f'{digest:08x}'
 sd=2166136261
 for name in ['factions','faction_definitions','rankings','faction_turn_might','replay','continents','bunglist','scenario','scenario_name','objectives','monuments','tectonic']:
  sd=word(word(sd,len(spans[name])),fnv(spans[name]))
 # UI hashing consumes decoded words in file order, matching explicit source game_io.
 ud=2166136261
 for n in struct.unpack('<61i',ui):ud=word(ud,n)
 result=dict(version=str(version),marker=str(marker),expanded=str(int(expanded)),player=str(player),turn=str(pref(0x44)),year=str(pref(0x48)),vehicles=str(vehicles),bases=str(bases),width=str(w),height=str(h),replay=str(replay),tick=f'{tick:08x}',prefs=f'{record_hash(p,"GamePrefs",not expanded):08x}',vehicle_digest=records('vehicles','VEH',vehicles,52),prototype_digest=records('prototypes','UNIT',512,52),base_digest=records('bases','BASE',bases,308 if version>=11 else 304),spans=f'{sd:08x}',map=map_model(b'TERRANMAP\0\x1a'+struct.pack('<I',5)+m+spans['tiles']+spans['abstract'])['tile_digest'],ui=f'{ud:08x}',rc='0')
 return result,offsets,spans

def cases():
 result=[]
 for version in range(7,13):
  for scenario in [False,True]:result.append((f'v{version}-scenario{int(scenario)}',fixture(version,scenario),True))
 result += [('empty-actors',fixture(12,vehicles=0,bases=0,replay=0),True),('max-replay',fixture(11,replay=8192),True),('max-actors',fixture(12,vehicles=2048,bases=512),True),('expanded-marker-old-data',fixture(7,marker=12),True)]
 data=fixture();_,offsets,_=decode(data)
 def mutate(name,off,value,valid=False):
  m=bytearray(data);struct.pack_into('<I',m,off,value&0xffffffff);result.append((name,bytes(m),valid))
 for n in range(12):result.append((f'header-truncated{n}',data[:n],False))
 for i in range(8):
  m=bytearray(data);m[i]^=0x80;result.append((f'header-corrupt{i}',bytes(m),False))
 for n in [0,0x55,0x57,0xffffffff]:mutate(f'outer-version{n}',8,n)
 for n in [0,6,13,0x7fffffff,0xffffffff]:mutate(f'data-version{n}',12+0x304,n)
 mutate('encrypted',12+0x30,0x10000010);mutate('network',12+0x310,0x20)
 for name,off,values in [('vehicles',12+0x38,[-1,2049,0x7fffffff]),('bases',12+0x3c,[-1,513,0x7fffffff]),('player',offsets['player'],[-1,8,0x7fffffff]),('replay',offsets['replay_size'],[-1,8193,0x7fffffff]),('width',offsets['metadata'],[0,3,4097]),('height',offsets['metadata']+4,[0,4097])]:
  for n in values:mutate(name+str(n),off,n)
 for name,n in offsets.items():
  if n>=12:result.append(('truncated-before-'+name,data[:n],False))
 result += [('last-byte-missing',data[:-1],False),('trailing-byte',data+b'\0',False),('trailing-many',data+bytes(999),False)]
 # Expanded marker/current preference versions are independently bounded.
 m=bytearray(fixture(12));struct.pack_into('<i',m,12+796+0x380,13);result.append(('current-version-too-new',bytes(m),False))
 return result

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'game-probe';cmd=[a.cc,*flags,*[str(SRC/n) for n in ['original_map.cpp','original_game.cpp','original_game_probe.cpp']],'-o',str(exe)];subprocess.run(cmd,check=True)
 controls=cases()+[('original-'+p.name,p.read_bytes(),True) for p in a.original];records=[]
 for i,(name,data,valid) in enumerate(controls):
  path=out/(str(i)+'.SC');path.write_bytes(data);r=subprocess.run([*runner,str(exe),str(path)],capture_output=True,text=True,timeout=30,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  assert r.returncode==(0 if valid else 20),(name,r.returncode,r.stdout,r.stderr)
  expected=None
  if valid:
   expected,_,_=decode(data);actual=dict(s.split('=') for s in r.stdout.split());assert actual==expected,(name,actual,expected)
  records.append(dict(name=name,valid=valid,passed=True,sha256=hashlib.sha256(data).hexdigest(),result=expected))
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,cases=records,binarySHA256=hashlib.file_digest(exe.open('rb'),'sha256').hexdigest()),indent=2)+'\n');print(f'{len(records)} original-game cases passed')
if __name__=='__main__':main()
