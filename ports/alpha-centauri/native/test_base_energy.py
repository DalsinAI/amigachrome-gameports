#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Complete explicit energy proposals; independent integer and psych models."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
import test_base_psych as psychmodel
from test_energy_inefficiency import expected as ineff
HERE=Path(__file__).resolve().parent
MIN,MAX=-(1<<31),(1<<31)-1
def checked(v):
 if not MIN<=v<=MAX:raise OverflowError(v)
 return v
def div(a,b):return (1 if a*b>=0 else -1)*(abs(a)//abs(b))
def clip(n,lo,hi):return max(lo,min(hi,n))
def corpus():
 rows=[];ps=psychmodel.corpus()
 for edition in [0,1]:
  for n in range(768):
   p=ps[edition*1536+n].copy();r=random.Random(0x454e47+n);fac=p[22]
   for i in [1,2,7,8,9,13,14,15,16]:
    if r.randrange(2):fac|=1<<i
   p[22]=fac;here=sum(1<<i for i in [0,6,7,12,14,18,21,25,26,27] if r.randrange(3)==0);p[24]|=here
   alien=int(edition and n%3==0);sanction=int(n%17==0);pa=(r.randrange(128) if edition else 0);ss=r.randrange(128);imports=[r.randrange(14) if not(alien or sanction or pa&(1<<i) or ss&(1<<i)) else 0 for i in range(7)]
   psy=n%4;labs=(n//4)%(11-psy);a=[r.randrange(-30,500),r.randrange(9),r.randrange(7),*imports,psy,labs,p[7],0 if fac&2 else 16,int(bool(fac&4)),p[2],*[j*17+n%13 for j in range(9)],n*31]
   c=[]
   for _ in range(7):c.extend([r.randrange(2),r.randrange(2),r.randrange(7),r.randrange(5),r.randrange(8)])
   x=[r.randrange(1,26),r.getrandbits(32),r.getrandbits(32),here,r.randrange(-5,8),r.randrange(int(bool(fac&256)),513),int(edition and n%5==0),alien,(n//3)%2,sanction,pa,ss,(n//6)%2,r.randrange(64) if edition else 0,here.bit_count()+r.randrange(20),n%2,-69 if edition else -38,-69 if edition else -38,r.randrange(-5,80)]
   rows.append(p+a+c+x)
 template=next(row for row in rows if expected(row)[0]==0 and row[26]>0)
 for index,value in [(0,2),(1,2),(2,2),(3,-1),(4,128),(26,2049),(149+10,11),(149+11,-1),(149+12,MAX),(149+14,2),(175,2),(176,2),(210+5,513),(210+6,2),(210+7,2),(210+8,2),(210+9,2),(210+10,128),(210+11,128),(210+12,2),(210+13,64),(210+14,65),(210+15,2)]:
  row=template.copy();row[index]=value;rows.append(row)
 for index,value in [(149,MAX),(149,MIN),(149+16,MAX),(149+25,MAX),(177,MAX),(178,MAX),(179,MIN),(210+4,MAX),(210+18,MAX)]:
  row=template.copy();row[4]=20;row[3]=25;row[2]=row[164]=1;row[149]=500;row[150]=5;row[151]=0;row[162]=16;row[22]&=~2;row[210+3]|=(1<<12)|(1<<18);row[210+16]=row[210+17]=-38;row[index]=value;rows.append(row)
 return rows

def expected(row):
 return expected_parts(row[:149],row[149:175],[row[175+5*i:180+5*i] for i in range(7)],row[210:])

def expected_parts(p,a,c,x):
 fail=lambda e:[e]+[0x5a5a5a5a]*120
 threshold,packed0,packed1,here,rule,nodes,gridowned,alien,enabled,sanction,pa,ss,qualified,extra,projectcount,policy,item,previous,minerals=x
 if p[26]>2048:return fail(1)
 if any(p[i] not in [0,1] for i in [0,1,2,13,25]) or not 0<=p[3]<=127 or not 0<=p[4]<=p[3] or not 0<=p[5]<=255 or not 0<=p[6]<=5 or p[8]<0 or not 0<=p[9]<=511 or not 0<=p[10]<=512 or p[11]<0 or p[12]<0 or p[17]<0 or not 0<=p[21]<=255:return fail(1)
 if any(not(0<=p[109+5*i]<=511 and 0<=p[110+5*i]<=255 and -128<=p[111+5*i]<=127 and 0<=p[112+5*i]<=2 and p[113+5*i] in [0,1]) for i in range(p[26])):return fail(1)
 if any(v not in [0,1] for v in [gridowned,alien,enabled,sanction,qualified,policy]) or not 0<=pa<=127 or not 0<=ss<=127 or not 0<=extra<=63 or not 0<=projectcount<=64 or not 0<=nodes<=512:return fail(1)
 if not p[0] and (alien or gridowned or pa):return fail(1)
 if a[12]!=p[7] or a[15]!=p[2] or a[14]!=int(bool(p[22]&4)) or not 0<=a[10]<=10 or not 0<=a[11]<=10 or a[10]+a[11]>10:return fail(1)
 if any(a[3+i] and (alien or sanction or pa&(1<<i) or ss&(1<<i)) for i in range(7)):return fail(1)
 if any(v not in [0,1] for citizen in c for v in citizen[:2]):return fail(1)
 if alien and (enabled or here&(1<<21)) and not qualified and policy==0:return fail(1)
 try:
  intake=checked(a[0]+a[1]);consumed=a[2];first_net=checked(intake-consumed);first=ineff([first_net,*a[12:16],*a[16:25]])
  if first[0]:return fail(first[0])
  initial=checked(first_net-first[1])
  for imp in a[3:10]:intake=checked(intake+imp)
  grid=0
  if alien and enabled:
   grid=(p[22].bit_count()+p[23].bit_count()+extra.bit_count()+5*projectcount)//2
   if sanction:grid=0
   intake=checked(intake+grid)
  faction=a[25]
  if a[15]==1:faction=checked(faction+clip(initial,0,99999))
  net=checked(intake-consumed);second=ineff([net,*a[12:16],*first[2:]])
  if second[0]:return fail(second[0])
  loss=second[1];surplus=checked(net-loss);total=clip(surplus,0,9999);pr,lr=a[10:12];psych=clip(div(total*pr+4,10),0,total);econ=clip(div(total*(10-lr-pr)+4,10),0,total-psych);labs=total-psych-econ;unknown=clip(div(total*(9-lr-pr)+4,10),0,total-psych)
  best=0;score=-999
  if p[4]>16:
   for i,(pre,obsolete,ec,ps,la) in enumerate(c):
    if pre:
     value=checked(3*ps)
     if p[3]>=threshold:value=checked(checked(value+ec)+la)
     if value>score:score=value;best=i
  packed=[packed0,packed1]
  for i in range(p[4]):
   selected=min(6,(packed[i//8]>>((i%8)*4))&15) if i<16 else best
   if c[selected][1]:
    old=c[selected][2:]
    for j,citizen in enumerate(c):
     if citizen[0] and not citizen[1] and all(v<=w for v,w in zip(old,citizen[2:])):
      selected=j
      if i<16:
       shift=(i%8)*4;packed[i//8]=(packed[i//8]&~(15<<shift))|(j<<shift)
      break
   ec,ps,la=c[selected][2:];econ=checked(econ+ec);psych=checked(psych+ps);labs=checked(labs+la)
  fac=lambda i:bool(p[22]&(1<<i));proj=lambda i:bool(here&(1<<i));owned=lambda i:bool(p[24]&(1<<i))
  labs=checked(labs+rule)
  if fac(9):labs=checked(labs+2)
  ce=6 if fac(7) or (p[0] and gridowned) else 4;cl=6 if fac(8) else 4;cp=6 if fac(11) or (fac(8) and owned(6)) else 4
  for i in [17,18]:
   if fac(i):cp+=1;cl+=2
  for i in [13,14]:
   if fac(i):cp+=2;ce+=2
  for i in [15,16]:
   if fac(i):ce+=2;cl+=2
  scale=lambda v,m,add,denom:div(checked(checked(v*m)+add),denom)
  psych=scale(psych,cp,3,4);labs=scale(labs,cl,3,4)
  for i in [12,18]:
   if proj(i):labs=scale(labs,2,0,1)
  econ=scale(econ,ce,3,4)
  if proj(27):econ=scale(econ,2,0,1)
  if proj(14) and p[20]==1:econ=checked(econ+div(econ,2))
  unknown=scale(unknown,ce,3,4)
  if proj(21):
   labs=checked(labs+nodes)
   if alien:labs=checked(labs+grid)
   elif not sanction:
    for i,v in enumerate(a[3:10]):
     if not(ss&(1<<i) or pa&(1<<i)):labs=checked(labs+v)
  penalty=abs(pr-10+2*lr)//2*(4-clip(a[12],-4,4))*2
  if penalty:
   heavy=10-lr-pr<lr;lp=min(40 if heavy else 80,penalty*(1 if heavy else 2));ep=min(80 if heavy else 40,penalty*(2 if heavy else 1));labs=scale(labs,100-lp,50,100);econ=scale(econ,100-ep,50,100)
  repeat=-69 if p[0] else -38
  if item==previous==repeat and minerals>0:
   stock=div(checked(minerals+1),2)
   if p[0] and gridowned:stock=scale(stock,5,3,4)
   econ=checked(econ+stock)
  p=p.copy();p[18]=psych;population=psychmodel.expected(p)
  if population[0]:return fail(population[0])
  return [0,intake,consumed,loss,surplus,econ,psych,labs,unknown,faction,*second[2:],*population[1:],*packed]
 except OverflowError:return fail(3)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.out/'base-energy';flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'];cmd=[a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(HERE/f) for f in ['energy_inefficiency.cpp','base_psych.cpp','base_energy.cpp','base_energy_probe.cpp']],'-o',str(exe)];subprocess.run(cmd,check=True);rows=corpus();data=b'BEE1'+struct.pack('<I',len(rows))+b''.join(struct.pack('<229I',*[v&0xffffffff for v in row]) for row in rows);(a.out/'controls.bin').write_bytes(data);want=b''.join(struct.pack('<121I',*[v&0xffffffff for v in expected(row)]) for row in rows);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];r=subprocess.run([*runner,str(exe),str(a.out/'controls.bin')],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(a.out/'outputs.bin').write_bytes(r.stdout);(a.out/'expected.bin').write_bytes(want);(a.out/'stderr.txt').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want,(len(r.stdout),len(want),next((i for i,(x,y) in enumerate(zip(r.stdout,want)) if x!=y),None));assert (a.out/'controls.bin').read_bytes()==data;status=[expected(row)[0] for row in rows];receipt={'cases':len(rows),'accepted':status.count(0),'invalid':status.count(1),'overflow':status.count(3),'factsAliasesEach':3,'garrisonAliasesWhenNonempty':True,'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'scope':'Explicit complete composer only; supplied commerce/technology/ownership/garrison facts, explicit alien-zero policy. No World/authority/commit/upkeep/turn/UI/save.','native':'UNRUN'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k!='command'}))
if __name__=='__main__':main()
