# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess
import test_land_combat as parent
from test_original_game import decode
from test_units import sections,material,reference
from test_world_unit_stats import FACTION_SCHEMA
from test_saved_speed import calculate
SOURCES=[s for s in parent.SOURCES if s!='land_combat_probe.cpp']+['battle_entry.cpp','world_battle_entry.cpp','world_battle_entry_probe.cpp']
def controls():
 for name,variant,text,data in parent.controls():
  k=int(name.rsplit('-',1)[1])
  if k>=3:continue
  b=bytearray(data);info,o,sp=decode(b);version=int(info['version']);fs=0x20cc if version>=11 else 0x20bc
  schema=FACTION_SCHEMA['Faction'if version>=11 else'LegacyFactionV10']
  se=next(f['offset']for f in schema['fields']if f['name']=='SE_morale')
  for f in range(8):
   struct.pack_into('<i',b,o['factions']+fs*f+se,0)
   mf=FACTION_SCHEMA['MFaction'];rm=next(x['offset']for x in mf['fields']if x['name']=='rule_morale');struct.pack_into('<i',b,o['faction_definitions']+0x59c*f+rm,0);struct.pack_into('<i',b,o['faction_definitions']+0x59c*f+0x50c,0)
  for i in range(int(info['vehicles'])):
   at=o['vehicles']+52*i;uid=struct.unpack_from('<h',b,at+10)[0];struct.pack_into('<I',b,o['prototypes']+uid*52+32,0);b[o['prototypes']+uid*52+42]=0;b[at+14]=1+i%7;b[at+16]=(i+k)%10;b[at+36]=(i+k)%7;b[at+40]=[0,1,2,3,255][(i+k)%5];struct.pack_into('<I',b,at+4,[0,0x1000,0x40000,0x41000,0x80000000][(i+k)%5]);struct.pack_into('<h',b,at+46,-1)
  yield name,variant,text,bytes(b)
  if k==0:
   uid=struct.unpack_from('<h',b,o['vehicles']+10)[0]
   x=bytearray(b);struct.pack_into('<I',x,o['prototypes']+uid*52+32,0x100000);yield name+'-nerve-held',variant,text,bytes(x)
   x=bytearray(b);x[o['vehicles']+14]=0;yield name+'-native-held',variant,text,bytes(x)
   if version>=11:
    x=bytearray(b);at=o['faction_definitions']+(uid//64)*0x59c;struct.pack_into('<i',x,at+0x50c,1);struct.pack_into('<i',x,at+0x510,15);struct.pack_into('<i',x,at+0x530,20);yield name+'-grant-held',variant,text,bytes(x)
   ts=sections(text);ts['RULES'][0][0]='100';yield name+'-speed-overflow',variant,material(ts),bytes(b)
def expected(text,data):
 info,o,sp=decode(data);n=int(info['vehicles']);ed=int(info['version'])>=11;tab=reference(text,'expansion'if ed else'base');road=int(sections(text)['RULES'][0][0]);bs=308 if ed else 304;width=int(info['width'])
 raw,_=parent.expected(text,data);strength=[struct.unpack_from('<8I',raw,j*32)for j in range(n*n)];rows=[];accepted=0;scaled=0;reports=0;zero_penalty_lookup=0
 def s(x):return (x+2**31)%2**32-2**31
 def div(x,d):return abs(x)//d*(-1 if x<0 else 1)
 for count in [0,3,4]:
  for a in range(n):
   av=sp['vehicles'][52*a:52*(a+1)];uid=struct.unpack_from('<h',av,10)[0];ap=sp['prototypes'][52*uid:52*(uid+1)];state=struct.unpack_from('<I',av,4)[0];flags=struct.unpack_from('<I',ap,32)[0]
   for b in range(n):
    bv=sp['vehicles'][52*b:52*(b+1)];duid=struct.unpack_from('<h',bv,10)[0];bp=sp['prototypes'][52*duid:52*(duid+1)];x,y=struct.unpack_from('<hh',bv);items=struct.unpack_from('<I',sp['tiles'],((y*width+x)//2)*44+8)[0];fd=sp['faction_definitions'][uid//64*0x59c:(uid//64+1)*0x59c];grant=ed and uid>=64 and any(struct.unpack_from('<i',fd,0x510+4*i)[0]==15 for i in range(struct.unpack_from('<i',fd,0x50c)[0]))
    e=1 if a==b else 3 if not av[14]or not bv[14]or ap[42]>3 or bp[42]>3 or flags or grant or items&1 or any(struct.unpack_from('<hh',sp['bases'],i*bs)==(x,y)for i in range(int(info['bases'])))else 0
    before=strength[a*n+b]
    if not e and before[0]:e=1
    speed=calculate([int(ed),uid,ap[36],ap[37],ap[39],flags,ap[42],road,av[16],0,av[36]],tab)[1]
    if not e and speed>255:e=4
    if e:rows.append([e]+[0]*19);continue
    off=s(before[1]);pen=0;sc=lookup=append=0;spent=av[40];nc=count
    if not state&0x1000:
     if av[14] and uid!=8 and (not ed or uid!=15):
      rem=min(max(speed-spent,0),999,road);off=div(s(rem*off),road);pen=div(s((rem-road)*100),road);sc=lookup=1
      if pen and nc<4:nc+=1;append=1
     spent=(spent+road)%256
    ds=struct.unpack_from('<I',bv,4)[0]|4;ns=(state&~0x1000)|0x44
    rows.append([0]+list(before[1:])+[speed,road,0,ns,ds,spent,sc,off,pen,nc,lookup,append]);accepted+=1;scaled+=sc;reports+=append;zero_penalty_lookup+=lookup and not pen
 guards=14+(13 if int(info['bases']) else 12) if accepted else 14
 # Four invalid-pointer aliases and ten runtime refusals always execute. The
 # successful World adds thirteen non-null source/rule/runtime output aliases.
 return b''.join(struct.pack('<20I',*(x&0xffffffff for x in row))for row in rows)+struct.pack('<I',guards),dict(queries=len(rows),accepted=accepted,scaled=scaled,reports=reports,zeroPenaltyLookup=zero_penalty_lookup)
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];cmd=[a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/f)for f in SOURCES]+['-o',str(out/'probe')];subprocess.run(cmd,check=True)
 digest=hashlib.sha256();totals={};worlds=0
 for name,variant,text,data in controls():
  d=out/name;d.mkdir();(d/'rules.txt').write_bytes(text);(d/'world.sc').write_bytes(data);want,counts=expected(text,data);(d/'expected.bin').write_bytes(want);r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe'),str(d/'rules.txt'),str(d/'world.sc')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(d/'actual.bin').write_bytes(r.stdout);(d/'stderr').write_bytes(r.stderr);assert r.returncode==0,(name,r.returncode,r.stderr[:1000]);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));digest.update(r.stdout);worlds+=1
  for k,v in counts.items():totals[k]=totals.get(k,0)+v
 assert totals['accepted']and totals['scaled']and totals['reports']and totals['zeroPenaltyLookup'];receipt=dict(worlds=worlds,counts=totals,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
