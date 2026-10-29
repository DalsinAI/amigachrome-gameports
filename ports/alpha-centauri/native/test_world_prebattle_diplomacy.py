# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
from test_original_world import valid_fixture
from test_original_game import decode
SOURCES=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prebattle_notification.cpp','prebattle_aggression.cpp','world_prebattle_diplomacy.cpp','world_prebattle_diplomacy_probe.cpp']
def pack(ns):return struct.pack('<'+'I'*len(ns),*(x&0xffffffff for x in ns))
def model(data):
 info,off,sp=decode(data);count=int(info['vehicles']);fs=0x20cc if int(info['version'])>=11 else 0x20bc;indices=list(range(min(count,9)))+[count,0xffffffff];rows=[];wanted=[];routes=[0]*4;aggression={};accepted=0
 def word(f,o,j):return struct.unpack_from('<I',sp['factions'],fs*f+o+4*j)[0]
 def add(a,b,obs,current,human,flags,popup,turn,prior):
  nonlocal accepted
  rows.append(pack([a,b,obs,current,human,flags,popup,turn,prior]));error=1 if a==b else 2 if obs!=1 else 1 if a>=count or b>=count else 0
  if not error:
   af=sp['vehicles'][52*a+14];bf=sp['vehicles'][52*b+14]
   if not af or not bf:error=3
   elif current>7:error=1
   elif af==bf:error=3
  if error:
   wanted.append(pack([error]+[0xa5a5a5a5]*3+[error]+[0xa5a5a5a5]*5));return
  forward=word(af,0x18,bf);reverse=word(bf,0x18,af);spoke=word(bf,0x78,af);warning=prior;peace_without_surprise=True
  if reverse&6:peace_without_surprise=False
  elif not reverse&16 and (reverse&8 or forward&0x800):peace_without_surprise=False
  route=0
  if human>>bf&1 and not peace_without_surprise:
   route=1 if current==bf else 2 if popup else 3
   if route==3:spoke=turn
  elif (forward|reverse)&2 and current not in [af,bf]:
   warning=next((v for cond,v in [(word(current,0x18,bf)&1,2),(word(current,0x18,af)&1,3),(flags&128,1)]if cond),prior)
  routes[route]+=1;accepted+=1
  ae=4 if reverse&7 else 5 if current in [af,bf]else 0;aggression[str(ae)]=aggression.get(str(ae),0)+1
  aw=[ae]+[0xa5a5a5a5]*5
  if ae==0:
   reset=bool(forward&7);aw=[0,(forward&0xfaeef378&0xff7fffff)|0x80008118,(reverse&0xfaeef378&0xff7fffff)|0x80008118,0 if reset else word(af,0x1ac,bf),0 if reset else word(bf,0x1ac,af),int(reset)]
  wanted.append(pack([0,route,warning,spoke]+aw))
 for a in indices:
  for b in indices:
   bf=sp['vehicles'][52*b+14]if b<count else 2
   for current in range(8):
    for k in range(7):add(a,b,1,current,[0,255,1<<bf,0xffffff00,0xffffffff,1<<bf,255][k],[0,128,0xffffffff][k%3],[0,1,0x80000000][k%3],[0,99,0x80000000,0x7fffffff][k%4],[0,1,2,3,0xa5a5a5a5][k%5])
 for obs in [0,2,0xffffffff]:add(0,1,obs,3,255,128,1,99,7)
 for current in [8,0xffffffff]:add(0,1,1,current,255,128,1,99,7)
 guards=50-4*int(count==0)-4*int(int(info['bases'])==0)
 return b''.join(rows),b''.join(wanted)+pack([guards]),dict(queries=len(rows),notificationAccepted=accepted,routes=routes,aggression=aggression,guards=guards)
def controls():
 rng=random.Random(0x95d1a8)
 for version in range(7,13):
  for k in range(4):
   b=bytearray(valid_fixture(version,9,2));_,off,_=decode(b);fs=0x20cc if version>=11 else 0x20bc
   for f in range(8):
    for j in range(8):
     status=[0,2,8,16,0x800,0xffffffff,rng.getrandbits(32)&0xfffffff8][(f*8+j+k)%7]
     struct.pack_into('<I',b,off['factions']+fs*f+0x18+4*j,status)
     struct.pack_into('<I',b,off['factions']+fs*f+0x78+4*j,rng.getrandbits(32))
     struct.pack_into('<I',b,off['factions']+fs*f+0x1ac+4*j,rng.getrandbits(32))
   yield f'v{version}-{k}',bytes(b)
 yield 'empty',valid_fixture(12,0,0)
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--original',type=Path,action='append',default=[]);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/s)for s in SOURCES]+['-o',str(out/'probe')],check=True)
 digest=hashlib.sha256();totals=dict(queries=0,notificationAccepted=0,guards=0,routes=[0]*4,aggression={});worlds=0;original_hash={str(p):hashlib.sha256(p.read_bytes()).hexdigest()for p in a.original}
 for name,data in list(controls())+[(p.name,p.read_bytes())for p in a.original]:
  folder=out/name;folder.mkdir();file=folder/'world.sc';file.write_bytes(data);raw,want,counts=model(data);(folder/'inputs.bin').write_bytes(raw);(folder/'expected.bin').write_bytes(want);r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe'),str(file)],input=raw,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(folder/'actual.bin').write_bytes(r.stdout);(folder/'stderr').write_bytes(r.stderr);assert r.returncode==0,(name,r.returncode,r.stderr[:1500]);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));digest.update(r.stdout);worlds+=1
  for k in ['queries','notificationAccepted','guards']:totals[k]+=counts[k]
  totals['routes']=[x+y for x,y in zip(totals['routes'],counts['routes'])]
  for k,v in counts['aggression'].items():totals['aggression'][k]=totals['aggression'].get(k,0)+v
 for p,h in original_hash.items():assert hashlib.sha256(Path(p).read_bytes()).hexdigest()==h
 assert all(totals['routes'])and all(totals['aggression'].get(k)for k in ['0','4','5']);r=dict(worlds=worlds,counts=totals,originalFilesSHA256=original_hash,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
if __name__=='__main__':main()
