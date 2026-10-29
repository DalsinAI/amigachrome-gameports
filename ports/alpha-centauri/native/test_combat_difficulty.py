# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--original',type=Path);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/'combat_difficulty.cpp'),str(src/'combat_difficulty_probe.cpp'),'-o',str(out/'probe')],check=True)
def s(n):return (n+2**31)%2**32-2**31
def div(n,d):return abs(n)//d*(-1 if n<0 else 1)
def pack(ns):return struct.pack('<'+'I'*len(ns),*(x&0xffffffff for x in ns))
rows=[];wants=[];accepted=handicap=advantage=original=0

def add(row,observed=None):
 global accepted,handicap,advantage
 obs,ed,atk,defend,mask,ad,dd,off=row;error=2 if obs!=1 else 1 if ed>1 or atk>7 or defend>7 else 3 if not atk or not defend else 0
 if error:want=[error,0xa5a5a5a5,0xa5a5a5a5]
 else:
  flags=0;value=off
  if dd<2 and mask&(1<<defend)and not mask&(1<<atk):value=div(s(s(dd+1)*value),4);flags|=1
  if ad<2 and mask&(1<<atk)and not mask&(1<<defend):value=div(s(s(4-ad)*value),2);flags|=2
  if observed is not None:assert value==observed
  accepted+=1;handicap+=bool(flags&1);advantage+=bool(flags&2);want=[0,value,flags]
 rows.append(pack(row));wants.append(pack(want))
rng=random.Random(0x51bb12)
for i in range(16384):
 ad=[-2147483648,-7,-1,0,1,2,3,5,2147483647,s(rng.getrandbits(32))][i%10];dd=[-2147483648,-7,-1,0,1,2,5,2147483647,s(rng.getrandbits(32))][i//10%9];atk=1+i%7;defend=1+i//7%7;mask=[0,255,1<<atk,1<<defend,(1<<atk)|(1<<defend),0xffffff00,0xffffffff,rng.getrandbits(32)][i//70%8];off=[-2147483648,-1,0,1,100,2147483647,s(rng.getrandbits(32))][i//560%7];add([1,i%2,atk,defend,mask,ad,dd,off])
for index,values in {0:[0,2,0xffffffff],1:[2,0xffffffff],2:[0,8,0xffffffff],3:[0,8,0xffffffff]}.items():
 for val in values:r=[1,0,1,2,2,0,0,100];r[index]=val;add(r)
if a.original:
 for ed,profile in enumerate(['base','expansion']):
  folder=a.original/(profile+'-original1');cases=json.loads((folder/'cases.json').read_text());raw=(folder/'actual.bin').read_bytes();assert len(raw)==len(cases)*512
  for i,c in enumerate(cases):
   value=struct.unpack_from('<i',raw,i*512+(88 if not ed else 236))[0];add([1,ed]+[c[k]for k in ['attacker','defender','humanMask','attackerDifficulty','defenderDifficulty','offense']],value);original+=1
raw=b''.join(rows);want=b''.join(wants);run=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe')],input=raw,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'inputs.bin').write_bytes(raw);(out/'expected.bin').write_bytes(want);(out/'actual.bin').write_bytes(run.stdout);(out/'stderr').write_bytes(run.stderr);assert run.returncode==0,(run.returncode,run.stderr[:1500]);assert run.stdout==want,(len(run.stdout),len(want),next((i for i,(x,y)in enumerate(zip(run.stdout,want))if x!=y),None));assert handicap and advantage and accepted
r=dict(cases=len(rows),accepted=accepted,rejected=len(rows)-accepted,humanDefenderAdjustments=handicap,humanAttackerAdjustments=advantage,originalJoins=original,aliasGuards=2*len(rows),outputSHA256=hashlib.sha256(run.stdout).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
