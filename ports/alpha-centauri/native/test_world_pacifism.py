#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Pacifism from real home actors/location, independent explicit wire model."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_units import fixture as tables,material,reference
from test_original_world import valid_fixture
from test_original_game import decode
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','world_pacifism.cpp','world_pacifism_probe.cpp']
def rules(profile):return material(tables(profile))
def fixture(v,c):
 nv=0 if c==0 else 2048 if c==31 else 12;nb=0 if c==0 else 2;b=bytearray(valid_fixture(v,nv,nb));info,off,_=decode(b);bs=308 if v>=11 else 304
 for i in range(nb):b[off['bases']+i*bs+4]=0 if c%7==0 and i==0 else i+1
 for i in range(512):
  p=off['prototypes']+52*i;b[p+36]=i%9;b[p+37]=i%(21 if v<11 else 26);b[p+38]=255;b[p+39]=0;b[p+42]=[0,1,2,3,4,5,9,10,11,255][(c+i)%10];struct.pack_into('<I',b,p+32,0x10000 if c%2 else 0)
  if c==13:b[p+36]=2;b[p+42]=4
  if c==14:b[p+37]=1
  if c==15:b[p+36]=9
  if c==16:b[p+37]=255
 for tile in range(int(info['width'])//2*int(info['height'])):
  p=off['tiles']+44*tile;b[p+7]=[-128,-1,0,1,2,7,8,127][(c+tile)%8]&255;b[p+2]=(c+tile)%16;struct.pack_into('<I',b,p+8,1 if (c+tile)%3==0 else 0)
 for i in range(nv):
  p=off['vehicles']+52*i;home=i%2;owner=0 if c%7==0 and home==0 else home+1
  b[p+14]=owner if i%4 else (owner+1)%8;struct.pack_into('<h',b,p+46,home if i%5 else -1);struct.pack_into('<h',b,p+10,[0,15,63,64,127,128,320,448,511][i%9]);struct.pack_into('<hh',b,p,2*(i%4),2*((i//4)%3))
 return bytes(b)
def model(text,data,profile):
 table=reference(text,profile);info,_,sp=decode(data);v=int(info['version']);bases=int(info['bases']);result=bytearray();ok=bad=0;counts=[]
 for base in range(min(8,bases)):
  owner=sp['bases'][base*(308 if v>=11 else 304)+4];n=home=away=air=both=0;error=False
  for i in range(int(info['vehicles'])):
   actor=sp['vehicles'][i*52:(i+1)*52]
   if actor[14]!=owner or struct.unpack_from('<h',actor,46)[0]!=base:continue
   id=struct.unpack_from('<h',actor,10)[0];p=sp['prototypes'][id*52:(id+1)*52];plan=p[42]
   if plan>=10:continue
   weapon=p[37]
   if weapon>=int(table['weapon_count']):error=True;break
   offense=int(table[f'weapons.{weapon}.offense'])
   if not -128<=offense<=127:error=True;break
   if not offense:continue
   chassis=p[36]
   if chassis>=9:error=True;break
   triad=int(table[f'chassis.{chassis}.triad'])
   if not 0<=triad<=2:error=True;break
   x,y=struct.unpack_from('<hh',actor);p=((y*int(info['width'])+x)//2)*44;tile=sp['tiles'][p:p+44];territory=struct.unpack_from('<b',tile,7)[0];valid_base=bool(struct.unpack_from('<I',tile,8)[0]&1) and tile[2]&15<8
   if not valid_base and territory>7:error=True;break
   foreign=not valid_base and not (territory>0 and territory==owner);airborne=triad==2 and plan!=4
   home+=1;away+=foreign;air+=airborne;both+=foreign and airborne;n+=foreign or airborne
  if error:values=[1]+[0x5a5a5a5a]*7;bad+=1;counts.append(None)
  else:values=[0,int(v>=11),base,n,home,away,air,both];ok+=1;counts.append(n)
  result+=struct.pack('<8i',*values)
 guards=3+(12 if bases and int(info['vehicles']) else 11 if bases else 0);result+=struct.pack('<I',guards);return bytes(result),ok,bad,guards,counts

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--rules-data',nargs=2,action='append',default=[]);ap.add_argument('--original',nargs=3,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
 exe=out/'pacifism-probe';cmd=[a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True)
 corpus=[(rules('base' if v<11 else 'expansion'),fixture(v,c),'base' if v<11 else 'expansion','synthetic') for v in range(7,13) for c in range(32)]
 for profile,path in a.rules_data:corpus += [(Path(path).read_bytes(),fixture(7 if profile=='base' else 12,c),profile,'owner-rules') for c in range(32)]
 for profile,rulespath,save in a.original:corpus.append((Path(rulespath).read_bytes(),Path(save).read_bytes(),profile,'owner-world'))
 digest=hashlib.sha256();accepted=rejected=guards=0;records=[]
 for i,(text,data,profile,kind) in enumerate(corpus):
  rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);want,ok,bad,g,_=model(text,data,profile);r=subprocess.run([*runner,str(exe),profile,str(rp),str(sp)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30)
  assert r.returncode==0 and r.stdout==want,(i,r.returncode,r.stderr,len(r.stdout),len(want),next((j for j,(x,y) in enumerate(zip(r.stdout,want)) if x!=y),None));assert sp.read_bytes()==data and rp.read_bytes()==text
  digest.update(r.stdout);accepted+=ok;rejected+=bad;guards+=g;records.append(dict(profile=profile,kind=kind,sha256=hashlib.sha256(r.stdout).hexdigest()))
  if i==191:public=digest.hexdigest()
 receipt=dict(worlds=len(corpus),acceptedBases=accepted,rejectedBases=rejected,guards=guards,publicOutputSHA256=public,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),records=records,command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
