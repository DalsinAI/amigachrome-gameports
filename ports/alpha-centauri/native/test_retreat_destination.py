# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,struct,json,hashlib,subprocess,os
from test_world_zoc import controls as world_controls,queries as zoc_queries
from test_retreat_init import controls as rule_controls
from test_original_game import decode
from test_units import sections,reference,material
from test_supply_pods import model as pod
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','vehicle_stack.cpp','world_zoc.cpp','supply_pods.cpp','retreat_destination.cpp','retreat_destination_probe.cpp']
def controls():
 rules={name:(variant,text)for name,variant,text,b in rule_controls()}
 for name,data in world_controls():
  variant,text=rules[name];b=bytearray(data);info,o,s=decode(b);k=int(name.rsplit('-',1)[1]);t=sections(text);pf=o['preferences']if variant=='expansion'else o['legacy'];struct.pack_into('<I',b,o['metadata']+8,[0,1,17,0xffffffff][k%4]);struct.pack_into('<I',b,pf+12,0x2000 if k%3==0 else 0)
  for i in range(len(s['tiles'])//44):
   at=o['tiles']+i*44;it=struct.unpack_from('<I',b,at+8)[0];it|=[0,32,0x2000,0x10000000,0x4000000][(i+k)%5];struct.pack_into('<I',b,at+8,it)
  struct.pack_into('<i',b,pf+0x84+16*4,0 if k==14 else -1);struct.pack_into('<i',b,pf+0x84+8*4,0 if k==15 else -1)
  if k>=13:
   at=o['vehicles'];old=struct.unpack_from('<h',b,at+10)[0];b[o['prototypes']+15*52:o['prototypes']+16*52]=b[o['prototypes']+old*52:o['prototypes']+(old+1)*52];struct.pack_into('<h',b,at+10,15)
   if k==13:t['WEAPONS'][1][2]='-1'
  yield name,variant,material(t),bytes(b)
  if k==15:
   fungus=bytearray(b);struct.pack_into('<I',fungus,o['metadata']+8,0);struct.pack_into('<I',fungus,pf+12,0)
   struct.pack_into('<i',fungus,pf+0x84+16*4,-1);struct.pack_into('<i',fungus,pf+0x84+8*4,-1)
   for tile in range(len(s['tiles'])//44):
    at=o['tiles']+tile*44;fungus[at]=96;it=struct.unpack_from('<I',fungus,at+8)[0];struct.pack_into('<I',fungus,at+8,(it&3)|32)
   yield name+'-native-fungus',variant,material(t),bytes(fungus)
  if k==0:
   for rawflat in (2,3):
    held=bytearray(b);struct.pack_into('<I',held,o['metadata']+28,rawflat);yield name+f'-rawflat{rawflat}',variant,material(t),bytes(held)
def expected(text,data):
 info,o,s=decode(data);w=int(info['width']);h=int(info['height']);n=int(info['vehicles']);v=int(info['version'])>=11;tab=reference(text,'expansion'if v else'base');seed=struct.unpack_from('<I',s['metadata'],8)[0];flat=struct.unpack_from('<I',s['metadata'],28)[0]!=0;pf=s['preferences']if v else s['legacy'];rules=struct.unpack_from('<I',pf,12)[0];fs=0x20cc if v else 0x20bc;bs=308 if v else 304;lookup={q:e for q,e in zoc_queries(data)};rows=[]
 for net in range(2):
  for incoming in range(8):
   for actor in range(n):
    a=s['vehicles'][actor*52:(actor+1)*52];x,y=struct.unpack_from('<hh',a);fac=a[14];uid=struct.unpack_from('<h',a,10)[0];p=s['prototypes'][uid*52:(uid+1)*52];ch=p[36]
    if not -128<=int(tab[f'weapons.{p[37]}.offense'])<=127 or struct.unpack_from('<I',s['metadata'],28)[0]>1 or int(tab[f'chassis.{ch}.triad'])or int(tab[f'chassis.{ch}.missile'])or lookup[x,y,fac,net,0][0]:rows.append([1,0,0,0,0]);continue
    native=uid<64 and(int(tab[f'weapons.{p[37]}.offense'])<0 or(v and uid==15));mask=0;best=0;direction=-1;tx=ty=0
    for i,(dx,dy)in enumerate([(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]):
     xx=x+dx;yy=y+dy
     if not flat:xx=xx+w if xx<0 else xx-w if xx>=w else xx
     if not(0<=xx<w and 0<=yy<h):continue
     if lookup[xx,yy,fac,net,1][1]:continue
     t=s['tiles'][((yy*w+xx)//2)*44:((yy*w+xx)//2+1)*44];items=struct.unpack_from('<I',t,8)[0]
     if(t[0]&224)<96 or pod([w,h,seed,t[0],items,xx,yy,rules,61,11])[1]:continue
     project=struct.unpack_from('<i',pf,0x84+16*4)[0];pholus=project>=0 and s['bases'][project*bs+4]==fac
     if items&32 and(t[0]&224)>=64 and not(native or pholus):continue
     owner=t[2]&15
     if items&3 and owner<8 and owner!=fac and not(struct.unpack_from('<I',s['factions'],owner*fs+0x18+fac*4)[0]&1):continue
     mask|=1<<i;angle=abs(i-(incoming^4))%8;angle=min(angle,8-angle)
     if angle>2 and angle>best:best=angle;direction=i;tx=xx;ty=yy
    rows.append([0,direction&0xffffffff,tx,ty,mask])
 return b''.join(struct.pack('<5I',*row)for row in rows)+struct.pack('<I',19),len(rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();d=a.out.resolve();d.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti'];flags+=['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all'];exe=d/'probe';subprocess.run([a.cc,*flags,*[str(src/f)for f in SOURCES],'-o',str(exe)],check=True);digest=hashlib.sha256();total=0;worlds=0
 for name,variant,text,data in controls():
  f=d/name;f.mkdir();(f/'world.sc').write_bytes(data);(f/'rules.txt').write_bytes(text);want,count=expected(text,data);cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(f/'rules.txt'),str(f/'world.sc')];r=subprocess.run(cmd,capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(f/'stderr').write_bytes(r.stderr);assert not r.returncode,(name,r.returncode,r.stderr);(f/'actual.bin').write_bytes(r.stdout);(f/'expected.bin').write_bytes(want);assert r.stdout==want,(name,[(i,struct.unpack_from('<5I',r.stdout,i*20),struct.unpack_from('<5I',want,i*20))for i in range(count)if r.stdout[i*20:(i+1)*20]!=want[i*20:(i+1)*20]][:3]);digest.update(r.stdout);total+=count;worlds+=1
 receipt={'worlds':worlds,'queries':total,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest()};(d/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
