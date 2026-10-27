#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent integer oracle for the pinned default energy allocation policy."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
SENTINEL=0x5a5a5a5a
def pack(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))
def signed(n):return n if n<0x80000000 else n-0x100000000
def trunc(n,d):return n//d if n>=0 else -((-n)//d)
def expected(words):
 surplus,labs,psych,eff,pop,count,minimum,*_=words
 types=list(words[7:9]);pre=words[9:16];obsolete=words[16:23];table_count=words[23];citizens=[words[i:i+5]for i in range(24,59,5)];flags=words[59:74];nodes,commerce=words[74:76]
 fail=lambda error:[error]+[SENTINEL]*13
 if not(0<=labs<=10 and 0<=psych<=10-labs and 0<=pop<=127 and 0<=count<=pop and 0<=minimum<=100 and table_count==7 and 0<=nodes<=512 and commerce>=0):return fail(1)
 if any(not(-2<=c[0]<89 and -2<=c[1]<89 and min(c[2:])>=0 and pre[i]in(0,1)and obsolete[i]in(0,1))for i,c in enumerate(citizens)):return fail(1)
 def checked(n):
  if not -2147483648<=n<=2147483647:raise OverflowError
  return n
 try:
  total=max(0,min(9999,surplus));p=max(0,min(total,trunc(total*psych+4,10)));e=max(0,min(total-p,trunc(total*(10-labs-psych)+4,10)));unknown=max(0,min(total-p,trunc(total*(9-labs-psych)+4,10)));l=total-p-e;special=[0,0,0]
  best=0;best_score=-2147483648
  if count>16:
   for i,c in enumerate(citizens):
    if pre[i]and not obsolete[i]and(c[3]>=2 or pop>=minimum):
     value=checked(c[2]+c[4]+2*c[3])
     if value>best_score:best,best_score=i,value
  for i in range(count):
   kind=min(6,(types[i//8]>>(4*(i%8)))&15)if i<16 else best
   old=citizens[kind]
   if obsolete[kind]:
    for j,c in enumerate(citizens):
     if pre[j]and not obsolete[j]and all(c[k]>=old[k]for k in(2,3,4)):kind=j;break
   if i<16:types[i//8]=(types[i//8]&~(15<<(4*(i%8))))|(kind<<(4*(i%8)))
   c=citizens[kind];e=checked(e+c[2]);p=checked(p+c[3]);l=checked(l+c[4]);special=[checked(special[j]+c[j+2])for j in range(3)]
  bio,bank,node,theatre,hospital,nano,tree,hybrid,fusion,quantum,collider,theory,elevator,longevity,backbone=map(bool,flags)
  if bio:l=checked(l+2)
  ce=4+2*sum([bank,tree,hybrid,fusion,quantum]);cp=4+2*theatre+hospital+nano+2*sum([tree,hybrid]);cl=4+2*sum([node,hospital,nano,fusion,quantum])
  e,p,l,unknown=[trunc(checked(checked(value*coef)+3),4)for value,coef in zip([e,p,l,unknown],[ce,cp,cl,ce])]
  if collider:l=checked(l*2)
  if theory:l=checked(l*2)
  if elevator:e=checked(e*2)
  if longevity:e=checked(e+trunc(e,2))
  if backbone:l=checked(checked(l+nodes)+commerce)
  effic=4-max(-4,min(4,eff));imbalance=abs(trunc(2*labs+psych-10,2))
  if imbalance and effic:
   penalty=imbalance*effic*2;minor=min(40,penalty);major=min(80,2*penalty);lp,ep=(major,minor)if 2*labs+psych<=10 else(minor,major)
   l=trunc(checked(checked(l*(100-lp))+50),100);e=trunc(checked(checked(e*(100-ep))+50),100)
  # specialist list is stored economy/psych/labs, as are diagnostic counters.
  return [0,total,e,p,l,unknown,ce,cp,cl,*types,*special]
 except OverflowError:return fail(3)
def fixture(rng):
 labs=rng.randrange(11);psych=rng.randrange(11-labs);pop=rng.randrange(128);count=rng.randrange(pop+1)
 table=[]
 for i in range(7):table.extend([-1,-2,rng.randrange(7),rng.randrange(7),rng.randrange(7)])
 return [rng.choice([-20,-1,0,1,2,3,6,7,9,10,9999,10000,2147483647]),labs,psych,rng.randrange(-8,9),pop,count,5,rng.getrandbits(32),rng.getrandbits(32),*[rng.randrange(2)for _ in range(14)],7,*table,*[rng.randrange(2)for _ in range(15)],rng.randrange(513),rng.randrange(1000)]
def cases():
 rng=random.Random(0xA110CA7E);rows=[fixture(rng)for _ in range(4000)]
 # Every legal slider split at several efficiencies and rounding-sensitive totals.
 for labs in range(11):
  for psych in range(11-labs):
   for eff in[-4,0,4]:
    for total in[0,1,2,6,7,19,9999]:
     row=fixture(rng);row[:7]=[total,labs,psych,eff,5,0,5];row[59:74]=[0]*15;rows.append(row)
 for index,value in[(1,-1),(1,11),(2,11),(4,-1),(4,128),(5,-1),(5,128),(6,-1),(6,101),(9,2),(16,2),(23,6),(24,-3),(24,89),(26,-1),(74,-1),(74,513),(75,-1)]:
  row=fixture(rng);row[index]=value;rows.append(row)
 # Overflow at specialist addition, coefficients, projects, backbone and penalty.
 for kind in range(6):
  row=fixture(rng);row[:7]=[9999,5,0,0,1,1,0];row[7:9]=[0,0];row[9:23]=[1]*7+[0]*7;row[59:74]=[0]*15
  if kind==0:row[26]=2147483647
  if kind==1:row[26]=2147483646;row[60]=1
  if kind==2:row[28]=400000000;row[69]=row[70]=1
  if kind==3:row[71]=1;row[26]=1000000000
  if kind==4:row[73]=1;row[75]=2147483647
  if kind==5:row[1]=10;row[28]=40000000
  rows.append(row)
 return rows
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--binary',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.binary or a.out/'energy-allocation-probe';cmd=[]
 if not a.binary:
  cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',str(HERE/'rules_text.cpp'),str(HERE/'energy_allocation.cpp'),str(HERE/'energy_allocation_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 rows=cases();input_path=a.out/'cases.bin';input_path.write_bytes(pack([len(rows)])+b''.join(pack(row)for row in rows));wanted=b''.join(pack(expected(row))for row in rows);run=subprocess.run([str(exe),str(input_path)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert run.returncode==0,(run.returncode,run.stderr)
 if run.stdout!=wanted:
  for i,row in enumerate(rows):
   got=list(struct.unpack_from('<14I',run.stdout,56*i));want=[n&0xffffffff for n in expected(row)]
   assert got==want,(i,row,got,want)
 (a.out/'results.bin').write_bytes(run.stdout);receipt=dict(cases=len(rows),bytes=len(wanted),sha256=hashlib.sha256(wanted).hexdigest(),command=cmd,native='UNRUN',policy='Thinker pinned default restoration, not recovered original base_energy');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(rows)} energy allocation cases passed')
if __name__=='__main__':main()
