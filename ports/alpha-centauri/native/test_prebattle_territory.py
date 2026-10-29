# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--original',type=Path);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/'prebattle_territory.cpp'),str(src/'prebattle_territory_probe.cpp'),'-o',str(out/'probe')],check=True)
def pack(ns):return struct.pack('<'+'I'*len(ns),*(x&0xffffffff for x in ns))
def signed(n):return(n+2**31)%2**32-2**31
rows=[];wants=[];accepted=treaties=frictions=negotiations=originals=0;holds={}
def add(row,observed=None):
 global accepted,treaties,frictions,negotiations
 obs,ed,atk,defend,current,owner,pact,defstatus,ownerstatus,ownermil,defmil,friction,nega,negb,neg=row;error=2 if obs!=1 else 1 if ed>1 or atk>7 or defend>7 or current>7 or owner< -128 or owner>127 else 3 if not atk or not defend or atk==defend else 0
 if error:want=[error]+[0xa5a5a5a5]*6;holds[str(error)]=holds.get(str(error),0)+1
 else:
  change=(owner in range(1,8) and owner not in [atk,defend] and bool(pact&1) and bool(defstatus&8) and not defstatus&16 and ownermil<defmil);do_friction=bool(change and ownerstatus&0x800000);newfr=max(0,min(20,signed(friction+3)))if do_friction else friction;newneg=(neg+3)&0xffffffff if do_friction and nega==defend and negb==owner else neg
  want=[0,ownerstatus|0x2000 if change else ownerstatus,defstatus|0x2000 if change else defstatus,newfr,newneg,int(change),int(do_friction)];accepted+=1;treaties+=bool(change);frictions+=do_friction;negotiations+=newneg!=neg
  if observed is not None:assert pack(want[1:5])==pack(observed)
 rows.append(pack(row));wants.append(pack(want))
rng=random.Random(0x51b8b0)
for i in range(8192):
 atk=1+i%7;defend=1+i//7%7
 if atk==defend:defend=defend%7+1
 owner=[-128,-1,0,1,2,3,4,5,6,7,8,127][i%12];pact=[0,1,0xffffffff][i//12%3];defstatus=[0,8,16,24,0xffffffff][i//36%5];ownerstatus=[0,0x800000,0xffffffff][i//180%3];ownermil=[-2147483648,-1,0,1,2147483647][i//540%5];defmil=[-2147483648,-1,0,1,2147483647][i//2700%5];fr=[-2147483648,-1,0,17,18,19,20,2147483647][i%8];nega=defend if i%3 else rng.getrandbits(32);negb=owner if owner>=0 and i%3 else rng.getrandbits(32)
 add([1,i%2,atk,defend,i%8,owner,pact,defstatus,ownerstatus,ownermil,defmil,fr,nega,negb,rng.getrandbits(32)])
#Guaranteed branch and wrap coverage, including signed rank equality boundaries.
for ed in range(2):
 for current in range(8):
  for ownermil,defmil in [(-2147483648,2147483647),(-1,0),(0,0),(1,-1),(2147483647,-2147483648)]:
   for fr in [-2147483648,-1,0,17,18,19,20,2147483647]:add([1,ed,1,2,current,3,1,8,0x800000,ownermil,defmil,fr,2,3,0xffffffff])
#An existing0x2000 treaty marker does not suppress this caller's friction.
for ed in range(2):
 for fr in [-2147483648,-1,0,17,18,19,20,2147483647]:add([1,ed,1,2,3,4,1,0x2008,0x802000,-1,0,fr,2,4,0xffffffff])
for index,values in {0:[0,2,0xffffffff],1:[2,0xffffffff],2:[0,2,8,0xffffffff],3:[0,1,8,0xffffffff],4:[8,0xffffffff],5:[-2147483648,-129,128,2147483647]}.items():
 for v in values:row=[1,0,1,2,3,4,1,8,0x800000,-1,0,17,2,4,7];row[index]=v;add(row)
if a.original:
 for ed,profile in enumerate(['base','expansion']):
  folder=a.original/(profile+'-original3');cases=json.loads((folder/'cases.json').read_text());raw=(folder/'actual.bin').read_bytes();assert len(raw)==16*len(cases)
  for i,c in enumerate(cases):
   at=c['attacker'];de=c['defender'];owner=c['owner'];selected=owner if owner in range(8)else 0;status=c['status'];mil=c['milStrength'];fr=c['friction'];add([1,ed,at,de,c['currentFaction'],owner,status[at*8+selected],status[de*8+selected],status[selected*8+de],mil[selected],mil[de],signed(fr[de*8+selected]),c['negotiationA'],c['negotiationB'],c['negotiationValue']],list(struct.unpack_from('<4I',raw,i*16)));originals+=1
raw=b''.join(rows);want=b''.join(wants);run=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe')],input=raw,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'inputs.bin').write_bytes(raw);(out/'expected.bin').write_bytes(want);(out/'actual.bin').write_bytes(run.stdout);(out/'stderr').write_bytes(run.stderr);assert run.returncode==0,(run.returncode,run.stderr[:1500]);assert run.stdout==want,(len(run.stdout),len(want),next((i for i,(x,y)in enumerate(zip(run.stdout,want))if x!=y),None));assert treaties and frictions and negotiations and len(holds)==3
r=dict(cases=len(rows),accepted=accepted,rejected=sum(holds.values()),treaties=treaties,frictions=frictions,negotiationUpdates=negotiations,holds=holds,originalJoins=originals,aliasGuards=11*len(rows),outputSHA256=hashlib.sha256(run.stdout).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
