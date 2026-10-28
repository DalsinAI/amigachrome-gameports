# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import struct,json,hashlib,subprocess,os,argparse
from test_land_combat import controls as land_controls,expected as land_expected,SOURCES as LAND_SOURCES
from test_retreat_init import expected as init_expected
from test_retreat_destination import expected as destination_expected
from test_original_game import decode
from test_units import reference,sections,material
SOURCES=[x for x in LAND_SOURCES if x!='land_combat_probe.cpp']+['vehicle_stack.cpp','combat_round.cpp','retreat_init.cpp','world_retreat_init.cpp','world_zoc.cpp','supply_pods.cpp','retreat_destination.cpp','land_engagement.cpp','land_engagement_probe.cpp']
def controls():
 for name,variant,text,data in land_controls():
  b=bytearray(data);info,o,s=decode(b);width=int(info['width']);groups={}
  for i in range(int(info['vehicles'])):
   at=o['vehicles']+i*52;x,y=struct.unpack_from('<hh',b,at);groups.setdefault((x,y),[]).append(i);b[at+16]=[0,1,5,9][(i+int(name.rsplit('-',1)[1]))%4];b[at+17]=0
   uid=struct.unpack_from('<h',b,at+10)[0];b[o['prototypes']+uid*52+40]=0
   if name.endswith('-1'):b[o['prototypes']+uid*52+39]=1+i%4
   if name.endswith('-2')and i==0:b[o['prototypes']+uid*52+42]=5
   if name.endswith('-3')and i==0:b[o['prototypes']+uid*52+42]=12
  for(x,y),ids in groups.items():
   for k,i in enumerate(ids):struct.pack_into('<hh',b,o['vehicles']+i*52+48,ids[k+1]if k+1<len(ids)else-1,ids[k-1]if k else-1)
   at=o['tiles']+((y*width+x)//2)*44;it=struct.unpack_from('<I',b,at+8)[0];struct.pack_into('<I',b,at+8,it|2)
   if not it&1:b[at+2]=(b[at+2]&240)|b[o['vehicles']+ids[0]*52+14]
  yield name,variant,text,bytes(b)
  if name.endswith('-0'):
   t=sections(text);special=bytearray(b);pf=o['preferences']if variant=='expansion'else o['legacy'];struct.pack_into('<I',special,o['metadata']+8,0);struct.pack_into('<I',special,pf+12,0x2000)
   for ch,row in enumerate(t['CHASSIS']):row[8]='3'if ch==1 else'1';row[9]='0';row[11]='0'
   t['WEAPONS'][1][2]='1';t['DEFENSES'][0][2]='1'
   for f in range(8):struct.pack_into('<i',special,o['faction_definitions']+f*0x59c+0x50c,0);struct.pack_into('<i',special,o['faction_definitions']+f*0x59c+0x508,0)
   for tile in range(len(s['tiles'])//44):
    at=o['tiles']+tile*44;special[at]=96;special[at+5]=0;items=struct.unpack_from('<I',special,at+8)[0];struct.pack_into('<I',special,at+8,items&3)
   for i in range(int(info['vehicles'])):
    at=o['vehicles']+i*52;uid=struct.unpack_from('<h',special,at+10)[0];p=o['prototypes']+uid*52;special[at+14]=1;special[at+16]=0;special[at+17]=0;struct.pack_into('<I',special,at+4,0);special[p+36]=0 if i==0 else 1;special[p+37]=1;special[p+38]=0;special[p+39]=1;special[p+40]=0;special[p+42]=0;struct.pack_into('<I',special,p+32,0)
   bases={struct.unpack_from('<hh',s['bases'],j*(308 if variant=='expansion'else 304))for j in range(int(info['bases']))}
   positions=[(x,y)for y in range(int(info['height']))for x in range(width)if(x+y)%2==0 and(x,y)not in bases];assert len(positions)>=int(info['vehicles'])
   for tile in range(len(s['tiles'])//44):
    at=o['tiles']+tile*44;items=struct.unpack_from('<I',special,at+8)[0];struct.pack_into('<I',special,at+8,items&~2)
   for i in range(int(info['vehicles'])):
    x,y=positions[i];at=o['vehicles']+i*52;struct.pack_into('<hh',special,at,x,y);struct.pack_into('<hh',special,at+48,-1,-1);at=o['tiles']+((y*width+x)//2)*44;items=struct.unpack_from('<I',special,at+8)[0];struct.pack_into('<I',special,at+8,items|2);special[at+2]=(special[at+2]&240)|1
   yield name+'-retreat',variant,material(t),bytes(special)
def expected(text,data):
 info,o,s=decode(data);n=int(info['vehicles']);width=int(info['width']);tab=reference(text,'expansion'if int(info['version'])>=11 else'base');strength,_=land_expected(text,data);initial,_=init_expected(text,data);dest,_=destination_expected(text,data);want=bytearray();counts={'accepted':0,'rejected':0,'retreated':0,'attacker_casualty':0,'defender_casualty':0,'rounds':0,'draws':0}
 for profile in range(2):
  for a in range(n):
   for b in range(n):
    error,*st=struct.unpack_from('<8I',strength,(a*n+b)*32);ie,health,counter=struct.unpack_from('<3I',initial,(a*n+b)*12);va=s['vehicles'][a*52:(a+1)*52];vb=s['vehicles'][b*52:(b+1)*52];ua=struct.unpack_from('<h',va,10)[0];ub=struct.unpack_from('<h',vb,10)[0];pa=s['prototypes'][ua*52:(ua+1)*52];pb=s['prototypes'][ub*52:(ub+1)*52]
    if a==b or error or ie or not(1<=pa[39]<=4 and 1<=pb[39]<=4)or not(0<=pa[42]<=3 and(pb[42]<=3 or pb[42]==12)):
     want+=struct.pack('<21I',2 if a!=b and(error==2 or ie==2)else 1,*([0]*20));counts['rejected']+=1;continue
    seed=0xffffffff if profile else 0;ad=va[16];bd=vb[16];rounds=draws=0;ic=counter;direction=0xffffffff;x=y=0;vehicles=bytearray(s['vehicles']);tiles=bytearray(s['tiles']);boundary=0
    while True:
     ah=max(0,(1 if pa[42]==12 else pa[39]*10)-ad);bh=max(0,(1 if pb[42]==12 else pb[39]*10)-bd)
     if not ah or not bh:boundary=0 if not ah else 1;break
     ar=dr=0
     if st[0]>1:seed=(214013*seed+2531011)&0xffffffff;ar=((seed>>16)&32767)%st[0];draws+=1
     if st[1]>1:seed=(214013*seed+2531011)&0xffffffff;dr=((seed>>16)&32767)%st[1];draws+=1
     rounds+=1;assert rounds<=80
     if ar<=dr:ad+=1
     else:
      bd+=1;bh=max(0,(1 if pb[42]==12 else pb[39]*10)-bd)
      if bh and counter:
       counter-=1
       if not counter:
        de,di,dx,dy,mask=struct.unpack_from('<5I',dest,((profile*8+(3 if profile else 0))*n+b)*20)
        if de:error=de;break
        if di!=0xffffffff:
         mf=(ub//64)*(len(s['faction_definitions'])//8);bonus=struct.unpack_from('<i',s['faction_definitions'],mf+0x50c)[0]
         if pb[40]or struct.unpack_from('<I',pb,32)[0]&128 or(int(info['version'])>=11 and ub>=64 and any(struct.unpack_from('<i',s['faction_definitions'],mf+0x510+4*j)[0]==15 for j in range(bonus))):error=1;break
         direction=di;x=dx;y=dy;boundary=2;sx,sy=struct.unpack_from('<hh',vb);source=((sy*width+sx)//2)*44;target=((y*width+x)//2)*44;items=struct.unpack_from('<I',tiles,source+8)[0];struct.pack_into('<I',tiles,source+8,items&~2);top=next((j for j in range(n)if struct.unpack_from('<hh',vehicles,j*52)==(x,y)and struct.unpack_from('<h',vehicles,j*52+50)[0]==-1),-1);struct.pack_into('<hh',vehicles,b*52,x,y);struct.pack_into('<hh',vehicles,b*52+48,top,-1)
         if top>=0:struct.pack_into('<h',vehicles,top*52+50,b)
         elif not struct.unpack_from('<I',tiles,target+8)[0]&1:tiles[target+2]=(tiles[target+2]&240)|vb[14]
         items=struct.unpack_from('<I',tiles,target+8)[0]|2|(0x100000 if vb[14]else 0);struct.pack_into('<I',tiles,target+8,items);ah=max(0,(1 if pa[42]==12 else pa[39]*10)-ad);break
    if error:want+=struct.pack('<21I',error,*([0]*20));counts['rejected']+=1;continue
    vehicles[a*52+16]=ad;vehicles[b*52+16]=bd;row=[0,*st,rounds,draws,ic,counter,seed,ad,bd,ah,bh,direction,x,y,boundary];assert len(row)==21;want+=struct.pack('<21I',*row)+vehicles+tiles;counts['accepted']+=1;counts[['attacker_casualty','defender_casualty','retreated'][boundary]]+=1;counts['rounds']+=rounds;counts['draws']+=draws
 return bytes(want),counts
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all']);exe=out/'probe';subprocess.run([a.cc,*flags,*[str(src/f)for f in SOURCES],'-o',str(exe)],check=True);digest=hashlib.sha256();totals={};worlds=0
 for name,variant,text,data in controls():
  d=out/name;d.mkdir();(d/'world.sc').write_bytes(data);(d/'rules.txt').write_bytes(text);want,counts=expected(text,data);(d/'counts.json').write_text(json.dumps(counts));cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(d/'rules.txt'),str(d/'world.sc')];r=subprocess.run(cmd,capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(d/'stderr').write_bytes(r.stderr);(d/'actual.bin').write_bytes(r.stdout);(d/'expected.bin').write_bytes(want);assert not r.returncode,(name,r.returncode,r.stderr);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));digest.update(r.stdout);worlds+=1
  for k,v in counts.items():totals[k]=totals.get(k,0)+v
 receipt={'worlds':worlds,'counts':totals,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest()};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
