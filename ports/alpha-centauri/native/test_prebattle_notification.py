# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--original',type=Path);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all']
subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/'prebattle_notification.cpp'),str(src/'prebattle_notification_probe.cpp'),'-o',str(out/'probe')],check=True)
def pack(ns):return struct.pack('<'+'I'*len(ns),*(x&0xffffffff for x in ns))
rows=[];wants=[];routes=[0]*4;originals=0;holds={}
def add(row,observed=None):
 obs,ed,atk,defend,current,human,flags,popup,turn,priorwarning,priorspoke=row[:11];matrix=row[11:];error=2 if obs!=1 else 1 if ed>1 or atk>7 or defend>7 or current>7 else 3 if not atk or not defend or atk==defend else 0
 if error:want=[error]+[0xa5a5a5a5]*3;holds[str(error)]=holds.get(str(error),0)+1
 else:
  forward=matrix[atk*8+defend];reverse=matrix[defend*8+atk];peace_without_surprise=True
  if reverse&6:peace_without_surprise=False
  elif not reverse&16 and (reverse&8 or forward&0x800):peace_without_surprise=False
  route=0;warning=priorwarning;spoke=priorspoke
  if human>>defend&1 and not peace_without_surprise:
   route=1 if current==defend else 2 if popup!=0 else 3
   if route==3:spoke=turn
  elif (forward|reverse)&2 and current!=atk and current!=defend:
   candidates=[(matrix[current*8+defend]&1,2),(matrix[current*8+atk]&1,3),(flags&128,1)]
   warning=next((value for condition,value in candidates if condition),priorwarning)
  want=[0,route,warning,spoke];routes[route]+=1
  if observed is not None:assert want[1:]==observed
 rows.append(pack(row));wants.append(pack(want))
rng=random.Random(0x51b63c)
for i in range(16384):
 atk=1+i%7;defend=1+i//7%7
 if atk==defend:defend=defend%7+1
 matrix=[rng.getrandbits(32)for _ in range(64)];matrix[atk*8+defend]=[0,2,0x800,0xffffffff][i%4];matrix[defend*8+atk]=[0,2,4,6,8,16,24,0xffffffff][i//4%8]
 add([1,i%2,atk,defend,i%8,[0,1<<defend,255,0xffffff00,0xffffffff][i//32%5],[0,128,0xffffffff][i//160%3],[0,1,0x80000000][i//480%3],rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32)]+matrix)
for index,values in {0:[0,2,0xffffffff],1:[2,0xffffffff],2:[0,2,8,0xffffffff],3:[0,1,8,0xffffffff],4:[8,0xffffffff]}.items():
 for v in values:
  row=[1,0,1,2,3,255,128,0,99,0xa5a5a5a5,34]+[0]*64;row[index]=v;add(row)
if a.original:
 for ed,profile in enumerate(['base','expansion']):
  folder=a.original/(profile+'-original2');cases=json.loads((folder/'cases.json').read_text());raw=(folder/'actual.bin').read_bytes();assert len(raw)==12*len(cases)
  for i,c in enumerate(cases):
   add([1,ed]+[c[k]for k in ['attacker','defender','currentFaction','humanStatus','gameFlags','popupFlag','turn','priorWarning','priorSpoke']]+c['status'],list(struct.unpack_from('<3I',raw,i*12)));originals+=1
raw=b''.join(rows);want=b''.join(wants);run=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe')],input=raw,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'inputs.bin').write_bytes(raw);(out/'expected.bin').write_bytes(want);(out/'actual.bin').write_bytes(run.stdout);(out/'stderr').write_bytes(run.stderr);assert run.returncode==0,(run.returncode,run.stderr[:1500]);assert run.stdout==want,(len(run.stdout),len(want),next((i for i,(x,y)in enumerate(zip(run.stdout,want))if x!=y),None));assert all(routes) and len(holds)==3
r=dict(cases=len(rows),accepted=sum(routes),rejected=sum(holds.values()),routes=routes,holds=holds,originalJoins=originals,aliasGuards=74*len(rows),outputSHA256=hashlib.sha256(run.stdout).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
