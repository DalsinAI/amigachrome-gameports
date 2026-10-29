#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Pre-specialist scoring, independent integer model, invariants and errors."""
import argparse,hashlib,json,os,random,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
def model(row):
 available,worked,pop,special,n,m,e,eff,psych,threshold,grow,riot,*raw=row
 yields=[raw[i:i+3] for i in range(0,63,3)]
 valid=lambda v: -2147483648<=v<=2147483647
 if not available&1 or available&~0x1fffff or not 0<=pop<=127 or not 0<=special<=pop or not 0<=eff<=16 or not 0<=psych<=10 or threshold<0 or min(raw)<0:return [1]
 mask=(worked&available)|1;missing=pop+1-mask.bit_count()-special;reset=missing<0 or not worked
 if reset:mask,special,missing=1,0,pop
 for i in range(21):
  if mask>>i&1:
   n,m,e=n+yields[i][0],m+yields[i][1],e+yields[i][2]
   if not all(map(valid,[n,m,e])):return [3]
 added=[]
 while missing:
  options=[]
  for i in range(1,21):
   if not available>>i&1 or mask>>i&1:continue
   if riot and eff>=12 and psych and not valid(e*psych+4):return [3]
   ev=6 if riot and eff>=12 and psych and int((e*psych+4)/10)<2 else 4
   nv=max(4+4*(n<0)+4*(n<threshold)+(6 if pop==1 and n<=0 and m>0 else 0)+max(0,5-pop)-max(0,n-1)//4,2) if grow else (8 if n<0 else 2)
   score=32*(yields[i][0]*nv+yields[i][1]*(5+5*(m<0)+4*(m<2))+(yields[i][2]*eff+15)//16*ev)-i
   if not valid(score):return [3]
   if score>0:options.append((score,-i))
  if not options:break
  i=-max(options)[1];mask|=1<<i;added.append(i);missing-=1
  n,m,e=n+yields[i][0],m+yields[i][1],e+yields[i][2]
  if not all(map(valid,[n,m,e])):return [3]
 return [0,mask,special,missing,n,m,e,len(added),int(reset),int(bool(missing or special or added)),int(bool(riot)),*added,*([0]*(20-len(added)))]
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,default=Path('/tmp/alpha-worker-allocation'));p.add_argument('--cc',default='g++');p.add_argument('--binary',type=Path);a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True);exe=a.binary or a.out/'probe'
 command=[]
 if not a.binary:
  command=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',str(HERE/'worker_allocation.cpp'),str(HERE/'worker_allocation_probe.cpp'),'-o',str(exe)];subprocess.run(command,check=True)
 randomizer=random.Random(0x68ac);rows=[]
 for _ in range(6000):
  pop=randomizer.randrange(0,30);special=randomizer.randrange(pop+1);available=randomizer.randrange(1<<21)|1
  rows.append([available,randomizer.randrange(1<<25),pop,special,randomizer.randrange(-50,60),randomizer.randrange(-10,30),randomizer.randrange(-5,70),randomizer.randrange(17),randomizer.randrange(11),randomizer.randrange(10),randomizer.randrange(2),randomizer.randrange(2),*[randomizer.randrange(9) for _ in range(63)]])
 seed=[0x1fffff,1,4,0,-8,-2,0,16,2,4,1,1]+[1,1,1]*21
 empty=seed.copy();empty[1]=0;empty[3]=3
 rows.extend([seed,seed[:7]+[0]+seed[8:],empty])
 for index,value in [(0,0),(0,0x3fffff),(2,-1),(2,128),(3,5),(7,17),(8,11),(9,-1),(12,-1),(12,2147483647),(6,2147483647),(4,2147483647)]:
  r=seed.copy();r[index]=value;rows.append(r)
 text=''.join(' '.join(map(str,r))+'\n' for r in rows).encode();(a.out/'input.txt').write_bytes(text)
 run=subprocess.run([str(exe)],input=text,capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert run.returncode==0,(run.returncode,run.stderr)
 actual=[list(map(int,line.split())) for line in run.stdout.decode().splitlines()];expected=[model(r) for r in rows];assert len(actual)==len(expected)
 for i,(got,want) in enumerate(zip(actual,expected)):assert got==want,(i,rows[i],got,want)
 receipt=dict(cases=len(rows),accepted=sum(x[0]==0 for x in actual),outputSHA256=hashlib.sha256(run.stdout).hexdigest(),inputSHA256=hashlib.sha256(text).hexdigest(),command=command,native='UNRUN',noInputMutationAndTransactionalOutput=True,preSpecialistProposalOnly=True)
 (a.out/'output.txt').write_bytes(run.stdout);(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k!='command'}))
if __name__=='__main__':main()
