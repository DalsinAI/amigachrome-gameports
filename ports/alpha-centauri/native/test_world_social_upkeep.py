#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
from test_original_world import valid_fixture
from test_original_game import decode
from test_social import reference
from test_social_upkeep import model
SRC=Path(__file__).resolve().parent

def rules(profile):
 tech='#TECHNOLOGY\n'+'\n'.join(f'Test {i},T{i},0,1,2,3,None,None,1' for i in range(89))+'\n'
 rows=[[f'E{i}' for i in range(11)],[f'Label{i}' for i in range(11)],[f'Category{i}' for i in range(4)]]
 for c in range(4):
  for m in range(4):
   effects=[((c*7+m*3+e*2)%7)-3 for e in range(11)]
   rows.append([f'Model{c}{m}','None',*[('+'*v if v>=0 else '-'*(-v))+f'E{e}' for e,v in enumerate(effects)]])
 keys=[f'BONUS{i}' for i in range(29 if profile=='base' else 41)]
 return (tech+'#SOCIO\n'+'\n'.join(','.join(row) for row in rows)+'\n#BONUSNAMES\n'+'\n'.join(','.join(keys[i:i+8]) for i in range(0,len(keys),8))+'\n#END\n').encode()

def fixture(version,mode):
 rng=random.Random(version*100+mode);b=bytearray(valid_fixture(version,0,0 if mode==24 else 8));info,o,_=decode(b);fs=0x20bc if version<11 else 0x20cc;pref=o['legacy'] if version<11 else o['preferences']
 for f in range(8):
  p=o['factions']+f*fs;d=o['faction_definitions']+f*0x59c
  struct.pack_into('<4i',b,p+0x228,*[rng.randrange(4) for _ in range(4)]);struct.pack_into('<11i',b,p+0x2d0,*[rng.randrange(-10,11) for _ in range(11)])
  struct.pack_into('<I',b,d+0x508,rng.getrandbits(32));struct.pack_into('<i',b,d+0x50c,mode%9)
  for j in range(8):
   k=rng.choice([0,1,4,5,6,8,10,18]);a=rng.randrange(11) if k in [4,10] else rng.randrange(4) if k in [5,6] else rng.randrange(-20,21);z=rng.randrange(4) if k in [5,6] else rng.randrange(-10,11)
   for off,val in [(0x510,k),(0x530,a),(0x550,z)]:struct.pack_into('<i',b,d+off+4*j,val)
 for index in [21,24,13,23]:struct.pack_into('<i',b,pref+0x84+4*index,-1 if mode==24 else rng.choice([-2,-1,0,1,2,3,4,5,6,7]))
 for i in range(int(info['width'])*int(info['height'])//2):
  p=o['tiles']+44*i;struct.pack_into('<I',b,p+12,rng.choice([0,0x2000,0x80002000,0x01002000,0x8000]));b[p+4]=rng.getrandbits(8);struct.pack_into('<b',b,p+7,rng.randrange(-1,8))
 if mode==23:
  p=o['faction_definitions']+0x59c;struct.pack_into('<i',b,p+0x50c,1);struct.pack_into('<i',b,p+0x510,3);struct.pack_into('<i',b,p+0x530,0);struct.pack_into('<i',b,p+0x550,2147483647);struct.pack_into('<i',b,o['factions']+fs+0x2d0,-3)
 if mode==25:struct.pack_into('<i',b,o['factions']+fs+0x228,4)
 if mode==26:struct.pack_into('<i',b,pref+0x84+21*4,int(info['bases']))
 if mode==27:struct.pack_into('<i',b,o['faction_definitions']+2*0x59c+0x50c,9)
 if mode==28:struct.pack_into('<i',b,o['factions']+fs+0x2d0,2147483647)
 if mode==29:struct.pack_into('<i',b,o['faction_definitions']+3*0x59c+0x50c,1);struct.pack_into('<i',b,o['faction_definitions']+3*0x59c+0x510,19)
 if mode==30:
  p=o['faction_definitions']+4*0x59c;struct.pack_into('<i',b,p+0x50c,1);struct.pack_into('<i',b,p+0x510,4);struct.pack_into('<i',b,p+0x530,11)
 if mode==31:
  for index in [21,24,13,23]:struct.pack_into('<i',b,pref+0x84+index*4,-2147483648)
 return bytes(b)

def expected(text,data):
 info,o,spans=decode(data);v=int(info['version']);profile='base' if v<11 else 'expansion';social=reference(text,profile);effects=[int(social[f'model.{c}.{m}.effect.{e}']) for c in range(4) for m in range(4) for e in range(11)];fs=0x20bc if v<11 else 0x20cc;pref=o['legacy'] if v<11 else o['preferences'];answers=[]
 for f in range(1,8):
  p=o['factions']+f*fs;d=o['faction_definitions']+f*0x59c;mask=0;invalid=False
  for j,index in enumerate([21,24,13,23]):
   at=struct.unpack_from('<i',data,pref+0x84+4*index)[0]
   if at<0:continue
   if at>=int(info['bases']):invalid=True;continue
   if struct.unpack_from('<b',data,o['bases']+at*(304 if v<11 else 308)+4)[0]==f:mask|=1<<j
  for i in range(int(info['width'])*int(info['height'])//2):
   at=o['tiles']+44*i;landmark=struct.unpack_from('<I',data,at+12)[0];owner=struct.unpack_from('<b',data,at+7)[0]
   if landmark&0x80002000==0x2000 and landmark&0xff000000==0 and owner==f and data[at+4]&(1<<f):mask|=16;break
  h=[int(v>=11),*struct.unpack_from('<4i',data,p+0x228),*struct.unpack_from('<11i',data,p+0x2d0),struct.unpack_from('<i',data,d+0x50c)[0],*struct.unpack_from('<24i',data,d+0x510),struct.unpack_from('<I',data,d+0x508)[0],mask,*effects]
  answers.append(struct.pack('<I',1)+bytes([90])*152 if invalid else model(h))
 return b''.join(answers)

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='c++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--original',nargs=2,action='append',default=[]);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);files=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','social_upkeep.cpp','world_social_upkeep.cpp','world_social_upkeep_probe.cpp'];cmd=[a.cc,*flags,*[str(SRC/n) for n in files],'-o',str(out/'probe')];subprocess.run(cmd,check=True)
 corpus=[(rules('base' if v<11 else 'expansion'),fixture(v,m),'synthetic') for v in range(7,13) for m in range(32)]+[(Path(t).read_bytes(),Path(s).read_bytes(),'original') for t,s in a.original];digest=hashlib.sha256();records=[];accepted=guards=0
 for i,(text,data,kind) in enumerate(corpus):
  info,_,_=decode(data);profile='base' if int(info['version'])<11 else 'expansion';tp=out/(str(i)+'.txt');sp=out/(str(i)+'.SC');tp.write_bytes(text);sp.write_bytes(data);want=expected(text,data)
  r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),profile,str(tp),str(sp)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(i,r.returncode,r.stderr);assert r.stdout[:-4]==want,(i,'bytes');g=struct.unpack_from('<I',r.stdout,len(want))[0];assert g>=14;guards+=g;digest.update(r.stdout);assert tp.read_bytes()==text and sp.read_bytes()==data;accepted+=sum(struct.unpack_from('<I',want,j*156)[0]==0 for j in range(7));records.append(dict(kind=kind,inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(r.stdout).hexdigest()))
 receipt=dict(worlds=len(corpus),queries=len(corpus)*7,accepted=accepted,rejected=len(corpus)*7-accepted,guards=guards,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd,records=records);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
