#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Explicit psych proposals against an independent five-phase integer model."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
MIN,MAX=-(1<<31),(1<<31)-1
def word(n):
 if not MIN<=n<=MAX:raise OverflowError(n)
 return n
def div(a,b):return (1 if a*b>=0 else -1)*(abs(a)//abs(b))
def clamp(v,lo,hi):return max(lo,min(hi,v))
def corpus():
 rows=[]
 for edition in [0,1]:
  for n in range(1536):
   r=random.Random(0x5145+n);pop=r.randrange(1,26);special=r.randrange(pop+1);human=r.randrange(2);diff=r.randrange(6);eff=r.randrange(-5,7);mapfactor=r.choice([0,7,14,28,56]);bases=r.randrange(1,60);capture=r.choice([0,1,9,10,11,30,255]);rd=r.choice([0,2,3,5]);rt=r.choice([0,2,3,7]);tal=r.randrange(-4,5);r.randrange(-5,-1);pac=r.randrange(7);psych=r.randrange(80);gene=r.randrange(1,5);econ=r.randrange(4);staple=r.randrange(2);fac={i:r.randrange(2) for i in [6,8,11,12,17,18,22,23]};projects={i:r.choice([-1,1,2]) for i in [0,6,7,14,26,25]};units=[];ur=random.Random(0x504f4c+n)
   for i in range(8):
    double=ur.randrange(2);native=ur.randrange(2);sea=ur.randrange(2);plan=ur.randrange(6)
    if n%32==0:
     plan=5 if i<7 else 0
     if i==7:double=native=sea=0
    units.extend([15 if i==7 else i,plan,-1 if native else 1,sea,double])
   rows.append([edition,human,n%2,pop,special,capture,diff,eff,mapfactor,0,bases,rd,rt,int(n>=512),(n%9)-4 if n>=512 else 0,tal,n%11-6,pac,psych,gene,econ,staple,sum(1<<i for i,v in fac.items() if v),8 if edition and n%3==0 else 0,sum(1<<i for i,v in projects.items() if v==1),int(bool(n%2 or n%32==0)),8 if n%32==0 else n%9,*[i*17+n%13 for i in range(82)],*units])
 template=rows[1900]
 for index,value in [(0,2),(1,2),(2,2),(3,-1),(3,128),(4,-1),(4,128),(5,-1),(5,256),(6,-1),(6,6),(8,-1),(9,-1),(9,512),(10,-1),(10,513),(11,-1),(12,-1),(13,2),(17,-1),(21,256),(25,2),(26,2049),(109,-1),(109,512),(110,-1),(110,256),(111,-129),(111,128),(112,-1),(112,3),(113,2)]:
  row=template.copy();row[26]=8;row[index]=value;rows.append(row)
 for index,value in [(7,MAX),(8,MAX),(12,MAX),(14,MIN),(15,MIN),(15,MAX),(16,MAX),(17,MAX),(19,MIN),(27+81,MAX),(27+81,MIN)]:
  row=template.copy();row[2]=1;row[3]=25;row[4]=0;row[8]=56;row[11]=0;row[12]=2;row[13]=1;row[14]=0;row[15]=2;row[16]=-6;row[17]=7;row[19]=2;row[22]|=1<<22;row[index]=value;rows.append(row)
 for pop in [0,1,127]:
  for special in [0,pop]:
   row=template.copy();row[3]=pop;row[4]=special;row[15]=10000;rows.append(row)
 return rows

def expected(row):
 fail=lambda e:[e]+[0x5a5a5a5a]*100
 edition,human,stage,pop,special,capture,diff,eff,mfactor,bid,bases,rd,rt,hasno,nodrone,talent,police,pac,psych,gene,econ,staple,faclo,fachi,projects,worm,count=row[:27]
 if count>2048:return fail(1)
 if any(x not in [0,1] for x in [edition,human,stage,hasno,worm]) or not 0<=pop<=127 or not 0<=special<=pop or not 0<=capture<=255 or not 0<=diff<=5 or mfactor<0 or not 0<=bid<=511 or not 0<=bases<=512 or rd<0 or rt<0 or pac<0 or not 0<=staple<=255:return fail(1)
 units=[row[109+5*i:114+5*i] for i in range(count)]
 if any(not(0<=u[0]<=511 and 0<=u[1]<=255 and -128<=u[2]<=127 and 0<=u[3]<=2 and u[4] in [0,1]) for u in units):return fail(1)
 fac=lambda i:bool((faclo if i<32 else fachi)&(1<<(i%32)))
 proj=lambda i:bool(projects&(1<<i))
 try:
  effective=diff if human else 3;limit=div(word(word(word(max(eff,0)+4)*(8-effective))*mfactor),112)
  bureaucratic=clamp(div(bid%limit+bases-limit,limit),0,pop) if limit else 0
  drones=clamp(pop-6+effective,0,pop)+max(0,min(div(pop-2+effective,4),(capture+9)//10))
  if rd:drones+=div(pop,rd)
  talents=div(word(rt-1+pop),rt) if rt else 0
  if edition and hasno:drones=max(0,word(drones-nodrone))
  facility=gene if fac(22) else 0
  if fac(6):facility=word(facility-2)
  if (fac(8) and proj(6)) or fac(11):facility=word(facility-2)
  if proj(7) and pop<4:facility=word(facility-1)
  if fac(17):facility=word(facility-1)
  if fac(18):facility=word(facility-1)
  def phases(tal,pol):
   d=word(drones-min(0,tal));t=word(talents+max(0,tal));d=word(clamp(d,0,pop)+bureaucratic);s=clamp(d-pop,0,pop);d=clamp(d,0,pop);rows=[]
   for phase in range(5):
    if phase==1:t=word(t+div(psych,2))
    if phase==2:t=word(t+2*fac(12));d=word(d+facility)
    if phase==3:
     quota=clamp(word(pol+2),0,4)
     if quota>=2:quota-=1
     used=0
     if quota and proj(25):d=max(0,d-1-int(pol>2));used=1
     for priority in [True,False]:
      for proto,plan,offense,triad,double in units:
       if used>=quota:break
       native=bool(edition and worm and proto<64 and (offense<0 or proto==15) and triad==0)
       if plan<4 and bool(double or native)==priority:d=max(0,d-1-double-int(pol>2)-int(native));used+=1
     extra=pac if pol==-4 else max(0,pac-1) if pol==-3 else word(2*pac) if pol<-4 else 0
     d=word(d+extra)
    if phase==4:
     t=word(t+int(proj(0))+2*int(proj(26)))
     if proj(14):d=word(d-int(econ==2)-int(econ!=1))
     if fac(23) or staple:t=d=0
    if phase:t=clamp(t,0,pop);d=clamp(d,0,pop)
    word(word(t+d)+s)
    while t+d+s>pop-special:
     if t:
      t-=1
      if s:s-=1
      elif d:d-=1
     elif d<=pop-special:s=clamp(s,0,d);break
     else:d-=1
    rows.append([t,d,s])
   return rows
  social=row[27:109].copy()
  if stage:
   for ti in range(8,0,-1):
    for pi in range(9,0,-1):
     t,d,_=phases(ti-4,pi-6)[-1];v=t-d
     if v>0 and (d or t*2<pop):v=div(v,2)
     if v<0:
      v-=pop
      if pop<6:v*=2
     social[ti*9+pi]=word(social[ti*9+pi]+v)
  if edition and fac(35):police=word(police+2)
  rows=phases(talent,police)
  return [0,*[v for r in rows for v in r],*rows[-1],*social]
 except OverflowError:return fail(3)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.out/'base-psych';flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'];cmd=[a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(HERE/n) for n in ['base_psych.cpp','base_psych_probe.cpp']],'-o',str(exe)];subprocess.run(cmd,check=True);rows=corpus();data=b'PSY1'+struct.pack('<I',len(rows))+b''.join(struct.pack('<149I',*[v&0xffffffff for v in row]) for row in rows);p=a.out/'controls.bin';p.write_bytes(data);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];r=subprocess.run([*runner,str(exe),str(p)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(a.out/'outputs.bin').write_bytes(r.stdout);(a.out/'stderr.txt').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);want=b''.join(struct.pack('<101I',*[v&0xffffffff for v in expected(row)]) for row in rows);(a.out/'expected.bin').write_bytes(want);assert r.stdout==want,(len(r.stdout),len(want),next((i for i,(x,y) in enumerate(zip(r.stdout,want)) if x!=y),None));assert p.read_bytes()==data;statuses=[expected(row)[0] for row in rows];receipt={'cases':len(rows),'accepted':statuses.count(0),'invalid':statuses.count(1),'overflow':statuses.count(3),'factsAliasesPerCase':3,'garrisonAliasWhenNonempty':True,'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN','scope':'Explicit five-phase psych/social accounting proposal only. No World/garrison extraction, ability resolution, stackrepair, authority/commit/upkeep/turn/UI/save.'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k!='command'}))
if __name__=='__main__':main()
