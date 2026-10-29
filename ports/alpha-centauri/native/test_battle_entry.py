#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent integer model plus optional private original-observation joins."""
from pathlib import Path
import argparse, hashlib, json, os, random, shlex, struct, subprocess
p=argparse.ArgumentParser();p.add_argument('--probe');p.add_argument('--runner',default='');p.add_argument('--output',required=True);p.add_argument('--observations');p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args()
out=Path(a.output);out.mkdir(parents=True,exist_ok=False)
if not a.probe:
 src=Path(__file__).resolve().parent;a.probe=str((out/'probe').resolve())
 flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all']
 subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/'battle_entry.cpp'),str(src/'battle_entry_probe.cpp'),'-o',a.probe],check=True)
 if a.m68k_linux:a.runner='qemu-m68k -cpu m68040'
def u(n):return n&0xffffffff
def s(n):return (n+2**31)%2**32-2**31
def d(n,b):return abs(n)//b*(-1 if n<0 else 1)
def words(ns):return b''.join(struct.pack('<I',u(n))for n in ns)
def rem(speed,spent):return max(0,min(999,s(speed-spent)))
requests=[];wanted=[];types=[];accepted=0;originals=0
def add(t,x,explicit=None):
 global accepted
 x=x+[0]*(16-len(x));requests.append(words([t]+x));types.append(t)
 if t==2:r=[1,1]
 elif t==0:
  obs,ed,mode,road,speed,spent=x[:6];e=2 if obs!=1 else 1 if ed>1 or spent>255 else 3 if road<=0 else 0
  use=int(bool(u(mode)&3)and mode<4);r=[e,12345,6789]if e else[0,rem(speed,spent)if use else road,use]
 else:
  obs,ed,flags,st,ds,base,fac,uid,spent,speed,mode,road,charge,off,count=x[:15]
  e=2 if obs!=1 else 1 if ed>1 or spent>255 or base>1 or fac>7 or uid>=512 or count<0 else 3 if flags!=2 or road<=0 else 0
  if e:r=[e]+[0x5a5a5a5a]*9
  else:
   ns=st&~0x40000 if base else st;np=spent;pen=0;scaled=0;lookup=0;append=0
   if not ns&0x1000:
    if fac and uid!=8 and (ed==0 or uid!=15):
     n=min(rem(speed,spent),road);off=d(s(n*off),road);pen=d(s((n-road)*100),road);scaled=lookup=1
     if pen and count<4:count+=1;append=1
    if mode!=1:np=(spent+charge)%256
   r=[0,(ns&~0x1000)|0x44,ds|4,np,scaled,off,pen,count,lookup,append]
 if explicit is not None:assert list(map(u,r))==list(map(u,explicit)),(t,x,r,explicit)
 wanted.append(words(r));accepted+=int(r[0]==0)
rng=random.Random(50819)
for i in range(8192):
 ed=i&1;speed=rng.getrandbits(32);spent=i%256;road=[1,3,7,13,100,1000,32767,2147483647][i//8%8];mode=[0,1,2,3,4,8,-1,-2147483648,2147483647][i%9]
 add(0,[1,ed,mode,road,speed,spent])
 add(1,[1,ed,2,rng.getrandbits(32),rng.getrandbits(32),(i>>4)&1,i%8,[8,15,19,64,511][i%5],spent,speed,mode,road,s(rng.getrandbits(32)),s(rng.getrandbits(32)),i%7])
# Every rejection keeps the sentinel output; include required observation and bounds.
for t,initial,changes in [(0,[1,0,2,3,9,1],{0:[0,2],1:[2,0xffffffff],3:[0,-1],5:[256,0xffffffff]}),(1,[1,0,2,0,0,0,1,64,0,9,0,3,3,320,0],{0:[0,2],1:[2,0xffffffff],2:[0,1,3,0xffffffff],5:[2,0xffffffff],6:[8,0xffffffff],7:[512,0xffffffff],8:[256,0xffffffff],11:[0,-1],14:[-1,-2147483648]})]:
 for index,vals in changes.items():
  for v in vals:x=initial.copy();x[index]=v;add(t,x)
add(2,[])
if a.observations:
 root=Path(a.observations)
 for ed,name in enumerate(['base','expansion']):
  for c in json.loads((root/(name+'-original3')/'cases.json').read_text()):
   speed=0 if c['unit']==19 and ed else min(c['chassisSpeed'],99)*c['road']
   scaled=int(not c['state']&0x1000 and c['faction']!=0 and c['unit']!=8 and (ed==0 or c['unit']!=15))
   add(1,[1,ed,2,c['state'],c['defenderState'],int(c['base']>=0),c['faction'],c['unit'],c['spent'],speed,c['mode'],c['road'],c['charge'],c['strength'],c['count']], [0,c['resultState'],c['defenderState']|4,c['resultSpent'],scaled,c['resultStrength'],c['penalty'],c['resultCount'],scaled,int(c['resultCount']!=c['count'])]);originals+=1
  for c in json.loads((root/(name+'-charge2')/'cases.json').read_text()):
   speed=0 if c['unit']==19 and ed else min(c['chassisSpeed'],99)*c['road']
   add(0,[1,ed,c['mode'],c['road'],speed,c['spent']],[0,c['resultCharge'],int(bool(u(c['mode'])&3)and c['mode']<4)]);originals+=1
raw=b''.join(requests);expected=b''.join(wanted);(out/'inputs.bin').write_bytes(raw);(out/'expected.bin').write_bytes(expected)
env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
r=subprocess.run(shlex.split(a.runner)+[str(Path(a.probe).resolve())],input=raw,capture_output=True,env=env);(out/'actual.bin').write_bytes(r.stdout);(out/'stderr.txt').write_bytes(r.stderr)
assert r.returncode==0,(r.returncode,r.stderr.decode(errors='replace'))
assert r.stdout==expected, next((i for i,(x,y)in enumerate(zip(r.stdout,expected))if x!=y),('length',len(r.stdout),len(expected)))
receipt=dict(cases=len(types),charge=types.count(0),hasty=types.count(1),alias=types.count(2),accepted=accepted,privateOriginalJoins=originals,outputSHA256=hashlib.sha256(r.stdout).hexdigest(),probeSHA256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest())
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
