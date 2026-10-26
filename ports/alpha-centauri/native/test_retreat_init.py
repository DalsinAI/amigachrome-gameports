# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,struct,subprocess,json,hashlib,os
from pathlib import Path
from test_world_basic_combat import controls as parent_controls
from test_original_game import decode
from test_units import reference,sections,material
from test_saved_speed import calculate
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','combat_round.cpp','retreat_init.cpp','world_retreat_init.cpp','retreat_init_probe.cpp']
def controls():
 rows=list(parent_controls())
 for variant in ['base','expansion']:
  _,_,text,data=next(x for x in rows if x[1]==variant)
  for k in range(16):
   b=bytearray(data);info,o,sp=decode(b);t=sections(text);n=int(info['vehicles']);width=int(info['width'])
   for i,row in enumerate(t['CHASSIS']):row[8]=str(1+i%3);row[9]='0';row[11]='0'
   for row in t['WEAPONS']:row[2]='3';row[3]='0'
   t['WEAPONS'][0][2]='0'
   for f in range(8):struct.pack_into('<i',b,o['faction_definitions']+f*0x59c+0x50c,0)
   for j in range(int(info['tile_count'])) if 'tile_count'in info else range(len(sp['tiles'])//44):struct.pack_into('<I',b,o['tiles']+j*44+8,0)
   for i in range(n):
    at=o['vehicles']+52*i;old=struct.unpack_from('<h',b,at+10)[0];uid=320+i;p=o['prototypes']+52*uid;b[p:p+52]=b[o['prototypes']+52*old:o['prototypes']+52*(old+1)]
    x=2*(i%(width//2));y=2*(i//(width//2));struct.pack_into('<hh',b,at,x,y);struct.pack_into('<h',b,at+10,uid);struct.pack_into('<hh',b,at+48,-1,-1);b[at+14]=1;struct.pack_into('<I',b,at+4,64 if k==3 else 0);b[at+17]=2 if k==4 else 0;b[at+16]=[0,1,9,10,39,254,255][(i+k)%7];b[at+36]=3;struct.pack_into('<h',b,at+46,-1)
    b[p+36]=i%3;b[p+37]=0 if k==5 else 1;b[p+39]=[1,2,3,4][(i+k)%4];b[p+42]=[0,12,255][(i+k)%3];struct.pack_into('<I',b,p+32,[0,0x200,0x2000000,0x2000200][(i+k)%4]);tile=o['tiles']+((y*width+x)//2)*44;struct.pack_into('<I',b,tile+8,2|([0x800,0x40000,1][k-6]if 6<=k<=8 else 0))
   if k==8:struct.pack_into('<hh',b,o['bases'],0,0)
   if k in [9,10] and n>1:
    first=o['vehicles'];second=first+52;x,y=struct.unpack_from('<hh',b,first);ox,oy=struct.unpack_from('<hh',b,second);struct.pack_into('<I',b,o['tiles']+((oy*width+ox)//2)*44+8,0);struct.pack_into('<hh',b,second,x,y)
    if k==9:struct.pack_into('<h',b,first+48,1);struct.pack_into('<h',b,second+50,0)
   if k==11:
    for row in t['CHASSIS']:row[9]='2'
   if k==12:
    for row in t['CHASSIS']:row[11]='1'
   if variant=='expansion' and k>=13:
    ability=9 if k==13 else 25;t['ABILITIES'][ability][2]='None' if k!=15 else 'Disable'
    owner=5;at=o['faction_definitions']+owner*0x59c;struct.pack_into('<i',b,at+0x50c,1);struct.pack_into('<i',b,at+0x510,15);struct.pack_into('<i',b,at+0x530,ability)
   yield f'{variant}-{k}',variant,material(t),bytes(b)
def expected(text,data):
 info,o,s=decode(data);v=int(info['version'])>=11;tab=reference(text,'expansion'if v else'base');n=int(info['vehicles']);width=int(info['width']);road=int(sections(text)['RULES'][0][0]);profiles=[]
 for i in range(n):
  a=s['vehicles'][i*52:(i+1)*52];uid=struct.unpack_from('<h',a,10)[0];p=s['prototypes'][uid*52:(uid+1)*52];flags=struct.unpack_from('<I',p,32)[0];triad=int(tab[f'chassis.{p[36]}.triad']);missile=int(tab[f'chassis.{p[36]}.missile']);speed=calculate([int(v),uid,p[36],p[37],p[39],flags,p[42],road,a[16],8,0],tab)[1];power=1 if p[42]==12 else min(100,max(1,p[39]))*10;health=max(0,power-a[16]);effective=flags
  if v:
   owner=uid//64;f=s['faction_definitions'][owner*0x59c:(owner+1)*0x59c];fc=struct.unpack_from('<i',f,0x50c)[0]
   for q in range(fc):
    if struct.unpack_from('<i',f,0x510+q*4)[0]==15:
     ability=struct.unpack_from('<i',f,0x530+q*4)[0]
     if int(tab[f'abilities.{ability}.prerequisite'])==-1:effective|=1<<ability
  profiles.append((a,p,flags,triad,missile,speed,health,effective))
 out=[]
 for i,(a,ap,af,at,am,asp,ah,ae)in enumerate(profiles):
  for j,(b,bp,bf,bt,bm,bsp,bh,be)in enumerate(profiles):
   if at or bt or am or bm or (not v and (af|bf)&~((1<<24)-1)):out+=[1,0,0];continue
   if asp>255 or bsp>255:out+=[2,0,0];continue
   # Synthetic isolated stacks, or the deliberate paired case; disconnected
   # same-coordinate fixture is refused instead of silently repaired.
   x,y=struct.unpack_from('<hh',b);ids=[q for q in range(n)if struct.unpack_from('<hh',s['vehicles'],q*52)==(x,y)];tile=struct.unpack_from('<I',s['tiles'],((y*width+x)//2)*44+8)[0]
   if len(ids)>1 and all(struct.unpack_from('<hh',s['vehicles'],q*52+48)==(-1,-1)for q in ids):out+=[1,0,0];continue
   bs=308 if v else 304;matches=sum(struct.unpack_from('<hh',s['bases'],q*bs)==(x,y)for q in range(int(info['bases'])))
   if tile&1 and matches!=1:out+=[1,0,0];continue
   eligible=asp<bsp and int(tab[f'weapons.{bp[37]}.offense'])!=0 and not(struct.unpack_from('<I',b,4)[0]&64)and (not(ae&0x200)or(v and be&0x2000000))and b[17]!=2 and not(tile&(1|0x40800))and len(ids)==1
   out += [0,bh,(bh+1)//2 if eligible else 0]
 out+= [value if index%3!=2 else 0 for index,value in enumerate(out)]
 return struct.pack('<%dI'%(len(out)+1),*out,17),n*n*2
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();src=Path(__file__).resolve().parent;d=a.out.resolve();d.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti'];flags+=['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all'];exe=d/'probe';subprocess.run([a.cc,*flags,*[str(src/f)for f in SOURCES],'-o',str(exe)],check=True)
 digest=hashlib.sha256();total=0
 for name,variant,text,data in controls():
  folder=d/name;folder.mkdir();(folder/'rules.txt').write_bytes(text);(folder/'world.sc').write_bytes(data);want,count=expected(text,data);cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(folder/'rules.txt'),str(folder/'world.sc')];r=subprocess.run(cmd,capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(folder/'stderr').write_bytes(r.stderr);assert not r.returncode,(name,r.returncode,r.stderr)
  (folder/'actual.bin').write_bytes(r.stdout);(folder/'expected.bin').write_bytes(want);assert r.stdout==want,(name,[(i,struct.unpack_from('<3I',r.stdout,i*12),struct.unpack_from('<3I',want,i*12))for i in range(count)if r.stdout[i*12:(i+1)*12]!=want[i*12:(i+1)*12]][:3]);digest.update(r.stdout);total+=count
 receipt={'worlds':32,'queries':total,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest()};(d/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
