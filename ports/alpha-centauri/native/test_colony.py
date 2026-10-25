#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,subprocess
from pathlib import Path

def model():
 h=2166136261;costs=yields=growth=0
 def word(n):
  nonlocal h
  for b in (n&0xffffffff).to_bytes(4,'little'):h=((h^b)*16777619)&0xffffffff
 for n in [1,10,100]:
  for m in [1,10,100]:
   for size in range(7):
    for g in range(-5,9):
     for i in range(-5,9):
      base_n=n*(8 if size==0 else 9)//10 if size<2 else n
      base_m=m*(8 if size==0 else 9)//10 if size<2 else m
      nutrient=(base_n*(10-max(-2,min(5,g)))+9)//10
      num,den=(13,10) if i<=-3 else { -2:(6,5),-1:(11,10),0:(1,1),1:(9,10),2:(4,5),3:(7,10),4:(3,5)}.get(i,(1,2))
      mineral=(base_m*num+den-1)//den
      word(max(1,nutrient));word(max(1,mineral));costs+=1
 for economy in range(-5,7):
  for rock in range(3):
   for moisture in range(3):
    for flags in range(64):
     base=bool(flags&1);forest=bool(flags&2);river=bool(flags&4);count=1 if flags&8 else 2;hq=bool(flags&16)
     if base:
      n,m,e=2,1,1+int(hq)
      if economy>=5:e+=4
      elif economy==4:e+=2
      elif economy>=0:e+=int(economy==1)
      else:e+=economy+int(not hq or economy< -1 or count==1)
     elif forest:n,m,e=1,2,1
     else:n,m,e=(0 if rock==2 else moisture),int(rock>0),0
     e+=int(river)+int(economy>=2)
     if base:e=max(0,min(99,e))
     elif not flags&32:n,m,e=min(2,n),min(2,m),min(2,e)
     for v in [n,m,e]:word(v)
     yields+=1
 for pop in [1,2,7,127]:
  for accum in [-5,0,19,20,100000]:
   for surplus in [-5,0,2,5]:
    for factor in [1,10]:
     for rate in [-3,0,6]:
      for limit in [1,7,127]:
       for flags in range(16):
        p=pop;a=accum;recompute=append=removed=False;ceiling=-1;cost=(pop+1)*factor;allowed=pop<limit or bool(flags&1)
        if accum>=0:
         if (rate>=6 or flags&2) and surplus>=2 and allowed:
          p=min(127,pop+1);ceiling=cost;recompute=True
         elif accum>=cost and rate<=-3:a=cost
         elif accum>=cost and allowed:p=min(127,pop+1);a=0;recompute=append=True
         else:a=(cost//2 if accum>=cost else accum)+surplus
        else:
         if flags&4 and (pop>1 or not flags&8):p-=1
         if p<=0:removed=True
         else:a=surplus
        for v in [p,a,recompute,append,removed,ceiling]:word(v)
        if recompute:
         next_surplus=surplus-(p-pop)*2
         if next_surplus>=0 and a<0:a=0
         elif next_surplus<0 and a==0:a=-1
         if ceiling>=0:a=min(a,ceiling)
         if append:a+=next_surplus
         for v in [p,a,False,False,removed,-1]:word(v)
        growth+=1
 return dict(costs=str(costs),yields=str(yields),growth=str(growth),digest=f'{h:08x}',rc='0')

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 src=Path(__file__).resolve().parent;out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'colony-control';command=[a.cc,*flags,str(src/'colony.cpp'),str(src/'colony_control.cpp'),'-o',str(exe)];subprocess.run(command,check=True)
 r=subprocess.run([*runner,str(exe)],capture_output=True,text=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
 assert r.returncode==0,(r.returncode,r.stdout,r.stderr);actual=dict(v.split('=') for v in r.stdout.split());assert actual==model(),(actual,model())
 (out/'receipt.json').write_text(json.dumps(dict(command=command,runner=runner,results=actual,binarySHA256=hashlib.file_digest(exe.open('rb'),'sha256').hexdigest()),indent=2)+'\n');print(r.stdout,end='')
if __name__=='__main__':main()
