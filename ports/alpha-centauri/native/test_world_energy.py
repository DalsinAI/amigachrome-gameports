#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh world income through happiness, independent wire/component models."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_world_fresh_minerals as minerals
import test_world_psych as population
import test_world_commerce as commerce
import test_base_energy as energy
from world_energy_reference import resource_model
from test_citizen_definitions import tables as citizen_tables,reference as citizen_reference
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','citizens_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','world_unit_support.cpp','vehicle_stack.cpp','tile_yields.cpp','world_tile_yields.cpp','technology_effects.cpp','world_fresh_tile_yields.cpp','world_worker_radius.cpp','world_terraform_ecology.cpp','world_stable_intake.cpp','base_nutrients.cpp','world_fresh_nutrients.cpp','world_convoy_nutrients.cpp','base_minerals.cpp','world_fresh_minerals.cpp','world_commerce.cpp','world_citizen_energy.cpp','world_police.cpp','world_pacifism.cpp','base_psych.cpp','world_psych_internal.cpp','energy_inefficiency.cpp','base_energy.cpp','world_energy.cpp','world_energy_probe.cpp']
def rules(profile):
 text=minerals.convoy.nutrients.rules(profile);_,rows=citizen_tables();return text+b'\n#CITIZENS\n'+('\n'.join(','.join(r) for r in rows)+'\n#END\n').encode()
def fixture(v,c):
 profile='base' if v<11 else 'expansion';b=bytearray(minerals.fixture(profile,v,c));info,o,s=population.decode(b);fs=0x20bc if v<11 else 0x20cc;bs=304 if v<11 else 308;schema=population.SCHEMA['LegacyFactionV10' if v<11 else 'Faction'];pf=o['legacy'] if v<11 else o['preferences'];shift=0x7c if v<11 else 0
 def put(f,name,value,index=0):
  field=next(x for x in schema['fields'] if x['name']==name);struct.pack_into('<i',b,o['factions']+fs*f+field['offset']+4*index,value)
 for f in range(8):
  for name,val in [('base_count',1 if f in [1,2] else 0),('diff_level',c%6),('SE_effic_pending',c%10-5),('SE_talent_pending',c%7-3),('SE_police_pending',c%11-6),('SE_Economics_pending',c%4),('SE_economy_pending',c%8-2),('SE_alloc_psych',c%4),('SE_alloc_labs',(c//4)%(11-c%4)),('energy_surplus_total',c*31),('sanction_turns',int((c+f)%13==0)),('tech_commerce_bonus',0x7fffffff)]:put(f,name,val)
  for i in range(9):put(f,'social_effic',i*13)
  for i in range(72):put(f,'social_psych',i*17-50,index=i)
  for i in range(8):put(f,'social_support',i*19,index=i)
  for i in range(8):put(f,'diplo_status',0 if c%5==0 else (2 if c%3 else 3) if (i+f)%2 else 0,index=i)
  d=o['faction_definitions']+f*0x59c
  for off,val in [(0x4e0,c%5),(0x4e4,c%6),(0x4dc,c%7-3),(0x504,0),(0x508,(0x80 if v>=11 and c%5==0 and f==1 else 0)|0x40),(0x50c,0)]:struct.pack_into('<i',b,d+off,val)
 for i in range(int(info['bases'])):
  p=o['bases']+bs*i;struct.pack_into('<i',b,p+(0xb4 if v<11 else 0xb8),c*3+11*i);struct.pack_into('<i',b,p+(0xc4 if v<11 else 0xc8),0x7fffffff);struct.pack_into('<i',b,p+(0xd0 if v<11 else 0xd4),0x7fffffff);struct.pack_into('<i',b,p+(0xd4 if v<11 else 0xd8),0x7fffffff)
  # The current first energy field is a distinct original rank-stage input.
  b[p+7]=c%17;b[p+8]=int(c%11==0);struct.pack_into('<i',b,p+0x44,-38 if v<11 else -69)
  for j,bit in enumerate([1,2,7,8,9,11,13,14,15,16,17,18,35]):
   if (c+j+i)%4==0:b[p+0x8c+bit//8]|=1<<(bit%8)
 if c%17==0:struct.pack_into('<i',b,o['bases']+0x50,-38 if v<11 else -69)
 if c%23==0:struct.pack_into('<i',b,o['bases']+0x7c,1)
 for i in [0,6,7,12,14,18,21,25,26,27,36]:
  if v<11 and i>=33:continue
  struct.pack_into('<i',b,pf+0x84+i*4,[-1,0,1][(c+i)%3])
 struct.pack_into('<iii',b,pf+0x184-shift,c%3,0,0);struct.pack_into('<ii',b,pf+0x194-shift,c%3,c%2)
 if c==29:put(1,'SE_alloc_labs',11)
 if c==30:put(1,'SE_effic_pending',0x7fffffff)
 return bytes(b)
def model(text,data,profile):
 info,_,sp=population.decode(data);v=int(info['version']);fs=0x20bc if v<11 else 0x20cc;bs=304 if v<11 else 308;schema=population.SCHEMA['LegacyFactionV10' if v<11 else 'Faction'];b=sp['bases'];owner=b[4];f=sp['factions'][owner*fs:(owner+1)*fs];d=sp['faction_definitions'][owner*0x59c:(owner+1)*0x59c];get=lambda name,index=0:population.scalar(f,schema,name,index);dg=lambda name:population.scalar(d,population.SCHEMA['MFaction'],name)
 pref=sp['legacy'] if v<11 else sp['preferences'];projects=struct.unpack_from('<'+('33i' if v<11 else '64i'),pref,0x84);table=population.police.sections(text);m=resource_model(profile,text,data);result=[];statuses=[];factrows=[]
 if m[0]==0:
  population.model(text,data,profile,factrows);com,*_=commerce.model(text,data);trade=list(struct.unpack_from('<28i',com,112))
  cr=citizen_reference(text,profile);keys={row[1].lower():i for i,row in enumerate(table['TECHNOLOGY'])};keys.update(none=-1,disable=-2);owners=pref[(0x1e0 if v>=11 else 0x164):][:89]
  def has(id):
   if owner==0:return False
   if id==-1:return True
   if not 0<=id<88:return False
   a,z=table['TECHNOLOGY'][id][6:8];return a.lower()!='disable' and (z.lower()!='disable' or a.lower()=='none') and bool(owners[id]&(1<<owner))
  citizens=[]
  for i in range(7):
   pre,obsolete,ec,ps,la=struct.unpack_from('<5i',cr,8+276*i+256);citizens.append([int(has(pre)),int(has(obsolete)),ec,ps,la])
  nodes=sum(bool(b[i*bs+0x8c+1]&1) for i in range(int(info['bases'])))
  here=sum(1<<i for i in range(32) if projects[i]==0);grid=int(v>=11 and projects[36]>=0 and b[projects[36]*bs+4]==owner);alien=int(v>=11 and bool(dg('rule_flags')&128));sanction=int(get('sanction_turns')!=0)
  pa=ss=0
  for i in range(1,8):
   if v>=11 and struct.unpack_from('<I',sp['faction_definitions'],i*0x59c+0x508)[0]&128:pa|=1<<(i-1)
   if population.scalar(sp['factions'][i*fs:(i+1)*fs],schema,'sanction_turns'):ss|=1<<(i-1)
  width=int(info['width']);flat=bool(struct.unpack_from('<I',sp['metadata'],28)[0]);x,y=struct.unpack_from('<hh',b);best=999
  for i in range(int(info['bases'])):
   hb=b[i*bs:(i+1)*bs]
   if hb[4]!=owner or not hb[0x8c]&2:continue
   hx,hy=struct.unpack_from('<hh',hb);dx=abs(hx-x);dy=abs(hy-y)
   if not flat and dx>width//2:dx=width-dx
   dist=max(dx,dy)-(((dx+dy)//2-min(dx,dy)+1)//2);best=min(best,dist)
  hq=16 if best==999 else best
 for stage in range(2):
  for policy in range(2):
   e=m[0];values=[]
   if not e:
    if trade[0]:e=7
    elif factrows[stage] is None:e=1
    else:
     p=factrows[stage];a=[m[20],m[3],m[7],*trade[12:19],get('SE_alloc_psych'),get('SE_alloc_labs'),get('SE_effic_pending'),hq,int(bool(b[0x8c]&4)),stage,*[get('social_effic',i) for i in range(9)],get('energy_surplus_total')]
     z=[int(table['RULES'][10][0]),*struct.unpack_from('<2I',b,0x84),here,dg('rule_research'),nodes,grid,alien,int(profile=='expansion'),sanction,pa,ss,trade[-1],b[0x94]&63 if v>=11 else 0,sum(at==0 for at in projects),policy,struct.unpack_from('<i',b,0x50)[0],struct.unpack_from('<i',b,0x44)[0],m[-5]]
     answer=energy.expected_parts(p,a,citizens,z);e=7 if answer[0]==3 else answer[0];values=[*m[1:],*trade[1:],hq,*answer[1:]]
   result.extend([e,*([0x5a5a5a5a]*278 if e else values)]);statuses.append(e)
 guards=84;result.append(guards);return struct.pack('<'+str(len(result))+'I',*[n&0xffffffff for n in result]),statuses,guards

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--binary',type=Path);ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--rules-data',nargs=2,action='append',default=[]);ap.add_argument('--original',nargs=3,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
 exe=a.binary.resolve() if a.binary else out/'world-energy';cmd=[] if a.binary else [a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)]
 if cmd:subprocess.run(cmd,check=True)
 corpus=[(rules('base' if v<11 else 'expansion'),fixture(v,c),'base' if v<11 else 'expansion','synthetic') for v in range(7,13) for c in range(1,33)]
 for profile,path in a.rules_data:corpus += [(Path(path).read_bytes(),fixture(7 if profile=='base' else 12,c),profile,'owner-rules') for c in range(1,33)]
 for profile,rulespath,save in a.original:corpus.append((Path(rulespath).read_bytes(),Path(save).read_bytes(),profile,'owner-world'))
 digest=hashlib.sha256();statuses=[];records=[]
 for i,(text,data,profile,kind) in enumerate(corpus):
  rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);wanted,status,guards=model(text,data,profile);r=subprocess.run([*runner,str(exe),profile,str(rp),str(sp)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.out').write_bytes(r.stdout);(out/f'{i}.expected').write_bytes(wanted);(out/f'{i}.stderr').write_bytes(r.stderr)
  assert r.returncode==0 and r.stdout==wanted,(i,r.returncode,r.stderr,len(r.stdout),len(wanted),next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert sp.read_bytes()==data and rp.read_bytes()==text
  digest.update(r.stdout);statuses+=status;records.append(dict(kind=kind,profile=profile,statuses=status,sha256=hashlib.sha256(r.stdout).hexdigest()))
  if i==191:public=digest.hexdigest()
 receipt=dict(worlds=len(corpus),queries=len(statuses),accepted=statuses.count(0),rejected=len(statuses)-statuses.count(0),guards=len(corpus)*84,publicOutputSHA256=public,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),records=records,command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
