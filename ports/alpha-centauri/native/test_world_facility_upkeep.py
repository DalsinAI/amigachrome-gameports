#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Wire/technology/faction-bonus oracle for original facility upkeep proposals."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_world_energy as energy
import test_world_stable_intake as intake
from test_units import sections,material
from test_facility_definitions import tables
from test_facility_upkeep import expected
HERE=Path(__file__).resolve().parent
NAMES='facility_upkeep rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats world_facility_upkeep world_facility_upkeep_probe'.split()
def rules(variant):
 t=sections(intake.rules(variant));_,f=tables(variant)
 for i,row in enumerate(f,1):row[1:5]=[str(i+2),str(i%5),f'T{i%4}', 'Disable']
 t['FACILITIES']=f
 for i,row in enumerate(t['REACTORS']):row[3]='None'if i==0 else f'T{i}'
 return material(t)
def fixture(variant,version,case):
 b=bytearray(energy.fixture(variant,version,case));_,off,_=intake.decode(b);legacy=version<11;fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308;pf=off['legacy']if legacy else off['preferences'];schema=intake.FACTIONS['LegacyFactionV10'if legacy else 'Faction'];fo=lambda n:off['factions']+fs+next(f['offset']for f in schema['fields']if f['name']==n)
 struct.pack_into('<i',b,fo('diff_level'),case%6)
 bits=sum(1<<id for id in [6,8,27,33,34,38,64,65]);length=8 if legacy else 12;b[off['bases']+0x8c:off['bases']+0x8c+length]=bits.to_bytes(12,'little')[:length];struct.pack_into('<i',b,off['bases']+0x50,[-6,-27,130][case%3])
 struct.pack_into('<i',b,pf+0x84+25*4,[-1,-2,0,1][case%4]);b[off['bases']+bs+4]=2
 for i in range(4):b[pf+0x1e0-(0x7c if legacy else 0)+i]=2 if case>>i&1 else 0
 fd=off['faction_definitions']+0x59c;struct.pack_into('<i',b,fd+0x50c,3);struct.pack_into('<3i',b,fd+0x510,2,12,12);struct.pack_into('<3i',b,fd+0x530,6,8,27)
 return bytes(b)
def model(variant,text,data,mode):
 info,o,spans=intake.decode(data);version=int(info['version']);base=version<11;bs=304 if base else 308;fs=0x20bc if base else 0x20cc;pref=spans['legacy']if base else spans['preferences'];bp=spans['bases'][:bs];owner=bp[4];f=spans['factions'][owner*fs:(owner+1)*fs];fd=spans['faction_definitions'][owner*0x59c:(owner+1)*0x59c];schema=intake.FACTIONS['LegacyFactionV10'if base else 'Faction'];field=lambda n:next(x['offset']for x in schema['fields']if x['name']==n);t=sections(text);tech=t['TECHNOLOGY'];keys={r[1].lower():i for i,r in enumerate(tech)};keys.update(none=-1,disable=-2)
 def known(token):
  id=keys[token.lower()]
  if id==-1:return True
  if not 0<=id<88:return False
  a,b=(keys[tech[id][j].lower()]for j in [6,7]);return a!=-2 and not(b==-2 and a!=-1)and bool(pref[0x1e0-(0x7c if base else 0)+id]&(1<<owner))
 reactor=next((i+1 for i in reversed(range(4))if known(t['REACTORS'][i][3])),1);first=39 if base else 70;project=struct.unpack_from('<i',pref,0x84+25*4)[0];sac=int(project>=0 and spans['bases'][project*bs+4]==owner);built=int.from_bytes(bp[0x8c:0x8c+(8 if base else 12)],'little');free=0;cost=[0]*70;maint=[0]*70;count=struct.unpack_from('<i',fd,0x50c)[0]
 for id in range(1,first):
  row=t['FACILITIES'][id-1];cost[id]=int(row[1]);maint[id]=int(row[2])
  for j in range(count):
   bonus=struct.unpack_from('<i',fd,0x510+4*j)[0];value=struct.unpack_from('<i',fd,0x530+4*j)[0]
   if value==id and(bonus==2 or bonus==12 and known(row[3])):free|=1<<id
 diff=struct.unpack_from('<i',f,field('diff_level'))[0];queue=struct.unpack_from('<i',bp,0x50)[0];v=[first,diff,reactor,100 if mode&1 else 0,7,10,queue,0x200|(32 if mode&2 else 0),sac,*[(bits>>(32*i))&0xffffffff for bits in [built,free]for i in range(3)],*cost,*maint];return expected(v)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.out/'world-facility-upkeep-probe';cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(n+'.cpp'))for n in NAMES],'-o',str(exe)];subprocess.run(cmd,check=True);records=[]
 for version in range(7,13):
  variant='base'if version<11 else 'expansion';text=rules(variant);rp=a.out/(variant+'.txt');rp.write_bytes(text)
  for case in range(12):
   data=fixture(variant,version,case);path=a.out/f'v{version}-{case}.SC';path.write_bytes(data);mode=case%4;run=subprocess.run([str(exe),variant,str(rp),str(path),str(mode)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30);assert run.returncode==0,(version,case,run.returncode,run.stderr);want=model(variant,text,data,mode);got=list(struct.unpack('<83I',run.stdout));assert got==want,(version,case,got,want)
   records.append(dict(name=path.name,mode=mode,passed=True,expectedOutputSHA256=hashlib.sha256(run.stdout).hexdigest()))
 receipt=dict(cases=len(records),command=cmd,casesDetail=records,native='UNRUN',source='Original fac_maint/base_maint/best_reactor and physical built predicate');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(records)} connected facility upkeep cases passed')
if __name__=='__main__':main()
