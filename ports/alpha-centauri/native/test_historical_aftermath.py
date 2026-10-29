# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,subprocess,struct,json,hashlib,os
import test_territorial_aftermath as parent
from test_original_game import decode
SOURCES=[s for s in parent.SOURCES if s!='territorial_aftermath_probe.cpp']+['world_combat_history.cpp','historical_aftermath.cpp','historical_aftermath_probe.cpp']
def controls():
 for name,variant,text,data in parent.controls():
  yield name,variant,text,data
  if name.endswith('-attacker-strong'):
   b=bytearray(data);info,o,sp=decode(b);uid=struct.unpack_from('<h',sp['vehicles'],10)[0];struct.pack_into('<h',b,o['vehicles']+52+10,uid);yield name+'-shared-design',variant,text,bytes(b)
def expected(data,raw):
 info,o,sp=decode(data);n=int(info['vehicles']);nt=len(sp['tiles'])//44;pos=0;out=bytearray();shared=0;changed=0;accepted=0
 for mode in range(2):
  for a in range(n):
   for b in range(n):
    start=pos;row=struct.unpack_from('<21I',raw,pos);pos+=84
    if row[0]:out+=raw[start:pos];continue
    detail=struct.unpack_from('<9I',raw,pos);pos+=36+52*detail[7]+44*nt+8*(512+1024+12)+32+24+48+387*4+0x27a0+142*4
    ad=struct.unpack_from('<h',sp['vehicles'],a*52+10)[0];dd=struct.unpack_from('<h',sp['vehicles'],b*52+10)[0];af=sp['vehicles'][a*52+14];df=sp['vehicles'][b*52+14];flags=bytearray(sp['prototypes'][j*52+45]for j in range(512));av=flags[ad];dv=flags[dd];flags[ad]|=1<<df;flags[dd]|=1<<af;summary=[ad,dd,af,df,av,dv,flags[ad],flags[dd],int(ad==dd)];out+=raw[start:pos]+struct.pack('<9I',*summary)+flags;accepted+=1;shared+=ad==dd;changed+=flags[ad]!=av or flags[dd]!=dv
 assert pos==len(raw)
 return bytes(out),dict(history_accepted=accepted,shared_design_accepted=shared,changed_history=changed)
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--reuse',type=Path);p.add_argument('--parent-corpus',type=Path);a=p.parse_args();D=a.out.resolve();D.mkdir(parents=True,exist_ok=False);S=Path(__file__).resolve().parent;flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);subprocess.run([a.cc,*flags,*[str(S/s)for s in SOURCES],'-o',str(D/'probe')],check=True);digest=hashlib.sha256();totals={};worlds=0
 for name,variant,text,data in controls():
  d=D/name;d.mkdir();(d/'rules.txt').write_bytes(text);(d/'world.sc').write_bytes(data)
  if a.reuse and(a.reuse/name/'expected.bin').is_file():
   prior=a.reuse/name;assert(prior/'rules.txt').read_bytes()==text and(prior/'world.sc').read_bytes()==data;want=(prior/'expected.bin').read_bytes();counts=json.loads((prior/'counts.json').read_text())
  else:
   if a.parent_corpus and(a.parent_corpus/name/'expected.bin').is_file():
    prior=a.parent_corpus/name;assert(prior/'rules.txt').read_bytes()==text and(prior/'world.sc').read_bytes()==data;raw=(prior/'expected.bin').read_bytes();counts=json.loads((prior/'counts.json').read_text())
   else:raw,counts=parent.expected(text,data)
   want,extra=expected(data,raw);counts={**counts,**extra}
  (d/'expected.bin').write_bytes(want);(d/'counts.json').write_text(json.dumps(counts)+'\n');r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(D/'probe'),str(d/'rules.txt'),str(d/'world.sc')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(d/'actual.bin').write_bytes(r.stdout);(d/'stderr').write_bytes(r.stderr);assert r.returncode==0,(name,r.returncode,r.stderr[:1500]);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(a,b)in enumerate(zip(r.stdout,want))if a!=b),None));assert r.stderr==f"COMPOSITION_GUARD {int(bool(counts['accepted']))}\nRUNTIME_GUARDS {14*int(bool(counts['accepted']))}\n".encode();digest.update(r.stdout);worlds+=1
  for k,v in counts.items():totals[k]=totals.get(k,0)+v
 assert totals['shared_design_accepted']and totals['changed_history']and totals['attacker_deleted']and totals['defender_deleted'];j={'worlds':worlds,'counts':totals,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest(),'native':'UNRUN'};(D/'receipt.json').write_text(json.dumps(j,indent=2)+'\n');print(json.dumps(j))
if __name__=='__main__':main()
