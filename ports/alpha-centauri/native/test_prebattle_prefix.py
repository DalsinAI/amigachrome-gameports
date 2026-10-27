# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess,functools
import test_world_combat_text as parent
from test_original_game import decode
from prebattle_prefix_sources import SOURCES
#The numeric wire model depends only on immutable rules/world bytes, not labels.
parent.parent.expected=functools.lru_cache(maxsize=2)(parent.parent.expected)
def pack(ns):return struct.pack('<'+'I'*len(ns),*(x&0xffffffff for x in ns))
def signed(n):return(n+2**31)%2**32-2**31
def fixtures():
 for name,ed,text,data in parent.parent.controls():
  if name not in ['base-0','base-1','base-2','expansion-0','expansion-1','expansion-2']:continue
  for profile in range(3):
   b=bytearray(data);info,o,sp=decode(b);fs=len(sp['factions'])//8;mil=0x1060 if int(info['version'])>=11 else 0x105c
   for f in range(8):
    struct.pack_into('<i',b,o['factions']+fs*f+mil,-1 if f==4 else 1)
    for j in range(8):
     struct.pack_into('<I',b,o['factions']+fs*f+0x18+4*j,(2 if profile==1 else 8)if f!=j else 0)
     struct.pack_into('<i',b,o['factions']+fs*f+0x1ac+4*j,41+f+j)
     struct.pack_into('<i',b,o['factions']+fs*f+0x78+4*j,-37)
     struct.pack_into('<i',b,o['factions']+fs*f+0x58+4*j,2147483647 if profile==0 else -1)
   for i in range(len(sp['tiles'])//44):b[o['tiles']+44*i+7]=255 if profile==2 else 4
   for f,j,value in [(1,2,2),(2,1,2 if profile==1 else 8),(1,4,1),(2,4,0x2008),(4,2,0x802000),(7,2,1)]:struct.pack_into('<I',b,o['factions']+fs*f+0x18+4*j,value)
   yield name+'-dip'+str(profile),text,bytes(b)
def expected(text,data,labels):
 raw,_=parent.expected(text,data,labels);info,o,sp=decode(data);count=int(info['vehicles']);fs=len(sp['factions'])//8;width=int(info['width']);mil=0x1060 if int(info['version'])>=11 else 0x105c
 packets=[raw[i*660:(i+1)*660]for i in range(count*count)];pairs=[(i//count,i%count)for i,p in enumerate(packets)if struct.unpack_from('<I',p)[0]==0][:3]+[(0,0)];pairs=list(dict.fromkeys(pairs));inputs=[];wanted=[];codes={};accepted=0
 for a,b in pairs:
  source=packets[a*count+b];numeric=list(struct.unpack_from('<20I',source));af=sp['vehicles'][52*a+14];bf=sp['vehicles'][52*b+14];x,y=struct.unpack_from('<hh',sp['vehicles'],52*a);idx=y*(width//2)+x//2;owner=struct.unpack_from('<b',sp['tiles'],44*idx+7)[0]
  for mode in range(12):
   obs=0 if mode==5 else 1;pbem=int(mode==6);current=bf if mode==3 else af if mode==4 else 8 if mode==7 else 7;human=255 if mode in [1,2,3]else 0;popup=int(mode==2);turn=0x80000000;warning=0xa5a5a5a5;nega=bf;negb=owner if owner>=0 else 0xffffffff;neg=0xffffffff;flags=0 if mode==9 else 2;cmode=2 if mode==10 else 0;execution=0 if mode==11 else 1;mp=int(mode==8)
   inputs.append(pack([a,b,obs,pbem,current,human,128,popup,turn,warning,nega,negb,neg,flags,cmode,execution,mp]));error=1 if a==b else 2 if obs!=1 else 1 if current>7 else 3 if pbem else 3 if mp or flags!=2 or cmode!=0 or execution!=1 else numeric[0];extra=[];out=bytearray(data)
   def get(f,at,j=0):return struct.unpack_from('<I',out,o['factions']+fs*f+at+4*j)[0]
   def put(f,at,j,value):struct.pack_into('<I',out,o['factions']+fs*f+at+4*j,value&0xffffffff)
   if not error:
    sf=get(af,0x18,bf);sr=get(bf,0x18,af);spoke=get(bf,0x78,af);route=0;newwarning=warning
    if human>>bf&1 and not(sr&6==0 and((sr&8==0 and sf&0x800==0)or sr&16)):
     route=1 if current==bf else 2 if popup else 3
     if route==3:spoke=turn
    elif(sf|sr)&2 and current not in [af,bf]:newwarning=2 if get(current,0x18,bf)&1 else 3 if get(current,0x18,af)&1 else 1
    error=7 if route in [1,2]else 8 if sr&7 else 9 if current in [af,bf]else 0
    if not error:
     for actor,state in [(a,numeric[11]),(b,numeric[12])]:struct.pack_into('<I',out,o['vehicles']+52*actor+4,state)
     out[o['vehicles']+52*a+40]=numeric[13]
     if route==3:put(bf,0x78,af,spoke)
     reset=bool(sf&7);nsf=(sf&0xfaeef378&0xff7fffff)|0x80008118;nsr=(sr&0xfaeef378&0xff7fffff)|0x80008118;fc=0 if reset else get(af,0x1ac,bf);rc=0 if reset else get(bf,0x1ac,af)
     put(af,0x18,bf,nsf);put(bf,0x18,af,nsr);put(af,0x1ac,bf,fc);put(bf,0x1ac,af,rc)
     if not 0<=owner<=7:error=3
     else:
      os=get(owner,0x18,bf);ds=get(bf,0x18,owner);fr=signed(get(bf,0x58,owner));change=owner in range(1,8)and owner not in [af,bf]and bool(get(af,0x18,owner)&1)and ds&0x18==8 and signed(get(owner,mil))<signed(get(bf,mil));do_friction=bool(change and os&0x800000)
      if change:os|=0x2000;ds|=0x2000
      nn=(neg+3)&0xffffffff if do_friction and nega==bf and negb==owner else neg
      if do_friction:fr=max(0,min(20,signed(fr+3)))
      put(owner,0x18,bf,os);put(bf,0x18,owner,ds);put(bf,0x58,owner,fr)
      extra=[route,newwarning,spoke,nsf,nsr,fc,rc,int(reset),idx,owner,af,bf,os,ds,fr,nn,int(bool(change)),int(do_friction)]
   codes[str(error)]=codes.get(str(error),0)+1
   if error:wanted.append(pack([error])+bytes(732));continue
   accepted+=1;wanted.append(source+pack(extra)+pack([len(out)])+out)
 return b''.join(inputs),b''.join(wanted),dict(cases=len(inputs),accepted=accepted,codes=codes,guards=accepted*16)
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--owner-labels',type=Path);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/f)for f in SOURCES]+['-o',str(out/'probe')],check=True)
 rows=[b'L'+str(i).encode()for i in range(1109)];profiles=[('synthetic',rows,parent.label_file(rows))]
 if a.owner_labels:
  file=a.owner_labels.read_bytes();lines=file.splitlines();at=lines.index(b'#LABELS');n=int(lines[at+1]);profiles.append(('owner',[s.strip(b' ')for s in lines[at+2:at+2+n]],file))
 totals=dict(cases=0,accepted=0,guards=0,codes={});digest=hashlib.sha256();worlds=0
 for name,text,data in fixtures():
  for profile,labels,file in profiles:
   d=out/(name+'-'+profile);d.mkdir();(d/'rules.txt').write_bytes(text);(d/'world.sc').write_bytes(data);(d/'labels.txt').write_bytes(file);raw,want,c=expected(text,data,labels);(d/'inputs.bin').write_bytes(raw);(d/'expected.bin').write_bytes(want);r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe'),str(d/'rules.txt'),str(d/'world.sc'),str(d/'labels.txt')],input=raw,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(d/'actual.bin').write_bytes(r.stdout);(d/'stderr').write_bytes(r.stderr);assert r.returncode==0,(name,profile,r.returncode,r.stderr[:1200]);assert r.stdout==want,(name,profile,len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));assert r.stderr==f"ACCEPTED {c['accepted']} GUARDS {c['guards']}\n".encode();digest.update(r.stdout);worlds+=1
   for k in ['cases','accepted','guards']:totals[k]+=c[k]
   for k,v in c['codes'].items():totals['codes'][k]=totals['codes'].get(k,0)+v
 assert totals['accepted']and all(totals['codes'].get(str(k))for k in [1,2,3,7,8,9]);receipt=dict(worldProfiles=worlds,counts=totals,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),ownerLabelsSupplied=bool(a.owner_labels),native='UNRUN');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
