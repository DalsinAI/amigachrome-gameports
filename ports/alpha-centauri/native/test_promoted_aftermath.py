# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import json,struct,hashlib,subprocess,os,argparse
import test_promoted_land_body as parent
from test_original_game import decode
from test_battle_aftermath import book_model
U=0xffffffff
SOURCES=[s for s in parent.SOURCES if s!='promoted_land_body_probe.cpp']+['battle_bookkeeping.cpp','world_battle_bookkeeping.cpp','promoted_aftermath.cpp','promoted_aftermath_probe.cpp']
def controls():
 for name,variant,text,data in parent.controls():
  yield name,variant,text,data
  if name.endswith('-attacker-strong'):
   b=bytearray(data);info,o,s=decode(b);fs=len(s['factions'])//8
   for f in range(8):
    struct.pack_into('<i',b,o['factions']+f*fs+0x288,-4);at=o['faction_definitions']+f*0x59c+0x508;flags=struct.unpack_from('<I',b,at)[0];struct.pack_into('<I',b,at,flags&~0x8000);struct.pack_into('<I',b,o['monuments']+f*0x4f4+0x160,0)
   yield name+'-quiet-morale',variant,text,bytes(b)
def morale_for(sp,id,rank):
 fs=len(sp['factions'])//8;v=sp['vehicles'][id*52:(id+1)*52];fac=v[14];social=struct.unpack_from('<i',sp['factions'],fac*fs+0x288)[0];rule=struct.unpack_from('<i',sp['faction_definitions'],fac*0x59c+0x4d8)[0];m=max(-4,min(4,social));m+=int(m<=-2)-int(m>=2);m+=rule
 if struct.unpack_from('<I',sp['faction_definitions'],fac*0x59c+0x508)[0]&0x8000:m=max(0,m)
 return max(0,min(6,rank+m))
env=dict(parent.env);env['morale_for']=morale_for
src=parent.src.replace('pools=[int(profile==0),0x7fffffff,1,0x7fffffff,int(profile==0),0xfffffffb]','pools=[1,0,1,0,1,0]').replace('eff=min(rank,6)','eff=morale_for(sp,winner,rank)').replace('min(rank+1,6)!=eff','morale_for(sp,winner,rank+1)!=eff')
exec(compile(src,'independent_promoted_aftermath_body','exec'),env);body_model=env['expected_promoted']
def expected(text,data):
 info,o,s=decode(data);n=int(info['vehicles']);modern=int(info['version'])>=11;fs=len(s['factions'])//8;nt=len(s['tiles'])//44;raw,_=body_model(text,data);pos=0;out=bytearray();counts=dict(accepted=0,rejected=0,attacker_deleted=0,defender_deleted=0,rounds=0,draws=0,helper_called=0,promoted=0,selection_random=0,promotion_random=0,promotion_notification=0,held_monument_ui=0)
 flags=[struct.unpack_from('<I',s['faction_definitions'],f*0x59c+0x508)[0]for f in range(8)];state=[(0x7fffffff+f)&U for f in range(8)]+[U-f for f in range(8)]
 for offset in [0x18c,0x1ac]:state+=[struct.unpack_from('<I',s['factions'],f*fs+offset+j*4)[0]for f in range(8)for j in range(8)]
 for f in range(8):state+=[struct.unpack_from('<I',s['monuments'],f*0x4f4+offset)[0]for offset in [0x150,0x158,0x15c,0x160,0x164,0x498]]
 turn=struct.unpack_from('<I',s['preferences']if modern else s['legacy'],0x44)[0]
 for profile in range(2):
  for a in range(n):
   for b in range(n):
    begin=pos;row=list(struct.unpack_from('<21I',raw,pos));pos+=84;detail=promotion=None
    if row[0]==0:
     detail=struct.unpack_from('<9I',raw,pos);pos+=36+detail[7]*52+nt*44+8*(512+1024+12)+32+24;promotion=struct.unpack_from('<12I',raw,pos);pos+=48
    code=row[0]
    if not code:
     af=s['vehicles'][a*52+14];bf=s['vehicles'][b*52+14];ctx=[int(modern),af,bf,0 if detail[0]==1 else 1,af if profile else U,0,0,1,turn,0x7fffffff]+flags;book=book_model(ctx+state);code=0 if book[0]==0 else 6 if book[0]==4 else 1
    if code:
     out+=struct.pack('<21I',code,*([0]*20));counts['rejected']+=1
     if code in[3,4,5,6]:counts[{3:'selection_random',4:'promotion_random',5:'promotion_notification',6:'held_monument_ui'}[code]]+=1
     continue
    assert detail and promotion
    out+=raw[begin:pos]+struct.pack('<387I',*book[1:]);after=book[193:385];mon=bytearray(s['monuments'])
    for f in range(8):
     for offset,v in zip([0x150,0x158,0x15c,0x160,0x164,0x498],after[144+f*6:150+f*6]):struct.pack_into('<I',mon,f*0x4f4+offset,v)
    out+=mon;counts['accepted']+=1;counts['attacker_deleted'if detail[0]==1 else'defender_deleted']+=1;counts['rounds']+=row[8];counts['draws']+=row[9];counts['helper_called']+=promotion[4];counts['promoted']+=promotion[9]
 assert pos==len(raw)
 return bytes(out),counts
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--reuse',type=Path);a=p.parse_args();D=a.out.resolve();D.mkdir(parents=True,exist_ok=False);S=Path(__file__).resolve().parent;flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);subprocess.run([a.cc,*flags,*[str(S/s)for s in SOURCES],'-o',str(D/'probe')],check=True);digest=hashlib.sha256();totals={};worlds=0
 for name,variant,text,data in controls():
  d=D/name;d.mkdir();(d/'rules.txt').write_bytes(text);(d/'world.sc').write_bytes(data)
  if a.reuse and(a.reuse/name/'expected.bin').is_file():
   assert(a.reuse/name/'rules.txt').read_bytes()==text and(a.reuse/name/'world.sc').read_bytes()==data;want=(a.reuse/name/'expected.bin').read_bytes();counts=json.loads((a.reuse/name/'counts.json').read_text())
  else:want,counts=expected(text,data)
  (d/'expected.bin').write_bytes(want);(d/'counts.json').write_text(json.dumps(counts)+'\n');r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(D/'probe'),str(d/'rules.txt'),str(d/'world.sc')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(d/'actual.bin').write_bytes(r.stdout);(d/'stderr').write_bytes(r.stderr);assert not r.returncode,(name,r.returncode,r.stderr[:1500]);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));assert r.stderr==f"COMPOSITION_GUARD {int(bool(counts['accepted']))}\nRUNTIME_GUARDS {8*int(bool(counts['accepted']))}\n".encode(),(name,r.stderr)
  digest.update(r.stdout);worlds+=1
  for k,v in counts.items():totals[k]=totals.get(k,0)+v
 assert totals['attacker_deleted']and totals['defender_deleted']and totals['promoted']and totals['held_monument_ui']and totals['promotion_notification']and totals['selection_random'],totals
 receipt={'worlds':worlds,'counts':totals,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest(),'native':'UNRUN'};(D/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
