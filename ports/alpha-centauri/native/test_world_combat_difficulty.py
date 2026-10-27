# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess
from test_original_world import valid_fixture
from test_original_game import decode
SOURCES=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','combat_difficulty.cpp','world_combat_difficulty.cpp','world_combat_difficulty_probe.cpp']
def pack(ns):return struct.pack('<'+'I'*len(ns),*(x&0xffffffff for x in ns))
def signed(n):return (n+2**31)%2**32-2**31
def trunc(n,d):return abs(n)//d*(-1 if n<0 else 1)
def model(data):
 info,off,sp=decode(data);count=int(info['vehicles']);fs=0x20cc if int(info['version'])>=11 else 0x20bc;indices=list(range(min(count,9)))+[count,0xffffffff];rows=[];wanted=[];accepted=handicap=advantage=0
 def add(a,b,obs,mask,value):
  nonlocal accepted,handicap,advantage
  rows.append(pack([a,b,obs,mask,value]));error=1 if a==b else 2 if obs!=1 else 1 if a>=count or b>=count else 0
  if not error:
   af=sp['vehicles'][52*a+14];bf=sp['vehicles'][52*b+14]
   if not af or not bf:error=3
  if error:wanted.append(pack([error,0xa5a5a5a5,0xa5a5a5a5]));return
  ad=struct.unpack_from('<i',sp['factions'],fs*af+8)[0];dd=struct.unpack_from('<i',sp['factions'],fs*bf+8)[0];applied=0
  if dd<2 and mask&(1<<bf)and not mask&(1<<af):value=trunc(signed(signed(dd+1)*value),4);applied|=1
  if ad<2 and mask&(1<<af)and not mask&(1<<bf):value=trunc(signed(signed(4-ad)*value),2);applied|=2
  accepted+=1;handicap+=bool(applied&1);advantage+=bool(applied&2);wanted.append(pack([0,value,applied]))
 for a in indices:
  for b in indices:
   af=sp['vehicles'][52*a+14]if a<count else 1;bf=sp['vehicles'][52*b+14]if b<count else 2
   for mask in [0,255,1<<af,1<<bf,0xffffff00,0xffffffff,128]:
    for value in [-2147483648,-1,0,1,99,2147483647]:add(a,b,1,mask,value)
 for obs in [0,2,0xffffffff]:add(0,1,obs,2,100)
 #11 output aliases,10 source aliases (the actual runtime input is legal),
 #minus missing actor/base arrays, plus the poisoned alias and empty World.
 guards=2+2*(11-int(count==0)-int(int(info['bases'])==0))-1
 return b''.join(rows),b''.join(wanted)+pack([guards]),dict(queries=len(rows),accepted=accepted,humanDefenderAdjustments=handicap,humanAttackerAdjustments=advantage,guards=guards)
def controls():
 for version in range(7,13):
  for k in [0,1]:
   b=bytearray(valid_fixture(version,9,2));_,off,_=decode(b);fs=0x20cc if version>=11 else 0x20bc;values=[-2147483648,-7,-1,0,1,2,5,2147483647]
   for f in range(8):struct.pack_into('<i',b,off['factions']+fs*f+8,values[(f+3*k)%8])
   yield f'v{version}-{k}',bytes(b)
 yield 'empty',valid_fixture(12,0,0)
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--original',type=Path,action='append',default=[]);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/s)for s in SOURCES]+['-o',str(out/'probe')],check=True)
 digest=hashlib.sha256();totals={};worlds=0;original_hash={str(p):hashlib.sha256(p.read_bytes()).hexdigest()for p in a.original}
 for name,data in list(controls())+[(p.name,p.read_bytes())for p in a.original]:
  folder=out/name;folder.mkdir();file=folder/'world.sc';file.write_bytes(data);raw,want,counts=model(data);(folder/'inputs.bin').write_bytes(raw);(folder/'expected.bin').write_bytes(want);r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe'),str(file)],input=raw,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(folder/'actual.bin').write_bytes(r.stdout);(folder/'stderr').write_bytes(r.stderr);assert r.returncode==0,(name,r.returncode,r.stderr[:1500]);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));digest.update(r.stdout);worlds+=1
  for k,v in counts.items():totals[k]=totals.get(k,0)+v
 for p,h in original_hash.items():assert hashlib.sha256(Path(p).read_bytes()).hexdigest()==h
 assert totals['accepted']and totals['humanDefenderAdjustments']and totals['humanAttackerAdjustments'];r=dict(worlds=worlds,counts=totals,originalFilesSHA256=original_hash,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
if __name__=='__main__':main()
