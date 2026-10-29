#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,shlex,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_units import fixture,material
LABELS=['Scout','Recon','Foil','Artillery','Battery','Marine','Marines']
def resolve(primary,alternate,parity,names,prefix,token,limit):
 distinct=primary!=alternate;stopped=False;i=0;seen=set()
 def suffix(s):
  k=s.rfind(token)
  if k<0 or len(s)-k>len(token)+1:return s+' '+token+'2',False
  if '2'<=s[-1]<='8':return s[:-1]+chr(ord(s[-1])+1),False
  return s,True
 while i<512:
  state=(i,primary,alternate,parity,stopped)
  if state in seen:return 4,None
  seen.add(state)
  if i not in names:i+=1;continue
  if not stopped and names[i]==primary:
   if distinct:parity=1
   primary,stop=suffix(primary);stopped|=stop;i=-1
  if distinct and alternate==(prefix if i<0 else names[i]):
   parity=0;alternate,stop=suffix(alternate);stopped|=stop;i=-1
  if max(len(primary),len(alternate))>=256:return 2,None
  i+=1
 result=alternate if parity else primary
 return (2,None) if len(result)>=limit else (0,result)
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--logical',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
names=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype_prefix_context.cpp','automatic_prototype_name.cpp','base_suffix_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+(['-DALPHA_LOGICAL_SUFFIX'] if a.logical else [])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True);cases=[];combined=bytearray()
for mode in range(24):
 v=int(mode==16);variant='expansion' if v else 'base';version=12 if v else 7;t=fixture(variant,count=0);t['WEAPONS'][3][2]='8';t['WEAPONS'][3][3]='0';t['DEFENSES'][1][2]='1';rules=material(t);rp=out/(str(mode)+'.txt');rp.write_bytes(rules);labels=LABELS[:];token='MK';same=mode in [6,7,8,9,10,11,12,17,23];stem='Artillery'
 if mode==8:stem='Artillery MK2'
 if mode==9:stem='Artillery MK'
 if mode==10:stem='Artillery MKLong'
 if same:labels[3]=labels[4]=stem
 if mode==11:token='X'
 if mode==12:token='2'
 primary='W3 '+labels[3];alternate='W3 '+labels[4];selected=[]
 if mode==1:selected=[primary]
 if mode==2:selected=[alternate]
 if mode==3:selected=[primary,alternate]
 if mode==4:selected=[primary,alternate,primary+' MK2']
 if mode==5:selected=[primary,alternate]+[primary+' MK'+str(i) for i in range(2,9)]+[alternate+' MK'+str(i) for i in range(2,9)]
 if mode in [6,7]:selected=[primary]+[primary+' MK'+str(i) for i in range(2,9+(mode==7))]
 if mode in [8,9,10]:selected=[primary]
 if mode==11:selected=[primary]+[primary+' X'+str(i) for i in range(2,9)]
 if mode==12:selected=[primary]+[primary+' 2'+str(i) for i in range(2,10)]
 if mode==13:selected=[alternate]+[alternate+' MK'+str(i) for i in range(2,10)]
 if mode==23:selected=[primary]
 if mode==17:selected=[primary];token='Y'*127
 if mode in [19,20,21,22]:selected=[primary]
 data=bytearray(valid_fixture(version,3,2));_,off,spans=decode(data);prefs=off['preferences'] if v else off['legacy'];data[prefs+0x58]=2;candidate=bytearray(52);struct.pack_into('<I',candidate,32,8);rec=2 if same and mode!=23 else 1;candidate[36:40]=bytes([0,3,1,rec]);candidate[43]=1;struct.pack_into('<Hh',candidate,48,1,-1);id=64
 for i in range(512):
  p=off['prototypes']+52*i;word=('unused'+str(i)).encode();data[p:p+32]=word+bytes(32-len(word));struct.pack_into('<H',data,p+48,0)
 lookup={}
 for i,name in enumerate(selected):
  p=off['prototypes']+52*i;b=name.encode();data[p:p+32]=b+bytes(32-len(b));lookup[i]=name
 if mode==19:
  # Matching component tuple is excluded even if its name collides.
  p=off['prototypes'];data[p+32:p+40]=candidate[32:40];lookup={}
 if mode==20:
  # Move the sole collision into another faction's design range.
  p=off['prototypes']+128*52;b=primary.encode();data[p:p+32]=b+bytes(32-len(b));p=off['prototypes'];b=b'unused0';data[p:p+32]=b+bytes(32-len(b));lookup={}
 if mode==21:
  # The selected previous design is excluded; no fake retirement/naming step.
  p=off['prototypes']+64*52;b=primary.encode();data[p:p+32]=b+bytes(32-len(b));p=off['prototypes'];b=b'unused0';data[p:p+32]=b+bytes(32-len(b));lookup={}
 if mode==22:
  # Inactive same-faction designs still reserve names.
  p=off['prototypes']+65*52;b=primary.encode();data[p:p+32]=b+bytes(32-len(b));p=off['prototypes'];b=b'unused0';data[p:p+32]=b+bytes(32-len(b));lookup={65:primary}
 prefix=bytes(data[off['bunglist']+4044:off['bunglist']+4096])+bytes(data[off['prototypes']:off['prototypes']+32]);prefix=prefix.split(b'\0')[0].decode('latin1');error,result=resolve(primary,alternate,(rec-1)^1,lookup,prefix,token,256 if a.logical else 32)
 if mode in [14,15,18]:error=1;result=None
 if mode==16:error=3;result=None
 if mode==18:struct.pack_into('<H',candidate,48,0)
 suffix=token.encode()+bytes(128-len(token))
 if mode==14:suffix=bytes(128)
 if mode==15:suffix=b'X'*128
 facts=candidate+struct.pack('<I',id)+b''.join(s.encode()+bytes(128-len(s)) for s in labels)+suffix;scene=out/(str(mode)+'.SC');scene.write_bytes(data);qp=out/(str(mode)+'.facts');qp.write_bytes(facts)
 r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(scene),str(rp),str(qp),str(v)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);expected=struct.pack('<I',error)+(result.encode()+bytes((256 if a.logical else 32)-len(result)) if not error else b'');assert r.returncode==0,(mode,r.returncode,r.stderr);assert r.stdout==expected,(mode,r.stdout,expected,result);combined+=r.stdout;cases.append(dict(mode=mode,variant=v,error=error,result=result,primary=primary,alternate=alternate,token=token))
(out/'outputs.bin').write_bytes(combined);rec=dict(cases=cases,logical=a.logical,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n');print(len(cases),rec['outputSHA256'])
