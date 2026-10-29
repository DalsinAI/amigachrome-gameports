# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import struct,json,hashlib,subprocess,os,argparse
from test_land_engagement import controls as parent_controls,SOURCES as PARENT_SOURCES
from test_land_combat import expected as land_expected
from test_retreat_init import expected as init_expected
from test_retreat_destination import expected as destination_expected
from test_original_game import decode
from test_units import reference,sections,material
from test_saved_speed import calculate,clamp
from test_world_unit_stats import scalar
from test_original_faction import SCHEMA
SOURCES=[s for s in PARENT_SOURCES if s!='land_engagement_probe.cpp']+['vehicle_removal.cpp','world_vehicle_removal.cpp','world_casualty.cpp','artifact_capture.cpp','land_resolution.cpp','land_resolution_probe.cpp']
def u32(b,o):return struct.unpack_from('<I',b,o)[0]
def set32(b,o,n):struct.pack_into('<I',b,o,n&0xffffffff)
def sh(b,o):return struct.unpack_from('<h',b,o)[0]
def put(b,o,n):struct.pack_into('<h',b,o,n)
def controls():
 for name,variant,text,data in parent_controls():
  yield name,variant,text,data
  if name.endswith('-retreat'):
   b=bytearray(data);info,o,sp=decode(b);uid=sh(b,o['vehicles']+52+10);proto=o['prototypes']+uid*52;b[proto+42]=12;b[proto+36]=0;b[o['vehicles']+52+14]=2;b[o['vehicles']+52+16]=0;set32(b,o['vehicles']+52+4,0x800000);struct.pack_into('<H',b,o['vehicles']+52+8,32)
   fs=len(sp['factions'])//8;b[o['factions']+2*fs:o['factions']+3*fs]=b[o['factions']+fs:o['factions']+2*fs];b[o['faction_definitions']+2*0x59c:o['faction_definitions']+3*0x59c]=b[o['faction_definitions']+0x59c:o['faction_definitions']+2*0x59c]
   set32(b,o['vehicles']+2*52+4,0x800000);struct.pack_into('<H',b,o['vehicles']+2*52+8,32);pf=o['preferences']if variant=='expansion'else o['legacy'];set32(b,pf,u32(b,pf)&~0x40000)
   yield name+'-capture',variant,text,bytes(b)
def engagement_expected(text,data):
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
        de,di,dx,dy,mask=struct.unpack_from('<5I',dest,b*20)
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
def expected(text,data):
 info,o,sp=decode(data);n=int(info['vehicles']);w=int(info['width']);nt=len(sp['tiles'])//44;modern=int(info['version'])>=11;fs=len(sp['factions'])//8;active=0x858 if modern else 0x854;lost=0xc58 if modern else 0xc54;combat=0x1058 if modern else 0x1054;counter=0x2084 if modern else 0x2074;tab=reference(text,'expansion'if modern else'base');ts=sections(text);road=clamp(int(ts['RULES'][0][0]),1,100)
 raw,_=engagement_expected(text,data);pos=0;out=bytearray();counts=dict(accepted=0,rejected=0,retreated=0,attacker_deleted=0,defender_deleted=0,defender_captured=0,held_interlude=0,rounds=0,draws=0)
 for profile in range(2):
  for a in range(n):
   for b in range(n):
    row=list(struct.unpack_from('<21I',raw,pos));pos+=84;actors=tiles=None
    if row[0]==0:actors=bytearray(raw[pos:pos+n*52]);pos+=n*52;tiles=bytearray(raw[pos:pos+nt*44]);pos+=nt*44
    held=False
    for id in (a,b):
     v=sp['vehicles'][id*52:(id+1)*52];uid=sh(v,10);p=sp['prototypes'][uid*52:(uid+1)*52];mf=sp['faction_definitions'][uid//64*0x59c:(uid//64+1)*0x59c];bc=struct.unpack_from('<i',mf,0x50c)[0]
     held|=not v[14] or sh(v,48)!=-1 or sh(v,50)!=-1 or p[40]!=0 or bool(u32(p,32)&128)or(modern and uid>=64 and any(struct.unpack_from('<i',mf,0x510+4*j)[0]==15 for j in range(bc)))
    error=1 if held or a==b else row[0]
    if error:out+=struct.pack('<21I',error,*([0]*20));counts['rejected']+=1;continue
    if row[15]==0 and row[16]==0:out+=struct.pack('<21I',1,*([0]*20));counts['rejected']+=1;continue
    factions=bytearray(sp['factions']);pools=[int(profile==0),0x7fffffff,1,0x7fffffff,int(profile==0),0xfffffffb];runtime=[0]*8;boundary=row[20];kind=0;victim=other=arg6=source_return=-1;aa=a;bb=b;currentn=n
    if boundary!=2:
     victim=a if boundary==0 else b;other=-1 if boundary==0 else a;arg6=6 if boundary==0 else 7;v=bytes(actors[victim*52:(victim+1)*52]);uid=sh(v,10);p=sp['prototypes'][uid*52:(uid+1)*52];faction=v[14];sx,sy=sh(v,0),sh(v,2)
     if p[42]==12:
      dest=actors[a*52:(a+1)*52];dx,dy=sh(dest,0),sh(dest,2)
      if boundary==0 or dest[14]==0 or int(tab[f'chassis.{p[36]}.triad'])!=0 or int(tab[f'chassis.{sp["prototypes"][sh(dest,10)*52+36]}.triad'])!=0 or int(tab[f'chassis.{sp["prototypes"][sh(dest,10)*52+36]}.missile']) or tiles[((dy*w+dx)//2)*44]&224<96:
       out+=struct.pack('<21I',1,*([0]*20));counts['rejected']+=1;continue
     elif u32(v,4)&0x800000 and other>=0 and not u32(sp['preferences']if modern else sp['legacy'],0)&0x40000:
      out+=struct.pack('<21I',1,*([0]*20));counts['rejected']+=1;counts['held_interlude']+=1;continue
     at=faction*fs+lost+uid*2;struct.pack_into('<H',factions,at,(struct.unpack_from('<H',factions,at)[0]+1)&65535)
     nxt,prev=sh(v,48),sh(v,50)
     if prev>=0:put(actors,prev*52+48,nxt)
     if nxt>=0:put(actors,nxt*52+50,prev)
     elif prev<0:at=((sy*w+sx)//2)*44+8;set32(tiles,at,u32(tiles,at)&~2)
     for off in (0,2,48,50):put(actors,victim*52+off,-1)
     at=faction*fs+active+uid;factions[at]=(factions[at]-1)&255
     if p[42]==12:
      newfac=actors[a*52+14];dx,dy=sh(actors,a*52),sh(actors,a*52+2);actors[victim*52+14]=newfac;actors[victim*52+16]=0;actors[victim*52+17]=0;set32(actors,victim*52+4,u32(v,4)&0xfcffbdff);at=newfac*fs+active+uid;factions[at]=(factions[at]+1)&255
      top=next(j for j in range(n)if j!=victim and sh(actors,j*52)==dx and sh(actors,j*52+2)==dy and sh(actors,j*52+50)==-1);put(actors,victim*52,dx);put(actors,victim*52+2,dy);put(actors,victim*52+48,top);put(actors,victim*52+50,-1);put(actors,top*52+50,victim);at=((dy*w+dx)//2)*44+8;set32(tiles,at,u32(tiles,at)|0x100002)
      assert uid>=64 and sh(v,46)==-1 # bounded fixtures; independent preexisting morale/speed model
      state=factions[newfac*fs:(newfac+1)*fs];definition=sp['faction_definitions'][newfac*0x59c:(newfac+1)*0x59c];m=clamp(scalar(state,SCHEMA['Faction'if modern else'LegacyFactionV10'],'SE_morale'),-4,4);m+=1 if m<=-2 else -1 if m>=2 else 0;m+=scalar(definition,SCHEMA['MFaction'],'rule_morale')
      if scalar(definition,SCHEMA['MFaction'],'rule_flags')&0x8000 and m<0:m=0
      morale=clamp(m+v[36],0,6);proto_speed,speed=calculate([int(modern),uid,p[36],p[37],p[39],u32(p,32),12,road,0,0,morale],tab)
      if speed>255:raise ValueError('unsupported capture speed in bounded fixture')
      actors[victim*52+40]=speed;pools[3]=(pools[3]+1)&0xffffffff;kind=3;source_return=0;runtime=[0,a,b,n,b,-1,sx,sy]
     else:
      if struct.unpack_from('<H',v,8)[0]&32:at=arg6*fs+counter;set32(factions,at,u32(factions,at)+1)
      if p[42]<8:at=faction*fs+combat;set32(factions,at,u32(factions,at)-1)
      actors[victim*52:(n-1)*52]=actors[(victim+1)*52:]
      for j in range(n-1):
       for off in (48,50):
        if sh(actors,j*52+off)>victim:put(actors,j*52+off,sh(actors,j*52+off)-1)
       ref=sh(actors,j*52+20)
       if actors[j*52+17]==1:
        if ref==victim:actors[j*52+17]=0
        elif ref>victim:put(actors,j*52+20,ref-1)
      refs=[0,a,b,n];refs=[-1 if q==victim else q-1 if q>victim else q for q in refs];runtime=refs+[b-int(b!=0 and b>=victim),victim,sx,sy];currentn=n-1;actors=actors[:currentn*52];aa=-1 if boundary==0 else a-int(a>victim);bb=b-int(b>victim)if boundary==0 else-1;kind=1 if boundary==0 else 2;source_return=1
      if pools[0]:pools[1]=(pools[1]+1)&0xffffffff
    out+=struct.pack('<21I',*row)+struct.pack('<9I',kind,victim&0xffffffff,other&0xffffffff,arg6&0xffffffff,source_return&0xffffffff,aa&0xffffffff,bb&0xffffffff,currentn,int(boundary!=2))+actors+tiles
    for j in range(8):out+=factions[j*fs+active:j*fs+active+512]+factions[j*fs+lost:j*fs+lost+1024]+factions[j*fs+combat:j*fs+combat+4]+factions[j*fs+counter:j*fs+counter+4]+factions[j*fs+0x220:j*fs+0x220+4]
    out+=struct.pack('<8i',*runtime)+struct.pack('<6I',*pools);counts['accepted']+=1;counts[['retreated','attacker_deleted','defender_deleted','defender_captured'][kind]]+=1;counts['rounds']+=row[8];counts['draws']+=row[9]
 assert pos==len(raw)
 return bytes(out),counts

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--reuse',type=Path);args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if args.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);exe=out/'probe';subprocess.run([args.cc,*flags,*[str(src/f)for f in SOURCES],'-o',str(exe)],check=True);digest=hashlib.sha256();total={};worlds=0
 cs=((d.name,'expansion'if 'expansion'in d.name else'base',(d/'rules.txt').read_bytes(),(d/'world.sc').read_bytes())for d in sorted(args.reuse.iterdir(),key=lambda d:(d.name.split('-')[0],int(d.name.split('-')[1])if len(d.name.split('-'))>1 else 0,d.name))if d.is_dir())if args.reuse else controls()
 for name,variant,text,data in cs:
  d=out/name;d.mkdir();(d/'world.sc').write_bytes(data);(d/'rules.txt').write_bytes(text)
  if args.reuse:want=(args.reuse/name/'expected.bin').read_bytes();counts=json.loads((args.reuse/name/'counts.json').read_text())
  else:want,counts=expected(text,data)
  (d/'counts.json').write_text(json.dumps(counts));(d/'expected.bin').write_bytes(want);r=subprocess.run((['qemu-m68k','-cpu','m68040']if args.m68k_linux else[])+[str(exe),str(d/'rules.txt'),str(d/'world.sc')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(d/'actual.bin').write_bytes(r.stdout);(d/'stderr').write_bytes(r.stderr);assert not r.returncode,(name,r.returncode,r.stderr);assert r.stderr==f"MIDDLE_CLONE_GUARD {int(bool(counts['attacker_deleted']+counts['defender_deleted']+counts['defender_captured']))}\n".encode(),(name,r.stderr);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));digest.update(r.stdout);worlds+=1
  for k,v in counts.items():total[k]=total.get(k,0)+v
 assert total['defender_captured'] and total['attacker_deleted'] and total['defender_deleted'] and total['retreated']
 receipt={'worlds':worlds,'counts':total,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
