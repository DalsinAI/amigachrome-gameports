# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
from test_original_world import valid_fixture
from test_original_game import decode
SOURCES=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prebattle_territory.cpp','world_prebattle_territory.cpp','world_prebattle_territory_probe.cpp']
def pack(ns):return struct.pack('<'+'I'*len(ns),*(x&0xffffffff for x in ns))
def signed(n):return(n+2**31)%2**32-2**31
def model(data):
 info,off,sp=decode(data);count=int(info['vehicles']);width=int(info['width']);fs=0x20cc if int(info['version'])>=11 else 0x20bc;mil=0x1060 if int(info['version'])>=11 else 0x105c;indices=list(range(min(count,9)))+[count,0xffffffff];rows=[];wanted=[];accepted=treaties=frictions=negotiations=contrasts=plan_holds=0
 def word(f,o,j=0):return struct.unpack_from('<I',sp['factions'],fs*f+o+4*j)[0]
 def add(a,b,obs,current,nega,negb,neg):
  nonlocal accepted,treaties,frictions,negotiations,contrasts,plan_holds
  rows.append(pack([a,b,obs,current,nega,negb,neg]));error=1 if a==b else 2 if obs!=1 else 1 if current>7 or a>=count or b>=count else 0
  if not error:
   af=sp['vehicles'][52*a+14];bf=sp['vehicles'][52*b+14]
   if not af or not bf or af==bf:error=3
   else:
    au=struct.unpack_from('<h',sp['vehicles'],52*a+10)[0];bu=struct.unpack_from('<h',sp['vehicles'],52*b+10)[0]
    if struct.unpack_from('<b',sp['prototypes'],52*au+0x2a)[0]==11 or struct.unpack_from('<b',sp['prototypes'],52*bu+0x2a)[0]==12:error=3;plan_holds+=1
   if not error:
    x,y=struct.unpack_from('<hh',sp['vehicles'],52*a);idx=y*(width//2)+x//2;owner=struct.unpack_from('<b',sp['tiles'],44*idx+7)[0]
    if not 0<=owner<=7:error=3
  if error:wanted.append(pack([error]+[0xa5a5a5a5]*10));return
  ap=word(af,0x18,owner);ds=word(bf,0x18,owner);os=word(owner,0x18,bf);om=signed(word(owner,mil));dm=signed(word(bf,mil));oldfr=signed(word(bf,0x58,owner));change=owner in range(1,8)and owner not in [af,bf]and bool(ap&1)and bool(ds&8)and not ds&16 and om<dm;df=bool(change and os&0x800000);fr=max(0,min(20,signed(oldfr+3)))if df else oldfr;nn=(neg+3)&0xffffffff if df and nega==bf and negb==owner else neg
  accepted+=1;treaties+=bool(change);frictions+=df;negotiations+=nn!=neg
  dx,dy=struct.unpack_from('<hh',sp['vehicles'],52*b);di=dy*(width//2)+dx//2;contrasts+=di!=idx and struct.unpack_from('<b',sp['tiles'],44*di+7)[0]!=owner
  wanted.append(pack([0,idx,owner,af,bf,os|0x2000 if change else os,ds|0x2000 if change else ds,fr,nn,int(bool(change)),int(df)]))
 for a in indices:
  for b in indices:
   bf=sp['vehicles'][52*b+14]if b<count else 2
   owner=0
   if a<count:
    x,y=struct.unpack_from('<hh',sp['vehicles'],52*a);owner=struct.unpack_from('<b',sp['tiles'],44*(y*(width//2)+x//2)+7)[0]
   for current in [0,1,2,3,7]:
    for mode in range(3):add(a,b,1,current,bf if mode!=2 else 0xffffffff,owner if mode==0 and owner>=0 else 0xffffffff,[0xffffffff,0x80000000,99][mode])
 for obs in [0,2,0xffffffff]:add(0,1,obs,3,2,4,99)
 for current in [8,0xffffffff]:add(0,1,1,current,2,4,99)
 guards=27-2*int(count==0)-2*int(int(info['bases'])==0)
 return b''.join(rows),b''.join(wanted)+pack([guards]),dict(queries=len(rows),accepted=accepted,treaties=treaties,frictions=frictions,negotiationUpdates=negotiations,contrastingOriginAndDefenderOwners=contrasts,specialPlanHolds=plan_holds,guards=guards)
def controls():
 rng=random.Random(0x1060)
 for version in range(7,13):
  for k in range(8):
   b=bytearray(valid_fixture(version,9,2));_,off,sp=decode(b);fs=0x20cc if version>=11 else 0x20bc;mil=0x1060 if version>=11 else 0x105c
   for t in range(len(sp['tiles'])//44):b[off['tiles']+44*t+7]=[-128,-1,0,1,2,3,4,5,6,7,8,127][(t+3*k)%12]&255;b[off['tiles']+44*t]=(t%8)*32
   for f in range(8):
    struct.pack_into('<i',b,off['factions']+fs*f+mil,[-2147483648,-1,0,1,2147483647][(f+k)%5])
    for j in range(8):struct.pack_into('<I',b,off['factions']+fs*f+0x18+4*j,[0,1,8,24,0x2008,0x800000,0x802000,0xffffffff][(f*8+j+k)%8]);struct.pack_into('<I',b,off['factions']+fs*f+0x58+4*j,rng.getrandbits(32))
   b[off['prototypes']+52*321+0x2a]=11 if k==2 else 12 if k==4 else 0
   b[off['prototypes']+52*322+0x2a]=12 if k==3 else 11 if k==4 else 0
   origin=1
   if k%2:struct.pack_into('<hh',b,off['vehicles']+52,7,5);origin=23
   b[off['tiles']+44*origin+7]=[4,4,4,-1,0,1,2,8][k]&255;b[off['tiles']+44*2+7]=5
   for f,j,status in [(1,4,1),(2,4,0x2008 if k==1 else 8),(4,2,0x802000 if k==1 else 0x800000),(1,5,0)]:struct.pack_into('<I',b,off['factions']+fs*f+0x18+4*j,status)
   struct.pack_into('<i',b,off['factions']+fs*4+mil,-1 if k!=2 else 1);struct.pack_into('<i',b,off['factions']+fs*2+mil,1)
   struct.pack_into('<i',b,off['factions']+fs*2+0x58+4*4,[-2147483648,-1,0,17,18,19,20,2147483647][k])
   yield f'v{version}-{k}',bytes(b)
 yield 'empty',valid_fixture(12,0,0)
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--original',type=Path,action='append',default=[]);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/s)for s in SOURCES]+['-o',str(out/'probe')],check=True)
 digest=hashlib.sha256();totals={};worlds=0;original_hash={str(p):hashlib.sha256(p.read_bytes()).hexdigest()for p in a.original}
 for name,data in list(controls())+[(p.name,p.read_bytes())for p in a.original]:
  folder=out/name;folder.mkdir();file=folder/'world.sc';file.write_bytes(data);raw,want,counts=model(data);(folder/'inputs.bin').write_bytes(raw);(folder/'expected.bin').write_bytes(want);r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe'),str(file)],input=raw,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(folder/'actual.bin').write_bytes(r.stdout);(folder/'stderr').write_bytes(r.stderr);assert r.returncode==0,(name,r.returncode,r.stderr[:1500]);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));digest.update(r.stdout);worlds+=1
  for k,v in counts.items():totals[k]=totals.get(k,0)+v
 for p,h in original_hash.items():assert hashlib.sha256(Path(p).read_bytes()).hexdigest()==h
 assert all(totals[k]for k in ['accepted','treaties','frictions','negotiationUpdates','contrastingOriginAndDefenderOwners','specialPlanHolds']);r=dict(worlds=worlds,counts=totals,originalFilesSHA256=original_hash,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
if __name__=='__main__':main()
