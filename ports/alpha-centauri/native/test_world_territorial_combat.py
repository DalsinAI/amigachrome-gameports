# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,subprocess,struct,hashlib,json,os
from test_original_world import valid_fixture
from test_original_game import decode
from test_world_promotion import SOURCES as PARENT
SOURCES=[s for s in PARENT if s!='world_promotion_probe.cpp']+['territorial_combat.cpp','world_territorial_combat.cpp','world_territorial_combat_probe.cpp']
U=0xffffffff
def signed(v):return((v+0x80000000)&U)-0x80000000
def corpus():
 for i in range(96):
  v=7+i%6;b=bytearray(valid_fixture(v));info,o,sp=decode(b);fs=len(sp['factions'])//8;climate=(i%8)*32;owner=i//8%10-1
  if i>=80:climate=96;owner=0
  for j in range(3):b[o['vehicles']+j*52+14]=j+1
  for j in range(len(sp['tiles'])//44):b[o['tiles']+j*44]=climate;b[o['tiles']+j*44+7]=owner&255
  for f in range(8):
   for j in range(8):
    struct.pack_into('<I',b,o['factions']+f*fs+0x18+j*4,23 if i>=80 else[0,16,7,23][(i+f*3+j)%4]);struct.pack_into('<i',b,o['factions']+f*fs+0x58+j*4,[-2147483648,-1,0,1,20,21,2147483647][(i+f+j)%7]);struct.pack_into('<i',b,o['factions']+f*fs+0x12c+j*4,0 if i>=80 else[0,1,-1,2147483647][(i+f+j)%4])
  yield f'world-{i}',bytes(b)
def expected(data):
 info,o,sp=decode(data);fs=len(sp['factions'])//8;out=[];counts={str(i):0 for i in range(5)};applied=0
 def get(offset,f,j,signed=False):return struct.unpack_from('<i'if signed else'<I',sp['factions'],f*fs+offset+j*4)[0]
 for mode in range(3):
  for a in range(3):
   for b in range(3):
    af=a+1;bf=b+1;idx=b;tile=sp['tiles'][idx*44:(idx+1)*44];owner=struct.unpack_from('<b',tile,7)[0];land=tile[0]>=96;code=1 if a==b else 3 if land and not 0<=owner<=7 else 0;counts[str(code)]+=1
    if code:out +=[code]+[0]*139;continue
    gifts=[[get(0x12c,f,j,True)for j in range(8)]for f in range(8)];friction=[[get(0x58,f,j,True)for j in range(8)]for f in range(8)];g=gifts[af][owner]if land else 0;r=friction[owner][af]if land else 0;nv=0 if mode==2 else 0x80000000;hit=land and owner not in[af,bf]and get(0x18,owner,bf)&16 and get(0x18,owner,af)&7;afterg=1 if hit and not g else g;afterr=max(0,min(20,signed(r-1)))if hit else r;aftern=(nv-1)&U if hit and mode==0 else nv
    if hit:gifts[af][owner]=afterg;friction[owner][af]=afterr;applied+=1
    out +=[0,idx,owner,af,bf,int(bool(hit)),g,afterg,r,afterr,nv,aftern]
    for f in range(8):out+=gifts[f]+friction[f]
 return struct.pack('<'+'I'*len(out),*[v&U for v in out]),counts,applied
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();D=a.out.resolve();D.mkdir(parents=True,exist_ok=False);S=Path(__file__).resolve().parent;flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);subprocess.run([a.cc,*flags,*[str(S/s)for s in SOURCES],'-o',str(D/'probe')],check=True);digest=hashlib.sha256();totals={};applied=0;worlds=0;accepted_worlds=0
 for name,data in corpus():
  d=D/name;d.mkdir();(d/'world.sc').write_bytes(data);want,cnt,n=expected(data);(d/'expected.bin').write_bytes(want);r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(D/'probe'),str(d/'world.sc')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(d/'actual.bin').write_bytes(r.stdout);(d/'stderr').write_bytes(r.stderr);assert r.returncode==0,(name,r.returncode,r.stderr[:1000]);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(a,b)in enumerate(zip(r.stdout,want))if a!=b),None));assert r.stderr==f"GUARDS {cnt['0']}\n".encode();digest.update(r.stdout);worlds+=1;applied+=n;accepted_worlds+=bool(cnt['0'])
  for k,v in cnt.items():totals[k]=totals.get(k,0)+v
 j={'worlds':worlds,'attempts':worlds*27,'statusCounts':totals,'applied':applied,'acceptedWorlds':accepted_worlds,'tinyBudgetPriorOutputGuards':totals['0'],'runtimeGuards':accepted_worlds*6,'poisonedRuntimeAliasHostOrNoWriteBE':accepted_worlds,'sourceLifetimeChecks':accepted_worlds,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest(),'native':'UNRUN'};(D/'receipt.json').write_text(json.dumps(j,indent=2)+'\n');print(json.dumps(j))
if __name__=='__main__':main()
