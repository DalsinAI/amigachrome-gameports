#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
from test_original_world import valid_fixture
from test_original_game import decode
from test_world_unit_stats import rules_fixture
from test_units import sections,material,reference
from test_repair_vehicle import model as leaf
SRC=Path(__file__).resolve().parent
NAMES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','repair_vehicle.cpp','world_repair_vehicle.cpp','world_repair_vehicle_probe.cpp']

def rules(variant,mode):
 t=sections(rules_fixture(variant));t['ABILITIES'][21][2]=['T0','None','Disable','T1','T2','T3'][mode%6];return material(t)

def fixture(version,mode):
 b=bytearray(valid_fixture(version,3,3));info,o,sp=decode(b);old=version<11;pref=o['legacy'] if old else o['preferences'];shift=0x7c if old else 0;bs=304 if old else 308;rng=random.Random(version*1000+mode);phase=mode%8
 struct.pack_into('<I',b,pref+0x44,rng.getrandbits(32));b[pref+0x58]=rng.getrandbits(8)
 for j in range(33 if old else 64):struct.pack_into('<i',b,pref+0x84+j*4,rng.choice([-2147483648,-2,-1,0,1,2]))
 for j in range(89):b[pref+0x1e0-shift+j]=rng.getrandbits(8)
 for j in range(3):
  p=o['bases']+j*bs;struct.pack_into('<hhb',b,p,2*j,0,rng.randrange(8));struct.pack_into('<I',b,p+0x30,rng.choice([0,2]));b[p+0x8c:p+0x8c+(8 if old else 12)]=rng.randbytes(8 if old else 12)
 for j in range(8):
  d=o['faction_definitions']+j*0x59c;struct.pack_into('<i',b,d+0x50c,1);struct.pack_into('<i',b,d+0x510,15);struct.pack_into('<i',b,d+0x530,21)
 for j in range(int(info['width'])*int(info['height'])//2):
  p=o['tiles']+j*44;b[p]=96;struct.pack_into('<I',b,p+8,0);b[p+7]=255
 p=o['tiles'];b[p]=[0,32,64,96,128,224][mode%6];b[p+2]=mode%16;b[p+7]=rng.randrange(8);struct.pack_into('<I',b,p+8,[0,32,0x800,0x40000,0x40820,0x40821][mode%6])
 for j,(uid,x,y,faction) in enumerate([([0,15,16,17,18,19,63,64,320,511][mode%10],0,0,phase),(510,0,0,(phase+1)%8),(64,4,2,0)]):
  p=o['vehicles']+j*52;struct.pack_into('<hh',b,p,x,y);struct.pack_into('<I',b,p+4,rng.getrandbits(32)&~4 if mode%3 else 12);struct.pack_into('<H',b,p+8,rng.getrandbits(16));struct.pack_into('<h',b,p+10,uid);b[p+14]=faction;b[p+16]=[0,1,2,3,9,19,99,255][mode%8];b[p+17]=mode%5;struct.pack_into('<h',b,p+28,[0,1,2,-1,-32768,32767][mode%6]);b[p+39]=rng.getrandbits(8);struct.pack_into('<hh',b,p+48,1 if j==0 else -1,0 if j==1 else -1)
  p=o['prototypes']+uid*52;b[p+36]=(mode//3)%3;b[p+37]=12 if mode&1 else 0;b[p+39]=[0,1,2,4,255][mode%5];b[p+40]=0 if mode&2 else 1;struct.pack_into('<I',b,p+32,0x200000 if mode&4 else 0)
 if mode==32:struct.pack_into('<h',b,o['vehicles']+52+48,0);struct.pack_into('<h',b,o['vehicles']+50,1)
 if mode==33:struct.pack_into('<h',b,o['vehicles']+52+50,-1)
 if mode==34:struct.pack_into('<hh',b,o['vehicles']+52+48,-1,-1);struct.pack_into('<h',b,o['vehicles']+48,-1)
 if mode==35:struct.pack_into('<hh',b,o['vehicles']+52,2,0)
 if mode==36:struct.pack_into('<I',b,o['tiles']+8,1);struct.pack_into('<hh',b,o['bases'],6,0)
 if mode==37:b[o['prototypes']+struct.unpack_from('<h',b,o['vehicles']+10)[0]*52+36]=255
 if mode==38:b[o['prototypes']+struct.unpack_from('<h',b,o['vehicles']+10)[0]*52+37]=255
 if mode in [39,43]:struct.pack_into('<i',b,o['faction_definitions']+7*0x59c+0x50c,9)
 if mode==40:struct.pack_into('<i',b,pref+0x84+22*4,99)
 if mode in [41,42,43,44,45,46,47]:
  # Force a land cargo heal path: grants, raw short-circuit, invalid grants,
  # unrelated grant and base-owner breed differences must actually be reached.
  v=o['vehicles'];struct.pack_into('<h',b,v+10,0);b[v+14]=1;b[v+16]=10;b[v+17]=1;struct.pack_into('<I',b,v+4,8);struct.pack_into('<h',b,v+28,0)
  p=o['prototypes'];b[p+36]=0;b[p+37]=0;b[p+39]=1;b[p+40]=1
  b[o['tiles']]=96;struct.pack_into('<I',b,o['tiles']+8,0)
  struct.pack_into('<I',b,o['prototypes']+510*52+32,0x200000 if mode==43 else 0)
  for j in range(33 if old else 64):struct.pack_into('<i',b,pref+0x84+j*4,-1)
  struct.pack_into('<i',b,o['faction_definitions']+7*0x59c+0x50c,9 if mode==42 else 1)
  if mode==45:struct.pack_into('<i',b,o['faction_definitions']+7*0x59c+0x530,99)
  if mode==46:struct.pack_into('<i',b,o['faction_definitions']+7*0x59c+0x530,20)
 if mode==47:
  struct.pack_into('<I',b,o['tiles']+8,1);struct.pack_into('<I',b,o['bases']+0x30,0);b[o['bases']+0x8c+3]|=8;struct.pack_into('<i',b,pref+0x84+4,99)
 return bytes(b)

def oracle(text,data,profile):
 info,o,sp=decode(data);old=int(info['version'])<11;tables=reference(text,profile);rawsections=sections(text);tech=rawsections['TECHNOLOGY'];keys={r[1].lower():i for i,r in enumerate(tech)};keys.update(none=-1,disable=-2);pr=[(keys[r[6].lower()],keys[r[7].lower()]) for r in tech];pref=sp['legacy'] if old else sp['preferences'];shift=0x7c if old else 0;owners=pref[0x1e0-shift:][:89];projects=list(struct.unpack_from('<'+('33i' if old else '64i'),pref,0x84));projects+=[-1]*(64-len(projects));bs=304 if old else 308;nv=int(info['vehicles']);nb=int(info['bases']);width=int(info['width']);height=int(info['height']);vehicles=[sp['vehicles'][i*52:(i+1)*52] for i in range(nv)];bases=[sp['bases'][i*bs:(i+1)*bs] for i in range(nb)]
 def project(index,f):
  b=projects[index]
  if b<0:return False
  if b>=nb:raise ValueError
  return bases[b][4]==f
 def fac(b,n):return bool(b[0x8c+n//8]&(1<<(n%8)))
 def bay(uid):
  p=sp['prototypes'][uid*52:(uid+1)*52]
  if struct.unpack_from('<I',p,32)[0]&0x200000:return True
  if old or uid<64:return False
  d=sp['faction_definitions'][uid//64*0x59c:(uid//64+1)*0x59c];count=struct.unpack_from('<i',d,0x50c)[0]
  if not 0<=count<=8:raise ValueError
  for j in range(count):
   kind=struct.unpack_from('<i',d,0x510+j*4)[0];bit=struct.unpack_from('<i',d,0x530+j*4)[0]
   if kind!=15:continue
   if not 0<=bit<int(tables['ability_count']):raise ValueError
   if bit!=21:continue
   t=int(tables['abilities.21.prerequisite']);f=uid//64
   if f and (t==-1 or 0<=t<88 and pr[t][0]!=-2 and not(pr[t][1]==-2 and pr[t][0]!=-1) and owners[t]&(1<<f)):return True
  return False
 def stack(index):
  original=vehicles[index];coords=original[:4];top=index;seen=set()
  while True:
   if top in seen:raise ValueError
   seen.add(top);prev=struct.unpack_from('<h',vehicles[top],50)[0]
   if prev==-1:break
   if not 0<=prev<nv or struct.unpack_from('<h',vehicles[prev],48)[0]!=top or vehicles[prev][:4]!=coords:raise ValueError
   top=prev
  seen=set();prev=-1;current=top;present=False
  while current!=-1:
   if current in seen or not 0<=current<nv:raise ValueError
   seen.add(current);v=vehicles[current]
   if v[:4]!=coords or struct.unpack_from('<h',v,50)[0]!=prev:raise ValueError
   if current!=index:present|=bay(struct.unpack_from('<h',v,10)[0])
   prev=current;current=struct.unpack_from('<h',v,48)[0]
  if index not in seen or seen!={i for i,v in enumerate(vehicles) if v[:4]==coords}:raise ValueError
  return present
 def answer(q):
  phase,local,index=q;bad=struct.pack('<I',1)+bytes([90])*72
  if not 0<=phase<8 or not 0<=local<8 or not 0<=index<nv:return bad
  v=vehicles[index];turn=struct.unpack_from('<I',pref,0x44)[0];h=[int(not old),phase,local,turn,index,0,0,0,0,0,0,0]
  if v[14]!=phase:return leaf(struct.pack('<12I',*h)+v)
  uid=struct.unpack_from('<h',v,10)[0];p=sp['prototypes'][uid*52:(uid+1)*52];chassis,weapon,reactor=p[36],p[37],p[39]
  if chassis>=9 or weapon>=int(tables['weapon_count']):return bad
  triad=int(tables[f'chassis.{chassis}.triad']);offense=int(tables[f'weapons.{weapon}.offense']);x,y=struct.unpack_from('<hh',v)
  if not(0<=x<width and 0<=y<height and (x+y)%2==0):return bad
  tile=sp['tiles'][(y*width+x)//2*44:][:44];items=struct.unpack_from('<I',tile,8)[0];mask=0
  try:
   tower=not old and uid==19;ogre=not old and uid in [16,17,18]
   if not ogre and (tower or not struct.unpack_from('<I',v,4)[0]&4) and v[16]:
    if phase>0 and struct.unpack_from('<b',tile,7)[0]==phase:mask|=1
    b=next((b for b in bases if b[:4]==v[:4]),None) if items&1 else None
    if items&1 and b is None:raise ValueError
    if b is not None:
     mask|=2;riots=struct.unpack_from('<I',b,48)[0]&2
     if riots:mask|=4
     else:
      if offense<0 and uid<64:
       project_bits=[project(n,b[4]) for n in [8,16,31,17]]
       if any(fac(b,n) for n in [31,32,9,30]) or any(project_bits):mask|=8
      else:
       if fac(b,27+triad):mask|=16
       if not mask&16 and (triad!=2 or not old) and project([1,10,35][triad],phase):mask|=32
    order=0 if v[17] in [1,2] and struct.unpack_from('<h',v,28)[0]==1 else v[17]
    if p[40]:mask|=64
    if triad==0 and p[40] and (order==1 or tile[0]&224<96) and stack(index):mask|=128
    if project(22,phase):mask|=256
    if phase and not mask&128 and b is None and not mask&256 and reactor and items&32 and tile[0]&224>63 and project(8,phase):mask|=512
    if pref[0x58]&(1<<phase):mask|=1024
   h[5:]=[triad,reactor,offense&0xffffffff,items,tile[0],tile[2],mask];return leaf(struct.pack('<12I',*h)+v)
  except ValueError:return bad
 return answer

def queries(data):
 info,_,_=decode(data);n=int(info['vehicles']);return [(f,l,i) for i in range(n) for f in range(8) for l in [0,1,7]]+[(f,l,i) for f,l,i in [(-1,1,0),(8,1,0),(1,-1,0),(1,8,0),(1,1,n),(1,1,0xffffffff)]]

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--owner-dir',type=Path);ap.add_argument('--scenario-dir',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(SRC/n) for n in NAMES],'-o',str(out/'probe')];subprocess.run(cmd,check=True)
 cases=[(f'v{v}-{mode}','base' if v<11 else 'expansion',rules('base' if v<11 else 'expansion',mode),fixture(v,mode)) for v in range(7,13) for mode in range(48)]+[(f'empty{v}','base' if v<11 else 'expansion',rules('base' if v<11 else 'expansion',0),valid_fixture(v,0,0)) for v in [7,12]]
 if a.owner_dir:
  assert a.scenario_dir
  cases += [('owner-'+name,'base',(a.owner_dir/'alpha.txt').read_bytes(),(a.scenario_dir/name).read_bytes()) for name in ['Explore.SC','Build.SC','conquer.SC']]
 digest=hashlib.sha256();records=[];accepted=rejected=guards=heals=draws=0
 for name,profile,text,data in cases:
  tp=out/(name+'.txt');sp=out/(name+'.SC');qp=out/(name+'.queries');tp.write_bytes(text);sp.write_bytes(data);qs=queries(data);qp.write_bytes(b''.join(struct.pack('<3I',*[x&0xffffffff for x in q]) for q in qs));answer=oracle(text,data,profile);want=b''.join(map(answer,qs));r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),profile,str(tp),str(sp),str(qp)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,timeout=60)
  assert r.returncode==0,(name,r.returncode,r.stderr);assert r.stdout[:-4]==want,(name,len(r.stdout),len(want),next((i for i,(x,y) in enumerate(zip(r.stdout,want)) if x!=y),None));g=struct.unpack_from('<I',r.stdout,len(want))[0];assert g>=19;guards+=g;digest.update(r.stdout);assert sp.read_bytes()==data and tp.read_bytes()==text
  errors=[struct.unpack_from('<I',want,j*76)[0] for j in range(len(qs))];accepted+=errors.count(0);rejected+=errors.count(1);heals+=sum(e==0 and struct.unpack_from('<I',want,j*76+60)[0] for j,e in enumerate(errors));draws+=sum(e==0 and struct.unpack_from('<I',want,j*76+64)[0] for j,e in enumerate(errors));records.append(dict(name=name,queries=len(qs),inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(r.stdout).hexdigest()))
 receipt=dict(worlds=len(cases),queries=accepted+rejected,accepted=accepted,rejected=rejected,healingAttempts=heals,drawPredicates=draws,guards=guards,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd,records=records);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
