# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import json,hashlib,subprocess,struct,os,argparse,inspect
import test_land_resolution as parent
from test_battle_aftermath import controls as positioned_controls
from test_promotion_selection import model as selection_model
from test_units import sections,material
from test_original_game import decode
SOURCES=[s for s in parent.SOURCES if s!='land_resolution_probe.cpp']+['world_promotion.cpp','promotion_selection.cpp','promoted_land_body.cpp','promoted_land_body_probe.cpp']
def controls():
 for name,variant,text,data in positioned_controls():
  for mode in range(2):
   b=bytearray(data);info,o,sp=decode(b);fs=len(sp['factions'])//8;pref=o['preferences']if variant=='expansion'else o['legacy'];b[pref+0x58]=254
   for j in range(int(info['vehicles'])):b[o['vehicles']+j*52+36]=0 if mode==0 else 2+j%3;struct.pack_into('<h',b,o['vehicles']+j*52+46,-1)
   for f in range(8):struct.pack_into('<i',b,o['factions']+f*fs+0x288,0);struct.pack_into('<i',b,o['faction_definitions']+f*0x59c+0x4d8,0)
   yield name+'-m'+str(mode),variant,text,bytes(b)
   if name in ['base-0-retreat','expansion-0-retreat']and mode==0:
    t=sections(text)
    for row in t['WEAPONS']:row[2]='99'
    for row in t['DEFENSES']:row[2]='1'
    yield name+'-attacker-strong',variant,material(t),bytes(b)
   if name in ['base-0-retreat','expansion-0-retreat']and mode==1:
    t=sections(text)
    for row in t['WEAPONS']:row[2]='0'
    for row in t['DEFENSES']:row[2]='0'
    for j in range(int(info['vehicles'])):b[o['vehicles']+j*52+36]=3
    yield name+'-weak',variant,material(t),bytes(b)
# Independent wire model: preserve the parent damage/deletion model and insert
# the recovered caller selection and raw morale edit BEFORE dense compaction.
env=dict(vars(parent));env['selection_model']=selection_model
src=inspect.getsource(parent.expected).replace('def expected(','def expected_promoted(')
src=src.replace('held=False','''held=False
    coords=[struct.unpack_from('<hh',sp['vehicles'],j*52)for j in range(n)];bs=308 if modern else 304;bases={struct.unpack_from('<hh',sp['bases'],j*bs)for j in range(int(info['bases']))}
    for id in(a,b):
     v=sp['vehicles'][id*52:(id+1)*52];p=sp['prototypes'][sh(v,10)*52:(sh(v,10)+1)*52];health=max(0,(1 if p[42]==12 else max(1,min(100,p[39]))*10)-v[16]);c=coords[id]
     held|=not health or p[42]==12 or c in bases or bool(u32(sp['tiles'],((c[1]*w+c[0])//2)*44+8)&1)or any(coords[j]==c for j in range(n)if j not in[a,b])''')
needle='    factions=bytearray(sp[\'factions\']);'
insert='''    if row[20]==2:out+=struct.pack('<21I',1,*([0]*20));counts['rejected']+=1;continue
    won=row[20]==1;winner=a if won else b;va=sp['vehicles'][a*52:(a+1)*52];vb=sp['vehicles'][b*52:(b+1)*52];pa=sp['prototypes'][sh(va,10)*52:(sh(va,10)+1)*52];pb=sp['prototypes'][sh(vb,10)*52:(sh(vb,10)+1)*52];ia=max(0,pa[39]*10-va[16]);ib=max(0,pb[39]*10-vb[16]);rank=actors[winner*52+36];eff=min(rank,6)
    sc=[int(won),pa[42],pa[39],row[13],pb[42],pb[39],row[14],row[1],row[2],ia,ib,eff,-1];se=selection_model(sc);assert se[0]==0
    extra=[*se[1:],0,0,0,0,0,0,ia,ib]
    if se[1]==2:out+=struct.pack('<21I',3,*([0]*20));counts['rejected']+=1;counts['selection_random']+=1;continue
    if se[1]==0:out+=struct.pack('<21I',1,*([0]*20));counts['rejected']+=1;continue
    if se[1]==1:
     if eff>2 and eff<6 and rank<6:out+=struct.pack('<21I',4,*([0]*20));counts['rejected']+=1;counts['promotion_random']+=1;continue
     if sh(actors,winner*52+46)!=-1:out+=struct.pack('<21I',1,*([0]*20));counts['rejected']+=1;continue
     promoted=eff<6 and rank<6
     if promoted and profile and actors[winner*52+14]==va[14] and min(rank+1,6)!=eff:out+=struct.pack('<21I',5,*([0]*20));counts['rejected']+=1;counts['promotion_notification']+=1;continue
     actors[winner*52+36]=rank+int(promoted);extra=[*se[1:],1,rank,rank+int(promoted),eff,1,int(promoted),ia,ib]
'''
assert needle in src;src=src.replace(needle,insert+needle)
src=src.replace('rounds=0,draws=0)','rounds=0,draws=0,helper_called=0,promoted=0,selection_random=0,promotion_random=0,promotion_notification=0)')
src=src.replace("struct.pack('<6I',*pools);counts['accepted']+=1","struct.pack('<6I',*pools)+struct.pack('<12I',*extra);counts['helper_called']+=extra[4];counts['promoted']+=extra[9];counts['accepted']+=1")
exec(compile(src,'independent_promoted_body_wire_model','exec'),env);expected=env['expected_promoted']
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--reuse',type=Path);a=p.parse_args();D=a.out.resolve();D.mkdir(parents=True,exist_ok=False);S=Path(__file__).resolve().parent;flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);subprocess.run([a.cc,*flags,*[str(S/s)for s in SOURCES],'-o',str(D/'probe')],check=True);digest=hashlib.sha256();totals={};worlds=0
 cs=controls()
 for name,variant,text,data in cs:
  out=D/name;out.mkdir();(out/'rules.txt').write_bytes(text);(out/'world.sc').write_bytes(data)
  if a.reuse and(a.reuse/name/'expected.bin').is_file():
   assert(a.reuse/name/'rules.txt').read_bytes()==text and(a.reuse/name/'world.sc').read_bytes()==data
   want=(a.reuse/name/'expected.bin').read_bytes();counts=json.loads((a.reuse/name/'counts.json').read_text())
  else:want,counts=expected(text,data)
  (out/'expected.bin').write_bytes(want);(out/'counts.json').write_text(json.dumps(counts)+'\n');r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(D/'probe'),str(out/'rules.txt'),str(out/'world.sc')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(out/'actual.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert not r.returncode,(name,r.returncode,r.stderr[:1500]);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));assert r.stderr==f"MIDDLE_CLONE_GUARD {int(bool(counts['accepted']))}\n".encode(),(name,r.stderr)
  digest.update(r.stdout);worlds+=1
  for k,v in counts.items():totals[k]=totals.get(k,0)+v
 assert totals['promoted']and totals['attacker_deleted']and totals['defender_deleted']and totals['selection_random']and totals['promotion_notification'],totals
 receipt={'worlds':worlds,'counts':totals,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest(),'native':'UNRUN'};(D/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
