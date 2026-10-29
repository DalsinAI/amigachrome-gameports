#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Edition-correct double police and ordered immutable garrison snapshots."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from test_units import fixture as tables,material,reference,sections
from test_original_world import valid_fixture
from test_original_game import decode
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','world_police.cpp','world_police_probe.cpp']
def rules(profile,c):
 t=tables(profile);pr=['None','Disable','T0','T1','T2','T88']
 for i,row in enumerate(t['ABILITIES']):row[2]=pr[(i+c)%6]
 t['TECHNOLOGY'][1][6:8]=['Disable','None'];t['TECHNOLOGY'][2][6:8]=['None','Disable'];t['TECHNOLOGY'][3][6:8]=['T0','Disable']
 return material(t)
def fixture(v,c):
 nv=0 if c==0 else 2048 if c==31 and v==12 else 1+c%12;nb=0 if c==0 else 2
 b=bytearray(valid_fixture(v,nv,nb));info,off,_=decode(b);r=random.Random(v*100+c);pf=off['preferences'] if v>=11 else off['legacy'];shift=0 if v>=11 else 0x7c
 b[pf+0x1e0-shift:pf+0x1e0-shift+89]=bytes([r.randrange(256) for _ in range(89)])
 for f in range(8):
  p=off['faction_definitions']+f*0x59c;count=[0,1,2,8][(c+f)%4]
  if c==25 and f==1:count=-1
  if c==26 and f==2:count=9
  struct.pack_into('<i',b,p+0x50c,count)
  for j in range(8):
   kind=15 if (j+c+f)%3 else 7;slot=22 if j%2==0 else 10
   if c in [24,27] and j==1:kind=15;slot=-1 if c==24 else 29
   struct.pack_into('<i',b,p+0x510+j*4,kind);struct.pack_into('<i',b,p+0x530+j*4,slot)
 for i in range(512):
  p=off['prototypes']+i*52;struct.pack_into('<I',b,p+32,(0x400000 if (i+c)%3==0 else 0)|0x100)
  b[p+36]=i%9;b[p+37]=i%(21 if v<11 else 26);b[p+38]=255;b[p+39]=0;b[p+42]=[0,1,2,3,4,5,255][(i+c)%7]
 ids=[0,15,63,64,127,128,255,320,448,511,65,449];order=list(range(nv));r.shuffle(order)
 for n,i in enumerate(order):
  p=off['vehicles']+i*52;struct.pack_into('<hh',b,p,0,2);struct.pack_into('<h',b,p+10,ids[i%len(ids)]);b[p+14]=i%8;struct.pack_into('<hh',b,p+48,order[n+1] if n+1<nv else -1,order[n-1] if n else -1)
 for tile in range(int(info['width'])//2*int(info['height'])):
  p=off['tiles']+44*tile+8;value=struct.unpack_from('<I',b,p)[0];struct.pack_into('<I',b,p,value&~2)
 tile=int(info['width']);p=off['tiles']+44*tile+8
 if nv:struct.pack_into('<I',b,p,struct.unpack_from('<I',b,p)[0]|2)
 if c==16 and nv>1:struct.pack_into('<h',b,off['vehicles']+52*order[1]+50,-1)
 if c==17 and nv>1:struct.pack_into('<h',b,off['vehicles']+52*order[-1]+48,order[0]);struct.pack_into('<h',b,off['vehicles']+52*order[0]+50,order[-1])
 if c==18 and nv>2:struct.pack_into('<h',b,off['vehicles']+52*order[0]+48,order[2])
 if c==19 and nv:struct.pack_into('<I',b,p,struct.unpack_from('<I',b,p)[0]&~2)
 if c==20 and nv>1:struct.pack_into('<hh',b,off['vehicles']+52*order[-1],2,2)
 if c==21:struct.pack_into('<I',b,off['tiles']+(int(info['width'])+1)*44+8,2)
 if c==22 and nv:b[off['prototypes']+ids[0]*52+36]=9
 return bytes(b)
def model(text,data,profile):
 table=reference(text,profile);t=sections(text);keys={row[1].lower():i for i,row in enumerate(t['TECHNOLOGY'])};keys.update(none=-1,disable=-2);prereq=[(keys[r[6].lower()],keys[r[7].lower()]) for r in t['TECHNOLOGY']];info,_,sp=decode(data);v=int(info['version']);pref=sp['preferences'] if v>=11 else sp['legacy'];shift=0 if v>=11 else 0x7c;owners=pref[0x1e0-shift:][:89]
 prototypes=[sp['prototypes'][i*52:(i+1)*52] for i in range(512)];defs=[sp['faction_definitions'][i*0x59c:(i+1)*0x59c] for i in range(8)]
 def has(id,f):
  if f==0:return False
  if id==-1:return True
  if not 0<=id<88:return False
  a,b=prereq[id];return a!=-2 and (b!=-2 or a==-1) and bool(owners[id]&(1<<f))
 def ability(id):
  p=prototypes[id]
  if struct.unpack_from('<I',p,32)[0]&0x400000:return 0,1
  if profile=='base' or id<64:return 0,0
  d=defs[id//64];count=struct.unpack_from('<i',d,0x50c)[0]
  if not 0<=count<=8:return 1,1
  for i in range(count):
   if struct.unpack_from('<i',d,0x510+i*4)[0]!=15:continue
   slot=struct.unpack_from('<i',d,0x530+i*4)[0]
   if not 0<=slot<int(table['ability_count']):return 1,1
   if has(int(table[f'abilities.{slot}.prerequisite']),id//64) and slot==22:return 0,1
  return 0,0
 result=bytearray();abilities=[]
 for i in range(512):
  a=ability(i);abilities.append(a);result+=struct.pack('<2I',*a)
 vehicles=[]
 for i in range(int(info['vehicles'])):
  p=sp['vehicles'][i*52:(i+1)*52];x,y=struct.unpack_from('<hh',p);id=struct.unpack_from('<h',p,10)[0];n,prev=struct.unpack_from('<hh',p,48);vehicles.append((x,y,id,n,prev))
 good=bad=0;bs=308 if v>=11 else 304
 for base in range(min(8,int(info['bases']))):
  x,y=struct.unpack_from('<hh',sp['bases'],base*bs);members=[i for i,u in enumerate(vehicles) if u[:2]==(x,y)];flag=bool(struct.unpack_from('<I',sp['tiles'],((y*int(info['width'])+x)//2)*44+8)[0]&2);roots=[i for i in members if vehicles[i][4]<0];ordered=[];error=flag!=bool(members) or (members and len(roots)!=1);last=-1;at=roots[0] if len(roots)==1 else -1
  while at>=0 and not error:
   if at not in members or at in ordered or vehicles[at][4]!=last:error=True;break
   ordered.append(at);last,at=at,vehicles[at][3]
  if len(ordered)!=len(members):error=True
  facts=[]
  if not error:
   for i in ordered:
    id=vehicles[i][2];p=prototypes[id];ch=p[36];weapon=p[37];plan=p[42]
    if ch>=9 or weapon>=int(table['weapon_count']):error=True;break
    triad=int(table[f'chassis.{ch}.triad']);offense=int(table[f'weapons.{weapon}.offense'])
    if not 0<=triad<=2 or not -128<=offense<=127:error=True;break
    e,doubled=ability(id) if plan<4 else (0,0)
    if e:error=True;break
    facts.extend([i,id,plan,offense,triad,doubled])
  if error:result+=struct.pack('<I',1);bad+=1
  else:result+=struct.pack('<4I',0,profile=='expansion',base,len(ordered))+struct.pack('<'+str(len(facts))+'i',*facts);good+=1
 guards=5+(24 if int(info['bases']) and int(info['vehicles']) else 22 if int(info['bases']) else -0)
 # Two table/profile guards only execute when a base exists.
 if int(info['bases']):guards+=2
 result+=struct.pack('<I',guards);return bytes(result),abilities,good,bad,guards

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--rules-data',nargs=2,action='append',default=[]);ap.add_argument('--original',nargs=3,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
 exe=out/'police-probe';cmd=[a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True)
 corpus=[(rules('base' if v<11 else 'expansion',c),fixture(v,c),'base' if v<11 else 'expansion','synthetic') for v in range(7,13) for c in range(32)]
 for profile,path in a.rules_data:corpus += [(Path(path).read_bytes(),fixture(7 if profile=='base' else 12,c),profile,'owner-rules') for c in range(32)]
 for profile,rulespath,save in a.original:corpus.append((Path(rulespath).read_bytes(),Path(save).read_bytes(),profile,'owner-world'))
 digest=hashlib.sha256();good=bad=guards=accepted=invalid=0;records=[]
 for i,(text,data,profile,kind) in enumerate(corpus):
  rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);wanted,abilities,g,b,gd=model(text,data,profile)
  p=subprocess.run([*runner,str(exe),profile,str(rp),str(sp)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30)
  assert p.returncode==0 and p.stdout==wanted,(i,p.returncode,p.stderr,len(p.stdout),len(wanted),next((j for j,(x,y) in enumerate(zip(p.stdout,wanted)) if x!=y),None));assert sp.read_bytes()==data and rp.read_bytes()==text
  digest.update(p.stdout);good+=g;bad+=b;guards+=gd;accepted+=sum(e==0 for e,_ in abilities);invalid+=sum(e!=0 for e,_ in abilities);records.append(dict(profile=profile,kind=kind,sha256=hashlib.sha256(p.stdout).hexdigest()))
  if i==191:public=digest.hexdigest()
 receipt=dict(worlds=len(corpus),abilityQueries=len(corpus)*512,acceptedAbilities=accepted,rejectedAbilities=invalid,acceptedGarrisons=good,rejectedGarrisons=bad,guards=guards,publicOutputSHA256=public,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),records=records,command=cmd)
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
