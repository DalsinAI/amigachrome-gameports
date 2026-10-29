# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,subprocess,struct,json,hashlib,os,random
from test_original_world import valid_fixture
from test_original_game import decode
from test_battle_aftermath import SOURCES as PARENT
SOURCES=[s for s in PARENT if s!='battle_aftermath_probe.cpp']+['world_promotion.cpp','world_promotion_probe.cpp']

def effective(rank,social,rul,uid,off):
 if uid<64 and off<0:return min(rank,6)
 m=max(-4,min(4,social));m+=int(m<=-2)-int(m>=2);m+=rul
 return max(0,min(6,rank+m))
def result(c,rank=None,current=None):
 uid,r,social,rul,plan,defense,missile,off,cur,human,fac,home=c
 if rank is not None:r=rank
 if current is not None:cur=current
 if not fac or home!=-1 or plan==11:return 2,None
 queried=not missile and(plan<8 or defense>1);eff=effective(r,social,rul,uid,off)if queried else 0
 promoted=queried and eff<6 and r<6
 if promoted and eff>2:return 3,None
 after=r+int(promoted)
 if promoted and cur==fac and human and effective(after,social,rul,uid,off)!=eff:return 4,None
 return 0,(r,after,eff,int(queried),int(promoted))
def corpus():
 rng=random.Random(5062)
 for i in range(576):
  uid=rng.choice([0,63,64,511]);rank=rng.choice([0,1,2,3,4,5,6,7,255]);social=rng.choice([-4,-2,0,2,4]);rul=rng.choice([-2,0,2]);plan=rng.choice([0,7,8,12,255]);defense=rng.choice([-128,1,2,127]);missile=rng.randrange(2);off=rng.choice([-1,1]);cur=rng.choice([-1,1,2]);human=rng.randrange(2);fac=1;home=-1
  if i%29==0:fac=0
  elif i%31==0:home=0
  elif i%37==0:plan=11
  # Guaranteed promoted/notification/random cases in every edition.
  if i<36:uid=64;rank=i%6;social=0;rul=0;plan=0;defense=2;missile=0;off=1;cur=1 if i%2 else -1;human=1;fac=1;home=-1
  c=[uid,rank,social,rul,plan,defense,missile,off,cur,human,fac,home];version=7+i%6;b=bytearray(valid_fixture(version));info,o,sp=decode(b);fs=0x20cc if version>=11 else 0x20bc
  v=o['vehicles'];struct.pack_into('<h',b,v+10,uid);b[v+14]=fac;b[v+36]=rank;struct.pack_into('<h',b,v+46,home)
  p=o['prototypes']+uid*52;b[p:p+52]=bytes(52);b[p+39]=1;b[p+42]=plan
  struct.pack_into('<i',b,o['factions']+fs+0x288,social);struct.pack_into('<i',b,o['faction_definitions']+0x59c+0x4d8,rul)
  # Human-mask byte offset found in the actual preferences schema.
  pref=o.get('preferences',o.get('legacy'));b[pref+0x58]=2 if human else 0
  yield c,bytes(b),o,sp

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--reuse',type=Path);a=p.parse_args();D=a.out.resolve();D.mkdir(parents=True,exist_ok=False);S=Path(__file__).resolve().parent
 if a.reuse:
  for n in ['inputs.bin','expected.bin','cases.json']:(D/n).symlink_to(a.reuse.resolve()/n)
 else:
  inputs=[];outputs=[];records=[]
  for c,b,o,sp in corpus():
   status,s=result(c);v=bytearray(b[o['vehicles']:o['vehicles']+len(sp['vehicles'])]);out=struct.pack('<I',status)
   if status==0:
    v[36]=s[1];again,ss=result(c,rank=s[1],current=-1);out+=struct.pack('<5I',*s)+v+struct.pack('<2I',again,ss[1]if ss else 0xffffffff)
   inputs.append(struct.pack('<5I',len(b),c[6],c[5]&0xffffffff,c[7]&0xffffffff,c[8]&0xffffffff)+b);outputs.append(out);records.append({'facts':c,'status':status})
  (D/'inputs.bin').write_bytes(struct.pack('<I',len(inputs))+b''.join(inputs));(D/'expected.bin').write_bytes(b''.join(outputs));(D/'cases.json').write_text(json.dumps(records)+'\n')
 flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);subprocess.run([a.cc,*flags,*[str(S/s)for s in SOURCES],'-o',str(D/'probe')],check=True)
 r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(D/'probe'),str(D/'inputs.bin')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(D/'actual.bin').write_bytes(r.stdout);(D/'stderr').write_bytes(r.stderr);want=(D/'expected.bin').read_bytes();assert not r.returncode,(r.returncode,len(r.stdout),r.stderr[:1200]);assert r.stdout==want,(len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None))
 records=json.loads((D/'cases.json').read_text());counts={str(s):sum(c['status']==s for c in records)for s in range(5)};assert all(counts[str(s)]for s in [0,2,3,4]),counts
 receipt={'worlds':len(records),'statuses':counts,'outputSHA256':hashlib.sha256(want).hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest(),'native':'UNRUN'};(D/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
