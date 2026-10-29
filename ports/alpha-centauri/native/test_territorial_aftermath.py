# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,subprocess,struct,json,hashlib,os
import test_promoted_aftermath as parent
from test_original_game import decode
SOURCES=[s for s in parent.SOURCES if s!='promoted_aftermath_probe.cpp']+['territorial_combat.cpp','world_territorial_combat.cpp','territorial_promoted_body.cpp','territorial_aftermath.cpp','territorial_aftermath_probe.cpp']
U=0xffffffff
def signed(v):return((v+0x80000000)&U)-0x80000000
def controls():
 for name,variant,text,data in parent.controls():
  raw=bytearray(data);info,o,sp=decode(raw);fs=len(sp['factions'])//8
  for i in range(len(sp['tiles'])//44):raw[o['tiles']+i*44+7]=0
  for f in range(8):
   struct.pack_into('<I',raw,o['factions']+0x18+f*4,0x17);struct.pack_into('<i',raw,o['factions']+f*fs+0x12c,0);struct.pack_into('<i',raw,o['factions']+0x58+f*4,-2147483648 if f%2 else 1)
  yield name,variant,text,bytes(raw)
  if name.endswith('-attacker-strong'):
   for mode in range(3):
    b=bytearray(raw)
    if mode==0:
     for i in range(len(sp['tiles'])//44):b[o['tiles']+i*44+7]=255
    if mode==1:
     for f in range(8):struct.pack_into('<I',b,o['factions']+0x18+f*4,0)
    if mode==2:
     for f in range(8):struct.pack_into('<i',b,o['factions']+f*fs+0x12c,7)
    yield name+'-territory-'+str(mode),variant,text,bytes(b)
def expected(text,data):
 info,o,sp=decode(data);n=int(info['vehicles']);fs=len(sp['factions'])//8;nt=len(sp['tiles'])//44;w=int(info['width']);raw,_=parent.expected(text,data);pos=0;out=bytearray();counts=dict(accepted=0,rejected=0,attacker_deleted=0,defender_deleted=0,applied=0,selection_random=0,promotion_notification=0,monument_ui=0,unowned_land=0,same_faction=0)
 for profile in range(2):
  for a in range(n):
   for b in range(n):
    start=pos;row=struct.unpack_from('<21I',raw,pos);pos+=84;detail=None
    if row[0]==0:
     detail=struct.unpack_from('<9I',raw,pos);pos+=36+detail[7]*52+nt*44+8*(512+1024+12)+32+24+48+387*4+0x27a0
    vx,vy=struct.unpack_from('<hh',sp['vehicles'],b*52);idx=(vy*w+vx)//2;tile=sp['tiles'][idx*44:(idx+1)*44];owner=struct.unpack_from('<b',tile,7)[0];land=tile[0]>=96;held=land and not 0<=owner<8;same=sp['vehicles'][a*52+14]==sp['vehicles'][b*52+14];code=1 if (held or same)and row[0]in[0,3,4,5,6]else row[0]
    if code:
     out+=struct.pack('<21I',code,*([0]*20));counts['rejected']+=1
     if code in[3,5,6]:counts[{3:'selection_random',5:'promotion_notification',6:'monument_ui'}[code]]+=1
     if held and row[0]in[0,3,4,5,6]:counts['unowned_land']+=1
     if same and row[0]in[0,3,4,5,6]:counts['same_faction']+=1
     continue
    assert detail;af=sp['vehicles'][a*52+14];bf=sp['vehicles'][b*52+14]
    gifts=[[struct.unpack_from('<i',sp['factions'],f*fs+0x12c+j*4)[0]for j in range(8)]for f in range(8)];friction=[[struct.unpack_from('<i',sp['factions'],f*fs+0x58+j*4)[0]for j in range(8)]for f in range(8)];g=gifts[af][owner]if land else 0;r=friction[owner][af]if land else 0;sd=struct.unpack_from('<I',sp['factions'],owner*fs+0x18+bf*4)[0]if land else 0;sa=struct.unpack_from('<I',sp['factions'],owner*fs+0x18+af*4)[0]if land else 0;hit=land and owner not in[af,bf]and sd&16 and sa&7;ga=1 if hit and not g else g;ra=max(0,min(20,signed(r-1)))if hit else r;nv=0x7fffffff if profile else 0x80000000;na=(nv-1)&U if hit and not profile else nv
    if hit:gifts[af][owner]=ga;friction[owner][af]=ra;counts['applied']+=1
    words=[idx,owner,af,bf,int(bool(hit)),g,ga,r,ra,nv,na,U if profile else owner,U if profile else af,na]
    for f in range(8):words+=gifts[f]+friction[f]
    assert len(words)==142;out+=raw[start:pos]+struct.pack('<142I',*[v&U for v in words]);counts['accepted']+=1;counts['attacker_deleted'if detail[0]==1 else'defender_deleted']+=1
 assert pos==len(raw)
 return bytes(out),counts
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--reuse',type=Path);a=p.parse_args();D=a.out.resolve();D.mkdir(parents=True,exist_ok=False);S=Path(__file__).resolve().parent;flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);subprocess.run([a.cc,*flags,*[str(S/s)for s in SOURCES],'-o',str(D/'probe')],check=True);digest=hashlib.sha256();totals={};worlds=0
 for name,variant,text,data in controls():
  d=D/name;d.mkdir();(d/'rules.txt').write_bytes(text);(d/'world.sc').write_bytes(data)
  if a.reuse and(a.reuse/name/'expected.bin').is_file():
   prior=a.reuse/name;assert(prior/'rules.txt').read_bytes()==text and(prior/'world.sc').read_bytes()==data;want=(prior/'expected.bin').read_bytes();counts=json.loads((prior/'counts.json').read_text())
  else:want,counts=expected(text,data)
  (d/'expected.bin').write_bytes(want);(d/'counts.json').write_text(json.dumps(counts)+'\n');r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(D/'probe'),str(d/'rules.txt'),str(d/'world.sc')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(d/'actual.bin').write_bytes(r.stdout);(d/'stderr').write_bytes(r.stderr);assert r.returncode==0,(name,r.returncode,r.stderr[:1500]);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(a,b)in enumerate(zip(r.stdout,want))if a!=b),None));assert r.stderr==f"COMPOSITION_GUARD {int(bool(counts['accepted']))}\nRUNTIME_GUARDS {14*int(bool(counts['accepted']))}\n".encode(),(name,r.stderr);digest.update(r.stdout);worlds+=1
  for k,v in counts.items():totals[k]=totals.get(k,0)+v
 assert totals['applied']and totals['attacker_deleted']and totals['defender_deleted']and totals['unowned_land']
 j={'worlds':worlds,'counts':totals,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest(),'native':'UNRUN'};(D/'receipt.json').write_text(json.dumps(j,indent=2)+'\n');print(json.dumps(j))
if __name__=='__main__':main()
