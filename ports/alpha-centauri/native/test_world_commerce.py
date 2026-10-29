#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original rank/trade rules with independently decoded wire facts."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_original_faction import SCHEMA
from test_units import sections
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','technology_effects.cpp','world_commerce.cpp','world_commerce_probe.cpp']
MIN,MAX=-(1<<31),(1<<31)-1
def checked(v):
 if not MIN<=v<=MAX:raise OverflowError
 return v
def div(a,b):return (1 if a>=0 else -1)*(abs(a)//b)
def fields(v):return {x['name']:x['offset'] for x in SCHEMA['Faction' if v>=11 else 'LegacyFactionV10']['fields']}
def rules(c):
 rows=[]
 for i in range(89):
  pre,obs=('Disable','None') if i==3 else ('None','Disable') if i==4 else ('T0','Disable') if i==5 else ('None','None')
  rows.append(f'Technology{i},T{i},0,0,0,0,{pre},{obs},{(4 if i<8 or i==88 else 0):09b}')
 return ('#TECHNOLOGY\n'+'\n'.join(rows)+'\n#END\n').encode()
def fixture(v,c):
 count=0 if c==0 else 512 if c==31 else [1,4,8,16,32][c%5]
 b=bytearray(valid_fixture(v,0 if c==0 else 3,count));_,off,_=decode(b);pf=off['preferences'] if v>=11 else off['legacy'];shift=0 if v>=11 else 0x7c;schema=fields(v);fs=0x20cc if v>=11 else 0x20bc;r=random.Random(v*100+c)
 struct.pack_into('<i',b,pf+0x184-shift,[-1,0,1,2,7,8,MAX,MIN][c%8]);struct.pack_into('<ii',b,pf+0x194-shift,c%3,c%2)
 b[pf+0x1e0-shift:pf+0x1e0-shift+89]=bytes([r.randrange(256) for _ in range(89)])
 for i in range(count):
  p=off['bases']+i*(308 if v>=11 else 304);b[p+4]=(i%4+1) if c%7 else i%8
  energy=r.choice([-100,-1,0,10,10,10,100,4096])
  if c in [26,27]:energy=[MIN,MAX][c%2] if i==0 else 10
  struct.pack_into('<i',b,p+(0xb8 if v>=11 else 0xb4),energy)
 for i in range(8):
  p=off['factions']+i*fs;d=off['faction_definitions']+i*0x59c
  for name,value in [('base_count',0 if (c+i)%9==0 else -1 if (c+i)%11==0 else 99),('tech_commerce_bonus',MAX if c==28 and i==2 else MIN if c==29 and i==1 else (c+i)%8-3),('SE_economy_pending',(c+i)%9-3),('sanction_turns',0 if (c+i)%5 else -1)]:struct.pack_into('<i',b,p+schema[name],value)
  for j in range(8):struct.pack_into('<I',b,p+schema['diplo_status']+j*4,r.randrange(4))
  struct.pack_into('<I',b,d+0x508,0x80 if c%3 and i%3==0 else 0);struct.pack_into('<i',b,d+0x504,i-2);struct.pack_into('<i',b,d+0x50c,0)
 return bytes(b)
def model(text,data):
 info,off,spans=decode(data);v=int(info['version']);schema=fields(v);fs=0x20cc if v>=11 else 0x20bc;pref=spans['preferences'] if v>=11 else spans['legacy'];shift=0 if v>=11 else 0x7c
 governor=struct.unpack_from('<i',pref,0x184-shift)[0];a,b=struct.unpack_from('<ii',pref,0x194-shift);trade=a>b;owners=pref[0x1e0-shift:][:89]
 tech=sections(text)['TECHNOLOGY'];keys={r[1].lower():i for i,r in enumerate(tech)};keys.update(none=-1,disable=-2);ts=[(int(r[8],2),keys[r[6].lower()],keys[r[7].lower()]) for r in tech];totaltech=sum(bool(flag&4) and pre>=-1 and obs>=-1 for flag,pre,obs in ts)
 factions=[]
 for i in range(8):
  p=i*fs;d=i*0x59c;state=spans['factions'];definition=spans['faction_definitions'];read=lambda name:struct.unpack_from('<i',state,p+schema[name])[0]
  f=dict(count=read('base_count'),saved=read('tech_commerce_bonus'),sanction=read('sanction_turns'),diplo=struct.unpack_from('<8I',state,p+schema['diplo_status']),alien=bool(struct.unpack_from('<I',definition,d+0x508)[0]&128));bonus=struct.unpack_from('<i',definition,d+0x504)[0];econ=read('SE_economy_pending')
  try:
   for t,(flag,pre,obs) in enumerate(ts[:88]):
    if i and pre!=-2 and (obs!=-2 or pre==-1) and owners[t]&(1<<i) and flag&4:bonus=checked(bonus+1)
   bonus=checked(bonus+(3 if econ>4 else 2 if econ>3 else 1 if econ>2 else 0));f['fresh']=bonus
  except OverflowError:f['fresh']=None
  # FUNGUS bonuses can overflow even for a commerce-only consumer; use actual
  # existing recomputation model for all fields, including unused increments.
  count=struct.unpack_from('<i',definition,d+0x50c)[0]
  from test_technology_effects import model as effects
  vals=[]
  for k in range(8):vals += [struct.unpack_from('<i',definition,d+0x510+4*k)[0],struct.unpack_from('<i',definition,d+0x530+4*k)[0]]
  context=[i,struct.unpack_from('<i',definition,d+0x504)[0],econ,count,*vals,*sum(([fl,pre,obs,owners[t]] for t,(fl,pre,obs) in enumerate(ts)),[])]
  error,answer=effects(context);f['fresh']=None if error else answer[0];factions.append(f)
 bases=[]
 for i in range(int(info['bases'])):
  p=i*(308 if v>=11 else 304);bases.append((spans['bases'][p+4],struct.unpack_from('<i',spans['bases'],p+(0xb8 if v>=11 else 0xb4))[0]))
 result=bytearray();success=overflow=0
 for selected,(own,energy) in enumerate(bases[:8]):
  for mode in ['saved','fresh']:
   try:
    if mode=='fresh' and any(f['fresh'] is None for f in factions):raise OverflowError
    score=lambda i:checked(checked(bases[i][1]*512)+i)
    ownscore=score(selected);rank=sum(score(i)>ownscore for i,x in enumerate(bases) if x[0]==own);partners=[-1]*7;imports=[0]*7;exports=[0]*7;qualified=0;total=0
    for other in range(1,8):
     me,them=factions[own],factions[other];slot=other-1
     if other==own or not ((me['diplo'][other]|them['diplo'][own])&2) or not them['count']:continue
     ordered=sorted([(score(i),i) for i,x in enumerate(bases) if x[0]==other],reverse=True)
     if rank>=len(ordered):continue
     partner=partners[slot]=ordered[rank][1];qualified=1;value=div(checked(energy+bases[partner][1]+7),8)
     if trade:value=checked(value*2)
     income=lambda f:div(checked(checked(checked(f[mode]+1)*value)+(totaltech+1)//2),totaltech+1)
     exp=income(them);imp=income(me)
     if not me['diplo'][other]&1:imp=div(imp,2);exp=div(exp,2)
     if governor==own:imp=checked(imp+1)
     if governor==other:exp=checked(exp+1)
     if me['sanction'] or them['sanction'] or (v>=11 and (me['alien'] or them['alien'])):imp=exp=0
     imports[slot]=imp;exports[slot]=exp;total=checked(total+imp)
    values=[0,int(v>=11),selected,rank,totaltech,*partners,*imports,*exports,total,qualified];success+=1
   except OverflowError:values=[2]+[0x5a5a5a5a]*27;overflow+=1
   result+=struct.pack('<28i',*values)
 guards=3+(26 if len(bases) and int(info['vehicles']) else 24 if len(bases) else 0)
 result+=struct.pack('<I',guards);return bytes(result),success,overflow,guards

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--rules-data',nargs=2,action='append',default=[]);ap.add_argument('--original',nargs=3,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
 exe=out/'commerce-probe';cmd=[a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True)
 corpus=[(rules(c),fixture(v,c),'synthetic') for v in range(7,13) for c in range(32)]
 for profile,path in a.rules_data:corpus += [(Path(path).read_bytes(),fixture(7 if profile=='base' else 12,c),'owner-rules') for c in range(32)]
 for profile,rulespath,save in a.original:corpus.append((Path(rulespath).read_bytes(),Path(save).read_bytes(),'owner-world'))
 digest=hashlib.sha256();success=overflow=guards=0;records=[]
 for i,(text,data,kind) in enumerate(corpus):
  rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);wanted,ok,over,guard=model(text,data)
  r=subprocess.run([*runner,str(exe),str(rp),str(sp)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30)
  assert r.returncode==0 and r.stdout==wanted,(i,r.returncode,r.stderr,len(r.stdout),len(wanted),next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert sp.read_bytes()==data and rp.read_bytes()==text
  digest.update(r.stdout);success+=ok;overflow+=over;guards+=guard;records.append(dict(kind=kind,sha256=hashlib.sha256(r.stdout).hexdigest()))
  if i==191:public=digest.hexdigest()
 receipt=dict(worlds=len(corpus),accepted=success,overflow=overflow,invalidAndAliasGuards=guards,publicOutputSHA256=public,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),records=records,command=cmd)
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
