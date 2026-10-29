# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--original',type=Path);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/'prebattle_aggression.cpp'),str(src/'prebattle_aggression_probe.cpp'),'-o',str(out/'probe')],check=True)
def pack(ns):return struct.pack('<'+'I'*len(ns),*(x&0xffffffff for x in ns))
rows=[];wants=[];accepted=resets=originals=0;holds={}
def add(row,observed=None):
 global accepted,resets
 obs,ed,atk,defend,current,sf,sr,cf,cr=row;error=2 if obs!=1 else 1 if ed>1 or atk>7 or defend>7 or current>7 else 3 if not atk or not defend or atk==defend else 4 if sr&7 else 5 if current in [atk,defend]else 0
 if error:want=[error]+[0xa5a5a5a5]*5;holds[str(error)]=holds.get(str(error),0)+1
 else:
  reset=bool(sf&7);new=[(sf&0xfaeef378&0xff7fffff)|0x80008118,(sr&0xfaeef378&0xff7fffff)|0x80008118,0 if reset else cf,0 if reset else cr]
  if observed is not None:assert observed==new+[0]
  want=[0]+new+[int(reset)];accepted+=1;resets+=reset
 rows.append(pack(row));wants.append(pack(want))
rng=random.Random(0x520520)
for i in range(8192):
 atk=1+i%7;defend=1+i//7%7
 if atk==defend:defend=defend%7+1
 current=i%8
 while current in [atk,defend]:current=(current+1)%8
 sf=[0,1,2,4,7,0xffffffff,0x8010,rng.getrandbits(32)][i%8];sr=rng.getrandbits(32)&0xfffffff8;add([1,i%2,atk,defend,current,sf,sr,rng.getrandbits(32),rng.getrandbits(32)])
for index,values in {0:[0,2,0xffffffff],1:[2,0xffffffff],2:[0,2,8,0xffffffff],3:[0,1,8,0xffffffff],4:[1,2,8,0xffffffff],6:[1,2,4,7,0xffffffff]}.items():
 for v in values:r=[1,0,1,2,3,0x10,0x10,3,4];r[index]=v;add(r)
if a.original:
 for ed,profile in enumerate(['base','expansion']):
  folder=a.original/(profile+'-original1');cases=json.loads((folder/'cases.json').read_text());raw=(folder/'actual.bin').read_bytes();assert len(raw)==20*len(cases)
  for i,c in enumerate(cases):add([1,ed]+[c[k]for k in ['attacker','defender','currentFaction','forwardStatus','reverseStatus','forwardCount','reverseCount']],list(struct.unpack_from('<5I',raw,i*20)));originals+=1
raw=b''.join(rows);want=b''.join(wants);run=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe')],input=raw,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'inputs.bin').write_bytes(raw);(out/'expected.bin').write_bytes(want);(out/'actual.bin').write_bytes(run.stdout);(out/'stderr').write_bytes(run.stderr);assert run.returncode==0,(run.returncode,run.stderr[:1500]);assert run.stdout==want,(len(run.stdout),len(want),next((i for i,(x,y)in enumerate(zip(run.stdout,want))if x!=y),None));assert accepted and resets and holds.get('4')and holds.get('5')
r=dict(cases=len(rows),accepted=accepted,rejected=len(rows)-accepted,combatCounterResets=resets,holds=holds,originalJoins=originals,aliasGuards=2*len(rows),idempotenceChecks=accepted,outputSHA256=hashlib.sha256(run.stdout).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
