#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original mineral stage: source body plus independent checked integer model."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent
FIELDS='intake convoy_to convoy_from maintenance raw_eco_damage population clean_minerals_modifier satellites atrocities tectonic_detonations sea_level sea_level_rises tech_ranking transcendent_techs planet native_life faction_difficulty global_difficulty queue_id'.split()
FLAGS='human ascending transcendence_enabled perihelion quantum_converter robotic_assembly genejack nanoreplicator singularity_inductor bulk_matter centauri_preserve temple pholus_mutagen space_elevator'.split()
SENTINEL=0x31415926
def clamp(n,lo,hi):return min(hi,max(lo,n))
def checked(n):
 if not(-2**31<=n<2**31):raise OverflowError
 return n
def divide(n,d):return (abs(n)//d)*(1 if n>=0 else -1)
def model(values,flags):
 d=dict(zip(FIELDS,values));d.update((n,bool(flags&(1<<i))) for i,n in enumerate(FLAGS))
 invalid=flags&~0x3fff or any(d[n]<0 for n in ['intake','convoy_to','convoy_from','maintenance','raw_eco_damage','population','clean_minerals_modifier','atrocities','tectonic_detonations','tech_ranking','transcendent_techs']) or d['population']>127 or d['transcendent_techs']>d['tech_ranking'] or not(0<=d['native_life']<=2) or (d['human'] and not(0<=d['faction_difficulty']<=5)) or (not d['human'] and not(0<=d['global_difficulty']<=5))
 if invalid:return [1]+[SENTINEL]*7
 try:
  bonus=int(d['quantum_converter'] or d['singularity_inductor'])+d['robotic_assembly']+d['genejack']+d['nanoreplicator']+d['bulk_matter']
  intake=checked(checked(d['intake']+d['convoy_to'])*(bonus+2))//2
  consumption=checked(d['maintenance']+d['convoy_from']);surplus=checked(intake-consumption);final_surplus=surplus
  damage=d['raw_eco_damage']//8;clean=checked(d['clean_minerals_modifier']+16)
  if damage>0:
   removed=min(damage,clean);damage-=removed;clean-=removed
  reduction=(2 if d['nanoreplicator'] or d['singularity_inductor'] else 1)+d['centauri_preserve']+d['temple']+d['pholus_mutagen']
  pollution=checked(checked(intake-clean)-clamp(d['satellites'],0,d['population']))
  damage=checked(damage+divide(pollution,reduction))
  if d['human']:
   denominator=clamp(d['sea_level'],0,100)//clamp(d['sea_level_rises'],1,100)+reduction
   damage=checked(damage+checked(d['atrocities']*5)//denominator)
   damage=checked(damage+checked(d['tectonic_detonations']*5)//denominator)
  damage=max(0,damage)
  if d['ascending'] and d['transcendence_enabled']:damage=checked(damage*2)
  if d['perihelion']:damage=checked(damage*2)
  difficulty=(2 if d['faction_difficulty']==0 else 3 if d['faction_difficulty']<=3 else 5) if d['human'] else clamp(6-d['global_difficulty'],1,3)
  effective=checked((d['tech_ranking']-d['transcendent_techs'])*(3-clamp(d['planet'],-3,2)))
  weighted=checked(effective*(d['native_life']+1));weighted=checked(weighted*damage);weighted=checked(weighted*difficulty)
  eco=(weighted//6+50)//100
  if d['space_elevator'] and d['queue_id'] in (-65,-66,-67,-68):intake=checked(intake*2);surplus=checked(intake-consumption)
  return [0,intake,consumption,surplus,final_surplus,eco,bonus,0]
 except OverflowError:return [2]+[SENTINEL]*7

def controls():
 rows=[]
 for f in range(1<<14):
  v=[f%37,3*(f%7),f%5,f%4,(f*73)%400,3+f%25,f%10,f%71-10,f%4,f%3,f%171-40,f%121-10,10+f%50,f%5,f%11-5,f%3,f%6,(f//6)%6,[0,-65,-66,-67,-68,-69,-70,320][f%8]]
  rows.append((v,f))
 baseline=[10,0,0,0,8,3,0,0,0,0,0,5,10,0,0,1,3,3,0]
 for k in range(19):
  for value in [-2147483648,-1,0,1,127,128,2147483647]:
   for human in (0,1):v=baseline[:];v[k]=value;rows.append((v,human))
 # The original instructions retain signed32 arithmetic; upper-half
 # products are rejected before a reconstructed-source reference can overflow.
 v=baseline[:];v[0]=0;v[4]=136;v[12]=1500000000;v[14]=2;v[15]=1;v[17]=5
 assert model(v,0)==[2]+[SENTINEL]*7
 rows.append((v,0));v=v[:];v[15]=2;rows.append((v,0))
 # Recovered unsigned resource multiplication would incorrectly allow this.
 v=baseline[:];v[0]=2147483647;v[4]=0;v[12]=0;rows.append((v,0))
 v=v[:];v[1]=1;rows.append((v,0))
 # Preserve original convoy amplification and stale final surplus after elevator.
 v=baseline[:];v[0]=8;v[1]=4;v[2]=1;v[3]=2;v[18]=-65
 flags=(1<<4)|(1<<13);expected=model(v,flags);assert expected[1:5]==[36,3,33,15],expected
 rows.append((v,flags))
 # Separate original atrocity/tectonic divisions: floor(5/3)+floor(5/3)=2,
 # whereas the recovered combined expression gives floor(10/3)=3.
 v=baseline[:];v[0]=16;v[4]=0;v[8]=v[9]=1;v[10]=2;v[11]=1;v[12]=600;v[14]=2;v[15]=0;v[16]=0
 assert model(v,1)[5]==4
 rows.append((v,1))
 rows.append((baseline,0x4000))
 return rows

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--probe',type=Path);ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=a.probe.resolve() if a.probe else out/'base-minerals-probe';command=[a.cc,*flags,str(SRC/'base_minerals.cpp'),str(SRC/'base_minerals_probe.cpp'),'-o',str(exe)]
 if not a.probe:subprocess.run(command,check=True)
 rows=controls();data=b'MNR1'+struct.pack('<I',len(rows))+b''.join(struct.pack('<19iI',*v,f) for v,f in rows)
 expected=b''.join(struct.pack('<8i',*model(v,f)) for v,f in rows);source=out/'controls.bin';dest=out/'results.bin';source.write_bytes(data)
 r=subprocess.run([*runner,str(exe),str(source),str(dest)],capture_output=True,text=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
 assert r.returncode==0,(r.returncode,r.stdout,r.stderr)
 actual=dest.read_bytes()
 if actual!=expected:
  for i,(v,f) in enumerate(rows):
   if actual[i*32:(i+1)*32]!=expected[i*32:(i+1)*32]:raise AssertionError((i,v,f,struct.unpack('<8i',actual[i*32:(i+1)*32]),model(v,f)))
 errors=[model(v,f)[0] for v,f in rows]
 receipt=dict(command=None if a.probe else command,suppliedProbe=str(exe) if a.probe else None,runner=runner,cases=len(rows),valid=errors.count(0),badState=errors.count(1),overflow=errors.count(2),outputSHA256=hashlib.sha256(actual).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),stdout=r.stdout)
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['cases','valid','badState','overflow','outputSHA256']}))
if __name__=='__main__':main()
