#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Plain explicit energy accounting versus independent source-stage model."""
import argparse,hashlib,itertools,json,os,struct,subprocess
from pathlib import Path
from test_energy_inefficiency import expected as ineff,MIN,MAX,word
HERE=Path(__file__).resolve().parent

def expected(row):
 intake,to,fr,*rest=row;imports=rest[:7];psy,labs,eff,distance,creche,stage=rest[7:13];hist=rest[13:22];faction=rest[22];fail=lambda e:[e]+[0x5a5a5a5a]*18
 if not 0<=psy<=10 or not 0<=labs<=10 or psy+labs>10:return fail(1)
 current=intake+to
 if not word(current) or not word(current-fr):return fail(3)
 first=ineff([current-fr,eff,distance,creche,stage,*hist])
 if first[0]:return fail(first[0])
 initial=current-fr-first[1]
 if not word(initial):return fail(3)
 if stage==1:faction+=max(0,min(99999,initial))
 if not word(faction):return fail(3)
 for v in imports:
  current+=v
  if not word(current):return fail(3)
 if not word(current-fr):return fail(3)
 second=ineff([current-fr,eff,distance,creche,stage,*first[2:]])
 if second[0]:return fail(second[0])
 surplus=current-fr-second[1]
 if not word(surplus):return fail(3)
 total=max(0,min(9999,surplus));psych=max(0,min(total,(total*psy+4)//10));econ=max(0,min(total-psych,(total*(10-labs-psy)+4)//10));research=total-psych-econ;unknown=max(0,min(total-psych,(total*(9-labs-psy)+4)//10))
 penalty=abs(max(-4,min(4,eff))-4)*2*(abs(psy-10+2*labs)//2)
 if penalty:
  if 10-labs-psy<labs:lp=min(40,penalty);ep=min(80,2*penalty)
  else:lp=min(80,2*penalty);ep=min(40,penalty)
  research=((100-lp)*research+50)//100;econ=((100-ep)*econ+50)//100
 return [0,current,fr,second[1],surplus,econ,psych,research,unknown,faction,*second[2:]]
def corpus():
 rows=[]
 for n in range(1,257):
  saved=[n*13%83,n*7%97,n*11%101,n*17%127];rank=int(saved[1]*512+1>saved[0]*512);partner=sorted([2,3],key=lambda i:saved[i]*512+i,reverse=True)[rank];imports=[0]*7;treaty=[0,2,3,2][n%4];governor=n%3;tech=n%7+1
  if treaty&2 or n&8:
   value=(saved[0]+saved[partner]+7)//8
   if n&4:value*=2
   imp=((tech+1)//2+(n%4+1)*value)//(tech+1)
   if not treaty&1:imp//=2
   if governor==1:imp+=1
   if n%17==0 or n%19==0:imp=0
   imports[1]=imp
  rows.append([5+n*9%190,n%8,n%5,*imports,n%4,(n//4)%(11-n%4),-4+n%9,0 if n//4%2 else 18 if n%2 else 6,n//2%2,n%2,*[j*17+n%13 for j in range(9)],n*31])
 for value,psy,labs,stage in itertools.product([-100,0,1,9999,MAX],[0,3,10],[0,3,10],[0,1]):rows.append([value,1,2,3,0,0,0,0,0,0,psy,labs,0,16,0,stage,*[0]*9,17])
 for index,value in [(10,-1),(11,-1),(13,-1),(13,999),(14,2),(15,2),(0,MIN),(25,MAX),(16,MAX)]:
  row=[100,1,2,3,0,0,0,0,0,0,2,5,0,16,0,1,*[0]*9,17];row[index]=value;rows.append(row)
 return rows

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.out/'energy-allocation';flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'];cmd=[a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(HERE/n) for n in ['energy_inefficiency.cpp','energy_allocation.cpp','energy_allocation_probe.cpp']],'-o',str(exe)];subprocess.run(cmd,check=True);rows=corpus();data=b'ENA1'+struct.pack('<I',len(rows))+b''.join(struct.pack('<26I',*[v&0xffffffff for v in row]) for row in rows);p=a.out/'controls.bin';p.write_bytes(data);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];r=subprocess.run([*runner,str(exe),str(p)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stderr);want=b''.join(struct.pack('<19I',*[v&0xffffffff for v in expected(row)]) for row in rows);assert r.stdout==want,(len(r.stdout),len(want),next((i for i,(x,y) in enumerate(zip(r.stdout,want)) if x!=y),None));assert p.read_bytes()==data;(a.out/'outputs.bin').write_bytes(r.stdout);a0=sum(expected(row)[0]==0 for row in rows);bad=sum(expected(row)[0]==1 for row in rows);over=sum(expected(row)[0]==3 for row in rows);original=b''.join(r.stdout[76*i+4:76*(i+1)] for i in range(256));receipt={'cases':len(rows),'accepted':a0,'invalid':bad,'overflow':over,'aliasesPerCase':3,'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'original256ProjectedOutputSHA256':hashlib.sha256(original).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN','scope':'Explicit plain prepsych accounting only; commerce supplied, noWorldauthority/bonusfacilities/specialists/aliengrid/psych/commit/upkeep/turn.'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k!='command'}))
if __name__=='__main__':main()
