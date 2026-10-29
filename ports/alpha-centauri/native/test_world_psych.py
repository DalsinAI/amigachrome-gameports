#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Current world psych binding vs independent wire extraction and phase model."""
import argparse,hashlib,json,math,os,random,struct,subprocess
from pathlib import Path
import test_world_police as police
import test_world_pacifism as pacifism
from test_original_game import decode
from test_original_faction import SCHEMA
from test_world_unit_stats import scalar
from test_base_psych import expected
HERE=Path(__file__).resolve().parent
SOURCES=police.SOURCES[:-1]+['world_pacifism.cpp','base_psych.cpp','world_psych_internal.cpp','world_psych.cpp','world_psych_probe.cpp']
def rules(profile,c):
 t=police.sections(police.rules(profile,c));f=json.loads((HERE/'basic_fixture.json').read_text());t['RULES']=f['rules_rows'][:-2] if profile=='base' else f['rules_rows'];t['WORLDBUILDER']=f['world_rows'];t['RULES'][11][0]=str(c%101);return police.material(t)
def fixture(v,c):
 b=bytearray(police.fixture(v,c));info,off,_=decode(b);r=random.Random(v*1000+c);fs=0x20cc if v>=11 else 0x20bc;schema=SCHEMA['Faction' if v>=11 else 'LegacyFactionV10'];bs=308 if v>=11 else 304;pf=off['preferences'] if v>=11 else off['legacy']
 b[pf+0x58]=r.randrange(256)
 for f in range(8):
  p=off['factions']+f*fs
  def put(name,value,index=0):
   field=next(x for x in schema['fields'] if x['name']==name);struct.pack_into('<i',b,p+field['offset']+4*index,value)
  for name,value in [('diff_level',r.randrange(6)),('base_count',r.randrange(1,100)),('SE_effic_pending',r.randrange(-5,7)),('SE_talent_pending',r.randrange(-4,5)),('SE_police_pending',c%11-6),('SE_Economics_pending',c%4)]:put(name,value)
  for i in range(80):put('saved_queue_items',i*11,index=i)
  for i in range(8):put('social_support',i*13,index=i)
  for i in range(72):put('social_psych',r.randrange(-100,101),index=i)
  d=off['faction_definitions']+f*0x59c;struct.pack_into('<ii',b,d+0x4e0,c%6,c%7)
  if c not in [24,25,26,27]:
   struct.pack_into('<i',b,d+0x50c,2);struct.pack_into('<i',b,d+0x510,14);struct.pack_into('<i',b,d+0x514,14);struct.pack_into('<i',b,d+0x530,c%9-4);struct.pack_into('<i',b,d+0x534,100)
  struct.pack_into('<i',b,d+0x508,0x40 if c%2 else 0)
 for i in range(int(info['bases'])):
  p=off['bases']+i*bs;pop=[0,1,3,6,12,25,127][(c+i)%7];b[p+6]=pop;b[p+7]=[0,1,10,11,255][c%5];b[p+8]=int(c%9==0);struct.pack_into('<i',b,p+0x7c,r.randrange(pop+1));struct.pack_into('<II',b,p+0x8c,r.getrandbits(32),r.getrandbits(4))
  if c==28:struct.pack_into('<i',b,p+0x7c,pop+1)
  if c==29:
   f=b[p+4];field=next(x for x in schema['fields'] if x['name']=='SE_effic_pending');struct.pack_into('<i',b,off['factions']+f*fs+field['offset'],0x7fffffff)
 for i in range(33 if v<11 else 64):struct.pack_into('<i',b,pf+0x84+4*i,r.choice([-1,0,1]) if int(info['bases']) else -1)
 return bytes(b)
def model(text,data,profile,fact_output=None):
 info,_,sp=decode(data);v=int(info['version']);bases=int(info['bases']);bs=308 if v>=11 else 304;fs=0x20cc if v>=11 else 0x20bc;schema=SCHEMA['Faction' if v>=11 else 'LegacyFactionV10'];pref=sp['preferences'] if v>=11 else sp['legacy'];projects=struct.unpack_from('<'+('64i' if v>=11 else '33i'),pref,0x84)
 po,*_=police.model(text,data,profile);pos=512*8;pa,*_=pacifism.model(text,data,profile);result=bytearray();ok=bad=overflow=0
 for base in range(min(8,bases)):
  ge=struct.unpack_from('<I',po,pos)[0];pos+=4;units=[];count=0
  if ge==0:
   _,_,count=struct.unpack_from('<3I',po,pos);pos+=12
   for i in range(count):units+=list(struct.unpack_from('<6i',po,pos)[1:]);pos+=24
  pe,_,_,pac,*_=struct.unpack_from('<8i',pa,base*32)
  b=sp['bases'][base*bs:(base+1)*bs];owner=b[4];f=sp['factions'][owner*fs:(owner+1)*fs];d=sp['faction_definitions'][owner*0x59c:(owner+1)*0x59c];get=lambda name,index=0:scalar(f,schema,name,index);dg=lambda name,index=0:scalar(d,SCHEMA['MFaction'],name,index)
  hasno=nodrone=0;valid=True
  if v>=11:
   nb=dg('faction_bonus_count');valid=0<=nb<=8
   if valid:
    for i in range(nb):
     if dg('faction_bonus_id',i)==14:hasno=1;nodrone=dg('faction_bonus_val1',i);break
  mask=0
  for i in [0,6,7,14,25,26]:
   at=projects[i]
   if at>=bases:valid=False
   elif at>=0 and sp['bases'][at*bs+4]==owner:mask|=1<<i
  social=[get('saved_queue_items',78),get('saved_queue_items',79)]+[get('social_support',i) for i in range(8)]+[get('social_psych',i) for i in range(72)]
  factor=math.isqrt(int(info['width'])//2*int(info['height']))
  for stage in range(2):
   if ge or pe or not valid:
    e=1;vals=[]
    if fact_output is not None:fact_output.append(None)
   else:
    row=[int(v>=11),int(bool(pref[0x58]&(1<<owner))),stage,struct.unpack_from('<b',b,6)[0],struct.unpack_from('<i',b,0x7c)[0],b[7],get('diff_level'),get('SE_effic_pending'),factor,base,get('base_count'),dg('rule_drone'),dg('rule_talent'),hasno,nodrone,get('SE_talent_pending'),get('SE_police_pending'),pac,base*13+stage*7,max(0,min(100,int(police.sections(text)['RULES'][11][0]))),get('SE_Economics_pending'),b[8],*struct.unpack_from('<2I',b,0x8c),mask,int(bool(dg('rule_flags')&0x40)),count,*social,*units]

    if fact_output is not None:fact_output.append(row)
    calculated=expected(row);e=calculated[0];vals=[int(v>=11),base,factor,pac,count,*calculated[1:]]
   if e:result+=struct.pack('<106I',e,*([0x5a5a5a5a]*105));bad+=e==1;overflow+=e==3
   else:result+=struct.pack('<106I',0,*[n&0xffffffff for n in vals]);ok+=1
 guards=3+(14 if bases and int(info['vehicles']) else 13 if bases else 0);result+=struct.pack('<I',guards);return bytes(result),ok,bad,overflow,guards

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--rules-data',nargs=2,action='append',default=[]);ap.add_argument('--original',nargs=3,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
 exe=out/'world-psych';cmd=[a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True)
 corpus=[(rules('base' if v<11 else 'expansion',c),fixture(v,c),'base' if v<11 else 'expansion','synthetic') for v in range(7,13) for c in range(32)]
 for profile,path in a.rules_data:corpus += [(Path(path).read_bytes(),fixture(7 if profile=='base' else 12,c),profile,'owner-rules') for c in range(32)]
 for profile,rulespath,save in a.original:corpus.append((Path(rulespath).read_bytes(),Path(save).read_bytes(),profile,'owner-world'))
 digest=hashlib.sha256();good=bad=overflow=guards=0;records=[]
 for i,(text,data,profile,kind) in enumerate(corpus):
  rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);want,g,b,o,gd=model(text,data,profile);r=subprocess.run([*runner,str(exe),profile,str(rp),str(sp)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60)
  (out/f'{i}.out').write_bytes(r.stdout);(out/f'{i}.expected').write_bytes(want);(out/f'{i}.stderr').write_bytes(r.stderr)
  assert r.returncode==0 and r.stdout==want,(i,r.returncode,r.stderr,len(r.stdout),len(want),next((j for j,(x,y) in enumerate(zip(r.stdout,want)) if x!=y),None));assert sp.read_bytes()==data and rp.read_bytes()==text
  digest.update(r.stdout);good+=g;bad+=b;overflow+=o;guards+=gd;records.append(dict(profile=profile,kind=kind,sha256=hashlib.sha256(r.stdout).hexdigest()))
  if i==191:public=digest.hexdigest()
 receipt=dict(worlds=len(corpus),accepted=good,rejected=bad,overflow=overflow,guards=guards,publicOutputSHA256=public,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),records=records,command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
